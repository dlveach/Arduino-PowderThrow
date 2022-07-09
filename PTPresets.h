/*
 * PTPresets.h
 * 
 * Powder Throw Presets
 * 
 */
#include "Arduino.h"
#include <Adafruit_FRAM_I2C.h>
#include "PTUtility.h"
#include "ArduinoBLE.h"
#include "PTConfig.h"

#ifndef PTPRESETS_H
#define PTPRESETS_H 

#define PRESETS_VERSION 10003
//0 base, 50 presets
#define MAX_PRESETS 24
#define PRESETS_ADDR_BASE CONFIG_DATA_SIZE + 8
#define NAME_LEN 16

/*
 * Type def for PresetData.  
 * Data structure for a preset in the list.
 */
typedef struct _preset_data_t {
  int preset_version;             //version of this structure
  int preset_number;              //preset number (1 based)
  float charge_weight;            //powder grains (-1 if preset "empty")
  int powder_index;               //index into powder list
  int bullet_weight;              //bullet grains
  char preset_name[NAME_LEN+1];   //name of preset (load)
  char bullet_name[NAME_LEN+1];   //name of bullet
  char brass_name[NAME_LEN+1];    //name of brass
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
    char* getBulletName();
    bool setBulletName(char*);
    int getBulletWeight();
    void setBulletWeight(int);
    char* getBrassName();
    bool setBrassName(char*);
    
    // Actions
    bool init(Adafruit_FRAM_I2C, int);
    void incPresetChargeWeight(int);
    void decPresetChargeWeight(int);
    void incPresetNameChar(int);
    void decPresetNameChar(int);
    void incBulletNameChar(int);
    void decBulletNameChar(int);
    void incBrassNameChar(int);
    void decBrassNameChar(int);
    void incBulletWeight();
    void decBulletWeight();
    bool loadPreset(int);
    bool savePreset(bool = false);
    bool isDirty();
    bool resetCurrentPreset();
    bool isDefined();

    // BLE support
    bool getBLEDataStruct(byte[], int);
    bool getDefaults(byte[], int);
      
  private:

    // Vars
    Adafruit_FRAM_I2C _fram;
    PresetDataStorage _preset_buffer;  // storage read/write buffer
    PresetDataStorage _defaults = { PRESETS_VERSION, 0, 0.0, -1, 0, "EMPTY           ", "--              ", "--              "};
    boolean _dirty;
    int _cur_preset;
    char _error_buff[100];  //TODO: is this used?
    
    // Functions
    boolean _writePresetData(int);
    boolean _readPresetData(byte[], int);
};

#endif
