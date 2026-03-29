#include <SoftwareSerial.h>
#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <AccelStepper.h>

SoftwareSerial mySerial(2, 3); // RX=2, TX=3
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
AccelStepper stepper(1, 9, 8); // 1=Driver, 9=STEP, 8=DIR

// Servo Pulse Calibration (Tune these if your servos buzz at their limits)
#define SERVOMIN  150 // Pulse for 0 degrees
#define SERVOMAX  600 // Pulse for 180 degrees
#define SERVO100  400 // Pulse for 100 degrees (Shoulder Limit)

// Variables for non-blocking serial read
const byte numChars = 64; // Large buffer to prevent data cutoff
char receivedChars[numChars];
boolean newData = false;

// Default to 512 (center) so motors don't violently snap to 0 on startup
int potValues[6] = {512, 0, 512, 512, 512, 512}; 

void setup() {
  Serial.begin(9600);
  mySerial.begin(9600);

  pwm.begin();
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(60); 

  stepper.setMaxSpeed(1000);
  stepper.setAcceleration(500);
}

void loop() {
  // 1. Constantly check for new data without pausing the Arduino
  recvWithEndMarker();

  // 2. If a full packet of data has arrived, parse it and update targets
  if (newData == true) {
    parseData();
    updateMotors();
    newData = false; // Reset flag to listen for the next packet
  }

  // 3. This MUST run constantly at max speed to allow the stepper to rotate smoothly
  stepper.run();
}

// --- NON-BLOCKING SERIAL READ FUNCTION ---
void recvWithEndMarker() {
  static byte ndx = 0;
  char endMarker = '\n';
  char rc;

  while (mySerial.available() > 0 && newData == false) {
    rc = mySerial.read();

    // Ignore carriage returns to prevent data corruption
    if (rc == '\r') {
      continue; 
    }

    if (rc != endMarker) {
      receivedChars[ndx] = rc;
      ndx++;
      if (ndx >= numChars) {
        ndx = numChars - 1; // Prevent buffer overflow
      }
    } else {
      receivedChars[ndx] = '\0'; // Terminate the string
      ndx = 0;
      newData = true;
    }
  }
}

// --- DATA PARSING & VALIDATION FUNCTION ---
void parseData() {
  // Packet Validation: Count the commas to ensure the message is fully intact.
  int commaCount = 0;
  for (int i = 0; i < strlen(receivedChars); i++) {
    if (receivedChars[i] == ',') {
      commaCount++;
    }
  }

  // If a comma was lost due to electrical noise, throw the bad packet away.
  if (commaCount != 5) {
    return; 
  }

  char * strtokIndx; 

  // Extract Pot 1
  strtokIndx = strtok(receivedChars, ",");
  if (strtokIndx != NULL) potValues[0] = atoi(strtokIndx);
  
  // Extract Pots 2 through 6
  for(int i = 1; i < 6; i++) {
    strtokIndx = strtok(NULL, ",");
    if (strtokIndx != NULL) {
      potValues[i] = atoi(strtokIndx);
    }
  }
}

// --- MOTOR UPDATE FUNCTION ---
void updateMotors() {
  // 1. Base Stepper (Pot 1) - Maps 0-1023 to 0-800 steps
  long targetPosition = map(potValues[0], 0, 1023, 0, 800);
  stepper.moveTo(targetPosition);

  // 2. Shoulder Mirrored Servos (Pot 2) - Max 100 degrees
  int shoulderLeft = map(potValues[1], 0, 1023, SERVOMIN, SERVO100);
  int shoulderRight = map(potValues[1], 0, 1023, SERVO100, SERVOMIN); // Inverted Mirror
  pwm.setPWM(4, 0, shoulderLeft);
  pwm.setPWM(5, 0, shoulderRight);

  // 3. Elbow Servo (Pot 3) - Reversed Direction (180 down to 0)
  int elbowPulse = map(potValues[2], 0, 1023, SERVOMAX, SERVOMIN);
  pwm.setPWM(3, 0, elbowPulse);

  // 4. Wrist 1 (Pot 4) - 0 to 180 degrees
  int wrist1Pulse = map(potValues[3], 0, 1023, SERVOMIN, SERVOMAX);
  pwm.setPWM(2, 0, wrist1Pulse);

  // 5. Wrist 2 (Pot 5) - 0 to 180 degrees
  int wrist2Pulse = map(potValues[4], 0, 1023, SERVOMIN, SERVOMAX);
  pwm.setPWM(1, 0, wrist2Pulse);

  // 6. Gripper (Pot 6) - 0 to 180 degrees
  int gripperPulse = map(potValues[5], 0, 1023, SERVOMIN, SERVOMAX);
  pwm.setPWM(0, 0, gripperPulse);
}