/*
 * PTConfig.h - PowderThrow config implementation.
 * Created by David Veach.  2022
 * Private repository
 */

#ifndef PTConfig_h
#define PTConfig_h

#include "Arduino.h"
#include <Adafruit_FRAM_I2C.h>

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

    // Getters/Setters
    int getVersion();
    //float getTargetWeight();
    //void setTargetWeight(float);
    float getDecelThreshold();
    void setDecelThreshold(float);
    float getBumpThreshold();
    void setBumpThreshold(float);
    int getDecelLimit();
    void setDecelLimit(int);
    float getFcurveP();
    void setFcurveP(float);
    float getKernelFactor();
    void setKernelFactor(float);
    float getGnTolerance();
    void setGnTolerance(float);
    float getMgTolerance();
    void setMgTolerance(float);
    int getPreset();
    void setPreset(int);
    bool isDirty();

    // Actors
    boolean init(Adafruit_FRAM_I2C fram);
    boolean resetConfig();
    boolean loadConfig();
    boolean saveConfig(boolean = false);
    void printConfig();
    //void eraseFRAM();
    
  private:
    // Vars
    ConfigDataStorage _config_buffer;  // storage read/write buffer
    ConfigDataStorage _defaults = {0, -1.5, 1.0, 0.10, 4000, 0.021, 0.002, CONFIG_VERSION};
    Adafruit_FRAM_I2C _fram;
    int _preset;  // current preset
    //int _powder   // from current preset
    float _target_weight;   // from current preset
    long _kernel_factor;    // from current preset -> powder
    bool _dirty;    // config data changed, needs saving
    
    // Functions
    boolean _writeConfigData ();
    boolean _readConfigData ();
};

#endif //PTConfig_h
