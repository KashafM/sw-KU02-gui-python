#include <Arduino.h>
#include <Adafruit_TinyUSB.h> // for Serial
#include <Wire.h>

#define DS1631_ADDRESS  0x48
#define IMU_ADDRESS  0x69

void setup() {
  Serial.begin(115200);
  while (!Serial);
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
  Wire.beginTransmission(IMU_ADDRESS); // Begin data transmission to the MPU-6050 sensor (IMU) with address 0x1101000 (104)
  Wire.write(0x6B);                  // Talk to the register 6B
  Wire.write(0x00);                  // Reset the sensor
  Wire.endTransmission(true);        // End the transmission
  Wire.beginTransmission(IMU_ADDRESS);       // Configure accel. sensitivity
  Wire.write(0x1C);                  // Communicate with the ACCEL_CONFIG register 28 (0x1C)
  Wire.write(0x00);                  // Set the register bits as 00000000 (0x18) (+/- 16g full scale range)
  Wire.endTransmission(true);        // End the transmission
  Wire.beginTransmission(IMU_ADDRESS);       
  Wire.write(0x1B);                  // Communicate with the GYRO_CONFIG register 27 (0x1B)
  Wire.write(0x00);                  // Set the register bits as 00010000 (1000deg/s full scale)
  Wire.endTransmission(true);        // End the transmission
}

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

void loop(){
  delay(1000);

  raw_t = ds1631_temperature();
  c_temp = float(raw_t)/256;
  MPU_accelgyro();
  c_temp_MPU = float(raw_t_MPU)/340.0 + 36.53;

  // Print temp from DS1631+    
  Serial.print("\nTemp DS1631+ = ");
  Serial.print(c_temp);
  Serial.print("\nTemp MPU-6050 = ");
  Serial.print(c_temp_MPU);

  // Print accelerometer and gyroscope readings
  Serial.print("\nACCELEROMETER: ");
  Serial.print("x = ");
  Serial.print(AccX);
  Serial.print(", y = ");
  Serial.print(AccY);
  Serial.print(", z = ");
  Serial.print(AccZ);
  Serial.print("\nGYROSCOPE:");
  Serial.print("x = ");
  Serial.print(GyroX);
  Serial.print(", y = ");
  Serial.print(GyroY);
  Serial.print(", z = ");
  Serial.print(GyroZ);
  Serial.print("\nroll = ");
  Serial.print(roll);
  Serial.print("\npitch = ");
  Serial.print(pitch);
  Serial.print("\nyaw = ");
  Serial.print(yaw);
}

int16_t ds1631_temperature() {
  Wire.beginTransmission(DS1631_ADDRESS); // connect to DS1631 (send DS1631 address)
  Wire.write(0xAA);                       // read temperature command
  Wire.endTransmission(false);            // send repeated start condition
  Wire.requestFrom(DS1631_ADDRESS, 2);    // request 2 bytes from DS1631 and release I2C bus at end of reading

  // calculate full temperature (raw value)
  int16_t raw_t = Wire.read() << 8 | Wire.read();
  return raw_t;
}

void MPU_accelgyro() {
  // === Read acceleromter data === //
  Wire.beginTransmission(IMU_ADDRESS);
  Wire.write(0x3B); // Start with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.requestFrom(IMU_ADDRESS, 6, true); // Read 6 registers total, each axis value is stored in 2 registers
  //For a range of +-2g, we need to divide the raw values by 16384, according to the datasheet
  AccX = (Wire.read() << 8 | Wire.read()) / 16384.0; // X-axis value
  AccY = (Wire.read() << 8 | Wire.read()) / 16384.0; // Y-axis value
  AccZ = (Wire.read() << 8 | Wire.read()) / 16384.0; // Z-axis value
  // Calculating Roll and Pitch from the accelerometer data
  accAngleX = (atan(AccY / sqrt(pow(AccX, 2) + pow(AccZ, 2))) * 180 / PI) - 0.58; // AccErrorX ~(0.58) See the calculate_IMU_error()custom function for more details
  accAngleY = (atan(-1 * AccX / sqrt(pow(AccY, 2) + pow(AccZ, 2))) * 180 / PI) + 1.58; // AccErrorY ~(-1.58)
  // === Read gyroscope data === //
  previousTime = currentTime;        // Previous time is stored before the actual time read
  currentTime = millis();            // Current time actual time read
  elapsedTime = (currentTime - previousTime) / 1000; // Divide by 1000 to get seconds
  Wire.beginTransmission(IMU_ADDRESS);
  Wire.write(0x43); // Gyro data first register address 0x43
  Wire.endTransmission(false);
  Wire.requestFrom(IMU_ADDRESS, 6, true); // Read 4 registers total, each axis value is stored in 2 registers
  GyroX = (Wire.read() << 8 | Wire.read()) / 131.0; // For a 250deg/s range we have to divide first the raw value by 131.0, according to the datasheet
  GyroY = (Wire.read() << 8 | Wire.read()) / 131.0;
  GyroZ = (Wire.read() << 8 | Wire.read()) / 131.0;
  // Currently the raw values are in degrees per seconds, deg/s, so we need to multiply by sendonds (s) to get the angle in degrees
  gyroAngleX = gyroAngleX + GyroX * elapsedTime; // deg/s * s = deg
  gyroAngleY = gyroAngleY + GyroY * elapsedTime;
  yaw =  yaw + GyroZ * elapsedTime;
  // Complementary filter - combine acceleromter and gyro angle values
  roll = 0.96 * gyroAngleX + 0.04 * accAngleX;
  pitch = 0.96 * gyroAngleY + 0.04 * accAngleY;
  // Read temperature of sensor 
  Wire.beginTransmission(IMU_ADDRESS);
  Wire.write(0x41); // Gyro data first register address 0x43
  Wire.endTransmission(false);
  Wire.requestFrom(IMU_ADDRESS, 2, true); // Read 4 registers total, each axis value is stored in 2 registers
  raw_t_MPU = Wire.read() << 8 | Wire.read(); // For a 250deg/s range we have to divide first the raw value by 131.0, according to the datasheet
}
