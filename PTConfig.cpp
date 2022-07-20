/*
 * PTConfig.cpp - PowderThrow config implementation.
 * Created by David Veach.  2022
 * Private repository
 */
#include "Arduino.h"
#include "PTConfig.h"
#include "PTUtility.h"

PTConfig::PTConfig() {
  _config_buffer = _defaults;
  _dirty = false;
  _preset_name[0] = 0x00; // from current preset
  _target_weight = -1;    // from current preset
  _powder_name[0] = 0x00; // from current preset -> powder
  _kernel_factor = -1;    // from current preset -> powder
}

/* Initialize the config object.  
 * Connects to FRAM, loads current config from storage.
 * Returns true if successful, false if not. */
bool PTConfig::init(Adafruit_FRAM_I2C fram, LiquidCrystal_PCF8574 lcd) {
  _fram = fram;
//  _version_reset = false;
//  _ladder_mode = false;
  _run_mode = pt_auto;
//  _ladder_step_count = 0;   
//  _ladder_start_weight = 0.0; 
//  _ladder_step_interval = 0.0;  
/*
  this->ladder_data.is_configured = false;
  this->ladder_data.step_count = 0;
  this->ladder_data.start_weight = 0.0; 
  this->ladder_data.step_interval = 0.0;  
*/
  ladder_data = {false, 0, 0.0, 0.0}; 

  if (!loadConfig()) {
    util_handleSystemError(F("Can't load config."));
    _updateBLE = false;
    return (false);
  }
  if (_version_reset) {
    lcd.setCursor(0,2);
    lcd.print(F("FRAM resetting ...  "));
    util_eraseFRAM(fram);
    saveConfig(true);  //initialize new config to defaults (TODO: not working, why?)
  }
  _updateBLE = true;
  return (true);
}

bool PTConfig::isRunReady() { return ((_target_weight > 0) && (_kernel_factor > 0)); }

bool PTConfig::isBLEUpdateNeeded() { return (_updateBLE); }

bool PTConfig::isDirty() { return (_dirty); }

int PTConfig::getVersion() { return (_config_buffer._config_data.config_version); }

int PTConfig::getPreset() {  return (_config_buffer._config_data.preset); }

void PTConfig::setPreset(int value) {
  _config_buffer._config_data.preset = value;
  _dirty = true;
  _updateBLE = true;
}

float PTConfig::getFcurveP() {  return (_config_buffer._config_data.fscaleP);  }

void PTConfig::setFcurveP(float value) {
  _config_buffer._config_data.fscaleP = value;
  _dirty = true;
  _updateBLE = true;
}

float PTConfig::getDecelThreshold() {  return (_config_buffer._config_data.decel_threshold);  }

void PTConfig::setDecelThreshold(float value) {
  _config_buffer._config_data.decel_threshold = value;
  _dirty = true;
  _updateBLE = true;
}

float PTConfig::getBumpThreshold() {  return (_config_buffer._config_data.bump_threshold); }

void PTConfig::setBumpThreshold(float value) {
  _config_buffer._config_data.bump_threshold = value;
  _dirty = true;
  _updateBLE = true;
}

int PTConfig::getDecelLimit() {  return (_config_buffer._config_data.decel_limit); }

void PTConfig::setDecelLimit(int value) {
  _config_buffer._config_data.decel_limit = value;
  _dirty = true;
  _updateBLE = true;
}

float PTConfig::getGnTolerance() { return (_config_buffer._config_data.gn_tolerance); }

void PTConfig::setGnTolerance(float value) {
  _config_buffer._config_data.gn_tolerance = value;  
  _dirty = true;
  _updateBLE = true;
}

int PTConfig::getTricklerSpeed() {
  return (_config_buffer._config_data.trickler_speed);  
}

void PTConfig::setTricklerSpeed(int value) {
  _config_buffer._config_data.trickler_speed = value;  
  _dirty = true;
  _updateBLE = true;
} 

char* PTConfig::getPresetName() { return (_preset_name); }

void PTConfig::setPresetName(char* buff) {
  strncpy(_preset_name, buff, NAME_LEN);
  _preset_name[NAME_LEN]=0;
}

char* PTConfig::getPowderName() { return (_powder_name); }

void PTConfig::setPowderName(char* buff) {
  strncpy(_powder_name, buff, NAME_LEN);
  _powder_name[NAME_LEN]=0;
}

float PTConfig::getKernelFactor() { return (_kernel_factor); }

void PTConfig::setKernelFactor(float value) { _kernel_factor = value; }

void PTConfig::setTargetWeight(float value) { _target_weight = value; }

float PTConfig::getTargetWeight() { return (_target_weight); }

void PTConfig::setRunMode(PTConfig::run_mode_t run_mode) { 
  _run_mode = run_mode; 
  if (_run_mode == pt_ladder) {
    sprintf(_preset_name, "Step:%02d/%02d", ladder_data.current_step, ladder_data.step_count);
    _target_weight = ladder_data.start_weight + (ladder_data.step_interval * (ladder_data.current_step - 1));
  }
}

PTConfig::run_mode_t PTConfig::getRunMode() { return(_run_mode); }

/* Reset system config to current config buffer.  */
boolean PTConfig::resetConfig() {
  if (!_readConfigData())  {
    DEBUGLN(F("resetConfig(): ERROR: could not read FRAM storage."));
    return (false);
  }  
  DEBUGLN(F("resetConfig(): reset settings to last saved values."));
  _dirty = false;
  return (true);
}

/* Load config buffer from FRAM and set system config. */
boolean PTConfig::loadConfig() {
  DEBUGLN(F("loadConfig(): reading FRAM storage for config."));
  if (!_readConfigData())  {
    DEBUGLN(F("loadConfig(): ERROR: could not read FRAM storage."));
    return (false);
  } else {
    if (_config_buffer._config_data.config_version != CONFIG_VERSION)  {
      DEBUGLN(F("loadConfig(): Config version out of sync, set to defaults."));    
      saveConfig(true);
      _version_reset = true;
    }
  }
  _dirty = false;
  return (true);
}

/* Copy current system config to config buffer and save to storage.
 * Returns true if successful, false if not. */
boolean PTConfig::saveConfig(boolean init) {
 if (init) {
    DEBUGLN(F("saveSettings(): Initializing config to defaults."));
    _config_buffer = _defaults;
  }
  if (!_dirty) { return (true); } //no changes to save
  DEBUGLN(F("saveSettings(): Saving config settings to FRAM storage."));
  if (_writeConfigData()) {
    _dirty = false;
    return (true);
  }
  return (false);
}

// TODO: move this to PTBLE.ino?
 bool PTConfig::updateBLE(BLECharacteristic BLEChar) {
   if (BLEChar.writeValue(_config_buffer.raw_data, CONFIG_DATA_SIZE)) {
     _updateBLE = false;
     return (true);
   }
   return (false);
 }

/*******************
 * PRIVATE
 ******************/
 
/* Writes the config buffer to FRAM. */
boolean PTConfig::_writeConfigData () {
  uint16_t addr = CONFIG_DATA_ADDR;
  if ((addr + CONFIG_DATA_SIZE-1) > FRAM_SIZE) return (false); //overflow
  for (int i=0; i<CONFIG_DATA_SIZE; i++) _fram.write8(addr + i, _config_buffer.raw_data[i]);
  return (true); 
}

/* Reads the FRAM into the config buffer.
 * Returns true if successful, false if not. */
boolean PTConfig::_readConfigData () {
  uint16_t addr = CONFIG_DATA_ADDR;
  if ((addr + CONFIG_DATA_SIZE-1) > FRAM_SIZE) {
    DEBUGLN(F("FATAL: _readConfigData(): FRAM storage address overflow."));
    return (false); //overflow
  }
  for (int i=0; i<CONFIG_DATA_SIZE; i++)  _config_buffer.raw_data[i] = _fram.read8(addr + i);
  return (true);
}

/*********************
 * DEBUG / TEST
 *********************/

/* Debug helper printConfig().  Dump current config buffer to Serial. */
void PTConfig::printConfig() {
  char buff[80];
  int bsize = sizeof(buff);
  DEBUGLN(F("Stored (FRAM) data:"));
  sprintf(buff, "Config Version: %-5d", _config_buffer._config_data.config_version);
  DEBUGLN(buff);  
  sprintf(buff, "Config Decel Threshold: %4.2f", _config_buffer._config_data.decel_threshold);
  DEBUGLN(buff);  
  sprintf(buff, "Config Decel Limit: %-5d", _config_buffer._config_data.decel_limit);
  DEBUGLN(buff);  
  sprintf(buff, "Config Bump Threshold: %4.2f", _config_buffer._config_data.bump_threshold);
  DEBUGLN(buff);  
  sprintf(buff, "Config Fscale P: %-5.2f", _config_buffer._config_data.fscaleP);
  DEBUGLN(buff);  
  sprintf(buff, "Config GN Tol: %-5.3f", _config_buffer._config_data.gn_tolerance);
  DEBUGLN(buff);
  sprintf(buff, "Config Trickler Speed: %-5d", _config_buffer._config_data.trickler_speed);
  DEBUGLN(buff);  
  sprintf(buff, "Config Preset Index: %-2d", _config_buffer._config_data.preset);
  DEBUGLN(buff);
  sprintf(buff, "Config stored data size: %d", CONFIG_DATA_SIZE);
  DEBUGLN(buff);
  DEBUGLN(F("Indirect (from preset/powder) data:"));
  sprintf(buff, "Preset Name: %s", _preset_name);
  DEBUGLN(buff);
  sprintf(buff, "Preset Target Weight: %-6.2f gn", _target_weight);
  DEBUGLN(buff);  
  sprintf(buff, "Powder Name: %s", _powder_name);
  DEBUGLN(buff);
  sprintf(buff, "Powder Kernel Factor: %-8.6f", _kernel_factor);
  DEBUGLN(buff);
}
