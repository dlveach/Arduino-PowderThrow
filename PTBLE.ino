
/*
970a6f6e-e01b-11ec-9d64-0242ac120002 - Service
970a71b2-e01b-11ec-9d64-0242ac120002 - Button
970a745a-e01b-11ec-9d64-0242ac120002 - String
970a75a4-e01b-11ec-9d64-0242ac120002 - Scale Data (currently string, become structure?)
970a769e-e01b-11ec-9d64-0242ac120002 - Scale Condition
970a77a2-e01b-11ec-9d64-0242ac120002 - System State
970a7892-e01b-11ec-9d64-0242ac120002 - Decel Thresh
970a798c-e01b-11ec-9d64-0242ac120002 - Config data structure
970a7aae-e01b-11ec-9d64-0242ac120002
970a7c20-e01b-11ec-9d64-0242ac120002
*/

#define BLE_SERVICE_GUID "970a6f6e-e01b-11ec-9d64-0242ac120002"
#define BLE_BTN_CHAR_GUID "970a71b2-e01b-11ec-9d64-0242ac120002"
#define BLE_STR_CHAR_GUID "970a745a-e01b-11ec-9d64-0242ac120002"
#define BLE_SCALE_DATA_GUID "970a75a4-e01b-11ec-9d64-0242ac120002"
#define BLE_SCALE_COND_GUID "970a769e-e01b-11ec-9d64-0242ac120002"
#define BLE_SYSTEM_STATE_GUID "970a77a2-e01b-11ec-9d64-0242ac120002"
#define BLE_DECEL_THRESH_GUID "970a7892-e01b-11ec-9d64-0242ac120002"
#define BLE_CONFIG_DATA_GUID "970a798c-e01b-11ec-9d64-0242ac120002"

#define BLE_DATA_UPDATE_INTERVAL 100

BLEService ptBLE_Service(BLE_SERVICE_GUID); 
BLEByteCharacteristic btnChar(BLE_BTN_CHAR_GUID, BLERead | BLEWrite);
BLEDescriptor btnDescriptor("2901", "LED Button");
BLEStringCharacteristic strChar(BLE_STR_CHAR_GUID, BLERead | BLENotify, 31);
BLEDescriptor strDescriptor("2901", "Scale string value");
BLEFloatCharacteristic scaleDataChar(BLE_SCALE_DATA_GUID, BLERead | BLENotify);
BLEDescriptor scaleDataDescriptor("2901", "Scale float value");
BLEStringCharacteristic scaleCondChar(BLE_SCALE_COND_GUID, BLERead | BLENotify, 31);
BLEDescriptor scaleCondDescriptor("2901", "Scale Condition");
BLEStringCharacteristic systemStateChar(BLE_SYSTEM_STATE_GUID, BLERead | BLENotify, 31);
BLEDescriptor systemStateDescriptor("2901", "System State");
BLEStringCharacteristic decelThreshChar(BLE_DECEL_THRESH_GUID, BLERead | BLEWrite | BLENotify, 31);
BLEDescriptor decelThreshDescriptor("2901", "Decel Threshold");
BLECharacteristic configDataChar(BLE_CONFIG_DATA_GUID, BLERead | BLEWrite | BLENotify, CONFIG_DATA_SIZE, true);
BLEDescriptor configDataDescriptor("2901", "Config data structure");

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
  btnChar.addDescriptor(btnDescriptor);
  strChar.addDescriptor(strDescriptor);
  scaleDataChar.addDescriptor(scaleDataDescriptor);
  scaleCondChar.addDescriptor(scaleCondDescriptor);
  systemStateChar.addDescriptor(systemStateDescriptor);
  decelThreshChar.addDescriptor(decelThreshDescriptor);
  configDataChar.addDescriptor(configDataDescriptor);
  
  // add the characteristic to the service
  ptBLE_Service.addCharacteristic(btnChar);
  ptBLE_Service.addCharacteristic(strChar);
  ptBLE_Service.addCharacteristic(scaleDataChar);
  ptBLE_Service.addCharacteristic(scaleCondChar);
  ptBLE_Service.addCharacteristic(systemStateChar);
  ptBLE_Service.addCharacteristic(decelThreshChar);
  ptBLE_Service.addCharacteristic(configDataChar);
  
  // add service
  BLE.addService(ptBLE_Service);

  // assign event handlers for connected, disconnected to peripheral
  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  // assign event handlers for characteristics
  btnChar.setEventHandler(BLEWritten, btnCharWritten);
  decelThreshChar.setEventHandler(BLEWritten, decelThreshCharWritten);

  // set the initial value for the characteristics:
  btnChar.writeValue(0);
  char* stringCharValue = new char[32];
  stringCharValue = "string";
  strChar.writeValue(stringCharValue);  
  scaleDataChar.writeValue(32.5);

  // start advertising
  BLE.advertise();
  Serial.println("Bluetooth® device active, waiting for connections..");
}
void blePeripheralConnectHandler(BLEDevice central) {
  // central connected event handler
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
  g_mcp.digitalWrite(MCP_LED_PUR_PIN, HIGH);
  delay(1000);
  Serial.println("Update data on BLE connection");
  updateBLEData(true);
}

void blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
  g_mcp.digitalWrite(MCP_LED_PUR_PIN, LOW);
}

void btnCharWritten(BLEDevice central, BLECharacteristic characteristic) {
  // central wrote new value to characteristic, update LED
  Serial.print("btnChar event, written: Value = ");
  printHexData(characteristic.value(), characteristic.valueLength());
  int val = characteristic.value()[0];
  switch (val) 
  {
    case 1:
      Serial.println("Toggle Red LED");
      g_LED_Red.toggle();
      break;
    case 2:      
      Serial.println("Toggle Green LED");
      g_LED_Grn.toggle();
      break;
    case 3:
      Serial.println("Toggle Blue LED");
      g_LED_Blu.toggle();
      break;
    case 0:
      Serial.println("Turn all LEDs off");
      g_LED_Red.setOff();
      g_LED_Grn.setOff();
      g_LED_Blu.setOff();
      break;
    default:
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

void updateBLEData(bool force) {
  static long _last_update = 0;
  static char* buff = new char[32];
  static float _last_val = 0.0;
  static int _last_scale_cond = -1;
  static int _last_system_state = -1;
  static float _last_decel_thresh = -1;

  if (!BLE.connected()) { return; }
  if (!g_scale.isConnected()) { return; }

  if (_last_update == 0) {_last_update = millis(); } //init first time
  if ((millis() - _last_update > BLE_DATA_UPDATE_INTERVAL) || force) {
    _last_update = millis();
    if (force) { Serial.println("forced update of BLE data"); }
    float val = g_scale.getWeight();
    int scale_cond = g_scale.getCondition();
    int system_state = g_state.getState(); 
    float decel_thresh = g_config.getDecelThreshold(); 
    //only update things if changed or forced
    if ((val != _last_val) || force) {
      if (!scaleDataChar.writeValue(val)) {Serial.println("Failed to write scale float value.");}
      if (g_scale.getMode() == SCALE_MODE_GRAM) {
        sprintf(buff, "%8.3f", val);
      } else {
        sprintf(buff, "%8.2f", val);
      }
      if (!strChar.writeValue(buff)) {Serial.println("Failed to write scale str value.");}
      _last_val = val;
    }
    if ((scale_cond != _last_scale_cond) || force) {
      sprintf(buff, "%.30s", g_scale.getConditionLongName());
      if (!scaleCondChar.writeValue(buff)) {Serial.println("Failed to write scale condition.");}
      _last_scale_cond = scale_cond;
    }
    if ((system_state != _last_system_state) || force) {
      sprintf(buff, "%.30s", g_state.getStateName());
      if (!systemStateChar.writeValue(buff)) {Serial.println("Failed to write system state.");}
      _last_system_state = system_state;
    }
    if ((decel_thresh != _last_decel_thresh) || force) {
      sprintf(buff, "%3.1f", g_config.getDecelThreshold());
      if (!decelThreshChar.writeValue(buff)) {Serial.println("Failed to write decel thresh.");}
      _last_decel_thresh = decel_thresh;
    }    
    if (g_config.isBLEUpdateNeeded()) {
      Serial.print("TODO: update configDataChar in PTBLE.ino ");
      Serial.println(__LINE__);
      if (!g_config.updateBLE(configDataChar)) {Serial.println("Failed to write config data.");}
    }
  }
}
