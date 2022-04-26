
#ifndef PTUtility_h
#define PTUtility_h

#define DEBUG_SERIAL TRUE //Comment out to disable Serial debug
#ifdef DEBUG_SERIAL
  #define DEBUGP(x) Serial.print(x)
  #define DEBUGLN(x) Serial.println(x)
  #define SERIAL_ENABLED    F("Serial enabled")
  #define MCP_I2C_ERROR     F("MCP I2C comm err") 
  #define MCP_I2C_ENABLED   F("MCP I2C Enabled")
  #define FRAM_I2C_ENABLED  F("FRAM I2C Enabled")
  #define FRAM_I2C_ERROR    F("FRAM I2C comm err")
  #define LCD_I2C_ENABLED   F("LCD I2C Enabled")
  #define LCD_I2C_ERROR     F("LCD I2C comm err: ")
  #define TIC_I2C_ENABLED   F("TIC I2C drivers enabled")
#else
  #define DEBUGP(x)
  #define DEBUGLN(x)
#endif 


/******************************************************
 * Funciton templates
 */
void util_handleSystemError(String msg);
void util_setFscaleCurve(float* _curve_map, float _fscaleP);
void util_eraseFRAM(Adafruit_FRAM_I2C _fram);

#endif //PTUtility_h
