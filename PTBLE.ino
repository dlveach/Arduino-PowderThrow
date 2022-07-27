/*
970a6f6e-e01b-11ec-9d64-0242ac120002 - Service
71adfe5e-eda2-11ec-8ea0-0242ac120002 - Command + Parameter
970a745a-e01b-11ec-9d64-0242ac120002 - Scale Weight (String)
970a75a4-e01b-11ec-9d64-0242ac120002 - Scale Target Weight (String)
970a769e-e01b-11ec-9d64-0242ac120002 - Scale Condition
970a77a2-e01b-11ec-9d64-0242ac120002 - System State
970a7892-e01b-11ec-9d64-0242ac120002 - Decel Thresh
970a798c-e01b-11ec-9d64-0242ac120002 - Config data structure
970a7aae-e01b-11ec-9d64-0242ac120002 - Preset data structure
2ab6726c-ec1a-11ec-8ea0-0242ac120002 - Preset List item
970a7c20-e01b-11ec-9d64-0242ac120002 - Powder data structure
970a71b2-e01b-11ec-9d64-0242ac120002 - Powder List Item
71ae0138-eda2-11ec-8ea0-0242ac120002 - Trickler calibration data
71ae0282-eda2-11ec-8ea0-0242ac120002 - Ladder data
71ae03ea-eda2-11ec-8ea0-0242ac120002 - Screen Navigation (more explicit and not dependent on system state, esp for local button navigigation)
71ae058e-eda2-11ec-8ea0-0242ac120002
71ae0b7e-eda2-11ec-8ea0-0242ac120002
71ae0d18-eda2-11ec-8ea0-0242ac120002
71ae0e76-eda2-11ec-8ea0-0242ac120002
71ae1010-eda2-11ec-8ea0-0242ac120002
71ae116e-eda2-11ec-8ea0-0242ac120002
*/


//TODO: refactor powder and preset FRAM data structure to include index, clean up code
//TODO: handle preset & powder save (modify current and save in FRAM)
//TODO: handle preset & powder selections in app (change current)
//TODO: add config settings edit in app, handle save
//TODO: extend config settings FRAM data structure, disp/edit, & sync with app
//TODO: lots of cleanup, debug removal etc.

#define BLE_DATA_UPDATE_INTERVAL 100

// BLE Parameter command codes
#define BLE_COMMAND_REQ_CONFIG_DATA 0x01
#define BLE_COMMAND_SET_CURRENT_PRESET 0x20
#define BLE_COMMAND_REQ_PRESET_BY_INDEX 0x21
#define BLE_COMMAND_REQ_PRESET_NAME_AT_INDEX 0x22
#define BLE_COMMAND_SET_CURRENT_POWDER 0x30
#define BLE_COMMAND_REQ_POWDER_BY_INDEX 0x31
#define BLE_COMMAND_REQ_POWDER_NAME_AT_INDEX 0x32
#define BLE_COMMAND_CALIBRATE_TRICKLER_START 0x41
#define BLE_COMMAND_CALIBRATE_TRICKLER_CANCEL 0x42
#define BLE_COMMAND_CALIBRATE_SCALE 0x43
#define BLE_COMMAND_SYSTEM_SET_STATE 0x50
#define BLE_COMMAND_SYSTEM_ESTOP 0x51
#define BLE_COMMAND_SYSTEM_AUTO_ENABLE 0x52
#define BLE_COMMAND_SYSTEM_MANUAL_RUN 0x53
#define BLE_COMMAND_MANUAL_THROW 0x61
#define BLE_COMMAND_MANUAL_TRICKLE 0x62

// BLE Screen Navigation
#define BLE_SCREEN_GO_BACK 0
#define BLE_SCREEN_MENU 1
#define BLE_SCREEN_RUN 2
#define BLE_SCREEN_SETTINGS 3
#define BLE_SCREEN_PRESETS 4
#define BLE_SCREEN_POWDERS 5

// BLE Services and Charactaristics
#define BLE_SERVICE_GUID "970a6f6e-e01b-11ec-9d64-0242ac120002"
#define BLE_PARAMETER_COMMAND_GUID "71adfe5e-eda2-11ec-8ea0-0242ac120002"
#define BLE_SCALE_WEIGHT_GUID "970a745a-e01b-11ec-9d64-0242ac120002"
#define BLE_SCALE_TARGET_GUID "970a75a4-e01b-11ec-9d64-0242ac120002"
#define BLE_SCALE_COND_GUID "970a769e-e01b-11ec-9d64-0242ac120002"
#define BLE_SYSTEM_STATE_GUID "970a77a2-e01b-11ec-9d64-0242ac120002"
#define BLE_DECEL_THRESH_GUID "970a7892-e01b-11ec-9d64-0242ac120002"
#define BLE_CONFIG_DATA_GUID "970a798c-e01b-11ec-9d64-0242ac120002"
#define BLE_PRESET_DATA_GUID "970a7aae-e01b-11ec-9d64-0242ac120002"
#define BLE_PRESET_LIST_ITEM_GUID "2ab6726c-ec1a-11ec-8ea0-0242ac120002"
#define BLE_POWDER_DATA_GUID "970a7c20-e01b-11ec-9d64-0242ac120002"
#define BLE_POWDER_LIST_ITEM_CHAR_GUID "970a71b2-e01b-11ec-9d64-0242ac120002"
#define BLE_TRICKLER_CAL_DATA_CHAR_GUID "71ae0138-eda2-11ec-8ea0-0242ac120002"
#define BLE_LADDER_DATA_CHAR_GUID "71ae0282-eda2-11ec-8ea0-0242ac120002"
#define BLE_SCREEN_NAVIGATION "71ae03ea-eda2-11ec-8ea0-0242ac120002"

// BLE misc definitions
#define PARAMETER_COMMAND_SIZE 2

// For transmitting Preset and Powder name list items
typedef struct _list_item_t {
  int list_index;
  bool empty;
  char item_name[NAME_LEN + 1];
} ListItem;
#define LIST_ITEM_SIZE sizeof(_list_item_t)
typedef union _list_item_buffer_t {
  ListItem data;
  byte raw_data[LIST_ITEM_SIZE];
} ListItemBuffer;

BLEService ptBLE_Service(BLE_SERVICE_GUID);
//TODO: for some reason trickler calibration needs parameter command "without response", it's never getting one and connection drops.
//BLECharacteristic parameterCommandChar(BLE_PARAMETER_COMMAND_GUID, BLERead | BLEWrite | BLENotify, PARAMETER_COMMAND_SIZE, true);
BLECharacteristic parameterCommandChar(BLE_PARAMETER_COMMAND_GUID, BLERead | BLEWrite | BLEWriteWithoutResponse | BLENotify, PARAMETER_COMMAND_SIZE, true);
BLEDescriptor parameterCommandDescriptor("2901", "Parameter Command");
BLECharacteristic scaleWeightChar(BLE_SCALE_WEIGHT_GUID, BLERead | BLENotify, 5, true);
BLEDescriptor scaleWeightDescriptor("2901", "Scale weight");
BLECharacteristic scaleTargetChar(BLE_SCALE_TARGET_GUID, BLERead | BLENotify, 5, true);  
BLEDescriptor scaleTargetDescriptor("2901", "Scale Target weight");
BLEIntCharacteristic scaleCondChar(BLE_SCALE_COND_GUID, BLERead | BLENotify);
BLEDescriptor scaleCondDescriptor("2901", "Scale Condition");
BLEIntCharacteristic systemStateChar(BLE_SYSTEM_STATE_GUID, BLERead | BLENotify);
BLEDescriptor systemStateDescriptor("2901", "System State");
BLEFloatCharacteristic decelThreshChar(BLE_DECEL_THRESH_GUID, BLERead | BLEWrite | BLENotify);
BLEDescriptor decelThreshDescriptor("2901", "Decel Threshold");
BLECharacteristic configDataChar(BLE_CONFIG_DATA_GUID, BLERead | BLEWrite | BLENotify, CONFIG_DATA_SIZE, true);
BLEDescriptor configDataDescriptor("2901", "Config data structure");
BLECharacteristic presetDataChar(BLE_PRESET_DATA_GUID, BLERead | BLEWrite | BLENotify, PRESET_DATA_SIZE, true);  
BLEDescriptor presetDataDescriptor("2901", "Preset Data");
BLECharacteristic presetListItemChar(BLE_PRESET_LIST_ITEM_GUID, BLERead | BLENotify, LIST_ITEM_SIZE);
BLEDescriptor presetListItemDescriptor("2901", "Preset List Item");
BLECharacteristic powderDataChar(BLE_POWDER_DATA_GUID, BLERead | BLEWrite | BLENotify, POWDER_DATA_SIZE, true);  
BLEDescriptor powderDataDescriptor("2901", "Powder Data");
BLECharacteristic powderListItemChar(BLE_POWDER_LIST_ITEM_CHAR_GUID, BLERead | BLENotify, LIST_ITEM_SIZE);
BLEDescriptor powderListItemDescriptor("2901", "Powder List Item");
BLECharacteristic tricklerCalDataChar(BLE_TRICKLER_CAL_DATA_CHAR_GUID, BLERead | BLENotify, 12, true);
BLEDescriptor tricklerCalDataDescriptor("2901", "Trickler Calibration Data");
BLECharacteristic ladderDataChar(BLE_LADDER_DATA_CHAR_GUID, BLERead | BLEWrite | BLENotify, 17, true);
BLEDescriptor ladderDataDescriptor("2901", "Ladder Data");
BLEIntCharacteristic screenNavigationChar(BLE_SCREEN_NAVIGATION, BLERead | BLEWrite | BLENotify);
BLEDescriptor screenNavigationDescriptor("2901", "Screen Navigation");

/*** Setup Bluetooth Low Energy (BLE). ***/
bool initBLE() {
  if (!BLE.begin()) {
    logError("Starting Bluetooth® Low Energy failed!", __FILE__, __LINE__, true);
    while (1);
  }
  DEBUGLN(F("BLE started"));

  // set advertised local name and service UUID:
  BLE.setDeviceName("PowderThrow");
  BLE.setLocalName("Nano 33 BLE");
  BLE.setAdvertisedService(ptBLE_Service);

  // add descriptors to the charactaristics
  parameterCommandChar.addDescriptor(parameterCommandDescriptor);
  scaleWeightChar.addDescriptor(scaleWeightDescriptor);
  scaleTargetChar.addDescriptor(scaleTargetDescriptor);
  scaleCondChar.addDescriptor(scaleCondDescriptor);
  systemStateChar.addDescriptor(systemStateDescriptor);
  decelThreshChar.addDescriptor(decelThreshDescriptor);
  configDataChar.addDescriptor(configDataDescriptor);
  presetDataChar.addDescriptor(presetDataDescriptor);
  presetListItemChar.addDescriptor(presetListItemDescriptor);
  powderDataChar.addDescriptor(powderDataDescriptor);
  powderListItemChar.addDescriptor(powderListItemDescriptor);
  tricklerCalDataChar.addDescriptor(tricklerCalDataDescriptor);
  ladderDataChar.addDescriptor(ladderDataDescriptor);
  screenNavigationChar.addDescriptor(screenNavigationDescriptor);

  // add the characteristic to the service
  ptBLE_Service.addCharacteristic(parameterCommandChar);
  ptBLE_Service.addCharacteristic(scaleWeightChar);
  ptBLE_Service.addCharacteristic(scaleTargetChar);
  ptBLE_Service.addCharacteristic(scaleCondChar);
  ptBLE_Service.addCharacteristic(systemStateChar);
  ptBLE_Service.addCharacteristic(decelThreshChar);
  ptBLE_Service.addCharacteristic(configDataChar);
  ptBLE_Service.addCharacteristic(presetDataChar);
  ptBLE_Service.addCharacteristic(presetListItemChar);
  ptBLE_Service.addCharacteristic(powderDataChar);
  ptBLE_Service.addCharacteristic(powderListItemChar);
  ptBLE_Service.addCharacteristic(tricklerCalDataChar);
  ptBLE_Service.addCharacteristic(ladderDataChar);
  ptBLE_Service.addCharacteristic(screenNavigationChar);

  // add service
  BLE.addService(ptBLE_Service);

  // assign event handlers for connected, disconnected to peripheral
  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  // assign event handlers for characteristics
  decelThreshChar.setEventHandler(BLEWritten, decelThreshSliderCharWritten);
  parameterCommandChar.setEventHandler(BLEWritten, parameterCommandCharWritten);
  presetDataChar.setEventHandler(BLEWritten, presetDataCharWritten);
  powderDataChar.setEventHandler(BLEWritten, powderDataCharWritten);
  configDataChar.setEventHandler(BLEWritten, configDataCharWritten);
  ladderDataChar.setEventHandler(BLEWritten, ladderDataCharWritten);

  BLE.advertise();
  DEBUGLN(F("Bluetooth® device active, waiting for connections.."));
}

/* Handle a connection from BLE central */
void blePeripheralConnectHandler(BLEDevice central) {
  DEBUGP(F("Connected event, central: "));
  DEBUGLN(central.address());
  g_mcp.digitalWrite(MCP_LED_PUR_PIN, HIGH);
  delay(500);
  updateBLEData(true);
}

/* Handle a disconnect from the BLE central */
void blePeripheralDisconnectHandler(BLEDevice central) {
  DEBUGP(F("Disconnected event, central: "));
  DEBUGLN(central.address());
  g_mcp.digitalWrite(MCP_LED_PUR_PIN, LOW);
  // TESTING: try clearing ladder data
  g_config.ladder_data.is_configured = false;
}

/* Handler for decel threshold update from device slider.  */
void decelThreshSliderCharWritten(BLEDevice central, BLECharacteristic characteristic) {
  static float floatval;
  characteristic.readValue(&floatval, 4);
  g_config.setDecelThreshold(floatval);
  g_config.saveConfig();
  if (g_state.getState() == g_state.pt_cfg) {
    g_display_changed = true;
    displayUpdate();
  }
}

union ByteArrayToValue {
  byte array[4];
  int32_t int_value;
  float_t float_value;
};

/*** Handler for config data update (on save) from central. ***/
void configDataCharWritten(BLEDevice central, BLECharacteristic characteristic) {
  DEBUGLN(F("ConfigDataCharWritten()"));
  printByteData(characteristic.value(), characteristic.valueSize());

  ConfigDataStorage new_config;
  memcpy(new_config.raw_data, characteristic.value(), characteristic.valueLength());
  if (new_config._config_data.config_version != CONFIG_VERSION) {
    logError("Config version out of date in BLE data.", __FILE__, __LINE__);
  } else {
    g_config.setPreset(new_config._config_data.preset);
    g_config.setFcurveP(new_config._config_data.fscaleP);
    g_config.setDecelThreshold(new_config._config_data.decel_threshold);
    g_config.setBumpThreshold(new_config._config_data.bump_threshold);
    g_config.setDecelLimit(new_config._config_data.decel_limit);
    g_config.setGnTolerance(new_config._config_data.gn_tolerance);
    g_config.setTricklerSpeed(new_config._config_data.trickler_speed);
    g_config.saveConfig();
    g_display_changed = true;
    displayUpdate(true);
  }
}

/*** Handler for ladder data update from central. ***/
void ladderDataCharWritten(BLEDevice central, BLECharacteristic characteristic) {
  DEBUGLN(F("ladderDataCharWritten()"));
  static char buff[NAME_LEN + 1];
  static ByteArrayToValue converter;
  printByteData(characteristic.value(), characteristic.valueSize());
  memcpy(&g_config.ladder_data.is_configured, &characteristic.value()[0], 1);
  if (g_config.ladder_data.is_configured) {
    memcpy(converter.array, &characteristic.value()[1], 4);
    g_config.ladder_data.step_count = converter.int_value;
    memcpy(converter.array, &characteristic.value()[5], 4);
    g_config.ladder_data.current_step = converter.int_value;
    memcpy(converter.array, &characteristic.value()[9], 4);
    g_config.ladder_data.start_weight = converter.float_value;
    memcpy(converter.array, &characteristic.value()[13], 4);
    g_config.ladder_data.step_interval = converter.float_value;
    g_state.setState(PTState::pt_ladder);
    g_config.setRunMode(PTConfig::pt_ladder);
    g_display_changed = true;
    displayUpdate(true);
  } else {
    g_config.ladder_data.step_count = 0;
    g_config.ladder_data.current_step = 0;
    g_config.ladder_data.start_weight = 0.0;
    g_config.ladder_data.step_interval = 0.0;
    g_state.setState(PTState::pt_manual);
    g_config.setRunMode(PTConfig::pt_manual);    
    setConfigPresetData();
    g_display_changed = true;
    displayUpdate(true);
  }
}

/*** Handler for preset data update (on save) from central. ***/
void presetDataCharWritten(BLEDevice central, BLECharacteristic characteristic) {
  DEBUGLN(F("presetDataCharWritten"));
  printByteData(characteristic.value(), characteristic.valueSize());
  static ByteArrayToValue converter;
  static char buff[NAME_LEN + 1];
  memcpy(converter.array, &characteristic.value()[0], 4);
  int preset_version = converter.int_value;
  memcpy(converter.array, &characteristic.value()[4], 4);
  int preset_number = converter.int_value;
  memcpy(converter.array, &characteristic.value()[8], 4);
  float charge_weight = converter.float_value;
  g_presets.setPresetChargeWeight(charge_weight);
  memcpy(converter.array, &characteristic.value()[12], 4);
  int powder_index = converter.int_value;
  g_presets.setPowderIndex(powder_index);
  memcpy(converter.array, &characteristic.value()[16], 4);
  int bullet_weight = converter.int_value;
  g_presets.setBulletWeight(bullet_weight);
  memcpy(buff, &characteristic.value()[20], NAME_LEN);
  buff[NAME_LEN] = 0x00;
  g_presets.setPresetName(buff);
  memcpy(buff, &characteristic.value()[37], NAME_LEN);
  buff[NAME_LEN] = 0x00;
  g_presets.setBulletName(buff);
  memcpy(buff, &characteristic.value()[54], NAME_LEN);
  buff[NAME_LEN] = 0x00;
  g_presets.setBrassName(buff);
  if (preset_version != PRESETS_VERSION) {
    logError("BLE Data read, Preset Version is out of date.'", __FILE__, __LINE__);
    return;
  }
  // Save the data
  g_presets.savePreset();
  setConfigPresetData();
  // Update display
  g_display_changed = true;
  displayUpdate(true);
}

/*** Handler for pwoder data update (on save) from central. ***/
void powderDataCharWritten(BLEDevice central, BLECharacteristic characteristic) {
  DEBUGLN(F("powderDataCharWritten()"));
  printByteData(characteristic.value(), characteristic.valueSize());
  static ByteArrayToValue converter;
  static char buff[NAME_LEN + 1];
  memcpy(converter.array, &characteristic.value()[0], 4);
  int powder_version = converter.int_value;
  memcpy(converter.array, &characteristic.value()[4], 4);
  int powder_number = converter.int_value;
  memcpy(converter.array, &characteristic.value()[8], 4);
  float powder_factor = converter.float_value;
  g_powders.setPowderFactor(powder_factor);
  memcpy(buff, &characteristic.value()[12], NAME_LEN);
  buff[NAME_LEN] = 0x00;
  g_powders.setPowderName(buff);
  memcpy(buff, &characteristic.value()[29], NAME_LEN);
  buff[NAME_LEN] = 0x00;
  g_powders.setPowderLot(buff);
  if (powder_version != POWDERS_VERSION) {
    logError("BLE Data read, Powder Version is out of date.'", __FILE__, __LINE__);
    return;
  }
  // Save the data
  g_powders.savePowder();
  setConfigPresetData();  
  // Update display
  g_display_changed = true;
  displayUpdate(true);
}

/* Handler for parameter command.  See BLE Parameter command codes definitions above. */
void parameterCommandCharWritten(BLEDevice central, BLECharacteristic characteristic) {
  static ListItemBuffer list_buffer;    // buffer for writing a list item
  static PresetDataStorage preset;      // buffer for preset data structure retrieval
  static PowderDataStorage powder;      // buffer for preset data structure retrieval
  static char name_buff[NAME_LEN + 1];  // buffer for preset/powder names
  static int parameter;                 // the command parameter

  //BLE.poll();  //allow BLE to provide write response?  Didn't help.

  //byte cmd = byte(characteristic.value()[0]);
  parameter = int(characteristic.value()[1]);
  int index = parameter - 1;  //Parameter is Preset number, 1 based, preset list is 0 based
  switch (byte(characteristic.value()[0])) {

    case BLE_COMMAND_MANUAL_THROW:
      DEBUGLN(F("BLE Command: Manual Throw"));
      if (g_scale.getMode() == PTScale::pan_off) {
        DEBUGLN(F("Pan is not on scale, cannot throw."));
      } else if (g_state.getState() == PTState::pt_man_throw) { 
        DEBUGLN(F("Already throwing, ignore."));
      } else {  
        manualThrow();
      }
      break;

    case BLE_COMMAND_MANUAL_TRICKLE:
      DEBUGLN(F("BLE Command: Manual Trickle"));
      toggleTrickler();
      break;

    case BLE_COMMAND_CALIBRATE_TRICKLER_START:
      DEBUGLN(F("BLE Command: Calibratre Trickler Start"));
      if (g_scale.getCondition() == g_scale.pan_off) { return; 
      } else if (g_scale.getMode() != SCALE_MODE_GRAIN) { return; }
      g_state.setState(g_state.pt_man_cal_trickler);
      calibrateTrickler();
      g_state.setState(g_state.pt_cfg); //return to config settings if started from BLE
      g_display_changed = true;
      displayUpdate(true);
      break;

    case BLE_COMMAND_CALIBRATE_TRICKLER_CANCEL:
      DEBUGLN(F("BLE Command: Calibratre Trickler Stop"));
      if (g_state.getState() == g_state.pt_man_cal_trickler) {
        calibrateTrickler(); // called again while calibrating will toggle it off
      }
      break;

    case BLE_COMMAND_CALIBRATE_SCALE:
      Serial.println("BLE Command: Calibratre Scale");
      Serial.println("TODO: impliment");
      break;

    case BLE_COMMAND_SYSTEM_ESTOP:
      DEBUGLN(F("BLE Command: System EStop"));
      stopAll(true);
      break;

    case BLE_COMMAND_SYSTEM_AUTO_ENABLE:
      // Ignore if not in correct state
      if ((g_state.getState() == PTState::pt_ready) || 
         (g_state.getState() == PTState::pt_manual) ||
         (g_state.getState() == PTState::pt_ladder)) 
      {
        switch (parameter) {
          case 0:
              g_state.setState(PTState::pt_ready);
              g_config.setRunMode(PTConfig::pt_auto);
              setConfigPresetData();
            break;
          case 1:
            if (g_config.ladder_data.is_configured) {
              g_state.setState(PTState::pt_ladder);
              g_config.setRunMode(PTConfig::pt_ladder);
            } else {
              g_state.setState(PTState::pt_manual);
              g_config.setRunMode(PTConfig::pt_manual);
              setConfigPresetData();
            }
            break;
          default:
              logError("Command System Auto Enable: Unknown parameter.", __FILE__, __LINE__);
              return;
              break;
            break;
        }
        g_display_changed = true;
        displayUpdate(true);
      }
      break;

    case BLE_COMMAND_SYSTEM_MANUAL_RUN:
      if (g_state.getState() == PTState::pt_ladder) {
        g_state.setState(PTState::pt_ladder_run);
      } else if (g_state.getState() == PTState::pt_manual) {
        g_state.setState(PTState::pt_manual_run);
      }
      break;

    case BLE_COMMAND_SET_CURRENT_PRESET:
      DEBUGLN(F("BLE Command: Set Current Preset"));
      setConfigPresetData();
      g_display_changed = true;
      displayUpdate(true);  //TODO: this or
      //TODO: update BLE runtime?
      //TODO: am I using this command?
      break;

    case BLE_COMMAND_SET_CURRENT_POWDER:
      Serial.println("BLE Command: Set Current Powder");
      //Serial.print("Parameter: ");
      //Serial.print(parameter);
      //Serial.print(" Index: ");
      //Serial.println(index);
      Serial.println("TODO: Am I using this command?");
      break;

    case BLE_COMMAND_REQ_PRESET_BY_INDEX:
      DEBUGLN(F("BLE Command: Request Preset by index."));
      if ((index >= 0) || (index < MAX_PRESETS)) {
        BLEWritePresetDataAt(index);
        g_presets.loadPreset(index);
        if (g_presets.isDefined()) { 
          setConfigPresetData(); 
          BLEWritePowderDataAt(g_presets.getPowderIndex());  //TESTING FIX
        }
        if (g_state.getState() == g_state.pt_presets) {
          g_display_changed = true;
          displayUpdate(true);
        }
      } else {
        logError("Preset index out of range", __FILE__, __LINE__);
      }
      break;

    case BLE_COMMAND_REQ_PRESET_NAME_AT_INDEX:
      //Serial.println("BLE Command: Request Preset Name by index.");
      if (g_presets.getBLEDataStruct(preset.raw_data, index)) {
        if (preset._preset_data.preset_version != PRESETS_VERSION) {
          g_presets.getDefaults(preset.raw_data, sizeof(preset.raw_data));
        } else {
          snprintf(list_buffer.data.item_name, NAME_LEN + 1, "%s", preset._preset_data.preset_name);
        }
        if (preset._preset_data.powder_index >= 0) {  //TODO: add empty flag to preset data struct
          list_buffer.data.empty = false;
        } else {
          list_buffer.data.empty = true;
        }
        list_buffer.data.list_index = parameter;
        if (!presetListItemChar.writeValue(list_buffer.raw_data, LIST_ITEM_SIZE)) { logError("BLE Failed to write preset list item.", __FILE__, __LINE__); }
      } else {
        logError("Failed to load preset buffer", __FILE__, __LINE__);
      }
      break;

    case BLE_COMMAND_REQ_POWDER_BY_INDEX:
      DEBUGLN(F("BLE Command: Request Powder by index."));
      if ((index >= 0) || (index < MAX_POWDERS)) {
        g_powders.loadPowder(index);
        BLEWritePowderDataAt(index);
        if (g_state.getState() == g_state.pt_powders) {
          g_display_changed = true;
          displayUpdate(true);
        }
      } else { logError("Powder index out of range", __FILE__, __LINE__); }
      break;

    case BLE_COMMAND_REQ_POWDER_NAME_AT_INDEX:
      //DEBUGLN(F("BLE Command: Request Powder Name by index."));
      if (g_powders.getBLEDataStruct(powder.raw_data, index)) {
        if (powder._powder_data.powder_version != POWDERS_VERSION) {
          g_powders.getDefaults(powder.raw_data, sizeof(powder.raw_data));
        } else {
          snprintf(list_buffer.data.item_name, NAME_LEN + 1, "%s", powder._powder_data.powder_name);
        }
        if (powder._powder_data.powder_factor > 0) {  //TODO: add empty flag to powder data struct?
          list_buffer.data.empty = false;
        } else {
          list_buffer.data.empty = true;
        }
        list_buffer.data.list_index = parameter;
        if (!powderListItemChar.writeValue(list_buffer.raw_data, LIST_ITEM_SIZE)) { logError("BLE Failed to write powder list item", __FILE__, __LINE__); }
      } else {
        logError("Failed to load powder buffer", __FILE__, __LINE__);
      }
      break;

    case BLE_COMMAND_SYSTEM_SET_STATE:
      DEBUGLN(F("BLE Command: Set System State."));
      if (g_state.isValidState(parameter)) {
        switch ((PTState::state_t)parameter) {
          case g_state.pt_ready:
            //This should be checked on app side but just to be sure, check again
            //TODO: is there any way to respond to this BLE command in app?
            if (g_config.isRunReady()) {
              g_state.setState(g_state.pt_ready);
            }
            break;
          default:
            g_state.setState((PTState::state_t)parameter);
            break;
        }
        g_display_changed = true;
        displayUpdate();
      } else {
        logError("Parameter is an unknown state", __FILE__, __LINE__);
      }
      break;

    default:
      logError("BLE Parameter Command: Unknown command", __FILE__, __LINE__);
      break;
  }
}

/* 
 * Dynamically update runtime BLE data.
 * Updates any advertized runtime ("pushed" data) if data has changed  
 * since last update interval.
 * 
 * TODO: make most of this on demand.  Only RT scale data (weight, mode, etc.) needs
 * to be sent immediately.
 */
void updateBLEData(bool force) {
  static long _last_update = 0;
  static char* name_buff = new char[32];
  static uint8_t* value_buff = new uint8_t[5];
  static float _last_scale_weight = 0.0;
  static int _last_scale_cond = -1;
  static float _last_target_weight = 0.0;
  static int _last_system_state = -1;
  static float _last_decel_thresh = -1;
  static int _last_preset_number = -1;
  static int _last_scale_mode = -1;

  static float scale_weight;
  static float target_weight;
  static int scale_cond;
  static int system_state;
  static float decel_thresh;
  static bool scale_mode_changed;

  if (!BLE.connected()) { return; }
  if (!g_scale.isConnected()) { return; }

  if (_last_update == 0) { _last_update = millis(); }                    //init first time
  if (_last_scale_mode == -1) { _last_scale_mode = g_scale.getMode(); }  //init first time

  if ((millis() - _last_update > BLE_DATA_UPDATE_INTERVAL) || force) {
    _last_update = millis();
    if (force) { DEBUGLN(F("BLE forced update of data")); }
    scale_weight = g_scale.getWeight();
    target_weight = g_config.getTargetWeight();
    scale_cond = g_scale.getCondition();
    system_state = g_state.getState();
    decel_thresh = g_config.getDecelThreshold();
    if (g_scale.getMode() != _last_scale_mode) {
      scale_mode_changed = true;
      _last_scale_mode = g_scale.getMode();
    } else {
      scale_mode_changed = false;
    }
    //only update things if changed or forced
    if ((scale_cond != _last_scale_cond) || force) {
      if (!scaleCondChar.writeValue(scale_cond)) { logError("BLE Failed to write scale condition.", __FILE__, __LINE__); }
      _last_scale_cond = scale_cond;
    }
    if ((system_state != _last_system_state) || force) {
      if (!systemStateChar.writeValue(system_state)) { logError("BLE Failed to write system state.", __FILE__, __LINE__); }
      _last_system_state = system_state;
    }
    if ((scale_weight != _last_scale_weight) || scale_mode_changed || force) {
      if (g_scale.getMode() == SCALE_MODE_GRAM) {
        value_buff[0] = 0;
      } else {
        value_buff[0] = 1;
      }
      memcpy(&value_buff[1], &scale_weight, 4);
      if (!scaleWeightChar.writeValue(value_buff, 5)) { logError("BLE Failed to write scale weight.", __FILE__, __LINE__); }
      _last_scale_weight = scale_weight;
    }

    //TODO: The rest of this should be on demand?

    if ((target_weight != _last_target_weight) || scale_mode_changed || force) {  
      if (g_scale.getMode() == SCALE_MODE_GRAM) {
        value_buff[0] = 0;
      } else {
        value_buff[0] = 1;
      }
      memcpy(&value_buff[1], &target_weight, 4);
      if (!scaleTargetChar.writeValue(value_buff, 5)) { logError("BLE Failed to write target weight.", __FILE__, __LINE__); }
      _last_target_weight = target_weight;
    }

    if ((decel_thresh != _last_decel_thresh) || force) {  //TODO: make on demand as part of config data manager in ap
      if (!decelThreshChar.writeValue(decel_thresh)) { logError("BLE Failed to write decel thresh.", __FILE__, __LINE__); }
      _last_decel_thresh = decel_thresh;
    }

    if (g_config.isBLEUpdateNeeded()) {  //TODO: make on demand, use config data manager in app
      if (!g_config.updateBLE(configDataChar)) { logError("BLE Failed to write config data.", __FILE__, __LINE__); }
    }

    if (force) {
      if (g_presets.loadPreset(g_config.getPreset())) {
        BLEWritePresetDataAt(g_config.getPreset());
        BLEWritePowderDataAt(g_presets.getPowderIndex());
      } else {
        logError("Failed to load preset in forced write.", __FILE__, __LINE__);
      }
    }
  }
}

/////////////////////
// Support functions for both dynamic and on demand BLE updates:
/////////////////////

void BLEWriteScreenChange(int new_screen) {
  if (!screenNavigationChar.writeValue(new_screen)) { logError("BLE Failed to write screen change.", __FILE__, __LINE__); }
}

void BLEWritePresetDataAt(int index) {
  static PresetDataStorage preset;
    if (!g_presets.getBLEDataStruct(preset.raw_data, index)) {
      logError("Failed to load requested preset data.", __FILE__, __LINE__);  //TODO: any good way to handle this?
    } else {
      if (preset._preset_data.preset_version != PRESETS_VERSION) {
        DEBUGLN(F("WARN: Preset undefined: set to defaults"));
        g_presets.getDefaults(preset.raw_data, sizeof(preset.raw_data));
        preset._preset_data.preset_number = index + 1;
      } 
      DEBUGLN(F("Writing preset to BLE"));
      printByteData(preset.raw_data, sizeof(preset.raw_data));
      if (!presetDataChar.writeValue(preset.raw_data, sizeof(preset.raw_data))) { logError("BLE Failed to write preset data.", __FILE__, __LINE__); }
    }
}

void BLEWritePowderDataAt(int index) {
  static PowderDataStorage powder;
  if (!g_powders.getBLEDataStruct(powder.raw_data, index)) {
    logError("Failed to load requested powder data.", __FILE__, __LINE__);  //TODO: any good way to handle this?
  } else {
      if (powder._powder_data.powder_version != POWDERS_VERSION) {
        DEBUGLN(F("WARN: Powder undefined: set to defaults"));
        g_powders.getDefaults(powder.raw_data, sizeof(powder.raw_data));
        powder._powder_data.powder_number = index + 1;
      } 
      DEBUGLN(F("Writing powder to BLE"));
      printByteData(powder.raw_data, sizeof(powder.raw_data));
    if (!powderDataChar.writeValue(powder.raw_data, sizeof(powder.raw_data))) { logError("BLE Failed to write powder data.", __FILE__, __LINE__); }
  }
}

void BLEWriteTricklerCalData(uint8_t* _data, int len) {
  printByteData(_data, len);
  if (!tricklerCalDataChar.writeValue(_data, len)) { logError("BLE Failed to write trickler cal data.", __FILE__, __LINE__); }
}
