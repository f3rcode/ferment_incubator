// Important:
// Arduino IDE concatenates all .ino files, starting with the .ino file that matches the sketch folder name, 
// followed by the rest of the .ino files in ALPHABETICAL order.
// This file is named as main for convenience.
// https://arduino.stackexchange.com/questions/60656/split-up-arduino-code-into-multiple-files-using-arduino-ide

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
  
void fermentLoop()
{ 
  boolean changeInValues = true;
  char cdownHoursMin[6] = "\0", cdownHoursMinOld[6] = "\0";
  float hOld, tOld, outputDimmerOld;
  
  unsigned long startingTime = millis();
  
  warmDimmerAlgorithm();
  myPID.setTimeStep(PID_TIMESTEP);

  while(true){
    
      if (countdown(startingTime) == 0)
      {
         lcdPrint("Ready!");
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

// preHeat function heats the incubator 
// and prevents the ferments from the first peak of temperature,
// which is the highest.
// Note that time doesn't really starts until preHeat function returns
// and fermentLoop begins.
void preHeat()
{
  char cdownHoursMin[6] = "\0";
  char preheatText[8] = "PREHEAT";
  boolean increasingFlag = false;
  float previousTemp = 0; 
  unsigned long startingTime = millis();
  
  warmDimmerAlgorithm();
  myPID.setTimeStep(PID_TIMESTEP);

  while(true){

      if (!increasingFlag && t > setpoint)
      {
        increasingFlag = true;
      }
      
      // if temperature is decreasing 
      // and it's around a degree above the setpoint
      if (increasingFlag && 
         t < previousTemp && 
         t <= setpoint + 1)
      {
        break;
      }

      previousTemp = t;
      
      boolean isSensorReadingSuccess = bypassingDimmerSensorReading();
      reckonCountdownHoursMin(millis()-startingTime,cdownHoursMin);

      // TODO: check change in values 
      
      if (isSensorReadingSuccess)
      {
        warmDimmerAlgorithm();
                
        printSensorValuesText(isSensorReadingSuccess, cdownHoursMin, preheatText);
     }

     delay(1000);       
  }
}
