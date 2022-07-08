/*
 * PTScale.cpp
 * 
 * A&D FXi Scale Communication and support
 */

#include "Arduino.h"
#include "PTScale.h"
#include "PTUtility.h"

bool _debug = false;
bool print_diag = false;  //debug diagnostics


PTScale::PTScale()  {
  _cond = PTScale::undef;
  _connected = false;
  _calibrated = false;
  _delta = -1;
  _weight = 0;
  _kernels = -1;
  _off_scale_weight = 0; 
  _con_fail_count = 0;
}

/*
 * Initialize the scale object.  
 * Setup Serial1 for RS232 scale comm.
 * Returns true if successful, false if not.
 * Will set a system error.
 */
bool PTScale::init(PTConfig cfg) {
  _ptconfig = cfg;
  Serial1.begin(19200, SERIAL_8N1);
  while (!Serial1);
  delay(10);
  checkScale();
  return (_connected);
}

const char* PTScale::getConditionName() { return ConditionNames[_cond]; }

const char* PTScale::getConditionLongName() { return ConditionLongNames[_cond]; }

const char* PTScale::getModeName() { return ModeNames[_mode]; }

float PTScale::getDelta() { return (_delta); }

float PTScale::getWeight() { return (_weight); }

float PTScale::getKernels() { return (_kernels); }

int PTScale::getCondition() { return (_cond); }

int PTScale::getMode() { return (_mode); }

bool PTScale::isStable() { return (_stable); }

bool PTScale::isConnected() { return (_connected); }

/*
 * During calibration this is called when pan is off scale.
 * Save that empty scale weight.  Scale was zeroed with pan on
 * so this empty weight will be a negative number.
 * Only set if below a threshold.  Protect against forgot to
 * remove pan and scale drifted negative.
 */
void PTScale::setOffScaleWeight() {
  float threshold = MIN_CALIBRATION_WEIGHT;
  if (_mode == SCALE_MODE_GRAIN) { threshold = threshold * GM_TO_GN_FACTOR; }
  if (_weight < threshold) { 
    _off_scale_weight = _weight; 
    _calibrated = true;
  }
}

float PTScale::getOffScaleWeight() { return (_off_scale_weight); }

boolean PTScale::isCalibrated() { return (_calibrated); }

void PTScale::zeroScale(){
  if (!(_serial_lock)) {
    _serial_lock = true;
    int bytesSent = Serial1.write(_cmd_reZero, 3);
    _serial_lock = false;
  }
}

/*
 * Request and process data from scale.  Sets scale state and internal 
 * data accordingly.
 */
void PTScale::checkScale()
{  
  byte idx = 0;                   // Buffer index where to store the character
  char in_char = -1;              // The character read
  static char serial_data[20];    // Data buffer 
  unsigned long _t;
  unsigned long timeout;
  static unsigned long _max_time = 0;
  static unsigned long _diag_time = 0;
  static float _tol;
  static float _decel_thresh;
  static float _bump_thresh;
  static float _target;
  
  if (_diag_time == 0) _diag_time = millis();   //init first time
  for (int i=0; i<20; i++) { serial_data[i] = 0; }  //clear every time

  if (!(_serial_lock)) {
    _t = micros(); //diagnostics timer
    _serial_lock = true;

    // Send data request command to the Scale
    Serial1.write(_cmd_reqData, 3); 
    Serial1.flush();  //blocks until all data transmited    
    timeout = millis();
    while (Serial1.available() == 0) {
      if ((millis() - timeout) > SERIAL_TIMEOUT) {
        DEBUGLN(F("Scale command response timeout!"));
        _cond = PTScale::undef;
        _serial_lock = false;
        _con_fail_count = _con_fail_count + 1;
        if (_con_fail_count > SCALE_MAX_CON_FAILS) {
          _connected = false;
          _calibrated = false;  // in case scale was powered off, force recalibration.
          DEBUGLN(F("Scale exceeded max sequential serial comm fails. Scale disconnected."));
        }
        return;
      }
    }
    // We got a response!
    _connected = true;  
    _con_fail_count = 0;

    //Read serial data sent back from scale
    timeout = millis();
    while (true) {
      if ((millis() - timeout) > SERIAL_TIMEOUT) {
        DEBUGLN(F("Scale read data timeout!"));
        _cond = PTScale::undef;
        _serial_lock = false;
        _con_fail_count = _con_fail_count + 1;
        if (_con_fail_count > SCALE_MAX_CON_FAILS) {
          _connected = false;
          _calibrated = false;  // in case scale was powered off, force recalibration.
          DEBUGLN(F("Scale exceeded max sequential serial comm fails. Scale disconnected."));
        }
        return;        
      }
      while (Serial1.available() > 0) {
        if (idx < 19) {
          in_char = Serial1.read(); // Read a character
          serial_data[idx] = in_char; // Store it
          idx++; // Increment where to write next
          serial_data[idx] = '\0'; // Null terminate the string
        } else {
          DEBUGLN(F("Serial read error, too much data in buffer!")); //TODO: handle this better?
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
    
    if (_debug) {
      DEBUGP("Bytes read: ");
      DEBUGLN(idx);
      DEBUGP("Data read: ");
      DEBUGLN(serial_data);      
    }
    
    //Process scale respnse. Calculate values & state from serial data
    _stable = (serial_data[0] == 'S');
    _target = _ptconfig.getTargetWeight();
    if (serial_data[14] == 'g') {
      if (_mode != SCALE_MODE_GRAM) { 
        //changed from gn -> mg
        _target = _target * GM_TO_GN_FACTOR;
        _off_scale_weight = _off_scale_weight * GM_TO_GN_FACTOR;
      }
      _mode = SCALE_MODE_GRAM;
    } else if ((serial_data[13] == 'G') && (serial_data[14] == 'N')) {
      if (_mode != SCALE_MODE_GRAIN) { 
        //changed from mg -> gn
        _target = _target / GM_TO_GN_FACTOR;
        _off_scale_weight = _off_scale_weight / GM_TO_GN_FACTOR;
      }
      _mode = SCALE_MODE_GRAIN;
    } else {
      // uh oh!  Might happen if scale settings changed.
      util_handleSystemError(F("Unknown scale mode. "));
      _serial_lock = false;
      return;
    }
    
    //copy weight char data to a buffer, convert to float
    byte cpyIndex = 9;
    while (cpyIndex--) *(_weight_data + cpyIndex) = *(serial_data + cpyIndex + 3);
    _weight_data[9] = 0;
    _weight = atof(_weight_data); 
    
    //calculate scale data values and set states
    _delta = _target - _weight;
    _kernels = _delta / _ptconfig.getKernelFactor();
    _tol = _ptconfig.getGnTolerance();    
    _decel_thresh = _ptconfig.getDecelThreshold();
    _bump_thresh = _ptconfig.getBumpThreshold();

    //adjust for grams if necessary
    if (_mode == SCALE_MODE_GRAM) {
      _kernels = (_delta / GM_TO_GN_FACTOR) / _ptconfig.getKernelFactor(); //kernel factor is in grains
      _tol = _tol * GM_TO_GN_FACTOR;
      _decel_thresh = _decel_thresh * GM_TO_GN_FACTOR;
      _bump_thresh = _bump_thresh * GM_TO_GN_FACTOR;              
    } 
       
    // now evaluate condition of scale based on weight and configuration
    if (!isCalibrated()) {
      _cond = PTScale::undef;
    } else if (_weight < (_off_scale_weight / 2)) {
      _cond = PTScale::pan_off;
    } else if (_weight == 0) {
      _cond = PTScale::zero;
    } else if (abs(_delta) <= _tol) {
      _cond = PTScale::on_tgt;
    } else if (_weight > (_target + _tol)) {
      _cond = PTScale::over_tgt;
    } else if ((_delta <= _decel_thresh) && (_delta > _bump_thresh) && (_weight < _target)) {
      _cond = PTScale::close_to_tgt;
    } else if ((_delta <= _bump_thresh) && (_weight < _target)) {
      _cond = PTScale::very_close_to_tgt;
    } else {
      _cond = PTScale::under_tgt; 
    }
    if (print_diag) {
      if ((micros() - _t) > _max_time) { _max_time = micros() - _t; }
      if ((millis() - _diag_time) > 1000) {
        _diag_time = millis();
        DEBUGP("Max Micros spent in checkScale() this period: ");
        DEBUGLN(micros() - _t);
        _max_time = 0;    
      }
    }
    _serial_lock = false;
  }
}

/*
 * Diagnostic helper
 */
void PTScale::printConfig() {
  //TODO: incorporate DEBUG definitions
  static char buff[80];
  DEBUGLN(F("Scale object data:"));
  DEBUGP(F("Scale Connected: "));
  DEBUGLN(_connected);
  DEBUGP(F("Scale Target: "));
  DEBUGLN(g_config.getTargetWeight());
  sprintf(buff, "Scale grain tolerance: %8.6f", _ptconfig.getGnTolerance()); 
  DEBUGLN(buff);
  sprintf(buff, "Scale milligram tolerance: %8.6f", _ptconfig.getGnTolerance() * GM_TO_GN_FACTOR); 
  DEBUGLN(buff);
  DEBUGP(F("Scale Condition: "));
  DEBUGLN(getConditionName());
  DEBUGP(F("Scale Mode: "));
  DEBUGLN(getModeName());
  DEBUGP(F("Scale Off Scale Weight: "));
  DEBUGLN(_off_scale_weight);
  DEBUGP(F("Scale is calibrated: "));
  DEBUGLN(isCalibrated());
}
