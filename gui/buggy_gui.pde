
import processing.net.*;

Client myClient;    //192.168.4.121    was 192.168.67.121

String arduinoIP = "192.168.154.121"; 
int arduinoPort = 5200;       

String buggyStatus = "Unknown";
String mode = "n";
String direction_state = "";
String speed_status = "";
String distance_status = "n";

// --- GUI-related variables ---
float irLeftValue = 0;
float irRightValue = 0;
float RefSpeed = 0;
float oldValue;
String distance_obj = "null";
//int buggySpeed = 0;
boolean connected = false;

float sliderX, sliderY; // Position of the slider
float sliderWidth = 200; // Width of the slider
float sliderMin = 0; // Minimum reference speed --need to change
float sliderMax = 20; // Maximum reference speed -- need to xhange =---------------------------------------------------------------
float sliderValue = 0; // Current value
boolean sliderActive = false; // If the slider is being dragged

Button start;
Button stop;
Button Mode1;
Button Mode2;

// --- Colors ---
color bgColor = color(240, 250, 255); // Light blue background
color textColor = color(20, 50, 80);   // Dark blue text
color buttonColorStart = color(0, 200, 0); // Green Start button
color buttonColorStop = color(250, 0, 0); // Red Stop button
color buttonTextColor = color(20, 50, 80);
color connectedColor = color(50, 200, 50); // Green
color disconnectedColor = color(200, 50, 50); // Red
color buggyColor = color(100, 100, 100); // Grey for the buggy body
color wheelColor = color(40, 40, 40);     // Dark grey for wheels
color sensorActiveColor = color(255, 0, 0); // Red for active sensor
color sensorInactiveColor = color(50, 50, 50); // Dark grey for inactive
color statusBoxColor = color(255); // Light gray for the status box


void setup() {
  size(900, 675);
  myClient = new Client(this, arduinoIP, arduinoPort);

// Initialize slider position
  sliderX = 30; 
  sliderY = 525; // Adjust position as needed

  // --- Initialize GUI elements ---
  start = new Button(30, 50, 120, 100, "Start");
  stop = new Button(180, 50, 120, 100, "Stop");
  Mode1 = new Button(30, 220, 120, 180, "Mode 1");
  Mode2 = new Button(180, 220, 120, 180, "Mode 2");
}

void draw() {
  background(bgColor);

  // --- Reference Speed Slider ---
  fill(255);
  stroke(0);
  line(sliderX, sliderY, sliderX + sliderWidth, sliderY); // Slider track

  // Calculate knob position
  float knobX = map(sliderValue, sliderMin, sliderMax, sliderX, sliderX + sliderWidth);
  
  // Draw slider knob
  fill(0, 100, 255);
  ellipse(knobX, sliderY, 15, 15); // Slider knob
  
  // Display the value
  fill(textColor);
  textSize(16);
  text("Reference Speed: " + sliderValue + " cm/s", sliderX, sliderY - 15);



  // --- Connection Status ---
  if (connected) {
    fill(connectedColor);
    text("Connected", 700, 45);
  } else {
    fill(disconnectedColor);
    text("Disconnected", 700, 45);
  }

  // --- Draw GUI elements ---
  fill(0);
  start.displayStart();
  stop.displayStop();
  Mode2.displayMode2();
  Mode1.displayMode1();


  // --- Buggy directional and go/stop Status Box ----------------------------------------------------------------
  fill(statusBoxColor);
  rect(360, 60, 530, 50); // Status Box
  fill(textColor);
  textSize(20);
  text("Buggy Status:", 365, 80); 
  if ((buggyStatus.contains("Right"))||(buggyStatus.contains("Left"))||(buggyStatus.contains("TURNED OFF"))||(buggyStatus.contains("stopping"))||(buggyStatus.contains("Forward"))|| (buggyStatus.contains("Stopped"))){
    direction_state = buggyStatus;
  }
  text(direction_state, 365,100); //output this constantly 
 
 
  // --- Distance Box --------------------------------------------------------------------------------------------
fill(statusBoxColor);
  rect(360, 160, 530, 50); // Status Box
  fill(textColor);
  textSize(20);
  text("Distance Travelled:", 365, 180);
  if (buggyStatus.contains("(this journey)")){
    distance_status = buggyStatus;  
  }
   text(distance_status, 530, 180); // Display the distance travelled 
   
   
  // --- current mode box ---------------------------------------------------------------------------------------
  fill(statusBoxColor);
  rect(360, 285, 530, 50); 
  fill(textColor);
  textSize(20);
  text("Mode:", 365,305);
  if (mode == "F"){ 
      text("Following object at 15cm distance. ", 420, 305); 
      if (buggyStatus.contains("from object is:")){ 
        distance_obj = buggyStatus;//will only be putting distance form object in here - speed will be constantly reported
      }
      text(distance_obj, 365, 335); 
  }  
  else if (mode == "R"){
      text("Current reference speed set to: " + sliderValue + " cm/s", 420, 305); 
  }
  
 if (buggyStatus.contains("Received")){    //debugging -------------------------------------------------------------------
         text(buggyStatus, 300, 300);    //print this out below the previous text
 }
 
 //---this is for the reference speed text box----------------------------------------------------------------------
  fill(255);
  stroke(0);
  //rect(30, 500, 250, 50);
  // Display the text inside the box
  fill(textColor);
  textSize(20);
  text("Change Reference Speed Here:", 30, 495);
 
  
   // --- Buggy speed box -------------------------------------------------------------------------------------------
  fill(statusBoxColor);
  rect(360, 500, 530, 50); //Status Box
  fill(textColor);
  textSize(20);
  text("Actual buggy speed:", 365, 520); 
  if (buggyStatus.contains("cm/s")){
    speed_status = buggyStatus;
  }
  text(speed_status, 540, 520); // Display the speed
  
  
  
  // --- Receive data from Arduino (over Wi-Fi) ---
  if (myClient.available() > 0) {
    String data = myClient.readStringUntil('\n');
    if (data!= null) {
      buggyStatus = data.trim(); // Update buggyStatus with received data
    }
  }
}

void mousePressed() {
  
   if (dist(mouseX, mouseY, map(sliderValue, sliderMin, sliderMax, sliderX, sliderX + sliderWidth), sliderY) < 10) {
    sliderActive = true;
   }
  
  if (start.isClicked(mouseX, mouseY)) {
    myClient.write("G\n");
  }
  if (stop.isClicked(mouseX, mouseY)) {
    myClient.write("S\n");
    
  }
  if (Mode2.isClicked(mouseX, mouseY)){
     myClient.write("F\n");
     mode = "F";
  }
  
  if (Mode1.isClicked(mouseX, mouseY)){
     myClient.write("R\n");
     mode = "R";
  }
  
}


void mouseDragged() {
  if (sliderActive) {
    // Constrain the value within the slider range
    sliderValue = map(mouseX, sliderX, sliderX + sliderWidth, sliderMin, sliderMax);
    sliderValue = constrain(sliderValue, sliderMin, sliderMax);
    
    // Send new value to the buggy if mode is "R"
    if ((mode == "R") && (sliderValue != oldValue)){//---------------------------------------------------------------------mayeb change
      myClient.write(sliderValue + "\n");
      oldValue = sliderValue;
    }
  }
}

void mouseReleased() {
  sliderActive = false;
}

// --- Button Class ---
class Button {
  int x, y, w, h; // Changed to int
  String label;

  Button(int x, int y, int w, int h, String label) { 
    this.x = x;
    this.y = y;
    this.w = w;
    this.h = h;
    this.label = label;
  }

  void displayStart() {
    fill(buttonColorStart);
    rect(x, y, w, h, 7.5);
    fill(buttonTextColor);
    textAlign(CENTER, CENTER);
    textSize(30);
    text(label, x + w / 2, y + h / 2);
    textAlign(LEFT, BASELINE);
  }

  void displayStop() {
    fill(buttonColorStop);
    rect(x, y, w, h, 7.5);
    fill(buttonTextColor);
    textAlign(CENTER, CENTER);
    textSize(30);
    text(label, x + w / 2, y + h / 2);
    textAlign(LEFT, BASELINE);
  }
  
  void displayMode2() {
     fill(buggyColor);
    rect(x, y, w, h, 7.5);    //do i need to change these?
    fill(buttonTextColor);
    textAlign(CENTER, CENTER);
    textSize(30);
    text(label, x + w / 2, y + h / 2);
    textAlign(LEFT, BASELINE);
  }
  
  void displayMode1() {
     fill(buggyColor);
    rect(x, y, w, h, 7.5);  //do i need to change these? no not here
    fill(buttonTextColor);
    textAlign(CENTER, CENTER);
    textSize(30);
    text(label, x + w / 2, y + h / 2);
    textAlign(LEFT, BASELINE);
  }
  
  

  boolean isClicked(float mx, float my) {
    return mx >= x && mx <= x + w && my >= y && my <= y + h;
  }
}
