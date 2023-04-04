




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
  //for (int i = 0; i < 255; i++) {
  //  Serial.print(systemSettings.currentRunNumbers[i]);
    
  //}
  //Serial.println("");
  //for (int i = 0; i < 255; i++) {
  //  Serial.print(systemSettings.BLEDone[i]);  
  //}
  //Serial.println("");
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

void outputLog(){

  file.open(FILENAME, Adafruit_LittleFS_Namespace::FILE_O_READ);
  // file existed
  if (file) {
    Serial.println(FILENAME " output:");

    uint32_t readlen;
    char buffer[256] = { 0 };
    readlen = file.read(buffer, sizeof(buffer));

    buffer[readlen] = 0;
    Serial.println(buffer);
    file.close();
  }
}
void ReadLastLog(){

  file.open(FILENAME, Adafruit_LittleFS_Namespace::FILE_O_READ);
  // file existed
  if (file) {

    uint32_t readlen;
    char buffer[256] = { 0 };
    readlen = file.read(buffer, sizeof(buffer));

    buffer[readlen] = 0;
    Serial.println(buffer[0]);
    file.close();
  }
}

void systemLogger(String str) {
  const int MAX_LINES = 500;
  
  file.open(FILENAME, Adafruit_LittleFS_Namespace::FILE_O_READ | Adafruit_LittleFS_Namespace::FILE_O_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }

  // Get number of lines in the file
  int num_lines = 0;
  while (file.available()) {
    if (file.read() == '\n') {
      num_lines++;
    }
  }

  // If file has reached max lines, remove first line and append new data to last line
  if (num_lines >= MAX_LINES) {
    // Get current file size
    long size = file.size();
    // Find position of first line break
    file.seek(0);
    int first_line_break = file.find('\n');
    // Calculate position of second line
    int second_line_start = first_line_break + 1;
    // Calculate remaining size of file after removing first line
    long remaining_size = size - second_line_start;
    // Allocate buffer for remaining data
    char* remaining_data = new char[remaining_size + 1];
    // Read remaining data from file
    file.seek(second_line_start);
    file.readBytes(remaining_data, remaining_size);
    remaining_data[remaining_size] = '\0';
    // Overwrite file with remaining data
    file.seek(0);
    file.write(remaining_data);
    delete[] remaining_data;
    // Move write position to end of file
    file.seek(size - first_line_break - 1);
  }

  // Write new data to file
  file.print(str);
  file.println();
  file.close();
}