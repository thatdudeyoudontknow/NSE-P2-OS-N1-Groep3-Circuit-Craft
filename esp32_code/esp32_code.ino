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

#define   MESH_PREFIX     "Circuit-Craft-Mash"
#define   MESH_PASSWORD   "Hond1234"
#define   MESH_PORT       5555

#define ssid "Werner"
#define password "Hond1234"

#define node_red_server "http://192.168.137.1:5000/test"
Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

const char* ntpServer = "nl.pool.ntp.org";
const long gtmOffset_sec = 3600;
const int daylightOffset_sec = 3600;

const int glob_buf_size = (64* sizeof(char));
char *glob_time_buf;

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
bool is_root = false;

int nodeNumber = 1;
String readings;
String receivedTime;

String getReadings () {
  JSONVar jsonReadings;
  jsonReadings["node"] = nodeNumber;
  jsonReadings["temp"] = bme.readTemperature()+273.15;
  jsonReadings["hum"] = bme.readHumidity();
  jsonReadings["pres"] = bme.readPressure()/100.0F;
  if(is_root){
    getLocalTime(glob_time_buf, glob_buf_size);
    jsonReadings["time"] = glob_time_buf;
  }
  else{
    jsonReadings["time"] = receivedTime;
  }
  readings = JSON.stringify(jsonReadings);
  if(is_root){
    sendData(readings, node_red_server );
  }
  return readings;
}

void sendMessage () {
  String msg = getReadings();
  mesh.sendBroadcast(msg);
}


void printValues() {
  Serial.print("Temperature = ");
  Serial.print(bme.readTemperature());
  Serial.println(" *C");
  
  // Convert temperature to Fahrenheit
  /*Serial.print("Temperature = ");
  Serial.print(1.8 * bme.readTemperature() + 32);
  Serial.println(" *F");*/
  
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

Task taskSendMessage( TASK_SECOND * 30 , TASK_FOREVER, &sendMessage );


// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("startHere: Received from %u msg=%s\n", from, msg.c_str());
  if (msg.startsWith("Time =") && !is_root) {
    // Extract the time information from the message (assuming the format is "Time = <actual_time>")
    receivedTime = msg.substring(7); // Skip "Time = "
  }
  else if(is_root){
    sendData(msg.c_str(), node_red_server );
  }
}

void newConnectionCallback(uint32_t nodeId) {
    Serial.printf("--> startHere: New Connection, nodeId = %u\n", nodeNumber);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
    Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}

void WiFi_connect(){
int count = 0;
Serial.println(" CONNECTING TO WIFI\r\n");
/*verbind met de wifi volgens de ssid en password in in week1-3.h staan*/
WiFi.begin(ssid, password);
while (WiFi.status() != WL_CONNECTED && count != 10) {
/*print een punt af en toe als er geen verbinding is.*/
delay(500);
Serial.println(".");
count ++;
}
if(WiFi.status() != WL_CONNECTED){
  Serial.println("couldn't connect to WiFi");
  WiFi.disconnect(true);
  return;
}
/* print CONNECTED als er verbinding is */ 
Serial.println(" WiFi CONNECTED\r\n");
return;
}

void SNTP_connect() {
  const time_t old_past = 1577836880;
  Serial.println("\r\nConnect to SNTP server\n");

  // Set SNTP operating mode before initializing
  sntp_setoperatingmode(SNTP_OPMODE_POLL);

  // Set SNTP server name
  sntp_setservername(0, "pool.ntp.org");

  // Initialize SNTP client
  sntp_init();

  // Set time zone
  setenv("TZ", "CET-1CEST-2, M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
  tzset();

  int retryCount = 0;

  while (not sntp_connected) {
    delay(500);
    time_t now;
    if (time(&now) < old_past) {
      Serial.println(".");
      retryCount++;
    } else {
      Serial.println(" SNTP CONNECTED\r\n");
      sntp_connected = true;
      return;  // Successful connection, exit the function
    }
  }

  // If here, the function failed after 10 retries
  Serial.println(" SNTP connection failed after 10 retries.\r\n");
  sntp_stop();

  // Optionally, you can perform additional actions or reset your device here
  // For now, let's just return from the function
  return;
}


/*function to get the current time*/
void getLocalTime(char * time_buf, int time_buf_size) {
  time_t now;
  struct tm timeinfo;
  time(&now);
  localtime_r(&now, &timeinfo);
  strftime(time_buf, time_buf_size, "%c", &timeinfo);
  return;
}

void sendData(String data, String endpoint) {
  HTTPClient http;
  http.begin(endpoint); 
  http.addHeader("Content-Type", "application/json");

  // Create a JSON payload
  String jsonPayload = String(data);

  Serial.print("JSON Payload: ");
  Serial.println(jsonPayload);


  // Send the POST request with JSON payload
  int httpResponseCode = http.POST(jsonPayload);

  if (httpResponseCode > 0) {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
  } else {
    Serial.print("HTTP POST request failed, error: ");
    Serial.println(httpResponseCode);
  }

  http.end();
  return;
}


void setup() {
  Serial.begin(115200);
  glob_time_buf = (char*)malloc(glob_buf_size);
  assert( glob_time_buf != NULL);
  WiFi_connect();

  if (WiFi.status() == WL_CONNECTED){
    SNTP_connect();
    is_root = true; //als er wifi is dan gedraagt hij zich als root.
  }
  
  Serial.println(F("BME280 test"));

  bool status;

  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  status = bme.begin(0x76);  
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    while (1);
  }

  Serial.println("-- Default Test --");
  delayTime = 1000;

  Serial.println();
  
  //mash code
  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask( taskSendMessage );
  taskSendMessage.enable();
}


void loop() { 
  //printValues();
  //mash code
  mesh.update();

  if (is_root){
    getLocalTime(glob_time_buf, glob_buf_size);
    String msg = "Time = " + String(glob_time_buf);
    mesh.sendBroadcast(msg);

  }
  delay(delayTime);

}