  
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
