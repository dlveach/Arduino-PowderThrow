
/*
 * Circular increment an alpha-numeric character.
 * Supports ' ', 'A-Z', 'a-z', '0-9'
 */
char incChar(char c) {
  if ((c < ' ') || (c > 'z')) { c = ' '; }
  else if (c == 'z') { c = ' '; }
  else if (c == ' ') { c = '0'; }
  else if (c == '9') { c = 'A'; }
  else if (c == 'Z') { c = 'a'; }
  else { c++; }
  return c;
}

/*
 * Circular decrement an alpha-numeric character.
 * Supports ' ', 'A-Z', 'a-z', '0-9'
 */
char decChar(char c) {
  if ((c < ' ') || (c > 'z')) { c = ' '; }
  else if (c == ' ') { c = 'z'; }
  else if (c == 'a') { c = 'Z'; }
  else if (c == 'A') { c = '9'; }
  else if (c == '0') { c = ' '; }
  else { c--; }
  return c;
}

void printHexData(const unsigned char data[], int len) {
#ifdef DEBUG_SERIAL
  for (int i = 0; i < len; i++) {
    unsigned char b = data[i];
    Serial.print(b, HEX);
  }
  Serial.println();
#endif  
}

void printStringData(const unsigned char data[], int len) {
#ifdef DEBUG_SERIAL
  char c;
  for (int i = 0; i < len; i++) {
    c = toascii(data[i]);
    Serial.print(c);
  }
  Serial.println();
#endif  
}

void printByteData(const unsigned char data[], int len) {
#ifdef DEBUG_SERIAL
  static char buff[5];
  Serial.print("[");
  for (int i = 0; i < len - 1; i++) {
    sprintf(buff, "%d", byte(data[i]));
    Serial.print(buff);
    if (((i+1) % 10) == 0) { Serial.print(" | ");
    } else { Serial.print(", "); }
  }
  sprintf(buff, "%d]", byte(data[len - 1]));
  Serial.print(buff);
  Serial.println();
#endif  
}

/*** Overwrite entire FRAM with 0x00 to erase all storage. ***/
//TODO: change this to write defaults into config, presets, and powders
void util_eraseFRAM(Adafruit_FRAM_I2C _fram) {
  DEBUGP(F("Zeroing out FRAM "));
  uint16_t addr = CONFIG_DATA_ADDR;
  int idx = 0;
  while ((addr + idx) < FRAM_SIZE) {
    _fram.write8(addr + idx++, 0x00);
    if ((idx % 1000) == 0) {
      #ifdef DEBUG_SERIAL
      DEBUGP(".");
      delay(250);  // slow it down a touch more when debugging.
      #else
      delay(10);
      #endif
    }
  }
  DEBUGLN(".");
  DEBUGLN(F("FRAM zeroed out."));
}

void logError(String err_msg, String file_name, int line_no, bool fatal) {
  // Using Strings here ok, probably fatal anyway.
  String msg = "ERROR: " + err_msg;
  if (file_name != "") {
    file_name = file_name.substring(file_name.lastIndexOf('/')+1); 
  }
  if (file_name != "") { msg = msg + ": " + file_name; }
  if (line_no > 0) { msg = msg + " at " + String(line_no); }
  if (fatal) {
    Serial.println(msg);
  } else {
    DEBUGLN(msg);
  }
  if (fatal) { 
    g_state.setState(PTState::pt_error);
    msg = err_msg.substring(0,19);
    g_state.setSystemMessage(msg);
    if (file_name != "") { g_state.setSystemFileName(file_name); } else { g_state.setSystemFileName(""); }
    if (line_no > 0) { g_state.setSystemLineNo(String(line_no)); } else { g_state.setSystemLineNo(""); }
    displaySystemError(); 
  }
}

/*** Handle System Error. ***/
void util_handleSystemError(String msg) {
  g_state.setState(PTState::pt_error);
  g_state.setSystemMessage(msg);
  displaySystemError();
}

/********************************************
   FLOATING POINT AUTOSCALE FUNCTION
   By: Paul Badger 2007
   Modified from code by Greg Shakar
   http://playground.arduino.cc/Main/Fscale

 ********************************************/
void util_setFscaleCurve(float* _curve_map, float _fscaleP = 1.0) {
  for (int j = 1; j <= 100; j++) {
    _curve_map[j] = fscale(0, 100, 0, 100, j, _fscaleP);
  }
  //dumpFCurve(); // Uncomment to debug
}
float fscale(float originalMin, float originalMax, float newBegin, float newEnd, float inputValue, float curve) {
  float OriginalRange = 0;
  float NewRange = 0;
  float zeroRefCurVal = 0;
  float normalizedCurVal = 0;
  float rangedValue = 0;
  boolean invFlag = 0;

  // condition curve parameter
  // limit range
  if (curve > 10) curve = 10;
  if (curve < -10) curve = -10;

  curve = (curve * -.1) ; // - invert and scale - this seems more intuitive - postive numbers give more weight to high end on output
  curve = pow(10, curve); // convert linear scale into lograthimic exponent for other pow function

  // Check for out of range inputValues
  if (inputValue < originalMin) { inputValue = originalMin; }
  if (inputValue > originalMax) { inputValue = originalMax; }

  // Zero Refference the values
  OriginalRange = originalMax - originalMin;

  if (newEnd > newBegin) { 
    NewRange = newEnd - newBegin; 
  } else {
    NewRange = newBegin - newEnd;
    invFlag = 1;
  }

  zeroRefCurVal = inputValue - originalMin;
  normalizedCurVal  =  zeroRefCurVal / OriginalRange;   // normalize to 0 - 1 float

  // Check for originalMin > originalMax  - the math for all other cases i.e. negative numbers seems to work out fine
  if (originalMin > originalMax ) { return 0; }

  if (invFlag == 0) {
    rangedValue =  (pow(normalizedCurVal, curve) * NewRange) + newBegin;
  } else { 
    // invert the ranges
    rangedValue =  newBegin - (pow(normalizedCurVal, curve) * NewRange);
  }
  return rangedValue;
}

/*
 * Testing function.  Dump the FCurve to Serial.
 */
void dumpFCurve() {
  for (int i=1; i<=100; ++i) {
    DEBUGP(F("FCurve["));
    DEBUGP(i);
    DEBUGP(F("] = "));
    DEBUGLN(g_curve_map[i]);
  }
}

/*
 * enum class TicError
   45 {
   46   IntentionallyDeenergized = 0,
   47   MotorDriverError         = 1,
   48   LowVin                   = 2,
   49   KillSwitch               = 3,
   50   RequiredInputInvalid     = 4,
   51   SerialError              = 5,
   52   CommandTimeout           = 6,
   53   SafeStartViolation       = 7,
   54   ErrLineHigh              = 8,
   55   SerialFraming            = 16,
   56   RxOverrun                = 17,
   57   Format                   = 18,
   58   Crc                      = 19,
   59   EncoderSkip              = 20,
   60 };
 */
#ifdef DEBUG_SERIAL
void debugTICErrors(uint32_t errors) {
  if (errors & (1 << (uint8_t)TicError::IntentionallyDeenergized)) { Serial.println(F("IntentionallyDeenergized error")); }
  if (errors & (1 << (uint8_t)TicError::MotorDriverError)) { Serial.println(F("motor driver error")); }
  if (errors & (1 << (uint8_t)TicError::LowVin)) { Serial.println(F("LowVin error")); }
  if (errors & (1 << (uint8_t)TicError::KillSwitch)) { Serial.println(F("KillSwitch  error")); }
  if (errors & (1 << (uint8_t)TicError::RequiredInputInvalid)) { Serial.println(F("RequiredInputInvalid  error")); }
  if (errors & (1 << (uint8_t)TicError::SerialError)) { Serial.println(F("SerialError  error")); }
  if (errors & (1 << (uint8_t)TicError::CommandTimeout)) { Serial.println(F("CommandTimeout  error")); }
  if (errors & (1 << (uint8_t)TicError::SafeStartViolation)) { Serial.println(F("SafeStartViolation  error")); }
  if (errors & (1 << (uint8_t)TicError::ErrLineHigh)) { Serial.println(F("ErrLineHigh  error")); }
  if (errors & (1 << (uint8_t)TicError::SerialFraming)) { Serial.println(F("SerialFraming  error")); }
  if (errors & (1 << (uint8_t)TicError::RxOverrun)) { Serial.println(F("RxOverrun  error")); }
  if (errors & (1 << (uint8_t)TicError::Format)) { Serial.println(F("Format  error")); }
  if (errors & (1 << (uint8_t)TicError::Crc)) { Serial.println(F("Crc  error")); }
  if (errors & (1 << (uint8_t)TicError::EncoderSkip)) { Serial.println(F("EncoderSkip  error")); }
}
#else
void debugTICErrors(uint32_t errors) { return; }
#endif
