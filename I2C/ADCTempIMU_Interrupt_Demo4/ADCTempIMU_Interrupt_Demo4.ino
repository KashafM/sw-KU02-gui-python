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
int print_setting = 2;
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
float error_Ax = 0;
float error_Ay = 0;
float error_Az = 0;
float error_Gx = 0;
float error_Gy = 0;
float error_Gz = 0;
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
  if (print_setting == 0) {
    sprintf(buf1, "\nE%d: %.1f %.1f", total_num_samp_EOG, V_A0, V_A1);
    Serial.print(buf1);
  }
  if (sample_count_EOG == 2) {
    total_num_samp_IMU = total_num_samp_IMU + 1;
    sample_count_IMU = sample_count_IMU + 1;
    MPU_accelgyro();
    if (print_setting == 1) {
      sprintf(buf2, "\nA%d: %.1f %.1f %.1f\nG%d: %.1f %.1f %.1f", total_num_samp_IMU, AccX, AccY, AccZ, total_num_samp_IMU, GyroX, GyroY, GyroZ);
      Serial.print(buf2);
    }
    sample_count_EOG = 0;
  }
  if (sample_count_IMU == 100) {
    total_num_samp_TEMP = total_num_samp_TEMP + 1;
    raw_t = ds1631_temperature();
    c_temp = float(raw_t) / 256;
    c_temp_MPU = float(raw_t_MPU) / 340.0 + 36.53;
    if (print_setting == 2) {
      sprintf(buf3, "\nT%d: %.1f %.1f", total_num_samp_TEMP, c_temp, c_temp_MPU);
      Serial.print(buf3);
    }
    sample_count_IMU = 0;
  }
}

void MPU_accelgyro() {
  // Sample gyroscope data
  Vector rawAccel = mpu.readRawAccel(); // Read in the raw accelerometer voltages
  //Vector normAccel = mpu.readNormalizeAccel();
  AccX = ConverttoSigned(rawAccel.XAxis)/16384.0 - error_Ax;
  AccY = ConverttoSigned(rawAccel.YAxis)/16384.0 - error_Ay;
  AccZ = ConverttoSigned(rawAccel.ZAxis)/16384.0 - error_Az;
  // Sample gyroscope data
  Vector rawGyro = mpu.readRawGyro(); // Read in the raw gyroscope voltages
  //Vector normGyro = mpu.readNormalizeGyro();
  GyroX = ConverttoSigned(rawGyro.XAxis)/131.0 - error_Gx;
  GyroY = ConverttoSigned(rawGyro.YAxis)/131.0 - error_Gy;
  GyroZ = ConverttoSigned(rawGyro.ZAxis)/131.0 - error_Gz;
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

void calibrate_IMU() {

  int samp_count_cal = 0;

  Serial.print("IMU calibration in progress. Place the breadboard flat on a table. Do not move the breadboard.");

  // We can call this funtion in the setup section to calculate the accelerometer and gyro data error. From here we will get the error values used in the above equations printed on the Serial Monitor.
  // Note that we should place the IMU flat in order to get the proper values, so that we then can the correct values
  // Read accelerometer values 200 times
  while (samp_count_cal < 200) {
    Vector rawAccel = mpu.readRawAccel();
    AccX = ConverttoSigned(rawAccel.XAxis)/16384.0;
    AccY = ConverttoSigned(rawAccel.YAxis)/16384.0;
    AccZ = ConverttoSigned(rawAccel.ZAxis)/16384.0;
    // Sum all readings
    error_Ax = error_Ax + (AccX); //; ((atan((AccY) / sqrt(pow((AccX), 2) + pow((AccZ), 2))) * 180 / PI));
    error_Ay = error_Ay + (AccY); // ((atan(-1 * (AccX) / sqrt(pow((AccY), 2) + pow((AccZ), 2))) * 180 / PI));
    error_Az = error_Az + (AccZ);
    delay(5);
    samp_count_cal++;
  }
  //Divide the sum by 200 to get the error value
  error_Ax = error_Ax / 200;
  error_Ay = error_Ay / 200;
  error_Az = (error_Az / 200) - 1;

  samp_count_cal = 0;
  mpu.calibrateGyro();
  // Read gyro values 200 times
  while (samp_count_cal < 200) {
    Vector rawGyro = mpu.readRawGyro(); // Read in the raw gyroscope voltages
    GyroX = ConverttoSigned(rawGyro.XAxis)/131.0;
    GyroY = ConverttoSigned(rawGyro.YAxis)/131.0;
    GyroZ = ConverttoSigned(rawGyro.ZAxis)/131.0;
    // Sum all readings
    error_Gx = error_Gx + (GyroX);
    error_Gy = error_Gy + (GyroY);
    error_Gz = error_Gz + (GyroZ);
    delay(5);
    samp_count_cal++;
  }
  //Divide the sum by 200 to get the error value
  error_Gx = error_Gx / 200;
  error_Gy = error_Gy / 200;
  error_Gz = error_Gz / 200;
  // Print the error values on the Serial Monitor
  Serial.print("\nIMU calibration complete. Sampling starting.");
  Serial.print("\nError Ax: ");
  Serial.print(error_Ax);
  Serial.print("\nError Ay: ");
  Serial.print(error_Ay);
  Serial.print("\nError Az: ");
  Serial.print(error_Az);
  Serial.print("\nError Gx: ");
  Serial.print(error_Gx);
  Serial.print("\nError Gy: ");
  Serial.print(error_Gy);
  Serial.print("\nError Gz: ");
  Serial.print(error_Gz);
  delay(1000);
}

float ConverttoSigned(float x){
  uint16_t x_uint;
  float x_sign;
  if (x > 32767) {
    //printf("\n\nConverting:\n%.0f", x);
    x = x - 1;
    //printf("\nSub1:\n%.0f", x);
    x_uint = (uint16_t)x;
    //printf("\nuint:\n%d", x_uint);
    x_uint = ~x_uint;
    //printf("\nInv:\n%d", x_uint);
    x_sign = -1.0 * (int)x_uint;
    //printf("\nSigned:\n%f", x_sign);
  }
  else {
    x_sign = x;
  }
  return x_sign;
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
  delay(5000);
  while (!mpu.begin(MPU6050_SCALE_250DPS, MPU6050_RANGE_2G))
  {
    Serial.println("Could not find a valid MPU6050 sensor, check wiring!");
    delay(500);
  }
  checkSettings();
  calibrate_IMU();


  // Set up the timer-driven interrupt
  if (ITimer.attachInterruptInterval(HW_TIMER_INTERVAL_MS * 1000, TimerHandler))
  {
    Serial.print("\nTimer started\n");
  }
  else
    Serial.println("Can't set ITimer. Select another freq. or timer");

  ISR_Timer.setInterval(TIMER_INTERVAL_5MS,  SampleAll);
}

void loop() {
}
