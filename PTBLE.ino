
/*
970a6f6e-e01b-11ec-9d64-0242ac120002 - Service
970a71b2-e01b-11ec-9d64-0242ac120002 - Command Button
970a745a-e01b-11ec-9d64-0242ac120002 - Scale Weight (String)
970a75a4-e01b-11ec-9d64-0242ac120002 - Scale Target Weight (String)
970a769e-e01b-11ec-9d64-0242ac120002 - Scale Condition
970a77a2-e01b-11ec-9d64-0242ac120002 - System State
970a7892-e01b-11ec-9d64-0242ac120002 - Decel Thresh
970a798c-e01b-11ec-9d64-0242ac120002 - Config data structure
970a7aae-e01b-11ec-9d64-0242ac120002 - Preset name
970a7c20-e01b-11ec-9d64-0242ac120002 - Powder name
*/

//TODO: add a scale mode charactaristic,
//TODO: add a persistent data model in phone app
//TODO: handle display switches on mode in app
//TODO: add preset & powder select/edit in app
//TODO: add config settings edit in app
//TODO: lots of cleanup, debug removal etc.

#define BLE_DATA_UPDATE_INTERVAL 100

#define BLE_COMMAND_REQ_CONFIG_DATA 0x01
#define BLE_COMMAND_REQ_PRESET_LIST_DATA 0x02
#define BLE_COMMAND_REQ_POWDER_LIST_DATA 0x03

// BLE Services and Charactaristics

#define BLE_SERVICE_GUID "970a6f6e-e01b-11ec-9d64-0242ac120002"
#define BLE_COMMAND_CHAR_GUID "970a71b2-e01b-11ec-9d64-0242ac120002"
#define BLE_SCALE_STRING_GUID "970a745a-e01b-11ec-9d64-0242ac120002"
#define BLE_SCALE_TARGET_GUID "970a75a4-e01b-11ec-9d64-0242ac120002"
#define BLE_SCALE_COND_GUID "970a769e-e01b-11ec-9d64-0242ac120002"
#define BLE_SYSTEM_STATE_GUID "970a77a2-e01b-11ec-9d64-0242ac120002"
#define BLE_DECEL_THRESH_GUID "970a7892-e01b-11ec-9d64-0242ac120002"
#define BLE_CONFIG_DATA_GUID "970a798c-e01b-11ec-9d64-0242ac120002"
#define BLE_PRESET_NAME_GUID "970a7aae-e01b-11ec-9d64-0242ac120002"
#define BLE_POWDER_NAME_GUID "970a7c20-e01b-11ec-9d64-0242ac120002"

BLEService ptBLE_Service(BLE_SERVICE_GUID); 
BLEByteCharacteristic commandChar(BLE_COMMAND_CHAR_GUID, BLERead | BLEWrite);
BLEDescriptor commandDescriptor("2901", "Command button");
BLEStringCharacteristic scaleStringChar(BLE_SCALE_STRING_GUID, BLERead | BLENotify, 31);
BLEDescriptor scaleDataStringDescriptor("2901", "Scale weight, string value");
BLEStringCharacteristic scaleTargetChar(BLE_SCALE_TARGET_GUID, BLERead | BLENotify, 31);
BLEDescriptor scaleTargetDescriptor("2901", "Scale Target weight, string value");
BLEStringCharacteristic scaleCondChar(BLE_SCALE_COND_GUID, BLERead | BLENotify, 31);
BLEDescriptor scaleCondDescriptor("2901", "Scale Condition");
BLEStringCharacteristic systemStateChar(BLE_SYSTEM_STATE_GUID, BLERead | BLENotify, 31);
BLEDescriptor systemStateDescriptor("2901", "System State");
BLEStringCharacteristic decelThreshChar(BLE_DECEL_THRESH_GUID, BLERead | BLEWrite | BLENotify, 31);
BLEDescriptor decelThreshDescriptor("2901", "Decel Threshold");
BLECharacteristic configDataChar(BLE_CONFIG_DATA_GUID, BLERead | BLEWrite | BLENotify, CONFIG_DATA_SIZE, true);
BLEDescriptor configDataDescriptor("2901", "Config data structure");
BLEStringCharacteristic presetNameChar(BLE_PRESET_NAME_GUID, BLERead | BLENotify, NAME_LEN+1);
BLEDescriptor presetNameDescriptor("2901", "Preset Name");
BLEStringCharacteristic powderNameChar(BLE_POWDER_NAME_GUID, BLERead | BLENotify, NAME_LEN+1);
BLEDescriptor powderNameDescriptor("2901", "Powder Name");

/*
 * Setup Bluetooth Low Energy (BLE) 
 */
bool initBLE()
{
  // begin initialization
  if (!BLE.begin()) {
    Serial.println("starting Bluetooth® Low Energy failed!");
    while (1);
  }
  Serial.println("BLE started");

  // set advertised local name and service UUID:
  BLE.setDeviceName("PowderThrow");
  BLE.setLocalName("Nano 33 BLE");
  BLE.setAdvertisedService(ptBLE_Service);

  // add descriptors to the charactaristics
  commandChar.addDescriptor(commandDescriptor);
  scaleStringChar.addDescriptor(scaleDataStringDescriptor);
  scaleTargetChar.addDescriptor(scaleTargetDescriptor);
  scaleCondChar.addDescriptor(scaleCondDescriptor);
  systemStateChar.addDescriptor(systemStateDescriptor);
  decelThreshChar.addDescriptor(decelThreshDescriptor);
  configDataChar.addDescriptor(configDataDescriptor);
  presetNameChar.addDescriptor(presetNameDescriptor);
  powderNameChar.addDescriptor(powderNameDescriptor);
  
  // add the characteristic to the service
  ptBLE_Service.addCharacteristic(commandChar);
  ptBLE_Service.addCharacteristic(scaleStringChar);
  ptBLE_Service.addCharacteristic(scaleTargetChar);
  ptBLE_Service.addCharacteristic(scaleCondChar);
  ptBLE_Service.addCharacteristic(systemStateChar);
  ptBLE_Service.addCharacteristic(decelThreshChar);
  ptBLE_Service.addCharacteristic(configDataChar);
  ptBLE_Service.addCharacteristic(presetNameChar);
  ptBLE_Service.addCharacteristic(powderNameChar);
  
  // add service
  BLE.addService(ptBLE_Service);

  // assign event handlers for connected, disconnected to peripheral
  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  // assign event handlers for characteristics
  commandChar.setEventHandler(BLEWritten, commandCharWritten);
  decelThreshChar.setEventHandler(BLEWritten, decelThreshCharWritten);

  // set the initial value for the characteristics:
  // TODO: trigger the update function instead?
  Serial.println("Update data on BLE connection");
  //updateBLEData(true);

  // start advertising
  BLE.advertise();
  Serial.println("Bluetooth® device active, waiting for connections..");
}

/*
 * Handle a connection from BLE central
 */
void blePeripheralConnectHandler(BLEDevice central) {
  // central connected event handler
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
  g_mcp.digitalWrite(MCP_LED_PUR_PIN, HIGH);
  delay(500);
  updateBLEData(true);
}

/*
 * Handle a disconnect from the BLE central
 */
void blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
  g_mcp.digitalWrite(MCP_LED_PUR_PIN, LOW);
}

/*
 * Process a recieved command from the BLE Central.
 * Call respective handler function for the command.
 */
void commandCharWritten(BLEDevice central, BLECharacteristic characteristic) {
  // central wrote new value to characteristic, update LED
  Serial.print("commandChar event, written: Value = ");
  printHexData(characteristic.value(), characteristic.valueLength());
  Serial.println();
  int val = characteristic.value()[0];
  switch (val) 
  {
    case BLE_COMMAND_REQ_CONFIG_DATA:
      Serial.println("TODO: BLE transmit config data");
      break;
    case BLE_COMMAND_REQ_PRESET_LIST_DATA:      
      Serial.println("TODO: BLE transmit preset list");
      break;
    case BLE_COMMAND_REQ_POWDER_LIST_DATA:
      Serial.println("TODO: BLE transmit powder list");
      break;
    default:
      Serial.println("WARNING: unknown BLE command recieved");
      //ignore everything else
      break;
    }
}

/*
 * Handler for decel threshold update from device slider
 * TODO: Write FRAM every update?  Use user action with button on phone?
 */
void decelThreshCharWritten(BLEDevice central, BLECharacteristic characteristic) {
  char* buff = new char[4];
  for (int i=0; i<3; i++) {
    buff[i]=toascii(characteristic.value()[i]);
  }
  buff[3] = '\0';
  float val = atof(buff);
  g_config.setDecelThreshold(val);
  g_config.saveConfig();
  if (g_state.getState() == g_state.pt_cfg) {
    g_display_changed = true;
    displayUpdate();
  }
}


void printHexData(const unsigned char data[], int len) {
  for (int i=0; i<len; i++) {
    unsigned char b = data[i];
    Serial.print(b, HEX);
  }
  Serial.println();
}

void printStringData(const unsigned char data[], int len){
  char c;
  for (int i=0; i<len; i++) {
    c = toascii(data[i]);
    Serial.print(c);
  }
  Serial.println();
}

/* 
 * Dynamically update runtime BLE data.
 * Updates any advertized runtime ("pushed" data) if data has changed  
 * since last update interval.
 */
void updateBLEData(bool force) {
  static long _last_update = 0;
  static char* buff = new char[32];
  static float _last_scale_weight = 0.0;
  static int _last_scale_cond = -1;
  static float _last_target_weight = 0.0;
  static int _last_system_state = -1;
  static float _last_decel_thresh = -1;
  static int _last_preset_number = -1;
  static int _last_scale_mode = -1;

  float scale_weight;
  int scale_cond;
  int system_state; 
  float decel_thresh;
  bool scale_mode_changed; 

  if (!BLE.connected()) { return; }
  if (!g_scale.isConnected()) { return; }

  if (_last_update == 0) {_last_update = millis(); } //init first time
  if (_last_scale_mode == -1) { _last_scale_mode = g_scale.getMode(); } //init first time

  if ((millis() - _last_update > BLE_DATA_UPDATE_INTERVAL) || force) {
    _last_update = millis();
    if (force) { Serial.println("BLE forced update of data"); }
    scale_weight = g_scale.getWeight();
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
    if ((scale_weight != _last_scale_weight) || scale_mode_changed || force) {
      if (g_scale.getMode() == SCALE_MODE_GRAM) {
        sprintf(buff, "%8.3f g", scale_weight);
      } else {
        sprintf(buff, "%8.2f gn", scale_weight);
      }
      if (!scaleStringChar.writeValue(buff)) {Serial.println("BLE Failed to write scale str value.");}
      _last_scale_weight = scale_weight;
    }
    if ((g_config.getTargetWeight() != _last_target_weight) || scale_mode_changed || force) {
      if (g_scale.getMode() == SCALE_MODE_GRAM) {
        sprintf(buff, "%6.3f g", (g_config.getTargetWeight() * GM_TO_GN_FACTOR));
      } else {
        sprintf(buff, "%6.2f gn", g_config.getTargetWeight());
      }
      if (!scaleTargetChar.writeValue(buff)) {Serial.println("BLE Failed to write scale str value.");}
      _last_target_weight= g_config.getTargetWeight();
    }
    if ((scale_cond != _last_scale_cond) || force) {
      sprintf(buff, "%.30s", g_scale.getConditionLongName());
      if (!scaleCondChar.writeValue(buff)) {Serial.println("BLE Failed to write scale condition.");}
      _last_scale_cond = scale_cond;
    }
    if ((system_state != _last_system_state) || force) {
      sprintf(buff, "%.30s", g_state.getStateName());
      if (!systemStateChar.writeValue(buff)) {Serial.println("BLE Failed to write system state.");}
      _last_system_state = system_state;
    }
    if ((decel_thresh != _last_decel_thresh) || force) {
      sprintf(buff, "%3.1f", g_config.getDecelThreshold());
      if (!decelThreshChar.writeValue(buff)) {Serial.println("BLE Failed to write decel thresh.");}
      _last_decel_thresh = decel_thresh;
    }    
    if (g_config.isBLEUpdateNeeded()) {
      Serial.print("TODO: make configDataChar \"update on demand\" in PTBLE.ino ");
      Serial.println(__LINE__);
      if (!g_config.updateBLE(configDataChar)) {Serial.println("BLE Failed to write config data.");}
    }
    if ((g_config.getPreset() != _last_preset_number) || force)
    {
      if (!presetNameChar.writeValue(g_config.getPresetName())) {Serial.println("BLE Failed to write preset name.");}
      if (!powderNameChar.writeValue(g_config.getPowderName())) {Serial.println("BLE Failed to write powder name.");}
    }
  }
}
