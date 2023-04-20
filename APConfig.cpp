#include "APConfig.h"

class CaptiveRequestHandler : public AsyncWebHandler {
public:
  CaptiveRequestHandler() {}
  virtual ~CaptiveRequestHandler() {}

  bool canHandle(AsyncWebServerRequest* request) {
    //request->addInterestingHeader("ANY");
    return true;
  }

  void handleRequest(AsyncWebServerRequest* request) {

    request->send(SPIFFS, "/index.html", "text/html");
    //request->send(200,"text/html","hi");
  }
};


APConfig::APConfig() {
  preferences.begin("config", false);
  setup = preferences.getBool("setup", true);
  wifiSsid = preferences.getString("wifiSsid", "");
  wifiPassword = preferences.getString("wifiPassword", "");

  mqttIP = preferences.getString("mqttIP");
  mqttPort = preferences.getString("mqttPort");

  mqttClient = preferences.getString("mqttClient");
  mqttPassword = preferences.getString("mqttPassword");
  preferences.end();
  macAddress = "nothing";
  if (!SPIFFS.begin()) {
    Serial.println("An error while mounting SPIFFS");
  }
}

String APConfig::getWifiSsid() {
  return wifiSsid;
}

String APConfig::getWifiPassword() {
  return wifiPassword;
}

IPAddress APConfig::getMqttIP() {
  IPAddress ip;
  ip.fromString(mqttIP);
  return ip;
}

int APConfig::getMqttPort() {
  return mqttPort.toInt();
}

String APConfig::getMqttClient() {
  return mqttClient;
}

String APConfig::getMqttPassword() {
  return mqttPassword;
}


bool APConfig::getSetup() {
  return setup;
}

void APConfig::setAP(String APSSID, String APPassword) {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(APSSID.c_str(), APPassword.c_str());
  setupServer();
  Serial.println("Starting DNS Server");
  dnsServer.start(53, "*", WiFi.softAPIP());
  server->addHandler(new CaptiveRequestHandler()).setFilter(ON_AP_FILTER);  //only when requested from AP
  server->begin();
}

void APConfig::processNextRequest() {
  dnsServer.processNextRequest();
}

void APConfig::writeDefaults() {
  preferences.begin("config", false);
  preferences.clear();

  preferences.putBool("setup", false);

  preferences.putString("wifiSsid", wifiSsid);
  preferences.putString("wifiPassword", wifiPassword);

  preferences.putString("mqttIP", mqttIP);
  preferences.putString("mqttClient", mqttClient);
  preferences.putString("mqttPort", mqttPort);
  preferences.putString("mqttPassword", mqttPassword);

  preferences.end();
}

void APConfig::writeFactoryDefaults() {
  preferences.begin("config", false);
  preferences.clear();

  preferences.putBool("setup", true);

  preferences.putString("wifiSsid", "");
  preferences.putString("wifiPassword", "");

  preferences.putString("mqttIP", "");
  preferences.putString("mqttClient", "");
  preferences.putString("mqttPassword","");
  preferences.putString("mqttPort", "");
  preferences.putString("wifiPassword", "");


  preferences.end();
}

/*bool APConfig::startWiFi(const char* ssid, const char* password) {
  static int connAttempts = 0;
  static unsigned long startTime = 0;
  static bool isConnected = false;
  
  if (!isConnected) {
    if (millis() - startTime > 20000) {
      Serial.println("\nFailed to connect to a Wi-Fi network");
      return false;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print(F("WiFi connected at: "));
      Serial.println(WiFi.localIP());
      isConnected = true;
      return true;
    }
    
    if (connAttempts == 0) {
      WiFi.begin(ssid, password);
      startTime = millis();
    }
    
    if (millis() - startTime > 500 && connAttempts <= 20) {
      Serial.print(".");
      connAttempts++;
      startTime = millis();
    }
  }
  
  return isConnected;
}
*/

bool APConfig::startWiFi(const char* ssid, const char* password) {
  int connAttempts = 0;
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    connAttempts++;
    if (connAttempts > 20) {
      Serial.println("\nFailed to connect to a Wi-Fi network");
      return false;
    }
  }
  Serial.print(F("WiFi connected at: "));
  Serial.println(WiFi.localIP());
  return true;
}


void APConfig::setupServer() {
  server = new AsyncWebServer(80);

  server->on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server->on("/src/font/1.otf", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SPIFFS, "/src/font/1.otf", "font/otf");
  });

  server->on("/src/img/favicon.ico", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SPIFFS, "/src/img/favicon.ico", "img");
  });

  //src/css/form.css
  server->on("/src/css/form.css", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SPIFFS, "/src/css/form.css", "text/css");
  });

  server->on("/sendVariables", HTTP_GET, [this](AsyncWebServerRequest* request) {
    String inputMessage;
    String inputParam;

    if (request->hasParam("ssid")) {
      inputMessage = request->getParam("ssid")->value();
      wifiSsid = inputMessage;
      Serial.println(inputMessage);
      writeDefaults();
    }

    if (request->hasParam("wifiPassword")) {
      inputMessage = request->getParam("wifiPassword")->value();
      wifiPassword = inputMessage;
      Serial.println(inputMessage);
      writeDefaults();
    }

    if (request->hasParam("mqttIP")) {
      inputMessage = request->getParam("mqttIP")->value();
      mqttIP = inputMessage;
      Serial.println(inputMessage);
      writeDefaults();
    }

    if (request->hasParam("port")) {
      inputMessage = request->getParam("port")->value();
      mqttPort = inputMessage;
      Serial.println(inputMessage);
      writeDefaults();
    }

    if (request->hasParam("client")) {
      inputMessage = request->getParam("client")->value();
      mqttClient = inputMessage;
      Serial.println(inputMessage);
      writeDefaults();
    }

    if (request->hasParam("mqttPassword")) {
      inputMessage = request->getParam("mqttPassword")->value();
      mqttPassword = inputMessage;
      Serial.println(inputMessage);
      writeDefaults();
    }
    /*
      preferences.begin("config", false);
  preferences.clear();

  preferences.putBool("setup", false);

  preferences.putString("wifiSsid", wifiSsid);
  preferences.putString("wifiPassword", wifiPassword);

  preferences.putString("mqttIP", mqttIP);
  preferences.putString("mqttClient", mqttClient);
  preferences.putString("mqttPort", mqttPort);
  preferences.putString("mqttPassword", mqttPassword);

  preferences.end();
  */

    request->send_P(200, "text/html", "Zariadenie sa restartuje");
    delay(1000);
    ESP.restart();
  });
}

bool APConfig::isDefaultState() {
  preferences.begin("config", false);

  setup = preferences.getBool("setup");
  Serial.println(setup);

  wifiSsid = preferences.getString("wifiSsid", "");
  wifiPassword = preferences.getString("wifiPassword", "");

  mqttIP = preferences.getString("mqttIP");
  mqttPort = preferences.getString("mqttPort");
  mqttClient = preferences.getString("mqttClient");
  mqttPassword = preferences.getString("mqttPassword");

  preferences.end();

  return setup;
}
