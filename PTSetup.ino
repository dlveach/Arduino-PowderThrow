  #define SERIAL_ENABLED    F("Serial enabled")
  #define MCP_I2C_ERROR     F("MCP I2C comm err") 
  #define MCP_I2C_ENABLED   F("MCP I2C Enabled")
  #define FRAM_I2C_ENABLED  F("FRAM I2C Enabled")
  #define FRAM_I2C_ERROR    F("FRAM I2C comm err")
  #define LCD_I2C_ENABLED   F("LCD I2C Enabled")
  #define LCD_I2C_ERROR     F("LCD I2C comm err: ")
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
  DEBUGLN(SERIAL_ENABLED);
  #endif

  delay (500);
  
  // Check for LCD display
  int error;
  Wire.beginTransmission(0x27);
  error = Wire.endTransmission();
  if (error == 0) {
    g_lcd.begin(20, 4);
    delay(10);
    DEBUGLN(LCD_I2C_ENABLED);
    g_lcd.setBacklight(255);
    g_lcd.clear();  
    g_lcd.setCursor (0, 0);
    g_lcd.print("PowderThrow 2.0");
    g_lcd.setCursor(0,2);
    g_lcd.print(F("System init ...     "));
  } else {
    DEBUGP(LCD_I2C_ERROR);
    DEBUGP(error);
    while (1) delay(10);
  }  
  delay(1000);
  
  // Check for MCP expander
  if (!g_mcp.begin_I2C()) 
  {
    DEBUGLN(MCP_I2C_ERROR);
    while (1) delay(10);
  }
  DEBUGLN(MCP_I2C_ENABLED);
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
  delay(1000);
  g_mcp.getLastInterruptPin();  //clear it just in case
  delay(10);
  g_mcp.getLastInterruptPin();  //be really sure!!!

  // Initialize FRAM
  Adafruit_FRAM_I2C fram = Adafruit_FRAM_I2C();
  if (!fram.begin())
  {  
    DEBUGLN(FRAM_I2C_ERROR);
    util_handleSystemError(FRAM_I2C_ERROR);
  }
  DEBUGLN(FRAM_I2C_ENABLED);
  g_lcd.setCursor(0,2);
  g_lcd.print(F("FRAM initialized ..."));
  delay (1000);

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
  g_config.printConfig(); // Uncomment to debug
  delay (1000);

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
  DEBUGP(F("Presets addr base: "));
  DEBUGLN(PRESETS_ADDR_BASE);
  DEBUGP(F("Size of each preset: "));
  DEBUGLN(PRESET_DATA_SIZE);
  delay (1000);

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
  DEBUGP(F("Powders addr base: "));
  DEBUGLN(POWDERS_ADDR_BASE);
  DEBUGP(F("Size of each powder: "));
  DEBUGLN(POWDER_DATA_SIZE);
  delay (1000);

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
#ifdef DEBUG_SERIAL
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
  DEBUGP(F("Thrower TIC current limit = "));
  DEBUGLN(g_TIC_thrower.getCurrentLimit());
  DEBUGP(F("Thrower TIC Max Speed = "));
  DEBUGLN(g_TIC_thrower.getMaxSpeed());
  DEBUGP(F("Thrower TIC Start Speed = "));
  DEBUGLN(g_TIC_thrower.getStartingSpeed());
  DEBUGP(F("Thrower TIC Max Accel = "));
  DEBUGLN(g_TIC_thrower.getMaxAccel());
  DEBUGP(F("Thrower TIC Max Decel = "));
  DEBUGLN(g_TIC_thrower.getMaxDecel());
  DEBUGP(F("Trickler TIC current limit = "));
  DEBUGLN(g_TIC_trickler.getCurrentLimit());
  DEBUGP(F("Trickler TIC Max Speed = "));
  DEBUGLN(g_TIC_trickler.getMaxSpeed());
  DEBUGP(F("Trickler TIC Start Speed = "));
  DEBUGLN(g_TIC_trickler.getStartingSpeed());
  DEBUGP(F("Trickler TIC Max Accel = "));
  DEBUGLN(g_TIC_trickler.getMaxAccel());
  DEBUGP(F("Trickler TIC Max Decel = "));
  DEBUGLN(g_TIC_trickler.getMaxDecel());
#endif
  g_TIC_trickler.setTargetVelocity(0);
  g_TIC_thrower.setTargetVelocity(0);
  g_TIC_trickler.exitSafeStart();
  g_TIC_thrower.exitSafeStart();
  DEBUGLN(F("TIC I2C drivers enabled"));
  g_lcd.setCursor(0,2);
  g_lcd.print(F("Step Drvrs enbld ..."));
  delay (1000);
  
  // Setup scale   TODO: a retry loop?
  if (g_scale.init(g_state, g_config))
  {
    DEBUGLN(F("Scale serial connection established."));
    g_lcd.setCursor(0,2);
    g_lcd.print(F("Scale connected.    "));
    delay (1000);  
    calibrateScale();
    //calibrateTrickler();  //TODO: move this to the Manual Operation menu?
    setThrowerHome();
    delay(10);
    g_scale.checkScale(); //update scale state
    g_lcd.setCursor(0,2);
    g_lcd.print(F("Scale calibrated.   "));
    g_lcd.setCursor(0,3);
    g_lcd.print(F("                    "));  
    delay (10);
    //DEBUG
    DEBUGP(F("Pan Off weight = "));
    DEBUGLN(g_scale.getOffScaleWeight());    
    DEBUGP(F("Scale cond: "));
    DEBUGLN(g_scale.getCondition());
    delay (1000);
  }
  else
  {
    DEBUGLN(F("WARNING: Scale serial connection failed."));
    g_lcd.setCursor(0,2);
    g_lcd.print(F("Scale not connected."));
    g_lcd.setCursor(0,3);
    g_lcd.print(F("Press any button ..."));
    pauseForAnyButton();  
    g_lcd.setCursor(0,3);
    g_lcd.print(F("                    "));
  }
    
  util_setFscaleCurve(g_curve_map, g_config.getFcurveP());
  g_lcd.setCursor(0,2);
  g_lcd.print(F("FCurve generated ..."));
 
  delay(1000);
  g_state.setState(PTState::pt_menu);
  g_display_changed = true;
  displayUpdate();
  g_mcp.getLastInterruptPin();  //clear it just in case
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
  DEBUGP(F("Scale pan off, weight ="));
  DEBUGLN(g_scale.getWeight());
  g_scale.setOffScaleWeight();
  DEBUGP("After setting pan_off_scale_weight: ");
  DEBUGLN(g_scale.getOffScaleWeight());
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
  //TODO: set a state to indicate calibrated?
  //TODO: is tic.getPositionUncertain() enough?
  DEBUGLN(F("Thrower home pos = 0"));
  DEBUGP(F("Thrower current pos = "));
  DEBUGLN(g_TIC_thrower.getCurrentPosition());
  DEBUGP(F("Thrower bottom pos = "));
  DEBUGLN(g_thrower_bottom_pos);
 }
