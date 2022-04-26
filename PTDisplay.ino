
/*
 * 
 */
void displayUpdate()
{
//                        12345678901234567890
  static char buff[21] = "                    ";
  
  if (!g_display_changed) return;
  g_display_changed = false;
  switch (g_state.getState())
  {
    case g_state.pt_undefined:
    case g_state.pt_setup:
    case g_state.pt_error:
    case g_state.pt_cal_scale:
    case g_state.pt_cal_trickler:
      //Direct display output states. Nothing to do here.  Generally shouldn't happen.
      DEBUGP(g_state.getStateName());
      DEBUGLN(F(" is a direct output state, no display update."));
      return;
    case g_state.pt_ready:
      g_lcd.clear();
      switch(g_scale.getCondition())
      {
        case PTScale::undef:
          sprintf(buff, "Pre: %02d  No Scale ", g_config.getPreset()+1);
          break;
        case PTScale::zero:
          sprintf(buff, "Pre: %02d  Sys Ready", g_config.getPreset()+1);
          break;
        case PTScale::pan_off:
          sprintf(buff, "Pre: %02d  Pan Off  ", g_config.getPreset()+1);
          break;
        case PTScale::under_tgt:
          sprintf(buff, "Pre: %02d  Under tgt", g_config.getPreset()+1);
          break;
        case PTScale::close_to_tgt:
          sprintf(buff, "Pre: %02d  Close tgt", g_config.getPreset()+1);
          break;
        case PTScale::on_tgt:
          sprintf(buff, "Pre: %02d  On Target", g_config.getPreset()+1);
          break;
        case PTScale::over_tgt:
          sprintf(buff, "Pre: %02d  OVERTHROW", g_config.getPreset()+1);
          break;
        default:
          sprintf(buff, "Pre: %02d  Unkn cond", g_config.getPreset()+1);
          break;
      }
      g_lcd.print(buff);
      g_lcd.setCursor(0,1);
      g_presets.getPresetName(buff);
      buff[10]=0x00;
      g_lcd.print(buff);
      g_powders.loadPowder(g_presets.getPowderIndex());
      g_powders.getPowderName(buff);
      buff[9]=0x00;
      g_lcd.setCursor(11,1);
      g_lcd.print(buff);
      g_lcd.setCursor(0,2);
      g_presets.loadPreset(g_config.getPreset());
      sprintf(buff, "T:%6.2fgn D:%6.2f ", g_scale.getTarget(), g_scale.getDelta());
      g_lcd.print(buff);
      g_lcd.setCursor(0,3);
      sprintf(buff, "C:%6.2fgn K:%6.1f ", g_scale.getWeight(), g_scale.getKernels());
      g_lcd.print(buff);
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
      break;
    case g_state.pt_cfg_bump:
      g_lcd.clear();
      g_lcd.print(F("TODO: Bmpthrsh disp "));
      break;
    case g_state.pt_cfg_dec_lim:
      g_lcd.clear();
      g_lcd.print(F("TODO: Dcel lim disp "));
      break;
    case g_state.pt_cfg_dec_tgt:
      g_lcd.clear();
      g_lcd.print(F("TODO: Dcel tgt disp "));
      break;
    case g_state.pt_cfg_fcurve:
      g_lcd.clear();
      g_lcd.print(F("TODO: FcurveP disp  "));
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
      break;
    default:
      DEBUGP(g_state.getStateName());
      DEBUGLN(F(" is unknown state in displayUpdate()"));
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
