#include <Arduino.h>
#include "painlessMesh.h"

#define MESH_PREFIX     "whateverYouLike"
#define MESH_PASSWORD   "somethingSneaky"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

// uncomment only one of the following
#define WHITE
//#define RED
//#define GREEN

#ifdef WHITE
#define ROLE    "white"
#define VERSION "WHITE v1.0.0"
#define MESSAGE "White "
#endif

#ifdef RED
#define ROLE    "red"
#define VERSION "RED - 1.0.0"
#define MESSAGE "Red "
#endif

#ifdef GREEN
#define ROLE    "green"
#define VERSION "GREEN - 1.0.0"
#define MESSAGE "Green "
#endif

bool calc_delay = false;
SimpleList<uint32_t> nodes;
uint32_t nsent=0;
char buff[512];

void sendMessage(String msg);

void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Rx from %u <- %s\n", from, msg.c_str());

  if (strcmp(msg.c_str(),"GETRT") == 0) {
    mesh.sendBroadcast(mesh.subConnectionJson(true).c_str());
  } else {
    Serial.printf("Rx:%s\n", msg.c_str());
  }
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("--> Start: New Connection, nodeId = %u\n", nodeId);
  Serial.printf("--> Start: New Connection, %s\n", mesh.subConnectionJson(true).c_str());
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");

  nodes = mesh.getNodeList();
  Serial.printf("Num nodes: %d\n", nodes.size());
  Serial.printf("Connection list:");
  SimpleList<uint32_t>::iterator node = nodes.begin();
  while (node != nodes.end()) {
    Serial.printf(" %u", *node);
    node++;
  }
  Serial.println();
  calc_delay = true;

  Serial.printf("Nodes:%d\n", nodes.size());
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u Offset = %d\n", mesh.getNodeTime(), offset);
}

void onNodeDelayReceived(uint32_t nodeId, int32_t delay) {
  Serial.printf("Delay from node:%u delay = %d\n", nodeId, delay);
}

void sendMessage(String msg) {
  if (strcmp(msg.c_str(), "") == 0) {
    nsent++;
    msg = MESSAGE;
    msg += nsent;
  }
  mesh.sendBroadcast(msg);
  Serial.printf("Tx-> %s\n", msg.c_str());

  if (calc_delay) {
    SimpleList<uint32_t>::iterator node = nodes.begin();
    while (node != nodes.end()) {
      mesh.startDelayMeas(*node);
      node++;
    }
    calc_delay = false;
  }
}

void simulateButtonPress(int buttonPin) {
  sendMessage("GETRT");  // Simulate button press by sending a message
}

void setup() {
  Serial.begin(115200);

  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  mesh.initOTAReceive(ROLE);

  pinMode(2, INPUT_PULLUP); // Use GPIO 2 as the first button (connect to ground to simulate button press)
  pinMode(4, INPUT_PULLUP); // Use GPIO 4 as the second button (connect to ground to simulate button press)

  attachInterrupt(digitalPinToInterrupt(2), []() { simulateButtonPress(2); }, FALLING);
  attachInterrupt(digitalPinToInterrupt(4), []() { simulateButtonPress(4); }, FALLING);
}

void loop() {
  mesh.update();
}
