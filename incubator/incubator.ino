#include <DHT.h>
#include <DHT_U.h>


#define DHTPIN 7 
#define LED 13 //led interno
#define RELAYPIN A0
#define WARNING_LED 9
#define DHTTYPE DHT22   // DHT 22  (AM2302)


//Temperature
#define TEMPEH 31 // 28-32 Celsius degrees (max 33 (or 35?)!! CHECK)
#define WARNING_TEMPEH 32
#define NATTO  40 //from about body temperature to 45
#define KOJI   28 // 27–35°C
#define ERROR_TEMP 1 
#define ERROR_EXOTHERMIC_TEMP 2

boolean thresholdFlag= false;
boolean decreasingFlag= false;

//Debug
boolean Debugging=false;


float Setpoint=TEMPEH;
float initStop=2; //first of all, it stops warming 2degrees below setpoint (in theory, by inerce it arrives the Setpoint¿)
float maxError=1; //it adjust when it's 1 degree below
float threshold=initStop;
byte relayOutput=0;

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
  
  //Warning led setup
  pinMode(WARNING_LED, OUTPUT);
  digitalWrite(WARNING_LED, LOW);

  
  dht.begin();
 // Serial.println("Begining!:");
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

        //it only executes once: first time temperature gets higher than Setpoint-initStop
        if ((t>(Setpoint-ERROR_TEMP))&&(!thresholdFlag)){
            threshold=ERROR_TEMP;
            thresholdFlag=true; 
        }

        /*thought to be executed when temperature gets higher than warning value
       when exothermic reaction starts to happen.
        */
        if ((t> WARNING_TEMPEH)&&(thresholdFlag)){
          threshold=ERROR_EXOTHERMIC_TEMP;
          digitalWrite(WARNING_LED, HIGH);
          }
        
        Serial.println("stop it!");
        relayOutput=0;
        digitalWrite(RELAYPIN, LOW);
                         
       }
     else{
        
          Serial.println("warm it!");
          relayOutput=1;
          digitalWrite(RELAYPIN, HIGH);       
        }

 
    
      Serial.print(t);
      Serial.print(':');
      Serial.print(relayOutput);
      Serial.println(':');

  //---------Blinking internal led so I can see it's working--------
  }
    
  digitalWrite(LED, HIGH); 
  delay(200);              
  digitalWrite(LED, LOW);    
  delay(200);   
  //-------------------------------

}
