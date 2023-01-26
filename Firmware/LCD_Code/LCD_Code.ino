//https://bigdanzblog.wordpress.com/2015/08/26/nhd-c0220biz-fsw-fbw-3v3m-i2c-lcd-display-and-teensy-field-guide/
////////////////////////////////////////////////////////////////////////////////////
// []    i2c LCD library Display Test Demo
// []    Original was 4-2-2009  dale@wentztech.com
// []    DGG 2012-04-11 - Libraries were modified to run under Arduino 1.0
// []    DH  2015-08-26 - Altered for my own testing of NewHaven I2C LCD
////////////////////////////////////////////////////////////////////////////////////

#include                "LCD_C0220BiZ.h"
#include                "ST7036.h"
#include                <Wire.h>

const int               backlightPin        = 23;
const int               cols                = 20;
const int               rows                = 2;

ST7036                  lcd                 (rows, cols, 0x78);

//----------------------------------------------------------------------------
void Cursor_Type(
    ){

lcd.setCursor(0,0);
lcd.print("Underline Cursor");
lcd.setCursor(1,0);
lcd.cursor_on();
delay(1000);

lcd.cursor_off();
lcd.setCursor(0,0);
lcd.print("Block Cursor    ");
lcd.setCursor(1,0);
lcd.blink_on();
delay(1000);

lcd.blink_off();
lcd.setCursor(0,0);
lcd.print("No Cursor      ");
lcd.setCursor(1,0);
delay(1000);

} // Cursor_Type

//----------------------------------------------------------------------------
// Assume 16 character lcd

void Characters(
    ){

char                    a;
int                     chartoprint     = 48;

lcd.clear();

for(int i=0 ; i < rows ; i++){
    for(int j=0 ; j < cols ; j++){
        lcd.setCursor(i,j);
        a = char(chartoprint);
        lcd.print(char(chartoprint));
        chartoprint++;
        if(chartoprint == 127)
            return;
        }
    }
} // Characters

//----------------------------------------------------------------------------
void Every_Line(
    int                 lines
    ){

lcd.clear();
for(int i=0 ; i < lines ; i++){
    lcd.setCursor(i,0);
    lcd.print("Line : ");
    lcd.print(i,DEC);
    }
} // Every_Line

//----------------------------------------------------------------------------
void Every_Pos(
    int                 lines,
    int                 cols
    ){

lcd.clear();

for(int i=0 ; i < lines ; i++){
    for(int j=0 ; j< cols ; j++){
        lcd.setCursor(i,j);
        lcd.print(i,DEC);
        }
    }
} // Every_Pos

//----------------------------------------------------------------------------
void setup(
    ) {

int                     duty;
int                     i;

lcd.init();                                                 // Init the display, clears the display
lcd.clear ();

pinMode(backlightPin, OUTPUT);
digitalWrite(backlightPin, LOW);                            // turn it on (actually already is)

lcd.setCursor(0, 0);
lcd.print("Hello World!");                                  // Classic Hello World!
delay(1000);

// dim display
for (i = 0; i <= 100; i = i + 10) {
    analogWrite(backlightPin, map(i, 0, 100, 255, 0));
    delay(500UL);
    }

// brighten display
for (i = 100; i >= 0; i = i - 10) {
    analogWrite(backlightPin, map(i, 0, 100, 255, 0));
    delay(500UL);
    }

// Reset display to full on
lcd.clear ();
lcd.print("Turn Backlight Off");
pinMode(backlightPin, OUTPUT);                              // pinMode must be called to clear analogWrite
digitalWrite(backlightPin, HIGH);
delay(1000UL);

lcd.clear ();
lcd.print("Turn Backlight On");
digitalWrite(backlightPin, LOW);
delay(1000UL);

}

//----------------------------------------------------------------------------
void loop(
    ){

lcd.clear();
lcd.print ("Cursor Test");
delay(1000);
Cursor_Type();

lcd.clear();
lcd.print("Characters Test");
delay(1000);
Characters();
delay(1000);

lcd.clear();
lcd.print("Every Line");
delay(1000);
Every_Line(rows);
delay(1000);

lcd.clear();
lcd.print("Every Position");
delay(1000);
Every_Pos(rows,cols);
delay(1000);

} // loop
