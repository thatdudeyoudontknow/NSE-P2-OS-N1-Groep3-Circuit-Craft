#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

#define MESH_PREFIX     "CircuitCraft-Wifi"
#define MESH_PASSWORD   ""
#define MESH_PORT       5555
#define   STATION_SSID     "CircuitCraft-Wifi"
#define   STATION_PASSWORD ""
#define   STATION_PORT     5555
uint8_t   station_ip[4] =  {10, 80, 93, 5};
Adafruit_BME280 bme;
int nodeNumber = 2;
String readings;

Scheduler userScheduler;
painlessMesh mesh;

String getReadings() {
  JSONVar jsonReadings;
  jsonReadings["node"] = nodeNumber;
  jsonReadings["temp"] = bme.readTemperature();
  jsonReadings["hum"] = bme.readHumidity();
  jsonReadings["pres"] = bme.readPressure() / 100.0F;
  readings = JSON.stringify(jsonReadings);
  return readings;
}

void sendMessage() {
  String msg = getReadings();
  
  // Replace the following line with the IP address of your Raspberry Pi
  IPAddress piIPAddress(10, 80, 93, 5);
  
  mesh.sendSingle(piIPAddress, msg);
}



Task taskSendMessage(TASK_SECOND * 1, TASK_FOREVER, &sendMessage);

void initBME() {
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }
}

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  // Handle the received message on the ESP8266 as needed
  Serial.printf("bridge: Received from %u msg=%s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void setup() {
  Serial.begin(115200);

  initBME();
  
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION | REMOTE | SYNC | GENERAL | COMMUNICATION | MSG_TYPES | DEBUG);

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA,0);
  mesh.initOTAReceive("bridge");
  mesh.stationManual(STATION_SSID, STATION_PASSWORD, STATION_PORT, station_ip);
  mesh.setRoot(true);
  mesh.setContainsRoot(true);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();
}

void loop() {
  mesh.update();
}
