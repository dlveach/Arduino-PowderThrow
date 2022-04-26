/***********************************************************************
 * PowderThrow 2.0 Prototype
 * 
 * 
 * Just getting started.  Trying out new hardware.
 * 
 * I2C Addresses:
 *   20x4 LCD Display driver:  0x27
 *   MCP23017 GPIO Expander:  0x20
 *   TIC stepper driver board 1: 0x0Esteam
 *   TIC stepper driver board 2: 0x0F
 *   FRAM board: 0x50
 *   
 *   
 ***********************************************************************/

#include "PowderThrow.h"

/*
 * The main loop
 */
void loop() {
  g_loop_time = millis();
  static long _last_scale_poll;
  
  checkButtons();

  if (g_state.getState() == PTState::pt_ready)
  {
    if ((g_loop_time - _last_scale_poll) > SCALE_POLLING_RATE)
    { 
      _last_scale_poll = g_loop_time;
      g_scale.checkScale();
      displayUpdate();
    }
  }
   
  //TODO: do other cool stuff

}
