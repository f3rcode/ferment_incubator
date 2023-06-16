
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

unsigned long countdown(unsigned long& startingTime)
{
  unsigned long currentTime = millis();
  
  if ((hoursLimit) - (currentTime-startingTime) > 0)
  {
    return (hoursLimit) - (currentTime-startingTime);
  }
  
  return 0;
}
