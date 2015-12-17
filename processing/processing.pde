/*
 * Ball Tracking source code originally provided by Aj. Ittichote.
 * Edited by @hbk
 *
 * Features
 *   - Realtime hue value adjustment.
 *   - Support 2 colours
 *   - No more annoying welcome messages in console
 *   - Non-blocking delay for sending commands in ball tracking mode
 *   - Implement simple state machine
 *
 * Customization
 *   - Change CONTOUR_AREA to the right values for filtering out small objects.
 */

import ipcapture.*;
import gab.opencv.*;
import org.opencv.imgproc.Imgproc;
import org.opencv.core.MatOfPoint2f;
import org.opencv.core.Point;
import org.opencv.core.Size;
import org.opencv.core.Mat;
import org.opencv.core.CvType;
import processing.video.*;
import java.awt.*;
import processing.serial.*; 

IPCapture cam;

OpenCV opencv;
PImage bw, h, g, firstColorSnapshot, secondColorSnapshot;
Rectangle block;
ArrayList<Contour> contoursColor1, contoursColor2;
ArrayList<Contour> polygons;

int ballCountColor1 = 0, ballCountColor2 = 0;

Serial port;

final int CONTOUR_AREA = 1000;

// Variables for performing inRange(start, end) on the image to find balls
int startColor1 = 0, endColor1 = 1;
int startColor2 = 0, endColor2 = 1;

// true : color 1, false : color 2
boolean colorSwitch = true;

// Window width and height
final int width = 640 + 320;
final int height = 480 + 90;

ArrayList<Character> preCommandQueue = new ArrayList<Character>(); 

// Define constants for commands
final Character FULL_TRACK = '0';
final Character RELEASE_BALL = '1';
final Character SWEEP = '2';
final Character COLLECT_BALL = '3';
final Character GO_TO_BASE = '4';

final char[] states = new char[] {
  FULL_TRACK, RELEASE_BALL, SWEEP, COLLECT_BALL, GO_TO_BASE
};

int currentStateIndex = 0;
double globalCounter = 0;

boolean sendingCommand = false; // A flag that determines whether the command should be sent.
boolean directCommandSwitch = false; // A flag that determines if keypress should be sent directly without processing

void setup() {
  size(width, height);
  cam = new IPCapture(this, "http://192.168.43.1:8080/video", "", ""); // Android's Ipwebcam
  //cam = new IPCapture(this, "http://192.168.1.109/live", "", ""); // iOS's iPCamera (Doesn't work yet)
  cam.start();

  port = new Serial(this, "/dev/cu.hikiBot-DevB", 115200);

  // Initialize opencv here to avoid getting welcome messages
  opencv = new OpenCV(this, 640, 480);

  // this works as well:

  // cam = new IPCapture(this);
  // cam.start("url", "username", "password");

  // It is possible to change the MJPEG stream by calling stop()
  // on a running camera, and then start() it with the new
  // url, username and password.

  // Add preload task (state) in the queue
  // preCommandQueue.add(FULL_TRACK);
}

void draw() 
{
  int x = 0, y = 0, size = 0, error = 0, count = 0, speed = 0;

  // reset counter variables
  ballCountColor1 = 0;
  ballCountColor2 = 0;

  if (cam.isAvailable()) 
  {
    cam.read();    
    opencv.loadImage(cam);
    opencv.useColor(HSB);
    h = opencv.getSnapshot(opencv.getH());

    opencv.loadImage(cam);
    opencv.useGray();
    g = opencv.getSnapshot();

    opencv.useGray();

    // color 1
    opencv.loadImage(h);
    opencv.inRange(startColor1, endColor1);
    firstColorSnapshot = opencv.getSnapshot();
    contoursColor1 = opencv.findContours();

    // color 2
    opencv.loadImage(g);
    opencv.inRange(startColor2, endColor2);
    secondColorSnapshot = opencv.getSnapshot();
    contoursColor2 = opencv.findContours();

    // draw
    // 1) original image
    // 2) color1 processed image
    // 3) color2 processed image
    image(cam, 0, 0);
    image(firstColorSnapshot, 640, 0, 320, 240);
    image(secondColorSnapshot, 640, 240, 320, 240);

    // Erode and dilate to eliminate small blob    
    opencv.erode();
    opencv.dilate();

    // draw rectangle for color 2 with green border
    for (Contour contour : contoursColor2) 
    {
      stroke(0, 255, 0);
      noFill();
      if (contour.area() > CONTOUR_AREA) // Look for large object
      {
        // Draw rectangular block over the ball 
        block = contour.getBoundingBox();
        rect(block.x, block.y, block.width, block.height);    
        // Note the center of the block is (block.x+block.width/2, block.y+block.height/2)
        x = block.x + block.width/2;
        y = block.y + block.height/2;
        size = block.width;
        point(x, y);
        // Increment counter
        ballCountColor2 += 1;
      }
    }

    // draw rectangle for color 1 with red border
    for (Contour contour : contoursColor1) 
    {
      stroke(255, 0, 0);
      noFill();
      if (contour.area() > CONTOUR_AREA) // Look for large object
      {        
        // Draw rectangular block over the ball 
        block = contour.getBoundingBox();
        rect(block.x, block.y, block.width, block.height);    
        // Note the center of the block is (block.x+block.width/2, block.y+block.height/2)
        x = block.x + block.width/2;
        y = block.y + block.height/2;
        size = block.width;
        point(x, y);
        // Increment counter
        ballCountColor1 += 1;
      }
    }    

    report();
  } // end camera available  

  /*
   * Communicate with arduino
   *   - sendingCommand : global flag to control sending command to arduino
   */
  if (sendingCommand) {

    // handle preCommand first
    if (preCommandQueue.size() > 0) {
      Character command = preCommandQueue.remove(0);
      port.write(command);
    } 
    // pre command has been handled, process other commands as normal
    else {

      if (states[currentStateIndex] == COLLECT_BALL) {
        /* Collect balls */
        // I use globalCounter instead of calling to delay() to keep video smooth
        if ((int) globalCounter % 20 == 0) {          

          // find error from center ( 320 comes from 640/2 or width of image / 2 )
          error = 320 - x;
          println("error : " + error);

          // Turn the robot according to the error
          if (error > 80) { // Turn left
            println("turning left");
            port.write('9');
          } else if (error < -80) { // Turn right
            println("turning right");
            port.write('8');
          } else { // Ball is centered
            if (size > 65) { // Move forward .. catch it!
              println("Size < 85 .. moving forward");
              port.write('7');
            } else { // Move forward to get closer to it
              println("Almost there .. going closer");
              port.write('6');
            }
          }
        }
      }
    }// end Collect balls

    delay(1);

    // Uncomment line below for debugging
    //sendingCommand = false;
  } // end sendingCommand

  globalCounter += 1;
} // end draw

/**
 * Handle keyboard pressed
 */
void keyPressed() 
{

  /* Global level command */
  if (key == ' ') 
  {
    if (cam.isAlive()) cam.stop();
    else cam.start();
  }
  if (key == 'q') {
    directCommandSwitch = !directCommandSwitch;
  }

  /* Handle command differently according to the directCommandSwitch */
  if (directCommandSwitch) { // Command switch enabled, send key directly to arduino
    println("Sending " + key + " Via direct command channel ");
    port.write(key);
  } else { // Command switch disabled, interpret normally

    if (key == 's') {
      if (sendingCommand) {
        println("Stop sending commands");
        sendingCommand = false;
      } else {
        println("Start sending commands");
        sendingCommand = true;
      }
    }

    /** 
     * Keys for adjusting start and end values for inRange().
     * o : decrement start value by 1
     * p : increment start value by 1
     * [ : decrement end value by 1
     * ] : increment end value by 1
     * c : toggle between colors (Toggle colorSwitch)
     *
     * r : Report values of both colors
     */
    if (key == 'o') {
      if (colorSwitch) {
        startColor1 -= 1;
      } else {
        startColor2 -= 1;
      }
    }
    if (key == 'p') {
      if (colorSwitch) {
        startColor1 += 1;
      } else {
        startColor2 += 1;
      }
    }
    if (key == '[') {
      if (colorSwitch) {
        endColor1 -= 1;
      } else {
        endColor2 -= 1;
      }
    }
    if (key == ']') {
      if (colorSwitch) {
        endColor1 += 1;
      } else {
        endColor2 += 1;
      }
    }
    if (key == 'c') {
      colorSwitch = !colorSwitch;
    }

    /**
     * Keys for switching between states
     * ',' (<) : go to previous state
     * '.' (>) : go to next state
     */
    if (key == ',') {
      currentStateIndex = prevStateIndexFrom(states[currentStateIndex]);
    }
    if (key == '.') {
      currentStateIndex = nextStateIndexFrom(states[currentStateIndex]);
    }
  }
}

/**
 * handle serial event
 */
void serialEvent(Serial p) { 
  char inByte = (char) p.read();    
  println("Serial event : " + (char) inByte);

  currentStateIndex = nextStateIndexFrom(inByte);
} 

/**
 *  Return next state index from the current one
 */
int nextStateIndexFrom(char current) { 

  if (current == FULL_TRACK) {
    return 1;
  } else if (current == RELEASE_BALL) {
    return 2;
  } else if (current == SWEEP) {
    return 3;
  } else if (current == COLLECT_BALL) {
    return 4;
  } else if (current == GO_TO_BASE) {
    return 1;
  } else {
    return 1;
  }
}

/**
 *  Return previous state index from the current one
 */
int prevStateIndexFrom(char current) { 

  if (current == FULL_TRACK) {
    return 1;
  } else if (current == RELEASE_BALL) {
    return 1;
  } else if (current == SWEEP) {
    return 1;
  } else if (current == COLLECT_BALL) {
    return 2;
  } else if (current == GO_TO_BASE) {
    return 3;
  } else {
    return 1;
  }
}

/* 
 * Update on screen report
 */
void report() {
  // clear report portion
  fill(50, 50, 50);
  noStroke();
  rect(0, 480, width, height);

  // customize style
  fill(61, 197, 241);    
  textSize(16);

  text("# COLOR 1: " + ballCountColor1, 20, 500);
  text("# COLOR 2: " + ballCountColor2, 20, 520);
  text("Hue range of color 1: " + startColor1 + " - " + endColor1, 20, 540);
  text("Hue range of color 2: " + startColor2 + " - " + endColor2, 20, 560);

  if (colorSwitch) {
    text("Working on COLOR 1", 320, 500);
  } else {
    text("Working on COLOR 2", 320, 500);
  }

  if (sendingCommand) {
    text("Sending command switch ON", 320, 520);
  } else {
    text("Sending command switch OFF", 320, 520);
  }  

  if (directCommandSwitch) {
    text("Direct command switch is ON", 320, 540);
  } else {
    text("Direct command switch is OFF", 320, 540);
  }

  char state = states[currentStateIndex];

  if (state == FULL_TRACK) {
    text("Current state: FULL_TRACK", 320, 560);
  } else if (state == RELEASE_BALL) {
    text("Current state: RELEASE_BALL", 320, 560);
  } else if (state == SWEEP) {
    text("Current state: SWEEP", 320, 560);
  } else if (state == COLLECT_BALL) {
    text("Current state: COLLECT_BALL", 320, 560);
  } else if (state == GO_TO_BASE) {
    text("Current state: GO_TO_BASE", 320, 560);
  }
}

