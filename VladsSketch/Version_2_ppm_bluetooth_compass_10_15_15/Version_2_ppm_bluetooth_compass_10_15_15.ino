#include <TinyGPS++.h>
#include <SoftwareSerial.h>
//#include <HM55B_Compass.h>
#include <Wire.h>
//#include <stdlib.h>
#include <HM55B_Compass.h>
HM55B_Compass compass(8, 9, 10);


//#include <HMC5883L.h>
#define COMPASSTOLERANCE 7
//HMC5883L compass;
//int error = 0;




//          PPM
///////////////////////////////////////////////////
#define chanel_number 6  //set the number of chanels
#define default_servo_value 1500  //set the default servo value
#define PPM_FrLen 22000  //set the PPM frame length in microseconds (1ms = 1000µs)
#define PPM_PulseLen 300  //set the pulse length
#define onState 1  //set polarity of the pulses: 1 is positive, 0 is negative
#define sigPin 4 //set PPM signal output pin on the arduino
#define FowardReverse 0 //set foward and reverse channel
#define LeftRight 1 //set left and right channel
/*
  #define straight 1500
  #define slightLeft 1250
  #define slightRight 1750
  #define hardLeft 1100
  #define hardRight 1900
*/
#define MAXWAYPOINTS 15

#define straight 1450


//#define straight 1500
#define slightLeft 1550
#define slightRight 1350
#define hardLeft 1900
#define hardRight 1100


#define neutral 1500
int slowForward =1700;
int fastForward = 1700;
int slowBackward =1300;
int fastBackward = 1300;


int ppm[chanel_number];
//PPM
int FRValue = 1500;
int LRValue = 1500;
///////////////////////////////////////////////////////






typedef struct waypoint {
  double latitude = 0.0;
  double longitude = 0.0;
  float courseToWaypoint;
  float distance;
 // int estimatedArrivalTime;
};


//GPS
//////////////////////////////////////////
static const int RXPin = 4, TXPin = 2;
static const uint32_t GPSBaud = 9600;
static void smartDelay(unsigned long ms);
//////////////////////////////////////



// The TinyGPS++ object
TinyGPSPlus gps;
//SoftwareSerial ssGPS(RXPin, TXPin);


//SoftwareSerial mySerial (rxPin, txPin);
//Bluetooth module(RX to 7)////////////////////////////////////////
//                (TX to 6)
SoftwareSerial bt(6, 7 );
//HM55B_Compass compass(8, 9, 10);


//BlueTooth
String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

void setup()
{
  Wire.begin();
  delay(1000);

  /////////////////////////////////////////////////////////
  //Serial.begin(250000);
  //while(!Serial);
  smartDelay(5000);
  




/*
  compass = HMC5883L(); // Construct a new HMC5883 compass.
  delay(10);
  //Serial.println("Setting scale to +/- 1.3 Ga");
  
  error = compass.SetScale(1.3); // Set the scale of the compass.
  delay(10);
  //if (error != 0) // If there is an error, print it out.
    //Serial.println(compass.GetErrorText(error));

 // Serial.println("Setting measurement mode to continous.");
  error = compass.SetMeasurementMode(Measurement_Continuous); // Set the measurement mode to Continuous
  delay(10);
  //if (error != 0) // If there is an error, print it out.
    //Serial.println(compass.GetErrorText(error));
*/

compass.initialize();

  
  bt.begin(1200);
  delay(1000);
  //bt.print("AT+BAUD1");
  //delay(1000); 
//  bt.stop();
//  bt.begin(1200);
  
  
  //while(!bt);
  //bt.listen(); 


  inputString.reserve(200);
 
///////////////////////////////////////////////////////////////////////////
  for (int i = 0; i < chanel_number; i++) {
    ppm[i] = default_servo_value;
  }

  pinMode(sigPin, OUTPUT);
  digitalWrite(sigPin, !onState);  //set the PPM signal pin to the default state (off)

  cli();
  TCCR1A = 0; // set entire TCCR1 register to 0
  TCCR1B = 0;

  OCR1A = 100;  // compare match register, change this
  TCCR1B |= (1 << WGM12);  // turn on CTC mode
  TCCR1B |= (1 << CS11);  // 8 prescaler: 0,5 microseconds at 16mhz
  TIMSK1 |= (1 << OCIE1A); // enable timer compare interrupt
  sei(); 

 // Serial.println("done initializing");  
}


bool followingGPS = false;
waypoint waypoint[MAXWAYPOINTS];
int waypointCounter = 0;
int routeCounter = 0;

int counter = 0;
void loop()
{
  //Serial.println("Here"); 

  if (followingGPS)
  {
    getGPSData();
    wichWay();
    bluetooth();
  } else
    bluetooth();

  ppm[FowardReverse] = FRValue;
  ppm[LeftRight] = LRValue;

 // Serial.print(FRValue); 
 // Serial.print("       ");
 // Serial.println(LRValue); 
  


}




void bluetooth()
{
   //Serial.println("In the bluetooth "); 
   

  serialEvent();
  if (stringComplete) {
   //Serial.println(inputString);
    if (inputString == "~forward\n") {
      FRValue = slowForward;
    } else if (inputString == "~notforward\n") {
      FRValue = neutral;
    } else if (inputString == "~backward\n") {
      FRValue = slowBackward;
    } else if (inputString == "~notbackward\n") {
      FRValue = neutral;
    } else if (inputString == "~left\n") {
      LRValue = hardLeft;
    } else if (inputString == "~notleft\n") {
      LRValue = straight;
    } else if (inputString == "~right\n") {
      LRValue = hardRight;
    } else if (inputString == "~notright\n") {
      LRValue = straight;
    } else if (inputString == "~start\n") {
      LRValue = straight;
      FRValue = neutral;
    } else if (inputString == "~startroute\n") {
     /* 
      int test = 0;
      while (test < waypointCounter)
      {
        //Serial.print(test);
        Serial.print(waypoint[test].latitude, 6);
        Serial.print(",");
        Serial.println(waypoint[test].longitude, 6);
        test++;
      }*/
      
      if (waypointCounter > 0)
      {
        routeCounter = 0;
        
        followingGPS = true;
      } else
      followingGPS = false; 
      
    } else if (inputString == "~stoproute\n") {
      followingGPS = false;
      LRValue = straight;
      FRValue = neutral;
      //clearWaypointData();

    }else if (inputString == "~clearroute\n") {
      
      //clearWaypointData();

    } else if (inputString == "~uadlocation\n") {
      
      /*
      smartDelay(1000);
      float x = 0, y = 0;

      x = gps.location.lat();
      y = gps.location.lng();

      bt.print(x, 6);
      bt.print(",");
      bt.print(y, 6);
      bt.print("\n");
*/ 
     /*
      Serial.print("Current location  "); 
      Serial.print(gps.location.lat(), 6);
      Serial.print(",");
      Serial.print(gps.location.lng(), 6);
      Serial.print("\n");
   */ 
      
      bt.print(gps.location.lat(), 6);
      bt.print(",");
      bt.print(gps.location.lng(), 6);
      bt.print("\n");

      smartDelay(1000); 

      while(gps.location.lat() == 0)
      {
       // Serial.println("here");
        smartDelay(1000); 
      }

    
    } else if (inputString == "~select\n") {


    } else if (inputString == "~a\n") {
        slowForward += 50;
        fastForward += 50;
        slowBackward  -=50;
        fastBackward  -= 50;

        if(slowForward > 2000)
          {
            slowForward = 1600; 
            fastForward = 1600; 
            slowBackward = 1400; 
            fastBackward = 1400; 
          }


    } else if (inputString == "~b\n") {
        slowForward -= 50;
        fastForward -= 50;
        slowBackward  +=50;
        fastBackward  += 50;

        if(slowForward < 1600)
          {
            slowForward = 1600; 
            fastForward = 1600; 
            slowBackward = 1400; 
            fastBackward = 1400; 
          }
    } else if (isdigit(inputString[0])) {

      String waypointNumber = inputString;
      //delay(10);
      String latitude = inputString;
      //delay(10);
      String longitude = inputString;
      //delay(10);
      //find where the first comma is at
      int one = inputString.indexOf(',');
      //find where the second comma is at
      int two = inputString.lastIndexOf(',');

      //trim the string to just the number
      waypointNumber.remove(one);
      latitude.remove(0, one + 1);
      latitude.remove(two - 2);
      longitude.remove(0, two + 1);

      waypoint[waypointCounter].latitude = (double)strtod(latitude.c_str(), NULL);
      delay(10);
      waypoint[waypointCounter].longitude = (double)strtod(longitude.c_str(), NULL);

      waypointCounter++;
      if (waypointCounter > 10)
        waypointCounter = 0;
    } else {
      followingGPS = false;
      LRValue = straight;
      FRValue = neutral;
    }

    inputString = "";
    stringComplete = false;
  }

}




void serialEvent() {

  while (bt.available()) {
    // add it to the inputString:

  char inChar = (char)bt.read();
    
    inputString += inChar; 

   // Serial.print(inChar); 
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
 
   
    //Serial.print(inChar); 
    
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}


void getGPSData()
{


  smartDelay(500);
  waypoint[routeCounter].distance =
    (unsigned long)TinyGPSPlus::distanceBetween(
      gps.location.lat(),
      gps.location.lng(),
      waypoint[routeCounter].latitude,
      waypoint[routeCounter].longitude);

  waypoint[routeCounter].courseToWaypoint =
    TinyGPSPlus::courseTo(
      gps.location.lat(),
      gps.location.lng(),
      waypoint[routeCounter].latitude,
      waypoint[routeCounter].longitude);

// waypoint[routeCounter].distance = 100; 
// waypoint[routeCounter].courseToWaypoint = 200; 
    
}


void clearWaypointData()
{
  int counter = 0;
  while (counter <= waypointCounter)
  {
    waypoint[counter].latitude = 0;
    waypoint[counter].longitude = 0;
    waypoint[counter].courseToWaypoint = 0;
    waypoint[counter].distance = 0;
    //waypoint[counter].estimatedArrivalTime = 0;
    counter++; 
  }

  waypointCounter = 0; 
  routeCounter = 0;


}



void wichWay()
{

    
  // MagnetometerRaw raw = compass.ReadRawAxis();
 if (waypointCounter > 0)
 { 
  /*
  MagnetometerScaled scaled = compass.ReadScaledAxis();
  int MilliGauss_OnThe_XAxis = scaled.XAxis;// (or YAxis, or ZAxis)
  float heading = atan2(scaled.YAxis, scaled.XAxis);
  float declinationAngle = 0.0346;
  heading += declinationAngle;

  // Correct for when signs are reversed.
  if (heading < 0)
    heading += 2 * PI;

  // Check for wrap due to addition of declination.
  if (heading > 2 * PI)
    heading -= 2 * PI;

  // Convert radians to degrees for readability.
  int angle = heading * 180 / M_PI;
*/
  
  int angle = compass.read();
  if (angle == HM55B_Compass::NO_VALUE)
    return; 
  if (angle < 0)
    angle = 360 + angle;
    

 
/*  Serial.print("Angle   "); 
  Serial.print(angle); 
  Serial.print("   Going to   "); 
  Serial.print(waypoint[routeCounter].courseToWaypoint); 
  Serial.print("   Waypoint number   ");
  Serial.print(routeCounter); 
  Serial.print("   number of waypoints    ");
  Serial.print(waypointCounter);
  Serial.print("   distance    "); 
  Serial.println(waypoint[routeCounter].distance);
  
  */ 

    float newCourse;
  if ((waypoint[routeCounter].courseToWaypoint <= angle + COMPASSTOLERANCE) && 
  (waypoint[routeCounter].courseToWaypoint >= angle - COMPASSTOLERANCE)){
  //  Serial.print("Go straight");
    LRValue = straight;//go straight
  }
  else if (waypoint[routeCounter].courseToWaypoint <= 180){//---------------- waypoint is in 2/3 qudrant 
    newCourse = angle - waypoint[routeCounter].courseToWaypoint ;
    if (newCourse < 0)
      newCourse += 360;
    if (newCourse == 180){
      //Serial.print("Turn around");
      LRValue = hardLeft;
    } else if (newCourse < 180) 
      {

        LRValue = slightLeft;
      }
        else if (newCourse > 180) 
      {
        LRValue = slightRight;
      }}
  else if (waypoint[routeCounter].courseToWaypoint > 180){//-----------------------waypoint is in 1/4 qudrant 
    newCourse = (angle + (360 - waypoint[routeCounter].courseToWaypoint)); 
    if (newCourse > 360) 
      newCourse -= 360; 
    if (newCourse == 180){
      //Serial.print("Turn around"); 
      LRValue = hardLeft; 
    }else if (newCourse < 180) 
      {
        //Serial.print("Go left  "); 
        //Serial.print(newCourse);  
        //Serial.println(); 
        LRValue = slightLeft;
      }
    else if (newCourse > 180) 
      {
        //Serial.print("Go right   "); 
        //Serial.print(360 - newCourse); 
        //Serial.println(); 
        LRValue = slightRight;
      }}
 
  if (waypoint[routeCounter].distance > 10) {   //tells the car to go forwards fast as long as the car is at least 10 meters away from the way point
    FRValue = fastForward;
  } else if ((waypoint[routeCounter].distance <= 10 ) && (waypoint[routeCounter].distance  > 5)) {   // slows the car down as it approaches the way point
    FRValue = slowForward;
  } else if (waypoint[routeCounter].distance <= 5) {  // once within 2 meters of way point it increases the counter for the next point if its the last point it ends the trip
    //Serial.println("here");
    routeCounter++;
    if (routeCounter >= waypointCounter) {
      FRValue = neutral;
      LRValue = straight;
      followingGPS = false;
      routeCounter = 0;
      waypointCounter = 0;  
      
      //clearWaypointData();
    }
  }
 }
  

   
}



// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms)
{

  unsigned long start = millis();
  do 
  {
    Wire.requestFrom(8, 1, false);
    delay(1); 
    while ((Wire.available()))
    {
     char x = (char)Wire.read(); 
    // Serial.print(x); 
     gps.encode(x);

    }
  } while (millis() - start < ms);

}



ISR(TIMER1_COMPA_vect) { //leave this alone
  static boolean state = true;

  TCNT1 = 0;

  if (state) { //start pulse
    digitalWrite(sigPin, onState);
    OCR1A = PPM_PulseLen * 2;
    state = false;
  }
  else { //end pulse and calculate when to start the next pulse
    static byte cur_chan_numb;
    static unsigned int calc_rest;

    digitalWrite(sigPin, !onState);
    state = true;

    if (cur_chan_numb >= chanel_number) {
      cur_chan_numb = 0;
      calc_rest = calc_rest + PPM_PulseLen;//
      OCR1A = (PPM_FrLen - calc_rest) * 2;
      calc_rest = 0;
    }
    else {
      OCR1A = (ppm[cur_chan_numb] - PPM_PulseLen) * 2;
      calc_rest = calc_rest + ppm[cur_chan_numb];
      cur_chan_numb++;
    }
  }
}


