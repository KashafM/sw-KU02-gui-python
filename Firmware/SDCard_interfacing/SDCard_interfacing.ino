/*
  SD card datalogger

 ** MOSI - pin 24
 ** MISO - pin 23
 ** CLK - pin 25
 ** CS - pin 2

*/

#include <SPI.h>
#include <SD.h>

const int chipSelect = 2;


void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);


  Serial.print("Initializing SD card...");
  SPI.setPins(23, 25, 24);
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    while (1){
      Serial.println("Card failed, or not present");
    }
  }
  Serial.println("card initialized.");
}

void loop() {
  // make a string for assembling the data to log:
  String dataString = "";

  


  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open("datalog.txt", FILE_WRITE);
  dataString = "TESTTTTTTT";
  // if the file is available, write to it:
  if (dataFile) {
    dataFile.println(dataString);
    dataFile.close();
    // print to the serial port too:
    Serial.println(dataString);
  }
  // if the file isn't open, pop up an error:
  else {
    Serial.println("error opening datalog.txt");
  }
}
