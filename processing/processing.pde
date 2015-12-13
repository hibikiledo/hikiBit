/*
 * Ball Tracking source code originally provided by Aj. Ittichote.
 * Edited by @hbk
 *
 * Features
 *   - Realtime hue value adjustment.
 *   - Support 2 colours
 *   - No more annoying welcome messages in console
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
PImage bw, h, firstColorSnapshot, secondColorSnapshot;
Rectangle block;
ArrayList<Contour> contoursColor1, contoursColor2;
ArrayList<Contour> polygons;

Serial port;

final int CONTOUR_AREA = 5000;

// Variables for performing inRange(start, end) on the image to find balls
int startColor1 = 0, endColor1 = 1;
int startColor2 = 0, endColor2 = 1;
// true : color 1, false : color 2
boolean colorSwitch = true;

boolean sendingCommand = false; // A flag that determines whether the command should be sent.

void setup() {
  size(640 * 2, 480);
  cam = new IPCapture(this, "http://192.168.1.109:8080/video", "", ""); // Android's Ipwebcam
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
}

void draw() 
{
  int x=0, y=0, size=0, error=0, count=0, speed=0;

  if (cam.isAvailable()) 
  {
    cam.read();    
    opencv.loadImage(cam);

    opencv.useColor(HSB);
    h = opencv.getSnapshot(opencv.getH());

    opencv.useGray();

    // color 1
    opencv.loadImage(h);
    opencv.inRange(startColor1, endColor1);
    firstColorSnapshot = opencv.getSnapshot();
    contoursColor1 = opencv.findContours();

    // color 2
    opencv.loadImage(h);
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
      }
    }
    
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
      }
    }   
    
    /*
    println("found " + contours.size() + " balls");
    println("x = " + x + ", y = " + y +", size = " + size);
    */
  }

  if (sendingCommand) {

    //communicate with arduino here
    //we want to center the ball
    error = 320-x;

    if (error>5) { // Turn left
      port.write('j');
      delay(25);
      port.write('d');
    } else if (error<-5) { // Turn right
      port.write('l');
      delay(25);
      port.write('d');
    } else { // Ball is centered
      if (size<85) { // Move forward
        port.write('i');
        delay(500);
        port.write('d');
      }
    }
    delay(25);
  }
}//end draw();


/*
 * Handle keyboard pressed
 */
void keyPressed() 
{
  if (key == ' ') 
  {
    if (cam.isAlive()) cam.stop();
    else cam.start();
  }

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
    if (colorSwitch) {
      println("Switched to Color 2");
    } else {
      println("Switched to Color 1");
    }
    colorSwitch = !colorSwitch;
  }
  if (key == 'o' || key == 'p' || key == '[' || key == ']') {
    if (colorSwitch) {
      println("Color 1 # Start :" + startColor1, " End : " + endColor1);
    } else {
      println("Color 2 # Start :" + startColor2, " End : " + endColor2);
    }
  }
  if (key == 'r') {
    println("Report colors start and end values");
    println("Color 1 # Start :" + startColor1, " End : " + endColor1);
    println("Color 2 # Start :" + startColor2, " End : " + endColor2);
  }
}

