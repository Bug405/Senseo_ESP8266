#include <Arduino.h>
#include <ArduinoOTA.h>
#include "ESPAsyncWebServer.h"
#include <ESPAsyncTCP.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <WiFiConnection.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include "SenseoState.h"
#include "ConfigFile.h"
#include <MakeCoffee.h>
#include <SenseoTimer.h>
#include <Ticker.h>
#include <AsyncMqttClient.h>
#include <MqttMessage.h>
#include <WebSocket.h>


//Initialize WiFi
WiFiConnection wifi;

const char *password = "SenseoESP8266";                                               //AP password
const char *mySsid = "SENSEO_CONFIG";                                                 //AP ssid

boolean restartWiFi;

String hostname = "Senseo";                                                           //hostname

//webserver login
const char* user = "senseo";                                                          //webserver user
const char* pwd = "senseo";                                                           //webserver password

//Initialize async webserver
AsyncWebServer server(80);                                                            //webserver on port 80
AsyncWebSocket socket("/ws");                                                         //socket
WebSocket webSocket;

int timeOffset = 0;                                                                   //offset time client in hour

//mqtt client
AsyncMqttClient mqttClient;
MqttMessage mqtt;
Ticker mqttReconnectTimer;

long checkMqqt;

String mqtt_host = "";
int mqtt_port = 1883;

String mqtt_user = "";
String mqtt_pwd = "";

String topic = "senseo/";
String stateTopic = "/state";

boolean useMqtt = false;

boolean restartMqtt;

WiFiUDP ntpUDP;

NTPClient timeClient(ntpUDP, "3.de.pool.ntp.org", 0, 60000);
boolean restartTimeClient;

String lang = "ger";                                                                  //language for website

//config files
String configWifI = "/configWifI.json";                                               //wifi config file name
String configLang = "/configFile.json";                                               //language config file name
String configTimers = "/configTimers.json";                                           //timer config file name
String configMqtt = "/configMqtt.json";

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
  wifi.setAP(mySsid, password, hostname);
  wifi.setConfigFile(configWifI);
  wifi.wifiConnect();
}

//init ota updates
void initOTA(){
  ArduinoOTA.setHostname("Senseo");                                                   //set ota hsotname
  ArduinoOTA.setPassword("senseo");                                                   //set ota password
  ArduinoOTA.begin();                                                                 //begin ota
}

// int filesystem
void intiFileSystem()
{
  LittleFS.begin(); // begin LittleFS
  Serial.println("LittleFS ready");
}

void reconnectMqtt(){
  if(WiFi.isConnected()){
    mqttClient.connect();
  }
}

void onMqttConnect(bool sessionPresent) {
  String inTopic = topic + "/press_button";
  stateTopic = topic + "/state";
  mqttClient.subscribe(inTopic.c_str(), 2);
  mqttClient.setWill(stateTopic.c_str(), 1, true, "not connected");
  mqttClient.publish(stateTopic.c_str(), 1, true, state.c_str());
  mqttClient.setKeepAlive(60);

  Serial.println("mqtt has connected");
}

void onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
  Serial.println("Disconnected from MQTT.");

  if (WiFi.isConnected()) {
    mqttReconnectTimer.once(60, reconnectMqtt);
  }
}

void initMqtt(){
  mqttClient.onConnect(onMqttConnect);
  mqttClient.onMessage(mqtt.onMqttMessage);

  if(mqttClient.connected()){
    mqttClient.disconnect();
  }

  if(useMqtt){
    mqttClient.setCredentials(mqtt_user.c_str(), mqtt_pwd.c_str());
    mqttClient.setServer(mqtt_host.c_str(), mqtt_port);
    mqttClient.connect();

    Serial.println("mqtt client is ready");
  }

  mqttClient.onDisconnect(onMqttDisconnect);
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

  if(!jsonDocument.isNull()){
    lang = jsonDocument["lang"].as<String>();
    timeOffset = jsonDocument["offset"].as<int>();
  }
}

void loadMqttConfig(){
  DynamicJsonDocument jsonDocument = configFile.loadJsonFile(configMqtt);

  if(!jsonDocument.isNull()){
    mqtt_host = jsonDocument["mqtt_host"].as<String>();
    mqtt_port = jsonDocument["mqtt_port"].as<int>();
    mqtt_user = jsonDocument["mqtt_user"].as<String>();
    mqtt_pwd = jsonDocument["mqtt_pwd"].as<String>();
    topic = jsonDocument["topic"].as<String>();
    useMqtt = jsonDocument["use_mqtt"].as<boolean>();
  }
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
  webSocket.startWebServer(&server);
}

//notify all clients
void notifyClients(String state) {
  DynamicJsonDocument document(1024);
  document["state"] = state;

  socket.textAll(document.as<String>());
}


void setup() {  
  Serial.begin(115200);                                                            //init serial

  starttime = millis();

  checkMqqt = millis();

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

  loadMqttConfig();                                                                //load mqtt config

  initTimeClient();                                                                //init time client

  delay(1000);

  initTimer();
  
  initWebSocket();                                                                 //init websocket

  startWebServer();                                                                //start webserver

  delay(1000);

  initMqtt();

  initOTA();                                                                       //init ota
}

void loop() { 
  //close ap afer 15 minutes
  if(WiFi.getMode() == WIFI_AP && millis() - starttime >= 15 * 60 * 1000){
    WiFi.mode(WIFI_OFF);                                                           //turn wifi off
    Serial.println("disconnect ap");     
  }

  if(restartWiFi){
    restartWiFi = false;
    wifi.wifiReconnect();                                                          //restart wifi if new settings arrive
  }

  if(restartMqtt){
    restartMqtt = false; 
    initMqtt();                                                                    //restart mqtt client if new settings arrive
  }
  
  if(millis() - checkMqqt >= 5 * 60 * 1000){                                       //check mqtt cenection state every 5 minutes
    if(!mqttClient.connected() && useMqtt){
      initMqtt();                                                                  //if mqtt is not connectet -> restart client
    }

    checkMqqt = millis();
  }

  ArduinoOTA.handle();                                                             //handle ota update

  timeClient.update();                                                             //update timeclient

  state = senseoState.getSenseoState();                                            //get last state from senseo       

  String senseo_state = senseoState.getState(state_input);                         //check senseo state

  //if senseo has a new state, send the state to all connected clients  
  if(!senseo_state.equals(state)){
    senseoState.setSenseoState(senseo_state);                                      //set new state in senseo
    notifyClients(senseo_state);                                                   //send new state

    if(mqttClient.connected()){
      mqttClient.publish(stateTopic.c_str(), 1, true, senseo_state.c_str());
    }
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

//get state as json
DynamicJsonDocument INFO(){
  DynamicJsonDocument document(1024);
  document["hour"] = timeClient.getHours();
  document["minute"] = timeClient.getMinutes();
  document["timeIsSet"] = timeClient.isTimeSet();
  document["mqtt_host"] = mqtt_host;
  document["mqtt_port"] = mqtt_port;
  document["mqtt_user"] = mqtt_user;
  document["use_mqtt"] = useMqtt;
  document["topic"] = topic;
  document["lang"] = lang;
  document["state"] = state;

  return document;
}


//handle web socket msg
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;

  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
   
    //send state
    if (strcmp((char*)data, "INFO") == 0) {  
       socket.text(client->id(), INFO().as<String>());       
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

          //write file if ssid and pwd > 0
          if(String(jsonDocument["ssid"].as<String>()).length() > 0 && 
          String(jsonDocument["password"].as<String>()).length() > 0){
            configFile.saveJsonFile(configWifI, jsonDocument);                      //write wifi settings file
          
            restartWiFi = true;
          }                                                                         //reconnect wifi
        }

        else if(jsonDocument.containsKey("mqtt_host") && jsonDocument.containsKey("mqtt_port") && jsonDocument.containsKey("topic")){
          Serial.println("get mqtt data");

          if(String(jsonDocument["mqtt_host"].as<String>()).length() > 0 && 
          String(jsonDocument["mqtt_port"].as<String>()).length() > 0 && 
          String(jsonDocument["topic"].as<String>()).length() > 0){
            
            if(!jsonDocument.containsKey("mqtt_pwd")){
              jsonDocument["mqtt_pwd"] = mqtt_pwd;
            } else{
              mqtt_pwd = jsonDocument["mqtt_pwd"].as<String>();
            }

            configFile.saveJsonFile(configMqtt, jsonDocument);                      //write wifi settings file
                      
            mqtt_host = jsonDocument["mqtt_host"].as<String>();
            mqtt_port = jsonDocument["mqtt_port"].as<int>();
            mqtt_user = jsonDocument["mqtt_user"].as<String>();
            topic = jsonDocument["topic"].as<String>();
            useMqtt = jsonDocument["use_mqtt"].as<boolean>();

            Serial.println(jsonDocument.as<String>());
            
            restartMqtt = true;                                                     //reconnect wifi
          }
        }

        else if(jsonDocument.containsKey("offset") && jsonDocument.containsKey("lang")){
          lang = jsonDocument["lang"].as<String>();                                 //set language   

          int offset = jsonDocument["offset"].as<int>();                            //set time offset

          if(timeClient.isTimeSet()){
            timeOffset = (timeOffset + offset) - timeClient.getHours();             //get new offset

            initTimeClient();                                                       //init timeClient
            
            DynamicJsonDocument document(1024);
            document["lang"] = lang;
            document["offset"] = timeOffset;
            
            Serial.println("set time offset: " + String(timeOffset));

            configFile.saveJsonFile(configLang, document);                          //write config file           
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
