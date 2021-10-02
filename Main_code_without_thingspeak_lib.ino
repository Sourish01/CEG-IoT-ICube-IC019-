#include <ESP8266WiFi.h>

#define PI 3.1415926535897932384626433832795


const char ssid[20] = "VENKATAKSH";   
const char pass[20] = "09May2016";

const char server[30] = "api.thingspeak.com";
String writeAPIKey = "SYYOUXDGPRHQKNPZ";
unsigned long myChannelNumber = 1513842; // enter the channel number here

long currentMillis = 0;
long previousMillis = 0;
int interval = 10;
float calibrationFactor = 7.5;
volatile byte pulseCount,pulseCount_1;
byte pulse1Sec = 0, pulse1Sec_1=0 ;
double flowRate,flowRate_1;
double flowLitres,flowLitres_1;
double totalLitres,totalLitres_1 = 0.0;
double init_duration = 401.00;
long currentMillis_1 = 0;
long previousMillis_1 = 0;

const int pulseRead = 2;    //data pin of Flow Sensor
const int triggerPin = 13;  //trigger pin of UL sensor
const int echoPin = 14;     //echo pin of UL sensor

double tankHeight = 0.226524;   //distance from sensor to base in metres...
double h = 0.1054725;   //height in which the Ultrasonic sensor is place above the water holding capacity of the tank....
double d = 0.014964;   //distance from tap to base
double r  = 0.12;  // radius of the tank ....
double tankCapacity = 6.45;   // tank's usable capacity in litres....
float waterused = 0.0;  // to store the usage of water

WiFiClient client;    // configuring ESP8266 as HTTP client


void IRAM_ATTR pulseCounter()
{
  pulseCount++;
}


void IRAM_ATTR pulseCounter_1()
{
  pulseCount_1++;
}



void setup() {
  // put your setup code here, to run once:
    pulseCount = 0;
    flowRate = 0.0;
    previousMillis = 0;
    pulseCount_1 = 0;
    flowRate_1 = 0.0;
    previousMillis_1 = 0.0;

    WiFi.mode(WIFI_STA); 
    pinMode(pulseRead,INPUT_PULLUP); 
    pinMode(triggerPin,OUTPUT);
    pinMode(echoPin,INPUT);
    Serial.begin(115200);  // here 115200 is the baud rate for the esp module...
    Serial.println("Starting serial monitor");
    attachInterrupt(digitalPinToInterrupt(pulseRead), pulseCounter, RISING); //connect pulseRead pulsed voltage to pulsecounter function  
    attachInterrupt(digitalPinToInterrupt(pulseRead) , pulseCounter_1, RISING);//    
}





double waterFlownTap(){
      // code for water flow meter starts from here....
   currentMillis = millis();
  if (currentMillis - previousMillis > interval) 
  {
    pulse1Sec = pulseCount;
    pulseCount = 0;
    flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
    previousMillis = millis();
    flowLitres = (flowRate / 60);
    if(flowLitres>0.005){ //to avoid recording air flow in pipe
      totalLitres += (flowLitres*1.7);// 1.7 factor chosen to account for lack of pressurized flow
      Serial.print(totalLitres);
      Serial.println( " tap flow consumption");
    }
    else{
      Serial.println("no water flow");
    }
  }
  return totalLitres;
}






float overFlow(){
        // code for water flow meter starts from here....
    currentMillis = millis();
  if (currentMillis - previousMillis > interval) 
  {
    pulse1Sec = pulseCount;
    pulseCount = 0;
    flowRate = ((1000.0 / (millis() - previousMillis)) * pulse1Sec) / calibrationFactor;
    previousMillis = millis();
    flowLitres = (flowRate / 60);
    totalLitres += (flowLitres*1.7);
    return totalLitres;
  }
  return 0.0;
  
}






double waterUsed(double duration){
  double distance = 0.0;
  double waterLevel;  // water level in meter's inside the tank...
  double volumeOfWater;  // amount of water in litres...
  distance = (duration/2)*0.000343;// distance in meters
  waterLevel = tankHeight-distance-d;
  volumeOfWater = (PI)*(r*r)*waterLevel*1000 ; // which gives the amount of water present inside the tank in litres
  waterused = tankCapacity - volumeOfWater;
  Serial.print(waterused);
  Serial.println(" litres left tank");
  return waterused;
  }





void loop() {
  // put your main code here, to run repeatedly:


  // code for ultrasonic sensor begins from here

    // Connect or reconnect to WiFi
  if(WiFi.status() != WL_CONNECTED){
    Serial.print("Attempting to connect to SSID: ");
    Serial.println(*ssid);
    WiFi.begin(ssid, pass);
    while(WiFi.status() != WL_CONNECTED){
        // Connect to WPA/WPA2 network. Change this line if using open or WEP network
      Serial.print(".");
      delay(5000);     
    } 
    Serial.println("\nConnected.");
  }
  digitalWrite(triggerPin,HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin,LOW);
  double duration = pulseIn(echoPin,HIGH);
  double tankUsage = 0.0;
  if(duration>=init_duration){ //to avoid erroneous readings from UL sensor
    tankUsage = waterUsed(duration);
    init_duration = duration;
  }
  else{
    Serial.print("error in reading time value ");
    tankUsage = waterUsed(init_duration);
  }
double tapUsage = waterFlownTap();
double overflowed = overFlow();
if (client.connect(server, 80)) // "184.106.153.149" or api.thingspeak.com
  {
    String postStr = writeAPIKey;
      postStr += "&field1=";
      postStr += String(double(tankUsage));
      postStr += "&field2=";
      postStr += String(double(tapUsage));
      postStr += "&field3=";
      postStr += String(double(overflowed));
      postStr += "\r\n\r\n";
    
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: " + writeAPIKey + "\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(postStr.length());
    client.print("\n\n");
    client.print(postStr);
   
  }
    client.stop();
 delay(1000);
}
