#include <MPU6050.h>

#define IMU_ADDRESS  0x68 // Address of the IMU sensor

MPU6050 mpu;

String setupMPU(){
  // Set up the MPU-6050 IMU sensor
  Serial.println("Initializing Inertial Motion Unit...");
  if (!mpu.begin(MPU6050_SCALE_250DPS, MPU6050_RANGE_2G)){
    Serial.println("Could not find a valid MPU6050 sensor, check wiring!");
    return ("IMU Error ");
  }
  else{
    checkSettingsMPU();
  }
  return "";
}

// Function for checking and printing the settings of the MPU-6050 sensor. From the MPU6050 library
void checkSettingsMPU()
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

void MPU_accelgyro() {
  // Sample gyroscope data
  Vector rawAccel = mpu.readRawAccel(); // Read in the raw accelerometer voltages 
  Vector normAccel = mpu.readNormalizeAccel();
  AccX = rawAccel.XAxis;
  AccY = rawAccel.YAxis;
  AccZ = rawAccel.ZAxis;
  //AccX = float(rawAccel.XAxis) / 16384.0;
  //AccY = float(rawAccel.YAxis) / 16384.0;
  //AccZ = float(rawAccel.ZAxis) / 16384.0;
  // Sample gyroscope data
  Vector rawGyro = mpu.readRawGyro(); // Read in the raw gyroscope voltages 
  Vector normGyro = mpu.readNormalizeGyro();
  GyroX = rawGyro.XAxis;
  GyroY = rawGyro.YAxis;
  GyroZ = rawGyro.ZAxis;

  if (GyroX > GyroMax || GyroY > GyroMax || GyroZ > GyroMax){
    sampling_error_message = "MPU ERROR!";
  }
  if (GyroX < GyroMin || GyroY < GyroMin || GyroZ < GyroMin){
    sampling_error_message = "MPU ERROR!";
  }
  
  //GyroX = float(rawGyro.XAxis) / 131.0;
  //GyroY = float(rawGyro.YAxis) / 131.0;
  //GyroZ = float(rawGyro.ZAxis) / 131.0;
  // Read the temperature of the MPU-6050 sensor
  Wire.beginTransmission(IMU_ADDRESS); // Begin transmission to the IMU sensor
  Wire.write(0x41); // Write the address of the first temperature data register (0x41)
  Wire.endTransmission(false); // Send a repeated start bit
  Wire.requestFrom(IMU_ADDRESS, 2, true); // Read from the two temperature data registers and then release I2C bus
  raw_t_MPU = Wire.read() << 8 | Wire.read(); // Calculate the raw temperature value in binary
}
