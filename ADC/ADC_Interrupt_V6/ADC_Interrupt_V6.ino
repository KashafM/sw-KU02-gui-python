#include <Arduino.h>
#include <Adafruit_TinyUSB.h> // for Serial
#include "NRF52TimerInterrupt.h"
#include "NRF52_ISR_Timer.h"

#define TIMER_INTERRUPT_DEBUG 0
//#define _TIMERINTERRUPT_LOGLEVEL_ 0

// #define HW_FREQ 200 //Frequency of the interrupt in Hz
#define HW_TIMER_INTERVAL_MS      1
#define TIMER_INTERVAL_5MS             5L

// Initialize timer 1
NRF52Timer ITimer(NRF_TIMER_1);
NRF52_ISR_Timer ISR_Timer;

int adcin1 = A0;
int adcin2 = A1;
float adcval1 = 0;
float adcval2 = 0;
float mv_per_lsb = 3300.0F/4096.0F; // 12-bit ADC with 3.3 V input range
float time = 0;
int sample_num;

void TimerHandler()
{
  ISR_Timer.run();
}

//void SampleEOG(){
//  // Obtain the current ADC values from pins A0 and A1
//  adcval1 = analogRead(adcin1); //Read and convert the analog value from pin A0
//  adcval2 = analogRead(adcin2); //Read and convert the analog value from pin A1
//  delay(5); 
//
//  // Print the converted voltages
//  Serial.print("A0 = ");
//  Serial.print((float)adcval1 * mv_per_lsb);
//  Serial.print(" mV\n");
//  Serial.print("A1 = ");
//  Serial.print((float)adcval2 * mv_per_lsb);
//  Serial.print(" mV\n");
//}

void SampleEOG(){
  // Print the sample number
  sample_num = sample_num + 1;
  Serial.print("Sample: ");
  Serial.print(sample_num);
  Serial.print("\n");
  
  // Print the second
  time = time + 0.005;
  Serial.print("Time: ");
  Serial.print(time);
  Serial.print(" sec \n");
  // Obtain the current ADC values from pins A0 and A1
  adcval1 = analogRead(adcin1); //Read and convert the analog value from pin A0
  adcval2 = analogRead(adcin2); //Read and convert the analog value from pin A1

  // Print the converted voltages
  Serial.print("A0: ");
  Serial.print((float)adcval1 * mv_per_lsb);
  Serial.print(" mV\n");
  Serial.print("A1: ");
  Serial.print((float)adcval2 * mv_per_lsb);
  Serial.print(" mV\n");
}

void setup() {
  analogReference(AR_VDD4); //Set the analog reference voltage to VDD (3.3 V)
  analogReadResolution(12); //Set the resolution to 12-bit
  Serial.begin(115200);
  while (!Serial && millis() < 5000);
  
  delay(100);
  
  if (ITimer.attachInterruptInterval(HW_TIMER_INTERVAL_MS * 1000, TimerHandler))
  {
    Serial.print("Starting Timer OK\n");
  }
  else
    Serial.println("Can't set ITimer. Select another freq. or timer");

  ISR_Timer.setInterval(TIMER_INTERVAL_5MS,  SampleEOG);
}

void loop() {
}
