#include <Arduino.h>
#include <Adafruit_TinyUSB.h> // for Serial
#include <Wire.h>
#include <MPU6050.h>
#include "NRF52TimerInterrupt.h"
#include "NRF52_ISR_Timer.h"

#define TIMER_INTERRUPT_DEBUG 0
#define HW_TIMER_INTERVAL_MS      1
#define TIMER_INTERVAL_5MS             5L
#define DS1631_ADDRESS  0x48
#define IMU_ADDRESS  0x69

// Initialize timer 1
NRF52Timer ITimer(NRF_TIMER_1);
NRF52_ISR_Timer ISR_Timer;
int adcin1 = A0;
int adcin2 = A1;
float adcval1 = 0;
float adcval2 = 0;
float mv_per_lsb = 3300.0F / 4096.0F; // 12-bit ADC with 3.3 V input range
float time = 0;
int sample_num = 0;
int sample_count_EOG = 0;
int sample_count_IMU = 0;
int total_num_samp_EOG = 0;
int total_num_samp_IMU = 0;
int total_num_samp_TEMP = 0;
float c_temp = 0;
float c_temp_MPU = 0;
int16_t raw_t = 0;
int16_t raw_t_MPU = 0;
float AccX;
float AccY;
float AccZ;
float GyroX, GyroY, GyroZ;
float accAngleX, accAngleY, gyroAngleX, gyroAngleY, gyroAngleZ;
float roll, pitch, yaw;
float AccErrorX, AccErrorY, GyroErrorX, GyroErrorY, GyroErrorZ;
float elapsedTime, currentTime, previousTime;
int c = 0;
char buf[64];
MPU6050 mpu;

void setup() {
  // Set up the analog to digital converter
  analogReference(AR_VDD4); //Set the analog reference voltage to VDD (3.3 V)
  analogReadResolution(12); //Set the resolution to 12-bit

  // Set up the serial monitor
  Serial.begin(115200);
  delay(100);

  // Set up the DS1631+ temperature sensor
  Wire.begin(); // Initialize I2C communication
  Wire.beginTransmission(DS1631_ADDRESS); //Begin data transmission to the DS1631+ sensor
  Wire.write(0xAC); // Talk to register 172 (0xAC) - configuration
  Wire.write(0x02); // Set the resolution to 9-bit
  Wire.beginTransmission(DS1631_ADDRESS); //Begin data transmission to the DS1631+ sensor
  Wire.write(0x51); // Talk to register 81 (0x51) - start conversions
  Wire.endTransmission(); // End the transmission

  // Set up the MPU-6050 IMU sensor

  while(!mpu.begin(MPU6050_SCALE_250DPS, MPU6050_RANGE_2G))
  {
    Serial.println("Could not find a valid MPU6050 sensor, check wiring!");
    delay(500);
  }
  checkSettings();
//  Wire.beginTransmission(IMU_ADDRESS); // Begin data transmission to the MPU-6050 sensor (IMU) with address 0x1101000 (104)
//  Wire.write(0x6B);                  // Talk to the register 6B
//  Wire.write(0x00);                  // Reset the sensor
//  Wire.endTransmission(true);        // End the transmission
//  Wire.beginTransmission(IMU_ADDRESS);       // Configure accel. sensitivity
//  Wire.write(0x1C);                  // Communicate with the ACCEL_CONFIG register 28 (0x1C)
//  Wire.write(0x00);                  // Set the register bits as 00000000 (0x18) (+/- 16g full scale range)
//  Wire.endTransmission(true);        // End the transmission
//  Wire.beginTransmission(IMU_ADDRESS);
//  Wire.write(0x1B);                  // Communicate with the GYRO_CONFIG register 27 (0x1B)
//  Wire.write(0x00);                  // Set the register bits as 00010000 (1000deg/s full scale)
//  Wire.endTransmission(true);        // End the transmission

  if (ITimer.attachInterruptInterval(HW_TIMER_INTERVAL_MS * 1000, TimerHandler))
  {
    Serial.print("Starting Timer OK\n");
  }
  else
    Serial.println("Can't set ITimer. Select another freq. or timer");

  ISR_Timer.setInterval(TIMER_INTERVAL_5MS,  SampleAll);
}

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

void SampleEOG() {
  // Obtain the current ADC values from pins A0 and A1
  adcval1 = analogRead(adcin1); //Read and convert the analog value from pin A0
  adcval2 = analogRead(adcin2); //Read and convert the analog value from pin A1
}

void SampleAll() {

  SampleEOG();
  sample_count_EOG = sample_count_EOG + 1;
  total_num_samp_EOG = total_num_samp_EOG + 1;
  sprintf(buf, "\nEOG %d: A0 = %f, A1 = %f", total_num_samp_EOG, adcval1, adcval2);
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
  // === Read acceleromter data === //
  Vector rawAccel = mpu.readRawAccel();
  Vector normAccel = mpu.readNormalizeAccel();
//  AccX = float(rawAccel.XAxis);
//  AccY = float(rawAccel.YAxis);
//  AccZ = float(rawAccel.ZAxis);
    AccX = float(normAccel.XAxis);
    AccY = float(normAccel.YAxis);
    AccZ = float(normAccel.ZAxis);
  // === Read gyroscope data === //
  Vector rawGyro = mpu.readRawGyro();
  Vector normGyro = mpu.readNormalizeGyro();
//  GyroX = float(rawGyro.XAxis);
//  GyroY = float(rawGyro.YAxis);
//  GyroZ = float(rawGyro.ZAxis);
  GyroX = float(normGyro.XAxis);
  GyroY = float(normGyro.YAxis);
  GyroZ = float(normGyro.ZAxis);
  // Read temperature of sensor
  Wire.beginTransmission(IMU_ADDRESS);
  Wire.write(0x41); // Temp data first register address 0x41
  Wire.endTransmission(false);
  Wire.requestFrom(IMU_ADDRESS, 2, true); // Read 4 registers total, each axis value is stored in 2 registers
  // Calculate full temperature (raw value)
  raw_t_MPU = Wire.read() << 8 | Wire.read(); // For a 250deg/s range we have to divide first the raw value by 131.0, according to the datasheet
  //float temp = mpu.readTemperature();
}

int16_t ds1631_temperature() {
  Wire.beginTransmission(DS1631_ADDRESS); // connect to DS1631 (send DS1631 address)
  Wire.write(0xAA);                       // read temperature command
  Wire.endTransmission(false);            // send repeated start condition
  Wire.requestFrom(DS1631_ADDRESS, 2);    // request 2 bytes from DS1631 and release I2C bus at end of reading
  // Calculate full temperature (raw value)
  int16_t raw_t = Wire.read() << 8 | Wire.read();
  return raw_t;
}

void loop() {
}
