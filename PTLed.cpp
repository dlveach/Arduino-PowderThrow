/*
 * PTLed.cpp - PowderThrow LED implementation.
 * Created by David Veach.  2022
 * Private repository
 */
#include "Arduino.h"
#include "PTLed.h"

/*
 * Constructor
 */
PTLed::PTLed() 
{
  _initialized = false;
}

/*
 * Initialize the PTled object with MCP device and pin number. 
 */
void PTLed::init(Adafruit_MCP23X17 mcp, int pin) 
{
  _mcp = mcp;
  _pin = pin;
  _mcp.pinMode(_pin, OUTPUT);
  _flashing = false;
  _last_flash = millis();
  _state = HIGH;
  _mcp.digitalWrite(_pin, _state);
  delay(500);
  _state = LOW;
  _mcp.digitalWrite(_pin, _state);
  _needs_update = false;
  _initialized = true;  
}
  
/*
 * 
 */
void PTLed::setOn()
{
  if (_initialized)
  {
    if (_flashing) { _flashing = false; }
    if (_state == LOW) {
      _state = HIGH;
      _needs_update = true;  
    }
  }
}

/*
 * 
 */
void PTLed::setOff()
{
  if (_initialized)
  {
    if (_flashing) { _flashing = false; }
    if (_state == HIGH) {
      _state = LOW;
      _needs_update = true;  
    }  
  }
}

/*
 * 
 */
void PTLed::setFlash()
{
  if (_initialized)
  {
    if (!_flashing) 
    {
      _flashing = true;
      _needs_update = true;
    }
  }
}

/*
 * Update the MCP pin state if update needed.
 * Expects to be called regularly, like main loop or run loop.
 * Doesn't do anything if not initialized or no update needed.
 */
void PTLed::update()
{
  if (_initialized)
  {
    if (_flashing)
    {
      if ((millis() - _last_flash) > FLASH_PERIOD)
      {
        _last_flash = millis();
        _state = !_state;
        _needs_update = true;
      }
    }  
    if (_needs_update) 
    { 
      _mcp.digitalWrite(_pin, _state);
      _needs_update = false; 
    }
  }
}
