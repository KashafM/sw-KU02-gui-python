
#include <SPI.h>
#include <SD.h>
#include <Adafruit_TinyUSB.h>  // for Serial
#include <Arduino.h>
#include <Wire.h>
#include <bluefruit.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include <LiquidCrystal_I2C.h>
#include <CircularBuffer.h>
#include "NRF52TimerInterrupt.h"
#include "NRF52_ISR_Timer.h"

#define TIMER_INTERRUPT_DEBUG 0
#define HW_TIMER_INTERVAL_MS 1  // Hardware timer period in ms
#define TIMER_INTERVAL_5MS 5L   // EOG sampling period in ms (interrupt timer period)

NRF52Timer ITimer(NRF_TIMER_1);
NRF52_ISR_Timer ISR_Timer;
LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address to 0x3F for a 16 chars and 2 line display
//using namespace Adafruit_LittleFS_Namespace;
#define FILENAME "/adafruit2.txt"
#define CONTENTS "Wow even the flash memory works\n"
Adafruit_LittleFS_Namespace::File file(InternalFS);
BLEService eog_service = BLEService(0x1108);
BLECharacteristic eog_characertistic = BLECharacteristic(0x2B3D);
BLEDis bledis;  // DIS (Device Information Service) helper class instance
BLEBas blebas;  // BAS (Battery Service) helper class instance
const int chipSelect = 2;
const uint8_t ITSY_TESTPULSE_PIN = 7;

// Define global variables
int adcin1 = A0;                       // Pin connected to analog voltage input (EOG) 1
int adcin2 = A1;                       // Pin connected to analog voltage input (EOG) 1
unsigned short adcval1 = 0;            // For storing the voltage on pin A0
unsigned short adcval2 = 0;            // For storing the voltage on pin A1
float mv_per_lsb = 3300.0F / 4096.0F;  // Number of millivolts per less significant bit for 12-bit ADC with a 3.3 V input range
byte sample_IMU = 0;                   // Count for tracking when to take accelerometer/gyroscope samples based on the number of EOG samples taken
byte sample_count_IMU = 0;             // Count for tracking when to take temperature samples based on the number of IMU samples taken
byte total_num_samp_EOG = 0;           // Count of the total number of EOG samples taken
byte total_num_samp_IMU = 0;           // Count of the total number of accelerometer and gyroscope samples taken
byte total_num_samp_TEMP = 0;          // Count of the total number of temperature samples taken
byte total_chunk_samp = 0;             // Count of the total number of temperature samples taken
float c_temp = 0;                      // For storing readings in degrees C from the DS1631+ temperature sensor
float c_temp_MPU = 0;                  // For storing the reading in degrees C from the temperature sensor in the MPU-6050 IMU
int16_t raw_t = 0;                     // For storing the raw reading from the DS1631+ temperature sensor
int16_t raw_t_MPU = 0;                 // For storing the raw reading from the temperature sensor in the MPU-6050 IMU
float AccX, AccY, AccZ;                // For storing raw readings from all three axes of the accelerometer
float GyroX, GyroY, GyroZ;             // For storing raw readings from all three axes of the gyroscope
char buf[64];
bool sample = false;
uint8_t bps = 0;
bool sampling = false;
bool BLE_switch = false;
bool sampling_switch = false;
int menuChoice;

struct chunk {
  String header;
  int temp;
  int EOG1[200];
  int EOG2[200];
  int GyroX[100];
  int GyroY[100];
  int GyroZ[100];
  bool done_sampling = false;
};
chunk current_chunk_sample;
chunk to_SD;
CircularBuffer<chunk, 20> chunks;

void setup() {
  Serial.begin(115200);

  pinMode(ITSY_TESTPULSE_PIN, OUTPUT);
  pinMode(13, INPUT_PULLDOWN);
  pinMode(12, INPUT_PULLDOWN);
  pinMode(11, INPUT_PULLDOWN);
  digitalWrite(ITSY_TESTPULSE_PIN, LOW);

  // Set up the analog to digital converter
  analogReference(AR_VDD4);  // Set the analog reference voltage to VDD (3.3 V)
  analogReadResolution(12);  // Set the resolution to 12-bit

  // Setup LCD
  Serial.println("Initializing LCD...");
  bool LCD_good = LCDsetup();
  if (LCD_good)
  Serial.println("LCD Initialized");

  // Setup Internal Flash
  Serial.println("Initializing Internal Flash Memory...");
  bool internal_flash_good = InternalFS.begin();
  if (internal_flash_good)
  Serial.println("Internal Flash Memory Initialized");

  // Setup SD Card
  bool SD_good = setupSD();

  // Setup Temp Sensor
  Serial.println("Initializing DS1632");
  bool temp_good = setupDS1631();
  if (temp_good)
  Serial.println("DS1632 Initialized");

  // Setup MPU
  bool MPU_good = setupMPU();
  if (MPU_good)
  Serial.println("Inertial Motion Unit Initialized");
  bool timer_good = timerSetup();
  //BLESetup();
  

  delay(1000);
}

void printLCD(String characters, u_int8_t col = 0, u_int8_t row = 0, bool clear = false) {
  ITimer.disableTimer();
  if (clear) {
    lcd.clear();
  }
  lcd.setCursor(col, row);  //Set cursor to character 2 on line 0
  lcd.print(characters);
  ITimer.enableTimer();
}

void connect_callback(uint16_t conn_handle) {
  // Get the reference to current connection
  BLEConnection* connection = Bluefruit.Connection(conn_handle);

  char central_name[32] = { 0 };
  connection->getPeerName(central_name, sizeof(central_name));

  Serial.print("Connected to ");
  Serial.println(central_name);
}

void cccd_callback(uint16_t conn_hdl, BLECharacteristic* chr, uint16_t cccd_value) {
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

void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  (void)conn_handle;
  (void)reason;

  Serial.print("Disconnected, reason = 0x");
  Serial.println(reason, HEX);
  Serial.println("Advertising!");
}

void loop() {
  //Get Swtch States
  BLE_switch = digitalRead(11);
  sampling_switch = digitalRead(12);
  //Serial.println(BLE_switch);
  //Serial.println(sampling_switch);
  if (!sampling_switch && BLE_switch){
  /*{
  if (Bluefruit.connected()) {
    uint8_t hrmdata[2] = { 0b00000110, bps++ };  // Sensor connected, increment BPS value

    // Note: We use .notify instead of .write!
    // If it is connected but CCCD is not enabled
    // The characteristic's value is still updated although notification is not sent
    if (eog_characertistic.notify(hrmdata, sizeof(hrmdata))) {
      Serial.print("Value updated to: ");
      Serial.println(bps);
    } else {
      Serial.println("ERROR: Notify not set in the CCCD or not connected!");
    }
  }
  */
  digitalWrite(ITSY_TESTPULSE_PIN, 1);
  }
  else{
    digitalWrite(ITSY_TESTPULSE_PIN, 0);
  }
  if (sampling_switch){
    if (!sampling){
      ITimer.enableTimer();
      
      sampling = true;
    }
  }
  else{
    if (sampling){
      ITimer.disableTimer();
      //Serial.println(chunks.size());
      Serial.println(stringifyChunk(chunks.first()));
      
    }
    sampling = false;
  }

  if (chunks.first().done_sampling) { 
    ITimer.disableTimer();
    Serial.println("SD TIme");
    SDWrite(stringifyChunk(chunks.shift()), "datalog.txt");
    //SDWrite("testtt", "datalog.txt");

    Serial.println("SD");
    Serial.println(chunks.size());
  }
 
  
  digitalToggle(LED_RED);
}
