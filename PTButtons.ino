/*
 * Globals used only here in this file.
 * TODO: consider a class?  Probably not.  Getting too big.
 */
bool interrupted = false;
long interrupt_time = 0;
int last_button = 0;

/*
 * Check for MCP interrupt on a button press defined as a 
 * state change from high to low on the MCP pin.  
 * Debounce defined by DEBOUNCE time during which no
 * further button state changes will be detected.
 * NOTE: only one button detected and if multiple buttons
 * are pressed the Adafruit MCP library will just return
 * the first one found (searched in order of A0 -> B7).
 * 
 */
void checkButtons() {
  if (interrupted) {
    if ((millis() - interrupt_time) > DEBOUNCE) {
      interrupted = false;
      g_mcp.getLastInterruptPin();  //clear it just in case      
    }
  } else {
    if (!digitalRead(INT_PIN)) {
      if (!interrupted) {
        interrupted = true;
        interrupt_time = millis();
        int btn = g_mcp.getLastInterruptPin();
        handleButton(btn);
      }
    }
  }
}

/*
 * Pause system until user presses any button.
 * Uses debounce delay.
 */
void pauseForAnyButton() {
  bool waiting = true;
  g_mcp.getLastInterruptPin(); // clear any prior button press/bounces
  while (waiting) {
    if (interrupted)  {
      if ((millis() - interrupt_time) > DEBOUNCE)  {
        interrupted = false;
        g_mcp.getLastInterruptPin();  //clear it just in case
        waiting = false;
      }
    } else {
      if (!digitalRead(INT_PIN)) {
        if (!interrupted) {
          interrupted = true;
          interrupt_time = millis();
        }
      }
    }
  }
}

/*
 * Handle a button pressed based on the MCP pin number
 * the button is on.
 * 
 * TODO: condider moving button logic into the associated objects, esp config? (in work)
 */
void handleButton(int btn)
{  
  static bool _regen_fcurve = false;  //flag to trigger Fcurve regen if parameters changed in config.
  int _state = g_state.getState(); 
  switch (_state)
  {
    ////////////////////////////
    // MENU
    ////////////////////////////
    case g_state.pt_menu:
      switch (btn)
      {
        case BTN_LEFT:
          // Ignore
          break;
        case BTN_OK:
        case BTN_RIGHT:
            if (g_disp_page == 1) {
              switch (g_cur_line)
              {
                case 0:
                  if (g_config.isRunReady()) { 
                    g_state.setState(g_state.pt_ready); 
                    BLEWriteScreenChange(BLE_SCREEN_RUN);
                  }
                  g_display_changed = true;
                  break;
                case 1:
                  g_state.setState(g_state.pt_cfg);
                  g_prev_line = g_cur_line;
                  g_cur_line = 0;
                  BLEWriteScreenChange(BLE_SCREEN_SETTINGS);
                  break;
                case 2:
                  g_state.setState(g_state.pt_presets);
                  g_prev_line = g_cur_line;
                  g_cur_line = 0;
                  BLEWriteScreenChange(BLE_SCREEN_PRESETS);
                  break;
                case 3:
                  g_state.setState(g_state.pt_powders);
                  g_prev_line = g_cur_line;
                  g_cur_line = 0;
                  BLEWriteScreenChange(BLE_SCREEN_POWDERS);
                  break;
                default:
                  DEBUGLN(F("WARN: Bad Line Number.")); //shouldn't happen
                  return;
              }
            } else if (g_disp_page == 2) {
              switch (g_cur_line)
              {
                case 0:
                  manualThrow();
                  break;
                case 1:
                  toggleTrickler();
                  break;
                case 2:
                  startTricklerCalibration();
                  break;
                case 3:
                  menuCalibrateScale();   
                  break;
                default:
                  DEBUGLN(F("WARN: Bad Line Number.")); //shouldn't happen
                  return;
              }
            }
        break;
        case BTN_UP:
          if (g_cur_line == 0) {
            if (g_disp_page > 1) {
              g_disp_page--;
              g_cur_line = 3;
            }
          } else {
            g_cur_line--;
          }
          break;
        case BTN_DOWN:
          if (g_cur_line == 3) {
            if (g_disp_page < 2) {
              g_disp_page++;
              g_cur_line = 0;
            }
          } else {
            g_cur_line++;
          }
          break;
      }
      break;
    ////////////////////////////
    // MANUAL: CAL TRICKLER
    ////////////////////////////
    case g_state.pt_man_cal_trickler:
      switch (btn)
      {
        case BTN_LEFT:
        case BTN_RIGHT:
        case BTN_UP:
        case BTN_DOWN:
        case BTN_OK:
          calibrateTrickler();  // any button will cancel and stop it when running
      break;
      }      
    ////////////////////////////
    // CONFIG
    ////////////////////////////
    case g_state.pt_cfg:
      switch (btn)
      {
        case BTN_LEFT:
          if (g_disp_edit)
          {
            if (g_config.isDirty()) { g_config.resetConfig(); }
            g_disp_edit = false;  // Cancel edit
          }
          else
          {
            g_cur_line = g_prev_line;
            g_state.setState(g_state.pt_menu);
            BLEWriteScreenChange(BLE_SCREEN_MENU);
          }
          break;
        case BTN_RIGHT:
          if (!g_disp_edit)
          {
            g_disp_edit = true;
          }
          break;
        case BTN_UP:
          if (g_disp_edit)
          {
            switch (g_cur_line)
            {
              case 0:
                if (g_config.getDecelThreshold() < (DECEL_THRESH_INC_LIMIT + DECEL_THRESH_INC)) {
                  g_config.setDecelThreshold(g_config.getDecelThreshold() + DECEL_THRESH_INC);
                }
                break;
              case 1:
                if (g_config.getDecelLimit() < (DECEL_LIMIT_INC_LIMIT + DECEL_LIMIT_INC))
                {
                  g_config.setDecelLimit(g_config.getDecelLimit() + DECEL_LIMIT_INC);
                }
                break;
              case 2:
                if (g_config.getBumpThreshold() < (g_config.getDecelThreshold() - DECEL_THRESH_INC))
                {
                  g_config.setBumpThreshold(g_config.getBumpThreshold() + BUMP_THRESH_INC);
                }
                break;
              case 3:
                if ((g_config.getFcurveP() + FCURVEP_INC) <= FCURVEP_INC_LIMIT)
                {
                  g_config.setFcurveP(g_config.getFcurveP() + FCURVEP_INC);
                  _regen_fcurve = true;
                }
                break;
              default:
                logError("Bad value for g_cur_line: ", __FILE__, __LINE__);
                return;
            }
          }
          else
          {
            if (--g_cur_line < 0) { g_cur_line = 0; }
          }
          break;
        case BTN_DOWN:
          if (g_disp_edit)
          {
            switch (g_cur_line)
            {
              case 0:
                //if ((g_config.getDecelThreshold() - 2*DECEL_THRESH_INC) > g_config.getBumpThreshold())
                if ((g_config.getDecelThreshold() - DECEL_THRESH_INC) > DECEL_THRESH_DEC_LIMIT) 
                {
                  g_config.setDecelThreshold(g_config.getDecelThreshold() - DECEL_THRESH_INC);
                }
                break;
              case 1:
                if ((g_config.getDecelLimit() - DECEL_LIMIT_INC) >= DECEL_LIMIT_DEC_LIMIT)
                {
                  g_config.setDecelLimit(g_config.getDecelLimit() - DECEL_LIMIT_INC);
                }
                break;
              case 2:
                if ((g_config.getBumpThreshold() - BUMP_THRESH_INC) >= 0)
                {
                  g_config.setBumpThreshold(g_config.getBumpThreshold() - BUMP_THRESH_INC);
                }
                break;
              case 3:
                if ((g_config.getFcurveP() - FCURVEP_INC) >= FCURVEP_DEC_LIMIT)
                {
                  g_config.setFcurveP(g_config.getFcurveP() - FCURVEP_INC);
                  _regen_fcurve = true;
                }
                break;
              default:
                logError("Bad value for g_cur_line: ", __FILE__, __LINE__);
                return;
            }
          }
          else
          {
            if (++g_cur_line > 3) { g_cur_line = 3; }
          }
          break;
        case BTN_OK:
          if (g_config.isDirty()) 
          { 
            g_config.saveConfig(); 
            if (_regen_fcurve == true) 
            { 
              util_setFscaleCurve(g_curve_map, g_config.getFcurveP()); 
              _regen_fcurve = false;
            }
          }
          g_disp_edit = !g_disp_edit; 
          break;
      }      
      break;
    ////////////////////////////
    // PRESETS
    ////////////////////////////
    case g_state.pt_presets:
      switch (btn) {
        case BTN_LEFT:
          g_cur_line = g_prev_line;
          g_state.setState(g_state.pt_menu);
          BLEWriteScreenChange(BLE_SCREEN_MENU);
          break;
        case BTN_RIGHT:
          g_state.setState(g_state.pt_presets_edit);
          g_cur_line = 0;
          break; 
        case BTN_UP:
          if (g_presets.getCurrentPreset() == 0) { return; }
          g_presets.loadPreset(g_presets.getCurrentPreset()-1);
          g_cur_line = 0;
          break;
        case BTN_DOWN:
          if (g_presets.getCurrentPreset() == MAX_PRESETS) { return; }
          g_presets.loadPreset(g_presets.getCurrentPreset()+1);
          g_cur_line = 0;
          break;
        case BTN_OK:
          if (g_presets.getPresetChargeWeight() > 0)
          {
            setConfigPresetData();
            g_cur_line = g_prev_line;
            g_state.setState(g_state.pt_menu);
          } else { return; }
          break;
      }      
      break;
    ////////////////////////////
    // PRESETS EDIT
    ////////////////////////////
    case g_state.pt_presets_edit:
      switch (g_cur_line)
      {
        case 0:  // Save or cancel edit preset
          if (btn == BTN_OK) {
            if (g_presets.isDefined()) {
              g_presets.savePreset();
              g_state.setState(g_state.pt_presets);
            } else { return; } 
          }
          else if (btn == BTN_LEFT) {
            g_presets.resetCurrentPreset();
            g_state.setState(g_state.pt_presets);
          } else if (btn == BTN_DOWN)  { 
            g_cur_line = 1;
          } else { return; }
          break;
        case 1:        
          // edit preset name
          switch (btn)
          {
            case BTN_LEFT:
              if (g_disp_edit) {
                //Move cursor left on name
                if (g_cursor_pos > 2) { g_cursor_pos--; }
                else { return; }
              }
              break;
            case BTN_RIGHT:
              if (g_disp_edit) {
                //Move cursor right on name
                if (g_cursor_pos < NAME_LEN+2) { g_cursor_pos++; }
                else { return; }
              } else {
                g_cursor_pos = 2;
                g_disp_edit = true;
              }
              break;
            case BTN_UP:
              if (g_disp_edit) { g_presets.incPresetNameChar(g_cursor_pos-2); }
              else { g_cur_line = 0; }
              break;
            case BTN_DOWN:
              if (g_disp_edit) { g_presets.decPresetNameChar(g_cursor_pos-2); }
              else { g_cur_line = 2; }
              break;
            case BTN_OK:
              if (g_disp_edit) { g_disp_edit = false; }
              break;
          }      
          break;
        case 2:
          // edit powder charge
          switch (btn)
          {
            case BTN_LEFT:
              if (g_disp_edit) {
                //Move cursor left on charge
                if (g_cursor_pos == 14) { g_cursor_pos = 12; }
                else if (g_cursor_pos > 10) { g_cursor_pos--; }
                else { return; }
              }
              break;
            case BTN_RIGHT:
              if (g_disp_edit) {
                //Move cursor right on charge
                if (g_cursor_pos == 12) { g_cursor_pos = 14; }
                else if (g_cursor_pos < 12) { g_cursor_pos++; }
                else { return; }
              } else  {
                g_cursor_pos = 10;
                g_disp_edit = true;
              }
              break;
            case BTN_UP:
              if (g_disp_edit) { g_presets.incPresetChargeWeight(g_cursor_pos-10); }
              else { g_cur_line = 1; }
              break;
            case BTN_DOWN:
              if (g_disp_edit) { g_presets.decPresetChargeWeight(g_cursor_pos-10); }
              else { g_cur_line = 3; }            
              break;
            case BTN_OK:
              if (g_disp_edit) { g_disp_edit = false; }
              break;
          }      
          break;
        case 3:
          // select a powder
          switch (btn)
          {
            case BTN_LEFT:
              //TODO: does anythign need to be reset?
              //if (g_disp_edit) { g_disp_edit = false; }
              //break;
              return;
            case BTN_RIGHT:
              if (!g_disp_edit) {
                g_cursor_pos = 2;
                g_disp_edit = true;
              }
              else { return; }
              break;
            case BTN_UP:
              if (g_disp_edit) { 
                if (g_presets.getPowderIndex() > 0) { g_presets.setPowderIndex(g_presets.getPowderIndex()-1); }
              } else {
                g_cur_line = 2;
              }
              break;
            case BTN_DOWN:
              if (g_disp_edit) {
                if (g_presets.getPowderIndex() < MAX_POWDERS) { g_presets.setPowderIndex(g_presets.getPowderIndex()+1); }
              } else { return; }
              break;
            case BTN_OK:
              if (g_disp_edit) {
                g_presets.setPowderIndex(g_powders.getCurrentPowder());
                g_disp_edit = false;
              }
              else { return; }
              break;
          }
          break;
        default:
          logError("Bad value for g_cur_line: ", __FILE__, __LINE__);
          return;
      }
      break;
    ////////////////////////////
    // POWDERS
    ////////////////////////////
    case g_state.pt_powders:
      switch (btn)
      {
        case BTN_LEFT:
          g_cur_line = g_prev_line;
          g_state.setState(g_state.pt_menu);
          BLEWriteScreenChange(BLE_SCREEN_MENU);
          break;
        case BTN_RIGHT:
          g_state.setState(g_state.pt_powders_edit);
          g_cur_line = 0;
          break; 
        case BTN_UP:
          if (g_powders.getCurrentPowder() == 0) { return; }
          g_powders.loadPowder(g_powders.getCurrentPowder()-1);
          g_cur_line = 0;
          break;
        case BTN_DOWN:
          if (g_powders.getCurrentPowder() == MAX_POWDERS) { return; }
          g_powders.loadPowder(g_powders.getCurrentPowder()+1);
          g_cur_line = 0;
          break;
        case BTN_OK:
          if (g_powders.getPowderFactor() > 0)
          {
            DEBUGLN(F("TODO: Anything?  OK in powders menu."));
          }
          else
          {
            return;
          }
          break;
      }      
      break;
    ////////////////////////////
    // POWDERS EDIT
    ////////////////////////////
    case g_state.pt_powders_edit:
      switch (g_cur_line)
      {
        case 0:
          // Save or cancel edit powder
          if (btn == BTN_OK) //save edits
          {
            g_powders.savePowder();
            g_state.setState(g_state.pt_powders);
          }
          else if (btn == BTN_LEFT) //cancel edits
          {
            g_powders.resetBuffer();
            g_state.setState(g_state.pt_powders);
          }
          else if (btn == BTN_DOWN) 
          { 
            g_cur_line = 1;
          }
          else
          {
            return; //ignore
          }
          break;
        case 1:        
          // edit powder name
          switch (btn)
          {
            case BTN_LEFT:
              if (g_disp_edit)
              {
                //Move cursor left on name
                if (g_cursor_pos > 2) { g_cursor_pos--; }
                else { return; }
              }
              break;
            case BTN_RIGHT:
              if (g_disp_edit)
              {
                //Move cursor right on name
                if (g_cursor_pos < NAME_LEN+2) { g_cursor_pos++; }
                else { return; }
              }
              else
              {
                g_cursor_pos = 2;
                g_disp_edit = true;
              }
              break;
            case BTN_UP:
              if (g_disp_edit)
              {
                g_powders.incNameChar(g_cursor_pos-2);
              }
              else
              {
                g_cur_line = 0;
              }
              break;
            case BTN_DOWN:
              if (g_disp_edit)
              {
                g_powders.decNameChar(g_cursor_pos-2);
              }
              else
              {
                g_cur_line = 2;
              }
              break;
            case BTN_OK:
              if (g_disp_edit) { g_disp_edit = false; }
              break;
          }      
          break;
        case 2:
          // edit powder factor
          switch (btn)
          {
            case BTN_LEFT:
              if (g_disp_edit)
              {
                //Move cursor left on value
                if (g_cursor_pos > 12) { g_cursor_pos--; }
                else { return; }
              }
              break;
            case BTN_RIGHT:
              if (g_disp_edit)
              {
                //Move cursor right on value
                if (g_cursor_pos < 19) { g_cursor_pos++; }
                else { return; }
              }
              else
              {
                g_cursor_pos = 12;
                g_disp_edit = true;
              }
              break;
            case BTN_UP:
              if (g_disp_edit)
              {
                g_powders.incPowderFactor(g_cursor_pos);
              }
              else
              {
                g_cur_line = 1;
              }
              break;
            case BTN_DOWN:
              if (g_disp_edit)
              {
                g_powders.decPowderFactor(g_cursor_pos);
              }
              else
              {
                g_cur_line = 3;
              }            
              break;
            case BTN_OK:
              if (g_disp_edit) { g_disp_edit = false; }
              break;
          }      
          break;
        default:
          return;
      }
      break;
    ////////////////////////////
    // SYSTEM READY
    // SYSTEM LADDER (Manual)
    ////////////////////////////
    case g_state.pt_ready:
    case g_state.pt_manual:
    case g_state.pt_ladder:
      switch (btn)
      {
        case BTN_RIGHT:
          if (g_state.getState() == PTState::pt_ready) {
            if (g_config.ladder_data.is_configured) {
              g_state.setState(PTState::pt_ladder);
              g_config.setRunMode(PTConfig::pt_ladder);
            } else {
              g_state.setState(PTState::pt_manual);
              g_config.setRunMode(PTConfig::pt_manual);
              setConfigPresetData();
            }
          } else {
            g_state.setState(PTState::pt_ready);
            setConfigPresetData();
          }       
          g_display_changed = true;
          displayUpdate(true);
          break;
        case BTN_LEFT:
          if (
            (_state == PTState::pt_ready) ||
            (_state == PTState::pt_locked) ||
            (_state == PTState::pt_manual) ||
            (_state == PTState::pt_ladder)) 
          {
            g_state.setState(g_state.pt_menu);
            g_LED_Blu.setOff();
            g_LED_Yel_1.setOff();
            g_LED_Yel_2.setOff();
            g_LED_Grn.setOff();
            g_LED_Red.setOff();  
            BLEWriteScreenChange(BLE_SCREEN_MENU);
          }
          break;
        case BTN_UP:  
          if (_state == PTState::pt_ladder) {
            DEBUGLN(F("TODO: FUTURE: Previous ladder step (ladder mode only)."));
          } else {
            DEBUGLN(F("Up button: nothing to do in non-ladder mode?"));
          }
          break;
        case BTN_DOWN:  
          if (_state == PTState::pt_ladder) {
            DEBUGLN(F("TODO: FUTRE: Next ladder step (ladder mode only)."));
          } else {
            DEBUGLN(F("Down button: nothing to do in non-ladder mode?"));
          }
          break;
        case BTN_OK:
          if (_state == PTState::pt_ladder) {
            if (g_config.ladder_data.is_configured) {
              DEBUGLN(F("Run ladder step."));
              g_state.setState(PTState::pt_ladder_run);
            }
          } else if (_state == PTState::pt_manual) {
              DEBUGLN(F("Run manual charge."));
              g_state.setState(PTState::pt_manual_run);
          }
          break;
        default:
          logError("Invalid button.", __FILE__, __LINE__);
      }
      break;
    ////////////////////////////
    // DEFAULT
    ////////////////////////////
    default:
      DEBUGP(g_state.getStateName());
      DEBUGLN(F(" not handled, handleButton(): nothing to do"));
      //Direct display output states. Nothing to do here.
      return;
  }
  g_display_changed = true;
  displayUpdate();
}
