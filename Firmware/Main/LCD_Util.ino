bool LCDsetup() {
  bool LCD_good = true;

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
    LCD_good = false;
    Serial.println("LCD Error!");
  }
  return LCD_good;
}

