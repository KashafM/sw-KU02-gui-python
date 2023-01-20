#define TIMER_INTERRUPT_DEBUG 0
#define _TIMERINTERRUPT_LOGLEVEL_ 0

#include <Arduino.h>
#include <Adafruit_TinyUSB.h> // for Serial
#include "NRF52TimerInterrupt.h"
#include "NRF52_ISR_Timer.h"
#include <Wire.h>

// Initialize the timer
NRF52Timer ITimer(NRF_TIMER_2);
NRF52_ISR_Timer ISR_Timer;

// Initialize global variables
int adcin1 = A0; // Pin assignment of the first EOG input
int adcin2 = A1; // Pin assignment of the second EOG input
int adcval1 = 0; // Voltage reading from the first EOG input
int adcval2 = 0; // Voltage reading from the first EOG input
int count_samples = 0; // Count the number of IMU samples taken
float mv_per_lsb = 3300.0F/4096.0F; // For converting values with 12-bit ADC with 3.3 V input range
const int IMU = 0x69; // Address of the IMU 0x1101001 (105)
const int TEMP = 0x48; // Address of the temperature sensor 0x1001000 (72)
float AccX, AccY, AccZ; // For saving accelerometer reading values from all three axes
float GyroX, GyroY, GyroZ; // For saving gyroscope reading values from all three axes
float temp_IMU, temp_DS1631;
float sample_time = 0; // Time at which a sample was taken

// Handler for the timer-based interrupt
void TimerHandler(){
  ISR_Timer.run();
}

// Function for ADC of EOG potentials
void SampleEOG(){
  // Obtain the current ADC values from pins A0 and A1
  adcval1 = analogRead(adcin1); //Read and convert the analog value from pin A0
  adcval2 = analogRead(adcin2); //Read and convert the analog value from pin A1
  delay(5); 

  // Print the converted voltages
  sample_time = millis();
  Serial.print("\nTime = ");
  Serial.println(sample_time);
  Serial.print("\n");
  Serial.print("A0 = ");
  Serial.print((float)adcval1 * mv_per_lsb);
  Serial.print(" mV\n");
  Serial.print("A1 = ");
  Serial.print((float)adcval2 * mv_per_lsb);
  Serial.print(" mV\n");
}

// Setup function
void setup() {

  // Serial display setup
  Serial.begin(115200);
  while (!Serial && millis() < 5000) delay(10);   // for nrf52840 with native usb

  // ADC setup
  analogReference(AR_VDD4); //Set the analog reference voltage to VDD (3.3 V)
  analogReadResolution(12); //Set the resolution to 12-bit

  // I2C setup
  Wire.begin();
  // IMU setup
  Wire.beginTransmission(IMU); // Begin data transmission to the MPU-6050 sensor (IMU) with address 0x1101001 (105)
  Wire.write(0x1A);                  // Talk to register 26 (0x1A) - configuration
  Wire.write(0x01);                  // Set the accel. and gyro. filter bandwidths to 184 Hz and 188 Hz, respectively
  Wire.endTransmission(true);        // End the transmission
  Wire.beginTransmission(IMU);       // Configure accel. sensitivity
  Wire.write(0x1C);                  // Communicate with the ACCEL_CONFIG register 28 (0x1C)
  Wire.write(0x18);                  // Set the register bits as 00011000 (+/- 16g full scale range)
  Wire.endTransmission(true);        // End the transmission
  Wire.beginTransmission(IMU);       
  Wire.write(0x1B);                  // Communicate with the GYRO_CONFIG register 27 (0x1B)
  Wire.write(0x10);                  // Set the register bits as 00010000 (1000deg/s full scale)
  Wire.endTransmission(true);        // End the transmission
  // Temperature sensor setup
  Wire.beginTransmission(TEMP); //Begin data transmission to the DS1631+ sensor (temperature) with address 0x1001000 (72)
  Wire.write(0xAC);                  // Talk to register 172 (0xAC) - configuration
  Wire.write(0x02);                  // Set the register bits as 00010000 (1000deg/s full scale)
  Wire.endTransmission(true);        // End the transmission
  Wire.beginTransmission(TEMP); //Begin data transmission to the DS1631+ sensor (temperature) with address 0x1001000 (72)
  Wire.write(0x51);                  // Talk to register 81 (0x51) - start conversions
  Wire.endTransmission(true);        // End the transmission
}

// Main loop
void loop() {

  // Read the IMU values
  // Accelerometer reading
  Wire.beginTransmission(IMU); // Begin data transmission to the MPU-6050 sensor (IMU)
  Wire.write(0x3B); // Start with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(IMU, 6, true); // Read 6 registers, each axis value is stored in 2 registers
  //For a range of +-16g, we need to divide the raw values by 2048, according to the datasheet
  AccX = (Wire.read() << 8 | Wire.read()) / 2048.0; // X-axis accel. value
  AccY = (Wire.read() << 8 | Wire.read()) / 2048.0; // Y-axis accel. value
  AccZ = (Wire.read() << 8 | Wire.read()) / 2048.0; // Z-axis accel. value
  // Gyroscope reading
  Wire.beginTransmission(IMU); // Begin data transmission to the MPU-6050 sensor (IMU)
  Wire.write(0x43); // Gyro. data first register address 0x43
  Wire.endTransmission(false); 
  Wire.requestFrom(IMU, 6, true); // Read 6 registers, each axis value is stored in 2 registers
  GyroX = (Wire.read() << 8 | Wire.read()) / 32.8 ; // X-axis gyro. value. For a 1000 deg/s range we have to divide first the raw value by 32.8, according to the datasheet
  GyroY = (Wire.read() << 8 | Wire.read()) / 32.8 ; // Y-axis gyro. value
  GyroZ = (Wire.read() << 8 | Wire.read()) / 32.8 ; // Z-axis gyro. value
  Wire.endTransmission(true);
  
  // Print the values on the serial monitor
  sample_time = millis();
  Serial.print("\nTime = ");
  Serial.println(sample_time);
  Serial.print("\n");
  Serial.print("AccX = ");
  Serial.print(AccX);
  Serial.print("\n AccY = ");
  Serial.print(AccY);
  Serial.print("\n AccZ = ");
  Serial.print(AccZ);
  Serial.print("\n GyroX = ");
  Serial.print(GyroX);
  Serial.print("\n GyroY = ");
  Serial.print(GyroY);
  Serial.print("\n GyroZ = ");
  Serial.print(GyroZ);

  count_samples = count_samples + 1;

  if (count_samples == 1){
    // Read temp of IMU
    sample_time = millis();
    //Temperature reading
    Wire.beginTransmission(IMU); // Begin data transmission to the MPU-6050 sensor (IMU)
    Wire.write(0x41); // Temp data first register address 0x41
    Wire.endTransmission(false);
    Wire.requestFrom(IMU, 2, true); // Read 2 registers
    temp_IMU = ((Wire.read() << 8 | Wire.read()) / 340.0) + 36.53;
    Wire.endTransmission(true);

    // Print temp of IMU
    Serial.print("\nTime = ");
    Serial.println(sample_time);    
    Serial.print("\n Temp IMU = ");
    Serial.print(temp_IMU);

    // Read temp from temperature sensor
    Wire.beginTransmission(TEMP);
    Wire.write(0xAA); // Temp data first register address 0xAA
    Wire.endTransmission(false);
    Wire.requestFrom(IMU, 2, true); // Read 2 registers
    temp_DS1631 = Wire.read() << 8 | Wire.read();
    Wire.endTransmission(true);

    // Print temp from DS1631+
    Serial.print("\nTime = ");
    Serial.println(sample_time);    
    Serial.print("\n Temp DS1631+ = ");
    Serial.print(temp_DS1631);
    
    count_samples = 0;
  }

  delay(10);
  
}
