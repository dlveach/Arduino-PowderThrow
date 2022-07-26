/*
 * PTState.cpp - PowderThrow state machine control implementation.
 * Created by David Veach.  2022
 * Private repository
 */
#include "PTState.h"

/*
 * Constructor
 */
PTState::PTState(state_t state) 
{
  _state = state;
  _system_message = String("");
}

/*
 * Set current state
 */
int PTState::getState()
{
  return _state;
}

/*
 * Return current state
 */
void PTState::setState(state_t state)
{
  _state = state;
}

/* Set current system error messages */
void PTState::setSystemMessage(String msg) { _system_message = msg; }
void PTState::setSystemFileName(String file_name) { _system_file_name = file_name; }
void PTState::setSystemLineNo(String line_no) { _system_line_no = line_no; }

/* Get current system error messages */
 String PTState::getSystemMessage() { return _system_message; }
 String PTState::getSystemFileName() { return _system_file_name; }
 String PTState::getSystemLineNo() { return _system_line_no; }

/*
 * Return display friendly state name
 */
const char* PTState::getStateName()
{
  return StateNames[_state];
}

/*
 * Return display friendly state long name
 */
const char* PTState::getStateLongName()
{
  return StateLongNames[_state];
}

// ?????????????????????????????????????????
// ????? TODO ?????  move button logic here?
// ?????????????????????????????????????????

void PTState::BtnOK()
{
  //TODO:
}

void PTState::BtnUp()
{
  //TODO:
}

void PTState::BtnDown()
{
  //TODO:
}

void PTState::BtnLeft()
{
  //TODO:
}

void PTState::BtnRight()
{
  //TODO:
}

bool PTState::isValidState(int value) {
  switch (value) {
    case pt_undefined:
    case pt_error:
    case pt_setup:
    case pt_ready:
    case pt_throwing:
    case pt_trickling:
    case pt_bumping:
    case pt_paused:
    case pt_locked:
    case pt_menu:
    case pt_cfg:
    case pt_ladder:
    case pt_man_throw:
    case pt_man_trickle:
    case pt_man_cal_trickler:
    case pt_man_cal_scale:
    case pt_powders:
    case pt_powders_edit:
    case pt_presets:
    case pt_presets_edit:
      return(true);
      break;
    default:
      return(false);
      break;
  }  
}
