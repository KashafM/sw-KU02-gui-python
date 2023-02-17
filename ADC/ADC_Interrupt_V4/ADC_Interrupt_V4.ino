#define TIMER_INTERRUPT_DEBUG 0
#define _TIMERINTERRUPT_LOGLEVEL_ 0

#include <Arduino.h>
#include <Adafruit_TinyUSB.h> // for Serial
#include "NRF52TimerInterrupt.h"
#include "NRF52_ISR_Timer.h"

// #define HW_FREQ 200 //Frequency of the interrupt in Hz
#define HW_TIMER_INTERVAL_MS 1
#define HW_TIMER_INTERVAL_US 100L //#define HW_TIMER_INTERVAL_MS 1 //Time period of the hardware, in useconds

//volatile uint32_t startMillis = 0;
//#define TIMER1_INTERVAL_MS        3000
//#define TIMER1_DURATION_MS        15000
//#define TIMER_INTERVAL_5MS 5L

// Initialize timer 1
NRF52Timer ITimer(NRF_TIMER_1);
NRF52_ISR_Timer ISR_Timer;

#define TIMER_INTERVAL_5MS 5L
#define TIMER_INTERVAL_1S             1000L
#define TIMER_INTERVAL_2S             2000L
#define TIMER_INTERVAL_5S             5000L

int adcin1 = A0;
int adcin2 = A1;
int adcval1 = 0;
int adcval2 = 0;
float mv_per_lsb = 3300.0F/4096.0F; // 12-bit ADC with 3.3 V input range

void TimerHandler(){
  //static bool toggle  = false;
  //static int timeRun  = 0;
  ISR_Timer.run();
}

void SampleEOG(){
  // Obtain the current ADC values from pins A0 and A1
  adcval1 = analogRead(adcin1); //Read and convert the analog value from pin A0
  adcval2 = analogRead(adcin2); //Read and convert the analog value from pin A1
  delay(5); 
  int sample_time = 0;
  // Print the converted voltages
  sample_time = millis();
  Serial.print("\nTime = ");
  Serial.println(sample_time);
  Serial.print("\n");
  Serial.print("A0 = ");
  Serial.print((float)adcval1 * mv_per_lsb);
  Serial.print(" mV\n");
  Serial.print("A1 = ");
  Serial.print((float)adcval2 * mv_per_lsb);
  Serial.print(" mV\n");
}

void setup() {
  analogReference(AR_VDD4); //Set the analog reference voltage to VDD (3.3 V)
  analogReadResolution(12); //Set the resolution to 12-bit
  Serial.begin(115200);
  while (!Serial && millis() < 5000) delay(10);   // for nrf52840 with native usb

  // Interval in microsecs
  if (ITimer.attachInterruptInterval(HW_TIMER_INTERVAL_US, TimerHandler))
  {
    Serial.print(F("Starting ITimer OK, millis() = "));
    Serial.println(millis());
  }
  else
    Serial.println(F("Can't set ITimer. Select another freq. or timer"));

  ISR_Timer.setInterval(TIMER_INTERVAL_5MS,  SampleEOG);
}

void loop() {
}
