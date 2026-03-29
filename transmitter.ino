#include <SoftwareSerial.h>

SoftwareSerial mySerial(2, 3); // RX=2, TX=3

const int potPins[6] = {A0, A1, A2, A3, A4, A5};
int potValues[6];

void setup() {
  Serial.begin(9600);      
  mySerial.begin(9600);    
}

void loop() {
  // Read all 6 potentiometers
  for (int i = 0; i < 6; i++) {
    potValues[i] = analogRead(potPins[i]);
  }

  // Send data sequentially separated by commas
  for (int i = 0; i < 6; i++) {
    mySerial.print(potValues[i]);
    if (i < 5) {
      mySerial.print(","); 
    }
  }
  mySerial.println(); // Send newline character as the end marker

  // 50ms delay keeps the data stream smooth without flooding the receiver
  delay(50); 
}