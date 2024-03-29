/*
 * PTState.h - PowderThrow state machine control implementation.
 * Created by David Veach.  2022
 * Private repository
 */

#include "Arduino.h"

#ifndef PTState_h
#define PTState_h

// TODO: change config -> Settings
static const char* StateNames[] = {"Undefined","Error","Setup","Ready","Manual","Manual Run","Ladder",
  "Ladder Run","Throwing","Trickling","Bumping","Paused","Locked","Menu","Config","Man Throw",
  "Man Trickle","Cal Trickler", "Cal Scale","Powders","Edit Powder","Presets","Edit Preset",};
    
static const char* StateLongNames[] = {"Undefined","Error","Setup","Ready","Manual","Manual Run","Ladder",
  "Ladder Run","Throwing","Trickling","Bumping","Paused","Locked","Menu","Config","Manual Throw",
  "Manual Trickle","Calibrate Trickler", "Calibrate Scale","Powders","Edit Powder","Presets","Edit Preset",};
    
class PTState {
  public:    
    enum state_t {
      pt_undefined,
      pt_error,
      pt_setup,
      pt_ready,
      pt_manual,
      pt_manual_run,
      pt_ladder,
      pt_ladder_run,
      pt_throwing,
      pt_trickling,
      pt_bumping,
      pt_paused,
      pt_locked,
      pt_menu,
      pt_cfg,
      pt_man_throw,
      pt_man_trickle,
      pt_man_cal_trickler,
      pt_man_cal_scale,
      pt_powders,
      pt_powders_edit,
      pt_presets,
      pt_presets_edit,
    };

    // Constructor
    PTState(state_t state = pt_undefined);

    // Getters/Setters
    void setState(state_t state);
    int getState();
    const char* getStateName();
    const char* getStateLongName();
    void setSystemMessage(String);
    String getSystemMessage();
    void setSystemFileName(String);
    String getSystemFileName();
    void setSystemLineNo(String);
    String getSystemLineNo();
    void BtnOK();
    void BtnUp();
    void BtnDown();
    void BtnLeft();
    void BtnRight();
    bool isValidState(int);
    
  private:
    state_t _state;
    String _system_message;
    String _system_file_name;
    String _system_line_no;
};

#endif  // PTState_h
