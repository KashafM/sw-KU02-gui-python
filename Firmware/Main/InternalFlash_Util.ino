


void writeInternal(){
  file.open(FILENAME, Adafruit_LittleFS_Namespace::FILE_O_WRITE);
  // file existed
  if ( file )
  {
    file.write(CONTENTS, strlen(CONTENTS));
    //file.println(CONTENTS);
    Serial.println("Wrote to internal");
    Serial.println();
    file.close();
  }
}

void formatInternal(){
  InternalFS.begin();

  Serial.print("Formating ... ");
  delay(1); // for message appear on monitor

  // Format 
  InternalFS.format();

  Serial.println("Done");
}

void readInternal(){
  file.open(FILENAME, Adafruit_LittleFS_Namespace::FILE_O_READ);
  // file existed
  if ( file )
  {
    Serial.println(FILENAME " file exists");
    
    uint32_t readlen;
    char buffer[256] = { 0 };
    readlen = file.read(buffer, sizeof(buffer));

    buffer[readlen] = 0;
    Serial.println(buffer);
    file.close();
  }
}
