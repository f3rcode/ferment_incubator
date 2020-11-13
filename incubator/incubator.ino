#include <DHT.h>
#include <DHT_U.h>
#include <SerialMenu.hpp>

#define SERIALMENU_MINIMAL_FOOTPRINT true


#define DHTPIN 7 
#define LED 13 //led interno
#define RELAYPIN A0
#define WARNING_LED 9
#define DHTTYPE DHT22   // DHT 22  (AM2302)

//Temperature
#define TEMPEH 31// 28-32 Celsius degrees (max 33 (or 35?)!! CHECK)
#define WARNING_TEMPEH 32
#define NATTO  42 //from about body temperature to 45
#define KOJI   28 // 27–35°C
#define ERROR_TEMP 1 
#define ERROR_EXOTHERMIC_TEMP 2

//Time (unsigned long range is from 0 to 4,294,967,295 (2^32 - 1))
#define ONE_DAY 86400000
#define TWO_DAYS 172800000
#define THREE_DAYS 259200000


boolean thresholdFlag= false;
boolean decreasingFlag= false;
boolean on= false;

//Debug
boolean debugging=false;

float t=0;
float h=0;

float setpoint=0;
float initStop=2; //first of all, it stops warming 2degrees below setpoint (in theory, by inerce it arrives the setpoint¿)
float maxError=1; //it adjust when it's 1 degree below
float threshold=initStop;
byte relayOutput=0;

//Temporal Variables
unsigned int timeLimit=0;
unsigned int startingTime=0;
int blinkingTime=200;


const SerialMenu& menu = SerialMenu::get();

///////////////////////////////////////////////////////////////////////////////
// Main menu
///////////////////////////////////////////////////////////////////////////////


const char mainMenuTempeh[] PROGMEM = "1 - Tempeh";
const char mainMenuNatto[] PROGMEM = "2 - Natto";
const char mainMenuKoji[] PROGMEM = "3 - Koji";
const char mainMenuConfig[] PROGMEM = "C - Config";
const char mainMenuMenu[] PROGMEM = "M - Menu";


// Forward declarations for the config-menu referenced before it is defined.
extern const SerialMenuEntry configMenu[];
extern const uint8_t configMenuSize;
void fermentLoop();

// Define the main menu
const SerialMenuEntry mainMenu[] = {
  {mainMenuTempeh, true, '1', [](){ setpoint=TEMPEH;  
                                    timeLimit=ONE_DAY;
                                    Serial.println("Go!");
                                    fermentLoop();
                                  } },
  {mainMenuNatto, true, '2', [](){setpoint=NATTO;  
                                  timeLimit=ONE_DAY;
                                  Serial.println("Go!");
                                  fermentLoop();
                                  } },
  {mainMenuKoji, true, '3', [](){ Serial.println("Still not available.");
                                  menu.show();} },
  {mainMenuConfig, true, 'C', [](){ menu.load(configMenu, configMenuSize); menu.show();} },
  {mainMenuMenu, true, 'M', [](){ menu.show(); } },

};
constexpr uint8_t mainMenuSize = GET_MENU_SIZE(mainMenu);

///////////////////////////////////////////////////////////////////////////////
// Config-menu
///////////////////////////////////////////////////////////////////////////////

const char configMenuTemp[] PROGMEM = "T - Temperature:";
const char configMenuTime[] PROGMEM = "H - Time:";
const char configMenuStart[] PROGMEM = "! - Go!";
const char configMenuMenu[] PROGMEM = "M - Menu";
const char configMenuBack[] PROGMEM = "< - Main Menu";


// Define the config-menu
const SerialMenuEntry configMenu[] = {
  {configMenuTemp, true, 'T', [](){ setpoint = menu.getNumber<float>("Temp: ");
                                    menu.show();} },
  {configMenuTime, true, 'H', [](){ timeLimit = 3600000* menu.getNumber<int>("Time (hours): "); 
                                    menu.show();} }, //SHOULD LET USE PARTS OF AN HOUR AS A VALUE!
  {configMenuStart, true, '!',[](){ if ((setpoint==0) || (timeLimit==0)){
                                        Serial.println("You need to set temp & time!");
                                        menu.show();
                                      }else{
                                      Serial.print("Temp:"); Serial.println(setpoint);
                                      Serial.print("Time: "); Serial.println(timeLimit);
                                      Serial.println("Go!");
                                      fermentLoop();} 
                                    }},                                    
  {configMenuMenu, true, 'M', [](){ menu.show(); }},
  {configMenuBack, true, '<', [](){ menu.load(mainMenu, mainMenuSize); menu.show(); }},
};
constexpr uint8_t configMenuSize = GET_MENU_SIZE(configMenu);



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

  // Install mainMenu as the current menu to run
  menu.load(mainMenu, mainMenuSize);
  // Display current menu (mainMenu)
  menu.show();


  dht.begin();
 // Serial.println("Begining!:");
  delay(500);
}

boolean sensorReading(float* h,float* t){

  *h = dht.readHumidity();
  *t = dht.readTemperature();
    
  if (isnan(*t) || isnan(*h)) {
         Serial.println("Failed to read from DHT");
         return false;
  }

  return true;
}
  
void fermentLoop(){ 

    startingTime=millis();

    on=true;

    while(on){
      
          if ((millis()-startingTime)>timeLimit) 
             on=false;
      
          if (sensorReading(&h,&t)){ 
       
        
    
             if (t>(setpoint-threshold)) {
        
                //it only executes once: first time temperature gets higher than setpoint-initStop
                if ((t>(setpoint-ERROR_TEMP))&&(!thresholdFlag)){
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
                
                //Serial.println("stop it!");
                relayOutput=0;
                digitalWrite(RELAYPIN, LOW);
                                 
             }else{
                
                  //Serial.println("warm it!");
                  relayOutput=1;
                  digitalWrite(RELAYPIN, HIGH);       
             }
                  
           
            //--------working--------
            digitalWrite(LED, HIGH); 
            delay(blinkingTime);              
            digitalWrite(LED, LOW);    
            delay(blinkingTime); 

            if (debugging){
              Serial.print(t);
              Serial.print(':');
              Serial.print(h);
              Serial.print(':');
              Serial.println(relayOutput);
            }
        }

       
       
    }//if fermentation is beyond its limit time
          
    digitalWrite(RELAYPIN, LOW); 
    digitalWrite(LED, LOW);    
    //delay(blinkingTime);
    return; 
    
  }

  
void loop() {

  

   menu.run(500);

   if(debugging && sensorReading(&h,&t)){
     Serial.print(t);
     Serial.print(':');
     Serial.print(h);
     Serial.print(':');
     Serial.println(relayOutput);
    }
  
   delay(500);
}
