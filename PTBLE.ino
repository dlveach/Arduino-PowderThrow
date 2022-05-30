
/*
970a6f6e-e01b-11ec-9d64-0242ac120002 - Service
970a71b2-e01b-11ec-9d64-0242ac120002 - Button
970a745a-e01b-11ec-9d64-0242ac120002 - String
970a75a4-e01b-11ec-9d64-0242ac120002 - Scale Data (currently string, become structure?)
970a769e-e01b-11ec-9d64-0242ac120002 - Scale Condition
970a77a2-e01b-11ec-9d64-0242ac120002
970a7892-e01b-11ec-9d64-0242ac120002
970a798c-e01b-11ec-9d64-0242ac120002
970a7aae-e01b-11ec-9d64-0242ac120002
970a7c20-e01b-11ec-9d64-0242ac120002
*/

#define BLE_SERVICE_GUID "970a6f6e-e01b-11ec-9d64-0242ac120002"
#define BLE_BTN_CHAR_GUID "970a71b2-e01b-11ec-9d64-0242ac120002"
#define BLE_STR_CHAR_GUID "970a745a-e01b-11ec-9d64-0242ac120002"
#define BLE_SCALE_DATA_GUID "970a75a4-e01b-11ec-9d64-0242ac120002"
#define BLE_SCALE_COND_GUID "970a769e-e01b-11ec-9d64-0242ac120002"

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
  
  // add the characteristic to the service
  ptBLE_Service.addCharacteristic(btnChar);
  ptBLE_Service.addCharacteristic(strChar);
  ptBLE_Service.addCharacteristic(scaleDataChar);
  ptBLE_Service.addCharacteristic(scaleCondChar);
  
  // add service
  BLE.addService(ptBLE_Service);

  // assign event handlers for connected, disconnected to peripheral
  BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);
  BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);

  // assign event handlers for characteristics
  btnChar.setEventHandler(BLEWritten, btnCharWritten);
  //strChar.setEventHandler(BLEWritten, strCharWritten);

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
/*. TODO: Will eventually need some inbound string data from IOS device.  Not in this charactaristic, outbound only.
void strCharWritten(BLEDevice central, BLECharacteristic characteristic) {
  Serial.print("strChar event, written: Hex Value = ");
  printHexData(characteristic.value(), characteristic.valueLength());
  Serial.print("strChar, String Value = ");
  printStringData(characteristic.value(), characteristic.valueLength());
} */

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

void updateBLEData() {
  static long _last_update = 0;
  static char* buff = new char[32];
  static float _last_val = 0.0;
  static int _last_scale_cond = -1;
  if (_last_update == 0) {_last_update = millis(); } //init first time
  if (millis() - _last_update > BLE_DATA_UPDATE_INTERVAL) {
    float val = g_scale.getWeight();
    int scale_cond = g_scale.getCondition(); 
    //only update if changed
    if (val != _last_val) {
      if (!scaleDataChar.writeValue(val)) {Serial.println("Failed to write float value.");}
      sprintf(buff, "%8.2f", val);
      if (!strChar.writeValue(buff)) {Serial.println("Failed to write str value.");}
      _last_val = val;
    }
    if (scale_cond != _last_scale_cond) {
      sprintf(buff, "%.30s", g_scale.getConditionLongName());
      if (!scaleCondChar.writeValue(buff)) {Serial.println("Failed to write scale condition.");}
      _last_scale_cond = scale_cond;
    }
  }
}
