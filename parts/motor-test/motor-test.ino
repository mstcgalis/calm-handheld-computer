const int motorPin = D8; 

void setup() {
  pinMode(motorPin, OUTPUT);
  Serial.begin(115200);
  Serial.println("Motor test starting...");
}

void loop() {
  Serial.println("Motor ON");
  digitalWrite(motorPin, HIGH);
  delay(500);

  Serial.println("Motor OFF");
  digitalWrite(motorPin, LOW);
  delay(500);
}
