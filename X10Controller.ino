/*
 * X-10 Controller Interface
 * 
 * Author: Melissa LeBlanc-Williams
 * Website: https://melissagirl.com
 * 
 * Additional Libraries Required to Compile
 * U8g2
 * MQTT by Joel Gaehwiler
 * ESP8266mDNS
 * ESP8266WebServer
 * ESP8266WiFi
 * ESP8266HTTPUpdateServer
 *
 * X10 FireCracker Library created by BroHogan and updated by Melissa LeBlanc-Williams
 * 
 * Note: After flashing the code once you can remotely access your device by going to http://HOSTNAMEOFDEVICE.local/firmware 
 * obviously replace HOSTNAMEOFDEVICE with whatever you defined below. The user name and password are also defined below.
 *
 */

// X10 FireCracker
#include "X10FireCracker.h"

// OLED
#include <Wire.h>
#include <U8g2lib.h>

// WiFi
#include <ESP8266WiFi.h>

// Update server
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266HTTPUpdateServer.h>

// MQTT
#include <MQTTClient.h>

#define BIT_DELAY 1

// Initialize the OLED Display
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);

/* DO NOT CHANGE ANY SETTINGS ABOVE THIS LINE */

/* Serial Pin Settings */
#define RTS_PIN 14  // DB9 Pin 7
#define DTR_PIN 12  // DB9 Pin 4

/* WIFI Settings */
const char* ssid = "WiFi SSID";
const char* password = "WiFi Password";

/* Web Updater Settings */
// Host Name of Device
const char* host = "X10Controller";

// Path to access firmware update page (Not Neccessary to change)
const char* update_path = "/firmware";

// Username to access the web update page
const char* update_username = "admin";

// Password to access the web update page
const char* update_password = "password";

/* MQTT Settings */
const char* mqttServer = "192.168.11.6";
const char* subscribeTopic = "MySmartHome/utilities/X10Controller";
const char* mqttDeviceID = "X10Controller1"; 

/* Time in seconds until screen is cleared. Set to 0 to disable. */
int screenClearTimeout = 5;

/* DO NOT CHANGE ANY SETTINGS BELOW THIS LINE */

unsigned long millisSinceLastEvent = 0;

// Web Server 
ESP8266WebServer httpServer(80);
ESP8266HTTPUpdateServer httpUpdater;

WiFiClient espClient;
MQTTClient client;

void setup() 
{
  X10.init(RTS_PIN, DTR_PIN, BIT_DELAY);
  u8g2.begin();
  connectWifi();
  client.begin(mqttServer, espClient);
  client.onMessage(messageReceived);

  MDNS.begin(host);

  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.begin();

  MDNS.addService("http", "tcp", 80);  
}

void loop()
{
  if (!client.connected()) {
    connectMQTT();
  }
  
  client.loop();
  delay(10);
  
  httpServer.handleClient();

  if (screenClearTimeout > 0 && millis() - millisSinceLastEvent > screenClearTimeout * 1000) {
    // Clear the screen
    startOled();
    outputOled();
  }
}

void connectWifi() {
  delay(10);

  startOled();
  u8g2.setCursor(0, 35);
  u8g2.print("Connecting to ");
  u8g2.print(ssid);
  outputOled();

  // Start WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Animated Connection Message while Connecting
  while (WiFi.status() != WL_CONNECTED) {
    delay(125);
    printMessage("Connecting");
    delay(125);
    printMessage("Connecting.");
    delay(125);
    printMessage("Connecting..");
    delay(125);
    printMessage("Connecting...");
  }

  // WiFi Info
  startOled();
  u8g2.setCursor(0, 20);
  u8g2.print("WiFi connected");
  u8g2.setCursor(0, 35);
  u8g2.print("IP address:");
  u8g2.setCursor(0, 50);
  u8g2.print(WiFi.localIP());
  outputOled();
}

void connectMQTT() {
  startOled();
  u8g2.setCursor(0, 10);
  u8g2.print("Attempting MQTT");
  u8g2.setCursor(0, 25);
  u8g2.print("connection...");
  u8g2.setCursor(0, 40);

  // Loop until we're reconnected
  while (!client.connect(mqttDeviceID)) {
    delay(1000);
  }

  u8g2.print("Connected!");
  u8g2.setCursor(0, 55);
  u8g2.print("Listening...");
  outputOled();
  
  // ... and subscribe
  client.subscribe(subscribeTopic);
  millisSinceLastEvent = millis();
}

void messageReceived(String &topic, String &payload) {
  byte error = 0;
  String tokens[3];
  int token = 0;
  int tokenStart = 0;
  int tokenEnd = 0;
  
  HouseCode houseCode;
  int deviceId = 0;
  CommandCode command;

  // Separate the message into tokens
  for (int i = 0; i < payload.length(); i++) {
    if (payload.charAt(i) == '-') {
      tokens[token] = payload.substring(tokenStart, i);
      tokenStart = i + 1;
      token++;

      if (token > sizeof(tokens)) {
        break;
      }
    }
  }

  // Grab the final token
  tokens[token] = payload.substring(tokenStart, payload.length()); 
  
  if (tokens[0] == "ON") {
    command = cmdOn;
  } else if (tokens[0] == "OFF") {
    command = cmdOff;
  } else if (tokens[0] == "BRIGHT") {
    command = cmdBright;
  } else if (tokens[0] == "DIM") {
    command = cmdDim;
  } else {
    error = 1;
  }

  char houseLetter = tokens[1].c_str()[0];
  if (houseLetter >= 'A' && houseLetter <= 'P') {
    houseCode = (HouseCode)(houseLetter - 'A');
  } else {
    error = 1;
  }
  
  if (tokens[2].toInt() >= 1 && tokens[2].toInt() <= 16) {
    deviceId = tokens[2].toInt();
  } else if (tokens[2] == "ALL" && command == cmdOn) {
    command = cmdAllOn;
  } else if (tokens[2] == "ALL" && command == cmdOff) {
    command = cmdAllOff;
  } else if (tokens[2] == "LAMPS" && command == cmdOn) {
    command = cmdLampsOn;
  } else if (tokens[2] == "LAMPS" && command == cmdOff) {
    command = cmdLampsOff;
  } else {
    error = 1;
  }

  if (error == 0) {
    sendCommand(houseCode, deviceId, command);
  } else {
    printMessage("Error: Bad Format");
  }
  millisSinceLastEvent = millis();
}

void startOled() {
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font  
}

void outputOled()
{
  u8g2.sendBuffer();          // transfer internal memory to the display
}

void printMessage(const char *s)
{
  startOled();
  u8g2.setCursor(0, 35);
  u8g2.print(s);  // write something to the internal memory
  outputOled();
}

// Send a command and Display command Info on OLED
void sendCommand(HouseCode houseCode, int deviceId, CommandCode command) {
  u8g2.clearBuffer();          // clear the internal memory
  u8g2.setFont(u8g2_font_ncenB08_tr); // choose a suitable font
  u8g2.setCursor(30, 10);
  
  u8g2.print("Sending ");
  u8g2.setCursor(30, 25);
  if (command == cmdOn) {
    u8g2.print("ON");
  } else if (command == cmdOff) {
    u8g2.print("OFF");
  } else if (command == cmdBright) {
    u8g2.print("BRIGHT");
  } else if (command == cmdDim) {
    u8g2.print("DIM");
  } else if (command == cmdAllOn) {
    u8g2.print("ALL ON");
  } else if (command == cmdAllOff) {
    u8g2.print("ALL OFF");
  } else if (command == cmdLampsOn) {
    u8g2.print("LAMPS ON");
  } else if (command == cmdLampsOff) {
    u8g2.print("LAMPS OFF");
  } else {
    u8g2.print("UNKNOWN");
  }
  
  u8g2.setCursor(30, 40);
  u8g2.print("House: ");
  char houseLetter = 'A' + houseCode;
  u8g2.print(houseLetter);

  if (deviceId > 0) {
    u8g2.setCursor(30, 55);
    u8g2.print("Device: ");
    u8g2.print(deviceId);
  }

  u8g2.sendBuffer();          // transfer internal memory to the display

  X10.sendCmd(houseCode, deviceId, command);
}
