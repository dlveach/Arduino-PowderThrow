/*
 * PTScale.cpp
 * 
 * A&D FXi Scale Communication and support
 */

#include "Arduino.h"
#include "PTScale.h"

/*
 * Constructor
 */
PTScale::PTScale() 
{
  _cond = PTScale::undef;
  _connected = false;
}

/*
 * Initialize the scale object.  
 * Setup Serial1.
 * Returns true if successful, false if not.
 * Will set a system error.
 */
bool PTScale::init(PTState s, PTConfig c)
{
  _ptstate = s;
  _ptconfig = c;
  _target = 0;
  _delta = 0;
  _weight = 0;
  _kernels = 0;
  Serial1.begin(19200);
  while (!Serial1);
  _last_poll = 0; // force poll in checkScale()
  checkScale();
  return (_connected);
}

/*
 * 
 */
void PTScale::setTarget(float t) { _target = t; }

/*
 * 
 */
float PTScale::getTarget() { return (_target); }

/*
 * 
 */
float PTScale::getDelta() { return (_delta); }

/*
 * 
 */
float PTScale::getWeight() { return (_weight); }

/*
 * 
 */
float PTScale::getKernels() { return (_kernels); }

/*
 * 
 */
int PTScale::getCondition() { return (_cond); }

/*
 * 
 */
int PTScale::getMode() { return (_mode); }

/*
 * 
 */
bool PTScale::isChanged() { return (_display_changed); }

/*
 * 
 */
bool PTScale::isStable() { return (_stable); }

/*
 * Return serial connection state
 */
bool PTScale::isConnected() { return (_connected); }

/*
 * During calibration this is called when pan is off scale.
 * Save that empty scale weight.  Scale was zeroed with pan on
 * so this empty weight will be a negative number.
 */
void PTScale::setOffScaleWeight() { _off_scale_weight = _weight; }
 
/*
 * 
 */
void PTScale::zeroScale()
{
  if (!(_serial_lock))
  {
    _serial_lock = true;
    int bytesSent = Serial1.write(_cmd_reZero, 3);
    _serial_lock = false;
  }
}

/*
 * 
 */
void PTScale::checkScale()
{  
  //tell the scale to give us data
  if (!(_serial_lock))
  {
    _serial_lock = true;
    int bytesSent = Serial1.write(_cmd_reqData, 3);
    if (bytesSent > 0) _connected = _readSerialData();  //TODO: can this ever return false?
    _serial_lock = false;
  }
}

/*
 * 
 */
boolean PTScale::_readSerialData()
{
  byte idx = 0; // Index into array; where to store the character
  char in_char = -1; // Where to store a character read
  char serial_data[20]; // data buffer

  // read rs-232 data record if data available
  if (Serial1.available())
  {
    idx = 0;
    while (true)
    {
      while (Serial1.available() > 0)
      {
        if (idx < 19)
        {
          in_char = Serial1.read(); // Read a character
          serial_data[idx] = in_char; // Store it
          idx++; // Increment where to write next
          serial_data[idx] = '\0'; // Null terminate the string
        }
      }
      // Exit loop on cr/lf data termination.
      // Need to do this check because Serial1.available() may not
      // keep up with the loop.   Only 2 or 3 char at a time available.
      if ((serial_data[idx - 2] == 13) && (serial_data[idx - 1] == 10)) break;

      //TODO: impliment a timeout & buffer overflow failsafe?  esp buffer overflow!
    }
  }

  // calculate values & state from serial data if we recieved data
  if (idx > 0)
  {
    _stable = (serial_data[0] == 'S');
    if (serial_data[14] == 'g')
    {
      _mode = SCALE_MODE_GRAM;
    }
    else if ((serial_data[13] == 'G') && (serial_data[14] == 'N'))
    {
      _mode = SCALE_MODE_GRAIN;
    }
    else
    {
      // uh oh!
      _ptstate.setState(PTState::pt_error);
      _ptstate.setSystemMessage(F("Unknown scale mode. "));
      return (false);
    }

    // copy weight char data to a buffer, convert to float
    byte cpyIndex = 9;
    while (cpyIndex--) *(_weight_data + cpyIndex) = *(serial_data + cpyIndex + 3);
    _weight_data[9] = 0; //null terminiate
    _weight = atof(_weight_data); 

    // now evaluate condition of scale based on weight and configuration
    if (_off_scale_weight == 0)
    {
      _cond = PTScale::undef;
    }
    else if (_weight < (_off_scale_weight / 2))
    {
      _cond = PTScale::pan_off;
    }
    else if (_weight == 0)
    {
      _cond = PTScale::zero;
    }
    else
    {
      long tol = _ptconfig.getGnTolerance();      
      if (_mode == SCALE_MODE_GRAM) tol = _ptconfig.getMgTolerance();
      switch (_mode)
      {
        case SCALE_MODE_GRAIN:
          _delta = _target - _weight;
          _kernels = _delta / _ptconfig.getKernelFactor();          
          break;
        case SCALE_MODE_GRAM:
          _delta = (_target * GM_TO_GN_FACTOR) - _weight;
          _kernels = _delta / (_ptconfig.getKernelFactor() * GM_TO_GN_FACTOR);
        break;              
      }
      if (abs(_delta) <= tol)
      {
        _cond = PTScale::on_tgt;
      }
      else if (_weight > (_target + tol))
      {
        _cond = PTScale::over_tgt;
      }
      else
      {
        _cond = PTScale::under_tgt; 
      }                
    }
    return (true);  // got data and processed successfully
  }
  _cond = PTScale::undef;
  return (false);  // No data recieved
}
