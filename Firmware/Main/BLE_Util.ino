void BLESetup() {
  Serial.println("Initialise the Bluefruit nRF52 module");
  Bluefruit.begin();
  // Set the connect/disconnect callback handlers
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);


  // Configure and Start the Device Information Service
  Serial.println("Configuring the Device Information Service");
  bledis.setManufacturer("KU02");
  bledis.setModel("NodSense1");
  bledis.begin();
  // Setup the service using
  // BLEService and BLECharacteristic classes
  Serial.println("Configuring the Services");
  setupEOGService();
  setupFileService();
  // Setup the advertising packet(s)
  Serial.println("Setting up the advertising payload(s)");
  startAdv();

  Bluefruit.Advertising.stop();



  Serial.println("Advertising");
}

void startAdv(void) {
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Include EOG Service UUID
  Bluefruit.Advertising.addService(eog_service);
  Bluefruit.Advertising.addService(file_service);

  // Include Name
  Bluefruit.Advertising.addName();

  /* Start Advertising
     - Enable auto advertising if disconnected
     - Interval:  fast mode = 20 ms, slow mode = 152.5 ms
     - Timeout for fast mode is 30 seconds
     - Start(timeout) with timeout = 0 will advertise forever (until connected)

     For recommended advertising interval
     https://developer.apple.com/library/content/qa/qa1931/_index.html
  */
  Bluefruit.Advertising.restartOnDisconnect(false);
  Bluefruit.Advertising.setInterval(32, 244);  // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);    // number of seconds in fast mode
  Bluefruit.Advertising.start(0);              // 0 = Don't stop advertising after n seconds
}

void setupEOGService(void) {
  eog_service.begin();


  // Note: You must call .begin() on the BLEService before calling .begin() on
  // any characteristic(s) within that service definition.. Calling .begin() on
  // a BLECharacteristic will cause it to be added to the last BLEService that
  // was 'begin()'ed!

  // Configure the Heart Rate Measurement characteristic
  // See: https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.characteristic.heart_rate_measurement.xml
  // Properties = Notify
  // Min Len    = 1
  // Max Len    = 8
  //    B0      = UINT8  - Flag (MANDATORY)
  //      b5:7  = Reserved
  //      b4    = RR-Internal (0 = Not present, 1 = Present)
  //      b3    = Energy expended status (0 = Not present, 1 = Present)
  //      b1:2  = Sensor contact status (0+1 = Not supported, 2 = Supported but contact not detected, 3 = Supported and detected)
  //      b0    = Value format (0 = UINT8, 1 = UINT16)
  //    B1      = UINT8  - 8-bit heart rate measurement value in BPM
  //    B2:3    = UINT16 - 16-bit heart rate measurement value in BPM
  //    B4:5    = UINT16 - Energy expended in joules
  //    B6:7    = UINT16 - RR Internal (1/1024 second resolution)
  eog_characertistic.setProperties(CHR_PROPS_NOTIFY);
  eog_characertistic.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  eog_characertistic.setMaxLen(20);                        // new
  eog_characertistic.setCccdWriteCallback(cccd_callback);  // Optionally capture CCCD updates
  eog_characertistic.begin();
  uint8_t eogbledata[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  eog_characertistic.write(eogbledata, 20);
}

void setupFileService(void) {
  file_service.begin();

  files_charactertistic.setProperties(CHR_PROPS_READ);
  files_charactertistic.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
  files_charactertistic.setMaxLen(20);  // new
  files_charactertistic.begin();
  uint8_t filebledata[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
  files_charactertistic.write(filebledata, 20);

 


  fileSelect_charactertistic.setProperties(CHR_PROPS_READ | CHR_PROPS_WRITE);
  fileSelect_charactertistic.setPermission(SECMODE_OPEN, SECMODE_OPEN);
  fileSelect_charactertistic.setWriteCallback(write_callback);

  fileSelect_charactertistic.begin();


}

//void setupFileSelectService(void) {
//  fileSelect_service.begin();

//  fileSelect_charactertistic.setProperties(CHR_PROPS_WRITE);
//  fileSelect_charactertistic.setPermission(SECMODE_OPEN, SECMODE_NO_ACCESS);
//  fileSelect_charactertistic.setMaxLen(1);  // new
//  fileSelect_charactertistic.begin();
//  uint8_t filebledata[20] = {0};
//  fileSelect_charactertistic.write(filebledata, 1);
  
//}


void updateBLEData() {
  if (BLE_buffer_index) {
    BLE_out_data[0] = chunk_from_SD.run;
    BLE_out_data[1] = chunk_from_SD.number;
  }
  for (int i = 0; i < 20; i++) {
    BLE_out_data[i] = chunk_from_SD.run;
    BLE_buffer_index = BLE_buffer_index + 1;
  }
}

void bufferToBLE(byte* buffer, byte* BLE_out) {

  for (int j = 0; j < 20; j++) {
    BLE_out[j + BLE_buffer_index] = buffer[BLE_buffer_index + j];
    //Serial.println(BLE_buffer_index + j);
  }
  BLE_buffer_index += 20;
}

void updateFileOptions() {
DetermineRunNumber();
uint8_t filebledata[20] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
int offset = 0;
  for (int j = 0; j <20; j++) {
    for (int i = 0; i < 8; i++) {
      if (systemSettings.currentRunNumbers[i + offset] == 1){
        filebledata[j] = filebledata[j] + pow(2, (7-i));
      }
      
    }
    offset = offset + 8;
  }
  
  files_charactertistic.write(filebledata, 20);
}