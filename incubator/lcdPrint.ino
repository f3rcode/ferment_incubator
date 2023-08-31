// lcdPrint overloads

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


void lcdPrint (float number1,float number2,const char* text2,const char* text1)
{
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(number1);
  lcd.setCursor(11, 0);
  lcd.print(text1);
  lcd.setCursor(0, 1);
  lcd.print(number2);
  lcd.setCursor(9, 1);
  lcd.print(text2);
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

void printSensorValuesText(boolean dhtSuccess, const char * cdownHoursMin = "\0", const char * text = "\0")
{
  if (!dhtSuccess)
  {
    lcdPrint("DHT error :(","u better shutdwn");
  }

  if (cdownHoursMin[0] != '\0')
  {        
    lcdPrint(t,h,text,cdownHoursMin);
  }
}
