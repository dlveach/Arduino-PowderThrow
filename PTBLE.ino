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
71ae0138-eda2-11ec-8ea0-0242ac120002
71ae0282-eda2-11ec-8ea0-0242ac120002
71ae03ea-eda2-11ec-8ea0-0242ac120002
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
#define BLE_COMMAND_SET_SYSTEM_STATE 0x50

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
BLECharacteristic parameterCommandChar(BLE_PARAMETER_COMMAND_GUID, BLERead | BLEWrite | BLENotify, PARAMETER_COMMAND_SIZE, true);
BLEDescriptor parameterCommandDescriptor("2901", "Parameter Command");
BLECharacteristic scaleWeightChar(BLE_SCALE_WEIGHT_GUID, BLERead | BLENotify, 5, true);
BLEDescriptor scaleWeightDescriptor("2901", "Scale weight");
BLECharacteristic scaleTargetChar(BLE_SCALE_TARGET_GUID, BLERead | BLENotify, 5, true);  // use structure (when scale holds copy of current preset)
BLEDescriptor scaleTargetDescriptor("2901", "Scale Target weight");
BLEIntCharacteristic scaleCondChar(BLE_SCALE_COND_GUID, BLERead | BLENotify);
BLEDescriptor scaleCondDescriptor("2901", "Scale Condition");
BLEIntCharacteristic systemStateChar(BLE_SYSTEM_STATE_GUID, BLERead | BLENotify);
BLEDescriptor systemStateDescriptor("2901", "System State");
BLEFloatCharacteristic decelThreshChar(BLE_DECEL_THRESH_GUID, BLERead | BLEWrite | BLENotify);
BLEDescriptor decelThreshDescriptor("2901", "Decel Threshold");
BLECharacteristic configDataChar(BLE_CONFIG_DATA_GUID, BLERead | BLEWrite | BLENotify, CONFIG_DATA_SIZE, true);
BLEDescriptor configDataDescriptor("2901", "Config data structure");
BLECharacteristic presetDataChar(BLE_PRESET_DATA_GUID, BLERead | BLEWrite | BLENotify, PRESET_DATA_SIZE + 4, true);  //TODO: fix this when preset_index moved in preset data structure
BLEDescriptor presetDataDescriptor("2901", "Preset Data");
BLECharacteristic presetListItemChar(BLE_PRESET_LIST_ITEM_GUID, BLERead | BLENotify, LIST_ITEM_SIZE);
BLEDescriptor presetListItemDescriptor("2901", "Preset List Item");
BLECharacteristic powderDataChar(BLE_POWDER_DATA_GUID, BLERead | BLENotify, POWDER_DATA_SIZE + 4, true);  //TODO: fix this when powder_index moved in preset data structure
BLEDescriptor powderDataDescriptor("2901", "Powder Data");
BLECharacteristic powderListItemChar(BLE_POWDER_LIST_ITEM_CHAR_GUID, BLERead | BLENotify, LIST_ITEM_SIZE);
BLEDescriptor powderListItemDescriptor("2901", "Powder List Item");


/*** Setup Bluetooth Low Energy (BLE). ***/
bool initBLE() {
  if (!BLE.begin()) {
    Serial.println("Starting Bluetooth® Low Energy failed!");
    logError("Starting Bluetooth® Low Energy failed!", __FILE__, __LINE__, true);
    while (1)
      ;
  }
  Serial.println("BLE started");

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

  // add service
  BLE.addService(ptBLE_Service);

  // assign event handlers for connected, disconnected to peripheral
  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  // assign event handlers for characteristics
  decelThreshChar.setEventHandler(BLEWritten, decelThreshSliderCharWritten);
  parameterCommandChar.setEventHandler(BLEWritten, parameterCommandCharWritten);
  presetDataChar.setEventHandler(BLEWritten, presetDataCharWritten);

  BLE.advertise();
  Serial.println("Bluetooth® device active, waiting for connections..");
}

/* Handle a connection from BLE central */
void blePeripheralConnectHandler(BLEDevice central) {
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
  g_mcp.digitalWrite(MCP_LED_PUR_PIN, HIGH);
  delay(500);
  updateBLEData(true);
}

/* Handle a disconnect from the BLE central */
void blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
  g_mcp.digitalWrite(MCP_LED_PUR_PIN, LOW);
}

/*
 * Handler for decel threshold update from device slider
 * TODO: Write FRAM every update?  Use user action with button on phone?
 */
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

/*** Handler for preset data update (on save) from central. ***/
void presetDataCharWritten(BLEDevice central, BLECharacteristic characteristic) {
  Serial.println("TODO: parse data and update preset at specified preset index.");
  Serial.print("valueSize: ");
  Serial.println(characteristic.valueSize());
  Serial.print("valueLength: ");
  Serial.println(characteristic.valueLength());
  Serial.print("Data recieved: ");
  printByteData(characteristic.value(), characteristic.valueSize());
  Serial.println();

  //parse the data

  static ByteArrayToValue converter;
  static char buff[NAME_LEN + 1];

  memcpy(converter.array, &characteristic.value()[0], 4);
  int preset_index = converter.int_value;
  Serial.print("Preset Index: ");
  Serial.println(preset_index);

  memcpy(converter.array, &characteristic.value()[4], 4);
  float charge_weight = converter.float_value;
  Serial.print("Charge Weight: ");
  Serial.println(charge_weight);
  g_presets.setPresetChargeWeight(charge_weight);

  memcpy(converter.array, &characteristic.value()[8], 4);
  int powder_index = converter.int_value;
  Serial.print("Powder Index: ");
  Serial.println(powder_index);
  g_presets.setPowderIndex(powder_index);

  memcpy(buff, &characteristic.value()[12], NAME_LEN);
  buff[NAME_LEN] = 0x00;
  Serial.print("Preset Name: ");
  Serial.println(buff);
  g_presets.setPresetName(buff);

  memcpy(buff, &characteristic.value()[29], NAME_LEN);
  buff[NAME_LEN] = 0x00;
  Serial.print("Bullet Name: ");
  Serial.println(buff);
  g_presets.setBulletName(buff);

  memcpy(converter.array, &characteristic.value()[48], 4);
  int bullet_weight = converter.int_value;
  Serial.print("Bullet Weight: ");
  Serial.println(bullet_weight);
  g_presets.setBulletWeight(bullet_weight);

  memcpy(buff, &characteristic.value()[52], NAME_LEN);
  buff[NAME_LEN] = 0x00;
  Serial.print("Brass Name: ");
  Serial.println(buff);
  g_presets.setBrassName(buff);

  memcpy(converter.array, &characteristic.value()[72], 4);
  int preset_version = converter.int_value;
  Serial.print("Preset Version: ");
  Serial.println(preset_version);

  if (preset_version != PRESETS_VERSION) {
    Serial.print("ERROR: BLE Data read, Preset Version '");
    Serial.print(preset_version);
    Serial.println("' is out of sync.");
    Serial.println("Data not saved.");
    return;
  }

  // Save the data
  Serial.print("Overwriting current preset at index: ");
  Serial.println(g_presets.getCurrentPreset());

  g_presets.savePreset();

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
  byte cmd = byte(characteristic.value()[0]);
  int parameter = int(characteristic.value()[1]);
  int index = parameter - 1;  //Parameter is Preset number, 1 based, preset list is 0 based
  switch (cmd) {

    case BLE_COMMAND_SET_CURRENT_PRESET:
      Serial.print("Set current preset. Parameter: ");
      Serial.print(parameter);
      Serial.print(" Index: ");
      Serial.println(index);
      setConfigPresetData();
      displayUpdate(true);  //TODO: this or
      //g_display_changed = true;

      //TODO: update BLE runtime?

      //TODO: am I using this command?

      break;

    case BLE_COMMAND_SET_CURRENT_POWDER:
      Serial.print("Set current powder. Parameter: ");
      Serial.print(parameter);
      Serial.print(" Index: ");
      Serial.println(index);

      //TODO: Am I using this command?

      break;

    case BLE_COMMAND_REQ_PRESET_BY_INDEX:
      if ((index >= 0) || (index < MAX_PRESETS)) {
        Serial.print("Current Preset index was: ");
        Serial.println(g_presets.getCurrentPreset());
        Serial.print("Preset data requested at index: ");
        Serial.println(index);
        BLEWritePresetDataAt(index);
        g_presets.loadPreset(index);
        if (g_presets.isDefined()) {
          setConfigPresetData();
        }
        Serial.print("Current Preset index now: ");
        Serial.println(g_presets.getCurrentPreset());
        if (g_state.getState() == g_state.pt_presets) {
          g_display_changed = true;
          displayUpdate(true);
        }
      } else {
        Serial.print("ERROR: preset index out of range at ");
        Serial.println(__LINE__);
      }
      break;

    case BLE_COMMAND_REQ_PRESET_NAME_AT_INDEX:
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
        if (!presetListItemChar.writeValue(list_buffer.raw_data, LIST_ITEM_SIZE)) { Serial.println("BLE Failed to write preset list item."); }
      } else {
        Serial.println("ERROR: Failed to load preset buffer");
      }
      break;

    case BLE_COMMAND_REQ_POWDER_BY_INDEX:
      if ((index > 0) || (index < MAX_POWDERS)) {
        BLEWritePowderDataAt(index);
      } else {
        Serial.print("ERROR: powder index out of range at ");
        Serial.println(__LINE__);
      }
      break;

    case BLE_COMMAND_REQ_POWDER_NAME_AT_INDEX:
      if (g_powders.getBLEDataStruct(powder.raw_data, index)) {
        if (powder._powder_data.powder_version != POWDERS_VERSION) {
          g_powders.getDefaults(powder.raw_data, sizeof(powder.raw_data));
        } else {
          snprintf(list_buffer.data.item_name, NAME_LEN + 1, "%s", powder._powder_data.powder_name);
        }
        if (powder._powder_data.powder_factor > 0) {  //TODO: add empty flag to powder data struct
          list_buffer.data.empty = false;
        } else {
          list_buffer.data.empty = true;
        }
        list_buffer.data.list_index = parameter;
        if (!powderListItemChar.writeValue(list_buffer.raw_data, LIST_ITEM_SIZE)) { Serial.println("BLE Failed to write powder list item."); }
      } else {
        Serial.println("ERROR: Failed to load powder buffer");
      }
      break;

    case BLE_COMMAND_SET_SYSTEM_STATE:
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
        Serial.print("ERROR: parameter is an unknown state. Line: ");
        Serial.print(__LINE__);
      }
      break;

    default:
      Serial.print("Unknown command: ");
      Serial.print(cmd);
      Serial.print(", parameter: ");
      Serial.println(parameter);
      break;
  }
}

/* 
 * Dynamically update runtime BLE data.
 * Updates any advertized runtime ("pushed" data) if data has changed  
 * since last update interval.
 * 
 * TODO: make most of this on demand.  Only RT scale data (weight, mode) needs
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

  float scale_weight;
  float target_weight;
  int scale_cond;
  int system_state;
  float decel_thresh;
  bool scale_mode_changed;

  if (!BLE.connected()) { return; }
  if (!g_scale.isConnected()) { return; }

  if (_last_update == 0) { _last_update = millis(); }                    //init first time
  if (_last_scale_mode == -1) { _last_scale_mode = g_scale.getMode(); }  //init first time

  if ((millis() - _last_update > BLE_DATA_UPDATE_INTERVAL) || force) {
    _last_update = millis();
    if (force) { Serial.println("BLE forced update of data"); }
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
      if (!scaleCondChar.writeValue(scale_cond)) { Serial.println("BLE Failed to write scale condition."); }
      _last_scale_cond = scale_cond;
    }
    if ((system_state != _last_system_state) || force) {
      if (!systemStateChar.writeValue(system_state)) { Serial.println("BLE Failed to write system state."); }
      _last_system_state = system_state;
    }
    if ((scale_weight != _last_scale_weight) || scale_mode_changed || force) {
      if (g_scale.getMode() == SCALE_MODE_GRAM) {
        value_buff[0] = 0;
      } else {
        value_buff[0] = 1;
      }
      memcpy(&value_buff[1], &scale_weight, 4);
      if (!scaleWeightChar.writeValue(value_buff, 5)) { Serial.println("BLE Failed to write scale weight."); }
      _last_scale_weight = scale_weight;
    }

    //TODO: The rest of this should be on demand

    if ((target_weight != _last_target_weight) || scale_mode_changed || force) {  //TODO: get target from config!
      if (g_scale.getMode() == SCALE_MODE_GRAM) {
        value_buff[0] = 0;
      } else {
        value_buff[0] = 1;
      }
      memcpy(&value_buff[1], &target_weight, 4);
      if (!scaleTargetChar.writeValue(value_buff, 5)) { Serial.println("BLE Failed to write target weight."); }
      _last_target_weight = target_weight;
    }

    if ((decel_thresh != _last_decel_thresh) || force) {  //TODO: make on demand as part of config data manager in ap
      if (!decelThreshChar.writeValue(decel_thresh)) { Serial.println("BLE Failed to write decel thresh."); }
      _last_decel_thresh = decel_thresh;
    }

    if (g_config.isBLEUpdateNeeded()) {  //TODO: make on demand, use config data manager in app
      Serial.print("TODO: make configDataChar \"update on demand\" in PTBLE.ino ");
      Serial.println(__LINE__);
      if (!g_config.updateBLE(configDataChar)) { Serial.println("BLE Failed to write config data."); }
    }

    if (force) {
      Serial.print("Forced write powder data at index ");
      Serial.println(g_presets.getPowderIndex());
      BLEWritePowderDataAt(g_presets.getPowderIndex());
    }
  }
}

/////////////////////
// Support functions for both dynamic and on demand BLE updates:
/////////////////////

void BLEWritePresetDataAt(int index) {
  static byte preset_buffer[PRESET_DATA_SIZE + sizeof(index)];  //TODO: eventually merge index into preset data structure stored in FRAM
  static PresetDataStorage preset;
  if (!g_presets.getBLEDataStruct(preset.raw_data, index)) {
    Serial.println("ERROR: failed to load requested preset data.");  //TODO: any good way to handle this?
  } else {
    if (preset._preset_data.preset_version != PRESETS_VERSION) {
      //Serial.println("Preset data at: Version mismatch: Set entire preset data structure to defaults.");
      g_presets.getDefaults(preset.raw_data, sizeof(preset.raw_data));
    }
    memcpy(preset_buffer, &index, sizeof(index));
    memcpy(preset_buffer + sizeof(index), preset.raw_data, PRESET_DATA_SIZE);

    Serial.println("Writing preset to BLE");
    Serial.print("Preset buffer length: ");
    Serial.println(sizeof(preset_buffer));
    Serial.print("Preset buffer data: ");
    printByteData(preset_buffer, sizeof(preset_buffer));
    Serial.println();

    if (!presetDataChar.writeValue(preset_buffer, sizeof(preset_buffer))) { Serial.println("BLE Failed to write preset data."); }
  }
}

void BLEWritePowderDataAt(int index) {
  static byte powder_buffer[POWDER_DATA_SIZE + sizeof(index)];  //TODO: eventually merge index into powder data structure stored in FRAM
  static PowderDataStorage powder;
  if (!g_powders.getBLEDataStruct(powder.raw_data, index)) {
    Serial.println("ERROR: failed to load requested powder data.");  //TODO: any good way to handle this?
  } else {
    if (powder._powder_data.powder_version != POWDERS_VERSION) {
      //Serial.println("Powder data at: Version mismatch: Set entire powder data structure to defaults.");
      g_powders.getDefaults(powder.raw_data, sizeof(powder.raw_data));
    }
    memcpy(powder_buffer, &index, sizeof(index));
    memcpy(powder_buffer + sizeof(index), powder.raw_data, POWDER_DATA_SIZE);
    if (!powderDataChar.writeValue(powder_buffer, sizeof(powder_buffer))) { Serial.println("BLE Failed to write powder data."); }
  }
}

void printHexData(const unsigned char data[], int len) {
  for (int i = 0; i < len; i++) {
    unsigned char b = data[i];
    Serial.print(b, HEX);
  }
  Serial.println();
}

void printStringData(const unsigned char data[], int len) {
  char c;
  for (int i = 0; i < len; i++) {
    c = toascii(data[i]);
    Serial.print(c);
  }
  Serial.println();
}

void printByteData(const unsigned char data[], int len) {
  static char buff[5];
  Serial.print("[");
  for (int i = 0; i < len - 1; i++) {
    sprintf(buff, "%d,", byte(data[i]));
    Serial.print(buff);
  }
  sprintf(buff, "%d]", byte(data[len - 1]));
  Serial.print(buff);
}