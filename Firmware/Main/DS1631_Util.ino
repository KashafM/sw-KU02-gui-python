#define DS1631_ADDRESS  0x48 // Address of the temperature sensor

String setupDS1631(){
  String error = "";
  // Set up the DS1631+ temperature sensor
  Wire.begin(); // Initialize the I2C communication
  Wire.beginTransmission(DS1631_ADDRESS); // Begin transmitting data to the DS1631+ sensor
  Wire.write(0xAC); // Talk to register 172 (0xAC) - configuration
  Wire.write(0x02); // Set the resolution to 9-bit
  Wire.beginTransmission(DS1631_ADDRESS); // Begin data transmission to the DS1631+ sensor
  Wire.write(0x51); // Talk to register 81 (0x51) to start conversions

  if (Wire.endTransmission() != 0)
  {
    Serial.println("DS1631 Error!");
    return "Temp Error ";
    
  }
  return "";
}

unsigned short ds1631_temperature() {
  Wire.beginTransmission(DS1631_ADDRESS); // Connect to the DS1631 (send DS1631 address)
  Wire.write(0xAA); // Send the read temperature command
  Wire.endTransmission(false); // Send a repeated start bit
  Wire.requestFrom(DS1631_ADDRESS, 2); // Request a read from the two temperature data registers and then release I2C bus
  raw_t = Wire.read() << 8 | Wire.read(); // Calculate the raw temperature value in binary

  if (raw_t > MAX_TEMP || raw_t < MIN_TEMP ){
    error_message = "TEMP ERROR!";
  }

  return raw_t;
}
