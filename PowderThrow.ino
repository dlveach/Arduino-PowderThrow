/***********************************************************************
 * PowderThrow 2.0
 * 
 * 
 * Redesigned with new hardware (including BlueTooth.
 * New OO design approach.
 * 
 * I2C Addresses:
 *   20x4 LCD Display driver:  0x27
 *   MCP23017 GPIO Expander:  0x20
 *   TIC stepper driver board 1: 0x0Esteam
 *   TIC stepper driver board 2: 0x0F
 *   FRAM board: 0x50
 *   
 *   TODO: 
 *   - Do a save on config after FRAM reset (or it will reset again)
 *   - LOTS of stuff for bluetooth (in work)
 *   - Clean up debug stuff
 *   - Evaluate headers, possible circular deps, possible refactoring.
 *   - Impliment scheduler to run LED update in another thread?
 *   - Evaluate use of more threads, like system run loop, Display, Buttons, BlueTooth, etc.?
 *   - Clean up modes, remove MANUAL
 *   - Find all "buffers" for name strings, use single static buffer defined globally.
 *
 *   HARDWARE: 
 *   - add a physical reset button
 *   
 *   BUGFIX:
 *   - a crash during running state leaves TIC running, impliment TIC library "reset command timout"
 *   - hang after failed calibrate trickler
 *   
 ***********************************************************************/

#include "PowderThrow.h"

/* The main loop */
void loop() {
  checkButtons();
  runSystem();
  checkBLE();
  updateLEDs();
}

/* BLE stuffs */
void checkBLE() {
  BLE.poll(); 
  if (BLE.connected())  {  updateBLEData(false);  }
}

/* System run state machine */
void runSystem() {
  long run_time = millis();
  static long _last_run_time = 0;
  static long _pause_time = 0;
  static long _dwell_time = 0;
  static long _lock_time = 0;
  static int _thrower_state = 0;  //0: stopped, 1: forward, 2: paused, 3:reverse
  static const bool FORWARD = true;
  static const bool REVERSE = false;

  if (_last_run_time == 0) { _last_run_time = run_time; } //init first time through
  if (_pause_time == 0) { _pause_time = run_time; } //init first time through
  if (_dwell_time == 0) { _dwell_time = run_time; } //init first time through
  if (_lock_time == 0) { _lock_time = run_time; } //init first time through

  //check interval
  if ((run_time - _last_run_time) >= RUN_INTERVAL) {
    _last_run_time = run_time;    
  } else { return; }

  g_scale.checkScale();

  //If not in a running state, do nothing
  int s = g_state.getState();
  if (!((s == PTState::pt_ready) ||
      (s == PTState::pt_manual) ||
      (s == PTState::pt_manual_run) ||
      (s == PTState::pt_ladder) ||
      (s == PTState::pt_ladder_run) ||
      (s == PTState::pt_throwing) ||
      (s == PTState::pt_trickling) ||
      (s == PTState::pt_bumping) ||
      (s == PTState::pt_paused) ||
      (s == PTState::pt_locked)))
  {
    //Not in running state, exit
      g_LED_Blu.setOff();
      g_LED_Yel_1.setOff();
      g_LED_Yel_2.setOff();
      g_LED_Grn.setOff();
      g_LED_Red.setOff();
      return;  
  }

  //TODO: put user resp dialogs in the next two checks and kick out to menu !!!
  
  if (!g_scale.isConnected()) { 
    DEBUGLN(F("Scale not connected while in running state, stop all."));
    //TODO: set a state?  
    //TODO: Should we really stop all in all states?
    stopAll();
    g_display_changed = true; 
    displayUpdate();
    return; 
  }   

  if (!isSystemCalibrated()) {
    //TODO: best way to handle this?
    DEBUGLN(F("System not calibrated while running"));
    stopAll();
    g_state.setState(PTState::pt_ready);  // TODO: should we kick out to the menu?
    g_display_changed = true; 
    displayUpdate();
    return; 
  }

  int scale_cond = g_scale.getCondition();

  if (scale_cond == PTScale::undef) {
    // TODO: Should just be a transient.  Need more testing to confirm.
    // for now just log and return to run loop.
    DEBUGLN(F("RunSystem(); Scale not ready. Condition == undef"));
    //delay(1000);  //DEBUGGING
    g_display_changed = true; 
    displayUpdate();
    return; 
  } 

  //TODO: better way to handle this?  Set a system error code/state?
  //TODO: has this ever happened?  More testing to evaluate.
  if (g_TIC_thrower.getOperationState() != TicOperationState::Normal) {
    DEBUGLN(F("Thrower TIC is not in operational state."));
    return;
  }
  if (g_TIC_trickler.getOperationState() != TicOperationState::Normal) {
    DEBUGLN(F("Trickler TIC is not in operational state."));
    return;
  }  

  // System has been switched from auto to a manual mode
  if ((g_state.getState() == PTState::pt_ladder) || (g_state.getState() == PTState::pt_manual)) {
      g_LED_Blu.setOn();
      g_LED_Yel_1.setOff();
      g_LED_Yel_2.setOff();
      g_LED_Grn.setOff();
      g_LED_Red.setOff();
      g_display_changed = true; 
      displayUpdate();
    return;
  }

  // Stop and lock system if scale pan is off in a "running" state.  I.E. User has lifted pan during a throw.
  if (scale_cond == PTScale::pan_off) {
    switch (g_state.getState()) {
      case PTState::pt_throwing:
      case PTState::pt_trickling:
      case PTState::pt_bumping:
      case PTState::pt_paused:
        DEBUGLN(F("Pan off scale while running, stop and lock."));
        stopAll();
        g_state.setState(PTState::pt_locked);
        _lock_time = run_time;  //start lock timer
        g_display_changed = true; 
        displayUpdate();
        return;
    }
  }

  // Handle running states
  switch (g_state.getState()) {
    case PTState::pt_ready:
    case PTState::pt_manual_run:
    case PTState::pt_ladder_run:
      if ((scale_cond == PTScale::zero) && (g_scale.isStable())) {
        DEBUGLN(F("State == Ready and pan on scale & stable, Start Throwing."));
        g_state.setState(PTState::pt_throwing);
      }
      g_LED_Blu.setFlash(1000);
      g_LED_Yel_1.setOff();
      g_LED_Yel_2.setOff();
      g_LED_Grn.setOff();
      g_LED_Red.setOff();       
      break;
    case PTState::pt_throwing:
      //TODO: replace this with call to manualThrow() ???? (don't dupe code).  What about delay() in manualThrow()?
      switch (_thrower_state) {
        case 0: //stopped
          DEBUGLN(F("Thrower stopped, start it forward and start trickler."));
          g_TIC_thrower.setTargetPosition(g_thrower_bottom_pos);
          startTrickler();
          _thrower_state = 1;
          break;
        case 1: //forward
          if (g_TIC_thrower.getCurrentPosition() >= g_thrower_bottom_pos) {
            DEBUGLN(F("Thrower at bottom, pause for thrower dwell."));
            _thrower_state = 2;
            _dwell_time = run_time; //start dwell timer
          }
          break;
        case 2: //dwell
          if ((run_time - _dwell_time) > THROWER_DWELL_TIME) {
            _dwell_time = run_time; //not needed?
            DEBUGLN(F("Thrower dwell time reached, start it reverse."));
            g_TIC_thrower.setTargetPosition(g_thrower_top_pos);
            _thrower_state = 3;
          }
          break;
        case 3: //reverse
          if (g_TIC_thrower.getCurrentPosition() <= g_thrower_top_pos) {
            DEBUGLN(F("Thrower back at top.  Set state to trickling."));
            g_state.setState(PTState::pt_trickling);
            _thrower_state = 0;
          }        
          break;
        default:
          logError("Unknown thrower state.", __FILE__, __LINE__);
          g_display_changed = true; 
          displayUpdate();
          return;
      }
      g_LED_Blu.setOff();
      g_LED_Yel_1.setOn();
      g_LED_Yel_2.setOff();
      g_LED_Grn.setOff();
      g_LED_Red.setOff();       
      break;
    case PTState::pt_trickling:
      if (scale_cond == PTScale::close_to_tgt) {
        setTricklerSpeed();
        g_LED_Blu.setOff();
        g_LED_Yel_1.setFlash();
        g_LED_Yel_2.setOff();
        g_LED_Grn.setOff();
        g_LED_Red.setOff();       
      } else if (scale_cond == PTScale::very_close_to_tgt) {
        DEBUGLN(F("Trickling and very close to tgt, pause before bumping."));
        stopAll();
        g_state.setState(PTState::pt_paused);
        _pause_time = run_time;
      } else if (scale_cond == PTScale::on_tgt) {
        DEBUGLN(F("Trickling and on target, stop and set state to pause."));
        stopAll();
        g_state.setState(PTState::pt_paused);
        _pause_time = run_time; //start pause timer 
      } else if (scale_cond == PTScale::over_tgt) {
        DEBUGLN(F("Trickling and over target, stop and set state to pause."));
        stopAll();
        g_state.setState(PTState::pt_paused);
        _pause_time = run_time; //start pause timer 
      } else if (scale_cond == PTScale::under_tgt) {
        //Make sure trickler is running
        if (g_TIC_trickler.getCurrentVelocity() ==  0) {
          DEBUGLN(F("WARN: Trickler stopped during trickling and under target.  Restart it."));
          startTrickler();
        }
        g_LED_Blu.setOff();
        g_LED_Yel_1.setOn();
        g_LED_Yel_2.setOff();
        g_LED_Grn.setOff();
        g_LED_Red.setOff();       
      } else if (scale_cond == PTScale::zero) {
        //Not expected, maybe powder container is empty (or closed)
        //Set back to ready to try another throw.
        DEBUGLN(F("WARN: Trickling and scale at zero.  Set ready to try throw again."));
        g_state.setState(PTState::pt_ready);
      } else {
        sprintf(g_msg, "Trickling and unexpected scale condition %s", ConditionNames[scale_cond]);
        logError(g_msg, __FILE__, __LINE__);
        //TODO: should we really exit here and lock?
        g_state.setState(PTState::pt_locked);
        _lock_time = run_time;
      }
      break;
    case PTState::pt_bumping:
      if (scale_cond == PTScale::on_tgt) {
        DEBUGLN(F("Bumping and on target, stop and set state to pause."));
        stopAll();
        g_state.setState(PTState::pt_paused);
        _pause_time = run_time; //start pause timer 
      } else if (scale_cond == PTScale::over_tgt) {
        DEBUGLN(F("Bumping and over target, stop and set state to pause."));
        stopAll();
        g_state.setState(PTState::pt_paused);
        _pause_time = run_time; //start pause timer 
      } else if ((scale_cond == PTScale::close_to_tgt) || (scale_cond == PTScale::under_tgt)) {
        //Unlikely but could happen?
        DEBUGLN(F("Bumping but below bump threshold. Resume trickling."));
        g_state.setState(PTState::pt_trickling); 
        setTricklerSpeed(true);
      } else if (scale_cond != PTScale::very_close_to_tgt) {
        sprintf(g_msg, "Bumping and unexpected scale condition %s", ConditionNames[scale_cond]);
        logError(g_msg, __FILE__, __LINE__);
        //TODO: should we really exit here and lock?
        g_state.setState(PTState::pt_locked);
        _lock_time = run_time;
      } else {
        bumpTrickler();
        g_LED_Blu.setOff();
        g_LED_Yel_1.setOn();
        g_LED_Yel_2.setFlash();
        g_LED_Grn.setOff();
        g_LED_Red.setOff(); 
      }
      break;
    case PTState::pt_paused:
      if (scale_cond == PTScale::on_tgt) {
        g_LED_Blu.setOff();
        g_LED_Yel_1.setOn();
        g_LED_Yel_2.setOn();
        g_LED_Grn.setFlash();
        g_LED_Red.setOff();         
      } else if (scale_cond == PTScale::over_tgt) {
        g_LED_Blu.setOff();
        g_LED_Yel_1.setOn();
        g_LED_Yel_2.setOn();
        g_LED_Grn.setOff();
        g_LED_Red.setFlash();                 
      }
      if ((run_time - _pause_time) > SYSTEM_PAUSE_TIME) {
        _pause_time = run_time;
        if ((scale_cond == PTScale::close_to_tgt) || (scale_cond == PTScale::very_close_to_tgt)) {
          g_state.setState(PTState::pt_bumping);        
        } else if (scale_cond == PTScale::on_tgt) {
          g_state.setState(PTState::pt_locked);
          _lock_time = run_time; 
        } else if (scale_cond == PTScale::over_tgt) {
          g_state.setState(PTState::pt_locked);
          _lock_time = run_time; 
        } else if (scale_cond == PTScale::under_tgt) {
          g_state.setState(PTState::pt_trickling); 
          setTricklerSpeed(true);
        } else if (scale_cond == PTScale::zero) {
          //This is a little strange.  Not sure it can happen. Best way to handle, go back to ready state?
          logError("Un-paused and scale at 0.  Return to sys ready/manual/ladder state.", __FILE__, __LINE__);
          if (g_config.getRunMode() == PTConfig::pt_ladder) { g_state.setState(PTState::pt_ladder);
          } else if (g_config.getRunMode() == PTConfig::pt_manual) { g_state.setState(PTState::pt_manual); 
          } else { g_state.setState(PTState::pt_ready); }
          _thrower_state = 0;
        } else {
          sprintf(g_msg, "Paused and unexpected scale condition %s", ConditionNames[scale_cond]);
          logError(g_msg, __FILE__, __LINE__);
          g_state.setState(PTState::pt_paused);
        }
      }
      break;
    case PTState::pt_locked:
      if (scale_cond == PTScale::on_tgt) {
        g_LED_Blu.setOff();
        g_LED_Yel_1.setOff();
        g_LED_Yel_2.setOff();
        g_LED_Grn.setOn();
        g_LED_Red.setOff();         
      } else if (scale_cond == PTScale::over_tgt) {
        g_LED_Blu.setOff();
        g_LED_Yel_1.setOff();
        g_LED_Yel_2.setOff();
        g_LED_Grn.setOff();
        g_LED_Red.setOn();                 
      }
      if ((run_time - _lock_time) > SYSTEM_LOCK_TIME) {
        _lock_time = run_time;
        switch (scale_cond) {
          case PTScale::pan_off:
            //reset system for another run
            DEBUGLN(F("Unlocked and pan off.  Set to run again."));
            if (g_config.getRunMode() == PTConfig::pt_ladder) { g_state.setState(PTState::pt_ladder);
            } else if (g_config.getRunMode() == PTConfig::pt_manual) { g_state.setState(PTState::pt_manual); 
            } else { g_state.setState(PTState::pt_ready); }
            _thrower_state = 0;
            break;
          case PTScale::on_tgt:
          case PTScale::over_tgt:
            //pan still on with full (over) charge, restart lock timer again
            DEBUGLN(F("Unlocked and pan full.  Relock."));
            _lock_time = run_time;
            break;
          case PTScale::close_to_tgt:
          case PTScale::very_close_to_tgt:
            //charge isn't full, restart bumping
            DEBUGLN(F("Unlocked and under target.  Start bumping."));
            g_state.setState(PTState::pt_bumping);
            break;
          case PTScale::zero:
            //pan back on empty, exit lock
            DEBUGLN(F("Unlocked and pan back on scale empty.  Set to run again."));
            if (g_config.getRunMode() == PTConfig::pt_ladder) { g_state.setState(PTState::pt_ladder);
            } else if (g_config.getRunMode() == PTConfig::pt_manual) { g_state.setState(PTState::pt_manual); 
            } else { g_state.setState(PTState::pt_ready); }
            _thrower_state = 0;
          default:
            //ERROR
            DEBUGLN(F("ERROR: runSystem() locked and unexpected scale condition."));
            break; 
        }
      }
      break;
    default:
      break;  // not in a running state, just ignore
  }
  g_display_changed = true; 
  displayUpdate();
}

/* Update LEDs.  Intended to be called often, from loop() */
void updateLEDs() {
  g_LED_Blu.update();
  g_LED_Yel_1.update();
  g_LED_Yel_2.update();
  g_LED_Grn.update();
  g_LED_Red.update();
}

/*  Run the trickler  */
void startTrickler() {
  DEBUGLN(F("Starting the trickler."));
  if (g_TIC_trickler.getCurrentVelocity() != 0) {
    g_TIC_trickler.setTargetVelocity(0);
    while (g_TIC_trickler.getCurrentVelocity() != 0);
  }
  g_TIC_trickler.haltAndSetPosition(0); //clear the position uncertain flag.  Need for bumping.
  g_TIC_trickler.setTargetVelocity(g_config.getTricklerSpeed() * TIC_PULSE_MULTIPLIER);
}

/*
 * Adjust trickler speed based on Fcurve map.
 * Only executes if state is trickling.
 * Optional force, defaults to false, true used to start trickler
 * when prev delta invalid. i.e. restarting trickler from a pause. 
 * Otherwise only adjust if scale delta changed from previous call.
 */
void setTricklerSpeed(bool force) {
  if (g_state.getState() != PTState::pt_trickling) { return; } 
  static float _prev_delta = -1;
  float delta = g_scale.getDelta();
  if (_prev_delta == -1)  { 
    _prev_delta = delta; //init first time
    force = true; 
  } 
  if (!force) {
    if (delta == _prev_delta) { return; } // nothing to do unless changed
  }
  float dt = g_config.getDecelThreshold();
  if (g_scale.getMode() == SCALE_MODE_GRAM) { dt = dt * GM_TO_GN_FACTOR; }    
  _prev_delta = g_scale.getDelta();
  int index = 100 - (100 * (dt - delta) / dt);
  if (index <= 1) index = 1;
  if (index >= 99) index = 99;
  float value = g_curve_map[index];
  value = value / 100.0;
   int limit = g_config.getDecelLimit() * TRICKLER_DIRECTION; 
  //Adjust speed never slowing beyond limit
  int new_speed = ((g_config.getTricklerSpeed()-limit)*value)+limit; 

  if (abs(new_speed) < g_config.getDecelLimit())  {
    //Shouldn't happen but just be sure not to slow beyond limit 
    new_speed = limit; 
  }
  g_TIC_trickler.setTargetVelocity(new_speed * TIC_PULSE_MULTIPLIER);  
}

/*  Bump the trickler if BUMP_INTERVAL time period elapsed.  */
void bumpTrickler() {
  static long _last_bump = 0;
  if (_last_bump == 0) { _last_bump = millis(); } //init first time
  if ((millis() - _last_bump) > BUMP_INTERVAL) {
    _last_bump = millis();
    int pos = g_TIC_trickler.getCurrentPosition();
    g_TIC_trickler.setTargetPosition(pos + BUMP_DISTANCE);
  }
}

/*  Stop the stepper drivers. */
void stopAll(bool setThrowerHome) {
  g_TIC_trickler.setTargetVelocity(0);
  g_TIC_thrower.setTargetVelocity(0);
  if (setThrowerHome) { 
    if (g_TIC_thrower.getCurrentPosition() != g_thrower_top_pos) {
      g_TIC_thrower.setTargetPosition(g_thrower_top_pos);    
    }
  }  
  int s = g_state.getState();
  if ((s == PTState::pt_ready) ||
      (s == PTState::pt_throwing) ||
      (s == PTState::pt_trickling) ||
      (s == PTState::pt_bumping) ||
      (s == PTState::pt_paused) ||
      (s == PTState::pt_locked))
  {
    //Was in a runnign state, set. to locked.  Won't be cleared until pan off scale.
    g_state.setState(g_state.pt_locked);
  } else {
    g_state.setState(g_state.pt_menu); //TODO: can e-stop be triggered in non-running states? test this
  }
}

/*  Check if thrower has been calibrated. */ 
 bool isSystemCalibrated() {
  bool cal = true;
  if (cal) { cal = !(g_TIC_thrower.getPositionUncertain()); }  // thrower calibrated
  if (cal) { cal = (g_scale.isCalibrated()); }  // Scale calibrated  
  return (cal);
 }

/*  Run a single manual throw. */
void manualThrow() {
  static bool _lock = false;  

  if (g_TIC_thrower.getOperationState() != TicOperationState::Normal) {
    logError("Thrower not in operational state.", __FILE__, __LINE__);
    return;
  }
  if (_lock) { return; } else { _lock = true; }  //only allow one call at a time
  
  bool throwing = true;
  int thrower_state = 0;
  long timeout = millis();
  while (throwing) {
    if ((millis() - timeout) >= 10000) { //timeout after 10 seconds, for safety
      logError("Manual throw timeed out", __FILE__, __LINE__);
      break;
    }
    switch (thrower_state) {
      case 0: //stopped
        DEBUGLN(F("Thrower stopped, start it."));
        g_TIC_thrower.setTargetPosition(g_thrower_bottom_pos);
        thrower_state = 1;
        break;
      case 1: //moving forward to bot
        if (g_TIC_thrower.getCurrentPosition() >= g_thrower_bottom_pos) {
          DEBUGLN(F("Thrower at end pos, pause for thrower dwell."));
          delay(THROWER_DWELL_TIME); //wait dwell time TODO: freezes system for dwell, is that ok?
          DEBUGLN(F("Move thrower back to start."));
          g_TIC_thrower.setTargetPosition(g_thrower_top_pos);
          thrower_state = 2;
        }
        break;
      case 2: //moving reverse to top
        if (g_TIC_thrower.getCurrentPosition() <= g_thrower_top_pos) {
          DEBUGLN(F("Thrower back at starting pos.  Done."));
          throwing = false;
        }        
        break;
      default:
        logError("Unknown thrower state.", __FILE__, __LINE__);
        throwing = false;
        break;  
    }
  }
  _lock = false;
}

/*  Manual trickle toggle run/stop  */
void toggleTrickler() {
  static bool _running = false;
  if (_running) {
    DEBUGLN(F("Manual: Stop trickler."));
    g_TIC_trickler.setTargetVelocity(0);
    _running = false;
  } else {
    DEBUGP(F("Manual: Start trickler with speed "));
    DEBUGLN(g_config.getTricklerSpeed());
    g_TIC_trickler.setTargetVelocity(g_config.getTricklerSpeed() * TIC_PULSE_MULTIPLIER);
    _running = true;
  }
}

/*  Calibrate scale.  Sets zero and pan on/pan off detection. */
void menuCalibrateScale() {
  g_state.setState(g_state.pt_man_cal_scale);
  g_mcp.getLastInterruptPin(); //clear any button interrupts
  g_lcd.clear();
  g_lcd.setCursor(0,0);
  g_lcd.print(F("Calibrate Scale ..."));
  g_lcd.setCursor(0,3);
  g_lcd.print(F("Press any button ..."));
  delay(500);
  pauseForAnyButton(); //first call in calibrateScale() is skipped in this code block. Why???  HACK: added 2nd call to make it work
  calibrateScale();
  delay(1000);
  setThrowerHome();
  g_scale.checkScale(); //update scale state
  g_lcd.setCursor(0,2);
  if (isSystemCalibrated()) {
    g_lcd.print(F("Scale calibrated.   "));
  } else {
    g_lcd.print(F("Scale Cal Failed.   "));
  }
  g_lcd.setCursor(0,3);
  g_lcd.print(F("Press any button ..."));
  pauseForAnyButton();
  g_state.setState(g_state.pt_menu);
}

/* Local menu command start trickler calibration */
void startTricklerCalibration() {
  g_state.setState(g_state.pt_man_cal_trickler);  
  if (g_scale.getCondition() == g_scale.pan_off) {
    g_lcd.setCursor(0,1);
    g_lcd.print("Pan off scale       ");
    g_lcd.setCursor(0,2);
    g_lcd.print("Cannot calibrate    ");
    g_lcd.setCursor(0,3);
    g_lcd.print("Press any button... ");
    pauseForAnyButton();
    g_state.setState(g_state.pt_menu); 
    g_display_changed = true;
  } else if (g_scale.getMode() != SCALE_MODE_GRAIN) {
    g_lcd.setCursor(0,1);
    g_lcd.print("Scale not in Grains ");
    g_lcd.setCursor(0,2);
    g_lcd.print("Cannot calibrate    ");
    g_lcd.setCursor(0,3);
    g_lcd.print("Press any button... ");
    pauseForAnyButton();
    g_state.setState(g_state.pt_menu); 
    g_display_changed = true;
  } else {
    calibrateTrickler();
    g_lcd.setCursor(0,3);
    g_lcd.print("Press any button... ");
    pauseForAnyButton();
  }
  g_state.setState(g_state.pt_menu); //return to system menu if started locally
  g_display_changed = true;  
}

/*
 * Calibrate trickler flow rate
 * A static flag (_running) is toggled if called during calibration (recursive if button pressed) and  
 *  triggers exit (setting static var _running to false, exiting calibration loop.  Also happens if
 *  central sends cancel command via BLE.
 * Will time out and auto exit after a number of samples or if successfully achieving
 *  a stable calibration before then.
 * Updates global calibration speed in configuration if successful.
 */
 #define SUCCESS_COUNT 10
void calibrateTrickler() {
  static bool _running = false;
  float flow_rate = 0.0;
  int test_speed = g_config.getTricklerSpeed();
  unsigned long last_check = 0;
  unsigned long last_poll = 0;
  float last_weight = 0;
  int count = 0; 
  float tot = 0.0;
  int max = 50;
  float avg = 0;
  char* buff = new char[41];
  float weight = 0;
  float diff = 0;
  float* samples = new float[5];
  int good_count = 0;
  bool success = false;
  static uint8_t* ble_data = new uint8_t[12];

  // if called recursively while running (either as button pressed or from BLE central)
  // during calibration then set the static flag to cancel it and just return. 
  if (_running) {
    _running = false;
    return; 
  }
  _running = true;  //start it

  //setup local LCD during calibration
  g_lcd.clear();
  g_lcd.setCursor(0,0);
  g_lcd.print("Calibrate Trickler:");
  g_lcd.setCursor(0,1);
  g_lcd.print("Sample:");
  g_lcd.setCursor(0,2);
  sprintf(buff, "Speed: %04d", test_speed);
  g_lcd.print(buff);
  g_lcd.setCursor(0,3);
  g_lcd.print("Avg gn/s:");

  // Start the test

  g_TIC_trickler.setTargetVelocity(test_speed * TIC_PULSE_MULTIPLIER);
  last_check = millis();
  while (((millis() - last_check) < 2000) && _running) {
    checkButtons();
    BLE.poll();
  }
  while (_running) {
    if ((millis() - last_check) > 2000) {
      last_check = millis();
      if (count++ > max) { 
        _running = false; 
        break;
      }
      g_lcd.setCursor(8,1);
      sprintf(buff, "%d", count);
      g_lcd.print(buff);
      g_lcd.setCursor(7,2);
      sprintf(buff, "%04d", test_speed);
      g_lcd.print(buff);
      g_scale.checkScale();  // what if scale comm fails or not in good state?
      if (g_scale.getMode() != SCALE_MODE_GRAIN) {
        // User switched scale during calibrate, bail out and fail!!!
        logError("Scale not in grains during calibrate.", __FILE__, __LINE__);
        _running = false;
        success = false;
        break;
      } 
      weight = g_scale.getWeight();  
      diff = weight - last_weight;
      last_weight = weight;
      if (diff <= 0) {
        //This should never happen, just here for testing.
        DEBUGLN(F("Diff below zero while calibrating trickler, skipping sample!!!"));
      } else {
        //Calculate moving average of 5 samples.
        tot = 0;
        for (int i=0; i<4; i++) {
          samples[i]=samples[i+1];
          tot = tot + samples[i];
        }
        samples[4] = diff;
        tot = tot + diff;
        avg = tot/5;
        //Start tuning when moving avg window is full
        if (count >= 5) {
          flow_rate = avg;
          g_lcd.setCursor(9,3);
          sprintf(buff, "%6.3f", avg);
          g_lcd.print(buff);
          if (avg > 1.1) {
            good_count = 0;            
            test_speed = test_speed - 50;
            g_TIC_trickler.setTargetVelocity(test_speed * TIC_PULSE_MULTIPLIER);            
          } else if (avg < 0.9) {
            good_count = 0;            
            test_speed = test_speed + 50;
            if (test_speed > MAX_TRICKLER_SPEED) { test_speed = MAX_TRICKLER_SPEED; }
            g_TIC_trickler.setTargetVelocity(test_speed * TIC_PULSE_MULTIPLIER);            
          } else {
            good_count++;
          }
        }
        memcpy(&ble_data[0], &count, 4);
        if (count < 5) { avg = -1.0; } // signal no average yet
        memcpy(&ble_data[4], &avg, 4);
        memcpy(&ble_data[8], &test_speed, 4);
        if (BLE.connected()) {
          BLEWriteTricklerCalData(ble_data, 12);
        }
      }
      if (good_count == SUCCESS_COUNT) {
        _running = false; 
        success = true;
        break;
      }
    }
    checkButtons();
    BLE.poll();
  }

  // Calibration is over
  g_TIC_trickler.setTargetVelocity(0);
  g_lcd.clear();
  g_lcd.setCursor(0,0);
  g_lcd.print("Calibrate Trickler:");
  if (success) {
    DEBUGLN(F("Calibration success"));
    if (BLE.connected()) {
      count = -99;  //stop signal to BLE central
      memcpy(&ble_data[0], &count, 4);
      memcpy(&ble_data[4], &avg, 4);
      memcpy(&ble_data[8], &test_speed, 4);
      BLEWriteTricklerCalData(ble_data, 12);
    }
    g_config.setTricklerSpeed(test_speed);
    g_config.saveConfig();
    g_lcd.setCursor(0,1);
    g_lcd.print("Calibration success!");
    g_lcd.setCursor(0,2);
    sprintf(buff, "New speed: %04d", test_speed);
    g_lcd.print(buff);
  } else {
    Serial.println("Calibration failed");
    if (BLE.connected()) {
      count = -99; //stop signal to BLE central
      memcpy(&ble_data[0], &count, 4);
      avg = -1.0; //indicate failure
      memcpy(&ble_data[4], &avg, 4);
      memcpy(&ble_data[8], &test_speed, 4);
      BLEWriteTricklerCalData(ble_data, 12);
    }
    g_lcd.setCursor(0,1);
    g_lcd.print("Calibration failed! ");
    g_lcd.setCursor(0,2);
    g_lcd.print("Trickler unchanged. ");
  }
}

// Set runtime config data for selected preset/powder  
//TODO: update BLE?
void setConfigPresetData() {
  if (g_presets.isDefined()) {
    g_config.setPreset(g_presets.getCurrentPreset());
    g_config.setTargetWeight(g_presets.getPresetChargeWeight());
    char buff[NAME_LEN];
    buff[0] = 0x00;
    g_presets.getPresetName(buff);    
    g_config.setPresetName(buff);
    if (!g_powders.loadPowder(g_presets.getPowderIndex())) {
      logError("Unable to load powder.", __FILE__, __LINE__, true);
    }
    if (g_powders.isPowderDefined()) {
      buff[0] = 0x00;
      g_powders.getPowderName(buff);
      g_config.setPowderName(buff);
      g_config.setKernelFactor(g_powders.getPowderFactor());
    } else {
      g_config.setPowderName(EMPTY_NAME);
      g_config.setKernelFactor(-1);
      g_config.setTargetWeight(-1);
    }
    if (!g_config.saveConfig()) {
      logError("Unable to save config.", __FILE__, __LINE__, true);
    }
  } else { 
    DEBUGLN(F("Runtime Config data not set")); 
    g_config.setPresetName(EMPTY_NAME);
    g_config.setTargetWeight(-1);
    g_config.setPowderName(EMPTY_NAME);
    g_config.setKernelFactor(-1);
  }
}
