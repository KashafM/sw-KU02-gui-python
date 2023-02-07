bool setupSD() {
  bool card_good = true;
  Serial.print("Initializing SD card...");
  SPI.setPins(23, 25, 24);
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    card_good = false;
  }
  else {
    Serial.println("card initialized.");
  }
  return card_good;
}

void SDWrite(String text, String file) {
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open(file, FILE_WRITE);
  // if the file is available, write to it:
  Serial.print("I mad eit here");
  if (dataFile) {
    Serial.print("I mad it here");
    dataFile.println(text);
    dataFile.close();
    // print to the serial port too:
    Serial.println("Written to SD: ");
    Serial.println(text);
  }
  // if the file isn't open, error:
  else {
    Serial.println("error opening specified file");
  }
}
/*
void SDWriteChunk(chunk data, String file) {
  String buf;
  buf += (String)data.temp;
  for(int i = 0; i < sizeof(data.EOG1); i++) 
  {
    buf += (String)data.EOG1[i];
  }


  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open(file, FILE_WRITE);
  // if the file is available, write to it:
  if (dataFile) {
    //dataFile.write(buf)
    dataFile.println(buf);
    dataFile.close();
    // print to the serial port too:
    Serial.println("Written to SD: ");
    Serial.println(buf);
  }
  // if the file isn't open, error:
  else {
    Serial.println("error opening specified file");
  }
}
*/
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
