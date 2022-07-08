
#ifndef PTUtility_h
#define PTUtility_h

//#define DEBUG_SERIAL  // uncomment for serial console debugging
#ifdef DEBUG_SERIAL
#define DEBUGLN(x)  Serial.println (x)
#define DEBUGP(x)   Serial.print (x)
#else
#define DEBUGLN(x)
#define DEBUGP(x)
#endif

/******************************************************
 * Funciton templates
 */
void logError(String errMsg = "", String fileName = "", int lineNo = -1, bool fatal = false);
void util_handleSystemError(String msg = (F("Unknown sys error.")));
void util_setFscaleCurve(float* _curve_map, float _fscaleP);
void util_eraseFRAM(Adafruit_FRAM_I2C _fram);
char incChar(char);
char decChar(char);
void printBytes(byte[], int);


#endif //PTUtility_h
