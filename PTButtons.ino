/*
 * Globals used only here
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
void checkButtons() 
{
  if (interrupted) 
  {
    if ((millis() - interrupt_time) > DEBOUNCE) 
    {
      interrupted = false;
      g_mcp.getLastInterruptPin();  //clear it just in case      
    }
  }
  else
  {
    if (!digitalRead(INT_PIN)) 
    {
      if (!interrupted) 
      {
        interrupted = true;
        interrupt_time = millis();
        int btn = g_mcp.getLastInterruptPin();
        //Serial.print("Button Pressed: ");
        //Serial.println(btn);
        handleButton(btn);
      }
    }
  }
}

/*
 * Pause system until user presses any button.
 * Uses debounce delay.
 */
void pauseForAnyButton()
{
  bool waiting = true;
  while (waiting)
  {
    if (interrupted) 
    {
      if ((millis() - interrupt_time) > DEBOUNCE) 
      {
        interrupted = false;
        g_mcp.getLastInterruptPin();  //clear it just in case
        waiting = false;
      }
    }
    else
    {
      if (!digitalRead(INT_PIN)) 
      {
        if (!interrupted) 
        {
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
 * TODO: condider moving button logic into the associated objects, esp config?
 */
void handleButton(int btn)
{  
  static bool _regen_fcurve = false;  //flag to trigger Fcurve regen if parameters changed in config.
  switch (g_state.getState())
  {
    case g_state.pt_menu:
      switch (btn)
      {
        case BTN_LEFT:
          if (g_presets.isDefined() && g_powders.isPowderDefined())
          {
            g_scale.setTarget(g_presets.getPresetChargeWeight());
            char buff[NAME_LEN];
            buff[0] = 0x00;
            g_presets.getPresetName(buff);
            g_config.setPresetName(buff);
            buff[0] = 0x00;
            g_powders.getPowderName(buff);
            g_config.setPowderName(buff);
            g_config.setKernelFactor(g_powders.getPowderFactor());
            g_state.setState(g_state.pt_ready);
          }
          else
          {
            DEBUGLN(F("System not ready"));
          }
          break;
        case BTN_OK:
        case BTN_RIGHT:
          switch (g_cur_line)
          {
            case 0:
              g_state.setState(g_state.pt_man);
              g_prev_line = g_cur_line;
              g_cur_line = 0;
              break;
            case 1:
              g_state.setState(g_state.pt_cfg);
              g_prev_line = g_cur_line;
              g_cur_line = 0;
              break;
            case 2:
              g_state.setState(g_state.pt_presets);
              g_prev_line = g_cur_line;
              g_cur_line = 0;
              break;
            case 3:
              g_state.setState(g_state.pt_powders);
              g_prev_line = g_cur_line;
              g_cur_line = 0;
              break;
            default:
              DEBUGLN(F("WARN: Bad Line Number.")); //shouldn't happen
              return;
          }
          break;
        case BTN_UP:
          if (--g_cur_line < 0) { g_cur_line = 0; }
          break;
        case BTN_DOWN:
          if (++g_cur_line > 3) { g_cur_line = 3; }
          break;
      }
      break;
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
                g_config.setDecelThreshold(g_config.getDecelThreshold() + DECEL_THRESH_INC);
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
                DEBUGP(F("ERROR: bad value for g_cur_line: "));
                DEBUGLN(__LINE__);
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
                if ((g_config.getDecelThreshold() - 2*DECEL_THRESH_INC) > g_config.getBumpThreshold())
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
                DEBUGP(F("ERROR: bad value for g_cur_line: "));
                DEBUGLN(__LINE__);
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
    case g_state.pt_man:
      switch (btn)
      {
        case BTN_LEFT:
          g_cur_line = g_prev_line;
          g_state.setState(g_state.pt_menu);
          break;
        case BTN_OK:
        case BTN_RIGHT:
          switch (g_cur_line)
          {
            case 0:
              DEBUGLN(F("TODO: force a bump?"));
              break;
            case 1:
              manualThrow();
              break;
            case 2:
              toggleTrickler();
              break;
            case 3:
              DEBUGLN(F("TODO: testing scale calibration"));
              g_mcp.getLastInterruptPin(); //clear any button interrupts
              g_lcd.clear();
              g_lcd.setCursor(0,0);
              g_lcd.print(F("Calibrate System ..."));
              g_lcd.setCursor(0,3);
              g_lcd.print(F("Press any button ..."));
              delay(500);
              pauseForAnyButton(); //first call in calibrateScale() is skipped in this code block. Why???  HACK: added to make it work
              calibrateScale();
              delay(1000);
              setThrowerHome();
              g_scale.checkScale(); //update scale state
              g_lcd.setCursor(0,2);
              if (isSystemCalibrated())
              {
                g_lcd.print(F("System calibrated.  "));
              }
              else
              {
                g_lcd.print(F("Calibrated Failed.  "));
              }
              g_lcd.setCursor(0,3);
              g_lcd.print(F("Press any button ..."));
              pauseForAnyButton();  
              break;
          }
          break;
        case BTN_UP:
          if (--g_cur_line < 0) { g_cur_line = 0; }
          break;
        case BTN_DOWN:
          if (++g_cur_line > 3) { g_cur_line = 3; }
          break;
      }      
      break;
    case g_state.pt_presets:
      switch (btn)
      {
        case BTN_LEFT:
          g_cur_line = g_prev_line;
          g_state.setState(g_state.pt_menu);
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
            g_config.setPreset(g_presets.getCurrentPreset());
            g_config.saveConfig();
            g_cur_line = g_prev_line;
            g_state.setState(g_state.pt_menu);
          }
          else
          {
            return;
          }
          break;
      }      
      break;
    case g_state.pt_presets_edit:
      switch (g_cur_line)
      {
        case 0:
          // Save or cancel edit preset
          if (btn == BTN_OK) //save edits
          {
            if (g_presets.isDefined())
            {
              g_presets.savePreset();
              g_state.setState(g_state.pt_presets);
            }
            else { return; } //not valid edits
          }
          else if (btn == BTN_LEFT) //cancel edits
          {
            g_presets.resetCurrentPreset();
            g_state.setState(g_state.pt_presets);
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
          // edit preset name
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
                if (g_cursor_pos < PRESET_NAME_LEN+2) { g_cursor_pos++; }
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
                g_presets.incNameChar(g_cursor_pos-2);
              }
              else
              {
                g_cur_line = 0;
              }
              break;
            case BTN_DOWN:
              if (g_disp_edit)
              {
                g_presets.decNameChar(g_cursor_pos-2);
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
          // edit powder charge
          switch (btn)
          {
            case BTN_LEFT:
              if (g_disp_edit)
              {
                //Move cursor left on charge
                if (g_cursor_pos == 14) { g_cursor_pos = 12; }
                else if (g_cursor_pos > 10) { g_cursor_pos--; }
                else { return; }
              }
              break;
            case BTN_RIGHT:
              if (g_disp_edit)
              {
                //Move cursor right on charge
                if (g_cursor_pos == 12) { g_cursor_pos = 14; }
                else if (g_cursor_pos < 12) { g_cursor_pos++; }
                else { return; }
              }
              else
              {
                g_cursor_pos = 10;
                g_disp_edit = true;
              }
              break;
            case BTN_UP:
              if (g_disp_edit)
              {
                //Increment charge
                float val = g_presets.getPresetChargeWeight();
                switch (g_cursor_pos)
                {
                  case 10:
                    if (val <= 100) {  val = val + 100; }
                    break;
                  case 11:
                    if (val <= 190) {  val = val + 10; }
                    break;
                  case 12:
                    if (val <= 199) {  val = val + 1; }
                    break;
                  case 14:
                    if (val < 200) {  val = val + 0.1; }
                    break;
                  default:
                    DEBUGLN(F("Invalid cursor pos for edit charge weight"));
                    return;
                }
                g_presets.setPresetChargeWeight(val);
              }
              else
              {
                g_cur_line = 1;
              }
              break;
            case BTN_DOWN:
              if (g_disp_edit)
              {
                //Decrement charge
                float val = g_presets.getPresetChargeWeight();
                switch (g_cursor_pos)
                {
                  case 10:
                    if (val > 100.0) {  val = val - 100; }
                    break;
                  case 11:
                    if (val > 10.0) {  val = val - 10; }
                    break;
                  case 12:
                    if (val > 1.0 ) {  val = val - 1; }
                    break;
                  case 14:
                    if (val > 0) {  val = val - 0.1; }
                    break;
                  default:
                    DEBUGLN(F("Invalid cursor pos for edit charge weight"));
                    return;
                }
                g_presets.setPresetChargeWeight(val);
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
              if (!g_disp_edit)
              {
                g_cursor_pos = 2;
                g_disp_edit = true;
              }
              else { return; }
              break;
            case BTN_UP:
              if (g_disp_edit)
              {
                if (g_presets.getPowderIndex() > 0)
                {
                  g_presets.setPowderIndex(g_presets.getPowderIndex()-1);
                }
              }
              else
              {
                g_cur_line = 2;
              }
              break;
            case BTN_DOWN:
              if (g_disp_edit)
              {
                if (g_presets.getPowderIndex() < MAX_POWDERS)
                {
                  g_presets.setPowderIndex(g_presets.getPowderIndex()+1);
                }
              }
              else { return; }
              break;
            case BTN_OK:
              if (g_disp_edit)
              {
                g_presets.setPowderIndex(g_powders.getCurrentPowder());
                g_disp_edit = false;
              }
              else { return; }
              break;
          }
          break;
        default:
          DEBUGP(F("ERROR: bad value for g_cur_line: "));
          DEBUGLN(__LINE__);
          return;
      }
      break;
    case g_state.pt_powders:
      switch (btn)
      {
        case BTN_LEFT:
          g_cur_line = g_prev_line;
          g_state.setState(g_state.pt_menu);
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
                if (g_cursor_pos < POWDER_NAME_LEN+2) { g_cursor_pos++; }
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
    case g_state.pt_ready:
      DEBUGLN(F("TODO: handleButton(): pt_run"));
      switch (btn)
      {
        case BTN_LEFT:
        case BTN_RIGHT:
        case BTN_UP:  //increment target weight?
        case BTN_DOWN:  //decrement target weight?
          return; // Do nothing
        case BTN_OK:
          //TODO: only go to menu if state: pt_ready or pt_locked
          g_state.setState(g_state.pt_menu);
          g_LED_Blu.setOff();
          g_LED_Yel_1.setOff();
          g_LED_Yel_2.setOff();
          g_LED_Grn.setOff();
          g_LED_Red.setOff();  
          break;
      }
      break;
    default:
      DEBUGP(g_state.getStateName());
      DEBUGLN(F(" not handled, handleButton(): nothing to do"));
      //Direct display output states. Nothing to do here.
      return;
  }
  g_display_changed = true;
  displayUpdate();
}
