#include <WiFiS3.h>
//need to change these for the wifi network which we will use
const char* ssid = "Galaxy A41DACE";
const char* password = "peter042";


WiFiServer server(5200);  //using port no.5200 - i believe this is a custom arduino port


float circumference = 19.5;
const float TICKS_PER_REV = 3.8f;
const float CM_PER_TICK = circumference / TICKS_PER_REV;
const int MIN_PWM = 50; //Minimum PWM value
const int MAX_PWM = 190; //Maximum PWM Value


const int LEYE = A3;
const int REYE = A4;
int reportSpeed = 0;
int stateL = 0;
int stateR = 0;


int isGo = 1;   //these are used to ensure that the loops do not continuously output direction, but rather only when it changes.
int isLeft = 1;
int isRight = 1;
int isStop = 1;
int isOFF = 0;
int isONtemp = 0;


String state; //this is so that recievedData doesnt overide the Go and stop loops
String mode; //this is so that recievedData doesnt overide things when ref speed is inputted


float prev_distance = 0;
float speed = 0;
float receivedValue = 0;


int LeftSpeed = 0;
int RightSpeed = 0;


//these are the pin positons for the trig and echo (ie. input and output) from the ultrasonic sensor
const int US_TRIG = 9;
const int US_ECHO = 8;


class Encoder{
  public:
    volatile int encoderPos = 0;
    volatile int prevTicks = 0;

    volatile float eDistance = 0;
    volatile float eSpeed = 0;
    volatile float ds;

    int ePin;

    void distance(){
      eDistance = (encoderPos / TICKS_PER_REV) * circumference;
    }
};


class USsensor {
  public:
    int US_TRIG;
    int US_ECHO;

    volatile int distance;
    volatile long duration;

    int readDist(){
      digitalWrite( US_TRIG, LOW );
      delayMicroseconds(2);

      digitalWrite( US_TRIG, HIGH );
      delayMicroseconds( 10 );

      digitalWrite( US_TRIG, LOW );

      duration = pulseIn(US_ECHO, HIGH);
      distance = duration * 0.0343 / 2;
      return distance;
    }
};


class Motor {
  public:
    int lPin;
    int rPin;
    int sPin;
    int speed = MIN_PWM;

    void forward(){
      digitalWrite(lPin, LOW);
      digitalWrite(rPin, HIGH);
    }

    void reverse(){
      digitalWrite(lPin, HIGH);
      digitalWrite(rPin, LOW);
    }

    void mStop(){
      digitalWrite(lPin, LOW);
      digitalWrite(rPin, LOW);
    }

    void changeSpeed(int mSpeed){
      speed = constrain(mSpeed, 0, MAX_PWM);
      analogWrite(sPin, speed);
    }
};


bool isNumeric(String str) {
  if (str.length() == 0) return false;
  bool decimalPoint = false;
  int startIndex = (str[0] == '-') ? 1 : 0; // Allow negative numbers

  for (int i = startIndex; i < str.length(); i++) {
    if (str[i] == '.') {
      if (decimalPoint) return false; // Only one decimal point allowed
      decimalPoint = true;
    } else if (!isDigit(str[i])) {
      return false; // Contains non-numeric character
    }
  }
  return true;
}


//creating objects for the 2 motors (left and right) as well as for the ultrasonic sensor
Motor LMOTOR;
Motor RMOTOR;
USsensor US;
Encoder lEncoder;
Encoder rEncoder;


volatile int lEncoderPos = 0;
volatile int rEncoderPos = 0;


unsigned long lastM = 0;

//Initialise to 0.5 so we don't divide by 0
volatile float dtL = 0.5;
volatile float dtR = 0.5;


volatile long prevTimeL = 0;
volatile long prevTimeR = 0;

//PID Constants
float Kp = 1, Ki = 0.2, Kd = 0.05; 

float refSpeed = 0;
float targetSpeed;

//PID Variables
float prevLeftError = 0, prevRightError = 0;
float leftIntegral = 0, rightIntegral = 0;


unsigned long lastPIDTime = 0;


void PID(int refSpeed, Motor& motor, Encoder& encoder, float& prevError, float& integral, float dt){ //PID Control function
  if(dt < 1){

    float error = refSpeed - encoder.eSpeed;
    integral += error * dt;
    integral = constrain(integral, -20, 20);
    float derivative = (error - prevError) / dt;

    if(abs(error) < 0.5){
      integral = 0;
    }
    float output = Kp * error + Ki * integral + Kd * derivative;

    int newSpeed = motor.speed + (int)(output);
    motor.changeSpeed(constrain(newSpeed, MIN_PWM, MAX_PWM));

    prevError = error;  
  }
}


volatile long lastTickL = 0;
volatile long lastTickR = 0;



void isIdle(Motor& motor, Encoder& encoder, volatile long& lastTick){ //Check if motor is idle and give a small boost if it is
  unsigned long currentTick = micros();
  if(currentTick - lastTick > 500000){
    encoder.eSpeed = 0;    
    motor.changeSpeed(motor.speed + 50);
  }
}


void rEncoderISR(){
  unsigned long currentTime = micros();
  dtR = (currentTime - prevTimeR) / 1000000.0;
  if(dtR > 0.02){ //Ignore very small dt
    rEncoder.eSpeed = CM_PER_TICK / dtR;
  }

  rEncoder.encoderPos += 1;
  prevTimeR = currentTime;
  lastTickR = currentTime;
}


void lEncoderISR(){
  unsigned long currentTime = micros();
  dtL = (currentTime - prevTimeL) / 1000000.0;
  if(dtL > 0.02){
    lEncoder.eSpeed = CM_PER_TICK / dtL;
  }
 
  lEncoder.encoderPos += 1;
  prevTimeL = currentTime;
  lastTickL = currentTime;
}


void setup() {
  Serial.begin(9600);
  WiFi.begin(ssid, password);
   
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);  
    Serial.println("Connecting...");
  }

  Serial.println("Connected to WiFi!"); //this will print out when the arduino connects - which will also show the IP address
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  server.begin();

  //Initialise and Change mode for all pins
  LMOTOR.lPin = 7;
  LMOTOR.rPin = 6;
  LMOTOR.sPin = 5;  

  RMOTOR.lPin = 12;
  RMOTOR.rPin = 13;
  RMOTOR.sPin = 11;

  US.US_TRIG = 8;
  US.US_ECHO = 9;

  lEncoder.ePin = 2;
  rEncoder.ePin = 3;

  pinMode( 5, OUTPUT);
  pinMode( 7, OUTPUT);
  pinMode( 6, OUTPUT);

  pinMode( 13, OUTPUT);
  pinMode( 12, OUTPUT);
  pinMode( 11, OUTPUT);
 
  pinMode( LEYE, INPUT );
  pinMode( REYE, INPUT );

  pinMode(US.US_TRIG, OUTPUT);
  pinMode(US.US_ECHO, INPUT);

  pinMode(lEncoder.ePin, INPUT_PULLUP);
  pinMode(rEncoder.ePin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(2), lEncoderISR, RISING);
  attachInterrupt(digitalPinToInterrupt(3), rEncoderISR, RISING);

  LeftSpeed = 0;    //setting back to zero just to be sure
  RightSpeed = 0;
  prev_distance = 0;
  receivedValue = 0;
}


void loop() { //this is the main loop which controls the buggy
  String receivedData = "null";   //this could be set to anything but makes it so we need to press go button to even start buggy
  String temp;
  delay (5000);
  Serial.println("Waiting for a client...");     //this is also for debugging - prints this on repeat when not connected - probably keep
  Serial.println(WiFi.localIP());
  WiFiClient client = server.available();     //this will get a connected client (i.e the processing GUI - which will take the role of cleint)
 
  if (client) {     //this checks if the client is connected - will be executed when it is  
    Serial.println("Client Connected!");    //debugging kinda  

    while (client.connected()) {    //this is always true if above if statement is true
       
      if (client.available() > 0) {    //is true when there is data sent from GUI to this
        receivedData = client.readStringUntil('\n');      // Read until newline
        receivedData.trim();    // Remove any trailing newline or spaces
        Serial.println("Received: " + receivedData);    // Print received data - kinda debugging would be cleaner without
      }

      if ((receivedData == "G") || (receivedData == "S")){ //ensure that go and stop arent overridden by mode changes etc.
        state = receivedData;
      }
      if ((receivedData == "F") || (receivedData == "R")){ //ensure that go and stop arent overridden by mode changes etc.
        mode = receivedData;
      }

      if (isNumeric(receivedData)) {   //this is such that the reference speed isnt overriden by other data sent over
        receivedValue = receivedData.toFloat();
        refSpeed = receivedValue;
      }

      if ((state == "G")){  //this is for the stop start button - has an else for the stop at the end
        if ((isOFF==1) || (isONtemp == 0)){   //this is simply just to output the message when start button is pressed, nothing more
          Serial.println("GO - TURNED ON");
          client.println("GO - TURNED ON");
          isOFF = 0;
          isONtemp = 1;
        }

        unsigned long currentM = millis();
        volatile int dist;

        if(currentM - lastM > 250){   //this is for the ultrasonic sensor
          dist = US.readDist();
          lEncoder.distance();
          client.println(String(lEncoder.eDistance) + " cm (this journey)");  //have gui print out distance travelled
          lastM = currentM;
        }

        bool lState = digitalRead(LEYE);
        bool rState = digitalRead(REYE);
        int max_dist = 10;  //this will be used so that we dont need to change all relevant values when we want to change stopping distance
            
        if(dist > max_dist){    //ie dont move if we have an obstacle within minimum distance    
          if ((mode == "F")){ //this will be for the following mode2------------------------------------------------------------------------------
            if(dist < max_dist*3){    
              targetSpeed = constrain(map(dist, max_dist, max_dist * 3, 10, 20), 10, 20);
            }

            else{
              targetSpeed = 20; //is this something we are required to have? could we not just set it to a value we know will work
            }
            if(currentM - lastPIDTime >= 100){
              isIdle(LMOTOR, lEncoder, lastTickL);
              isIdle(RMOTOR, rEncoder, lastTickR); 


              PID(targetSpeed, LMOTOR, lEncoder, prevLeftError, leftIntegral, dtL);
              PID(targetSpeed, RMOTOR, rEncoder, prevRightError, rightIntegral, dtR);
              lastPIDTime = currentM;
            }

            if (lState == HIGH && rState == HIGH ) {
              LMOTOR.forward();
              RMOTOR.forward();
              
            }

            else if (lState == HIGH && rState == LOW) {   //go right function
            
              LMOTOR.changeSpeed(165);
              RMOTOR.changeSpeed(130); 

              isIdle(LMOTOR, lEncoder, lastTickL);
              isIdle(RMOTOR, rEncoder, lastTickR);

              LMOTOR.forward();
              RMOTOR.reverse();

            } else if (lState == LOW && rState == HIGH ) {  //go left function

              RMOTOR.changeSpeed(130); 
              LMOTOR.changeSpeed(165); 

              isIdle(LMOTOR, lEncoder, lastTickL);
              isIdle(RMOTOR, rEncoder, lastTickR);

              LMOTOR.reverse();
              RMOTOR.forward();

            }
        
            client.println("current distance from object is: " + String(dist) + " cm");
          }


          else if ((mode == "R")){ //this will be for the reference speed mode1----------------------------------------------------------------------------
        
            targetSpeed = refSpeed;

            if(targetSpeed >= 1){
              if(currentM - lastPIDTime >= 100){
                isIdle(LMOTOR, lEncoder, lastTickL);
                isIdle(RMOTOR, rEncoder, lastTickR);

                PID(targetSpeed, LMOTOR, lEncoder, prevLeftError, leftIntegral, dtL);
                PID(targetSpeed, RMOTOR, rEncoder, prevRightError, rightIntegral, dtR);
                lastPIDTime = currentM;
              }

              if (lState == HIGH && rState == HIGH ) {

                LMOTOR.forward();
                RMOTOR.forward();
              
              } 
              else if (lState == HIGH && rState == LOW) {   //go right function

                RMOTOR.changeSpeed(130); 
                LMOTOR.changeSpeed(165); 

                isIdle(LMOTOR, lEncoder, lastTickL);
                isIdle(RMOTOR, rEncoder, lastTickR);

                LMOTOR.forward();
                RMOTOR.reverse();


              } else if (lState == LOW && rState == HIGH ) {  //go left function
                
                RMOTOR.changeSpeed(130); 
                LMOTOR.changeSpeed(165); 
              
                isIdle(LMOTOR, lEncoder, lastTickL);
                isIdle(RMOTOR, rEncoder, lastTickR);

                LMOTOR.reverse();
                RMOTOR.forward();
              }
            }
            else{
              RMOTOR.mStop();
              LMOTOR.mStop();

              lEncoder.eSpeed = 0;
              rEncoder.eSpeed = 0;
            }
          }

          reportSpeed = lEncoder.eSpeed;
          Serial.println(reportSpeed);
          client.println(String(reportSpeed) + " cm/s");
          client.println(String(LMOTOR.speed));
          //these 4 if statements are striclty for output to the client and to the serial monitor - such that the movement functions constanlty run        
          if( digitalRead( LEYE ) == HIGH && (digitalRead( REYE ) == HIGH && (isGo ==1 ))){
            Serial.println("Forward");
            client.println("Going Forward");

            isGo = 0;
            isLeft =1;
            isRight = 1;
            isStop = 1;

          }else if(digitalRead( LEYE ) == LOW && (digitalRead( REYE ) == LOW && (isStop==1))){
            Serial.println("Stop ");
            client.println("Stopped");

            isGo = 1;
            isLeft =1;
            isRight = 1;
            isStop = 0;
          }

          if( digitalRead( REYE ) == LOW &&  (digitalRead( LEYE ) == HIGH) && (isRight == 1)){
            Serial.println("Right  ");
            client.println("Turning Right");

            isGo = 1;
            isLeft =1;
            isRight = 0;
            isStop = 1;

          }else if(digitalRead( REYE ) == HIGH && (digitalRead( LEYE ) == LOW) && (isLeft == 1)){
            Serial.println("Left ");
            client.println("Turning Left");

            isGo = 1;
            isLeft = 0;
            isRight = 1;
            isStop = 1;
          } 
        }  //end of "if(dist > max_dist)"  condition

        else if(isStop == 1){   //otherwise (ie. within stopping distance) check that we arent stopped already and then execute
          LMOTOR.mStop();
          RMOTOR.mStop();

          lEncoder.eSpeed = 0;
          rEncoder.eSpeed = 0;
        
          Serial.println("stopping for obstacle at " + String(dist) + "cm distance (within "+ String(max_dist)+ "cm minimum)");
          client.println("stopping for obstacle at " + String(dist) + "cm distance (within "+ String(max_dist)+ "cm minimum)");

          client.println(String(0) + " cm/s");
          client.println(String(0));
        
          isGo = 1;
          isLeft =1;
          isRight = 1;
          isStop = 0;   //only need to do this once so no seperate function for these
        }
      }   //end of "if ((receivedData == "G")){" condition

      else if ((state == "S") && (isOFF == 0)){    //otherwise (i.e recievedData != G) check if it is S and that we havent already executed this code
        Serial.println("STOP - TURNED OFF");
        client.println("STOPPED - TURNED OFF. Distance Covered: " + String(lEncoder.eDistance) + " cm");
     
        LMOTOR.mStop();
        RMOTOR.mStop();

        client.println(String(0) + " cm/s");
        client.println(String(0));

        lEncoder.encoderPos = 0;
        rEncoder.encoderPos = 0;
        lEncoder.eSpeed = 0;
        rEncoder.eSpeed = 0;
        isOFF = 1;    //to ensure that we dont loop through this condition and repeatedly print out the text but rather execute it once
      }
      delay(10);
    }   //end of "while(client.connected)" condition/loop
  }   //end of "if(client)" statement
}   //end of loop
