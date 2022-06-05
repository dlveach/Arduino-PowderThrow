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
 *   - LOTS of stuff for bluetooth
 *   - Calibrate trickler speed
 *   - Clean up debug stuff
 *   - Evaluate headers, possible circular deps, possible refactoring.
 *   - Impliment scheduler to run LED update in another thread.
 *   - Evaluate use of more threads, like system run loop, Display, Buttons, BlueTooth, etc.
 *   
 *   BUGFIX
 *   - a crash during running state leaves TIC running, impliment "reset command timout"
 *   
 ***********************************************************************/

#include "PowderThrow.h"

//void setLEDs(bool forceOff=true); //TODO move to prototypes
//void updateLEDs(bool forceOff = false);

/*
 * The main loop
 */
void loop() {
  checkButtons();
  runSystem();
  checkBLE();
  updateLEDs();
}

/*
 * BLE stuffs
 */
void checkBLE()
{
  BLE.poll(); 
  if (BLE.connected()) 
  {
    updateBLEData(false);
  }
}

/*
 * System run state logic
 */
void runSystem()
{
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
  if ((run_time - _last_run_time) >= RUN_INTERVAL)
  {
    _last_run_time = run_time;    
  }
  else
  {
    return; //not time to run yet
  }

  g_scale.checkScale();

  //If not in a running state, do nothing
  int s = g_state.getState();
  if (!((s == PTState::pt_ready) ||
      (s == PTState::pt_throwing) ||
      (s == PTState::pt_trickling) ||
      (s == PTState::pt_bumping) ||
      (s == PTState::pt_paused) ||
      (s == PTState::pt_locked)))
  {
    //Not in running state, exit
    return;  
  }

  //TODO: pust user resp dialogs in the next two checks and kick out to menu !!!
  
  //g_scale.checkScale();  //TODO: try moving this up before state check so scale updates during calibration etc.

  if (!g_scale.isConnected()) 
  { 
    DEBUGLN(F("Scale not connected while in running state, stop all."));
    //TODO: set a state?  
    //TODO: Should we really stop all in all states?
    stopAll();
    g_display_changed = true; 
    displayUpdate();
    return; 
  }   

  if (!isSystemCalibrated()) 
  {
    //TODO: best way to handle this?
    DEBUGLN(F("System not calibrated while running"));
    stopAll();
    g_state.setState(PTState::pt_ready);  // TODO: should we kick out to the menu?
    g_display_changed = true; 
    displayUpdate();
    return; 
  }

  int scale_cond = g_scale.getCondition();

  if (scale_cond == PTScale::undef) 
  {
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
  if (g_TIC_thrower.getOperationState() != TicOperationState::Normal)
  {
    DEBUGLN(F("Thrower TIC is not in operational state."));
    return;
  }
  if (g_TIC_trickler.getOperationState() != TicOperationState::Normal)
  {
    DEBUGLN(F("Trickler TIC is not in operational state."));
    return;
  }  

  // Now check to see if scale pan is off in a "running" state.
  if (scale_cond == PTScale::pan_off)
  {
    switch (g_state.getState())
    {
      //Stop system and lock if in an active running state and pan removed
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
  switch (g_state.getState())
  {
    case PTState::pt_ready:
      if ((scale_cond == PTScale::zero) && (g_scale.isStable()))
      {
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
      //TODO: replace this with call to manualThrow() ???? (don't dupe code)
      switch (_thrower_state)
      {
        case 0: //stopped
          DEBUGLN(F("Thrower stopped, start it forward and start trickler."));
          g_TIC_thrower.setTargetPosition(g_thrower_bottom_pos);
          startTrickler();
          _thrower_state = 1;
          break;
        case 1: //forward
          if (g_TIC_thrower.getCurrentPosition() >= g_thrower_bottom_pos)
          {
            DEBUGLN(F("Thrower at bottom, pause for thrower dwell."));
            _thrower_state = 2;
            _dwell_time = run_time; //start dwell timer
          }
          break;
        case 2: //dwell
          if ((run_time - _dwell_time) > THROWER_DWELL_TIME)
          {
            _dwell_time = run_time; //not needed?
            DEBUGLN(F("Thrower dwell time reached, start it reverse."));
            g_TIC_thrower.setTargetPosition(g_thrower_top_pos);
            _thrower_state = 3;
          }
          break;
        case 3: //reverse
          if (g_TIC_thrower.getCurrentPosition() <= g_thrower_top_pos)
          {
            DEBUGLN(F("Thrower back at top.  Set state to trickling."));
            g_state.setState(PTState::pt_trickling);
            _thrower_state = 0;
          }        
          break;
        default:
          DEBUGP(F("ERROR Unknown thrower state. runSystem() line "));
          DEBUGLN(__LINE__);
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
      if (scale_cond == PTScale::close_to_tgt)
      {
        //DEBUGP(F("Trickling and close to tgt, slow down. Delta = "));
        //DEBUGLN(g_scale.getDelta());
        setTricklerSpeed();
        g_LED_Blu.setOff();
        g_LED_Yel_1.setFlash();
        g_LED_Yel_2.setOff();
        g_LED_Grn.setOff();
        g_LED_Red.setOff();       
      }
      else if (scale_cond == PTScale::very_close_to_tgt)
      {
        DEBUGLN(F("Trickling and very close to tgt, pause before bumping."));
        stopAll();
        g_state.setState(PTState::pt_paused);
        _pause_time = run_time;
      }
      else if (scale_cond == PTScale::on_tgt)
      {
        DEBUGLN(F("Trickling and on target, stop and set state to pause."));
        stopAll();
        g_state.setState(PTState::pt_paused);
        _pause_time = run_time; //start pause timer 
      }      
      else if (scale_cond == PTScale::over_tgt)
      {
        DEBUGLN(F("Trickling and over target, stop and set state to pause."));
        stopAll();
        g_state.setState(PTState::pt_paused);
        _pause_time = run_time; //start pause timer 
      }
      else if (scale_cond == PTScale::under_tgt) 
      {
        //Make sure trickler is running
        if (g_TIC_trickler.getCurrentVelocity() == 0)
        {
          DEBUGLN(F("WARN: Trickler stopped during trickling and under target.  Restart it."));
          startTrickler();
        }
        g_LED_Blu.setOff();
        g_LED_Yel_1.setOn();
        g_LED_Yel_2.setOff();
        g_LED_Grn.setOff();
        g_LED_Red.setOff();       
      }
      else if (scale_cond == PTScale::zero)
      {
        //Not expected, maybe powder container is empty (or closed)
        //Set back to ready to try another throw.
        DEBUGLN(F("WARN: Trickling and scale at zero.  Set ready to try throw again."));
        g_state.setState(PTState::pt_ready);
      }
      else
      {
        DEBUGP(F("ERROR: runSystem() trickling and unexpected scale condition: "));
        DEBUGLN(g_scale.getConditionName());
        DEBUGLN(F("Lock system."));
        //TODO: should we really exit here and lock?
        g_state.setState(PTState::pt_locked);
        _lock_time = run_time;
      }
      break;
    case PTState::pt_bumping:
      if (scale_cond == PTScale::on_tgt)
      {
        DEBUGLN(F("Bumping and on target, stop and set state to pause."));
        stopAll();
        g_state.setState(PTState::pt_paused);
        _pause_time = run_time; //start pause timer 
      }
      else if (scale_cond == PTScale::over_tgt)
      {
        DEBUGLN(F("Bumping and over target, stop and set state to pause."));
        stopAll();
        g_state.setState(PTState::pt_paused);
        _pause_time = run_time; //start pause timer 
      }
      else if ((scale_cond == PTScale::close_to_tgt) || (scale_cond == PTScale::under_tgt))
      {
        //Unlikely but could happen?
        DEBUGLN(F("Bumping but below bump threshold. Resume trickling."));
        g_state.setState(PTState::pt_trickling); 
        setTricklerSpeed(true);
      }
      else if (scale_cond != PTScale::very_close_to_tgt)
      {
        DEBUGP(F("ERROR: runSystem() bumping and unexpected scale condition: "));
        DEBUGLN(g_scale.getConditionName());
        DEBUGLN(F("Lock system."));
        //TODO: should we really exit here and lock?
        g_state.setState(PTState::pt_locked);
        _lock_time = run_time;
      }
      else
      {
        bumpTrickler();
        g_LED_Blu.setOff();
        g_LED_Yel_1.setOn();
        g_LED_Yel_2.setFlash();
        g_LED_Grn.setOff();
        g_LED_Red.setOff(); 
      }
      break;
    case PTState::pt_paused:
      if (scale_cond == PTScale::on_tgt)
      {
        g_LED_Blu.setOff();
        g_LED_Yel_1.setOn();
        g_LED_Yel_2.setOn();
        g_LED_Grn.setFlash();
        g_LED_Red.setOff();         
      }
      else if (scale_cond == PTScale::over_tgt)
      {
        g_LED_Blu.setOff();
        g_LED_Yel_1.setOn();
        g_LED_Yel_2.setOn();
        g_LED_Grn.setOff();
        g_LED_Red.setFlash();                 
      }
      if ((run_time - _pause_time) > SYSTEM_PAUSE_TIME)
      {
        _pause_time = run_time;
        if ((scale_cond == PTScale::close_to_tgt) || (scale_cond == PTScale::very_close_to_tgt))
        {
          DEBUGLN(F("Un-paused and (very) close to tgt, start bumping."));
          g_state.setState(PTState::pt_bumping);        
        }
        else if (scale_cond == PTScale::on_tgt)
        {
          DEBUGLN(F("Un-paused and on target target.  Lock system."));
          g_state.setState(PTState::pt_locked);
          _lock_time = run_time; //start lock timer
        }
        else if (scale_cond == PTScale::over_tgt)
        {
          DEBUGLN(F("Un-paused and over target.  Lock system."));
          g_state.setState(PTState::pt_locked);
          _lock_time = run_time; //start lock timer
        }
        else if (scale_cond == PTScale::under_tgt)
        {
          DEBUGLN(F("Un-paused and under target, resume trickler."));
          g_state.setState(PTState::pt_trickling);
          setTricklerSpeed(true);
        }
        else if (scale_cond == PTScale::zero)
        {
          //This is a little strange.  Best way to handle, go back to ready?
          DEBUGLN(F("WARN: Un-paused and scale at 0.  Return to sys ready state."));
          g_state.setState(PTState::pt_ready);
        }
        else
        {
          DEBUGP(F("ERROR: runSystem() paused and unexpected scale condition: "));
          DEBUGLN(g_scale.getConditionName());
          DEBUGLN(F("Pausing again."));
          g_state.setState(PTState::pt_paused);
        }
      }
      break;
    case PTState::pt_locked:
      if (scale_cond == PTScale::on_tgt)
      {
        g_LED_Blu.setOff();
        g_LED_Yel_1.setOff();
        g_LED_Yel_2.setOff();
        g_LED_Grn.setOn();
        g_LED_Red.setOff();         
      }
      else if (scale_cond == PTScale::over_tgt)
      {
        g_LED_Blu.setOff();
        g_LED_Yel_1.setOff();
        g_LED_Yel_2.setOff();
        g_LED_Grn.setOff();
        g_LED_Red.setOn();                 
      }
      if ((run_time - _lock_time) > SYSTEM_LOCK_TIME)
      {
        _lock_time = run_time;
        switch (scale_cond)
        {
          case PTScale::pan_off:
            //reset system for another run
            DEBUGLN(F("Unlocked and pan off.  Set ready."));
            g_state.setState(PTState::pt_ready);
            _thrower_state = 0;
            break;
          case PTScale::on_tgt:
          case PTScale::over_tgt:
            //pan still on with full (over) charge, start lock timer again
            DEBUGLN(F("Unlocked and pan full.  Relock."));
            _lock_time = run_time;
            break;
          case PTScale::close_to_tgt:
          case PTScale::very_close_to_tgt:
            //charge isn't full, start bumping
            DEBUGLN(F("Unlocked and under target.  Start bumping."));
            g_state.setState(PTState::pt_bumping);
            break;
          case PTScale::zero:
            //pan back on empty, exit lock
            DEBUGLN(F("Unlocked and pan back on scale empty.  Set ready."));
            g_state.setState(PTState::pt_ready);
          default:
            //ERROR
            DEBUGLN(F("ERROR: runSystem() locked and unexpected scale condition."));
            break; 
        }
      }
      break;
    default:
      // not in a running state, just ignore
      break;
  }
  g_display_changed = true; 
  displayUpdate();
}

/*
 * Update LEDs
 * Intended to be called often, from loop()
 */
void updateLEDs()
{
  g_LED_Blu.update();
  g_LED_Yel_1.update();
  g_LED_Yel_2.update();
  g_LED_Grn.update();
  g_LED_Red.update();
}

/*
 * Run the trickler
 */
void startTrickler()
{
  DEBUGLN(F("Starting the trickler."));
  if (g_TIC_trickler.getCurrentVelocity() != 0)
  {
    g_TIC_trickler.setTargetVelocity(0);
    while (g_TIC_trickler.getCurrentVelocity() != 0);
  }
  g_TIC_trickler.haltAndSetPosition(0); //clear the position uncertain flag.  Need for bumping.
  g_TIC_trickler.setTargetVelocity(g_trickler_cal_speed * TIC_PULSE_MULTIPLIER);
}

/*
 * Adjust trickler speed based on Fcurve map.
 * Only executes if state is trickling.
 * Optional force, defaults to false, true used to start trickler
 * when prev delta invalid. i.e. restarting trickler from a pause. 
 * Otherwise only adjust if scale delta changed from previous call.
 */
void setTricklerSpeed(bool force)
{
  if (g_state.getState() != PTState::pt_trickling) { return; } 
  static float _prev_delta = -1;
  float delta = g_scale.getDelta();
  if (_prev_delta == -1) 
  { 
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
  //speed is always negative so make limit negative
  int limit = g_config.getDecelLimit() * TRICKLER_DIRECTION; 
  //Adjust speed never slowing beyond limit
  int new_speed = ((g_trickler_cal_speed-limit)*value)+limit; 

  if (abs(new_speed) < g_config.getDecelLimit()) 
  {
    //Shouldn't happen but just be sure not to slow beyond limit 
    new_speed = limit; 
  }
  g_TIC_trickler.setTargetVelocity(new_speed * TIC_PULSE_MULTIPLIER);  
}

/*
 * Bump the trickler if BUMP_INTERVAL time period elapsed.
 */
void bumpTrickler()
{
  static long _last_bump = 0;
  if (_last_bump == 0) { _last_bump = millis(); } //init first time
  if ((millis() - _last_bump) > BUMP_INTERVAL)
  {
    _last_bump = millis();
    int pos = g_TIC_trickler.getCurrentPosition();
    g_TIC_trickler.setTargetPosition(pos + BUMP_DISTANCE);
  }
}


/*
 * Stop the system.
 * Set trickler and thrower speed to 0.
 */
void stopAll()
{
  g_TIC_trickler.setTargetVelocity(0);
  g_TIC_thrower.setTargetVelocity(0);
  //TODO:  if thrower not at home pos, move it there?
/*  
  if (g_TIC_thrower.getCurrentPosition() != g_thrower_top_pos)
  {
    g_TIC_thrower.setTargetPosition(g_thrower_top_pos);    
  }
*/  
}

/*
 * Check if thrower has been calibrated.   
 * 
 * TODO: get rid of this fn if this is all it does?
 */
 bool isSystemCalibrated()
 {
  bool cal = true;
  if (cal) { cal = !(g_TIC_thrower.getPositionUncertain()); }  // thrower calibrated
  if (cal) { cal = (g_scale.isCalibrated()); }  // Scale calibrated  
  return (cal);
 }

/*
 * Run a single manual throw
 */
void manualThrow()
{
  static bool _lock = false;  

  if (g_TIC_thrower.getOperationState() != TicOperationState::Normal)
  {
    DEBUGP(F("ERROR: Thrower not in operational state.  manualThrow() line "));
    DEBUGLN(__LINE__); 
    return;
  }
  if (_lock) { return; } else { _lock = true; }  //only allow one call at a time
  
  bool throwing = true;
  int thrower_state = 0;
  long timeout = millis();
  while (throwing)
  {
    if ((millis() - timeout) >= 10000) //timeout after 10 seconds, for safety
    {
      DEBUGLN(F("ERROR: manual throw timeed out"));
      break;
    }
    switch (thrower_state)
    {
      case 0: //stopped
        DEBUGLN(F("Thrower stopped, start it."));
        g_TIC_thrower.setTargetPosition(g_thrower_bottom_pos);
        thrower_state = 1;
        break;
      case 1: //moving forward to bot
        if (g_TIC_thrower.getCurrentPosition() >= g_thrower_bottom_pos)
        {
          DEBUGLN(F("Thrower at end pos, pause for thrower dwell."));
          delay(THROWER_DWELL_TIME); //wait dwell time TODO: freezes system for dwell, is that ok?
          DEBUGLN(F("Move thrower back to start."));
          g_TIC_thrower.setTargetPosition(g_thrower_top_pos);
          thrower_state = 2;
        }
        break;
      case 2: //moving reverse to top
        if (g_TIC_thrower.getCurrentPosition() <= g_thrower_top_pos)
        {
          DEBUGLN(F("Thrower back at starting pos.  Done."));
          throwing = false;
        }        
        break;
      default:
        DEBUGP(F("ERROR Unknown thrower state. manualThrow() line "));
        DEBUGLN(__LINE__);
        throwing = false;
        break;  
    }
  }
  _lock = false;
}

/*
 * Manual trickle toggle run/stop
 */
void toggleTrickler()
{
  static bool _running = false;
  if (_running)
  {
    DEBUGLN(F("Manual: Stop trickler."));
    g_TIC_trickler.setTargetVelocity(0);
    _running = false;
  }
  else
  {
    DEBUGP(F("Manual: Start trickler with speed "));
    DEBUGLN(g_trickler_cal_speed);
    g_TIC_trickler.setTargetVelocity(g_trickler_cal_speed * TIC_PULSE_MULTIPLIER);
    _running = true;
  }
}

/*
 * Calibrate trickler flow rate
 * runMe is true when called to start (manual menu)
 * runMe is false when button pressed during calibration (A recursive call) and  
 *  triggers exit (setting static var _running to false, exiting calibration loop.
 * Will time out and auto exit after a number of samples or if successfully achieving
 *  a stable calibration before then.
 * Updates global calibration speed if successful.
 */
 #define SUCCESS_COUNT 10
void calibrateTrickler(bool runMe)
{
  static bool _running = false;
  float flow_rate = 0.0;
  int test_speed = g_trickler_cal_speed;
  unsigned long last_check = 0;
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
  if (!runMe) {
    // button pressed during calibration
    _running = false;
    return; 
  }
  _running = true;
  g_state.setState(g_state.pt_man_cal_trickler);
  g_lcd.clear();
  g_lcd.setCursor(0,0);
  g_lcd.print("Calibrate Trickler:");
  if (g_scale.getMode() == SCALE_MODE_GRAM) {
    g_lcd.setCursor(0,1);
    g_lcd.print("Scale not in Grains ");
    g_lcd.setCursor(0,2);
    g_lcd.print("Cannot calibrate    ");
    g_lcd.setCursor(0,3);
    g_lcd.print("Press any button... ");
    pauseForAnyButton();
    pauseForAnyButton();
    g_state.setState(g_state.pt_man); 
    g_display_changed = true;
    return;
  }
  g_lcd.setCursor(0,1);
  g_lcd.print("Sample:");
  g_lcd.setCursor(0,2);
  sprintf(buff, "Speed: %04d", test_speed);
  g_lcd.print(buff);
  g_lcd.setCursor(0,3);
  g_lcd.print("Avg gn/s:");
  g_TIC_trickler.setTargetVelocity(test_speed * TIC_PULSE_MULTIPLIER);
  delay(2000);   
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
      weight = g_scale.getWeight();  
      diff = weight - last_weight;
      last_weight = weight;
      if (diff <= 0) {
        //This should never happen, just here for testing.
        Serial.println("Diff below zero, skipping sample!!!");
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
            g_TIC_trickler.setTargetVelocity(test_speed * TIC_PULSE_MULTIPLIER);            
          } else {
            good_count++;
          }
        }
      }
      if (good_count == SUCCESS_COUNT) {
        _running = false; 
        success = true;
        break;
      }
    }
    // Call checkButtons() here to update running state if button pressed
    checkButtons();
    //checkBLE()  TODO: when there's a calibrate display in the phone app
  }
  g_TIC_trickler.setTargetVelocity(0);
  g_lcd.clear();
  g_lcd.setCursor(0,0);
  g_lcd.print("Calibrate Trickler:");
  if (success) {
    g_trickler_cal_speed = test_speed;
    g_lcd.setCursor(0,1);
    g_lcd.print("Calibration success!");
    g_lcd.setCursor(0,2);
    sprintf(buff, "New speed: %04d", g_trickler_cal_speed);
    g_lcd.print(buff);
    g_lcd.setCursor(0,3);
    g_lcd.print("Press any button... ");
    pauseForAnyButton();
  } else {
    g_lcd.setCursor(0,1);
    g_lcd.print("Calibration failed! ");
    g_lcd.setCursor(0,2);
    g_lcd.print("Trickler unchanged. ");
    g_lcd.setCursor(0,3);
    g_lcd.print("Press any button... ");
    pauseForAnyButton();
    pauseForAnyButton();
    }
  g_state.setState(g_state.pt_man);
  g_display_changed = true;
}
  

