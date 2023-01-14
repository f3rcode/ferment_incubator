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

//****lcdPrint overloads

void lcdPrint (const char* text)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(text);
}

void lcdPrint (const char* text1, const char* text2)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(text1);
  lcd.setCursor(0, 1);
  lcd.print(text2);
}


void lcdPrint (float number1,float number2,float number3,const char* text)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(number1);
  lcd.setCursor(11, 0);
  lcd.print(text);
  lcd.setCursor(0, 1);
  lcd.print(number2);
  lcd.setCursor(12, 1);
  lcd.print(number3);
}
//*****end lcdPrint overloads

void warmDimmerAlgorithm()
{   
  //t_pid=t;
  //call every loop, but updates automatically at dimmerTimestep interval
  myPID.run();                                     
  //When casting from a float to an int, the value is truncated not rounded

  //bypassing an error in RBDimmer lib (light flickering when 0power)
  if (outputDimmer == 0)
  {
    dimmer.setState(OFF); 
  }
  else
  {
    dimmer.setPower(static_cast<int>(outputDimmer)); 
  }
  
  if (t> warningTemperature) 
  { 
    digitalWrite(WARNING_LED, HIGH);
  }
  
  return;
}
  
boolean sensorReading()
{

  t = dht.readTemperature(true); //force reading -> (DHT MIN_INTERVAL = 2000)
  h = dht.readHumidityAlreadyRead(); //save a critical zone elapsed time

    
  if (isnan(t) || isnan(h)) 
  {
         //Serial.println("Failed to read from DHT");
         //dhtError=true;
         return false;
  }
  
  return true;
}

boolean bypassingDimmerSensorReading()
{

  boolean sensorRead = true;
  
  dimmer.setState(OFF); //set it off so we avoid a very short flash before getting into the DHT critical zone
  
  //sensorRead=sensorReading();
  sensorReading();
  
  //bypassing RBDimmer lib error (light flickering when 0 power)
  if ((sensorRead) && (outputDimmer))
  {
    dimmer.setState(ON);
  }
  
  //if sensorReading returns false, function returns with dimmer switched off
  return sensorRead;
}

void reckonCountdownHoursMin(unsigned long cdown,char* conversion)
{
  char hours[5], minutes[5];
  unsigned long cdownMin = cdown / 60000;
  
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
    //Serial.print(t);
    //Serial.print(':');
    //Serial.print(h);
    //Serial.print(':');
    //Serial.println(*output);
  }
  
  if (!dhtSuccess)
  {
    lcdPrint("DHT error :(","u better shutdwn");
  }

  if (cdownHoursMin[0] != '\0')
  {        
    lcdPrint(t,h,*output,cdownHoursMin);
  }
}

void blinkLed()
{
  digitalWrite(LED, HIGH); 
  delay(BLINKING_TIME);              
  digitalWrite(LED, LOW);    
  delay(BLINKING_TIME); 
}

unsigned long countdown(unsigned long& startingTime)
{
  unsigned long currentTime = millis();
  
  if ((hoursLimit) - (currentTime-startingTime) > 0)
  {
    return (hoursLimit) - (currentTime-startingTime);
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
  boolean changeInValues = true;
  char cdownHoursMin[6] = "\0", cdownHoursMinOld[6] = "\0";
  float hOld, tOld, outputDimmerOld;
  
  unsigned long startingTime = millis();
  
  warmDimmerAlgorithm();
  myPID.setTimeStep(PID_TIMESTEP);

  while(true){
    
      if (countdown(startingTime) <= 0)
      {
         lcdPrint("Bye!");
         break;
      }
      hOld = h;
      tOld = t;
      outputDimmerOld = outputDimmer;
      //strcpy(cdownHoursMinOld,cdownHoursMin);

      
      boolean isSensorReadingSuccess = bypassingDimmerSensorReading();
      reckonCountdownHoursMin(countdown(startingTime),cdownHoursMin);

      /*if (checkChangeInValues(&tOld,&hOld,&outputDimmerOld) ||
        (strcmp(cdownHoursMinOld,cdownHoursMin) != 0 ))
      {
        changeInValues = false;
      }*/

      if (isSensorReadingSuccess)
      {      
        //Serial.println("reading succcess to warm");
        warmDimmerAlgorithm();
            
        //blinkLed();
  
        if (changeInValues)
        {          
          printSensorActuatorValues(&outputDimmer, isSensorReadingSuccess, cdownHoursMin);
        }
     }
  
     changeInValues = true;
     delay(1000);       
  }//if fermentation is beyond its limit time
  
 
  if (dimmer.getState())
  {
    dimmer.setState(OFF); //Switch dimmer off
  }
   
  digitalWrite(LED, LOW);    
  
  return;       
}


void loop() 
{ 
}
