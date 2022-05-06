/********************************************************
 * PowderThrow.h - 
 */
#ifndef POWDER_THROW_DEFINITIONS_H
#define POWDER_THROW_DEFINITIONS_H

#include "Arduino.h"
#include <stdlib.h>
#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <LiquidCrystal_PCF8574.h>
#include <Tic.h>
#include "PTState.h"
#include "PTConfig.h"
#include "PTUtility.h"
#include "PTScale.h"
#include "PTPresets.h"
#include "PTPowders.h"
#include "PTLed.h"

//HARDWARE
#define LCD_I2C_ADDR 0x27     // PCF8574 I2C 20x4 LCD shield (SunFounder)
#define MCP_I2C_ADDR 0x20     // Adafruit MCP 23017 I/O expander
#define TIC1_I2C_ADDR 0x0E    // Pololu TIC 500 Stepper Controller
#define TIC2_I2C_ADDR 0x0F    // Pololu TIC 500 Stepper Controller
#define FRAM_I2C_ADDR 0x50    // Adafruit FRAM breakout board
#define BTN_OK 3              // MCP23017 button pin 
#define BTN_LEFT 4            // MCP23017 button pin 
#define BTN_UP 5              // MCP23017 button pin 
#define BTN_DOWN 6            // MCP23017 button pin 
#define BTN_RIGHT 7           // MCP23017 button pin 
#define INT_PIN 7             // MCU pin for MCP interrupt
#define MCP_LED_RED_PIN 8
#define MCP_LED_GRN_PIN 9
#define MCP_LED_YEL2_PIN 10
#define MCP_LED_YEL1_PIN 11
#define MCP_LED_BLU_PIN 12
 

//TIC hardware settings
#define TIC_PULSE_MULTIPLIER 10000      // TIC uses pulses/10,000 Seconds for speed. See TIC documentation.
#define TIC_STEP_MODE TicStepMode::Microstep8  //TIC microstep mode.  WARNING: Many things need adjusting if changed
#define TIC_TRICKLER_CURRENT_LIMIT 350  //TIC current limit in milliamps (see TIC docs)
#define TIC_TRICKLER_MAX_ACCEL 500000   //steps per second per second
#define TIC_TRICKLER_MAX_DECEL 5000000  //steps per second per second
#define TIC_TRICKLER_STARTING_SPEED 100 * TIC_PULSE_MULTIPLIER    //steps per second
#define TIC_THROWER_CURRENT_LIMIT 500   //TIC current limit in milliamps (see TIC docs)
#define TIC_THROWER_MAX_ACCEL 1000000   //steps per second per second
#define TIC_THROWER_MAX_DECEL 1000000   //steps per second per second

//TODO: move these to config for tuneabilty?
#define THROWER_TRAVEL_DISTANCE 1350    //1600 pulses = 1 turn
#define MAX_THROWER_SPEED 4000          //pulses per sec (1/8 micro step, 150rpm) 
#define MAX_TRICKLER_SPEED -4000        //pulses per sec (1/8 micro step, 150rpm) 
#define THROWER_DWELL_TIME 1200         //time to dwell at end of throw & let powder drop/fill
#define SYSTEM_LOCK_TIME 5000           //time to stay locked
#define SYSTEM_PAUSE_TIME 1000          //time to pause & let scale settle when on target 
#define BUMP_INTERVAL 800               //bump interval  
#define BUMP_DISTANCE 75                //steps to bump  

//GUI config edit settings
#define DECEL_THRESH_INC 0.1
#define DECEL_LIMIT_INC 100
#define DECEL_LIMIT_INC_LIMIT 1000
#define DECEL_LIMIT_DEC_LIMIT 100
#define BUMP_THRESH_INC 0.01
#define FCURVEP_INC 0.1
#define FCURVEP_INC_LIMIT 10.0
#define FCURVEP_DEC_LIMIT -10.0

//SYSTEM RUN 
#define DEBOUNCE 500                    //Button debounce time millis
#define RUN_INTERVAL 50                 //main loop system run interval, 50ms
#define SERIAL_TIMEOUT 100              //Millis before serial comm timeout

/*
 * System wide globals
 */
Adafruit_MCP23X17 g_mcp;
LiquidCrystal_PCF8574 g_lcd(LCD_I2C_ADDR);  
TicI2C g_TIC_trickler(14);
TicI2C g_TIC_thrower(15);

PTConfig g_config;
PTState g_state(PTState::pt_undefined);
PTScale g_scale;
PresetManager g_presets;
PowderManager g_powders;
PTLed g_LED_Blu;
PTLed g_LED_Yel_1;
PTLed g_LED_Yel_2;
PTLed g_LED_Grn;
PTLed g_LED_Red;

float g_curve_map[101];  // Curve for trickler slowdown
int g_trickler_cal_speed = MAX_TRICKLER_SPEED;  //init at max until calibrated
int g_thrower_top_pos = -1; //not calibrated by default
int g_thrower_bottom_pos = -1;  //not calibrated by default

//display globals
bool g_display_changed = false;
int g_cur_line = 0;
int g_prev_line = 0;
int g_cursor_pos = 0;
bool g_disp_edit = false;

//function prototypes
void setTricklerSpeed(bool force=false);

#endif  // POWDER_THROW_DEFINITIONS_H
