
#include <SPI.h>
#include <SD.h>
#include <Adafruit_TinyUSB.h>  // for Serial
#include <Arduino.h>
#include <Wire.h>
#include <bluefruit.h>
#include <Adafruit_LittleFS.h>
#include <InternalFileSystem.h>
#include <LiquidCrystal_I2C.h>
#include <ArduinoQueue.h>
#include "NRF52TimerInterrupt.h"
#include "NRF52_ISR_Timer.h"
#include <math.h>
#include <stdio.h>

#define GyroMax 100000
#define GyroMin -100000
#define TempMax 100000
#define TempMin -100000
#define TIMER_INTERRUPT_DEBUG 0
#define HW_TIMER_INTERVAL_MS 1  // Hardware timer period in ms
#define TIMER_INTERVAL_5MS 5L   // EOG sampling period in ms (interrupt timer period)
#define MAX_TEMP 30
#define MIN_TEMP -30
#define MAX_POST_ERROR
struct binFromSDStruct {
  byte buffer[1412];
};
NRF52Timer ITimer(NRF_TIMER_1);
NRF52_ISR_Timer ISR_Timer;
LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address to 0x3F for a 16 chars and 2 line display
//using namespace Adafruit_LittleFS_Namespace;
#define FILENAME "/systemSettings.txt"
#define CONTENTS "Wow even the flash memory works\n"
Adafruit_LittleFS_Namespace::File file(InternalFS);
BLEService eog_service = BLEService(0x1108);
BLECharacteristic eog_characertistic = BLECharacteristic(0x2B3D);
BLEDis bledis;  // DIS (Device Information Service) helper class instance
BLEBas blebas;  // BAS (Battery Service) helper class instance
const int chipSelect = 2;
const uint8_t ITSY_TESTPULSE_PIN = 7;

// Define global variables
int adcin1 = A0;  // Pin connected to analog voltage input (EOG) 1
int adcin2 = A1;  // Pin connected to analog voltage input (EOG) 1

float mv_per_lsb = 3300.0F / 4096.0F;  // Number of millivolts per less significant bit for 12-bit ADC with a 3.3 V input range
byte sample_IMU = 0;                   // Count for tracking when to take accelerometer/gyroscope samples based on the number of EOG samples taken
byte sample_count_IMU = 0;             // Count for tracking when to take temperature samples based on the number of IMU samples taken
byte total_num_samp_EOG = 0;           // Count of the total number of EOG samples taken
byte total_num_samp_IMU = 0;           // Count of the total number of accelerometer and gyroscope samples taken
byte total_num_samp_TEMP = 0;          // Count of the total number of temperature samples taken
byte total_chunk_samp = 0;             // Count of the total number of temperature samples taken
float c_temp = 0;                      // For storing readings in degrees C from the DS1631+ temperature sensor
float c_temp_MPU = 0;                  // For storing the reading in degrees C from the temperature sensor in the MPU-6050 IMU
unsigned short raw_t;                  // For storing the raw reading from the DS1631+ temperature sensor
int16_t raw_t_MPU = 0;                 // For storing the raw reading from the temperature sensor in the MPU-6050 IMU
unsigned short AccX, AccY, AccZ;       // For storing raw readings from all three axes of the accelerometer
unsigned short GyroX, GyroY, GyroZ;    // For storing raw readings from all three axes of the gyroscope
char buf[64];
bool sample = false;
uint8_t bps = 0;
bool sampling = false;
bool BLE_switch = false;
bool sampling_switch = false;
bool need_SD_chunk = true;
String error_message = "";
bool writing_to_SD = false;
byte system_state = 1;
byte previous_system_state = 1;
bool screen_updated = false;
bool BLE_connected = false;
byte run_number = 0;
unsigned int chunk_num = 0;
unsigned int BLE_num = 0;
unsigned int BLE_buffer_index = 0;
uint8_t BLE_out_data[20];
unsigned short adcval1;  // For storing the voltage on pin A0
unsigned short adcval2;  // For storing the voltage on pin A1
byte binFromSD[1412];
byte buffer[1407];
byte bufferRead[1407];
byte bufferBLE[20];
unsigned int read_position = 0;
String sampling_error_message = "";

struct SystemSettings{
  byte runNumber = 0;
  byte currentRunNumbers[255] = {0};
  byte BLEDone[255] = {0};
  unsigned int BLEIndex;
};
SystemSettings systemSettings;

struct chunk {
  byte run; // 8 bits
  unsigned int number; // 32 bits
  unsigned short temp; // 16 bits
  unsigned short EOG1[200]; // 16 bits x 200
  unsigned short EOG2[200]; // 16 bits x 200
  unsigned short GyroX[100]; // 16 bits x 100
  unsigned short GyroY[100]; // 16 bits x 100
  unsigned short GyroZ[100]; // 16 bits x 100
  bool done_sampling = false;
};
chunk current_chunk_sample;
chunk chunk_to_SD;
chunk chunk_from_SD;
chunk POST_chunk;
// Queue creation:
chunk Q[10];   // Limit of 10 things in line
int tail = 0;  // Nothing currently in line

void printLCD(String characters, u_int8_t col = 0, u_int8_t row = 0, bool clear = false) {
  if (sampling) {
    ITimer.disableTimer();
  }
  if (clear) {
    lcd.clear();
  }
  lcd.setCursor(col, row);  //Set cursor to character 2 on line 0
  lcd.print(characters);
  if (sampling) {
    ITimer.enableTimer();
  }
}

void scrollMessage(int row, String message, int delayTime, int totalColumns) {
  for (int i = 0; i < totalColumns; i++) {
    message = " " + message;
  }
  message = message + " ";
  for (int position = 0; position < message.length(); position++) {
    lcd.setCursor(0, row);
    lcd.print(message.substring(position, position + totalColumns));
    delay(delayTime);
  }
}

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
  error_message = error_message + LCDsetup();

  // Setup Internal Flash
  Serial.println("Initializing Internal Flash Memory...");
  bool internal_flash_good = InternalFS.begin();
  if (internal_flash_good)
    Serial.println("Internal Flash Memory Initialized");
  else {
    Serial.println("Internal Flash Memory Error");
    error_message = error_message + "SPI Flash Error";
  }


  // Setup SD Card
  error_message = error_message + setupSD();

  // Setup Temp Sensor
  Serial.println("Initializing DS1632");
  error_message = error_message + setupDS1631();
  if (error_message == "")
    Serial.println("DS1632 Initialized");

  // Setup MPU
  error_message = error_message + setupMPU();

  error_message = error_message + timerSetup();
  BLESetup();
  error_message = analogPOST();
  if (error_message != "") {
    system_state = 0;
    while (1) {
      printLCD("RESET SYSTEM", 0, 1, 1);
      scrollMessage(0, error_message, 600, 16);
    }
  }
  //writeSystemSettingsFull(systemSettings);
  readSystemSetting();
  DetermineRunNumber();
  readSystemSetting();
  delay(1000);
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

String stringifyChunk(chunk data_chunk) {
  String output = "";
  output = output + data_chunk.run;
  output = output + (String)data_chunk.temp;
  for (int i = 0; i < 200; i++) {
    output = output + (String)data_chunk.EOG1[i];
    output = output + (String)data_chunk.EOG2[i];
  }

  for (int i = 0; i < 100; i++) {
    //output = output + (String)data_chunk.AccX[i/2] + (String)data_chunk.AccY[i/2] + (String)data_chunk.AccZ[i/2];
    output = output + (String)data_chunk.GyroX[i] + (String)data_chunk.GyroY[i] + (String)data_chunk.GyroZ[i];
  }
  return output;
}

String stringifyChunk2(chunk data_chunk) {
  String output = "";
  output = output + data_chunk.run;
  output = output + "," + data_chunk.number;
  output = output + "," + (String)data_chunk.temp;
  for (int i = 0; i < 200; i++) {
    output = output + "," + (String)data_chunk.EOG1[i];
    output = output + "," + (String)data_chunk.EOG2[i];
  }

  for (int i = 0; i < 100; i++) {
    //output = output + (String)data_chunk.AccX[i/2] + (String)data_chunk.AccY[i/2] + (String)data_chunk.AccZ[i/2];
    output = output + "," + (String)data_chunk.GyroX[i] + "," + (String)data_chunk.GyroY[i] + "," + (String)data_chunk.GyroZ[i];
  }
  return output;
}


void loop() {


  BLE_switch = digitalRead(11);
  sampling_switch = digitalRead(12);

  /*************************************************/

  if (error_message != "") {
    system_state = 0;
  } else {
    if (sampling_switch) {
      system_state = 3;
      if (previous_system_state != system_state) {
        screen_updated = false;
      }
    } else {
      if (BLE_switch) {
        system_state = 2;
        if (previous_system_state != system_state) {
          screen_updated = false;
        }
      } else {
        system_state = 1;
        if (previous_system_state != system_state) {
          screen_updated = false;
        }
      }
    }
  }

  if (previous_system_state == 2 && system_state != 2) {
    Bluefruit.Advertising.stop();
  }
  if (previous_system_state == 3 && system_state != 3) {
    ITimer.disableTimer();
    sampling = false;
  }
  switch (system_state) {
    case 0:
      printLCD(error_message, 0, 0, 1);
      printLCD("RESET SYSTEM", 0, 1, 0);
      ITimer.disableTimer();
      sampling = false;
      Bluefruit.Advertising.stop();
      while (1) {}
      break;
    case 1:
      if (!screen_updated) {
        printLCD("Power On,Idleing", 0, 0, 1);
        printLCD("Systems: Good", 0, 1, 0);
        screen_updated = true;
      }
      break;
    case 2:
      if (!screen_updated) {
        printLCD("BLE Connecting..", 0, 1, 1);
        screen_updated = true;
        startAdv();
      }


      if (Bluefruit.connected()) {
        if (!BLE_connected) {
          printLCD("Transmitting...", 0, 0, 1);
          printLCD("BLE Connected", 0, 1, 0);
        }
        BLE_connected = true;

        if (need_SD_chunk) {
          readChunkBinary(bufferRead);
          //bufferToBLE(bufferRead, bufferBLE);
          for (int j = 1; j < 20; j++) {
            bufferBLE[j] = bufferRead[BLE_buffer_index + j];
            //Serial.println(BLE_buffer_index + j);
          }
          bufferBLE[0] = (int)BLE_buffer_index/19;
          BLE_buffer_index += 19;
          if (BLE_buffer_index == 1425){
            Serial.println("Last packet");
            Serial.println(bufferBLE[0]);
            bufferBLE[0] = bufferBLE[0] + 128;
            Serial.println(bufferBLE[0]);
          }
          need_SD_chunk = false;
        }

        if (eog_characertistic.notify(bufferBLE, 20)) {
          for (int j = 0; j < 20; j++) {
            bufferBLE[j] = bufferRead[BLE_buffer_index + j];
            //Serial.println(BLE_buffer_index + j);
          }
          bufferBLE[0] = (int)BLE_buffer_index/19;
          BLE_buffer_index += 19;
          if (BLE_buffer_index == 1425){
            Serial.println("Last packet");
            Serial.println(bufferBLE[0]);
            bufferBLE[0] = bufferBLE[0] + 128;
            Serial.println(bufferBLE[0]);
          }
        } else {
          //Serial.println("ERROR: Notify not set");
        }
      }
      if (!Bluefruit.connected() && BLE_connected) {
        BLE_connected = false;
        screen_updated = false;
      }

      break;
    case 3:
      if (!sampling) {
        ITimer.enableTimer();
        sampling = true;
      }
      if (!screen_updated) {
        printLCD("Recording Data...", 0, 0, 1);
        printLCD("Systems: Good", 0, 1, 0);

        screen_updated = true;
      }
      break;
  }
  previous_system_state = system_state;
  if (tail > 0 && Q[0].done_sampling) {
    if (tail > 0) {
      chunk_to_SD = Q[0];
      for (int i = 0; i < tail; i++) {
        Q[i] = Q[i + 1];
      }
      tail--;
    }
    //Serial.println(stringifyChunk2(chunk_to_SD));
    //Serial.println("Writing Chunk to SD");
    //error_message = SDWrite(stringifyChunk2(chunk_to_SD), "datalog2.txt");
    //SDWrite2(chunk_to_SD, "test3.bin");
    chunkToBinary(chunk_to_SD, buffer);
    Serial.println("Chunk to Bin");
    //readChunkBinary(bufferRead);
    //readChunkFromSDtest();
  }
}
