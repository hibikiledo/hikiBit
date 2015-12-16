/*
  Wheel Robot:
  - 2 Wheels
  - Receive character command from serial port
  - Track line command
 */

#include <SoftwareSerial.h>// import the serial library

// Setup Pin 12 and 13 for Serial RX and TX
SoftwareSerial BTSerial(12, 13); // RX, TX

// Define directions
#define LEFT    0
#define CENTER  1
#define RIGHT   2

// Define forward or backward
#define FORWARD   1
#define REVERSE   2
#define NONE      0

// Define pin number for M1 = left
#define M1_A    4
#define M1_B    5
#define M1_PWM  10

// Define pin number for M2 = right
#define M2_A    2
#define M2_B    3
#define M2_PWM  11

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

// actual speed of M1 and M2
int M1_speed = 0;
int M2_speed = 0;

// variable to store current state of motor
int M1_state = NONE;
int M2_state = NONE;

// [CONFIG] variable to store speed value for MANUAL control
int speed_val_left = 200;
int speed_val_right = 250;

// [CONFIG] variable to setup the speed for tracking
const int forwardSpeedLeft = 150;
const int forwardSpeedRight = 200;
const int reverseSpeedLeft = 150;
const int reverseSpeedRight = 200;

int n = 0;

// the setup function runs once when you press reset or power the board
void setup() {

  // Setup hardware serial for AT command monitoring
  // Do not use port TX and RX for anything
  BTSerial.begin(115200);
  //BTSerial.println("Robot program started");

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
    track_line_from_start_to_base_task();
    release_ball_task();

  }
}

/* CONTROLLING MOTORS */

void M1_reverse(int x) {
  M1_state = REVERSE;
  digitalWrite(M1_A, LOW);
  digitalWrite(M1_B, HIGH);
  analogWrite(M1_PWM, x );
}

void M1_forward(int x) {
  M1_state = FORWARD;
  digitalWrite(M1_A, HIGH);
  digitalWrite(M1_B, LOW);
  analogWrite(M1_PWM, x );
}

void M1_stop() {
  if (M1_state == FORWARD) {
    M1_reverse(speed_val_left);
  } else if (M1_state == REVERSE) {
    M1_forward(speed_val_left);
  }
  delay(50);
  digitalWrite(M1_B, LOW);
  digitalWrite(M1_A, LOW);
  digitalWrite(M1_PWM, LOW);
  M1_state = NONE;
}

void M2_forward(int y) {
  M2_state = FORWARD;
  digitalWrite(M2_A, HIGH);
  digitalWrite(M2_B, LOW);
  analogWrite(M2_PWM, y );
}

void M2_reverse(int y) {
  M2_state = REVERSE;
  digitalWrite(M2_A, LOW);
  digitalWrite(M2_B, HIGH);
  analogWrite(M2_PWM, y);
}

void M2_stop() {
  if (M2_state == FORWARD) {
    M2_reverse(speed_val_right);
  } else if (M2_state == REVERSE) {
    M2_forward(speed_val_right);
  }
  delay(50);
  digitalWrite(M2_B, LOW);
  digitalWrite(M2_A, LOW);
  digitalWrite(M2_PWM, LOW);
  M2_state = NONE;
}

/*  MOB CONTROL */
void MOB_forward() {
  digitalWrite(MOB_A, LOW);
  digitalWrite(MOB_B, HIGH);
  analogWrite(MOB_PWM, 200);
}

void MOB_reverse() {
  digitalWrite(MOB_A, HIGH);
  digitalWrite(MOB_B, LOW);
  analogWrite(MOB_PWM, 200);
}

void MOB_stop() {
  analogWrite(MOB_PWM, 0);
}

/* MANUAL CONTROL */
void manual() {
  // if byte is equal to "105" or "i", go forward
  if (incomingByte == 105) {
    M1_forward(speed_val_left);
    M2_forward(speed_val_right);
  }
  // if byte is equal to "106" or "j", go left
  else if (incomingByte == 106) {
    M1_reverse(speed_val_left);
    M2_forward(speed_val_right);
  }
  // if byte is equal to "108" or "l", go right
  else if (incomingByte == 108) {
    M1_forward(speed_val_left);
    M2_reverse(speed_val_right);
  }
  // if byte is equal to "107" or "k", go reverse
  else if (incomingByte == 107) {
    M1_reverse(speed_val_left);
    M2_reverse(speed_val_right);
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
  else if (incomingByte == '9') { // Turn left (high precision)
    M1_reverse(speed_val_left);
    M2_forward(speed_val_right);
    delay(100);
    M1_stop();
    M2_stop();
  }
  else if (incomingByte == '8') { // Turn right (high precision)
    M1_forward(speed_val_left);
    M2_reverse(speed_val_right);
    delay(100);
    M1_stop();
    M2_stop();
  }
}

/* RELEASE BALL */
void release_ball_task() {

  if (incomingByte == '1') {

    // reverse direction of mob
    MOB_reverse();

    // move back a little
    M1_reverse(100);
    M2_reverse(100);

    delay(1000);

    M1_stop();
    M2_stop();
    MOB_stop();

    // Tell processing that release ball task is done :)
    BTSerial.write((char) '1');

  }

}

/* LINE TRACKING */
void track_line_from_start_to_base_task() {

  boolean endOfTrack = false;
  int lossCounter = 0;
  int previousState = 0;

  // if byte is equals to 48 or '0', track line to the end
  if (incomingByte == '0') {

    while (!endOfTrack) {

      int L = digitalRead(OPTO_L);
      int R = digitalRead(OPTO_R);
      int C = digitalRead(OPTO_C);

      if (L == LOW) {  // Turn right
        previousState = LEFT;
        M1_reverse(forwardSpeedLeft);
        M2_forward(reverseSpeedRight);
        lossCounter = 0;
      }
      else if (R == LOW) {  // Turn left
        previousState = RIGHT;
        M1_forward(reverseSpeedLeft);
        M2_reverse(forwardSpeedRight);
        lossCounter = 0;
      }
      else if (C == LOW) {  // Forward
        previousState = CENTER;
        M1_forward(forwardSpeedLeft);
        M2_forward(forwardSpeedRight);
        lossCounter = 0;
      }
      // we detect nothing here .. try to get back to track
      else if (L == HIGH && R == HIGH && C == HIGH) {
        lossCounter += 1;
        if (previousState == LEFT) {
          M1_reverse(forwardSpeedLeft);
          M2_forward(reverseSpeedRight);
        }
        else if (previousState == RIGHT) {
          M1_forward(reverseSpeedLeft);
          M2_reverse(forwardSpeedRight);
        }
        else if (previousState == CENTER) {
          M1_forward(forwardSpeedLeft);
          M2_reverse(reverseSpeedRight);
        }
      }

      if (lossCounter == 10000) {
        endOfTrack = true;
      }

    }

    M1_stop();
    M2_stop();

    // Tell processing that line tracking is done :)
    BTSerial.write((char) '0');
  }

}

