
/*
 * Circular increment an alpha-numeric character.
 * Supports ' ', 'A-Z', '0-9'
 */
char incChar(char c) {
  if ((c < ' ') || (c > 'Z')) { c = 'A'; }
  else if (c == 'Z') { c = ' '; }
  else if (c == ' ') { c = '0'; }
  else if (c == '9') { c = 'A'; }
  else { c++; }
  return c;
}

/*
 * Circular decrement an alpha-numeric character.
 * Supports ' ', 'A-Z', '0-9'
 */
char decChar(char c) {
  if ((c < ' ') || (c > 'Z')) { c = 'A'; }
  else if (c == ' ') { c = 'Z'; }
  else if (c == 'A') { c = '9'; }
  else if (c == '0') { c = ' '; }
  else { c--; }
  return c;
}

void printBytes(byte addr[], int byte_count) {
  char buff[100];
  Serial.print("printBytes() byte_count: ");
  Serial.println(byte_count);
  Serial.print("Address: ");
  Serial.println((unsigned int)&addr[0], HEX);
  Serial.print("Bytes: ");
  for (int i=0; i<byte_count-1; i++) {
    sprintf(buff, "[%d],", addr[i]);
    Serial.print(buff);
  }
  sprintf(buff, "[%d]", addr[byte_count-1]);
  Serial.println(buff);  
}

/*** Overwrite entire FRAM with 0x00 to erase all storage. ***/
void util_eraseFRAM(Adafruit_FRAM_I2C _fram) {
  DEBUGP(F("Zeroing out FRAM "));
  uint16_t addr = CONFIG_DATA_ADDR;
  int idx = 0;
  while ((addr + idx) < FRAM_SIZE) {
    _fram.write8(addr + idx++, 0x00);
    if ((idx % 1000) == 0) {
      DEBUGP(".");
      delay(250);  // slow it down a touch
    }
  }
  DEBUGLN(".");
  DEBUGLN(F("FRAM zeroed out."));
}

//TODO: filename and line no in displaySystemError
void logError(String errMsg, String fileName, int lineNo, bool fatal) {
  // Using Strings here ok, probably fatal anyway.
  String msg = "ERROR: " + errMsg;
  if (fileName != "") { msg = msg + ": " + fileName; }
  if (lineNo > 0) { msg = msg + " at " + String(lineNo); }
  DEBUGLN(msg);
  if (fatal) { displaySystemError(); }
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
