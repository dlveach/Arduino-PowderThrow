/********************************************************
 * PowderThrow.h - 
 */
#ifndef POWDER_THROW_DEFINITIONS_H
#define POWDER_THROW_DEFINITIONS_H

#define DEBUG_SERIAL // uncomment for serial console debugging

#ifdef DEBUG_SERIAL
#define DEBUGLN(x)  Serial.println (x)
#define DEBUGP(x)   Serial.print (x)
#else
#define DEBUGLN(x)
#define DEBUGP(x)
#endif

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


#define LCD_I2C_ADDR 0x27     // PCF8574 I2C 20x4 LCD shield (SunFounder)
#define MCP_I2C_ADDR 0x20     // Adafruit MCP 23017 I/O expander
#define TIC1_I2C_ADDR 0x0E    // Pololu TIC 500 Stepper Controller
#define TIC2_I2C_ADDR 0x0F    // Pololu TIC 500 Stepper Controller
#define FRAM_I2C_ADDR 0x50    // Adafruit FRAM breakout board

#define BTN_OK 3      // MCP23017 button pin 
#define BTN_LEFT 4    // MCP23017 button pin 
#define BTN_UP 5      // MCP23017 button pin 
#define BTN_DOWN 6    // MCP23017 button pin 
#define BTN_RIGHT 7   // MCP23017 button pin 

#define DEBOUNCE 500   // Button debounce time 
#define INT_PIN 7      // MCU pin for MCP interrupt

#define TIC_PULSE_MULTIPLIER 10000  // TIC uses pulses/10,000 Seconds. See TIC documentation.
//#define RUN_SPEED1 8000  // 8000 pulses per sec (1/8 micro step)
//#define RUN_SPEED2 10000  // 10000 pulses per sec (1/8 micro step)

//TODO: move these to config.h?
#define MAX_TRICKLE_SPEED 40000
#define DECEL_THRESH_INC 0.1
#define DECEL_LIMIT_INC 100
#define DECEL_LIMIT_INC_LIMIT 5000
#define DECEL_LIMIT_DEC_LIMIT 250
#define BUMP_THRESH_INC 0.01
#define FCURVEP_INC 0.1
#define FCURVEP_INC_LIMIT 10.0
#define FCURVEP_DEC_LIMIT -10.0


/*
 * System wide globals
 */
Adafruit_MCP23X17 g_mcp;
LiquidCrystal_PCF8574 g_lcd(LCD_I2C_ADDR);  
TicI2C g_tic1(14);
TicI2C g_tic2(15);

PTConfig g_config;
PTState g_state(PTState::pt_undefined);
PTScale g_scale;
PresetManager g_presets;
PowderManager g_powders;

long g_loop_time = 0;
//long g_last_loop_time = 0;
float g_curve_map[101];  // Curve for trickler slowdown
bool g_display_changed = false;
int g_cur_line = 0;
int g_prev_line = 0;
int g_cursor_pos = 0;
bool g_disp_edit = false;
int g_curr_preset = 0;


#endif  // POWDER_THROW_DEFINITIONS_H
