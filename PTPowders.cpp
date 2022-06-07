/*
 * PTPowders.cpp - PowderThrow powders implementation.
 * Created by David Veach.  2022
 * Private repository
 */
#include "Arduino.h"
#include "PTPowders.h"
#include "PTUtility.h"

/*
 * Constructor
 */
PowderManager::PowderManager() 
{
  //TODO: anything?
}

/*
 * Initialize the config object.  
 * Connects to FRAM, loads powder idx (def=00) from storage.
 * Returns true if successful, false if not.
 * Will set a system error.
 */
bool PowderManager::init(Adafruit_FRAM_I2C fram, int powder)
{
  _fram = fram;
  if (powder < 0) { powder = 0; }
  return (loadPowder(powder));
}

/*
 * Return state of current powder.
 * True if powder defined, false if not.
 */
 bool PowderManager::isPowderDefined()
 {
  return (_powder_buffer._powder_data.powder_factor > 0);
 }
 
/*
 * Return index of current powder in manager.
 */
int PowderManager::getCurrentPowder()
{
  return (_cur_powder);  
}

/*
 * Return config version.
 */
int PowderManager::getPowderVersion()
{
  return (_powder_buffer._powder_data.powder_version);
}

/*
 * Copy current loaded buffer's powder name into buff.
 * ERROR and return false if length of powder name too long.
 * TODO: change to just return pointer to char array.
 */
bool PowderManager::getPowderName(char* buff)
{
  //TODO: does this need a safety check?. 
  strcpy(buff, _powder_buffer._powder_data.powder_name);
  return (true);
}

/*
 * Copy NAME_LEN chars from buff to loaded buffer's name.
 * ERROR and return false if length of buff too long.
 */
bool PowderManager::setPowderName(char* buff)
{
  if (strlen(buff) > NAME_LEN)
  {
    DEBUGP(F("ERROR: string too long. "));
    DEBUGLN(__LINE__);
    util_handleSystemError("ERR: str too long");
    return (false);
  }
  strcpy(_powder_buffer._powder_data.powder_name, buff);
  _dirty = true;
  return (true);  
}

/*
 * Return Powder Factor value for current powder.
 */
float PowderManager::getPowderFactor()
{
  return (_powder_buffer._powder_data.powder_factor);
}

/*
 * Set powder factor in current powder.
 */
void PowderManager::setPowderFactor(float value)
{
  _powder_buffer._powder_data.powder_factor = value;
  _dirty = true;
}

/*
 * Increment powder name character at index i. 
 * Limited to A-Z, 0-9 and space
 */
void PowderManager::incNameChar(int i)
{
  char c = _powder_buffer._powder_data.powder_name[i];
  if ((c < ' ') || (c > 'Z')) { c = 'A'; }
  else if (c == 'Z') { c = ' '; }
  else if (c == ' ') { c = '0'; }
  else if (c == '9') { c = 'A'; }
  else { c++; }
  _powder_buffer._powder_data.powder_name[i] = c;
  _dirty = true;
}

/*
 * Decrement powder name character at index i.
 * Limited to A-Z, 0-9 and space
 */
void PowderManager::decNameChar(int i)
{
  char c = _powder_buffer._powder_data.powder_name[i];
  if ((c < ' ') || (c > 'Z')) { c = 'A'; }
  else if (c == ' ') { c = 'Z'; }
  else if (c == 'A') { c = '9'; }
  else if (c == '0') { c = ' '; }
  else { c--; }
  _powder_buffer._powder_data.powder_name[i] = c;
  _dirty = true;
}

void PowderManager::incPowderFactor(int pos)
{
  float val = _powder_buffer._powder_data.powder_factor;
  switch (pos)
  {
    case 12:
      if (val+0.1 < MAX_POWDER_FACTOR) {  val = val + 0.1; }
      break;
    case 13:
      if (val+0.01 < MAX_POWDER_FACTOR) {  val = val + 0.01; }
      break;
    case 14:
      if (val+0.001 < MAX_POWDER_FACTOR) {  val = val + 0.001; }
      break;
    case 15:
      if (val+0.0001 < MAX_POWDER_FACTOR) {  val = val + 0.0001; }
      break;
    case 16:
      if (val+0.00001 < MAX_POWDER_FACTOR) {  val = val + 0.00001; }
      break;
    case 17:
      if (val+0.000001 < MAX_POWDER_FACTOR) {  val = val + 0.000001; }
      break;
    case 18:
      if (val+0.0000001 < MAX_POWDER_FACTOR) {  val = val + 0.0000001; }
      break;
    case 19:
      if (val+0.00000001 < MAX_POWDER_FACTOR) {  val = val + 0.00000001; }
      break;
    default:
      DEBUGLN(F("Invalid cursor pos for incPowderFactor()"));
      return;
  }
  setPowderFactor(val);
}

void PowderManager::decPowderFactor(int pos)
{
  float val = _powder_buffer._powder_data.powder_factor;
  switch (pos)
  {
    case 12:
      if (val > 0.10000000) {  val = val - 0.1; }
      break;
    case 13:
      if (val > 0.01000000) {  val = val - 0.01; }
      break;
    case 14:
      if (val > 0.00100000) {  val = val - 0.001; }
      break;
    case 15:
      if (val > 0.00010000) {  val = val - 0.0001; }
      break;
    case 16:
      if (val > 0.00001000) {  val = val - 0.00001; }
      break;
    case 17:
      if (val > 0.00000100) {  val = val - 0.000001; }
      break;
    case 18:
      if (val > 0.00000010) {  val = val - 0.0000001; }
      break;
    case 19:
      if (val > 0.00000001) {  val = val - 0.00000001; }
      break;
    default:
      DEBUGLN(F("Invalid cursor pos for incPowderFactor()"));
      return;
  }
  setPowderFactor(val);  
}

/*
 * Return true if powder buffer data changed, false if not.
 */
bool PowderManager::isDirty()
{
  return (_dirty);
}

/*
 * Load the powder buffer from an indexed location in FRAM.
 * Param index: the powder index in the list (0 base). Default is _cur_powder.
 * If the powder data is out of sync, it is intialized to default and saved.
 * Returns true if successful, false if not.
 */ 
boolean PowderManager::loadPowder(int index)
{
  if ((index < 0) || (index > MAX_POWDERS))
  {
    DEBUGLN(F("ERROR: loadPowder(): Powder index out of range."));
    return (false); 
  }
  _cur_powder = index;
  //DEBUGLN(F("loadPowder(): reading FRAM storage for powder."));
  if (!_readPowderData()) 
  {
    DEBUGLN(F("loadPowder(): ERROR: could not read FRAM storage."));
    return (false);
  }
  else
  {
    if (_powder_buffer._powder_data.powder_version != POWDERS_VERSION) 
    {
      DEBUGP(F("loadPowder(): Powder version "));
      DEBUGP(_powder_buffer._powder_data.powder_version);
      DEBUGLN(F(" out of sync, setting to defaults."));    
      savePowder(true);
    }
  }
  _dirty = false;
  return (true);
}

/*
 * Restore buffer to saved data for current powder index in list.
 */
boolean PowderManager::resetBuffer()
{
  if (_dirty)
  {
    loadPowder(_cur_powder);
  }
}

/*
 * Save the current powder buffer to FRAM.
 * If init is true, intialize the buffer to defaults before
 * saving.  Defaults to false.
 * Returns true if successful, false if not.
 */
boolean PowderManager::savePowder(boolean init)
{
  if (init)
  {
    DEBUGLN(F("savePowder(): Initializing powder to defaults."));
    _powder_buffer = _defaults;
    _dirty = true;
  }
  if (!_dirty) { return (true); }  //nothing to save
  //sprintf(_error_buff, "savePowder(): Saving powder %02d to FRAM storage.", _cur_powder);
  //DEBUGLN(_error_buff);
  if (_writePowderData())
  {
    _dirty = false;
    return (true);
  }
  return (false);
}

/*
 * Writes the current powder buffer to FRAM.
 * FRAM location determined from current powder index.
 * Returns true if successful, false if not.
 */
boolean PowderManager::_writePowderData()
{
  uint16_t addr = POWDERS_ADDR_BASE + _cur_powder * POWDER_DATA_SIZE;
  if ((addr + POWDER_DATA_SIZE-1) > FRAM_SIZE)
  {
    DEBUGLN(F("FATAL: _writePowderData(): FRAM storage address overflow."));
    return (false);
  }
  int idx;
  for (int i=0; i<POWDER_DATA_SIZE; i++) {
    _fram.write8(addr + i, _powder_buffer.raw_data[i]);
    idx = i;
  }
  //sprintf(_error_buff, "Starting at addr %d, wrote %d bytes to FRAM",addr, idx);
  //DEBUGLN(_error_buff);
  return (true); 
}

/*
 * Reads the FRAM into the powder buffer.
 * FRAM location determined from current powder index.
 * Returns true if successful, false if not.
 */
boolean PowderManager::_readPowderData()
{
  uint16_t addr = POWDERS_ADDR_BASE + _cur_powder * POWDER_DATA_SIZE;
  if ((addr + POWDER_DATA_SIZE-1) > FRAM_SIZE) 
  {
    DEBUGLN(F("FATAL: _readPowderData(): FRAM storage address overflow."));
    return (false);
  }
  int idx;
  for (int i=0; i<POWDER_DATA_SIZE; i++) 
  {
    _powder_buffer.raw_data[i] = _fram.read8(addr + i);
    idx = i;
  }
  //sprintf(_error_buff, "Starting at addr %d, read %d bytes from FRAM",addr, idx);
  //DEBUGLN(_error_buff);  
  return (true);
}
