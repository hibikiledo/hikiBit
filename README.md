# hikiBit
Formally HikiBot

### Status
No longer maintain

### About 
This is part of EGCI405 Mechatronics class.
The repository contains an Arduino sketch namely `AutomateRobot.ino`.  
Inside processing directory, there is a source code namely `processing.pde` for processing which performs image processing.  

#### AutomateRobot.ino
This file contains basic instructions for controlling the robot including line tracking and movements.  
For example, move forward, backward, turn left/right.

#### processing/processing.pde
This file contains image processing codes and some robot control through serial communication.
There is a part that implement simple state machine. However, it is not completed.

### User manual
#### Processing
Here are the keys registered with processing program.

**Keys for controlling thresholding for colors**  
- `]` : increase upper threshold value
- `[` : decrease upper threshold value
- `p` : increase lower threshold value
- ``o : decrease lower threshold value

**Key for switching between the 2 colors**
There are 2 colors that the program can detects.  
Color1 uses hue with thresholding for detecting yellow ball.  
Color2 use grayscale for detecting white ball.
- `c` : toggle between color1 and color2

**Direct command mode**  
In direct command mode, any key pressed will be sent to Arduino directly without interpretion
- `q` : enable/disable direct command mode

**Sending command enabler**  
When first launched, processing doesn't send command right away.  
To send the command to the robot, one has to enable sending command flag.
- `s` : enable/disable command flag

**Switching between states**  
The application keep looping in states defined. However, this functionality is not finished.  
Manual switching between some states is requied.
- `.` : go to next state (from current state)
- `,` : go to previous state (from current state)

### Arduino
Codes in arduino is pretty straightforward and is commented throughoutly.
