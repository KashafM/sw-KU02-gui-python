/**************************************
TO DO: You can also append data to an existing SD card file with the appropriate SD.open() command so it is not necessary to read the whole SD card into memory to add to it.
https://forum.arduino.cc/t/using-binary-format-to-store-for-an-sd-card/525854
**************************************/


String setupSD() {
  Serial.print("Initializing SD card...");
  SPI.setPins(23, 25, 24);
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    return ("SD Error");
  } else {
    Serial.println("card initialized.");
  }
  return ("");
}

String SDWrite(String text, String file) {
  Serial.println("SDWrite");
  bool SD_written = false;
  //ITimer.disableTimer();
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open(file, FILE_WRITE);
  // if the file is available, write to it:
  if (dataFile) {

    dataFile.println(text);
    dataFile.close();
    SD_written = true;
    // print to the serial port too:

    //Serial.println(text);

  }
  // if the file isn't open, error:
  else {
    Serial.println("error opening specified file");
    return ("SD Error ");
  }
  return ("");
}

void SDRead(String file_read) {
  File myFile = SD.open(file_read);
  if (myFile) {
    Serial.println(file_read);

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening");
  }
}

/*
void SDReadChunk(String file_read) {
  File myFile = SD.open(file_read);
  int lineNum = 0;
  Serial.println("1");
  if (myFile) {
    Serial.println(file_read);

    while (myFile.available()) {
      String line = myFile.readStringUntil('\n');
      line.trim();
      int commaIndex = line.indexOf(',');
      if (commaIndex == -1) continue;  // skip lines without a comma

      // Read in Run number from written chunk
      chunk_from_SD.run = (byte)line.substring(0, commaIndex).toInt();
      line.remove(0, commaIndex + 1);
      commaIndex = line.indexOf(',');
      if (commaIndex == -1) continue;

      // Read in sample number from written chunk
      chunk_from_SD.number = (unsigned int)line.substring(0, commaIndex).toInt();
      line.remove(0, commaIndex + 1);
      commaIndex = line.indexOf(',');
      if (commaIndex == -1) continue;

      // Read in Temp values from written chunk
      //data.temp.x = (bit_9)line.substring(0, commaIndex);
      unsigned int value;
      sscanf(line.substring(0, commaIndex).c_str(), "%9u", &value);
      chunk_from_SD.temp.x = value;
      line.remove(0, commaIndex + 1);
      commaIndex = line.indexOf(',');
      if (commaIndex == -1) continue;

      // Read in EOG values from written chunk
      for (int i = 0; i < 400; i++) {
        int endIndex = line.indexOf(',');
        if (endIndex == -1) endIndex = line.length();
        sscanf(line.substring(0, commaIndex).c_str(), "%12u", &value);

        if (i%2 == 0){
          chunk_from_SD.EOG1[i].x = value;
          line.remove(0, endIndex + 1);
        } 
        else{
          chunk_from_SD.EOG2[i].x = value;
          line.remove(0, endIndex + 1);
        } 
      }

      // Read in IMU values from written chunk
      for (int i = 0; i < 100; i++) {
        for (int j = 0; j < 3; i++) {
          int endIndex = line.indexOf(',');
          if (endIndex == -1) endIndex = line.length();
          sscanf(line.substring(0, commaIndex).c_str(), "%16u", &value);
          if (j == 0){
            chunk_from_SD.GyroX[i].x = value;
            line.remove(0, endIndex + 1);
          }
          if (j == 1){
            chunk_from_SD.GyroY[i].x = value;
            line.remove(0, endIndex + 1);
          }
          if (j == 2){
            chunk_from_SD.GyroZ[i].x = value;
            line.remove(0, endIndex + 1);
          }
        }
      }
      lineNum++;
      Serial.println(stringifyChunk(chunk_from_SD));
    }
    // close the file:
    myFile.close();
    
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening");
  }
}
*/

