
void TimerHandler()
{
  ISR_Timer.run();
}

bool timerSetup(){
  bool timer_good = true;
  // Set up the timer-driven interrupt
  if (ITimer.attachInterruptInterval(HW_TIMER_INTERVAL_MS * 1000, TimerHandler))
  {
    Serial.print("Starting Timer OK\n");
  }
  else{
    Serial.println("Can't set ITimer. Select another freq. or timer");
    timer_good = false;
  }
  
  ISR_Timer.setInterval(TIMER_INTERVAL_5MS,  SampleAll);
  ITimer.disableTimer();
  return timer_good;
  
}