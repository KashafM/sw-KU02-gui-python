#include <Arduino.h>
#include <Adafruit_TinyUSB.h> // for Serial

int adcin1 = A0;
int adcin2 = A1;
int adcval1 = 0;
int adcval2 = 0;
float mv_per_lsb = 3300.0F/4096.0F; // 10-bit ADC with 3.6V input range

void setup() {
  analogReference(AR_VDD4); //Set the analog reference voltage to VDD (3.3 V)
  analogReadResolution(12); //Set the resolution to 12-bit
  Serial.begin(115200);
  while ( !Serial ) delay(10);   // for nrf52840 with native usb
}

void loop() {
  // Obtain the current ADC values from pins A0 and A1
  adcval1 = analogRead(adcin1); //Read and convert the analog value from pin A0
  adcval2 = analogRead(adcin2); //Read and convert the analog value from pin A1
  delay(5); 

  // Print the converted voltages
  Serial.print("A0 = ");
  Serial.print((float)adcval1 * mv_per_lsb);
  Serial.print(" mV\n");
  Serial.print("A1 = ");
  Serial.print((float)adcval2 * mv_per_lsb);
  Serial.print(" mV\n");

}
