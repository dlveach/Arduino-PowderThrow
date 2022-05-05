
//TODO: MAJOR BUG: reading FRAM in every display update!!!
//  Fix:  get powder name (and preset name?) from internal buffers. 
//        DO NOT reload buffer every time.
//        Only load buffer when necessary (selecting a new preset
//        or powder in a preset).  The buffers should stay current
//        for all future display updates.  Obviously buffers change
//        when saving preset or powder but same applies.

/*
 * 
 */
void displayUpdate()
{
//                        12345678901234567890
  static char buff[21] = "                    ";
  static bool _clear_disp = true; //static flag to avoid clear on every state update

  if (!g_display_changed) return;
  g_display_changed = false;
  switch (g_state.getState())
  {
    case g_state.pt_undefined:
    case g_state.pt_setup:
    case g_state.pt_error:
    case g_state.pt_cal_scale:
    case g_state.pt_cal_trickler:
      _clear_disp = true;
      //Direct display output states. Nothing to do here.  Generally shouldn't happen.
      DEBUGP(F("WARN: "));
      DEBUGP(g_state.getStateName());
      DEBUGLN(F(" is a direct output state, no display update."));
      return;
    case g_state.pt_ready:
    case g_state.pt_throwing:
    case g_state.pt_trickling:
    case g_state.pt_bumping:
    case g_state.pt_paused:
    case g_state.pt_locked:
      if (_clear_disp) { g_lcd.clear(); }
      g_lcd.noCursor();
      g_lcd.setCursor(0,0);
      if (g_scale.isConnected())
      {
        sprintf(buff, "%-10s %-9s", g_state.getStateName(), g_scale.getConditionName());
      }
      else
      {
        sprintf(buff, "%-10s No Scale ", g_state.getStateName());
      }
      g_lcd.print(buff);
      if (_clear_disp)
      {
        g_lcd.setCursor(0,1);
        sprintf(buff, "%02d %-8s %-8s", g_config.getPreset()+1, g_config.getPresetName(), g_config.getPowderName());
        g_lcd.print(buff);
      }
      if (_clear_disp)
      {
        g_lcd.setCursor(0,2);
        sprintf(buff, "T:%6.2fgn  D:%6.2f ", g_scale.getTarget(), g_scale.getDelta());
      }
      else //TODO: fix this, must display negative delta and kernels
      {
        g_lcd.setCursor(14,2);
        if ((g_scale.getCondition() == PTScale::pan_off) || (g_scale.getCondition() == PTScale::undef))
        {
          sprintf(buff, "%s", "--.-- ");
        }
        else
        {
          sprintf(buff, "%-6.2f", g_scale.getDelta());
        }
      }
      g_lcd.print(buff);
      g_lcd.setCursor(0,3);
        if ((g_scale.getCondition() == PTScale::pan_off) || (g_scale.getCondition() == PTScale::undef))
      {
        sprintf(buff, "C:%6.2fgn K:--.-- ", g_scale.getWeight());          
      }
      else
      {
        sprintf(buff, "C:%6.2fgn K:%6.2f ", g_scale.getWeight(), g_scale.getKernels());
      }
      g_lcd.print(buff);
      _clear_disp = false;
      break;
    case g_state.pt_menu:
      g_lcd.clear();
      g_lcd.print(F("  Manual            "));
      g_lcd.setCursor(0,1);
      g_lcd.print(F("  Config            "));
      g_lcd.setCursor(0,2);
      g_lcd.print(F("  Presets           "));
      g_lcd.setCursor(0,3);
      g_lcd.print(F("  Powders           "));
      g_lcd.setCursor(0,g_cur_line);
      g_lcd.print(F(">>"));
      _clear_disp = true;
      break;
    case g_state.pt_man:
      g_lcd.clear();
      g_lcd.print(F("  Force Bump mode   "));
      g_lcd.setCursor(0,1);
      g_lcd.print(F("  Throw a charge    "));
      g_lcd.setCursor(0,2);
      g_lcd.print(F("  Trickler on/off   "));
      g_lcd.setCursor(0,3);
      g_lcd.print(F("  Home Thrower      "));
      g_lcd.setCursor(0,g_cur_line);
      g_lcd.print(F(">>"));
      _clear_disp = true;
      break;
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
      if (g_disp_edit)
      {
        g_lcd.print(F("+-"));
      }
      else 
      {
        g_lcd.print(F(">>"));
      }
      _clear_disp = true;
      break;
    case g_state.pt_cfg_bump:
      g_lcd.clear();
      g_lcd.print(F("TODO: Bmpthrsh disp "));  //TODO: used?
      _clear_disp = true;
      break;
    case g_state.pt_cfg_dec_lim:
      g_lcd.clear();
      _clear_disp = true;
      g_lcd.print(F("TODO: Dcel lim disp "));  //TODO: used?
      break;
    case g_state.pt_cfg_dec_tgt:
      g_lcd.clear();
      _clear_disp = true;
      g_lcd.print(F("TODO: Dcel tgt disp "));  //TODO: used?
      break;
    case g_state.pt_cfg_fcurve:
      g_lcd.clear();
      g_lcd.print(F("TODO: FcurveP disp  "));  //TODO: used?
      _clear_disp = true;
      break;
    case g_state.pt_presets:
      g_lcd.clear();
      g_lcd.noCursor();
      g_lcd.noBlink();
      sprintf(buff, ">>%02d <_C OK_S >_E  ", g_presets.getCurrentPreset()+1); 
      g_lcd.print(buff);
      g_lcd.setCursor(0,1);
      g_lcd.print(F("  "));
      g_lcd.setCursor(2,1);
      g_presets.getPresetName(buff);
      g_lcd.print(buff);
      if (g_presets.isDefined())
      {
        g_lcd.setCursor(0,2);
        sprintf(buff, "  Charge: %05.1f gn", g_presets.getPresetChargeWeight());
        g_lcd.print(buff);
        g_lcd.setCursor(0,3);
        g_lcd.print(F("  "));
        g_lcd.setCursor(0,3);
        if (g_presets.getPowderIndex() >= 0) 
        {
          sprintf(buff, "  %02d:", g_presets.getPowderIndex()+1);
          g_lcd.print(buff);
          g_lcd.setCursor(5,3);
          g_powders.loadPowder(g_presets.getPowderIndex());
          if (g_powders.isPowderDefined())
          {
            g_powders.getPowderName(buff);
            g_lcd.print(buff); 
          }
          else
          {
            g_lcd.print(F("EMPTY"));
          }
        }
        else 
        { 
          g_lcd.print(F("  --")); 
        }
      }
      _clear_disp = true;
      break;
    case g_state.pt_presets_edit:
      g_lcd.clear();
      //sprintf(buff, "  %02d OK=sav <=cancl", g_presets.getCurrentPreset()+1); 
      sprintf(buff, "  %02d  OK_S <_C    ", g_presets.getCurrentPreset()+1); 
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
      if (g_presets.getPowderIndex() >= 0) 
      {
        sprintf(buff, "  %02d:", g_presets.getPowderIndex()+1);
        g_lcd.print(buff);
        g_lcd.setCursor(5,3);
        g_powders.loadPowder(g_presets.getPowderIndex());
        if (g_powders.isPowderDefined())
        {
          g_powders.getPowderName(buff);
          g_lcd.print(buff); 
        }
        else
        {
          g_lcd.print(F("EMPTY"));
        }
      }
      else 
      { 
        g_lcd.print(F("  --")); 
      }
      if (g_disp_edit)
      {
        g_lcd.setCursor(g_cursor_pos, g_cur_line);
      }
      else
      {
        g_lcd.setCursor(1,g_cur_line);
      }
      g_lcd.cursor();
      g_lcd.blink();  
      _clear_disp = true;
      break;    
    case g_state.pt_powders:
      g_lcd.clear();
      g_lcd.noCursor();
      g_lcd.noBlink();
      sprintf(buff, ">>%02d  OK_S >_E     ", g_powders.getCurrentPowder()+1); 
      g_lcd.print(buff);
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
      _clear_disp = true;
      break;    
    case g_state.pt_powders_edit:
      g_lcd.clear();
      //sprintf(buff, "  %02d OK=sav <=cancl", g_powders.getCurrentPowder()+1); 
      sprintf(buff, "  %02d  OK_S <_C     ", g_powders.getCurrentPowder()+1); 
      g_lcd.print(buff);
      g_lcd.setCursor(0,1);
      g_lcd.print(F("  "));
      g_lcd.setCursor(2,1);
      g_powders.getPowderName(buff);
      g_lcd.print(buff);
      g_lcd.setCursor(0,2);
      if (g_powders.isPowderDefined())
      {
        sprintf(buff, "  Factor: %010.8f", g_powders.getPowderFactor());
      }
      else
      {
        sprintf(buff, "  Factor: %010.8f", 0);
      }
      g_lcd.print(buff);
      if (g_disp_edit)
      {
        g_lcd.setCursor(g_cursor_pos, g_cur_line);
      }
      else
      {
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
