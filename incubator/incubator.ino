#include "DHT_MOD.h"
#include <AutoPID.h>
#include <RBDdimmer_MOD.h>
#include <LCDMenu.hpp>

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
#define WARNING_LED 5

//Temperature
#define TEMPEH 31// 28-32 Celsius degrees (max 33 (or 35?)!! CHECK)
#define WARNING_TEMPEH 32
#define NATTO  42 //from above body temperature to 45
#define WARNING_NATTO 44
#define KOJI   28 // 27–35°C
#define WARNING_KOJI 33
#define ERROR_TEMP 1 
#define ERROR_EXOTHERMIC_TEMP 2

//Time (unsigned long range is from 0 to 4,294,967,295 (2^32 - 1))
#define ONE_DAY 24//86400000
#define TWO_DAYS 48//172800000
#define THREE_DAYS 72//259200000
#define HOURS2MS 3600000
#define HOURS2CONFIG 36

#define BLINKING_TIME 900
#define INIT_STOP 2; //first of all, it stops warming 2degrees below setpoint (in theory, by inerce it arrives the setpoint)
#define PID_TIMESTEP BLINKING_TIME*2

//Debug
boolean debugging=true;
//boolean dhtError=false;

float t=0;
float h=0;
float warningTemperature;

float setpoint=0;
float outputDimmer=0;

//Temporal Variables
unsigned long startingTime = 0;
unsigned long hoursLimit = 0; //in milliseconds

void fermentLoop();

dimmerLamp dimmer(OUTPUTDIMMERPIN); 

DHT dht(DHTPIN, DHTTYPE);

//float and double in Arduino UNO occupy 4 bytes. The double implementation is exactly the same as the float, with no gain in precision.
AutoPID myPID((double* )&t, (double*)&setpoint, (double*)&outputDimmer, OUTPUT_MIN, OUTPUT_MAX, KP, KI, KD);

LCD_I2C lcd(0x27);

bool val = false;

void lcdPrint (const char* text);

void setup() 
{
  lcd.begin();
  lcd.backlight();
 
  //Warning led setup
  pinMode(WARNING_LED, OUTPUT);
  digitalWrite(WARNING_LED, LOW);
  {
    LCDMenu& menu = LCDMenu::get(lcd);
  
    // Install mainMenu as the current menu to run
    menu.getNumber("Temp: ", (uint8_t) TEMPEH, [](int v){
                          setpoint = v;
                          warningTemperature = v + ERROR_TEMP;;
                          });    
    menu.getNumber("Time (hours): ", (uint8_t) HOURS2CONFIG, [](int hours){
                          hoursLimit = HOURS2MS * hours;
                          });
  }//~LCDMenu is called when menu is out of scope

  myPID.setTimeStep(0); //so first time we call run(), it processes
  dht.begin(); 
  delay(500);
  dimmer.begin(NORMAL_MODE, ON); //2. dimmer.begin() inits interrupts                           

  lcdPrint("Go!");
  fermentLoop();
}

void loop() 
{ 
}
