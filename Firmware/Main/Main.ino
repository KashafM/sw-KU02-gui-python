int DIAGNOSTIC_MODE = 1;
//Diag 1: No Headset
//Diag 2: No Headset or SD card or LCD
//DIag 3: Just Pulse with LCD
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

#define IMU_MAX 500
#define TempMax 50
#define TempMin -40
#define TIMER_INTERRUPT_DEBUG 0
#define HW_TIMER_INTERVAL_MS 1  // Hardware timer period in ms
#define TIMER_INTERVAL_5MS 5L   // EOG sampling period in ms (interrupt timer period)
#define MAX_TEMP 60
#define MIN_TEMP -30
#define MAX_POST_ERROR
struct binFromSDStruct {
  byte buffer[1412];
};
NRF52Timer ITimer(NRF_TIMER_1);
NRF52_ISR_Timer ISR_Timer;
LiquidCrystal_I2C lcd(0x27, 16, 2);  // set the LCD address to 0x3F for a 16 chars and 2 line display
//using namespace Adafruit_LittleFS_Namespace;
#define FILENAME "/d=//systemLogger.txt"
//#define CONTENTS "Wow even the flash memory worksn"
Adafruit_LittleFS_Namespace::File file(InternalFS);
BLEService eog_service = BLEService(0x1108);
BLEService file_service = BLEService(0x1109);

BLECharacteristic eog_characertistic = BLECharacteristic(0x2B3D);
BLECharacteristic files_charactertistic = BLECharacteristic(0x2B3E);
BLECharacteristic fileSelect_charactertistic = BLECharacteristic(0x2B3F);
BLEDis bledis;  // DIS (Device Information Service) helper class instance
BLEBas blebas;  // BAS (Battery Service) helper class instance
const int chipSelect = 2;
const uint8_t ITSY_TESTPULSE_PIN = 9;
const uint8_t ITSY_BUZZER_PIN = 7;
int packets_per_chunk = 75;
// Define global variables
int adcin1 = A0;  // Pin connected to analog voltage input (EOG) 1
int adcin2 = A1;  // Pin connected to analog voltage input (EOG) 1
int diag3Counter = 0;
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
bool light_switch = false;
bool need_SD_chunk = true;
String error_message = "";
bool writing_to_SD = false;
bool advertising = false;
byte system_state = 1;
byte previous_system_state = 1;
bool BLE_connected = false;
byte run_number = 0;
int currentFileSize = 0;
int number_of_chunk_BLE = 0;
int number_of_chunk_sent_BLE = 0;
int backLightCounter = 50000;
unsigned int chunk_num = 0;
unsigned int BLE_num = 0;
unsigned int BLE_buffer_index = 0;
uint8_t BLE_out_data[20];
unsigned short adcval1;  // For storing the voltage on pin A0
unsigned short adcval2;  // For storing the voltage on pin A1
byte binFromSD[1412];
byte buffer[1407];
byte BLEbuffer;
byte bufferRead[1407];
byte bufferBLE[20];
bool testtest = true;
bool testtesttest = false;
bool BLE_force_stop = false;
bool fileSelected = false;
bool setupBypass = false;
int fileChoice;
unsigned int read_position = 0;
String sampling_error_message = "";
int wdt = 1;
int num = 0;
String LCDtext[2];
String LCDtext_Shown[2];
String hours = "0";
String minutes = "0";
String seconds = "0";
String PreviousSeconds = "0";
int totalSeconds = 0;

struct SystemSettings {
  byte runNumber = 0;
  byte currentRunNumbers[255] = { 0 };
  byte BLEDone[255] = { 0 };
  unsigned int BLEIndex;
};
SystemSettings systemSettings;

struct chunk {
  byte run;                   // 8 bits
  unsigned int number;        // 32 bits
  unsigned short temp;        // 16 bits
  unsigned short EOG1[200];   // 16 bits x 200
  unsigned short EOG2[200];   // 16 bits x 200
  unsigned short GyroX[100];  // 16 bits x 100
  unsigned short GyroY[100];  // 16 bits x 100
  unsigned short GyroZ[100];  // 16 bits x 100
  bool done_sampling = false;
};
chunk current_chunk_sample;
chunk chunk_to_SD;
chunk chunk_from_SD;
chunk POST_chunk;
// Queue creation:
chunk Q[10];   // Limit of 10 things in line
int tail = 0;  // Nothing currently in line


void beepOn(int option) {
  switch (option) {
    case 0:  // POWER ON
      tone(ITSY_BUZZER_PIN, 5000, 70);
      break;
    case 1:  // System Idle
      tone(ITSY_BUZZER_PIN, 5000, 70);
      delay(150);
      tone(ITSY_BUZZER_PIN, 5000, 70);
      break;
    case 2:  // ERROR
      tone(ITSY_BUZZER_PIN, 7000, 700);
      tone(ITSY_BUZZER_PIN, 3000, 700);
      delay(500);
      tone(ITSY_BUZZER_PIN, 7000, 700);
      tone(ITSY_BUZZER_PIN, 3000, 700);
      delay(500);
      tone(ITSY_BUZZER_PIN, 7000, 700);
      tone(ITSY_BUZZER_PIN, 3000, 700);
      delay(500);
      tone(ITSY_BUZZER_PIN, 7000, 700);
      tone(ITSY_BUZZER_PIN, 3000, 700);
      delay(500);
      tone(ITSY_BUZZER_PIN, 7000, 700);
      tone(ITSY_BUZZER_PIN, 3000, 700);
      delay(500);
      tone(ITSY_BUZZER_PIN, 7000, 700);
      tone(ITSY_BUZZER_PIN, 3000, 700);
      break;
    case 3:  // STart Record
      tone(ITSY_BUZZER_PIN, 4000, 800);

      break;
    case 4:  // Stop Record
      tone(ITSY_BUZZER_PIN, 5000, 70);
      delay(150);  // delay in between reads for stability
      tone(ITSY_BUZZER_PIN, 5000, 70);
      delay(150);  // delay in between reads for stability
      tone(ITSY_BUZZER_PIN, 5000, 70);
      delay(150);  // delay in between reads for stability
      tone(ITSY_BUZZER_PIN, 5000, 70);
      break;
    case 5:  // Start BLE
      tone(ITSY_BUZZER_PIN, 2000, 200);
      delay(150);  // delay in between reads for stability
      tone(ITSY_BUZZER_PIN, 3000, 200);
      delay(150);  // delay in between reads for stability
      tone(ITSY_BUZZER_PIN, 4000, 400);

      break;
  }
  digitalWrite(ITSY_BUZZER_PIN, LOW);
}

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
  pinMode(ITSY_BUZZER_PIN, OUTPUT);
  pinMode(13, INPUT_PULLDOWN);
  pinMode(12, INPUT_PULLDOWN);
  pinMode(11, INPUT_PULLDOWN);
  digitalWrite(ITSY_TESTPULSE_PIN, LOW);
  digitalWrite(ITSY_BUZZER_PIN, LOW);
  beepOn(0);

  // Setup Internal Flash
  Serial.println("Initializing Internal Flash Memory...");
  bool internal_flash_good = InternalFS.begin();
  if (internal_flash_good)
    Serial.println("Internal Flash Memory Initialized");
  else {
    Serial.println("Internal Flash Memory Error");
    error_message = error_message + "SPI Flash Error";
  }
  //formatInternal();
  //systemLogger("System Power On");
  //systemLogger("Diagnostic Mode: " + (String)DIAGNOSTIC_MODE);
  if (DIAGNOSTIC_MODE != 0) {
    //outputLog();
  }
  //for (int i = 0; i < length; i++) {
  //if (notes[i] == ' ') {
  //delay(beats[i] * tempo); // rest
  //} else {
  //playNote(notes[i], beats[i] * tempo);
  //}
  // pause between notes
  // delay(tempo);
  //}
  // Set up the analog to digital converter
  analogReference(AR_VDD4);  // Set the analog reference voltage to VDD (3.3 V)
  analogReadResolution(12);  // Set the resolution to 12-bit

  //Configure WDT.
  if (DIAGNOSTIC_MODE != 3){
    NRF_WDT->CONFIG         = 0x01;             // Configure WDT to run when CPU is asleep
    NRF_WDT->CRV            = 3932159;  // Timeout set to 120 seconds, timeout[s] = (CRV-1)/32768
    NRF_WDT->RREN           = 0x01;             // Enable the RR[0] reload register
    NRF_WDT->TASKS_START    = 1;                // Start WDT
  }

  if (DIAGNOSTIC_MODE != 2) {
    // Setup LCD
    Serial.println("Initializing LCD...");
    //systemLogger("System Power On");
    error_message = error_message + LCDsetup();
  }
  if (DIAGNOSTIC_MODE == 3) {
    printLCD("TEST PULSE ON", 0, 1, 1);
    digitalWrite(ITSY_TESTPULSE_PIN, HIGH);
    while (diag3Counter < 100000) {

      delay(60);
    }

    digitalWrite(ITSY_TESTPULSE_PIN, LOW);
  }


  if (DIAGNOSTIC_MODE != 2 && error_message == "") {
    // Setup SD Card
    error_message = error_message + setupSD();
  }
  bool notemp = false;
  if (DIAGNOSTIC_MODE != 1 && DIAGNOSTIC_MODE != 2 && error_message == "") {
    // Setup Temp Sensor
    Serial.println("Initializing DS1632");
    error_message = error_message + setupDS1631();
    if (error_message == "")
      Serial.println("DS1632 Initialized");

    // Setup MPU
    error_message = error_message + setupMPU();
  }
  if (error_message == "Temp Error IMU Error "){
    error_message = "NO HEADSET";
  }
  if (error_message == "") {
    error_message = error_message + timerSetup();
    BLESetup();
  }
  if (DIAGNOSTIC_MODE != 1 && DIAGNOSTIC_MODE != 2 && error_message == "") {
    error_message = analogPOST();
  }

  if (error_message != "") {
    system_state = 0;
    //systemLogger("System Setup Error: ");
    //systemLogger(error_message);
    while(1){
    if (DIAGNOSTIC_MODE != 2) {
      printLCD("RESET SYSTEM", 0, 1, 1);
    }
    int length = error_message.length();
    if (length < 18){
      printLCD(error_message, 0, 0, 0);
      beepOn(2);
    }
    else{
      beepOn(2);
      scrollMessage(0, error_message, 600, 16);
    }

    }
    beepOn(2);
  }

  //writeSystemSettingsFull(systemSettings);
  //readSystemSetting();
  DetermineRunNumber();
  beepOn(1);
  //systemLogger("System Setup Complete");
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

void write_callback(uint16_t conn_hdl, BLECharacteristic* chr, unsigned char* Recvalue, uint16_t Reclength) {

  String value = "";
  for (int i = 0; i < Reclength; i++) {
    value += (char)Recvalue[i];
  }
  if (!fileSelected) {
    fileChoice = value.toInt();
    Serial.print("File Choice: ");
    Serial.print(fileChoice);
    Serial.print(" Available: ");
    Serial.println(systemSettings.currentRunNumbers[fileChoice]);
  }

  if (systemSettings.currentRunNumbers[fileChoice] == 0) {
    Serial.println("Invalid File Choice");
  } else {
    fileSelected = true;
  }
}


void disconnect_callback(uint16_t conn_handle, uint8_t reason) {
  (void)conn_handle;
  (void)reason;

  Serial.print("Disconnected, reason = 0x");
  Serial.println(reason, HEX);
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
  if (LCDtext_Shown[0] != LCDtext[0] || LCDtext_Shown[1] != LCDtext[1]) {
    printLCD(LCDtext[0], 0, 0, 1);
    LCDtext_Shown[0] = LCDtext[0];
    printLCD(LCDtext[1], 0, 1, 0);
    LCDtext_Shown[1] = LCDtext[1];
  }
  if (DIAGNOSTIC_MODE != 3){
    // Reload the WDTs RR[0] reload register
    NRF_WDT->RR[0] = WDT_RR_RR_Reload;
  }
  

  BLE_switch = digitalRead(11);


  sampling_switch = digitalRead(12);

  light_switch = digitalRead(13);
  if (light_switch){
    backLightCounter = 50000;
  }
  if (backLightCounter>0){
    backLightCounter = backLightCounter - 1;
  }
  if (backLightCounter > 0){
     lcd.setBacklight(1);
  }
  else{
    if (DIAGNOSTIC_MODE == 0){
      lcd.setBacklight(0);
    }
    
  }
  /*************************************************/

  if (error_message != "") {
    system_state = 0;
  } else {
    if (sampling_switch) {
      system_state = 3;
      if (previous_system_state != system_state) {
        //systemLogger("State Updated");
        //systemLogger((String)system_state);
      }
    } else {
      if (BLE_switch) {
        system_state = 2;
        if (previous_system_state != system_state) {
          //systemLogger("State Updated");
          //systemLogger((String)system_state);
        }
      } else {
        system_state = 1;
        if (previous_system_state != system_state) {
          //systemLogger("State Updated");
          //systemLogger((String)system_state);
        }
      }
    }
  }

  if (previous_system_state == 2 && system_state != 2) {
    //systemLogger("99 BLE OFF");
    delay(50);
    BLESetup();
    BLE_connected = false;
    advertising = false;
  }
  if (previous_system_state == 3 && system_state != 3) {
    ITimer.disableTimer();
    sampling = false;
    beepOn(4);
  }
  switch (system_state) {
    case 0:
      printLCD(error_message, 0, 0, 1);
      printLCD("RESET SYSTEM", 0, 1, 0);
      //systemLogger("Runtime Error:");
      //systemLogger(error_message);
      ITimer.disableTimer();
      sampling = false;
      Bluefruit.Advertising.stop();
      while (1) {}
      break;
    case 1:
      if (DIAGNOSTIC_MODE == 0) {
        LCDtext[0] = "Power On,Idleing";
        LCDtext[1] = "Systems: Good";
      }
      if (DIAGNOSTIC_MODE == 1) {
        LCDtext[0] = "Power On,Idleing";
        LCDtext[1] = "Diagnostic 1";
      }
      if (DIAGNOSTIC_MODE == 2) {
        LCDtext[0] = "Power On,Idleing";
        LCDtext[1] = "Diagnostic 2";
      }

      //printLCD("Power On,Idleing", 0, 0, 1);
      //printLCD("Systems: Good", 0, 1, 0);

      break;
    case 2:
      if (BLE_force_stop) {
        LCDtext[0] = "Transmission";
        LCDtext[1] = "Done";
      } else {
        LCDtext[0] = "BLE Connecting..";
        LCDtext[1] = "Pair Device";
      }

      if (!advertising) {
        startAdv();
        beepOn(5);
        advertising = true;
        updateFileOptions();
      }

      if (Bluefruit.connected()) {

        LCDtext[0] = "BLE Connected";
        LCDtext[1] = "";


        BLE_connected = true;
        if (fileSelected) {
          if (need_SD_chunk) {
            readChunkBinary(bufferRead);
            //bufferToBLE(bufferRead, bufferBLE);
            for (int j = 1; j < 20; j++) {
              bufferBLE[j] = bufferRead[BLE_buffer_index + j];
              //Serial.println(BLE_buffer_index + j);
            }
            bufferBLE[0] = (int)BLE_buffer_index / 19;
            BLE_buffer_index += 19;
            need_SD_chunk = false;
          }

          if (eog_characertistic.notify(bufferBLE, 20)) {
            printLCD((String)((int)(100 * (number_of_chunk_sent_BLE*packets_per_chunk+(int)BLE_buffer_index / 19) / (number_of_chunk_BLE*packets_per_chunk))), 0, 1, 0);
            printLCD("% Sent", 3, 1, 0);
            for (int j = 1; j < 20; j++) {
              bufferBLE[j] = bufferRead[BLE_buffer_index + j];
              //Serial.println(BLE_buffer_index + j);
            }
            bufferBLE[0] = (int)BLE_buffer_index / 19;
            BLE_buffer_index += 19;

            if (number_of_chunk_sent_BLE == number_of_chunk_BLE - 1 && packets_per_chunk == (int)BLE_buffer_index / 19) {
              Serial.println("Last packet");
              Serial.println(bufferBLE[0]);
              bufferBLE[0] = bufferBLE[0] + 128;
              Serial.println(bufferBLE[0]);
            }
            if (number_of_chunk_sent_BLE == number_of_chunk_BLE - 1 && packets_per_chunk == (int)BLE_buffer_index / 19) {

              Bluefruit.disconnect(Bluefruit.connHandle());
              BLE_connected = false;
              BLE_force_stop = true;
            }
            if (BLE_buffer_index == 1444) {
              number_of_chunk_sent_BLE = number_of_chunk_sent_BLE + 1;
              need_SD_chunk = true;
              BLE_buffer_index = 0;
              //printLCD((String)((int)(100 * number_of_chunk_sent_BLE / number_of_chunk_BLE)), 0, 1, 0);
              //printLCD("% Sent", 3, 1, 0);
            }
          }
        }
      }
      if (!Bluefruit.connected() && BLE_connected) {
        Serial.println("Donso");
        BLE_connected = false;
      }

      break;
    case 3:
      if (!sampling) {
        ITimer.enableTimer();
        sampling = true;
        beepOn(3);
        digitalWrite(ITSY_TESTPULSE_PIN, HIGH);
        
      }
      num = (chunk_num / 3600);
      hours = (String)num;
      if (num < 10) {
        hours = "0" + (String)hours;
      }
      num = ((chunk_num % 3600) / 60);
      minutes = (String)num;
      if (num < 10) {
        minutes = "0" + (String)minutes;
      }
      num = (chunk_num % 60);
      seconds = (String)num;
      if (num < 10) {
        seconds = "0" + (String)seconds;
      }
      LCDtext[0] = "Recording Data";
      //LCDtext[1] = "File" + String(systemSettings.runNumber) + " " + hours + ":" + minutes + ":" + seconds;
      if (PreviousSeconds != seconds){
        printLCD("File" + String(systemSettings.runNumber) + " " + hours + ":" + minutes + ":" + seconds, 0, 1, 0);
        PreviousSeconds = seconds;
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
    chunkToBinary(chunk_to_SD, buffer);
    Serial.println("Chunk to Bin");
  }
}
