<div id="top"></div>
<!-- Template of README.mb inspired by https://github.com/othneildrew/Best-README-Template -->
<br />
<div align="center">
  <a href="https://github.com/Jajo1010/BananaPi-GPIO-SimpleWebSwitch">
    <img src="https://i.imgur.com/NqcXbFT.png" alt="Logo" width="200" height="200">
  </a>

  <h3 align="center">Smart termostat KOP</h3>

  <p align="center">
    ESP32 termostat integration to homebridge using MQTT
    <br />
    <br />
    <br />
  </p>
</div>


## Table of contents
  <ol>
    <li>
      <a href="#about-the-project">About The Project</a>
      <ul>
        <li><a href="#built-with">Built With</a></li>
      </ul>
    </li>
    <li>
      <a href="#getting-started">Getting Started</a>
      <ul>
        <li><a href="#prerequisites">Prerequisites</a></li>
        <li><a href="#installation">Installation</a></li>
      </ul>
    </li>
    <li><a href="#contact">Contact</a></li>
  </ol>



<!-- ABOUT THE PROJECT -->
## About The Project

</br>

<p align="center">
  <img src="https://i.imgur.com/iQ1g3a2.png" alt="Logo" width="377" height="325" >
</p>

Students highschool gradution project using modern ways to control IOT devices. MCU chosen in this project is ESP32-S3 and Dallas temperature sensors Ds18b2.The device is connected to homebridge using MQTT, homebridge then distributes this device to all kinds of home automation platforms such as :
* [Homekit](https://www.apple.com/home-app/accessories/) (mainly focused on)
* [Google Home](https://home.google.com/welcome/)  

<p align="center">
  <img src="https://i.imgur.com/7Li96Wu.jpg"  alt="Logo" width="401" height="809">
</p>
<p align="right">(<a href="#top">back to top</a>)</p>



### Built With

Frontend :
* HTML/CSS/JS
* [U8G2 OLED GFX Library](https://github.com/olikraus/u8g2)  with great support for different types OLEDs

Backend:
* [ESP Async Web Server](https://github.com/me-no-dev/ESPAsyncWebServer/)
* [PubSub MQTT Client](https://github.com/knolleary/pubsubclient)
* [Homebridge](https://homebridge.io/)

Raspberry Pi:
* [Raspberry Pi 2 Model B V1.1](https://www.raspberrypi.com/products/raspberry-pi-2-model-b/)

MCU:
* [ESP32-S3](https://www.espressif.com/en/products/socs/esp32-s3)

<p align="right">(<a href="#top">back to top</a>)</p>



<!-- GETTING STARTED -->
## Getting Started

To get a local copy up and running follow these simple example steps.

### <b>Setup Homebridge on RaspberryPi</b>
<ol>
  <li>Install Raspbian OS from <a href="https://www.raspberrypi.com/documentation/computers/getting-started.html">this tutorial</a></li>
  <li>Download and setup HomeBridge according <a href="https://github.com/homebridge/homebridge/wiki/Install-Homebridge-on-Raspbian">this tutorial</a></li>
  <li>Open Homebridge web UI, login and install these plugins</li>
  <ul>
    <li>Homebridge Mqttthing</li>
    <li>Aedes embedded MQTT Broker</li>
  </ul>
  <li>Setup Mqttthing according this JSON config</li>
  
  ```json
    {
      "type": "thermostat",
      "name": "Termostat",
      "url": "YOUR-MQTT-BROKER-ADRESS:PORT",
      "username": "YOUR-MQTT-CLIENT-NAME",
      "password": "YOUR-MQTT-CLIENT-PASSWORD",
      "logMqtt": true,
      "topics": {
          "getCurrentTemperature": "stat/thermostat/temperature",
          "getHeatingThresholdTemperature": "stat/thermostat/heatingThresholdTemperature",
          "setHeatingThresholdTemperature": "cmnd/thermostat/DisplayUnits/heatingThresholdTemperature",
          "getTargetHeatingCoolingState": "stat/thermostat/heatingCoolingState",
          "getTargetTemperature": "stat/thermostat/targetTemperature",
          "setTargetTemperature": "cmnd/thermostat/targetTemperature",
          "getTemperatureDisplayUnits": "stat/thermostat/temperatureDisplayUnits",
          "setTemperatureDisplayUnits": "cmnd/thermostat/setTemperatureDisplayUnits"
      },
      "accessory": "mqttthing"
   }
  ```
  
  <li>Setup Aedes MQTT Broker according this JSON config</li>
  
  ```json
  {
    "name": "MQTT",
    "host": "0.0.0.0",
    "auth": {
        "isEnabled": true,
        "username": "YOUR-MQTT-CLIENT-NAME",
        "password": "YOUR-MQTT-CLIENT-PASSWORD",
    },
    "_bridge": {
        "username": "0E:32:FB:65:E7:8C",
        "port": 36599
    },
    "platform": "HomebridgeAedes"
}
  ```
</ol>

### <b>Set up Arduino enviroment</b>
<ol>
  <li>Install latest Arduino IDE 2.X.X version</li>
  <li>Download ESP32 Boards go to <em> File -> Preferences -> Aditional boards manager URLs -> paste : https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json</em></li>
  <li>Go to <em>Tools -> Board -> esp32 -> select : ESP32S3 Dev module</em></li>
  <li>Install manually all these needed libraries</li>
  <ul>
  <li>Wire</li>
  <li>U8g2lib</li>
  <li>OneWire</li>
  <li>DallasTemperature</li>
  <li>NonBlockingDallas</li>
  <li>CircularBuffer</li>
  <li>WiFi</li>
  <li>PubSubClient</li>
  <li>APConfig</li>
  <li>AsyncTCP</li>
  <li>ESPAsyncWebServer</li>
  <li>AsyncElegantOTA</li>
  <li>TelnetStream</li>
  </ul>
  <li>For first install flash code into ESP32 directly, later upload code like this :</li>
  <li><strong>DUE TO UNSPORTED SPIFFS DATA UPLOAD, USE ARDUINO IDE 1.X.X AND UPLOAD CONTENT FROM DATA TROUGH THERE</strong> follow <a href="https://github.com/me-no-dev/arduino-esp32fs-plugin">these steps</a></li>
  <li>Select <em>Sketch -> Export Compiled library</em></li>
  <li>Select <em>Sketch -> Show Sketched Folder</em></li>
  <li>Find your ESP32S3s IP adress on local Network (for example <a href="https://www.advanced-ip-scanner.com/">Advanced IP Scanner</a>)</li>
  <li>Go to http:ESP32IPADRESS/update and upload the binary from exported binary path <strong><em>step 7</em></strong></li>
  <li>Connect on ESP32 Access point and configure the device</li>
</ol>


<!-- CONTACT -->
## Contact

Ladislav Štefún - ladislav.stefunjr@gmail.com

<p align="right">(<a href="#top">back to top</a>)</p>

