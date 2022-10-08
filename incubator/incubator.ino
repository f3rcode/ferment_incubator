#include <DHT.h>
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
#define NATTO  42 //from about body temperature to 45
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
//unsigned long timeLimit = 0;
int8_t hoursLimit = 0;
unsigned long startingTime = 0;

void fermentLoop();


dimmerLamp dimmer(OUTPUTDIMMERPIN); 

DHT dht(DHTPIN, DHTTYPE);

LCDMenu& menu = LCDMenu::get();

///////////////////////////////////////////////////////////////////////////////
// Main menu
///////////////////////////////////////////////////////////////////////////////


const char mainMenuTempeh[]  = "1 - Tempeh";
const char mainMenuNatto[]  = "2 - Natto";
const char mainMenuKoji[]  = "3 - Koji";
const char mainMenuConfig[]  = "C - Config";

// Forward declarations for the config-menu referenced before it is defined.
extern const LCDMenuEntry configMenu[];
extern const uint8_t configMenuSize;

// Define the main menu
const LCDMenuEntry mainMenu[] = {
  {mainMenuTempeh, [](){ setpoint=TEMPEH;  
                                    hoursLimit=ONE_DAY;
                                    warningTemperature=WARNING_TEMPEH;
                                    menu.print("Go!");
                                    fermentLoop();
                                  } },
  {mainMenuNatto, [](){setpoint=NATTO;  
                                  hoursLimit=ONE_DAY;
                                  warningTemperature=WARNING_NATTO;
                                  menu.print("Go!");
                                  fermentLoop();
                                  } },
  {mainMenuKoji, [](){ menu.print("Still not available.",(uint8_t) 1500);} },
  {mainMenuConfig, [](){ menu.getNumber("Temp: ", (uint8_t) TEMPEH, [](int v){
                         warningTemperature = v + ERROR_TEMP;
                         menu.getNumber("Time (hours): ", (uint8_t) HOURS2CONFIG, [](int v){
                         hoursLimit = HOURS2MS * v;
                         menu.print("Go!");
                         fermentLoop();});
                        }); }},

};
constexpr uint8_t mainMenuSize = GET_MENU_SIZE(mainMenu);

//float and double in Arduino UNO occupy 4 bytes. The double implementation is exactly the same as the float, with no gain in precision.
AutoPID myPID((double* )&t, (double*)&setpoint, (double*)&outputDimmer, OUTPUT_MIN, OUTPUT_MAX, KP, KI, KD);



void setup() 
{
  Serial.begin(9600);
  while (!Serial){};  
  Serial.println("Begining!:");

  //PID setup
  //if temperature is more than x degrees below or above setpoint, OUTPUT will be set to min or max respectively
  //myPID.setBangBang(threshold);
  //set PID update interval
  myPID.setTimeStep(PID_TIMESTEP);
  
  //Warning led setup
  pinMode(WARNING_LED, OUTPUT);
  digitalWrite(WARNING_LED, LOW);

  menu.lcdBegin();
  // Install mainMenu as the current menu to run
  menu.load(mainMenu, mainMenuSize);
  // Display current menu (mainMenu)
  menu.show();


  dht.begin();
  Serial.println("Begining!:");
  delay(500);

  dimmer.begin(NORMAL_MODE, OFF);

}


void warmDimmerAlgorithm()
{   
  //t_pid=t;
  //call every loop, but updates automatically at dimmerTimestep interval
  myPID.run();                                     
  //When casting from a float to an int, the value is truncated not rounded
  
  //bypassing an error in RBDimmer lib (light flickering when 0power)
  if (!outputDimmer)
  {
    dimmer.setState(OFF); 
  }
  else
  {
    dimmer.setPower((int)outputDimmer); 
  }
  
  if (t> warningTemperature) 
  { 
    digitalWrite(WARNING_LED, HIGH);
  }
  
  return;
}
  
boolean sensorReading(float* h,float* t)
{

  *h = dht.readHumidity();
  *t = dht.readTemperature();
    
  if (isnan(*t) || isnan(*h)) 
  {
         Serial.println("Failed to read from DHT");
         //dhtError=true;
         return false;
  }
  
  return true;
}

boolean bypassingDimmerSensorReading(float* h,float* t, float* out)
{

  boolean sensorRead;
  
  dimmer.setState(OFF); //set it off so we avoid a very short flash before getting into the DHT critical zone
  
  sensorRead=sensorReading(h,t);
  
  //bypassing RBDimmer lib error (light flickering when 0 power)
  if ((sensorRead) && (*out))
  {
    dimmer.setState(ON);
  }
  
  //if sensorReading returns false, function returns with dimmer switched off
  return sensorRead;
}

void reckonCountdownHoursMin(unsigned long cdown, char* conversion)
{
  char hours[5], minutes[5];
  unsigned long cdownMin = cdown / 1000;
  cdownMin = cdownMin / 60;
  
  sprintf(hours, "%d", cdownMin / 60);
  sprintf(minutes, "%d", cdownMin % 60);
  strcpy(conversion,hours); 
  strcat(conversion,"h");
  strcat(conversion,minutes);
  
  return;
}

void printSensorActuatorValues(float* output, boolean dhtSuccess, const char * cdownHoursMin = "\0")
{
  if (debugging)
  {
    Serial.print(t);
    Serial.print(':');
    Serial.print(h);
    Serial.print(':');
    Serial.println(*output);
  }
  
  if (!dhtSuccess)
  {
    menu.print(F("DHT error :("),F("u better shutdwn"));
  }

  if (cdownHoursMin[0] != '\0')
  {        
    menu.print(t,h,*output,cdownHoursMin);
  }
}

void blinkLed()
{
  digitalWrite(LED, HIGH); 
  delay(BLINKING_TIME);              
  digitalWrite(LED, LOW);    
  delay(BLINKING_TIME); 
}

unsigned long countdown()
{
  if (startingTime != 0)
  {
    return (HOURS2MS * hoursLimit) - (millis()-startingTime);
  }
  
  return 0;
}

boolean checkChangeInValues(float * tOld, float * hOld, float * outputDimmmerOld)
{
  float epsilon = 0.01f;
  float epsilon2 = 1;
  if(fabs(*tOld - t) >= epsilon)
      return true;
  if(fabs(*hOld - h) >= epsilon)
      return true;
  if(fabs(*outputDimmmerOld - outputDimmer) > epsilon2)
      return true;
  return false;
}
  
void fermentLoop()
{ 
  boolean on = true;
  boolean changeInValues = true;
  char cdownHoursMin[6], cdownHoursMinOld[6];
  float hOld, tOld, outputDimmerOld;
  
  startingTime = millis();
  
  while(on){
    
      if (countdown() <= 0)
      {
         on = false;
      }
      hOld = h;
      tOld = t;
      outputDimmerOld = outputDimmer;
      strcpy(cdownHoursMinOld,cdownHoursMin);
      
      boolean isSensorReadingSuccess = bypassingDimmerSensorReading(&h,&t,&outputDimmer);
      reckonCountdownHoursMin(countdown(),cdownHoursMin);
  
      if (checkChangeInValues(&tOld,&hOld,&outputDimmerOld) ||
        (strcmp(cdownHoursMinOld,cdownHoursMin) != 0 ))
      {
        changeInValues = false;
      }
      
      if (isSensorReadingSuccess)
      {      
        warmDimmerAlgorithm();
            
        blinkLed();
  
        if (!changeInValues)
        {          
          printSensorActuatorValues(&outputDimmer, isSensorReadingSuccess, cdownHoursMin);
        }
     }
  
     changeInValues = true;
     delay(1000);       
  }//if fermentation is beyond its limit time
  
  startingTime = 0;
  
  if (dimmer.getState())
  {
    dimmer.setState(OFF); //Switch dimmer off
  }
   
  digitalWrite(LED, LOW);    
  
  return;       
}

  
void loop() 
{    
  menu.run(700);
  
  //if (debugging) -> Serial.print DHT values (Serial should be let for only debugging purposes and get rid of 2nd argument)
  //UNCOMMENT printSensorActuatorValues(&outputDimmer, bypassingDimmerSensorReading(&h,&t,&outputDimmer));
}
