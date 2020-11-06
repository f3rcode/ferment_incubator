#include <DHT.h>
#include <DHT_U.h>


#define DHTPIN 7 
#define LED 13 //led interno
#define RELAYPIN A0

#define DHTTYPE DHT22   // DHT 22  (AM2302)

//Temperature
#define TEMPEH 31 // 28-32 Celsius degrees (max 33 (or 35?)!! CHECK)
#define NATTO  40 //from about body temperature to 45
#define KOJI   28 // 27–35°C
#define ERROR_TEMP 1 

boolean threshold_flag= false;
boolean decreasing_flag= false;

//Debug
boolean Debugging=false;


float Setpoint=TEMPEH;
float initStop=2; //first of all, it stops warming 3degrees below setpoint (in theory, by inerce it arrives the Setpoint¿)
float maxError=1; //it adjust when it's 1 degree below
float threshold=initStop;
byte relayOutput=0;
//boolean initStopFlag=false;

//Temporal Variables
const long timeInterval=2000;
unsigned long now;
unsigned long lastMeasure=0;//to store last measure time


DHT dht(DHTPIN, DHTTYPE);


void setup() {
  while (!Serial);      // wait for Serial Port to open
  Serial.begin(9600); 

 //Relay setup
  pinMode(RELAYPIN, OUTPUT);
  digitalWrite(RELAYPIN, HIGH);

  
  dht.begin();
  delay(500);
}

void loop() {


      

    //-------Sensor reading------
    float h = dht.readHumidity();
    float t = dht.readTemperature();
  
    if (isnan(t) || isnan(h)) {
      Serial.println("Failed to read from DHT");
    } else { 
     


    
     if (t>(Setpoint-threshold)) {

        if (t>(Setpoint-ERROR_TEMP))
            threshold=ERROR_TEMP;
        
        //Serial.println("stop it!");
        relayOutput=0;
        digitalWrite(RELAYPIN, LOW);
      

                       
       }
     else{
        
          //Serial.println("warm it!");
          relayOutput=1;
          digitalWrite(RELAYPIN, HIGH);       
        }


  
    
      Serial.print(t);
      Serial.print(':');
      Serial.print(relayOutput);
      Serial.println(':');

  //---------Blinking internal led--------
  }
    
  digitalWrite(LED, HIGH); 
  delay(200);              
  digitalWrite(LED, LOW);    
  delay(200);   
  //-------------------------------

}
