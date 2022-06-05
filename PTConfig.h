/*
 * PTConfig.h - PowderThrow config implementation.
 * Created by David Veach.  2022
 * Private repository
 */

#ifndef PTConfig_h
#define PTConfig_h

#include "Arduino.h"
#include <Adafruit_FRAM_I2C.h>
#include <ArduinoBLE.h>

#define NAME_LEN 16 //avoiding cirlular dep. Dup defs in PTPresets.h and PTPowders.h  TODO: move to top level .h?

#define FRAM_SIZE 32768  //max 32K Fram address
#define CONFIG_VERSION 10003  // unique version ID
#define CONFIG_DATA_ADDR 0x0  //base memory location of config data

/*
 * Type def for ConfigData.  
 * Config data structure.
 */
typedef struct _config_data_t {
  int preset;
  float fscaleP;          
  float decel_threshold;  
  float bump_threshold;
  int decel_limit;
  float gn_tolerance;
  float mg_tolerance;
  int config_version; 
} ConfigData;
#define CONFIG_DATA_SIZE sizeof(_config_data_t)

/*
 * Type def for ConfigDataStorage
 * Union for raw data storage read/write.
 */
typedef union _config_data_storage_t {
  ConfigData _config_data;
  byte raw_data[CONFIG_DATA_SIZE];
} ConfigDataStorage;

/*
 * Class PTConfig
 */
class PTConfig
{
  public:
    // Constructor
    PTConfig();

    // Vars

    // Getters/Setters for FRAM stored data
    int getVersion();
    float getDecelThreshold();
    void setDecelThreshold(float);
    float getBumpThreshold();
    void setBumpThreshold(float);
    int getDecelLimit();
    void setDecelLimit(int);
    float getFcurveP();
    void setFcurveP(float);
    float getGnTolerance();
    void setGnTolerance(float);
    //float getMgTolerance();
    //void setMgTolerance(float);
    int getPreset();
    void setPreset(int);
    bool isDirty();
    bool isBLEUpdateNeeded();

    // Indirect (copied preset/powder) data access 
    char* getPresetName();
    void setPresetName(char*);
    float getTargetWeight();
    void setTargetWeight(float);
    char* getPowderName();
    void setPowderName(char*);
    float getKernelFactor();
    void setKernelFactor(float);  
    
    // Actors
    boolean init(Adafruit_FRAM_I2C fram);
    boolean resetConfig();  // reload data buffer from FRAM
    boolean loadConfig();
    boolean saveConfig(boolean = false);
    boolean validateData();
    void printConfig();
    bool updateBLE(BLECharacteristic);
    
  private:
    // Vars
    ConfigDataStorage _config_buffer;  // storage read/write buffer
    ConfigDataStorage _defaults = {0, -1.5, 1.0, 0.10, 4000, 0.021, 0.002, CONFIG_VERSION};
    Adafruit_FRAM_I2C _fram;
    bool _dirty;    // config data changed, needs saving
    bool _updateBLE;  // config data changed, update BLE

    //Internal data from current preset/powder
    int _preset;            // current preset index
    char _preset_name[NAME_LEN+1];     // from current preset
    float _target_weight;   // from current preset
    char _powder_name[NAME_LEN+1];     // from current preset -> powder
    float _kernel_factor;    // from current preset -> powder
    
    // Functions
    boolean _writeConfigData ();
    boolean _readConfigData ();
    boolean _validateData ();  // Set internal data from current preset/powder
};

#endif //PTConfig_h
