


void writeInternal() {
  file.open(FILENAME, Adafruit_LittleFS_Namespace::FILE_O_WRITE);
  // file existed
  if (file) {
    file.write(CONTENTS, strlen(CONTENTS));
    //file.println(CONTENTS);
    Serial.println("Wrote to internal");
    Serial.println();
    file.close();
  }
}

void formatInternal() {
  InternalFS.begin();

  Serial.print("Formating ... ");
  delay(1);  // for message appear on monitor

  // Format
  InternalFS.format();

  Serial.println("Done");
}

void readInternal() {
  file.open(FILENAME, Adafruit_LittleFS_Namespace::FILE_O_READ);
  // file existed
  if (file) {
    Serial.println(FILENAME " file exists");

    uint32_t readlen;
    char buffer[256] = { 0 };
    readlen = file.read(buffer, sizeof(buffer));

    buffer[readlen] = 0;
    Serial.println(buffer);
    file.close();
  }
}

void readSystemSetting() {
  file.open(FILENAME, Adafruit_LittleFS_Namespace::FILE_O_READ);
  // file existed
  if (file) {
    // read the runNumber variable from the first line of the file
    String line = file.readStringUntil('\n');
    systemSettings.runNumber = line.toInt();

    // read the runSize array from the second line of the file
    line = file.readStringUntil('\n');
    int i = 0;
    while (line.length() > 0) {
      int commaIndex = line.indexOf(',');
      if (commaIndex >= 0) {
        systemSettings.currentRunNumbers[i] = line.substring(0, commaIndex).toInt();
        line = line.substring(commaIndex + 1);
      } else {
        systemSettings.currentRunNumbers[i] = line.toInt();
        line = "";
      }
      i++;
    }


    // read the BLEDone array from the third line of the file
    line = file.readStringUntil('\n');
    i = 0;
    while (line.length() > 0) {
      int commaIndex = line.indexOf(',');
      if (commaIndex >= 0) {
        systemSettings.BLEDone[i] = line.substring(0, commaIndex).toInt();
        line = line.substring(commaIndex + 1);
      } else {
        systemSettings.BLEDone[i] = line.toInt();
        line = "";
      }
      i++;
    }
    line = file.readStringUntil('\n');
    systemSettings.BLEIndex = line.toInt();

    file.close();
  }
  Serial.println(systemSettings.runNumber);
  for (int i = 0; i < 255; i++) {
    Serial.print(systemSettings.currentRunNumbers[i]);
    
  }
  Serial.println("");
  for (int i = 0; i < 255; i++) {
    Serial.print(systemSettings.BLEDone[i]);
    
  }
  Serial.println("");
}


void writeSystemSettingsFull(const SystemSettings& settings) {
  file.open(FILENAME, Adafruit_LittleFS_Namespace::FILE_O_READ);

  if (file) {
    // write the runNumber variable to the first line of the file
    file.print(settings.runNumber);
    file.println();

    // write the runSize array to the second line of the file
    for (int i = 0; i < 255; i++) {
      file.print(settings.currentRunNumbers[i]);
      if (i < 254) {
        file.print(",");
      }
    }
    file.println();

    // write the BLEDone array to the third line of the file
    for (int i = 0; i < 255; i++) {
      file.print(settings.BLEDone[i]);
      if (i < 254) {
        file.print(",");
      }
    }
    file.println();
    file.print(settings.BLEIndex);
    file.println();

    file.close();
  }
}