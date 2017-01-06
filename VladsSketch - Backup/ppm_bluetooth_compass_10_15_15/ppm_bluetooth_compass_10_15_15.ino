 #include <TinyGPS++.h>
#include <SoftwareSerial.h>
//#include <HM55B_Compass.h>
#include <Wire.h>
//#include <stdlib.h> 
 


#include <HMC5883L.h>
#define COMPASSTOLERANCE 5
HMC5883L compass;
int error = 0;




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
#define slightLeft 1650
#define slightRight 1350
#define hardLeft 1900
#define hardRight 1100


#define neutral 1500
#define slowForward 1650 
#define fastForward 1650 
#define slowBackward 1350
#define fastBackward  1350


int ppm[chanel_number];
//PPM
int FRValue = 1500;
int LRValue = 1500;
///////////////////////////////////////////////////////






typedef struct waypoint {
  double latitude =0.0000000; 
  double longitude =0.0000000;
  float courseToWaypoint; 
  float distance; 
  int estimatedArrivalTime;  
 };


//GPS
//////////////////////////////////////////
static const int RXPin = 4, TXPin = 2;
static const uint32_t GPSBaud = 9600;
static void smartDelay(unsigned long ms);
//////////////////////////////////////



// The TinyGPS++ object
TinyGPSPlus gps;
SoftwareSerial ssGPS(RXPin, TXPin);
SoftwareSerial bt(6,7);//tx rx
//HM55B_Compass compass(8, 9, 10);


//BlueTooth
String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete
//String coordinateBuffer[11]; 
//int numberOfCoordinatesInBuffer = 0; 



void setup()
{
  Wire.begin();
    
  for(int i=0; i<chanel_number; i++){
    ppm[i]= default_servo_value;
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
 
  Serial.begin(250000);

  compass = HMC5883L(); // Construct a new HMC5883 compass.

  //Serial.println("Setting scale to +/- 1.3 Ga");
  error = compass.SetScale(1.3); // Set the scale of the compass.
  if (error != 0) // If there is an error, print it out.
    Serial.println(compass.GetErrorText(error));
 
    //Serial.println("Setting measurement mode to continous.");
  error = compass.SetMeasurementMode(Measurement_Continuous); // Set the measurement mode to Continuous
  if (error != 0) // If there is an error, print it out.
    Serial.println(compass.GetErrorText(error));


   
  //ssGPS.begin(GPSBaud);
  //while(!ssGPS); 
  bt.begin(2400);
  //while(!bt); 


  //compass.initialize();//start compass
  //delay(100); 

  
  //getGPSData();
  inputString.reserve(200);
  //Serial.println("Done"); 

  smartDelay(3000);
 /*
  pinMode(2,INPUT); 
  attachInterrupt(0,react,FALLING); 
 */ Serial.println("done initializing");
   
}


bool followingGPS = false;
waypoint waypoint[MAXWAYPOINTS];
int waypointCounter = 0;
int routeCounter = 0;  

//bool test = false;


//Daryl House Staight ahead
//waypoint waypoint  = { 37.695730, -97.423314 , 0 , 0 ,0};


int counter = 0; 
void loop()
{
  /*
  Serial.println(counter); 
  counter++; 
  if (counter > 10000)
    counter = 0; 
  delay(100); */

  
  if(followingGPS)
  {
      getGPSData();
      wichWay();
      bluetooth();
  }else
    bluetooth();
  
  
  /*
  Serial.print(LRValue); 
  Serial.print("   "); 
  Serial.println(FRValue); 
  */



  //Serial.print("FRValue = "); 
  //Serial.print(FRValue); 
  //Serial.print(" LRValue = "); 
  //Serial.print(LRValue);

   //Serial.println(waypointCounter); 
   
  
  /*
  Serial.print(waypoint[routeCounter].latitude);
  Serial.print("     "); 
   Serial.println(waypoint[routeCounter].longitude);


  Serial.print(routeCounter);
  Serial.print("     "); 
  Serial.print(waypointCounter);
  Serial.print("     Going to "); 
  Serial.print(waypoint[routeCounter].latitude); 
  Serial.print("   "); 
  Serial.print(waypoint[routeCounter].longitude); 
  Serial.print("    Current loc ");
  Serial.print(gps.location.lat(),6);
  Serial.print("   ");
  Serial.print(gps.location.lng(),6); 
  Serial.print("   Distance ");   
  Serial.println(TinyGPSPlus::distanceBetween(
      gps.location.lat(),gps.location.lng(),
      waypoint[routeCounter].latitude, 
      waypoint[routeCounter].longitude));

  Serial.println(waypoint[routeCounter].distance); 
  
*/
   
  //Serial.println(counter++);


 // if (counter > 100 )
   // counter =0;
   
  ppm[FowardReverse] = FRValue;
  ppm[LeftRight] = LRValue;

    
}


ISR(TIMER1_COMPA_vect){  //leave this alone
  static boolean state = true;
  
  TCNT1 = 0;
  
  if(state) {  //start pulse
    digitalWrite(sigPin, onState);
    OCR1A = PPM_PulseLen * 2;
    state = false;
  }
  else{  //end pulse and calculate when to start the next pulse
    static byte cur_chan_numb;
    static unsigned int calc_rest;
  
    digitalWrite(sigPin, !onState);
    state = true;

    if(cur_chan_numb >= chanel_number){
      cur_chan_numb = 0;
      calc_rest = calc_rest + PPM_PulseLen;// 
      OCR1A = (PPM_FrLen - calc_rest) * 2;
      calc_rest = 0;
    }
    else{
      OCR1A = (ppm[cur_chan_numb] - PPM_PulseLen) * 2;
      calc_rest = calc_rest + ppm[cur_chan_numb];
      cur_chan_numb++;
    }     
  }
}


void bluetooth()
{

  
  serialEvent();
if(stringComplete){
 Serial.println(inputString);  
if(inputString == "~forward\n"){
     FRValue = slowForward;
}else if(inputString == "~notforward\n"){
     FRValue = neutral;   
}else if(inputString == "~backward\n"){
     FRValue = slowBackward;
}else if(inputString == "~notbackward\n"){
     FRValue = neutral; 
}else if(inputString == "~left\n"){
     LRValue = hardLeft;
}else if(inputString == "~notleft\n"){
     LRValue = straight;
}else if(inputString == "~right\n"){
     LRValue = hardRight;
}else if(inputString == "~notright\n"){
     LRValue = straight;
}else if(inputString == "~start\n"){

     LRValue = straight; 
     FRValue = neutral;
     

}else if(inputString == "~startroute\n"){
    int test = 0; 

    while (test < waypointCounter)
    {
     //Serial.print(test); 
     Serial.print(waypoint[test].latitude,6); 
     Serial.print(","); 
     Serial.println(waypoint[test].longitude,6); 
     test++; 
    }
    
    
    routeCounter = 0;
    followingGPS = true;

}else if(inputString == "~stoproute\n"){
     followingGPS = false;
     LRValue = straight; 
     FRValue = neutral;
     routeCounter = 0; 
     waypointCounter = 0; 
      clearWaypointData();

}else if(inputString == "~uadlocation\n"){
    smartDelay(1000);
float x = 0, y = 0; 
   
while(x == 0 || y == 0)
{        
  smartDelay(1000);
 x = gps.location.lat();
 y = gps.location.lng(); 
    
}
      
    
     bt.print(x,6); 
     bt.print(",");
     bt.print(y,6); 
     bt.print("\n"); 
//bt.print("Hello"); 

/*
     Serial.print(x,6); 
     Serial.print(","); 
     Serial.println(y,6); 
  */   
     
}else if(inputString == "~select\n"){

      
}else if(inputString == "~a\n"){
     /*while(FRValue < 2000) 
     {
        ppm[FowardReverse] = FRValue;
        ppm[LeftRight] = LRValue;
        FRValue++;
        //Serial.println(FRValue);  
     }*/
     routeCounter++;
 
}else if(inputString == "~b\n"){
     while(FRValue > 1000) 
     {
        ppm[FowardReverse] = FRValue;
        ppm[LeftRight] = LRValue;
        FRValue--; 
        //Serial.println(FRValue); 
     }

}else if (isdigit(inputString[0])){


/*
      //store the coordinate in an array 
      coordinateBuffer[numberOfCoordinatesInBuffer] = inputString; 
      Serial.print("Number of Coordinates in buffer = "); 
      Serial.print(numberOfCoordinatesInBuffer);
      Serial.print("    "); 
      Serial.println(coordinateBuffer[numberOfCoordinatesInBuffer]);
      numberOfCoordinatesInBuffer++; 
*/


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
      latitude.remove(0,one+1);
      latitude.remove(two-2);
      longitude.remove(0,two+1); 

     
////////////////////////////////////////////////////////////////////////////////////
/*
      //this is used for strtod function
      char *ptr;
      //delay(20);
      //waypoint[counter].latitude = strtod(waypointNumber.c_str(),&ptr);
      waypoint[waypointCounter].latitude = strtod(latitude.c_str(),&ptr);
      //delay(10);
      waypoint[waypointCounter].longitude = strtod(longitude.c_str(),&ptr);
      //delay(20);
*/
/////////////////////////////////////////////////////////////////////////    



     
      waypoint[waypointCounter].latitude = (double)strtod(latitude.c_str(),NULL);
      delay(10);
      waypoint[waypointCounter].longitude = (double)strtod(longitude.c_str(),NULL);

   
    /*
      Serial.print("waypointNumber String = "); 
      Serial.print(waypointNumber); 
      Serial.print("  waypointNumber = "); 
      Serial.println(waypointCounter); 


      Serial.print("latitude String = "); 
      Serial.print(latitude); 
      Serial.print("  latitude = "); 
      Serial.println(waypoint[waypointCounter].latitude,6); 


      Serial.print("longitude String = "); 
      Serial.print(longitude); 
      Serial.print("  longitude = ");
      Serial.println(waypoint[waypointCounter].longitude,6); 
      
      Serial.println(); 
      Serial.println(); 

      */
      waypointCounter++; 
  //    Serial.println(waypointCounter);
      if (waypointCounter > 10)
        waypointCounter =0; 
  
}else {
     followingGPS = false;
     LRValue = straight; 
     FRValue = neutral;
}


inputString ="";
stringComplete = false;
}

}
  



void serialEvent() {
  

  //if(bt.isListening())
  bt.listen();
  while (bt.available()) {
    // get the new byte:
    char inChar = (char)bt.read();
    Serial.print(inChar); 
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }
}


void getGPSData()
{

  
  smartDelay(200);

  waypoint[routeCounter].distance =
    (unsigned long)TinyGPSPlus::distanceBetween(
      gps.location.lat(),
      gps.location.lng(),
      waypoint[routeCounter].latitude, 
      waypoint[routeCounter].longitude);
  //Serial.print(waypoint[routeCounter].distance);
  //printInt(distance, gps.location.isValid(), 9);

  waypoint[routeCounter].courseToWaypoint =
    TinyGPSPlus::courseTo(
      gps.location.lat(),
      gps.location.lng(),
      waypoint[routeCounter].latitude, 
      waypoint[routeCounter].longitude);

  //if (millis() > 5000 && gps.charsProcessed() < 10)
    //Serial.println(F("No GPS data received: check wiring"));

}


void clearWaypointData()
{
     int counter =0; 
     while (counter < MAXWAYPOINTS)
     {
        waypoint[counter].latitude = 0; 
        waypoint[counter].longitude =0; 
        waypoint[counter].courseToWaypoint = 0; 
        waypoint[counter].distance =0; 
        waypoint[counter].estimatedArrivalTime = 0; 
     }

  
}

  

void wichWay()
{
  
  
  MagnetometerRaw raw = compass.ReadRawAxis();
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

  if (angle < 0) 
  angle = 360+angle; 

  //angle += 180; 
  /*
  Serial.print("  Compas = "); 
  Serial.print(angle); 
  Serial.print("   Going to = "); 
  Serial.println(waypoint[routeCounter].courseToWaypoint); 
  Serial.print("      ");
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
/*
    Serial.print("   ");
    Serial.print(waypoint[routeCounter].distance);
    Serial.print("   ");
*/    
    if (waypoint[routeCounter].distance > 10){    //tells the car to go forwards fast as long as the car is at least 10 meters away from the way point
       FRValue = fastForward;
    }else if ((waypoint[routeCounter].distance <= 10 ) && (waypoint[routeCounter].distance  > 2)){    // slows the car down as it approaches the way point
        FRValue = slowForward;
    }else if(waypoint[routeCounter].distance <= 2){    // once within 2 meters of way point it increases the counter for the next point if its the last point it ends the trip
      routeCounter++; 
      if (routeCounter >= waypointCounter){
        FRValue = neutral; 
        LRValue = straight;
        followingGPS = false;
        clearWaypointData();
      }
      
    }
    
    //else {
     // FRValue = neutral;    //idk if we actually need this i think its here for a safety if the program freaks out
    //} 
 
   
    
      
}



// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    Wire.requestFrom(8, 1);
    while ((Wire.available()))
      gps.encode(Wire.read());
  } while (millis() - start < ms);

}


/*

// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (ssGPS.available())
      gps.encode(ssGPS.read());
  } while (millis() - start < ms);
}
*/
void react() 
{
  Serial.print("jere"); 
    followingGPS = false;
     LRValue = straight; 
     FRValue = neutral;
     routeCounter = 0; 
     waypointCounter = 0; 
      clearWaypointData(); 
  
}

