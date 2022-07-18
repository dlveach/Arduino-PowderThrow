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
#include <LiquidCrystal_PCF8574.h>

#define NAME_LEN 16 //avoiding cirlular dep. Dup defs in PTPresets.h and PTPowders.h  TODO: move to top level .h?

#define FRAM_SIZE 32768  //max 32K Fram address
#define CONFIG_VERSION 10007  // unique version ID
#define CONFIG_DATA_ADDR 0x0  //base memory location of config data

//Config Defaults
#define DEFAULT_PRESET 0
#define DEFAULT_FCURVEP -1.5
#define DEFULT_DECEL_THRESHOLD 1.0
#define DEFAULT_BUMP_THRESHOLD 0.10
#define DEFAULT_DECEL_LIMIT 4000
#define DEFAULT_SCALE_TOLERANCE 0.021
#define DEFAULT_TRICKLER_SPEED 3000         //pulses per sec (1/8 micro step)

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
  int trickler_speed;
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

typedef struct ladder_data_t {
  bool is_configured;     // true if ladder is configured
  int step_count;         // steps in the ladder
  float start_weight;     // start target weight
  float step_interval;    // target weight interval per step      
} LadderData;

class PTConfig {

  public:
    enum run_mode_t {
      pt_auto,
      pt_manual,
      pt_ladder,
    };
  
    // Constructor
    PTConfig();

    // Vars
    LadderData ladder_data;

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
    int getTricklerSpeed();
    void setTricklerSpeed(int);
    int getPreset();
    void setPreset(int);
    bool isDirty();
    bool isBLEUpdateNeeded();

    // Indirect (copied preset/powder) data access 
    //TODO: eval code for inconsistency across g_scale, g_presets, g_powders
    char* getPresetName();
    void setPresetName(char*);
    float getTargetWeight();      
    void setTargetWeight(float);  
    char* getPowderName();
    void setPowderName(char*);
    float getKernelFactor();
    void setKernelFactor(float);  
    
    // Actors
    boolean init(Adafruit_FRAM_I2C, LiquidCrystal_PCF8574);
    boolean resetConfig();  // reload data buffer from FRAM
    boolean loadConfig();
    boolean saveConfig(boolean = false);
    boolean validateData();
    void printConfig();
    bool updateBLE(BLECharacteristic);
    bool isRunReady();
    run_mode_t getRunMode();
    void setRunMode(run_mode_t);
//    bool isManualMode();
//    void setManualMode(bool);
//    bool isLadderMode();
//    void setLadderMode(bool);
    int getLadderStepCount();
    void setLadderStepCount(int);
    float getLadderStartWeight();
    void setLadderStartWeight(float);
    float getLadderStepInterval();
    void setLadderStepInterval(float);

  private:
    // Vars
    ConfigDataStorage _config_buffer;  // FRAM storage read/write buffer
    ConfigDataStorage _defaults = {
      DEFAULT_PRESET, 
      DEFAULT_FCURVEP, 
      DEFULT_DECEL_THRESHOLD, 
      DEFAULT_BUMP_THRESHOLD, 
      DEFAULT_DECEL_LIMIT, 
      DEFAULT_SCALE_TOLERANCE, 
      DEFAULT_TRICKLER_SPEED, 
      CONFIG_VERSION
    };
    Adafruit_FRAM_I2C _fram;        // local ref to global Adafruit_FRAM_I2C object
    bool _dirty;                    // config data changed, needs saving
    bool _updateBLE;                // config data changed, update BLE
    bool _version_reset;            // flag, if config version was reset on init

    //Runtime data from current preset/powder
    char _preset_name[NAME_LEN+1];  // from current preset
    float _target_weight;           // from current preset
    char _powder_name[NAME_LEN+1];  // from current preset -> powder
    float _kernel_factor;           // from current preset -> powder

    //mode stuffs
    run_mode_t _run_mode;           // Current run mode
//    bool _manual_mode;              // "manual" run mode
//    bool _ladder_mode;              // "ladder" run mode
//    int _ladder_step_count;         // steps in the ladder
//    float _ladder_start_weight;     // start target weight
//    float _ladder_step_interval;    // target weight interval per step
    
    // Functions
    boolean _writeConfigData ();
    boolean _readConfigData ();
    boolean _validateData ();       // Set internal data from current preset/powder  TODO: rethink this duplication
};

#endif //PTConfig_h
