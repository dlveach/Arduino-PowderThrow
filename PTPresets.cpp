/*
 * PTPresets.cpp - PowderThrow presets implementation.
 * Created by David Veach.  2022
 * Private repository
 */
#include "Arduino.h"
#include "PTPresets.h"
#include "PTUtility.h"


/*
 * Constructor
 */
PresetManager::PresetManager() 
{
 //TODO: anything?
}

/*
 * Initialize the config object.  
 * Connects to FRAM, loads current config from storage.
 * Returns true if successful, false if not.
 * Will set a system error.
 */
bool PresetManager::init(Adafruit_FRAM_I2C fram, PTConfig cfg)
{
  _fram = fram;
  _config = cfg;
  int i = _config.getPreset();
  if (i < 0) { i = 0; }
  return (loadPreset(i));
}

bool PresetManager::isDefined()
{
  bool defined = ((_preset_buffer._preset_data.charge_weight > 0) && (_preset_buffer._preset_data.powder_index >= 0));
  return(defined);
}

/*
 * Return index of current preset in manager.
 */
int PresetManager::getCurrentPreset()
{
  return (_cur_preset);  
}

/*
 * Return config version.
 */
int PresetManager::getPresetVersion()
{
  return (_preset_buffer._preset_data.preset_version);
}

/*
 * Return current loaded buffer's charge weight (grains).
 */
float PresetManager::getPresetChargeWeight()
{
  return (_preset_buffer._preset_data.charge_weight);
}

/*
 * Set current loaded buffer's charge weight (grains).
 */
void PresetManager::setPresetChargeWeight(float value)
{
  _preset_buffer._preset_data.charge_weight = value;
  _dirty = true;
}

/*
 * Get current loaded buffer's powder index.
 */
int PresetManager::getPowderIndex()
{
  return (_preset_buffer._preset_data.powder_index);
}

/*
 * Set current loaded buffer's powder index.
 */
void PresetManager::setPowderIndex(int value)
{
  _preset_buffer._preset_data.powder_index = value;
  _dirty = true;
}

/*
 * Copy current loaded buffer's preset name into buff.
 * ERROR and return false if length of preset name too long.
 */
bool PresetManager::getPresetName(char* buff)
{
  //TODO: does this need a safety check?. 
  strcpy(buff, _preset_buffer._preset_data.preset_name);
  return (true);
}

/*
 * Copy PRESET_NAME_LEN chars from buff to loaded buffer's name.
 * ERROR and return false if length of buff too long.
 */
bool PresetManager::setPresetName(char* buff)
{
  if (strlen(buff) > PRESET_NAME_LEN)
  {
    DEBUGP(F("ERROR: string too long. "));
    DEBUGLN(__LINE__);
    util_handleSystemError("ERR: str too long");
    return (false);
  }
  strcpy(_preset_buffer._preset_data.preset_name, buff);
  _dirty = true;
  return (true);  
}

/*
 * Increment preset name character at index i. 
 * Limited to A-Z, 0-9 and space
 */
void PresetManager::incNameChar(int i)
{
  char c = _preset_buffer._preset_data.preset_name[i];
  if ((c < ' ') || (c > 'Z')) { c = 'A'; }
  else if (c == 'Z') { c = ' '; }
  else if (c == ' ') { c = '0'; }
  else if (c == '9') { c = 'A'; }
  else { c++; }
  _preset_buffer._preset_data.preset_name[i] = c;
  _dirty = true;
}

/*
 * Decrement preset name character at index i.
 * Limited to A-Z, 0-9 and space
 */
void PresetManager::decNameChar(int i)
{
  char c = _preset_buffer._preset_data.preset_name[i];
  if ((c < ' ') || (c > 'Z')) { c = 'A'; }
  else if (c == ' ') { c = 'Z'; }
  else if (c == 'A') { c = '9'; }
  else if (c == '0') { c = ' '; }
  else { c--; }
  _preset_buffer._preset_data.preset_name[i] = c;
  _dirty = true;
}

/*
 * Return true if preset buffer data changed, falase if not.
 */
bool PresetManager::isDirty()
{
  return (_dirty);
}

/*
 * Load the preset buffer from an indexed location in FRAM.
 * Param index: the preset index in the list (0 base). Default is _cur_preset.
 * If the preset data is out of sync, it is intialized to default and saved.
 * Returns true if successful, false if not.
 */ 
boolean PresetManager::loadPreset(int index)
{
  if ((index < 0) || (index > MAX_PRESETS))
  {
    DEBUGLN(F("ERROR: loadPreset(): Preset index out of range."));
    return (false); 
  }
  _cur_preset = index;
  DEBUGLN(F("loadPreset(): reading FRAM storage for preset."));
  if (!_readPresetData()) 
  {
    DEBUGLN(F("loadPreset(): ERROR: could not read FRAM storage."));
    return (false);
  }
  else
  {
    if (_preset_buffer._preset_data.preset_version == PRESETS_VERSION) 
    {
      DEBUGLN(F("loadPreset(): Loaded preset from FRAM storage."));    
    }
    else
    {
      DEBUGP(F("loadPreset(): Preset version "));
      DEBUGP(_preset_buffer._preset_data.preset_version);
      DEBUGLN(F(" out of sync, setting to defaults."));    
      savePreset(true);
    }
  }
  _dirty = false;
  return (true);
}

/*
 * Reset current preset (restore to saved data)
 */
boolean PresetManager::resetCurrentPreset()
{
  if (_dirty)
  {
    loadPreset(_cur_preset);
  }
}

/*
 * Save the current preset buffer to FRAM.
 * If init is true, intialize the buffer to defaults before
 * saving.  Defaults to false.
 * Returns true if successful, false if not.
 */
boolean PresetManager::savePreset(boolean init)
{
  if (init)
  {
    DEBUGLN(F("savePreset(): Initializing preset to defaults."));
    _preset_buffer = _defaults;
    _dirty = true;
  }
  if (!_dirty) { return (true); }  //nothing to save
  sprintf(_error_buff, "savePreset(): Saving preset %02d to FRAM storage.");
  DEBUGLN(_error_buff);
  if (_writePresetData())
  {
    _dirty = false;
    return (true);
  }
  return (false);
}

/*
 * Writes the current preset buffer to FRAM.
 * FRAM location determined from current preset index.
 * Returns true if successful, false if not.
 */
boolean PresetManager::_writePresetData()
{
  uint16_t addr = PRESETS_ADDR_BASE + _cur_preset * PRESET_DATA_SIZE;
  if ((addr + PRESET_DATA_SIZE-1) > FRAM_SIZE)
  {
    DEBUGLN(F("FATAL: _writePresetData(): FRAM storage address overflow."));
    return (false);
  }
  int idx;
  for (int i=0; i<PRESET_DATA_SIZE; i++) {
    _fram.write8(addr + i, _preset_buffer.raw_data[i]);
    idx = i;
  }
  sprintf(_error_buff, "Starting at addr %d, wrote %d bytes to FRAM",addr, idx);
  DEBUGLN(_error_buff);
  return (true); 
}

/*
 * Reads the FRAM into the preset buffer.
 * FRAM location determined from current preset index.
 * Returns true if successful, false if not.
 */
boolean PresetManager::_readPresetData()
{
  uint16_t addr = PRESETS_ADDR_BASE + _cur_preset * PRESET_DATA_SIZE;
  if ((addr + PRESET_DATA_SIZE-1) > FRAM_SIZE) 
  {
    DEBUGLN(F("FATAL: _readPresetData(): FRAM storage address overflow."));
    return (false);
  }
  int idx;
  for (int i=0; i<PRESET_DATA_SIZE; i++) 
  {
    _preset_buffer.raw_data[i] = _fram.read8(addr + i);
    idx = i;
  }
  sprintf(_error_buff, "Starting at addr %d, read %d bytes from FRAM",addr, idx);
  DEBUGLN(_error_buff);  return (true);
}
