/*
 * PTPresets.h
 * 
 * Powder Throw Presets
 * 
 */
#include "Arduino.h"
#include "PTConfig.h"
#include <Adafruit_FRAM_I2C.h>
#include "PTUtility.h"

#ifndef PTPRESETS_H
#define PTPRESETS_H 

#define PRESETS_VERSION 10001
//0 base, 50 presets
#define MAX_PRESETS 49 
#define PRESETS_ADDR_BASE CONFIG_DATA_SIZE + 8
#define PRESET_NAME_LEN 16

/*
 * Type def for PresetData.  
 * Data structure for a preset in the list.
 */
typedef struct _preset_data_t {
  float charge_weight;    //grains
  int powder_index;       //index into powder list
  char preset_name[PRESET_NAME_LEN+1];   
  int preset_version;
} PresetData;
#define PRESET_DATA_SIZE sizeof(_preset_data_t)

/*
 * Type def for PresetDataStorage
 * Union for raw data storage read/write.
 */
typedef union _preset_data_storage_t {
  PresetData _preset_data;
  byte raw_data[PRESET_DATA_SIZE];
} PresetDataStorage;


/*
 * Class PresetManager
 */
class PresetManager
{
  public:

    // Constructor
    PresetManager();

    // Vars

    // Getters/Setters
    int getCurrentPreset();
    int getPresetVersion();
    float getPresetChargeWeight();
    void setPresetChargeWeight(float);
    int getPowderIndex();
    void setPowderIndex(int);
    bool getPresetName(char*);
    bool setPresetName(char*);
    
    // Actions
    bool init(Adafruit_FRAM_I2C, PTConfig);
    void incNameChar(int);
    void decNameChar(int);
    bool loadPreset(int);
    bool savePreset(bool = false);
    bool isDirty();
    bool resetCurrentPreset();
    bool isDefined();
      
  private:

    // Vars
    PTConfig _config;
    Adafruit_FRAM_I2C _fram;
    PresetDataStorage _preset_buffer;  // storage read/write buffer
    PresetDataStorage _defaults = { 0.0, -1, "EMPTY", PRESETS_VERSION};
    boolean _dirty;
    int _cur_preset;
    char _error_buff[100];
    
    // Functions
    boolean _writePresetData();
    boolean _readPresetData();
};

#endif
