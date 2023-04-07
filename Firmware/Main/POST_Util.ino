void testCircuit(uint8_t state) {
  digitalWrite(ITSY_TESTPULSE_PIN, state);
}

String analogPOST() {
  bool lazyFlag = 1;
  String error = "";
  digitalWrite(ITSY_TESTPULSE_PIN, HIGH);
  ITimer.enableTimer();
  while (lazyFlag) {
    Serial.print("");
    if (tail > 0 && Q[0].done_sampling) {
      
      POST_chunk = Q[0];
      for (int i = 0; i < tail; i++) {
        Q[i] = Q[i + 1];
      }
      tail--;
      lazyFlag = false;
    }
  }
  ITimer.disableTimer();
  
  //Serial.println(stringifyChunk2(POST_chunk));
  double error1 = fitPOST(POST_chunk.EOG1);
  double error2 = fitPOST(POST_chunk.EOG2);
  if (error1 > 15){
    //error = "EOG 1 ERROR"
  }
  if (error2 > 15){
    //error = error + "EOG 2 ERROR"
  }
  digitalWrite(ITSY_TESTPULSE_PIN, LOW);
  return error;
}

double fitPOST(unsigned short input[200]) {
  double freq = 23.0; // starting frequency
  double amp = 0.1; // starting amplitude
  double dc_offset = 2.5; // starting DC offset
  double best_freq, best_amp, best_dc_offset, best_error;
  double error, t, val, sine_val;
  int i, j;

  best_error = 1e20; // very large initial error
  // iterate over frequencies
  for (i = 0; i < 10; i++) {
    // iterate over amplitudes
    amp = 0.1;
    for (j = 0; j < 20; j++) {
      // iterate over DC offsets      
      for (dc_offset = 0.8; dc_offset <= 1.2; dc_offset += 0.1) {
        error = 0.0;
        // calculate error for this combination of parameters
        for (t = 0.3; t < 0.4; t += 0.005) {
          sine_val = dc_offset + amp * sin(2.0 * PI * freq * t);
          val = (double)input[(int)(t * 200.0)] / 4090.0 * 3.3;
          error += pow(val - sine_val, 2.0);
        }
        // check if this is the best fit so far
        if (error < best_error) {
          best_error = error;
          best_freq = freq;
          best_amp = amp;
          best_dc_offset = dc_offset;
        }
      }
      amp += 0.1; // increment amplitude
    }
    freq += 0.5; // increment frequency
  }
  printf("Best fit parameters:\n");
  printf("Frequency: %.2f Hz\n", best_freq);
  printf("Amplitude: %.2f V\n", best_amp);
  printf("DC Offset: %.2f V\n", best_dc_offset);
  printf("Error: %.2f\n", best_error);
  return error;
}