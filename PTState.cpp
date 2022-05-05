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
int PTState::setState(state_t state)
{
  _state = state;
}

/*
 * Set current system message
 */
void PTState::setSystemMessage(String msg)
{
  _system_message = msg;
}

/*
 * Get current system message
 */
 String PTState::getSystemMessage()
 {
  return _system_message;
 }

/*
 * Return display friendly state name
 */
const char* PTState::getStateName()
{
  return StateNames[_state];
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
  
}

void PTState::BtnRight()
{
  
}
