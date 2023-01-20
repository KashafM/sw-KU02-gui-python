#include <Arduino.h>
#include <Adafruit_TinyUSB.h> // for Serial
#include <Wire.h>

#define DS1631_ADDRESS  0x48

void setup() {
  // put your setup code here, to run once:
  Wire.begin();
  Wire.beginTransmission(DS1631_ADDRESS); //Begin data transmission to the DS1631+ sensor
  Wire.write(0xAC);
  Wire.write(0x02);// Wire.write(0x02);
  Wire.beginTransmission(DS1631_ADDRESS);
  Wire.write(0x51);
  Wire.endTransmission();

}

byte val = 0;
float temp_IMU, temp_DS1631;
char c_buffer[11], f_buffer[11];

void loop() {
  // put your main code here, to run repeatedly:
  
  delay(1000);

  int16_t raw_t = ds1631_temperature();
  float c_temp = float(raw_t)/256;

  // Print temp from DS1631+    
  Serial.print("\n Temp DS1631+ = ");
  Serial.print(raw_t);
  Serial.print("\n = ");
  Serial.print(c_temp);

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
