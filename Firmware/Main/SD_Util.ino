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

String SDWrite2(chunk test, String file) {
  Serial.println("SDWrite");
  bool SD_written = false;
  //ITimer.disableTimer();
  // open the file. note that only one file can be open at a time,
  // so you have to close this one before opening another.
  File dataFile = SD.open(file, FILE_WRITE);
  // if the file is available, write to it:
  int x = 107;
  Serial.println(sizeof(test));
  Serial.print(test.run, BIN);
  Serial.print(test.number, BIN);
  Serial.print(test.temp, BIN);

  Serial.println("");
  if (dataFile) {

    dataFile.write((const uint8_t*)&test, sizeof(test));
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

void readChunkFromSDtest() {
  static int position = 0;
  //static byte binFromSD[1412];
  static chunk ugh;

  File dataFile = SD.open("test3.bin", FILE_READ);
  if (!dataFile) {
    Serial.println("Error opening data.bin");
  }

  //dataFile.seek(position * sizeof(ugh));
  dataFile.read((uint8_t*)&ugh.run, sizeof(ugh.run));
  dataFile.read((uint8_t*)&ugh.number, sizeof(ugh.number));
  dataFile.read((uint8_t*)&ugh.temp, sizeof(ugh.temp));
  dataFile.read((uint8_t*)&ugh.EOG1, sizeof(ugh.EOG1));
  dataFile.read((uint8_t*)&ugh.EOG2, sizeof(ugh.EOG2));
  dataFile.read((uint8_t*)&ugh.GyroX, sizeof(ugh.GyroX));
  dataFile.read((uint8_t*)&ugh.GyroY, sizeof(ugh.GyroY));
  dataFile.read((uint8_t*)&ugh.GyroZ, sizeof(ugh.GyroZ));
  dataFile.read((uint8_t*)&ugh.done_sampling, sizeof(ugh.done_sampling));
  //Serial.println(binFromSD);
  Serial.print(position);
  Serial.print(": ");

  Serial.print(ugh.run, BIN);
  Serial.print(ugh.number, BIN);
  Serial.print(ugh.temp, BIN);



  //Serial.print(",");

  Serial.println("");
  position++;
  dataFile.close();
}

void SDRead(String file_read) {
  File myFile = SD.open(file_read);
  if (myFile) {
    //Serial.println(file_read);

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


void SDReadChunk(String file_read) {
  File myFile = SD.open(file_read);
  int lineNum = 0;
  if (myFile) {
    //Serial.println(file_read);

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
      chunk_from_SD.temp = value;
      line.remove(0, commaIndex + 1);
      commaIndex = line.indexOf(',');
      if (commaIndex == -1) continue;

      // Read in EOG values from written chunk
      for (int i = 0; i < 400; i++) {
        int endIndex = line.indexOf(',');
        if (endIndex == -1) endIndex = line.length();
        sscanf(line.substring(0, commaIndex).c_str(), "%12u", &value);

        if (i % 2 == 0) {
          chunk_from_SD.EOG1[i] = value;
          line.remove(0, endIndex + 1);
        } else {
          chunk_from_SD.EOG2[i] = value;
          line.remove(0, endIndex + 1);
        }
      }

      // Read in IMU values from written chunk
      for (int i = 0; i < 100; i++) {
        for (int j = 0; j < 3; i++) {
          int endIndex = line.indexOf(',');
          if (endIndex == -1) endIndex = line.length();
          sscanf(line.substring(0, commaIndex).c_str(), "%16u", &value);
          if (j == 0) {
            chunk_from_SD.GyroX[i] = value;
            line.remove(0, endIndex + 1);
          }
          if (j == 1) {
            chunk_from_SD.GyroY[i] = value;
            line.remove(0, endIndex + 1);
          }
          if (j == 2) {
            chunk_from_SD.GyroZ[i] = value;
            line.remove(0, endIndex + 1);
          }
        }
      }
      lineNum++;
      //Serial.println(stringifyChunk(chunk_from_SD));
    }
    // close the file:
    myFile.close();

  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening");
  }
}
void readChunkBinary(byte* buffer){
  
  //Serial.println("DATA_"+(String)fileChoice+".bin");
  File dataFile = SD.open("DATA_"+(String)fileChoice+".bin", FILE_READ);

  currentFileSize = dataFile.size();
  //Serial.print("File Size: ");
  //Serial.println(dataFile.size());
  number_of_chunk_BLE = (int)currentFileSize/1407;
  if (dataFile) {
    dataFile.seek(read_position * 1407);
    for (int i = 0; i < 1407; i++) {
      dataFile.read((uint8_t*)&buffer[i], sizeof(buffer[i])); 
      
    }
    //for (int i = 0; i < 1407; i++) {
    //  for (int j = 7; j >= 0; j--) {
    //    Serial.print((buffer[i] >> j) & 1);
    //  }
    //  
    //}
    read_position = read_position + 1;
  }
  else{
    Serial.println("Couldnt open file");
  }
  dataFile.close();
}

void chunkToBinary(chunk input, byte* buffer) {
  // Write the "run" byte to the buffer
  
  
  buffer[0] = input.run;

  // Write the "number" unsigned int to the buffer
  buffer[1] = (input.number >> 24) & 0xFF;
  buffer[2] = (input.number >> 16) & 0xFF;
  buffer[3] = (input.number >> 8) & 0xFF;
  buffer[4] = input.number & 0xFF;

  // Write the "temp" unsigned short to the buffer
  buffer[5] = (input.temp >> 8) & 0xFF;
  buffer[6] = input.temp & 0xFF;

  // Write the "EOG1" array of unsigned short to the buffer
  for (int i = 0; i < 200; i++) {
    buffer[7 + (i * 2)] = (input.EOG1[i] >> 8) & 0xFF;
    buffer[8 + (i * 2)] = input.EOG1[i] & 0xFF;
  }

  // Write the "EOG2" array of unsigned short to the buffer
  for (int i = 0; i < 200; i++) {
    buffer[407 + (i * 2)] = (input.EOG2[i] >> 8) & 0xFF;
    buffer[408 + (i * 2)] = input.EOG2[i] & 0xFF;
  }

  // Write the "GyroX" array of unsigned short to the buffer
  for (int i = 0; i < 100; i++) {
    buffer[807 + (i * 2)] = (input.GyroX[i] >> 8) & 0xFF;
    buffer[808 + (i * 2)] = input.GyroX[i] & 0xFF;
  }

  // Write the "GyroY" array of unsigned short to the buffer
  for (int i = 0; i < 100; i++) {
    buffer[1007 + (i * 2)] = (input.GyroY[i] >> 8) & 0xFF;
    buffer[1008 + (i * 2)] = input.GyroY[i] & 0xFF;
  }

  // Write the "GyroZ" array of unsigned short to the buffer
  for (int i = 0; i < 100; i++) {
    buffer[1207 + (i * 2)] = (input.GyroZ[i] >> 8) & 0xFF;
    buffer[1208 + (i * 2)] = input.GyroZ[i] & 0xFF;
  }

  // Write the "done_sampling" bool to the buffer
  //buffer[1407] = input.done_sampling;
  //Serial.println("Write:");
  String fileName = "DATA_" + String(systemSettings.runNumber, 10) + ".bin";
  //fileName = fileName.substring(0, fileName.length() - 4) + String("000" + String(systemSettings.runNumber, 10)).substring(String(systemSettings.runNumber, 10).length()) + ".bin";
  //Serial.println(fileName);
  File dataFile = SD.open(fileName, FILE_WRITE | O_CREAT);

  if (dataFile) {
    //Serial.println(sizeof(*buffer));
    for (int i = 0; i < 1407; i++) {
      //for (int j = 7; j >= 0; j--) {
      //  Serial.print((buffer[i] >> j) & 1);
      //}
      dataFile.write((const uint8_t*)&buffer[i], sizeof(buffer[i]));
    }
  }
  else{
    Serial.print("Data Save error: couldn't open file: ");
    Serial.println(fileName);
  }
  dataFile.close();
 

}

void DetermineRunNumber() {
  //int currentRunNumbers[255] = {0}; // Initialize all values to 0
  int highestRunNumber = 0;
  File dir = SD.open("/"); // Open the root directory

  while (true) {
    File entry = dir.openNextFile(); // Open the next file in the directory
    if (!entry) { // If there are no more files, break out of the loop
      break;
    }

    String fileName = entry.name(); // Get the name of the file
    //Serial.println(fileName);
    if (fileName.startsWith("DATA_")) {
      // Get the digits following "DATA_" to determine the index
      int startIndex = fileName.indexOf("_") + 1;
      int endIndex = fileName.indexOf(".", startIndex);
      if (endIndex < 0) { // If there is no file extension, assume the end of the string
        endIndex = fileName.length();
      }
      String indexString = fileName.substring(startIndex, startIndex+3);
      int index = indexString.toInt();

      // Update the currentRunNumbers array and the highestRunNumber variable
      systemSettings.currentRunNumbers[index] = 1;
      if (index > highestRunNumber) {
        highestRunNumber = index;
      }
    }

    entry.close(); // Close the file
  }

  dir.close(); // Close the directory

  // Find the first unused run number and set it as the current run number
  for (int i = 1; i <= highestRunNumber + 1; i++) {
    if (systemSettings.currentRunNumbers[i] == 0) {
      // Set the current run number to i and break out of the loop
      systemSettings.runNumber = i;
      break;
    }
  }
  //Serial.println(systemSettings.runNumber);
}