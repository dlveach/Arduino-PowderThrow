
/*
 * Setup.ino  Arduino setup()
 */
void setup() {
  delay(100);
  Wire.begin();

  g_state.setState(PTState::pt_setup);

  #ifdef DEBUG_SERIAL  
  Serial.begin(9600);
  while (!Serial) delay(10);
  DEBUGLN(F("Serial enabled"));
  #endif

  delay (500);
  
  // Check for LCD display
  int error;
  Wire.beginTransmission(0x27);
  error = Wire.endTransmission();
  if (error == 0) {
    g_lcd.begin(20, 4);
    delay(10);
    DEBUGLN(F("LCD I2C Enabled"));
    g_lcd.setBacklight(255);
    g_lcd.clear();  
    g_lcd.setCursor (0, 0);
    g_lcd.print("PowderThrow 2.0");
    g_lcd.setCursor(0,2);
    g_lcd.print(F("System init ...     "));
  } else {
    DEBUGP(F("LCD I2C comm err: "));
    DEBUGP(error);
    while (1) delay(10);
  }  
  delay(1000);
  
  // Check for MCP expander
  if (!g_mcp.begin_I2C()) 
  {
    DEBUGLN(F("MCP I2C comm err"));
    while (1) delay(10);
  }
  DEBUGLN(F("MCP I2C Enabled"));
  g_lcd.setCursor(0,2);
  g_lcd.print(F("MCP initialized ... "));
  // MCP interrupt config.  Mirror INTA/B so only one wire required.  Active drive so INTA/B 
  // will not be floating.  INTA/B will be signaled with a LOW.
  g_mcp.pinMode(BTN_OK, INPUT_PULLUP);
  g_mcp.pinMode(BTN_LEFT, INPUT_PULLUP);
  g_mcp.pinMode(BTN_RIGHT, INPUT_PULLUP);
  g_mcp.pinMode(BTN_UP, INPUT_PULLUP);
  g_mcp.pinMode(BTN_DOWN, INPUT_PULLUP);
  g_mcp.setupInterrupts(true, false, LOW);  
  g_mcp.setupInterruptPin(BTN_OK, LOW);
  g_mcp.setupInterruptPin(BTN_LEFT, LOW);
  g_mcp.setupInterruptPin(BTN_RIGHT, LOW);
  g_mcp.setupInterruptPin(BTN_UP, LOW);
  g_mcp.setupInterruptPin(BTN_DOWN, LOW);
  pinMode(INT_PIN, INPUT);  // configure nano pin for MCP interrupt
  g_mcp.pinMode(MCP_LED_RED_PIN, OUTPUT);
  g_mcp.pinMode(MCP_LED_GRN_PIN, OUTPUT);
  g_mcp.pinMode(MCP_LED_YEL2_PIN, OUTPUT);
  g_mcp.pinMode(MCP_LED_YEL1_PIN, OUTPUT);
  g_mcp.pinMode(MCP_LED_BLU_PIN, OUTPUT);
  //clear MCP interrupts just in case
  g_mcp.getLastInterruptPin(); 
  delay(10);
  g_mcp.getLastInterruptPin();  //be really sure!!!

  // Initialize FRAM
  Adafruit_FRAM_I2C fram = Adafruit_FRAM_I2C();
  if (!fram.begin())
  {  
    DEBUGLN(F("FRAM I2C comm err"));
    util_handleSystemError(F("FRAM I2C comm err"));
  }
  DEBUGLN(F("FRAM I2C Enabled"));
  g_lcd.setCursor(0,2);
  g_lcd.print(F("FRAM initialized ..."));
  g_LED_Blu.init(g_mcp, MCP_LED_BLU_PIN);
  //delay (500);

  //util_eraseFRAM(fram);
  //delay (1000);

  // Initialize config
  if (!g_config.init(fram))
  {
    DEBUGLN(F("System halt.  Unable to load config."));
    delay(100);
    while (1) delay(10);
  }
  DEBUGLN(F("Config loaded."));
  g_lcd.setCursor(0,2);
  g_lcd.print(F("Config loaded ...   "));
  g_LED_Yel_1.init(g_mcp, MCP_LED_YEL1_PIN);
  //delay (500);

  // Initialize Preset Manager
  if (!g_presets.init(fram, g_config))
  {
    DEBUGLN(F("System halt.  Unable to init Preset Manager."));
    delay(100);
    while (1) delay(10);
  }
  DEBUGLN(F("Preset Manager initialized."));
  g_lcd.setCursor(0,2);
  g_lcd.print(F("Presets loaded ...  "));
  g_LED_Yel_2.init(g_mcp, MCP_LED_YEL2_PIN);
  //delay (500);

  //Initialize Powder Manager
  if (!g_powders.init(fram, g_presets.getPowderIndex()))
  {
    DEBUGLN(F("System halt.  Unable to init Powder Manager."));
    delay(100);
    while (1) delay(10);
  }
  DEBUGLN(F("Powder Manager initialized."));
  g_lcd.setCursor(0,2);
  g_lcd.print(F("Powders loaded ...  "));
  g_LED_Grn.init(g_mcp, MCP_LED_GRN_PIN);
  //delay (500);

  //Set copied data in config from presets and powders
  char buff[NAME_LEN];
  buff[0] = 0x00;
  g_presets.getPresetName(buff);
  g_config.setPresetName(buff);
  buff[0] = 0x00;
  g_powders.getPowderName(buff);
  g_config.setPowderName(buff);
  g_config.setKernelFactor(g_powders.getPowderFactor());
  
  // Enable and configure TIC stepper motor drivers
  g_TIC_thrower.setProduct(TicProduct::T500);
  g_TIC_thrower.setStepMode(TIC_STEP_MODE);  
  g_TIC_thrower.setCurrentLimit(TIC_THROWER_CURRENT_LIMIT);  
  g_TIC_thrower.setMaxAccel(TIC_THROWER_MAX_ACCEL);
  g_TIC_thrower.setMaxDecel(TIC_THROWER_MAX_DECEL);
  g_TIC_thrower.setMaxSpeed(MAX_THROWER_SPEED * TIC_PULSE_MULTIPLIER);
  g_TIC_trickler.setProduct(TicProduct::T500);
  g_TIC_trickler.setStepMode(TIC_STEP_MODE);
  g_TIC_trickler.setCurrentLimit(TIC_TRICKLER_CURRENT_LIMIT);
  g_TIC_trickler.setStartingSpeed(TIC_TRICKLER_STARTING_SPEED);
  g_TIC_trickler.setMaxAccel(TIC_TRICKLER_MAX_ACCEL);
  g_TIC_trickler.setMaxDecel(TIC_TRICKLER_MAX_DECEL);
  g_TIC_trickler.setMaxSpeed(MAX_TRICKLER_SPEED * TIC_PULSE_MULTIPLIER);  
  g_TIC_trickler.setTargetVelocity(0);
  g_TIC_thrower.setTargetVelocity(0);
  g_TIC_trickler.exitSafeStart();
  g_TIC_thrower.exitSafeStart();
  DEBUGLN(F("TIC I2C drivers enabled"));
  g_lcd.setCursor(0,2);
  g_lcd.print(F("Step Drvrs enbld ..."));
  g_LED_Red.init(g_mcp, MCP_LED_RED_PIN);
  //delay (500);
  g_LED_Blu.setOn();
  updateLEDs();
  // Setup scale   
  while (!g_scale.isConnected())
  {
    if (g_scale.init(g_state, g_config))
    {
      DEBUGLN(F("Scale serial connection established."));
      g_lcd.setCursor(0,2);
      g_lcd.print(F("Scale connected.    "));
      delay (500);  
      while (!g_scale.isCalibrated())
      {
        calibrateScale();
        if (!g_scale.isCalibrated()) { 
          //try agian
          g_lcd.setCursor(0,2);
          g_lcd.print(F("Calbiration failed  "));
          g_lcd.setCursor(0,3);
          g_lcd.print(F("Press any button ..."));
          pauseForAnyButton();  
          g_lcd.setCursor(0,3);
          g_lcd.print(F("                    "));
        }
      }
      //calibrateTrickler();  //TODO: move this to the Manual Operation menu?
      setThrowerHome();
      //delay(10);
      g_scale.checkScale(); //update scale state
      g_lcd.setCursor(0,2);
      g_lcd.print(F("Scale calibrated.   "));
      g_lcd.setCursor(0,3);
      g_lcd.print(F("                    "));  
      delay (500);
    }
    else
    {
      //try again
      DEBUGLN(F("WARNING: Scale serial connection failed."));
      g_lcd.setCursor(0,2);
      g_lcd.print(F("Scale not connected."));
      g_lcd.setCursor(0,3);
      g_lcd.print(F("Press any button ..."));
      pauseForAnyButton();  
      g_lcd.setCursor(0,3);
      g_lcd.print(F("                    "));
    }
  } 
  g_LED_Blu.setOff();
  updateLEDs();   
  util_setFscaleCurve(g_curve_map, g_config.getFcurveP());
  g_lcd.setCursor(0,2);
  g_lcd.print(F("FCurve generated ..."));
  delay(500);

  g_LED_Blu.setOn();
  g_LED_Yel_1.setOn();
  g_LED_Yel_2.setOn();
  g_LED_Grn.setOn();
  g_LED_Red.setOn();
  updateLEDs();
  delay(1000);
  g_LED_Blu.setOff();
  g_LED_Yel_1.setOff();
  g_LED_Yel_2.setOff();
  g_LED_Grn.setOff();
  g_LED_Red.setOff();
  updateLEDs();
  g_lcd.setCursor(0,2);
  g_lcd.print(F("Setup complete.     "));
  g_lcd.setCursor(0, 3);
  g_lcd.print(F("Press any button ..."));
  dumpSystemEnv();
  pauseForAnyButton();
  
  g_state.setState(PTState::pt_menu);
  g_display_changed = true;
  displayUpdate();
  g_mcp.getLastInterruptPin();  //clear it again, just in case
}

/*
 * 
 */
void dumpSystemEnv()
{
#ifdef DEBUG_SERIAL
  DEBUGLN();
  DEBUGLN(F("***** System Configuration *****"));
  DEBUGLN();
  g_config.printConfig();
  DEBUGLN();
  DEBUGP(F("Presets addr base: "));
  DEBUGLN(PRESETS_ADDR_BASE);
  DEBUGP(F("Presets Size of each preset: "));
  DEBUGLN(PRESET_DATA_SIZE);
  DEBUGP(F("Powders addr base: "));
  DEBUGLN(POWDERS_ADDR_BASE);
  DEBUGP(F("Powders Size of each powder: "));
  DEBUGLN(POWDER_DATA_SIZE);
  
  g_scale.printConfig();
  DEBUGLN();
  DEBUGP(F("TIC step mode = "));
  switch (g_TIC_trickler.getStepMode())
  {
    case TicStepMode::Microstep1:
      DEBUGLN(F("Full step"));
      break;
    case TicStepMode::Microstep2:
      DEBUGLN(F("Half step"));
      break;
    case TicStepMode::Microstep4:
      DEBUGLN(F("1/4 step"));
      break;
    case TicStepMode::Microstep8:
      DEBUGLN(F("1/8th step"));
      break;
    default:
      DEBUGLN(F("Invalid value for TIC 500."));
  }
  DEBUGP(F("TIC Thrower current limit = "));
  DEBUGLN(g_TIC_thrower.getCurrentLimit());
  DEBUGP(F("TIC Thrower Max Speed = "));
  DEBUGLN(g_TIC_thrower.getMaxSpeed());
  DEBUGP(F("TIC Thrower Start Speed = "));
  DEBUGLN(g_TIC_thrower.getStartingSpeed());
  DEBUGP(F("TIC Thrower Max Accel = "));
  DEBUGLN(g_TIC_thrower.getMaxAccel());
  DEBUGP(F("TIC Thrower Max Decel = "));
  DEBUGLN(g_TIC_thrower.getMaxDecel());
  DEBUGP(F("TIC Thrower current pos = "));
  DEBUGLN(g_TIC_thrower.getCurrentPosition());
  DEBUGP(F("TIC Thrower steps to bottom pos = "));
  DEBUGLN(g_thrower_bottom_pos);
  DEBUGLN();
  DEBUGP(F("TIC Trickler current limit = "));
  DEBUGLN(g_TIC_trickler.getCurrentLimit());
  DEBUGP(F("TIC Trickler Max Speed = "));
  DEBUGLN(g_TIC_trickler.getMaxSpeed());
  DEBUGP(F("TIC Trickler Start Speed = "));
  DEBUGLN(g_TIC_trickler.getStartingSpeed());
  DEBUGP(F("TIC Trickler Max Accel = "));
  DEBUGLN(g_TIC_trickler.getMaxAccel());
  DEBUGP(F("TIC Trickler Max Decel = "));
  DEBUGLN(g_TIC_trickler.getMaxDecel());
  DEBUGLN();
#endif  
}

/*
 * During setup.
 * Calibrate the scale for pan on and pan off.
 */
void calibrateScale()
{
  // Pan on scale
  g_lcd.setCursor(0,2);
  g_lcd.print(F("Put pan on scale,   "));
  g_lcd.setCursor(0,3);
  g_lcd.print(F("Press any button ..."));
  pauseForAnyButton();
  g_scale.zeroScale();  
  // Pan off scale
  g_lcd.setCursor(0,2);
  g_lcd.print(F("Take pan off scale, "));
  g_lcd.setCursor(0,3);
  g_lcd.print(F("Press any button ..."));
  pauseForAnyButton();
  g_scale.checkScale(); //call to update weight
  g_scale.setOffScaleWeight();
}

/*
 * During setup.  (Move to manual?)
 * TODO: impliment
 */
void calibrateTrickler()
{
  DEBUGLN(F("TODO: calibrate trickler"));
  g_lcd.setCursor(0,2);
  g_lcd.print(F("TODO: cal trickler, "));
  g_lcd.setCursor(0,3);
  g_lcd.print(F("Press any button ..."));
  pauseForAnyButton();
 }

/*
 * During setup, 
 * DeEnergize setpper, have user manually move thrower to 
 * home pos and set in thrower controller.  Engergize stepper.
 */
 void setThrowerHome()
 {
  DEBUGLN(F("Calibrating thrower home"));
  if (g_TIC_thrower.getEnergized())
  {
    g_TIC_thrower.deenergize();
  }
  delay(100);
  g_lcd.setCursor(0,2);
  g_lcd.print(F("Move thrower home   "));
  g_lcd.setCursor(0,3);
  g_lcd.print(F("Press any button ..."));
  pauseForAnyButton();
  g_TIC_thrower.energize();
  delay(100);
  g_TIC_thrower.haltAndSetPosition(0); //set home position
  g_thrower_top_pos = 0;
  g_thrower_bottom_pos = THROWER_TRAVEL_DISTANCE; 
  g_TIC_thrower.exitSafeStart(); //clear safe start after deenergize
 }
