
#include <bluefruit.h>
#include <LiquidCrystal_I2C.h>
#include <SPI.h>
#include <SD.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include <Adafruit_TinyUSB.h> // for Serial
#include <Arduino.h>
#include <Wire.h>
#include <MPU6050.h>
#include "NRF52TimerInterrupt.h"
#include "NRF52_ISR_Timer.h"

//using namespace Adafruit_LittleFS_Namespace;
#define FILENAME    "/adafruit.txt"
#define CONTENTS    "Wow even the flash memory works"
#define TIMER_INTERRUPT_DEBUG 0
#define HW_TIMER_INTERVAL_MS      1 // Hardware timer period in ms
#define TIMER_INTERVAL_5MS             5L // EOG sampling period in ms (interrupt timer period)
#define DS1631_ADDRESS  0x48 // Address of the temperature sensor
#define IMU_ADDRESS  0x68 // Address of the IMU sensor
LiquidCrystal_I2C lcd(0x27, 16, 2); // set the LCD address to 0x3F for a 16 chars and 2 line display

const int chipSelect = 2;
const uint8_t ITSY_TESTPULSE_PIN = 7;

BLEService        eog_service = BLEService(0x1108);
BLECharacteristic eog_characertistic = BLECharacteristic(0x2B3D);
BLEDis bledis;    // DIS (Device Information Service) helper class instance
BLEBas blebas;    // BAS (Battery Service) helper class instance

NRF52Timer ITimer(NRF_TIMER_1);
NRF52_ISR_Timer ISR_Timer;

Adafruit_LittleFS_Namespace::File file(InternalFS);
// Define global variables 
int adcin1 = A0; // Pin connected to analog voltage input (EOG) 1 
int adcin2 = A1; // Pin connected to analog voltage input (EOG) 1 
float adcval1 = 0; // For storing the voltage on pin A0
float adcval2 = 0; // For storing the voltage on pin A1
float V_A0 = 0; // Digital voltage reading from pin A0
float V_A1 = 0; // Digital voltage reading from pin A1
float mv_per_lsb = 3300.0F / 4096.0F; // Number of millivolts per less significant bit for 12-bit ADC with a 3.3 V input range
int sample_count_EOG = 0; // Count for tracking when to take accelerometer/gyroscope samples based on the number of EOG samples taken
int sample_count_IMU = 0; // Count for tracking when to take temperature samples based on the number of IMU samples taken
int total_num_samp_EOG = 0; // Count of the total number of EOG samples taken
int total_num_samp_IMU = 0; // Count of the total number of accelerometer and gyroscope samples taken
int total_num_samp_TEMP = 0; // Count of the total number of temperature samples taken
float c_temp = 0; // For storing readings in degrees C from the DS1631+ temperature sensor
float c_temp_MPU = 0; // For storing the reading in degrees C from the temperature sensor in the MPU-6050 IMU
int16_t raw_t = 0; // For storing the raw reading from the DS1631+ temperature sensor
int16_t raw_t_MPU = 0; // For storing the raw reading from the temperature sensor in the MPU-6050 IMU
float AccX, AccY, AccZ; // For storing raw readings from all three axes of the accelerometer
float GyroX, GyroY, GyroZ; // For storing raw readings from all three axes of the gyroscope
char buf[64];
MPU6050 mpu;
uint8_t  bps = 0;
bool SD_good;
int menuChoice;

void LCDsetup() {
  lcd.init();
  lcd.clear();
  lcd.backlight();      // Make sure backlight is on
  //lcd.noBacklight();

  // Print a message on both lines of the LCD.
  lcd.setCursor(2, 0);  //Set cursor to character 2 on line 0
  lcd.print("Hello world!");

  lcd.setCursor(3, 1);  //Move cursor to character 2 on line 1
  lcd.print("DEMO TIME!");
}

bool setupSD() {
  bool card_good = true;
  Serial.print("Initializing SD card...");
  SPI.setPins(23, 25, 24);
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    card_good = false;
  }
  else {
    Serial.println("card initialized.");
  }
  return card_good;
}

void setupMPU(){
  // Set up the MPU-6050 IMU sensor

  while (!mpu.begin(MPU6050_SCALE_250DPS, MPU6050_RANGE_2G))
  {
    Serial.println("Could not find a valid MPU6050 sensor, check wiring!");
    delay(500);
  }
  checkSettings();
}

void setupDS1631(){
  // Set up the DS1631+ temperature sensor
  Wire.begin(); // Initialize the I2C communication
  Wire.beginTransmission(DS1631_ADDRESS); // Begin transmitting data to the DS1631+ sensor
  Wire.write(0xAC); // Talk to register 172 (0xAC) - configuration
  Wire.write(0x02); // Set the resolution to 9-bit
  Wire.beginTransmission(DS1631_ADDRESS); // Begin data transmission to the DS1631+ sensor
  Wire.write(0x51); // Talk to register 81 (0x51) to start conversions
  Wire.endTransmission(); // End the transmission
}

// Function for checking and printing the settings of the MPU-6050 sensor. From the MPU6050 library
void checkSettings()
{
  Serial.println();

  Serial.print(" * Sleep Mode:            ");
  Serial.println(mpu.getSleepEnabled() ? "Enabled" : "Disabled");

  Serial.print(" * Clock Source:          ");
  switch (mpu.getClockSource())
  {
    case MPU6050_CLOCK_KEEP_RESET:     Serial.println("Stops the clock and keeps the timing generator in reset"); break;
    case MPU6050_CLOCK_EXTERNAL_19MHZ: Serial.println("PLL with external 19.2MHz reference"); break;
    case MPU6050_CLOCK_EXTERNAL_32KHZ: Serial.println("PLL with external 32.768kHz reference"); break;
    case MPU6050_CLOCK_PLL_ZGYRO:      Serial.println("PLL with Z axis gyroscope reference"); break;
    case MPU6050_CLOCK_PLL_YGYRO:      Serial.println("PLL with Y axis gyroscope reference"); break;
    case MPU6050_CLOCK_PLL_XGYRO:      Serial.println("PLL with X axis gyroscope reference"); break;
    case MPU6050_CLOCK_INTERNAL_8MHZ:  Serial.println("Internal 8MHz oscillator"); break;
  }

  Serial.print(" * Accelerometer:         ");
  switch (mpu.getRange())
  {
    case MPU6050_RANGE_16G:            Serial.println("+/- 16 g"); break;
    case MPU6050_RANGE_8G:             Serial.println("+/- 8 g"); break;
    case MPU6050_RANGE_4G:             Serial.println("+/- 4 g"); break;
    case MPU6050_RANGE_2G:             Serial.println("+/- 2 g"); break;
  }

  Serial.print(" * Accelerometer offsets: ");
  Serial.print(mpu.getAccelOffsetX());
  Serial.print(" / ");
  Serial.print(mpu.getAccelOffsetY());
  Serial.print(" / ");
  Serial.println(mpu.getAccelOffsetZ());

  Serial.println();
}

void TimerHandler()
{
  ISR_Timer.run();
}

void setup()
{
  Serial.begin(115200);
  delay(100);
  InternalFS.begin();
  pinMode(7, OUTPUT);
  pinMode(13, OUTPUT);
  pinMode(12, INPUT_PULLDOWN);
  LCDsetup();
  SD_good = setupSD();

  // Set up the analog to digital converter
  analogReference(AR_VDD4); // Set the analog reference voltage to VDD (3.3 V)
  analogReadResolution(12); // Set the resolution to 12-bit
  setupDS1631();
  setupMPU();
  
  // Set up the timer-driven interrupt
  if (ITimer.attachInterruptInterval(HW_TIMER_INTERVAL_MS * 1000, TimerHandler))
  {
    Serial.print("Starting Timer OK\n");
  }
  else
    Serial.println("Can't set ITimer. Select another freq. or timer");

  ISR_Timer.setInterval(TIMER_INTERVAL_5MS,  SampleAll);
  // Initialise the Bluefruit module
  Serial.println("Initialise the Bluefruit nRF52 module");
  Bluefruit.begin();

  // Set the connect/disconnect callback handlers
  Bluefruit.Periph.setConnectCallback(connect_callback);
  Bluefruit.Periph.setDisconnectCallback(disconnect_callback);

  // Configure and Start the Device Information Service
  Serial.println("Configuring the Device Information Service");
  bledis.setManufacturer("Adafruit Industries");
  bledis.setModel("ItsyBitsy"); //Bluefruit Feather52
  bledis.begin();

  // Start the BLE Battery Service and set it to 100%
  Serial.println("Configuring the Battery Service");
  blebas.begin();
  blebas.write(100);

  // Setup the Heart Rate Monitor service using
  // BLEService and BLECharacteristic classes
  Serial.println("Configuring the Service");
  setupEOGService();
  // Setup the advertising packet(s)
  Serial.println("Setting up the advertising payload(s)");
  startAdv();
  Serial.println("\nAdvertising");
  delay(1000);
}

void startAdv(void)
{
  // Advertising packet
  Bluefruit.Advertising.addFlags(BLE_GAP_ADV_FLAGS_LE_ONLY_GENERAL_DISC_MODE);
  Bluefruit.Advertising.addTxPower();

  // Include EOG Service UUID
  Bluefruit.Advertising.addService(eog_service);

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
  Bluefruit.Advertising.restartOnDisconnect(true);
  Bluefruit.Advertising.setInterval(32, 244);    // in unit of 0.625 ms
  Bluefruit.Advertising.setFastTimeout(30);      // number of seconds in fast mode
  Bluefruit.Advertising.start(0);                // 0 = Don't stop advertising after n seconds
}

void setupEOGService(void)
{
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
  eog_characertistic.setFixedLen(2);
  eog_characertistic.setCccdWriteCallback(cccd_callback);  // Optionally capture CCCD updates
  eog_characertistic.begin();
  uint8_t eogbledata[2] = { 0b00000110, 0x40 }; // Set the characteristic to use 8-bit values, with the sensor connected and detected
  eog_characertistic.write(eogbledata, 2);
}

void connect_callback(uint16_t conn_handle)
{
  // Get the reference to current connection
  BLEConnection* connection = Bluefruit.Connection(conn_handle);

  char central_name[32] = { 0 };
  connection->getPeerName(central_name, sizeof(central_name));

  Serial.print("Connected to ");
  Serial.println(central_name);
}

void printLCD(String characters, u_int8_t col = 0, u_int8_t row = 0, bool clear = false)
{
  if (clear) {
    lcd.clear();
  }
  lcd.setCursor(col, row);  //Set cursor to character 2 on line 0
  lcd.print(characters);
}
/**
   Callback invoked when a connection is dropped
   @param conn_handle connection where this event happens
   @param reason is a BLE_HCI_STATUS_CODE which can be found in ble_hci.h
*/
void disconnect_callback(uint16_t conn_handle, uint8_t reason)
{
  (void) conn_handle;
  (void) reason;

  Serial.print("Disconnected, reason = 0x"); Serial.println(reason, HEX);
  Serial.println("Advertising!");
}

void SDWrite(String text, String file) {
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open(file, FILE_WRITE);
  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(text);
    dataFile.close();
    // print to the serial port too:
    Serial.println("Written to SD: ");
    Serial.println(text);
  }
  // if the file isn't open, error:
  else {
    Serial.println("error opening specified file");
  }
}

void SDRead(String file_read) {
  File myFile = SD.open(file_read);
  if (myFile) {
    Serial.println(file_read);

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening");
  }
}

void cccd_callback(uint16_t conn_hdl, BLECharacteristic* chr, uint16_t cccd_value)
{
  // Display the raw request packet
  Serial.print("CCCD Updated: ");
  //Serial.printBuffer(request->data, request->len);
  Serial.print(cccd_value);
  Serial.println("");

  // Check the characteristic this CCCD update is associated with in case
  // this handler is used for multiple CCCD records.
  if (chr->uuid == eog_characertistic.uuid) {
    if (chr->notifyEnabled(conn_hdl)) {
      Serial.println("EOG 'Notify' enabled");
    } else {
      Serial.println("EOG 'Notify' disabled");
    }
  }
}
void testCircuit(uint8_t state) {
  digitalWrite(ITSY_TESTPULSE_PIN, state);
}

void readInternal(){
  file.open(FILENAME, Adafruit_LittleFS_Namespace::FILE_O_READ);
  // file existed
  if ( file )
  {
    Serial.println(FILENAME " file exists");
    
    uint32_t readlen;
    char buffer[256] = { 0 };
    readlen = file.read(buffer, sizeof(buffer));

    buffer[readlen] = 0;
    Serial.println(buffer);
    file.close();
  }
}

void writeInternal(){
  file.open(FILENAME, Adafruit_LittleFS_Namespace::FILE_O_WRITE);
  // file existed
  if ( file )
  {
    file.write(CONTENTS, strlen(CONTENTS));
    //file.println(CONTENTS);
    Serial.println("Wrote to internal");
    Serial.println();
    file.close();
  }
}

void formatInternal(){
  InternalFS.begin();

  Serial.print("Formating ... ");
  delay(1); // for message appear on monitor

  // Format 
  InternalFS.format();

  Serial.println("Done");
}

// Function for sampling and converting the voltages at pins A0 and A1 (EOG)
void SampleEOG() {
  // Obtain the current ADC values from pins A0 and A1
  adcval1 = analogRead(adcin1); //Read and convert the analog value from pin A0
  adcval2 = analogRead(adcin2); //Read and convert the analog value from pin A1
  V_A0 = (float)adcval1 * mv_per_lsb;
  V_A1 = (float)adcval2 * mv_per_lsb;
}

// ISR function for sampling EOG, temperature, and accelerometer data with the following sampling periods: 5 ms, 10 ms, and 1 ms
void SampleAll() {
  SampleEOG(); 
  sample_count_EOG = sample_count_EOG + 1;
  total_num_samp_EOG = total_num_samp_EOG + 1;
  sprintf(buf, "\nEOG %d: A0 = %f, A1 = %f", total_num_samp_EOG, V_A0, V_A1);
  Serial.print(buf);

  if (sample_count_EOG == 2) {
    total_num_samp_IMU = total_num_samp_IMU + 1;
    sample_count_IMU = sample_count_IMU + 1;
    MPU_accelgyro();
    sprintf(buf, "\nIMU %d: Ax=%.1f, Ay=%.1f, Az=%.1f, Gx=%.1f, Gy=%.1f, Gz=%.1f", total_num_samp_IMU, AccX, AccY, AccZ, GyroX, GyroY, GyroZ);
    Serial.print(buf);
    sample_count_EOG = 0;
  }
  if (sample_count_IMU == 100) {
    total_num_samp_TEMP = total_num_samp_TEMP + 1;
    raw_t = ds1631_temperature();
    c_temp = float(raw_t) / 256;
    c_temp_MPU = float(raw_t_MPU) / 340.0 + 36.53;
    sprintf(buf, "\nTEMP %d: DS=%.1f, MPU=%.1f", total_num_samp_TEMP, c_temp, c_temp_MPU);
    Serial.print(buf);
    sample_count_IMU = 0;
  }
}

void MPU_accelgyro() {
  // Sample gyroscope data
  Vector rawAccel = mpu.readRawAccel(); // Read in the raw accelerometer voltages 
  Vector normAccel = mpu.readNormalizeAccel();
  AccX = float(rawAccel.XAxis) / 16384.0;
  AccY = float(rawAccel.YAxis) / 16384.0;
  AccZ = float(rawAccel.ZAxis) / 16384.0;
  // Sample gyroscope data
  Vector rawGyro = mpu.readRawGyro(); // Read in the raw gyroscope voltages 
  Vector normGyro = mpu.readNormalizeGyro();
  GyroX = float(rawGyro.XAxis) / 131.0;
  GyroY = float(rawGyro.YAxis) / 131.0;
  GyroZ = float(rawGyro.ZAxis) / 131.0;
  // Read the temperature of the MPU-6050 sensor
  Wire.beginTransmission(IMU_ADDRESS); // Begin transmission to the IMU sensor
  Wire.write(0x41); // Write the address of the first temperature data register (0x41)
  Wire.endTransmission(false); // Send a repeated start bit
  Wire.requestFrom(IMU_ADDRESS, 2, true); // Read from the two temperature data registers and then release I2C bus
  raw_t_MPU = Wire.read() << 8 | Wire.read(); // Calculate the raw temperature value in binary
}

int16_t ds1631_temperature() {
  Wire.beginTransmission(DS1631_ADDRESS); // Connect to the DS1631 (send DS1631 address)
  Wire.write(0xAA); // Send the read temperature command
  Wire.endTransmission(false); // Send a repeated start bit
  Wire.requestFrom(DS1631_ADDRESS, 2); // Request a read from the two temperature data registers and then release I2C bus
  int16_t raw_t = Wire.read() << 8 | Wire.read(); // Calculate the raw temperature value in binary
  return raw_t;
}

void loop()
{
  Serial.println(digitalRead(12));
  Serial.println();
  Serial.println();
  Serial.println();
  Serial.println("Which demo would you like to see? ");
  Serial.println("1. SD Card Read");
  Serial.println("2. SD Card Write");
  Serial.println("3. Fun LCD");
  Serial.println("4. Write Internal FLash");
  Serial.println("5. Read Internal FLash");
  Serial.println("6. Current Temp");
  Serial.println("7. IMU");
  Serial.println("8. Run POST Circuit");
  Serial.println("9. BLE");


  while (Serial.available() == 0) {
  }

  menuChoice = Serial.parseInt();
  Serial.parseInt();
  switch (menuChoice) {
    case 1:

      Serial.print("SD Card Demo Read: ");
      SDRead("BME_DEMO.txt");
      break;

    case 2:

      Serial.print("Text written to SD: ");
      SDWrite("I can't beleive it worked!", "BME_DEMO.txt");
      break;

    case 3:

      Serial.print("Fun LCD: ");
      printLCD("UMAPATHY IS THE ", 0, 0, true);
      printLCD("BEST FLC AT TMU", 0, 1, false);
      break;
    case 4:
      writeInternal();
      break;
    case 5:
      readInternal();
      break;
    case 6:
      printLCD("Temp: Too Hot");
      break;
    case 7:
      printLCD("IMU");
      break;
    case 8:
      printLCD("Pulse On");
      testCircuit(1);
      break;

    case 9:
      Serial.println("Searching for Connection");
      while (1)
      {
        
        if (Bluefruit.connected())
        {
          uint8_t hrmdata[2] = {0b00000110, bps++}; // Sensor connected, increment BPS value

          // Note: We use .notify instead of .write!
          // If it is connected but CCCD is not enabled
          // The characteristic's value is still updated although notification is not sent
          if (eog_characertistic.notify(hrmdata, sizeof(hrmdata)))
          {
            Serial.print("Value updated to: ");
            Serial.println(bps);
          }
          else
          {
            Serial.println("ERROR: Notify not set in the CCCD or not connected!");
          }
        }
      }
      break;


    default:
      Serial.println("Please choose a valid selection");
  }

  digitalToggle(LED_RED);

}
