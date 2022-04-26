/*
 * PTConfig.cpp - PowderThrow config implementation.
 * Created by David Veach.  2022
 * Private repository
 */
#include "Arduino.h"
#include "PTConfig.h"
#include "PTUtility.h"

/*
 * Constructor
 */
PTConfig::PTConfig() 
{
  _config_buffer = _defaults;
  _kernel_factor = 0.0200; // gn of Varget per kernel TODO: move to FRAM for powder definitions
  _dirty = false;
}

/*
 * Initialize the config object.  
 * Connects to FRAM, loads current config from storage.
 * Returns true if successful, false if not.
 * Will set a system error.
 */
bool PTConfig::init(Adafruit_FRAM_I2C fram)
{
  _fram = fram;
  if (!loadConfig())
  {
    util_handleSystemError(F("Can't load config."));
    return (false);
  }
  return (true);
}

/*
 * 
 */
bool PTConfig::isDirty()
{
  return (_dirty);
}

/*
 * Return config version.
 */
int PTConfig::getVersion()
{
  return (_config_buffer._config_data.config_version);
}

/*
 * 
 */
float PTConfig::getKernelFactor()
{
  return (_kernel_factor); 
}

/*
 *
 */
int PTConfig::getPreset()
{
  return (_config_buffer._config_data.preset);
}

/*
 * 
 */
void PTConfig::setPreset(int value)
{
  _config_buffer._config_data.preset = value;
  _dirty = true;
}

/*
 * 
 */
float PTConfig::getFcurveP()
{
  return (_config_buffer._config_data.fscaleP);  
}

/*
 * 
 */
void PTConfig::setFcurveP(float value)
{
  _config_buffer._config_data.fscaleP = value;
  _dirty = true;
}

/*
 * 
 */
float PTConfig::getDecelThreshold()
{
  return (_config_buffer._config_data.decel_threshold);
}

/*
 * 
 */
void PTConfig::setDecelThreshold(float value)
{
  _config_buffer._config_data.decel_threshold = value;
  _dirty = true;
}

/*
 * 
 */
float PTConfig::getBumpThreshold()
{
  return (_config_buffer._config_data.bump_threshold);
}

/*
 * 
 */
void PTConfig::setBumpThreshold(float value)
{
  _config_buffer._config_data.bump_threshold = value;
  _dirty = true;
}

/*
 * 
 */
int PTConfig::getDecelLimit()
{
  return (_config_buffer._config_data.decel_limit);
}

/*
 * 
 */
void PTConfig::setDecelLimit(int value)
{
  _config_buffer._config_data.decel_limit = value;
  _dirty = true;
}

/*
 * 
 */
float PTConfig::getGnTolerance()
{
  return (_config_buffer._config_data.gn_tolerance);
}

/*
 * 
 */
void PTConfig::setGnTolerance(float value)
{
  _config_buffer._config_data.gn_tolerance = value;  
  _dirty = true;
}

/*
 * 
 */
float PTConfig::getMgTolerance()
{
  return (_config_buffer._config_data.mg_tolerance);  
}

/*
 * 
 */
void PTConfig::setMgTolerance(float value)
{
  _config_buffer._config_data.mg_tolerance = value;  
  _dirty = true;
}

/*
 * Reset system config to current config buffer.
 */
boolean PTConfig::resetConfig()
{
  DEBUGLN(F("resetConfig(): reading FRAM storage for config."));
  if (!_readConfigData()) 
  {
    DEBUGLN(F("resetConfig(): ERROR: could not read FRAM storage."));
    return (false);
  }  
  DEBUGLN(F("resetConfig(): reset settings to last saved values."));
  _dirty = false;
  return (true);
}

/*
 * Load config buffer from FRAM and set system config.
 */
boolean PTConfig::loadConfig()
{
  DEBUGLN(F("loadConfig(): reading FRAM storage for config."));
  if (!_readConfigData()) 
  {
    DEBUGLN(F("loadConfig(): ERROR: could not read FRAM storage."));
    return (false);
  }
  else
  {
    if (_config_buffer._config_data.config_version == CONFIG_VERSION) 
    {
      DEBUGLN(F("loadConfig(): Loaded settings from FRAM storage."));    
    }
    else
    {
      DEBUGLN(F("loadConfig(): Config version out of sync, set to defaults."));    
      saveConfig(true);
    }
  }
  _dirty = false;
  return (true);
}

/*
 * Copy current system config to config buffer and save to storage.
 * Returns true if successful, false if not.
 */
boolean PTConfig::saveConfig(boolean init)
{
  if (init)
  {
    DEBUGLN(F("saveSettings(): Initializing config to defaults."));
    _config_buffer = _defaults;
  }
  if (!_dirty) { return (true); } //no changes to save
  DEBUGLN(F("saveSettings(): Saving config settings to FRAM storage."));
  if (_writeConfigData())
  {
    _dirty = false;
    return (true);
  }
  return (false);
}

/*
 * Writes the config buffer to FRAM.
 */
boolean PTConfig::_writeConfigData ()
{
  uint16_t addr = CONFIG_DATA_ADDR;
  if ((addr + CONFIG_DATA_SIZE-1) > FRAM_SIZE) return (false); //overflow
  for (int i=0; i<CONFIG_DATA_SIZE; i++) _fram.write8(addr + i, _config_buffer.raw_data[i]);
  return (true); 
}

/*
 * Reads the FRAM into the config buffer.
 * Returns true if successful, false if not.
 */
boolean PTConfig::_readConfigData ()
{
  uint16_t addr = CONFIG_DATA_ADDR;
  if ((addr + CONFIG_DATA_SIZE-1) > FRAM_SIZE) 
  {
    DEBUGLN(F("FATAL: _readConfigData(): FRAM storage address overflow."));
    return (false); //overflow
  }
  for (int i=0; i<CONFIG_DATA_SIZE; i++)  _config_buffer.raw_data[i] = _fram.read8(addr + i);
  return (true);
}

/*
 * Debug helper printConfig().  
 * Dump current config buffer to Serial.
 */
void PTConfig::printConfig() {
  char buff[21];
  int bsize = sizeof(buff);
  snprintf(buff, bsize, "Cfg Ver: %-5d", _config_buffer._config_data.config_version);
  DEBUGLN(buff);  
  snprintf(buff, bsize, "Chrg Tgt: %-6.2f", _target_weight);
  DEBUGLN(buff);  
  snprintf(buff, bsize, "Dec Thresh: %4.2f", _config_buffer._config_data.decel_threshold);
  DEBUGLN(buff);  
  snprintf(buff, bsize, "Dec Lim: %-5d", _config_buffer._config_data.decel_limit);
  DEBUGLN(buff);  
  snprintf(buff, bsize, "Bmp Thresh: %4.2f", _config_buffer._config_data.bump_threshold);
  DEBUGLN(buff);  
  snprintf(buff, bsize, "fScale P: %-5.2f", _config_buffer._config_data.fscaleP);
  DEBUGLN(buff);  
  snprintf(buff, bsize, "GN Tol: %-5.3f", _config_buffer._config_data.gn_tolerance);
  DEBUGLN(buff);
  snprintf(buff, bsize, "MG Tol: %-5.3f", _config_buffer._config_data.mg_tolerance);
  DEBUGLN(buff);  
  snprintf(buff, bsize, "Preset: %-2d", _config_buffer._config_data.preset);
  DEBUGLN(buff);
  snprintf(buff, bsize, "Cfg data size: %d", CONFIG_DATA_SIZE);
  DEBUGLN(buff);
  
}
