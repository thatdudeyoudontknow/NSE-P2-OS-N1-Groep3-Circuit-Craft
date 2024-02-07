/*This file contains code for the BME280 sensor*/

void initBME() {
  /*This function checks if the BME280 sensor is connected. This function wil not end if the sensor is not connected.*/
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
}

String getReadings() {
  /*This function creates and returns a string in json format with the node number and the temperature in kelvin, humidity and air pressure that is measured by the BME280 sensor.*/
  JSONVar jsonReadings;
  jsonReadings["node"] = nodeNumber;
  jsonReadings["temp"] = bme.readTemperature() + 273.15 ;
  jsonReadings["hum"] = bme.readHumidity();
  jsonReadings["pres"] = bme.readPressure() / 100.0F;
  readings = JSON.stringify(jsonReadings);
  return readings;
}