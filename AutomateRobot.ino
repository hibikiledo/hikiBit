/*
  Wheel Robot:
  - 2 Wheels
  - Receive character command from serial port
  - Track line command
 */

#include <SoftwareSerial.h>// import the serial library

// Setup Pin 12 and 13 for Serial RX and TX
SoftwareSerial BTSerial(12, 13); // RX, TX

// Define pin number for M1 = left
#define M1_A    2
#define M1_B    3
#define M1_PWM  11

// Define pin number for M2 = right
#define M2_A    4
#define M2_B    5
#define M2_PWM  10

// Define pin number for mob motor control
#define MOB_A 0
#define MOB_B 1
#define MOB_PWM 9

// Define pin number for phototransistor
#define OPTO_L    6 // Left
#define OPTO_C    7 // Center
#define OPTO_R    8 // Right

// Define pin number for ultrasonic sensor
#define TRIGGER_PIN   9  // Arduino pin tied to trigger pin on the ultrasonic sensor.
#define ECHO_PIN      9  // Arduino pin tied to echo pin on the ultrasonic sensor.
#define MAX_DISTANCE  100 // Maximum distance we want to ping for (in centimeters). Maximum sensor distance is rated at 400-500cm.

// variable to store serial data
int incomingByte = 0;

// variable to store speed value
int speed_val = 200;

// actual speed of M1 and M2
int M1_speed = 0;
int M2_speed = 0;

int n = 0;

// the setup function runs once when you press reset or power the board
void setup() {

  // Setup hardware serial for AT command monitoring
  // Do not use port TX and RX for anything
  BTSerial.begin(115200);
  BTSerial.println("Robot program started");

  //Serial.begin(9600);
  //Serial.println("Report Serial");

  // initialize digitan pin 1-2 as output for mob rotation
  pinMode(MOB_A, OUTPUT);
  pinMode(MOB_B, OUTPUT);
  pinMode(MOB_PWM, OUTPUT);

  // initialize digital pin 2-5 as output for wheel control
  pinMode(2, OUTPUT);
  pinMode(3, OUTPUT);
  pinMode(4, OUTPUT);
  pinMode(5, OUTPUT);

  // initialize digital pin 6-8 as output for wheel control
  pinMode(6, INPUT);
  pinMode(7, INPUT);
  pinMode(8, INPUT);

  // initialize initial states for output pins


}

// the loop function runs over and over again forever
void loop() {

  // check for serial data
  if (BTSerial.available() > 0) {

    // read the incoming byte:
    incomingByte = BTSerial.read();

    // say what you got:
    //Serial.print("I received: ");
    //Serial.println((char) incomingByte);
    // delay 10 milliseconds to allow serial update time
    delay(10);

    // handle command received from the serial
    manual();
    track_line_to_base();

  }
}

/* Constrain speed value to between 0-255 */
void test_speed() {
  if (speed_val > 255) {
    speed_val = 255;
    //Serial.println(" MAX ");
  }
  if (speed_val < 0) {
    speed_val = 0;
    //Serial.println(" MIN ");
  }
}

/* CONTROLLING MOTORS */

void M1_reverse(int x) {
  digitalWrite(M1_A, LOW);
  digitalWrite(M1_B, HIGH);
  analogWrite(M1_PWM, x );
}

void M1_forward(int x) {
  digitalWrite(M1_A, HIGH);
  digitalWrite(M1_B, LOW);
  analogWrite(M1_PWM, x );
}

void M1_stop() {
  digitalWrite(M1_B, LOW);
  digitalWrite(M1_A, LOW);
  digitalWrite(M1_PWM, LOW);
}

void M2_forward(int y) {

  digitalWrite(M2_A, HIGH);
  digitalWrite(M2_B, LOW);
  analogWrite(M2_PWM, y );
}

void M2_reverse(int y) {
  digitalWrite(M2_A, LOW);
  digitalWrite(M2_B, HIGH);
  analogWrite(M2_PWM, y);
}

void M2_stop() {
  digitalWrite(M2_B, LOW);
  digitalWrite(M2_A, LOW);
  digitalWrite(M2_PWM, LOW);
}

/*  MOB CONTROL */
void MOB_forward() {
  digitalWrite(MOB_A, HIGH);
  digitalWrite(MOB_B, LOW);
  analogWrite(MOB_PWM, 200);
}

void MOB_reverse() {
  digitalWrite(MOB_A, LOW);
  digitalWrite(MOB_B, HIGH);
  analogWrite(MOB_PWM, 200);
}

void MOB_stop() {
  analogWrite(MOB_PWM, 0);
}

/* MANUAL CONTROL */
void manual() {
  // check incoming byte for direction
  // if byte is equal to "46" or "." - raise speed
  if (incomingByte == 46) {
    speed_val = speed_val + 5;
    test_speed();
    //Serial.println(speed_val);
  }
  // if byte is equal to "44" or "," - lower speed
  else if (incomingByte == 44) {
    speed_val = speed_val - 5;
    test_speed();
    //Serial.println(speed_val);
  }
  // if byte is equal to "47" or "/" - max speed
  else if (incomingByte == 47) {
    speed_val = 255;
    //Serial.println(" MAX ");
  }

  // if byte is equal to "105" or "i", go forward
  else if (incomingByte == 105) {
    M1_forward(speed_val);
    M2_forward(speed_val);
  }
  // if byte is equal to "106" or "j", go left
  else if (incomingByte == 106) {
    M1_forward(speed_val);
    M2_reverse(speed_val);
  }
  // if byte is equal to "108" or "l", go right
  else if (incomingByte == 108) {
    M1_reverse(speed_val);
    M2_forward(speed_val);
  }
  // if byte is equal to "107" or "k", go reverse
  else if (incomingByte == 107) {
    M1_reverse(speed_val);
    M2_reverse(speed_val);
  }
  // if byte is equal to "100" or "d", stop
  else if (incomingByte == 100) {
    M1_stop();
    M2_stop();
  }
  else if (incomingByte == 'z') {
    MOB_forward();
  }
  else if (incomingByte == 'x') {
    MOB_reverse();
  }
  else if (incomingByte == 'c') {
    MOB_stop();
  }
}

/* LINE TRACKING */
void track_line_to_base() {

  boolean onTrack = true;
  int lossCounter = 0;

  int forwardSpeed = 150;
  int reverseSpeed = 150;

  // if byte is equals to 48 or '0', track line to the end
  if (incomingByte == 48) {

    while (onTrack) {

      int L = digitalRead(OPTO_L);
      int R = digitalRead(OPTO_R);
      int C = digitalRead(OPTO_C);

      if (L == LOW) {  // Turn right
        M1_forward(forwardSpeed);
        M2_reverse(reverseSpeed);
        lossCounter = 0;
      }
      else if (R == LOW) {  // Turn left
        M1_reverse(reverseSpeed);
        M2_forward(forwardSpeed);
        lossCounter = 0;
      }
      else if (C == LOW) {  // Forward
        M1_forward(forwardSpeed);
        M2_forward(forwardSpeed);
        lossCounter = 0;
      }
      // we detect nothing here
      else if (L == HIGH && R == HIGH && C == HIGH) {
        lossCounter += 1;
      }
      if (lossCounter == 15000) {
        onTrack = false;
      }
    }
    M1_stop();
    M2_stop();

    // Tell processing that line tracking is done :)
    BTSerial.write((char) '0');
  }


}

