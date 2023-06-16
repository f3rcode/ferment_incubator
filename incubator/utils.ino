
void blinkLed()
{
  digitalWrite(LED, HIGH); 
  delay(BLINKING_TIME);              
  digitalWrite(LED, LOW);    
  delay(BLINKING_TIME); 
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
