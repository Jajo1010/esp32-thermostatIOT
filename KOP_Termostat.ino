#include "termostat_icons.h"
#include "Button.h"
#include "APConfig.h"

#include <Wire.h>
#include <U8g2lib.h>

#include <OneWire.h>
#include <DallasTemperature.h>
#include <NonBlockingDallas.h>
#include <CircularBuffer.h>

#include <WiFi.h>
#include <PubSubClient.h>
#include "APConfig.h"

#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncElegantOTA.h>
#include <TelnetStream.h>

#define OLED_SCL 17
#define OLED_SDA 16

#define W 128
#define H 64
#define RELAY_PIN 4
#define TEMPERATURE_SENSOR_PIN 5
#define TIME_INTERVAL 3000

#define SW1_PIN 15
#define SW2_PIN 13
#define SW3_PIN 14

#define MAIN_MENU 1
#define TEMPERATURE_CHANGE 2
#define SETTINGS 3

#define SHOW_IP 1
#define FACTORY_RESET 2
#define BACK 3
#define SHOW_IP_SCREEN 4


AsyncWebServer server(80);

//millis values
unsigned long lastReconnectAttempt = 0;
const unsigned long reconnectInterval = 2000;

//Thermostat variables
float targetTemperature;
float setTemperature = 22.00;
float tempSetTemperature = setTemperature;
float tempTemperatureSum = 0;
float maxTemperature;
float lowTemperature;
float currentTemperature;
bool heatingState;
float heatingThresholdTemperature = 0.0;
String temperatureUnit = "CELSIUS";
String heatingCoolingState;

//Temperature sensor
OneWire oneWire(TEMPERATURE_SENSOR_PIN);
DallasTemperature dallasTemp(&oneWire);
NonBlockingDallas sensorDs18b20(&dallasTemp);
DeviceAddress tempDeviceAddress;  // SOLVE WHEN DEVICE HERE
int numberOfDevices;

APConfig apConfig;  //APConfig - Runs webServer which handles config variables such as Wifi and MQTT

//Filtering
CircularBuffer<float, 60> buffer;
float averageTemperature;

//non blocking reading
unsigned long previousMillis = 0;
const long interval = 1000;

//bool variables
bool isConnected = false;
bool wasHeatingOn = false;

//MQTT PART
const char* mqttServer;
int mqttPort = 1883;
const char* clientId;
const char* clientPass;
WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

//Getter Topics
const char* getTemperatureStatTopic = "stat/thermostat/temperature";
const char* getDisplayUnitsStatTopic = "stat/thermostat/temperatureDisplayUnits";
const char* getHeatingCoolingStateStatTopic = "stat/thermostat/heatingCoolingState";
const char* getHeatingThresholdTemperatureStatTopic = "stat/thermostat/heatingThresholdTemperature";
const char* getTargetTemperatureStatTopic = "stat/thermostat/targetTemperature";

//Setter Topics
const char* targetTemperatureTopic = "cmnd/thermostat/targetTemperature";
const char* setTemperatureDisplayUnits = "cmnd/thermostat/setTemperatureDisplayUnits";
const char* setTargetHeatingCoolingState = "cmnd/thermostat/DisplayUnits/heatingCoolingState";
const char* setHeatingThresholdTemperature = "cmnd/thermostat/DisplayUnits/heatingThresholdTemperature";


//Display
U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE, OLED_SCL, OLED_SDA);

//Display icon positions
const int navBarHeight = 15;  // height of the nav bar
const int wifiIconX = 0;
const int wifiIconY = 0;
const int radiatorIconX = W - 16 - 0;  // x-coordinate of the WiFi icon
const int radiatorIconY = 0;           // y-coordinate of the WiFi icon

//Display text positions
const int setTemperatureX = W / 2 - 20;
const int setTemperatureY = 12;
const int currentTemperatureX = W / 2 - 65;
const int currentTemperatureY = H / 2 + 20;

//Display temp variables
float displayCurrentTemperature;
float displaySetTemperature;
int displayWiFiState;
int displayMenu = -1;
bool displayHeatingState;
int currentState = MAIN_MENU;

//Display menu variables
int menuScreen = MAIN_MENU;
int menuPos = SHOW_IP;
int previousMenuPos = 1;
bool confirm = false;

//Buttons
Button leftButton(SW1_PIN);
Button middleButton(SW2_PIN);
Button rightButton(SW3_PIN);

//APConfig - Runs webServer which handles config variables such as Wifi and MQTT
//APConfig apConfig;

bool setupScreen = true;

void fillBufferInitial() {
  using index_t = decltype(buffer)::index_t;
  for (index_t i = 0; i < buffer.size(); i++) {
    buffer.push(255);
  }
}

int wifiState() {
  int value = WiFi.RSSI();
  if (WiFi.status() != WL_CONNECTED) {
    return -1;
  }
  if (value > -30) {
    return 1;
  }
  if (value > -70) {
    return 2;
  }
  return 3;
  // return (value > -30) ? 1 : ((value > -70) ? 2 : 3);
}

void drawWifiIcon(int x, int y, int wifiStrenght) {
  switch (wifiStrenght) {
    case 1:  // Strongest WiFi signal
      u8g2.drawXBMP(x, y, 16, 16, wifi_high);
      break;
    case 2:  // Okay WiFi signal
      u8g2.drawXBMP(x, y, 16, 16, wifi_mid);
      break;
    case 3:  // Low WiFi Signal
      u8g2.drawXBMP(x, y, 16, 16, wifi_low);
      break;
    case -1:  // No WiFi Signal
      u8g2.drawXBMP(x, y, 16, 16, no_wifi);
      break;
  }
}

void drawRadiatorIcon(int x, int y, bool heating) {
  if (heating) {
    u8g2.drawXBMP(x, y, 16, 16, radiator_on);
  } else {
    u8g2.drawXBMP(x, y, 16, 16, radiator_off);
  }
}

void drawtargetTemperature(int x, int y, float value) {
  String temperatureString = formatTemperature(value);
  u8g2.setFont(u8g2_font_t0_11_mf);
  u8g2.drawStr(x, y, temperatureString.c_str());
  u8g2.sendBuffer();
}

void drawtargetTemperatureChange(float value) {
  String temperatureString = formatTemperature(value) + "C";
  u8g2.setFont(u8g2_font_inb21_mf);
  u8g2.drawStr(W / 2 - 50, H / 2 + 10, formatTemperature(value).c_str());
  u8g2.sendBuffer();
}

void drawTemperature(int x, int y, float value) {
  String temperatureString = formatTemperature(value);
  TelnetStream.println(temperatureString);
  u8g2.setFont(u8g2_font_inb21_mf);
  u8g2.drawStr(x, y, temperatureString.c_str());
  u8g2.sendBuffer();
}

void drawNavBar(int textX, int textY, int wifiIconX, int wifiIconY, int radiatorX, int radiatorY, float displayValue) {

  //horizontal line divider
  u8g2.drawLine(0, navBarHeight + 1, W, navBarHeight + 1);
  u8g2.drawLine(0, navBarHeight + 2, W, navBarHeight + 2);
  //draw icons
  drawWifiIcon(wifiIconX, wifiIconY, wifiState());
  drawRadiatorIcon(radiatorX, radiatorY, heatingState);
  drawtargetTemperature(textX, textY, displayValue);
}

void drawMainMenu() {
  u8g2.clearDisplay();
  u8g2.setDrawColor(1);
  drawNavBar(setTemperatureX, setTemperatureY, wifiIconX, wifiIconY, radiatorIconX, radiatorIconY, setTemperature);
  drawTemperature(currentTemperatureX, currentTemperatureY, currentTemperature);
  u8g2.sendBuffer();
}

void drawChangeTemperature() {
  // Clear the display
  u8g2.clearDisplay();

  u8g2.setDrawColor(1);

  // Set the cursor to the middle of the screen

  // Draw the temperature set value inside a small rectangle
  u8g2.drawBox(0, 12, W, 36);
  u8g2.setDrawColor(0);
  drawtargetTemperatureChange(setTemperature);
  // Update the display
  u8g2.sendBuffer();
}

void drawSettings(int selectedMenu) {
  // Clear the display
  u8g2.clearDisplay();

  u8g2.setFont(u8g2_font_7x14_tf);

  // Draw the first menu item
  u8g2.setDrawColor(1);
  if (selectedMenu == SHOW_IP) {
    u8g2.drawBox(0, 0, W, 18);
    u8g2.setDrawColor(0);
    u8g2.drawTriangle(W - 8, 8, W - 4, 4, W - 4, 12);
  }
  u8g2.setCursor(4, 13);
  u8g2.print("Show info");

  // Draw the second menu item
  u8g2.setDrawColor(1);
  if (selectedMenu == FACTORY_RESET) {
    u8g2.drawBox(0, 20, W, 18);
    u8g2.setDrawColor(0);
    u8g2.drawTriangle(W - 8, 28, W - 4, 32, W - 4, 24);
  }
  u8g2.setCursor(4, 33);
  u8g2.print("Factory reset");

  // Draw the third menu item
  u8g2.setDrawColor(1);
  if (selectedMenu == 3) {
    u8g2.drawBox(0, 40, W, 18);
    u8g2.setDrawColor(0);
    u8g2.drawTriangle(W - 8, 48, W - 4, 52, W - 4, 44);
  }
  u8g2.setCursor(4, 53);
  u8g2.print("> Back");

  // Update the display
  u8g2.sendBuffer();
}

void drawInfo() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawStr(0, 20, "ESP32 IP Address:");
  u8g2.drawStr(0, 40, WiFi.localIP().toString().c_str());
  u8g2.drawStr(50, 60, "> Back");
  u8g2.sendBuffer();
}


String formatTemperature(float temperature) {
  if (isnan(temperature)) {
    return " ";  // return empty string if temperature is NaN
  }
  String temperatureString = String(temperature, 1);  // 1 decimal place and °C symbol
  while (temperatureString.length() < 5) {
    temperatureString = " " + temperatureString + "C";  // add leading spaces
  }
  return temperatureString;
}

void handleIntervalElapsed(float temperature, bool valid, int deviceIndex) {
  if (valid == true && deviceIndex == 0) {
    currentTemperature = filterTemperature(temperature);
  }
  publishTemperature();
}

float filterTemperature(float temperature) {
  int validReadings = 0;
  if (temperature != abs(127)) {
    buffer.push(temperature);
  }
  for (byte i = 0; i < buffer.size() - 1; i++) {
    if (buffer[i] != 255) {
      validReadings++;
      tempTemperatureSum += buffer[i];
    }
  }

  averageTemperature = tempTemperatureSum / validReadings;
  tempTemperatureSum = 0;

  return round(averageTemperature * 10) / 10.0;
}

void updateMainScreen(int wifiState, float temperature, float targetTemperature, bool heatingState) {
  if (wifiState != displayWiFiState) {
    drawWifiIcon(wifiIconX, wifiIconY, wifiState);
    displayWiFiState = wifiState;
  }

  if (temperature != displayCurrentTemperature) {
    drawTemperature(currentTemperatureX, currentTemperatureY, temperature);
    displayCurrentTemperature = temperature;
  }

  if (targetTemperature != displaySetTemperature) {
    drawtargetTemperature(setTemperatureX, setTemperatureY, setTemperature);
    displaySetTemperature = targetTemperature;
  }

  if (heatingState != displayHeatingState) {
    drawRadiatorIcon(radiatorIconX, radiatorIconY, heatingState);
    displayHeatingState = heatingState;
  }

  if (wifiState != displayWiFiState || temperature != displayCurrentTemperature || targetTemperature != displaySetTemperature || heatingState != displayHeatingState) {
    u8g2.sendBuffer();
  }
}

void updateSetTemperatureScreen(float targetTemperature) {
  if (targetTemperature != displaySetTemperature) {
    u8g2.clearBuffer();
    drawtargetTemperature(setTemperatureX, setTemperatureY, setTemperature);
    displaySetTemperature = targetTemperature;
    u8g2.sendBuffer();
  }
}

void updateDrawSettings(int oldMenuPos, int newMenuPos) {
  if (oldMenuPos != newMenuPos) {
    TelnetStream.print("Old menu ");
    TelnetStream.println(oldMenuPos);
    drawSettings(newMenuPos);
    previousMenuPos = newMenuPos;
    TelnetStream.print("updatedMenu ");
    TelnetStream.println(newMenuPos);
  }
}

void changeMenu(int mode) {

  switch (mode) {
    case MAIN_MENU:
      drawMainMenu();
      break;
    case TEMPERATURE_CHANGE:
      drawChangeTemperature();
      break;
    case SETTINGS:
      drawSettings(menuPos);
      break;
  }
}

void updateMenuValues(int mode) {
  switch (mode) {
    case MAIN_MENU:
      updateMainScreen(wifiState(), currentTemperature, setTemperature, heatingState);
      break;
    case TEMPERATURE_CHANGE:
      drawtargetTemperatureChange(tempSetTemperature);
      break;
    case SETTINGS:
      updateDrawSettings(previousMenuPos, menuPos);
      break;
  }
}

void doActionForSetting(int settingOption) {
  switch (settingOption) {
    case SHOW_IP:
      menuPos = SHOW_IP_SCREEN;
      drawInfo();
      break;
    case FACTORY_RESET:
      factoryReset();
      break;
    case SHOW_IP_SCREEN:
      returnToMainMenu();
      break;
    case BACK:
      returnToMainMenu();
      break;
  }
}

void heatOn() {
  digitalWrite(RELAY_PIN, HIGH);
  heatingState = true;
}

void heatOff() {
  digitalWrite(RELAY_PIN, LOW);
  heatingState = false;
}

void regulateHeater() {
  if (currentTemperature < setTemperature) {
    if (!wasHeatingOn) {
      heatOn();
      wasHeatingOn = true;
      TelnetStream.println("Heating");
    }
  } else if (currentTemperature > setTemperature + heatingThresholdTemperature) {
    if (wasHeatingOn) {
      heatOff();
      wasHeatingOn = false;
      TelnetStream.println("Heating off");
    }
  }
}




void handleButtons(int mode, int settingOption) {

  if (leftButton.wasPressed()) {
    TelnetStream.println("Left was pressed");
    switch (mode) {
      case MAIN_MENU:
        menuScreen = TEMPERATURE_CHANGE;
        break;
      case TEMPERATURE_CHANGE:
        decreaseTemperature();
        break;
      case SETTINGS:
        moveUp();
        break;
    }
  }

  if (middleButton.wasPressed()) {
    switch (mode) {
      case MAIN_MENU:
        menuScreen = SETTINGS;
        break;
      case TEMPERATURE_CHANGE:
        setTemperature = tempSetTemperature;
        publishTemperatureSet();
        menuScreen = MAIN_MENU;
        break;
      case SETTINGS:
        doActionForSetting(settingOption);
    }
  }

  if (rightButton.wasPressed()) {
    TelnetStream.println("Right was pressed");
    switch (mode) {
      case MAIN_MENU:
        menuScreen = TEMPERATURE_CHANGE;
        break;
      case TEMPERATURE_CHANGE:
        increaseTemperature();
        break;
      case SETTINGS:
        moveDown();
        break;
    }
  }
}


void increaseTemperature() {
  tempSetTemperature += 0.5;
}

void decreaseTemperature() {
  tempSetTemperature -= 0.5;
}

void moveUp() {
  if (menuPos > SHOW_IP) {
    menuPos--;
  }
}

void moveDown() {
  if (menuPos < BACK) {
    menuPos++;
  }
}

void factoryReset() {
  apConfig.writeFactoryDefaults();
  ESP.restart();
}

void returnToMainMenu() {
  menuScreen = MAIN_MENU;
}



void nonBlockingReconnect() {
  if (millis() - lastReconnectAttempt > reconnectInterval) {
    lastReconnectAttempt = millis();
    if (client.connect("ESP32Thermostat", apConfig.getMqttClient().c_str(), apConfig.getMqttPassword().c_str())) {
      TelnetStream.println("Connected to MQTT server");
      //Publish Topics
      // const char* getTemperatureStatTopic = "stat/thermostat/temperature";
      // const char* getDisplayUnitsStatTopic = "stat/thermostat/temperatureDisplayUnits";
      // const char* getHeatingCoolingStateStatTopic = "stat/thermostat/heatingCoolingState";
      // const char* getHeatingThresholdTemperatureStatTopic = "stat/thermostat/heatingThresholdTemperature";
      // const char* getTargetTemperatureStatTopic = "stat/thermostat/targetTemperature";

      client.subscribe(targetTemperatureTopic);
      client.subscribe(setTemperatureDisplayUnits);
      client.subscribe(setTargetHeatingCoolingState);
      client.subscribe(setHeatingThresholdTemperature);

    } else {
      /*
      mqttServer = apConfig.getMqttIP().c_str();
      mqttPort = apConfig.getMqttPort();
      clientId = apConfig.getMqttClient().c_str();      
      clientPass = apConfig.getMqttPassword().c_str();
      */
      TelnetStream.println("failed, rc=");
      TelnetStream.println(apConfig.getMqttClient().c_str());
      TelnetStream.println(apConfig.getMqttPassword().c_str());
      //TelnetStream.println(apConfig.getMqttIP().c_str());
      TelnetStream.println(apConfig.getMqttIP());
      TelnetStream.println("192.168.0.19");
    }
  }
}

void mqttCallback(char* topic, byte* message, unsigned int length) {
  TelnetStream.print("Message arrived on topic: ");
  TelnetStream.print(topic);
  TelnetStream.print(". Message: ");
  String incomingMessage;

  for (int i = 0; i < length; i++) {
    TelnetStream.print((char)message[i]);
    incomingMessage += (char)message[i];
  }

  TelnetStream.println();
  String _topic = String(topic);
  if (_topic == targetTemperatureTopic) {
    setTemperature = incomingMessage.toFloat();
  } else if (_topic == setTemperatureDisplayUnits) {
    temperatureUnit = incomingMessage;
  } else if (_topic == setTargetHeatingCoolingState) {
    heatingCoolingState = incomingMessage;
  } else if (_topic == setHeatingThresholdTemperature) {
    heatingThresholdTemperature = incomingMessage.toFloat();
  }
}

void publishTemperatureSet() {
  String GetTargetTemperaturePayload = String(setTemperature);
  client.publish(getTargetTemperatureStatTopic, (const uint8_t*)GetTargetTemperaturePayload.c_str(), GetTargetTemperaturePayload.length(), 1);
  TelnetStream.println("Uploaded messages");
}

void publishTemperature() {
  String CurrentTemperaturePayload = String(currentTemperature);
  client.publish(getTemperatureStatTopic, (const uint8_t*)CurrentTemperaturePayload.c_str(), CurrentTemperaturePayload.length(), 1);
}

void publishHeatingState() {
}

void publishMqttMessages() {
  long now = millis();
  if (now - lastMsg > 1000 * 60) {
    lastMsg = now;
    String HeatingStatePayload = String("HEAT");
    String CurrentTemperaturePayload = String(currentTemperature);
    String GetTargetTemperaturePayload = String(setTemperature);
    String TemperatureDisplayUnitsPayload = temperatureUnit;
    String HeatingTreshold = String(heatingThresholdTemperature);

    //Publish Topics
    // const char* getTemperatureStatTopic = "stat/thermostat/temperature";
    // const char* getDisplayUnitsStatTopic = "stat/thermostat/temperatureDisplayUnits";
    // const char* getHeatingCoolingStateStatTopic = "stat/thermostat/heatingCoolingState";
    // const char* getHeatingThresholdTemperatureStatTopic = "stat/thermostat/heatingThresholdTemperature";
    // const char* getTargetTemperatureStatTopic = "stat/thermostat/targetTemperature";

    client.publish(getHeatingCoolingStateStatTopic, (const uint8_t*)HeatingStatePayload.c_str(), HeatingStatePayload.length(), 1);
    // Publishing the current temperature to topic "current_temperature"
    client.publish(getTemperatureStatTopic, (const uint8_t*)CurrentTemperaturePayload.c_str(), CurrentTemperaturePayload.length(), 1);
    // Publishing the target temperature to topic "target_temperature"
    client.publish(getTargetTemperatureStatTopic, (const uint8_t*)GetTargetTemperaturePayload.c_str(), GetTargetTemperaturePayload.length(), 1);
    // Publishing the temperature display units to topic "temperature_display_units"
    client.publish(getDisplayUnitsStatTopic, (const uint8_t*)TemperatureDisplayUnitsPayload.c_str(), TemperatureDisplayUnitsPayload.length(), 1);
    client.publish(getHeatingThresholdTemperatureStatTopic, (const uint8_t*)HeatingTreshold.c_str(), TemperatureDisplayUnitsPayload.length(), 1);
    TelnetStream.println("Uploaded messages");
  }
}


void setup() {

  fillBufferInitial();

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  menuScreen = 1;
  isConnected = false;

  sensorDs18b20.begin(NonBlockingDallas::resolution_11, NonBlockingDallas::unit_C, TIME_INTERVAL);
  sensorDs18b20.onIntervalElapsed(handleIntervalElapsed);
  sensorDs18b20.requestTemperature();
  sensorDs18b20.update();

  u8g2.begin();
  drawMainMenu();


  setupScreen = apConfig.isDefaultState();

  if (setupScreen) {
    apConfig.setAP("TermostatAP", "Temp12345");
  }

  else {

    if (apConfig.startWiFi(apConfig.getWifiSsid().c_str(), apConfig.getWifiPassword().c_str())) {
      isConnected = true;
      //Code upload


      AsyncElegantOTA.begin(&server);  // Start ElegantOTA
      server.begin();

      

      //Debugging
      TelnetStream.begin();
      TelnetStream.println("");
      TelnetStream.println(apConfig.getMqttClient().c_str());
      TelnetStream.println(apConfig.getMqttPassword().c_str());
      //TelnetStream.println(apConfig.getMqttIP().c_str());

      client.setServer(apConfig.getMqttIP(), apConfig.getMqttPort);
      client.setCallback(mqttCallback);
    } else {
      apConfig.writeFactoryDefaults();
      isConnected = false;
    }
  }
}

void loop() {
  if (setupScreen) {
    apConfig.processNextRequest();
  }

  if (!client.connected() && isConnected == true) nonBlockingReconnect();
  {
    client.loop();
  }

  regulateHeater();
  sensorDs18b20.update();
  handleButtons(menuScreen, menuPos);
  updateMenuValues(menuScreen);
  sensorDs18b20.update();


  if (currentState != menuScreen) {
    currentState = menuScreen;
    changeMenu(currentState);
  }
}