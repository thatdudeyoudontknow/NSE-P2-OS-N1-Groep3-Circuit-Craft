/*This code is used the measure the temperature, humidity and air pressure and broadcast it across the mesh so it can be received by a Raspberry pi or other devices on the mesh.*/
#include "painlessMesh.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Arduino_JSON.h>

//the ssid, password and port used by the mesh
#define MESH_PREFIX     "CircuitCraft-Wifi"
#define MESH_PASSWORD   ""
#define MESH_PORT       5555

//the nodeNumber is different from the nodeId generated by painlessmesh. The node number is used the identify the node more easily
int nodeNumber = 3;
String readings;

Scheduler userScheduler;
painlessMesh mesh;
Adafruit_BME280 bme;

#include "show_number.h"
#include "BME280_code.h"

void sendMessage() {
  /*This function broadcast the sensor readings accross the mesh.*/
  String msg = getReadings();
  mesh.sendBroadcast(msg);
}


//A task that run the sendMessage function every 5 seconds.
Task taskSendMessage(TASK_SECOND * 5, TASK_FOREVER, &sendMessage);

void receivedCallback(uint32_t from, String &msg) {
  /*This function is called when the node recives a message from another node.
    It will print the message on the serial output with the nodeId from the node it send.*/
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
  /*This function is called when a new node is connected to the mesh.
    It print the nodeId of the new node on the serial output.*/
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  /*This function is called when a connection in the mesh change.*/
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  /*This function is called when the time of the node is adjusted.
    It print the adjusted time with the offset*/
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void setup() {
  Serial.begin(115200);

  //Initializing the BME280 sensor.
  initBME();

 //Start and configure the mesh
  mesh.setDebugMsgTypes(ERROR | STARTUP);

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT, WIFI_AP_STA,0);
  mesh.initOTAReceive("bridge");
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  
  //Create and enable the task to send messages with the sensor data.
  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();

  //Turn on the LEDs to show its node number in binary.
  show_number(nodeNumber);
}

void loop() {
  //Keep updating the mesh.
  mesh.update();
}