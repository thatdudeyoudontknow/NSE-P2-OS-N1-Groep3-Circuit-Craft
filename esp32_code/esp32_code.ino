/*********
  Complete project details at http://randomnerdtutorials.com  
*********/

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
//mash code
#include "painlessMesh.h"
#include <Arduino_JSON.h>
#include <HTTPClient.h>
#include "time.h"
#include "esp_sntp.h"
#include <WiFi.h>

#define MESH_PREFIX "Circuit-Craft-Mash"
#define MESH_PASSWORD "Hond1234"
#define MESH_PORT 5555

#define ssid "Werner"
#define password "Hond1234"

#define node_red_server "http://192.168.137.1:5000/test"
#define LED_PIN 4
Scheduler userScheduler; // to control your personal task
painlessMesh mesh;

const char *ntpServer = "nl.pool.ntp.org";
const long gtmOffset_sec = 3600;
const int daylightOffset_sec = 3600;
int rootRSSI = -100; // Initial value set to a low signal strength

const int glob_buf_size = (64 * sizeof(char));
char *glob_time_buf;
long root_time;
int max_root_time = 60000;
long elect_time;
int max_elect_time = 15000;

//sensor code

/*#include <SPI.h>
#define BME_SCK 18
#define BME_MISO 19
#define BME_MOSI 23
#define BME_CS 5*/

#define SEALEVELPRESSURE_HPA (1013.25)

Adafruit_BME280 bme; // I2C
//Adafruit_BME280 bme(BME_CS); // hardware SPI
//Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI

unsigned long delayTime;
bool sntp_connected = false;

int nodeNumber = 0;          // unique identifier for each node
int rootNodeID = nodeNumber; // start with the assumption that this node is the root
bool is_root = true;

String readings;
String receivedTime;

void checkRootMessage(String msg, int rssi)
{
  int receivedID = msg.toInt();
  if (receivedID != nodeNumber && rssi > rootRSSI)
  {
    rootRSSI = rssi;
    rootNodeID = receivedID;
    is_root = false;
  }
}

String getReadings()
{
  JSONVar jsonReadings;
  jsonReadings["node"] = nodeNumber;
  jsonReadings["temp"] = bme.readTemperature() + 273.15;
  jsonReadings["hum"] = bme.readHumidity();
  jsonReadings["pres"] = bme.readPressure() / 100.0F;
  if (is_root)
  {
    getLocalTime(glob_time_buf, glob_buf_size);
    jsonReadings["time"] = glob_time_buf;
  }
  else
  {
    jsonReadings["time"] = receivedTime;
  }
  readings = JSON.stringify(jsonReadings);
  if (is_root)
  {
    sendData(readings, node_red_server);
  }
  return readings;
}

void sendMessage()
{
  String msg = getReadings();
  mesh.sendBroadcast(msg);
}

void printValues()
{
  Serial.print("Temperature = ");
  Serial.print(bme.readTemperature());
  Serial.println(" *C");

  Serial.print("Pressure = ");
  Serial.print(bme.readPressure() / 100.0F);
  Serial.println(" hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(" m");

  Serial.print("Humidity = ");
  Serial.print(bme.readHumidity());
  Serial.println(" %");

  Serial.println();
}

// mash code

Task taskSendMessage(TASK_SECOND *1, TASK_FOREVER, &sendMessage);

void receivedCallbackWrapper(uint32_t from, String &msg)
{
  receivedCallback(from, msg, WiFi.RSSI());
}

void receivedCallback(uint32_t from, String &msg, int32_t rssi)
{
  Serial.printf("startHere: Received from %u msg=%s with RSSI=%d\n", from, msg.c_str(), rssi);
  if (msg.startsWith("Time =") && !is_root)
  {
    // Extract the time information from the message (assuming the format is "Time = <actual_time>")
    receivedTime = msg.substring(7); // Skip "Time = "
  }
  else if (msg.startsWith("nummer: "))
  {
    checkRootMessage(msg.substring(8), rssi);
  }
  else if (is_root && msg.startsWith("{"))
  {
    sendData(msg.c_str(), node_red_server);
  }
}

void newConnectionCallback(uint32_t nodeId)
{
  Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeNumber);
}

void changedConnectionCallback()
{
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset)
{
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(), offset);
}

void WiFi_connect()
{
  int count = 0;
  Serial.println(" CONNECTING TO WIFI\r\n");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED && count != 10)
  {
    delay(500);
    Serial.println(".");
    count++;
  }
  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println("couldn't connect to WiFi");
    WiFi.disconnect(true);
    return;
  }
  Serial.println(" WiFi CONNECTED\r\n");
  return;
}

void SNTP_connect()
{
  const time_t old_past = 1577836880;
  Serial.println("\r\nConnect to SNTP server\n");

  sntp_setoperatingmode(SNTP_OPMODE_POLL);
  sntp_setservername(0, "pool.ntp.org");
  sntp_init();

  setenv("TZ", "CET-1CEST-2, M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
  tzset();

  int retryCount = 0;

  while (not sntp_connected)
  {
    delay(500);
    time_t now;
    if (time(&now) < old_past)
    {
      Serial.println(".");
      retryCount++;
    }
    else
    {
      Serial.println(" SNTP CONNECTED\r\n");
      sntp_connected = true;
      return;
    }
  }

  Serial.println(" SNTP connection failed after 10 retries.\r\n");
  sntp_stop();
  return;
}

void getLocalTime(char *time_buf, int time_buf_size)
{
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  strftime(time_buf, time_buf_size, "%c", &timeinfo);
  return;
}

void sendData(String data, String endpoint)
{
  if (elect_time + max_elect_time > millis())
  {
    Serial.println("Electing so not sending data");
    return;
  }
  return; //skip sending data for. THIS MUST BE REMOVED.
  HTTPClient http;
  http.begin(endpoint);
  http.addHeader("Content-Type", "application/json");

  String jsonPayload = String(data);

  Serial.print("JSON Payload: ");
  Serial.println(jsonPayload);

  int httpResponseCode = http.POST(jsonPayload);

  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
  }
  else
  {
    Serial.print("HTTP POST request failed, error: ");
    Serial.println(httpResponseCode);
  }

  http.end();
  return;
}

void rootElection()
{
  String msg = "nummer: " + String(nodeNumber);
  mesh.sendBroadcast(msg);

  if (root_time + 10000 < millis() && is_root == false)
  {
    Serial.println("timeout");
    is_root = true;
    rootNodeID = nodeNumber;
    elect_time = millis();
  }

  if (elect_time + max_elect_time < millis())
  {
    Serial.println("elect over");
    taskSendMessage.enable();
  }
  else
  {
    Serial.println("root electing");
    taskSendMessage.disable();
  }
}

void setup()
{
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  glob_time_buf = (char *)malloc(glob_buf_size);
  assert(glob_time_buf != NULL);
  WiFi_connect();

  if (WiFi.status() == WL_CONNECTED)
  {
    SNTP_connect();
  }

  Serial.println(F("BME280 test"));

  bool status;

  status = bme.begin(0x76);
  if (!status)
  {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1)
      ;
  }

  Serial.println("-- Default Test --");
  delayTime = 1000;

  Serial.println();

  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);

  mesh.onReceive(&receivedCallbackWrapper);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask(taskSendMessage);
  elect_time = millis();
}

void loop()
{
  mesh.update();
  rootElection();
  if (is_root)
  {
    getLocalTime(glob_time_buf, glob_buf_size);
    String time_msg = "Time = " + String(glob_time_buf);
    mesh.sendBroadcast(time_msg);
    digitalWrite(LED_PIN, HIGH);
    Serial.print("Root RSSI: ");
    Serial.println(rootRSSI);
  }
  else
  {
    digitalWrite(LED_PIN, LOW);
  }

  delay(delayTime);
}
