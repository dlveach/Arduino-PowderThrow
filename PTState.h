/*
 * PTState.h - PowderThrow state machine control implementation.
 * Created by David Veach.  2022
 * Private repository
 */

#include "Arduino.h"

#ifndef PTState_h
#define PTState_h

static const char* StateNames[] = {"Undefined","Error","Setup","Cal Scale","Cal Trickler","Ready","Throwing",
    "Trickling","Bumping","Paused","Locked","Menu","Config","Cfg Bump",
    "Cfg Decel Lim","Cfg Decel Tgt","Cfg FCurve","Manual","Man Throw","Man Trickle","Man Bump",
    "Powders","Edit Powder","Presets","Edit Preset",};
    
class PTState
{
  public:    
    enum state_t 
    {
      pt_undefined,
      pt_error,
      pt_setup,
      pt_cal_scale,
      pt_cal_trickler,
      pt_ready,
      pt_throwing,
      pt_trickling,
      pt_bumping,
      pt_paused,
      pt_locked,
      pt_menu,
      pt_cfg,
      pt_cfg_bump,  //TODO: not used?
      pt_cfg_dec_lim,  //TODO: not used?
      pt_cfg_dec_tgt,  //TODO: not used?
      pt_cfg_fcurve,  //TODO: not used?
      pt_man,
      pt_man_throw,
      pt_man_trickle,
      pt_man_bump,
      pt_powders,
      pt_powders_edit,
      pt_presets,
      pt_presets_edit,
    };

    // Constructor
    PTState(state_t state = pt_undefined);

    // Getters/Setters
    int setState(state_t state);
    int getState();
    const char* getStateName();
    void setSystemMessage(String msg);
    String getSystemMessage();
    void BtnOK();
    void BtnUp();
    void BtnDown();
    void BtnLeft();
    void BtnRight();
    
  private:
    state_t _state;
    String _system_message;
};

#endif  // PTState_h
