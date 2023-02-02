// Function for sampling and converting the voltages at pins A0 and A1 (EOG)
void SampleEOG() {
  // Obtain the current ADC values from pins A0 and A1
  adcval1 = analogRead(adcin1); //Read and convert the analog value from pin A0
  adcval2 = analogRead(adcin2); //Read and convert the analog value from pin A1
}

// ISR function for sampling EOG, temperature, and accelerometer data with the following sampling periods: 5 ms, 10 ms, and 1 ms
void SampleAll() {
  SampleEOG(); 
  current_chunk_sample.EOG1[total_chunk_samp] = adcval1;
  current_chunk_sample.EOG2[total_chunk_samp] = adcval2;
  total_chunk_samp = total_chunk_samp +1;
  sample_IMU = sample_IMU + 1;

  if (sample_IMU == 2) {
    total_num_samp_IMU = total_num_samp_IMU + 1;
    MPU_accelgyro();
    //current_chunk_sample.AccX[sample_count_IMU] = AccX;
    //current_chunk_sample.AccY[sample_count_IMU] = AccY;
    //current_chunk_sample.AccZ[sample_count_IMU] = AccZ;
    current_chunk_sample.GyroX[sample_count_IMU] = GyroX;
    current_chunk_sample.GyroY[sample_count_IMU] = GyroY;
    current_chunk_sample.GyroZ[sample_count_IMU] = GyroZ;
    Serial.println(total_chunk_samp);
    sample_IMU = 0;
    sample_count_IMU = sample_count_IMU + 1;
  }
  if (sample_count_IMU == 100) {
    total_num_samp_TEMP = total_num_samp_TEMP + 1;
    raw_t = ds1631_temperature();
    current_chunk_sample.temp = raw_t;
    c_temp = float(raw_t) / 256;
    c_temp_MPU = float(raw_t_MPU) / 340.0 + 36.53;
    sample_count_IMU = 0;
    total_chunk_samp = 0;
    current_chunk_sample.done_sampling = true;
  }
  if (current_chunk_sample.done_sampling){
    chunks.push(current_chunk_sample);
    //Serial.println(current_chunk_sample.EOG2[10]);   
    current_chunk_sample.done_sampling = false;
    sample_IMU = 0;
  } 
}

String stringifyChunk(chunk data_chunk){
  String output = "";
  output = output + data_chunk.header;
  output = output + (String) data_chunk.temp;
  Serial.println("Starting");
  for (int i = 0; i<200; i++){
    output = output + (String) data_chunk.EOG1[i];
    output = output + (String) data_chunk.EOG2[i];
  }
  for (int i = 0; i<100; i++){
      //output = output + (String)data_chunk.AccX[i/2] + (String)data_chunk.AccY[i/2] + (String)data_chunk.AccZ[i/2];
      output = output + (String)data_chunk.GyroX[i/2] + (String)data_chunk.GyroY[i/2] + (String)data_chunk.GyroZ[i/2];
  }

  Serial.println("Going");
  return output;
}