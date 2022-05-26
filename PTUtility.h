
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
void util_handleSystemError(String msg);
void util_setFscaleCurve(float* _curve_map, float _fscaleP);
void util_eraseFRAM(Adafruit_FRAM_I2C _fram);

#endif //PTUtility_h
