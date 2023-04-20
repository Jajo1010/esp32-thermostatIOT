#line 1 "C:\\Users\\Jajo\\Documents\\Arduino\\KOP_Termostat\\APConfig.h"
#ifndef __APConfig__H
#define __APConfig__H

#include <Preferences.h>
#include <DNSServer.h>  // https://github.com/me-no-dev/ESPAsyncWebServer
#include <AsyncTCP.h>   // https://github.com/me-no-dev/ESPAsyncTCP
#include <ESPAsyncWebServer.h>
#include "SPIFFS.h"
#include <Arduino.h>


class APConfig {
public:
  APConfig();
  void writeDefaults();
  void writeFactoryDefaults();
  void setupServer();
  void setAP(String APSSID, String APPassword);
  void processNextRequest();
  bool startWiFi(const char* ssid, const char* password);
  bool isDefaultState();

  String getWifiSsid();
  String getWifiPassword();
  IPAddress getMqttIP();
  String getMqttClient();
  String getMqttPassword();
  String getmacAdress();
  int getMqttPort();
  bool getSetup();


private:

  IPAddress convertString(String mqttIP);

  Preferences preferences;
  DNSServer dnsServer;
  AsyncWebServer* server;
  bool setup;
  String wifiSsid;
  String wifiPassword;
  String mqttIP;
  String mqttPort;
  String mqttClient;
  String mqttPassword;
  String macAddress;
};

#endif