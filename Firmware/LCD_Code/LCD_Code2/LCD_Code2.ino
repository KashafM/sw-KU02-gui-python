#include <LCD.h>
#include <ST7036.h>

#define TIMER_INTERRUPT_DEBUG 0
#define _TIMERINTERRUPT_LOGLEVEL_ 0

#include <Arduino.h>
#include <Adafruit_TinyUSB.h> // for Serial
#include <Wire.h>


const int LCD = 0x78; // Address of the IMU 0x1101001 (105)
/****************************************************
  Initialization For ST7036i
*****************************************************/
void init_LCD()
{
  I2C_Start();
  I2C_out(0x78);
  I2C_out(0x00);//Comsend = 0x00
  I2C_out(0x38);
  delay(10);
  Wire.write(0x39);
  delay(10);
  I2C_out(0x14);
  I2C_out(0x78);
  I2C_out(0x5E);
  I2C_out(0x6D);
  I2C_out(0x0C);
  I2C_out(0x01);
  I2C_out(0x06);
  delay(10);
  I2C_Stop();
}
void I2C_Stop(void)
{
  digitalWrite(21, LOW); // SDA = pin 21, SCL = pin 22
  digitalWrite(22, LOW); // SDA = pin 21, SCL = pin 22
  digitalWrite(22, HIGH); // SDA = pin 21, SCL = pin 22
  digitalWrite(21, HIGH); // SDA = pin 21, SCL = pin 22
}
/**
   /***************************************************
  I2C Start
*****************************************************/
void I2C_Start(void)
{
  digitalWrite(22, HIGH); // SDA = pin 21, SCL = pin 22
  digitalWrite(21, HIGH); // SDA = pin 21, SCL = pin 22
  digitalWrite(21, LOW); // SDA = pin 21, SCL = pin 22
  digitalWrite(22, LOW); // SDA = pin 21, SCL = pin 22
}
/*****************************************************/

/****************************************************
  Send string of ASCII data to LCD
*****************************************************/
void Show(unsigned char *text)
{
  int n, d;
  d = 0x00;
  I2C_Start();
  I2C_out(0x78);
  I2C_out(0x40);//Datasend=0x40
  for (n = 0; n < 20; n++) {
    I2C_out(*text);
    ++text;
  }
  I2C_Stop();
}
/*****************************************************/
/****************************************************
  Output command or data via I2C
*****************************************************/
void I2C_out(unsigned char j) //I2C Output
{
  int n;
  unsigned char d;
  d = j;
  for (n = 0; n < 8; n++) {
    if ((d & 0x80) == 0x80)
      digitalWrite(21, HIGH); // SDA = pin 21, SCL = pin 22
    else
      digitalWrite(21, LOW); // SDA = pin 21, SCL = pin 22
    d = (d << 1);
    digitalWrite(22, LOW); // SDA = pin 21, SCL = pin 22
    digitalWrite(22, HIGH);
    digitalWrite(22, LOW);
  }
  digitalWrite(22, HIGH);
  while (digitalRead(21) == HIGH) {
    digitalWrite(22, LOW);
    digitalWrite(22, HIGH);
  }
  digitalWrite(22, LOW);
}
/*****************************************************/

// Setup function
void setup() {
  init_LCD();
}

// Main loop
void loop() {
  unsigned char ta[] = "123";
  unsigned char* t = ta;
  Show(ta);

}
