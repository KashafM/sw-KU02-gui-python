String LCDsetup() {
  lcd.init();
  lcd.clear();
  lcd.backlight();  // Make sure backlight is on
  // Print a message on both lines of the LCD.
  lcd.setCursor(2, 0);  //Set cursor to character 2 on line 0
  lcd.print("Hello world!");
  //lcd.availableForWrite()
  Wire.beginTransmission(39);

  if (Wire.endTransmission() != 0)
  {
    Serial.println("LCD Error!");
    return "LCD Error ";
  }
  return "";
}

