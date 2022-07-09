/*
 * PTPowders.h
 * 
 * Powder Throw powder definitions
 * 
 */
#include "Arduino.h"
#include "PTConfig.h"
#include <Adafruit_FRAM_I2C.h>
#include "PTUtility.h"
#include "PTPresets.h"
#include <ArduinoBLE.h>

#ifndef PTPOWDER_H
#define PTPOWDER_H 

#define POWDERS_VERSION 10002
//0 base, 25 powders
#define MAX_POWDERS 24
#define POWDERS_ADDR_BASE  PRESETS_ADDR_BASE + (PRESET_DATA_SIZE * MAX_PRESETS)
#define MAX_POWDER_FACTOR 0.2

/* Type def for PowderData.  Data structure for a preset in the list. */
typedef struct _powder_data_t {
  int powder_version;
  int powder_number;
  float powder_factor;
  char powder_name[NAME_LEN+1];
  char powder_lot[NAME_LEN+1];
} PowderData;
#define POWDER_DATA_SIZE sizeof(_powder_data_t)

/* Type def for PowderDataStorage.  Union for raw data storage read/write. */
typedef union _powder_data_storage_t {
  PowderData _powder_data;
  byte raw_data[POWDER_DATA_SIZE];
} PowderDataStorage;

class PowderManager
{
  public:
    PowderManager();

    // Vars

    // Getters/Setters
    int getCurrentPowder();
    int getPowderVersion();
    bool getPowderName(char*);
    bool setPowderName(char*);
    bool getPowderLot(char*);
    bool setPowderLot(char*);
    float getPowderFactor();
    void setPowderFactor(float);
    void incPowderFactor(int);
    void decPowderFactor(int);
    
    // Actions
    bool init(Adafruit_FRAM_I2C, int=0);
    bool loadPowder(int);
    bool savePowder(bool = false);
    void incNameChar(int);
    void decNameChar(int);
    bool resetBuffer();
    bool isDirty();
    bool isPowderDefined();
    
    // BLE support
    bool getBLEDataStruct(byte[], int);
    bool getDefaults(byte[], int);

  private:

    // Vars
    Adafruit_FRAM_I2C _fram;
    PowderDataStorage _powder_buffer;  // storage read/write buffer
    PowderDataStorage _defaults = { POWDERS_VERSION, 0, 0.0, "EMPTY", "--              " };
    int _cur_powder;
    bool _dirty;
    char _error_buff[100];
    
    // Functions
    boolean _writePowderData(int);
    boolean _readPowderData(byte[], int);
};

#endif
