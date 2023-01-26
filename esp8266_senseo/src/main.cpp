#include <Arduino.h>
#include <ArduinoOTA.h>
#include "ESPAsyncWebServer.h"
#include <ESPAsyncTCP.h>
#include <FS.h>
#include <SPIFFSEditor.h>
#include <ArduinoJson.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "SenseoState.h"
#include "ConfigFile.h"
#include <MakeCoffee.h>
#include <SenseoTimer.h>


//Initialize WiFi
char *password = "SenseoESP8266";                                                     //AP password
char *mySsid = "SENSEO_CONFIG";                                                       //AP ssid

boolean restartWiFi;

IPAddress local_ip(192,168,0,1);                                                      //local ip
IPAddress gateway(192,168,0,2);                                                       //gateway
IPAddress netmask(255,255,255,0);                                                     //netmask

String hostname = "Senseo";                                                           //hostname

//Initialize async webserver
AsyncWebServer server(80);                                                            //webserver on port 80
AsyncWebSocket socket("/ws");                                                         //socket

//webserver login
const char* user = "senseo";                                                          //webserver user
const char* pwd = "senseo";                                                           //webserver password

int timeOffset = 0;                                                                   //offset time client in hour

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "3.de.pool.ntp.org", 0, 60000);

String lang = "ger";                                                                  //language for website

//config files
String configWifI = "/configWifI.json";                                               //wifi config file name
String configLang = "/configFile.json";                                               //language config file name
String configTimers = "/configTimers.json";                                           //timer config file name

int led = LED_BUILTIN;                                                                //led

int state_input = 4;                                                                  //gpio 4 (D2) power LED senseo
int power_button = 14;                                                                //gpio 14 (D5) power button
int oneCup_button = 12;                                                               //gpio 12 (D6) one cup button
int twoCups_button = 13;                                                              //gpio 13 (D7) two cups button

MakeCoffee makeCoffee;

SenseoState senseoState;                                                              //init the senseo state class

const int timerLength = 3;                                                            //length timer array

SenseoTimer timers[timerLength];                                                      //init the timer array

ConfigFile configFile;                                                                //init config file

String state = "off";                                                                 //state of senseo

long starttime;                                                                       //starttime

void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client,
 AwsEventType type, void *arg, uint8_t *data, size_t len);
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len);

JsonObject& getJsonObj(File file);

/*
  connect WiFi
  if wifi not avaible, open AP for wifi config
*/
void wifiConnect(){
  if(SPIFFS.exists(configWifI)){                                                      //check config file exist
    const char* ssid = "";
    const char* password = "";
    
    File configFile = SPIFFS.open(configWifI, "r");                                   //open config file

    if(configFile){
      size_t size = configFile.size();
      std::unique_ptr<char[]> buf(new char[size]);
      configFile.readBytes(buf.get(), size);
      configFile.close();
      
      DynamicJsonDocument jsonDocument(1024);
      DeserializationError error = deserializeJson(jsonDocument, buf.get());          //get json object

      if(!error){                                                                     //check json object
        ssid = jsonDocument["ssid"];                                                  //set ssid
        password = jsonDocument["password"];                                          //set password

        WiFi.mode(WIFI_STA);                                                          //set wifi sta mode
        WiFi.begin(ssid, password);                                                   //start wifi connection
        WiFi.hostname(hostname);                                                      //set hostname in network

        unsigned long startTime = millis();

        Serial.print("connecting Wifi");
        
        while (WiFi.status() != WL_CONNECTED)                                         //while wifi is not connected
        {
          delay(500);
          Serial.print(".");
          digitalWrite(led,!digitalRead(led));                                        //blink led

          if ((unsigned long)(millis() - startTime) >= 10000){ 
            break;                                                                    //break if no connection is possible
          }
        }
      }
    }
  }

  if (WiFi.status() == WL_CONNECTED){
    digitalWrite(led, LOW);                                                           //set led on if wifi is connected                  
  } else {
    WiFi.mode(WIFI_AP);                                                               //else set wifi AP mode
    WiFi.softAPConfig(local_ip, gateway, netmask);                                    //config AP
    WiFi.softAP(mySsid, password);                                                    //start AP
    digitalWrite(led,HIGH);                                                           //set led off
  }

  Serial.println("");
  WiFi.printDiag(Serial);                                                             //print connection state
  Serial.print("ip ");
  Serial.println(WiFi.localIP());
}

//reconnect wifi after get new settings
void wifiReconnect(boolean reconnect){
  if(reconnect){
    if(WiFi.isConnected()){
      WiFi.disconnect();                                                              //if wifi is connected disconnect wifi 
    }

    restartWiFi = false;

    delay(1000);

    wifiConnect();                                                                    //connect wifi
  }
}

//init ota updates
void initOTA(){
  ArduinoOTA.setHostname("Senseo");                                                   //set ota hsotname
  ArduinoOTA.setPassword("senseo");                                                   //set ota password
  ArduinoOTA.begin();                                                                 //begin ota
}

//int filesystem
void intiFileSystem(){
  SPIFFS.begin();                                                                     //begin SPIFFS
  Serial.println("SPIFFS ready");
}

//init websocket
void initWebSocket() {
  socket.onEvent(onEvent);                                                            //set web socket event
  server.addHandler(&socket);                                                         //add handler

  Serial.println("WebSocket ready");
}

//init timeClient
void initTimeClient() {
  timeClient = NTPClient(ntpUDP, "3.de.pool.ntp.org", timeOffset * 3600, 60000);
  timeClient.begin();                                                                //begin timeclient
  timeClient.update();                                                               //update time

  Serial.print("Current time: ");
  Serial.println(timeClient.getFormattedTime());
} 

//set language for website
void loadConfig(){
  DynamicJsonDocument jsonDocument = configFile.loadJsonFile(configLang);           //load config
  lang = jsonDocument["lang"].as<String>();
  timeOffset = jsonDocument["offset"].as<int>();
}

void saveConfig(String lang, int offset){                                           //save config gile
  DynamicJsonDocument document(1024);
  document["lang"] = lang;
  document["offset"] = offset;

  configFile.saveJsonFile(configLang, document);
}

//get timer as json
DynamicJsonDocument getTimerAsJson(){
  DynamicJsonDocument document(1024);
  document["null"] = "null";

  JsonArray array = document.createNestedArray("timerArray");                       //build json array

  for(int i = 0; i < timerLength; i++){
    DynamicJsonDocument timer = timers[i].getTimerAsJson();                         //get timer as json
    array.add(timer);                                                               //add timer
  }

  return document;
}

//set timer 
boolean setTimers(DynamicJsonDocument jsonDocument){
  boolean success = true;

  JsonArray array = jsonDocument["timerArray"];                                    //get json array

  //reset timers
  for (int i = 0; i < timerLength; i++){
    timers[i].setActive(false);
    timers[i].setVisible(false);
  }

  for (size_t i = 0; i < array.size() && i < timerLength; i++){
    success = timers[i].setTimerFromJson(array.getElement(i).as<String>());        //set timer from json

    if(!success){
      break;
    }
  }

  return success;
}

//get state as json
DynamicJsonDocument getState(){
  DynamicJsonDocument document(1024);
  document["hour"] = timeClient.getHours();
  document["minute"] = timeClient.getMinutes();
  document["timeIsSet"] = timeClient.isTimeSet();
  document["lang"] = lang;
  document["state"] = state;

  return document;
}

//init timers
void initTimer(){
  DynamicJsonDocument jsonDocument = configFile.loadJsonFile(configTimers);         //load timer configfile

  for(int i = 0; i < timerLength; i++){
    timers[i] = SenseoTimer(&timeClient);                                           //init timers
  }

  if(jsonDocument.containsKey("timerArray")){
    setTimers(jsonDocument);                                                        //set timers
    Serial.println("init timers");
  }
}

/*
  add requst handler 
  and start webserver
*/
void startWebServer() {

  //send index.html to webclient
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    if(!request->authenticate(user, pwd)){
      return request->requestAuthentication();                                       //check authentification
    }

    request->send(SPIFFS, "/index.html", "text/html");                               //send file from SPIFFS to client
  });

  //send settings.html to webclient
  server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request){  
    if(!request->authenticate(user, pwd)){
      return request->requestAuthentication();                                       //check authentification
    }

    request->send(SPIFFS, "/settings.html", "text/html");                            //send file from SPIFFS to client
  });

  //send timer.html to webclient
  server.on("/timer", HTTP_GET, [](AsyncWebServerRequest *request){  
    if(!request->authenticate(user, pwd)){
      return request->requestAuthentication();                                       //check authentification
    }

    request->send(SPIFFS, "/timer.html", "text/html");                               //send file from SPIFFS to client
  });

    //send webSocket.js to webclient
  server.on("/webSocket.js", HTTP_GET, [](AsyncWebServerRequest *request){    
    request->send(SPIFFS, "/webSocket.js", "text/html");                            //send file from SPIFFS to client
  });

  //send senseo.jpg to webclient
  server.on("/senseo.jpg", HTTP_GET, [](AsyncWebServerRequest *request){    
    request->send(SPIFFS, "/senseo.jpg", "text/css");                               //send file from SPIFFS to client
  });

  // Start server 
  server.begin();                                                                   //start webserver

  Serial.println("async web server ready");
}

//notify all clients
void notifyClients(String state) {
  DynamicJsonDocument document(1024);
  document["state"] = state;

  socket.textAll(document.as<String>());
}

//handle web socket msg
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;

  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
   
    //send state
    if (strcmp((char*)data, "INFO") == 0) {  
       socket.text(client->id(), getState().as<String>());       
    }

    //send timers
    else if (strcmp((char*)data, "getTimers") == 0) {  
       socket.text(client->id(), getTimerAsJson().as<String>());                   //send timer as json string to client
    }

    else{  
      DynamicJsonDocument jsonDocument(1024);
      DeserializationError error = deserializeJson(jsonDocument, data);            //get json object
      
      //check json object success
      if(!error){
        if(jsonDocument.containsKey("press_button")){
          String press_button = jsonDocument["press_button"].as<String>();

          //if msg is "power" press power button
          //for 300 ms
          if (press_button.equals("power")) { 
            makeCoffee.chancel();                                                  //chancel make coffe when ready 
            digitalWrite(power_button, LOW);                                       //press power button
          }

          //if msg is "one_cup" press one cup button
          //for 300 ms
          else if (press_button.equals("one_cup")) {  
            digitalWrite(oneCup_button, LOW);                                       //press one cup button 
          }

          //if msg is "two_cups" press two cups button
          //for 300 ms
          else if (press_button.equals("two_cups")) {  
            digitalWrite(twoCups_button, LOW);                                       //press two cups button
          }

          //if msg is "one_cup_whenReady" make one cup when ready
          else if (press_button.equals("one_cup_whenReady")) {  
            makeCoffee.makeOneCupCoffee(true);                                       //make one cup when ready
            notifyClients("makeOneCupWhenReady");

            Serial.println("make one cup coffe when ready");
          }

          //if msg is "two_cups_whenReady" make two cups when ready
          else if (press_button.equals("two_cups_whenReady")) {  
            makeCoffee.makeTwoCupsCoffee(true);                                      //make two cups when ready
            notifyClients("makeTwoCupsWhenReady");
            
            Serial.println("make two cups coffe when ready");
          }
        }
                                                                
        else if(jsonDocument.containsKey("ssid") && jsonDocument.containsKey("password")){
          Serial.println("get wifi data");
          Serial.println("ssid: " + String(jsonDocument["ssid"].as<String>()));
          Serial.println("password: " + String(jsonDocument["password"].as<String>()));

          //write file if ssid and pwd > 0
          if(String(jsonDocument["ssid"].as<String>()).length() > 0 && 
          String(jsonDocument["password"].as<String>()).length() > 0){
            configFile.saveJsonFile(configWifI, jsonDocument);                      //write wifi settings file
          
            restartWiFi = true;
          }                                                                         //reconnect wifi
        }

        else if(jsonDocument.containsKey("lang")){
          lang = jsonDocument["lang"].as<String>();                                 //set language          

          saveConfig(lang, timeOffset);                                             //write config file 
          Serial.println("set language: " + lang);       
        }

        else if(jsonDocument.containsKey("offset")){
          int offset = jsonDocument["offset"].as<int>();                            //set time offset

          if(timeClient.isTimeSet()){
            timeOffset = (timeOffset + offset) - timeClient.getHours();             //get new offset

            initTimeClient();                                                       //init timeClient
            
            Serial.println("set time offset: " + String(timeOffset));

            saveConfig(lang, timeOffset);                                           //write config file            
          }       
        }

        else if(jsonDocument.containsKey("timerArray")){         
          boolean success = setTimers(jsonDocument);

          if(success){
            configFile.saveJsonFile(configTimers, jsonDocument);                    //write timers config file
                       
            Serial.println("write timer file success");

            DynamicJsonDocument document(1024);
            document["timerUpdateSuccess"] = success;
            socket.text(client->id(), document.as<String>());                        //send client timer update succes
          } else {
            Serial.println("write timer update fail");
          }
        }
      }
    }
  }
}

//handle server event
void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
             void *arg, uint8_t *data, size_t len) {
  switch (type) {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(),      //client has connected
      client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());          //client has disconnected
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len, client);                              //handle websocket msg
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

void setup() {  
  Serial.begin(115200);                                                            //init serial

  starttime = millis();

  pinMode(led, OUTPUT);                                                            //init LED

  pinMode(power_button, OUTPUT);                                                   //init power button output
  digitalWrite(power_button, HIGH);   

  pinMode(oneCup_button, OUTPUT);                                                  //init one cup button output
  digitalWrite(oneCup_button, HIGH);

  pinMode(twoCups_button, OUTPUT);                                                 //init two cups button output
  digitalWrite(twoCups_button, HIGH);
                             
  pinMode(state_input, INPUT_PULLUP);                                              //init power led input
  
  intiFileSystem();                                                                //init file system

  delay(1000);                                                                     //wait
 
  wifiConnect();                                                                   //connect wifi

  delay(1000);                                                                     //wait

  loadConfig();                                                                    //set language for websocket

  initTimeClient();                                                                //init time client

  delay(1000);

  initTimer();
  
  initWebSocket();                                                                 //init websocket

  startWebServer();                                                                //start webserver

  initOTA();                                                                       //init ota
}

void loop() { 
  //close ap afer 15 minutes
  if(WiFi.getMode() == WIFI_AP && millis() - starttime >= 15 * 60 * 1000){
    WiFi.mode(WIFI_OFF);                                                           //turn wifi off
    Serial.println("disconnect ap");     
  }

  wifiReconnect(restartWiFi);                                                      //restart wifi if new settings arrive

  ArduinoOTA.handle();                                                             //handle ota update

  timeClient.update();                                                             //update timeclient

  state = senseoState.getSenseoState();                                            //get last state from senseo       

  String senseo_state = senseoState.getState(state_input);                         //check senseo state

  //if senseo has a new state, send the state to all connected clients  
  if(!senseo_state.equals(state)){
    senseoState.setSenseoState(senseo_state);                                      //set new state in senseo
    notifyClients(senseo_state);                                                   //send new state
  }

  //hold power button for 300 ms
  if(!digitalRead(power_button)){
    Serial.println("press power button");

    delay(300);                                                                    //sleep 300 ms
    digitalWrite(power_button, HIGH);                                              //release power button                                         
  }

  //hold one cup button for 300 ms
  if(!digitalRead(oneCup_button)){
    Serial.println("press one cup button");

    delay(300);                                                                    //sleep 300 ms
    digitalWrite(oneCup_button, HIGH);                                             //release one cup button 
  }
  
  //hold two cups button for 300 ms
  if(!digitalRead(twoCups_button)){
    Serial.println("press two cups button");

    delay(300);                                                                    //sleep 300 ms
    digitalWrite(twoCups_button, HIGH);                                            //release two cups button 
  }

  //make coffee when ready
  if(makeCoffee.pressPowerOn(senseo_state)){
    digitalWrite(power_button, LOW);                                              //make coffee power button 
  }

  if(makeCoffee.isHeatingTimerMax()){
    Serial.println("heattime max: set wait for ready back");                      //check max time ready state
  }

  //press power one cup for 300 ms
  if(makeCoffee.pressOneCupButton(senseoState.isSecureReady())){
    digitalWrite(oneCup_button, LOW);                                             //make coffee one cup button         
  }

  //press two cups button for 300 ms
  if(makeCoffee.pressTwoCupsButton(senseoState.isSecureReady())){
    digitalWrite(twoCups_button, LOW);                                            //make coffee two cups button
  }
  
  //timer
  if(timeClient.isTimeSet()){   
    for(int i = 0; i < timerLength; i++){
      if(timers[i].isActive()){

        //press power button for 300 ms
        if(timers[i].pressPowerOn(senseo_state)){
          digitalWrite(power_button, LOW);                                         //timer press power button 
        }

        if(timers[i].isHeatingTimerMax()){
          Serial.println("heattime max: set wait for ready back");                 //check max time ready state
        }

        //press power one cup for 300 ms
        if(timers[i].pressOneCupButton(senseoState.isSecureReady())){
          digitalWrite(oneCup_button, LOW);                                        //timer press one cup button         
        }

        //press two cups button for 300 ms
        if(timers[i].pressTwoCupsButton(senseoState.isSecureReady())){
          digitalWrite(twoCups_button, LOW);                                       //timer press two cups button
        }
        
        //update timer config file if active state change
        if(timers[i].isSaveFile()){
          configFile.saveJsonFile(configTimers, getTimerAsJson());                 //update timers config file
        }
      }
    }
  }
}