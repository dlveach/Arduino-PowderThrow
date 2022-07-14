
//TODO: fix format issues in Gram mode.

uint8_t up_arrow[] = {
  0b00100,
  0b01110,
  0b11111,
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b00000,
};
uint8_t down_arrow[] = {
  0b00100,
  0b00100,
  0b00100,
  0b00100,
  0b11111,
  0b01110,
  0b00100, 
  0b00000,
};
  
/*
 * Update display based on current state, page, line number, edit mode and cursor pos
 * Called regularly by main loop (sometimes by direct display output functions)
 *
 * Parameters:
 *  force: boolean, if true will force a full refresh.  Default: false.
 *
 * Globals:
 *  g_display_changed:  flag set by other fns to trigger display update.
 *  g_disp_edit:  edit mode flag.
 *  g_cursor_pos:  current cursor position
 *  g_cur_line:  current cursor line
 *  g_disp_page:  current page (when state has multiple pages)
 *
 */
void displayUpdate(bool force)
{
//                        12345678901234567890
  static char buff[21] = "                    ";
  static bool _clear_disp = true; //static flag to avoid clear on every state update
  if (force) { _clear_disp = true; }  // forced refresh
  if (!g_display_changed) return;
  g_display_changed = false;
  switch (g_state.getState())
  {
    //Direct display output states. Nothing to do here.  Generally shouldn't be called in
    //  these states but handle just in case.
    case g_state.pt_undefined:
    case g_state.pt_setup:
    case g_state.pt_error:
    case g_state.pt_man_cal_scale:
    case g_state.pt_man_cal_trickler:
      DEBUGP(F("WARN: "));
      DEBUGP(g_state.getStateName());
      DEBUGLN(F(" is a direct output state, no display update."));
      return;
    ////////////////////////////
    // SYSTEM RUNNING
    ////////////////////////////
    case g_state.pt_disabled:
    case g_state.pt_ready:
    case g_state.pt_throwing:
    case g_state.pt_trickling:
    case g_state.pt_bumping:
    case g_state.pt_paused:
    case g_state.pt_locked:
      if (_clear_disp) { g_lcd.clear(); }
      g_lcd.noCursor();
      g_lcd.setCursor(0,0);
      if (g_scale.isConnected()) {
        sprintf(buff, "%-10s %-9s", g_state.getStateName(), g_scale.getConditionName());
      } else {
        sprintf(buff, "%-10s No Scale ", g_state.getStateName());
      }
      g_lcd.print(buff);
      if (_clear_disp) {
        g_lcd.setCursor(0,1);
        sprintf(buff, "%02d %-8s %-8s", g_config.getPreset()+1, g_config.getPresetName(), g_config.getPowderName());  
        g_lcd.print(buff);
      }
      
      g_lcd.setCursor(0,2);
      if ((g_scale.getCondition() == PTScale::pan_off) || (g_scale.getCondition() == PTScale::undef) || (g_scale.getDelta() >= 10)) {
        if (g_scale.getMode() == SCALE_MODE_GRAM) {
          sprintf(buff, "T:% 05.3f G  D: -.---", g_config.getTargetWeight());
        } else {
          sprintf(buff, "T:% 05.2f gn D: --.--", g_config.getTargetWeight());
        } 
      } else {
        if (g_scale.getMode() == SCALE_MODE_GRAM) {
          sprintf(buff, "T:% 05.3f G  D:% 5.3f", g_config.getTargetWeight(), g_scale.getDelta());
        } else {
          sprintf(buff, "T:% 05.2f gn D:% 5.2f", g_config.getTargetWeight(), g_scale.getDelta());
        }
      }
      pad(buff);
      g_lcd.print(buff);
      g_lcd.setCursor(0,3);
      if ((g_scale.getCondition() == PTScale::pan_off) || (g_scale.getCondition() == PTScale::undef) || (g_scale.getKernels() >= 1000)) {
        if (g_scale.getMode() == SCALE_MODE_GRAM) {
          sprintf(buff, "C: -.--- G  K: ---.-");
        } else {
          sprintf(buff, "C: --.-- gn K: ---.-");
        }
      } else {
        if (g_scale.getMode() == SCALE_MODE_GRAM) {
          sprintf(buff, "C:% 05.3f G  K:% 05.1f", g_scale.getWeight(), g_scale.getKernels());
        } else {
          sprintf(buff, "C:% 05.2f gn K:% 05.1f", g_scale.getWeight(), g_scale.getKernels());
        }
      }
      pad(buff);
      g_lcd.print(buff);
      _clear_disp = false;
      break;
    ////////////////////////////
    // MENU
    ////////////////////////////
    case g_state.pt_menu:
      g_lcd.clear();
      if (g_disp_page == 1) {
        g_lcd.print(F("  Run System        "));
        g_lcd.setCursor(0,1);
        g_lcd.print(F("  Config            "));
        g_lcd.setCursor(0,2);
        g_lcd.print(F("  Presets           "));
        g_lcd.setCursor(0,3);
        g_lcd.print(F("  Powders           "));
        g_lcd.setCursor(19,3);
        g_lcd.print(LCD_DOWN_ARROW);
      } else if (g_disp_page == 2) {
        g_lcd.print(F("  Throw a charge   ^"));
        g_lcd.setCursor(0,1);
        g_lcd.print(F("  Trickler on/off   "));
        g_lcd.setCursor(0,2);
        g_lcd.print(F("  Calibrate Trickler"));
        g_lcd.setCursor(0,3);
        g_lcd.print(F("  Calibrate Scale   "));
        g_lcd.setCursor(19,0);
        g_lcd.print(LCD_UP_ARROW);
      } else {
        Serial.print("ERROR: invalid page at updateDisplay(): ");
        Serial.println(__LINE__);
      }
      g_lcd.setCursor(0,g_cur_line);
      g_lcd.print(F(">>"));      _clear_disp = true;
      break;
    ////////////////////////////
    // CONFIG
    ////////////////////////////
    case g_state.pt_cfg:
      g_lcd.clear();
      snprintf(buff, 20, "  DclThsh %5.2f", g_config.getDecelThreshold());
      g_lcd.print(buff);
      g_lcd.setCursor(0,1);
      snprintf(buff, 20, "  DclLim %-5d", g_config.getDecelLimit());
      g_lcd.print(buff);
      g_lcd.setCursor(0,2);
      snprintf(buff, 20, "  BmpThsh %5.2f", g_config.getBumpThreshold());
      g_lcd.print(buff);
      g_lcd.setCursor(0,3);
      snprintf(buff, 20, "  FcurveP %5.2f", g_config.getFcurveP());
      g_lcd.print(buff);
      g_lcd.setCursor(0,g_cur_line);
      if (g_disp_edit) {
        g_lcd.print(F("+-"));
      } else  {
        g_lcd.print(F(">>"));
      }
      _clear_disp = true;
      break;
    ////////////////////////////
    // PRESETS
    ////////////////////////////
    case g_state.pt_presets:
      g_lcd.clear();
      g_lcd.noCursor();
      g_lcd.noBlink();
      if (g_presets.isDefined()) {
        sprintf(buff, ">>%02d <:C OK:S >:E  ", g_presets.getCurrentPreset()+1); 
      } else {
        sprintf(buff, ">>%02d <:C >:E      ", g_presets.getCurrentPreset()+1); 
      }
      g_lcd.print(buff);
      if (g_presets.getCurrentPreset() > 0) {
        g_lcd.setCursor(19,0);
        g_lcd.print(LCD_UP_ARROW);
      }
      g_lcd.setCursor(0,1);
      g_lcd.print(F("  "));
      g_lcd.setCursor(2,1);
      g_presets.getPresetName(buff);
      g_lcd.print(buff);
      if (g_presets.isDefined()) {
        g_lcd.setCursor(0,2);
        sprintf(buff, "  Charge: %05.1f gn", g_presets.getPresetChargeWeight());
        g_lcd.print(buff);
        g_lcd.setCursor(0,3);
        g_lcd.print(F("  "));
        g_lcd.setCursor(0,3);
        if (g_presets.getPowderIndex() >= 0)  {
          sprintf(buff, "  %02d:", g_presets.getPowderIndex()+1);
          g_lcd.print(buff);
          g_lcd.setCursor(5,3);
          g_powders.loadPowder(g_presets.getPowderIndex());
          if (g_powders.isPowderDefined()) {
            g_powders.getPowderName(buff);
            g_lcd.print(buff); 
          } else {
            g_lcd.print(F("EMPTY"));
          }
        } else  { 
          g_lcd.print(F("  --")); 
        }
      }
      if (g_presets.getCurrentPreset() < MAX_PRESETS) {
        g_lcd.setCursor(19,3);
        g_lcd.print(LCD_DOWN_ARROW);
      }
      _clear_disp = true;
      break;
    ////////////////////////////
    // PRESETS EDIT
    ////////////////////////////
    case g_state.pt_presets_edit:
      g_lcd.clear();
      sprintf(buff, "  %02d  OK:S <:C    ", g_presets.getCurrentPreset()+1); 
      g_lcd.print(buff);
      g_lcd.setCursor(0,1);
      g_lcd.print(F("  "));
      g_lcd.setCursor(2,1);
      g_presets.getPresetName(buff);
      g_lcd.print(buff);
      g_lcd.setCursor(0,2);
      sprintf(buff, "  Charge: %05.1f gn", g_presets.getPresetChargeWeight());
      g_lcd.print(buff);
      g_lcd.setCursor(0,3);
      if (g_presets.getPowderIndex() >= 0)  {
        sprintf(buff, "  %02d:", g_presets.getPowderIndex()+1);
        g_lcd.print(buff);
        g_lcd.setCursor(5,3);
        g_powders.loadPowder(g_presets.getPowderIndex());
        if (g_powders.isPowderDefined()) {
          g_powders.getPowderName(buff);
          g_lcd.print(buff); 
        } else {
          g_lcd.print(F("EMPTY"));
        }
      } else  { 
        g_lcd.print(F("  --")); 
      }
      if (g_disp_edit) {
        g_lcd.setCursor(g_cursor_pos, g_cur_line);
      } else {
        g_lcd.setCursor(1,g_cur_line);
      }
      g_lcd.cursor();
      g_lcd.blink();  
      _clear_disp = true;
      break;    
    ////////////////////////////
    // POWDERS
    ////////////////////////////
    case g_state.pt_powders:
      g_lcd.clear();
      g_lcd.noCursor();
      g_lcd.noBlink();
      if (g_powders.isPowderDefined()) { 
        sprintf(buff, ">>%02d  OK:S >:E     ", g_powders.getCurrentPowder()+1); 
      } else {
        sprintf(buff, ">>%02d  >:E          ", g_powders.getCurrentPowder()+1); 
      }
      g_lcd.print(buff);
      if (g_powders.getCurrentPowder() > 0) {
        g_lcd.setCursor(19,0);
        g_lcd.print(LCD_UP_ARROW);
      }      
      g_lcd.setCursor(2,1);
      g_powders.getPowderName(buff);
      g_lcd.print(buff);
      if (g_powders.isPowderDefined())
      {
        g_lcd.setCursor(0,2);
        if (g_powders.getPowderFactor() < 0)
        {
          sprintf(buff, "  Factor: %010.8f", 0);
        }
        else
        {
          sprintf(buff, "  Factor: %010.8f", g_powders.getPowderFactor());
        }
        g_lcd.print(buff);
      }
      if (g_powders.getCurrentPowder() < MAX_POWDERS) {
        g_lcd.setCursor(19,3);
        g_lcd.print(LCD_DOWN_ARROW);
      }      
      _clear_disp = true;
      break;    
    ////////////////////////////
    // POWDERS EDIT
    ////////////////////////////
    case g_state.pt_powders_edit:
      g_lcd.clear();
      sprintf(buff, "  %02d  OK:S <:C     ", g_powders.getCurrentPowder()+1); 
      g_lcd.print(buff);
      g_lcd.setCursor(0,1);
      g_lcd.print(F("  "));
      g_lcd.setCursor(2,1);
      g_powders.getPowderName(buff);
      g_lcd.print(buff);
      g_lcd.setCursor(0,2);
      if (g_powders.isPowderDefined()) {
        sprintf(buff, "  Factor: %010.8f", g_powders.getPowderFactor());
      } else {
        sprintf(buff, "  Factor: %010.8f", 0);
      }
      g_lcd.print(buff);
      if (g_disp_edit) {
        g_lcd.setCursor(g_cursor_pos, g_cur_line);
      } else {
        g_lcd.setCursor(1,g_cur_line);
      }
      g_lcd.cursor();
      g_lcd.blink();
      _clear_disp = true;
      break;
    default:
      DEBUGP(g_state.getStateName());
      DEBUGLN(F(" is unknown state in displayUpdate()"));
      _clear_disp = true;
      return;
  }
}

/*
 * Pad buff out to 20 chars with trailing spaces.
 * WARNING!!!: code assumes char * buff[21]
 * TODO: for safety a buff length check?
 */
void pad(char* buff)
{
  if (strlen(buff) > 20) {
    DEBUGLN(F("ERROR: Display pad(), buffer overflow."));
    DEBUGP(F("buff = '"));
    DEBUGP(buff);
    DEBUGLN(F("'"));
    buff[20]=0x00;
    return;
  }
  if (strlen(buff) < 20) {
    int i=strlen(buff);
    while (i < 20) {
      buff[i++]=' ';
    }
    buff[20]=0x00; //null terminate
  }
}

/*
 * Hidden system function menu
 * TODO: am I using this?
 */
void hiddenMenu()
{
  DEBUGLN(F("TODO: Hidden Menu"));
}
 
void displaySystemError()
{
  g_lcd.clear();
  g_lcd.setCursor(0,0);
  g_lcd.print("FATAL SYSTEM ERROR!");
  g_lcd.setCursor(0,1);
  g_lcd.print(g_state.getSystemMessage());
  while (1) { delay(1); }  //Halt system
}
