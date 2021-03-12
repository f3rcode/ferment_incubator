#include <AutoPID.h>
#include <RBDdimmer.h>
#include "DHT.h"
#include <SerialMenu.hpp>

#define SERIALMENU_MINIMAL_FOOTPRINT true

//PID settings and gains
#define OUTPUT_MIN 0.0
#define OUTPUT_MAX 100.0
#define KP 45 //lets start lighting at kp% when inside gauging rate 
#define KI 0.0
#define KD 1.5

//Dimmer
//D2 <- zero-cross dimmer pin (by default and not changeable)
#define OUTPUTDIMMERPIN 12

//DHT
#define DHTPIN 7 
#define DHTTYPE DHT22   // DHT 22  (AM2302)

//LEDS
#define LED 13 //led interno
#define WARNING_LED 9

//Temperature
#define TEMPEH 31// 28-32 Celsius degrees (max 33 (or 35?)!! CHECK)
#define WARNING_TEMPEH 32
#define NATTO  42 //from about body temperature to 45
#define WARNING_NATTO 44
#define KOJI   28 // 27–35°C
#define WARNING_KOJI 33
#define ERROR_TEMP 1 
#define ERROR_EXOTHERMIC_TEMP 2

//Time (unsigned long range is from 0 to 4,294,967,295 (2^32 - 1))
#define ONE_DAY 86400000
#define TWO_DAYS 172800000
#define THREE_DAYS 259200000
#define HOURS2MS 3600000

#define BLINKING_TIME 900
#define INIT_STOP 2; //first of all, it stops warming 2degrees below setpoint (in theory, by inerce it arrives the setpoint)
#define PID_TIMESTEP BLINKING_TIME*2


//Debug
boolean debugging=true;

float t=0;
float h=0;
float warningTemperature;

float setpoint=0;
float outputDimmer=0;


//Temporal Variables
unsigned long timeLimit=0;

void fermentLoop();

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

// Define the main menu
const SerialMenuEntry mainMenu[] = {
  {mainMenuTempeh, true, '1', [](){ setpoint=TEMPEH;  
                                    timeLimit=ONE_DAY;
                                    warningTemperature=WARNING_TEMPEH;
                                    Serial.println("Go!");
                                    fermentLoop();
                                  } },
  {mainMenuNatto, true, '2', [](){setpoint=NATTO;  
                                  timeLimit=ONE_DAY;
                                  warningTemperature=WARNING_NATTO;
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
                                    warningTemperature = setpoint + ERROR_TEMP;
                                    menu.show();} },
  {configMenuTime, true, 'H', [](){ timeLimit = HOURS2MS * menu.getNumber<int>("Time (hours): "); 
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

//float and double in Arduino UNO occupy 4 bytes. The double implementation is exactly the same as the float, with no gain in precision.
AutoPID myPID((double* )&t, (double*)&setpoint, (double*)&outputDimmer, OUTPUT_MIN, OUTPUT_MAX, KP, KI, KD);

dimmerLamp dimmer(OUTPUTDIMMERPIN); 

DHT dht(DHTPIN, DHTTYPE);


void setup() {
  while (!Serial);      // wait for Serial Port to open
  Serial.begin(9600); 

  //PID setup
  //if temperature is more than x degrees below or above setpoint, OUTPUT will be set to min or max respectively
  //myPID.setBangBang(threshold);
  //set PID update interval
  myPID.setTimeStep(PID_TIMESTEP);
  
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

  dimmer.begin(NORMAL_MODE, OFF);

}


void warmDimmerAlgorithm(){
   
  //t_pid=t;
  //call every loop, but updates automatically at dimmerTimestep interval
  myPID.run();                                     
  //When casting from a float to an int, the value is truncated not rounded
  
  if (!outputDimmer) //bypassing an error in RBDimmer lib (light flickering when 0power)
    dimmer.setState(OFF); 
  else
    dimmer.setPower((int)outputDimmer); 


  if (t> warningTemperature) digitalWrite(WARNING_LED, HIGH);

  return;

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

boolean bypassingDimmerSensorReading(float* h,float* t, float* out){

  boolean sensorRead;

  dimmer.setState(OFF); //set it off so we avoid a very short flash before getting into the DHT critical zone

  sensorRead=sensorReading(h,t);

  if ((sensorRead) && (*out))//bypassing RBDimmer lib error (light flickering when 0 power)
    dimmer.setState(ON);

  //if sensorReading returns false, function returns with dimmer switched off
  return sensorRead;
}

void printSensorActuatorValues(float* output){

  
  if (debugging){
    Serial.print(t);
    Serial.print(':');
    Serial.print(h);
    Serial.print(':');
    Serial.println(*output);
 }

 return;

}

void  blinkLed(){

  digitalWrite(LED, HIGH); 
  delay(BLINKING_TIME);              
  digitalWrite(LED, LOW);    
  delay(BLINKING_TIME); 

  return;
}
  
void fermentLoop(){ 

    boolean on= true;

    unsigned long startingTime=millis();

    while(on){
      
          if ((millis()-startingTime)>timeLimit){ 
             on=false;
                
          if (bypassingDimmerSensorReading(&h,&t,&outputDimmer)){ 
       
            warmDimmerAlgorithm();
                
            blinkLed();
            
            printSensorActuatorValues(&outputDimmer);

        }

       
       
    }//if fermentation is beyond its limit time
          
    if (dimmer.getState())
      dimmer.setState(OFF); //Swith dimmer off
      
    digitalWrite(LED, LOW);    

    return; 
    
}

  
void loop() {

   menu.run(500);

   if(bypassingDimmerSensorReading(&h,&t,&outputDimmer))
    printSensorActuatorValues(&outputDimmer );

  
   delay(1000);
}
