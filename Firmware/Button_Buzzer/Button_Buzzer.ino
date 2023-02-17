void setup() {
  // put your setup code here, to run once:
  pinMode(13, OUTPUT);
  pinMode(12, INPUT_PULLDOWN);
}

void loop() {
  // put your main code here, to run repeatedly:
  if (digitalRead(12) == HIGH){
    digitalWrite(13, HIGH);
  }
  else{
    digitalWrite(13, LOW);
  }
}
