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

bool _debug = false;
bool print_diag = false;  //set true for debug diagnostics

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
  _delta = -1;
  _weight = 0;
  _kernels = -1;
  Serial1.begin(19200, SERIAL_8N1);
  while (!Serial1);
  delay(100);
  checkScale();
  delay(100);
  checkScale(); //First try fails, why?
  return (_connected);
}

/*
 * Return display friendly scale condition name
 */
const char* PTScale::getConditionName()
{
  return ConditionNames[_cond];
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
//bool PTScale::isChanged() { return (_display_changed); }

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

float PTScale::getOffScaleWeight() { return (_off_scale_weight); }
 
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
 * Request and process data from scale.  Sets scale state and internal 
 * data accordingly.
 * 
 * TODO: Tune SERIAL_TIMEOUT.
 */
void PTScale::checkScale()
{  
  byte idx = 0; // Index into array; where to store the character
  char in_char = -1; // The character read
  char serial_data[20]; // data buffer
  float tol = _ptconfig.getGnTolerance();      
  unsigned long _t;
  unsigned long timeout;
  static unsigned long _max_t = 0;
  static unsigned long _diag_time = 0;
  if (_diag_time == 0) _diag_time = millis();  //init first time.

  if (!(_serial_lock))
  {
    _t = micros(); //diagnostics timer
    _serial_lock = true;

    //Send data command
    Serial1.write(_cmd_reqData, 3); //request data from scale
    Serial1.flush();  //blocks until all data transmited    
    timeout = millis();
    while (Serial1.available() == 0)
    {
      if ((millis() - timeout) > SERIAL_TIMEOUT)
      {
        Serial.println("Scale command response timeout!");
        _connected = false;
        _serial_lock = false;
        return;
      }
    }
    _connected = true;  //we got a response

    //Read serial data sent back from scale
    timeout = millis();
    while (true)
    {
      if ((millis() - timeout) > SERIAL_TIMEOUT)
      {
        Serial.println("Scale read data timeout!");
        _cond = PTScale::undef;
        _serial_lock = false;
        return;        
      }
      while (Serial1.available() > 0)
      {
        if (idx < 19)
        {
          in_char = Serial1.read(); // Read a character
          serial_data[idx] = in_char; // Store it
          idx++; // Increment where to write next
          serial_data[idx] = '\0'; // Null terminate the string
        }
        else
        {
          Serial.println("Serial read error, too much data in buffer!"); //TODO: handle this better?
          _cond = PTScale::undef;
          _serial_lock = false;
          return;
        }
      }
      // Break on cr/lf data termination.
      // Need to do this check because Serial1.available() may not keep up
      // with the loop. Only 2 or 3 char at a time might be available at a time.
      if ((serial_data[idx - 2] == 13) && (serial_data[idx - 1] == 10)) { break; }
    }
    
    if (_debug)
    {
      Serial.print("Bytes read: ");
      Serial.println(idx);
      Serial.print("Data read: ");
      Serial.println(serial_data);      
    }
    
    //Process scale respnse. Calculate values & state from serial data
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
      // uh oh!  Might happen if scale settings changed.
      _ptstate.setState(PTState::pt_error);
      _ptstate.setSystemMessage(F("Unknown scale mode. "));
      _serial_lock = false;
      return;
    }

    // copy weight char data to a buffer, convert to float
    byte cpyIndex = 9;
    while (cpyIndex--) *(_weight_data + cpyIndex) = *(serial_data + cpyIndex + 3);
    _weight_data[9] = 0; //null terminiate
    _weight = atof(_weight_data); 

    // set delta and kernels to target calculations
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
       
    // now evaluate condition of scale based on weight and configuration
    if (_off_scale_weight == 0)
    {
      //off scale weight should be 0 until calibrated
      //TODO: is there a better way to handle this?
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
    else if (abs(_delta) <= tol)
    {
      _cond = PTScale::on_tgt;
    }
    else if (_weight > (_target + tol))
    {
      _cond = PTScale::over_tgt;
    }
    else if ((_delta <= _ptconfig.getDecelThreshold()) && (_delta > _ptconfig.getBumpThreshold()) && (_weight < _target))
    {
      _cond = PTScale::close_to_tgt;
    }
    else if ((_delta <= _ptconfig.getBumpThreshold()) && (_weight < _target))
    {
      _cond = PTScale::very_close_to_tgt;
    }
    else
    {
      _cond = PTScale::under_tgt; 
    }
    if (print_diag)
    {
      if ((micros() - _t) > _max_t) { _max_t = micros() - _t; }
      if ((millis() - _diag_time) > 1000)
      {
        _diag_time = millis();
        Serial.print("Max Micros spent in checkScale() this period: ");
        Serial.println(micros() - _t);
        _max_t = 0;    
      }
    }
    _serial_lock = false;
  }
}
