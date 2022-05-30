/*
 * PTScale.h
 * 
 * A&D FXi Scale Communication and support
 */
#include "Arduino.h"
#include "PTState.h"
#include "PTConfig.h"

#ifndef PTSCALE_H
#define PTSCALE_H 

#define SCALE_MODE_GRAM 0
#define SCALE_MODE_GRAIN 1
#define SERIAL_TIMEOUT 100          //Millis before serial comm timeout
#define MIN_CALIBRATION_WEIGHT -20  //in grains.  A "dead zone" in case scale drifts a tiny bit off zero.
#define SCALE_MAX_CON_FAILS 20      // Max serial communication fails before setting connected = false.

static float GM_TO_GN_FACTOR = 0.06479891; // grams per grain conversion factor
static const char* ConditionNames[] = {"Not Ready","Zero","Pan Off","Under","Close","Very Cls","On Tgt","Over Tgt","Undef"};
static const char* ConditionLongNames[] = {"Not Ready","Zero","Pan Off","Under Target","Close to Target","Very Close to Target","On Target","Over Target","Undefined"};
static const char* ModeNames[] = {"Milligram","Grain"};

/*
 * Class PTScale
 */
class PTScale
{
  public:

    enum condition_t 
    {
      not_ready,
      zero,
      pan_off,
      under_tgt,
      close_to_tgt,
      very_close_to_tgt,
      on_tgt,
      over_tgt,
      undef,
    };

    // Constructor
    PTScale();

    // Vars

    // Getters/Setters
    void setTarget(float);
    float getTarget();
    float getDelta();
    float getWeight();
    float getKernels();
    int getCondition();
    const char* getConditionName();
    const char* getConditionLongName();
    const char* getModeName();
    int getMode();
    //bool isChanged();
    bool isStable();
    void setOffScaleWeight();
    float getOffScaleWeight();
    bool isConnected();
    void printConfig();
    bool isCalibrated();
    
    // Actors
    boolean init(PTState, PTConfig);
    void checkScale();
    void zeroScale();

  private:
  
    // Vars
    PTState _ptstate;         // The PTState object.
    PTConfig _ptconfig;       // The PTConfig object.
    float _target;            // The weight target.
    bool _calibrated;         // Has the setup calibration been done successfully
    bool _connected;          // State of Serial connection to scale
    int _con_fail_count;      // Number of consecutive connection failures
    float _delta;             // Delta to target weight.
    float _weight;            // Current measured weight.
    int _cond;                // Condition of the scale (PTScale::condition_t enum).
    int _mode;                // Scale setting: Grains or Grams (read from serial).
    boolean _serial_lock;     // Mutex for Serial1 communication.
    boolean _stable;          // True if stable or false if not.
    float _kernels;           // #kernels of powder off target
    float _off_scale_weight;  //weght of empty platter during calibration

    // A&D FXi scale stuff
    char _weight_data[10] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', 0}; //always null terminated!
    const char _cmd_reqData[3] = {'Q', 10, 13}; // scale command: request data immediately
    const char _cmd_reZero[3] = {'Z', 10, 13}; // scale command: re-zero the scale

    // Functions

};

#endif //PTSCALE_H
