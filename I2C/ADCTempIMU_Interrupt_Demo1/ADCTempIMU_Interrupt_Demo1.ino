// Include libraries
#include <Arduino.h>
#include <Adafruit_TinyUSB.h> // for Serial
#include <Wire.h>
#include <MPU6050.h>
#include "NRF52TimerInterrupt.h"
#include "NRF52_ISR_Timer.h"

// Define constants
#define TIMER_INTERRUPT_DEBUG 0
#define HW_TIMER_INTERVAL_MS      1 // Hardware timer period in ms
#define TIMER_INTERVAL_5MS             5L // EOG sampling period in ms (interrupt timer period)
#define DS1631_ADDRESS  0x48 // Address of the temperature sensor
#define IMU_ADDRESS  0x68 // Address of the IMU sensor

// Initialize timer 1 for suse with an interrupt
NRF52Timer ITimer(NRF_TIMER_1);
NRF52_ISR_Timer ISR_Timer;

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
char buf1[64];
char buf2[64];
char buf3[64];
MPU6050 mpu;

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

// Handler function for configuring the interupt service routine linked to timer 1
void TimerHandler()
{
  ISR_Timer.run();
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

  if (sample_count_EOG == 2) {
    total_num_samp_IMU = total_num_samp_IMU + 1;
    sample_count_IMU = sample_count_IMU + 1;
    MPU_accelgyro();
    sample_count_EOG = 0;
  }
  if (sample_count_IMU == 100) {
    total_num_samp_TEMP = total_num_samp_TEMP + 1;
    raw_t = ds1631_temperature();
    c_temp = float(raw_t) / 256;
    c_temp_MPU = float(raw_t_MPU) / 340.0 + 36.53;
    sprintf(buf1, "\nE%d: %.1f %.1f", total_num_samp_EOG, V_A0, V_A1);
    Serial.print(buf1);
    delay(1);
    sprintf(buf2, "\nA%d: %.1f %.1f %.1f\nG%d: %.1f %.1f %.1f", total_num_samp_IMU, AccX, AccY, AccZ, total_num_samp_IMU, GyroX, GyroY, GyroZ);
    Serial.print(buf2);
    delay(1);
    sprintf(buf3, "\nT%d: %.1f %.1f", total_num_samp_TEMP, c_temp, c_temp_MPU);
    Serial.print(buf3);
    delay(1);
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

void setup() {
  // Set up the analog to digital converter
  analogReference(AR_VDD4); // Set the analog reference voltage to VDD (3.3 V)
  analogReadResolution(12); // Set the resolution to 12-bit

  // Set up the serial monitor
  Serial.begin(115200);
  delay(100);

  // Set up the DS1631+ temperature sensor
  Wire.begin(); // Initialize the I2C communication
  Wire.beginTransmission(DS1631_ADDRESS); // Begin transmitting data to the DS1631+ sensor
  Wire.write(0xAC); // Talk to register 172 (0xAC) - configuration
  Wire.write(0x02); // Set the resolution to 9-bit
  Wire.beginTransmission(DS1631_ADDRESS); // Begin data transmission to the DS1631+ sensor
  Wire.write(0x51); // Talk to register 81 (0x51) to start conversions
  Wire.endTransmission(); // End the transmission

  // Set up the MPU-6050 IMU sensor

  while (!mpu.begin(MPU6050_SCALE_250DPS, MPU6050_RANGE_2G))
  {
    Serial.println("Could not find a valid MPU6050 sensor, check wiring!");
    delay(500);
  }
  checkSettings();

  // Set up the timer-driven interrupt
  if (ITimer.attachInterruptInterval(HW_TIMER_INTERVAL_MS * 1000, TimerHandler))
  {
    Serial.print("Starting Timer OK\n");
  }
  else
    Serial.println("Can't set ITimer. Select another freq. or timer");

  ISR_Timer.setInterval(TIMER_INTERVAL_5MS,  SampleAll);
}

void loop() {
}
