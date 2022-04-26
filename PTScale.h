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

#define SCALE_MODE_GRAIN 1
#define SCALE_MODE_GRAM 2
#define SCALE_POLLING_RATE 20   //millis

// grams per grain conversion factor
static float GM_TO_GN_FACTOR = 0.06479891; 


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
      on_tgt,
      over_tgt,
      undef,
    };

    // Constructor
    PTScale();

    // Vars

    // Getters/Setters
    float getDelta();
    float getWeight();
    float getKernels();
    int getCondition();
    int getMode();
    bool isChanged();
    bool isStable();
    void setOffScaleWeight();
    bool isConnected();
    
    // Actors
    boolean init(PTState, PTConfig);
    void checkScale();
    void zeroScale();
    void setTarget(float);
    float getTarget();

  private:
  
    // Vars
    PTState _ptstate;   // The PTState object.
    PTConfig _ptconfig;   // The PTConfig object.
    float _target;    // The weight target.
    bool _connected;  // State of Serial connection to scale
    float _delta;   // Delta to target weight.
    float _weight;  // Current measured weight.
    int _cond;      // Condition of the scale (PTScale::condition_t enum).
    int _mode;      // Scale setting: Grains or Grams (read from serial).
    boolean _serial_lock; // Mutex for Serial1 communication.
    long _last_poll;    // Last loop time scale was polled
    boolean _display_changed; // True if new data to display.
    boolean _stable;  // True if stable or false if not.
    float _kernels;   // #kernels of powder off target
    float _off_scale_weight;  //weght of empty platter during calibration

    char _weight_data[10] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', 0}; //always null terminated!
    // A&D FXi scale commands
    const char _cmd_reqData[3] = {'Q', 10, 13}; // scale command: request data immediately
    const char _cmd_reZero[3] = {'Z', 10, 13}; // scale command: re-zero the scale

    // Functions
    boolean _readSerialData();

};

#endif //PTSCALE_H
