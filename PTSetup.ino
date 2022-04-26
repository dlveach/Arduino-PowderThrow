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
    
  // Enable motor drives
  g_tic1.setTargetVelocity(0);
  g_tic2.setTargetVelocity(0);
  g_tic1.exitSafeStart();
  g_tic2.exitSafeStart();
  DEBUGLN(TIC_I2C_ENABLED);
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
    calibrateTrickler();  //move this to the Manual Operation menu?
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
  //dumpFCurve(); // Uncomment to debug
 
  delay(1000);
  g_state.setState(PTState::pt_menu);
  g_display_changed = true;
  displayUpdate();
  g_mcp.getLastInterruptPin();  //clear it just in case
  //g_last_loop_time = millis();
}

/*
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
  g_lcd.print(F("                    "));
  g_scale.zeroScale();
  
  // Pan off scale
  g_lcd.setCursor(0,2);
  g_lcd.print(F("Take pan off scale, "));
  g_lcd.setCursor(0,3);
  g_lcd.print(F("Press any button ..."));
  pauseForAnyButton();
  g_scale.setOffScaleWeight();

  g_lcd.setCursor(0,2);
  g_lcd.print(F("Scale calibrated,   "));
  g_lcd.setCursor(0,3);
  g_lcd.print(F("Press any button ..."));
  pauseForAnyButton();
  g_lcd.setCursor(0,3);
  g_lcd.print(F("                    "));
  //TODO: set a state?
}

/*
 * 
 */
void calibrateTrickler()
{
  DEBUGLN(F("TODO: calibrate trickler"));
  g_lcd.setCursor(0,2);
  g_lcd.print(F("TODO: cal trickler, "));
  g_lcd.setCursor(0,3);
  g_lcd.print(F("Press any button ..."));
  pauseForAnyButton();
  g_lcd.setCursor(0,3);
  g_lcd.print(F("                    "));
  //TODO: set a state?
 }
