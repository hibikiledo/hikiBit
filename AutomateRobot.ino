/*
  Wheel Robot:
  - 2 Wheels
  - Receive character command from serial port
  - Track line command
 */

#include <SoftwareSerial.h>// import the serial library

// Setup Pin 12 and 13 for Serial RX and TX
SoftwareSerial BTSerial(12, 13); // RX, TX

// Define states for LINE TRACKER
#define LEFT    0
#define CENTER  1
#define RIGHT   2

/*
 * Define states for Motors
 *   FORWARD : motor is rotating in forward direction
 *   REVERSE : motor is rotating in reverse direction
 *   NONE : motor is stopped
 *
 * Note
 *   The states are used for implementing fast stop.
 *   Fast stop allows robot to suddenly stop without continue moving.
 */
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

// Define delay for motion in reverse direction for fast stop implementation
#define FAST_STOP_DELAY 10

// variable to store serial data
int incomingByte = 0;

// variable to store current state of motor
int M1_state = NONE;
int M2_state = NONE;

// [CONFIG] variable to store speed value for MANUAL control
#define speed_val_left  200
#define speed_val_right 250

// [CONFIG] variable to setup the speed for tracking
#define forwardSpeedLeft  150
#define forwardSpeedRight 200
#define reverseSpeedLeft  150
#define reverseSpeedRight 200

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

    //Serial.print("I received: ");
    //Serial.println((char) incomingByte);
    // delay 10 milliseconds to allow serial update time
    delay(10);

    // perform manual command dispatching
    manual();

    // check for task sent from processing
    if (incomingByte == '0') {
      track_line_from_start_to_base_task();
    }
    if (incomingByte == '1') {
      release_ball_task();
    }
  }
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
    delay(50);
    M1_stop();
    M2_stop();
  }
  else if (incomingByte == '8') { // Turn right (high precision)
    M1_forward(speed_val_left);
    M2_reverse(speed_val_right);
    delay(50);
    M1_stop();
    M2_stop();
  }
  else if (incomingByte == '7') { // Long forward
    M1_forward(speed_val_left);
    M2_forward(speed_val_right);
    delay(500);
    M1_stop();
    M2_stop();
  }
  else if (incomingByte == '6') { // Short forward
    M1_forward(speed_val_left);
    M2_forward(speed_val_right);
    delay(100);
    M1_stop();
    M2_stop();
  }
}

/* RELEASE BALL */
void release_ball_task() {

  // reverse direction of mob
  MOB_reverse();

  // move back a little
  M1_reverse(150);
  M2_reverse(150);

  delay(800);

  M1_stop();
  M2_stop();
  MOB_stop();

  // Tell processing that release ball task is done :)
  BTSerial.write((char) '1');

}

/* LINE TRACKING TASK */
void track_line_from_start_to_base_task() {

  MOB_forward();

  // call a shared implementation of line tracking
  track_line_to_base();

  MOB_stop();

  // notify processing that the task is done
  BTSerial.write((char) '0');

}


/* SHARED FUNCTIONs ACROSS TASKS */
void track_line_to_base() {

  boolean endOfTrack = false;

  int previousState = 0;
  int track_speed = 180;

  int center_loss_count = 0;
  int loss_count = 0;

  while (!endOfTrack) {

    int L = digitalRead(OPTO_L);
    int R = digitalRead(OPTO_R);
    int C = digitalRead(OPTO_C);

    if (L == LOW) {  // Turn left
      //BTSerial.println("LEFT FOUND");
      previousState = LEFT;
      M1_reverse(track_speed - 50);
      M2_forward(track_speed + 20);
    }
    else if (R == LOW) {  // Turn right
      //BTSerial.println("RIGHT FOUND");
      previousState = RIGHT;
      M1_forward(track_speed - 50);
      M2_reverse(track_speed + 20);
    }
    else if (C == LOW) {  // Forward
      //BTSerial.println("CENTER FOUND");
      previousState = CENTER;
      M1_forward(track_speed - 50 - 10);
      M2_forward(track_speed + 20 - 10);
    }
    else if (L == HIGH && C == HIGH && R == HIGH) {
      //BTSerial.println("LOSS");
      if (previousState == LEFT) {
        M1_reverse(track_speed - 50);
        M2_forward(track_speed + 20);
      }
      else if (previousState == RIGHT) {
        M1_forward(track_speed - 50);
        M2_reverse(track_speed + 20);
      }
      else if (previousState == CENTER) { // due to nature of the sensor . turning right solves the problem.
        center_loss_count += 1;

        // stop everthing and wait for robot to be stable
        M1_stop();
        M2_stop();
        delay(500);

        // reverse back a little with a hope that we get on the line
        M1_reverse(track_speed - 20);
        M2_reverse(track_speed + 20);
        delay(100);

        M1_stop();
        M2_stop();
        delay(500);

        // sweep with hope to find the line
        int turn_left_count = 0;
        int turn_right_count = 0;
        const int MAX_TURN_COUNT = 20;

        // sweep to the left
        do {
          C = digitalRead(OPTO_C);
          M1_reverse(track_speed - 50);
          M2_forward(track_speed + 20);
          delay(50);
          M1_stop();
          M2_stop();
          turn_left_count += 1;
        } while (C == HIGH && turn_left_count < MAX_TURN_COUNT);

        // we are back on track
        if (turn_left_count != MAX_TURN_COUNT) {
          continue;
        }

        // rotate back to center
        for (int i = 0; i < turn_left_count; i++) {
          M1_forward(track_speed - 50);
          M2_reverse(track_speed + 10);
          delay(50);
          M1_stop();
          M2_stop();
        }

        // we are probably at base
        if (turn_left_count == MAX_TURN_COUNT) {
          break;
        }

        // sweep to the right
        do {
          C = digitalRead(OPTO_C);
          M1_forward(track_speed - 50);
          M2_reverse(track_speed + 20);
          delay(50);
          M1_stop();
          M2_stop();
          turn_right_count += 1;
        } while (C == HIGH && turn_right_count < MAX_TURN_COUNT);

        if (turn_right_count != MAX_TURN_COUNT) {
          continue;
        }

        // rotate back to center
        for (int i = 0; i < turn_right_count; i++) {
          M1_reverse(track_speed - 50);
          M2_forward(track_speed + 20);
          delay(50);
          M1_stop();
          M2_stop();
        }

        // we are probably at base
        if (turn_right_count == MAX_TURN_COUNT) {
          break;
        }

      }

    }

    if (center_loss_count > 5) {
      break;
    }

    // check if any command has been sent, if so, leave line tracking mode
    if (BTSerial.available()) {
      break;
    }

  } // end of track

  //BTSerial.println("Seems like we are at base STOP!");
  M1_stop();
  M2_stop();
} // end line tracking



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
  delay(FAST_STOP_DELAY);
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
  delay(FAST_STOP_DELAY);
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
  analogWrite(MOB_PWM, 100);
}

void MOB_stop() {
  analogWrite(MOB_PWM, 0);
}

