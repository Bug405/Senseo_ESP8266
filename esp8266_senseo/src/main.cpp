#include <Arduino.h>
#include "ESPAsyncWebServer.h"
#include <ESPAsyncTCP.h>
#include <FS.h>
#include <SPIFFSEditor.h>
#include <ArduinoJson.h>
#include "SenseoState.h"


// Initialize WiFi
char *password = "SenseoESP8266";                                                     //AP password
char *mySsid = "SENSEO_CONFIG";                                                       //AP ssid                               

IPAddress local_ip(192,168,0,1);                                                      //local ip
IPAddress gateway(192,168,0,2);                                                       //gateway
IPAddress netmask(255,255,255,0);                                                     //netmask

String hostname = "Senseo8266";                                                           //hostname

// Initialize async webserver
AsyncWebServer server(80);                                                            //webserver on port 80
AsyncWebSocket socket("/ws");                                                         //socket

// webnserver login
const char* user = "senseo";                                                          //webserver user
const char* pwd = "senseo";                                                           //webserver password

String lang = "ger";                                                                  //language for website

// config files
String configWifI = "/configWifI.json";                                               //wifi config file name
String configLang = "/configLang.json";                                               //language config file name

int led = LED_BUILTIN;                                                                //led

int state_input = 4;                                                                  //gpio 4 (D2) power LED senseo
int power_button = 14;                                                                //gpio 14 (D5) power button
int oneCup_button = 12;                                                               //gpio 12 (D6) one cup button
int twoCups_button = 13;                                                              //gpio 13 (D7) two cups button

SenseoState senseoState;                                                              //init the senseo state class

String state = "off";                                                                 //state of senseo

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

      JsonObject& jObject = DynamicJsonBuffer().parseObject(buf.get());               //get json object

      if(jObject.success())                                                           //check json object
      {
        ssid = jObject["ssid"];                                                       //set ssid
        password = jObject["password"];                                               //set password

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

//write json to settings file
void handleSettingsUpdate(String configFile, JsonObject& var)
{ 
  File file = SPIFFS.open(configFile, "w");                                           //load file

  var.printTo(file);                                                                  //print json object to file          
  file.close();                                                                       //close file
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

  //send style_settings.css to webclient
  server.on("/style_settings.css", HTTP_GET, [](AsyncWebServerRequest *request){    
    request->send(SPIFFS, "/style_settings.css", "text/css");                       //send file from SPIFFS to client
  });

  //send senseo.jpg to webclient
  server.on("/senseo.jpg", HTTP_GET, [](AsyncWebServerRequest *request){    
    request->send(SPIFFS, "/senseo.jpg", "text/css");                               //send file from SPIFFS to client
  });

  //send style.css to webclient
  server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){    
    request->send(SPIFFS, "/style.css", "text/css");                                //send file from SPIFFS to client
  });

  //send script.js to webclient
  server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){ 
    request->send(SPIFFS, "/script.js", "text/html");                               //send file from SPIFFS to client
  });

  //send settings.js to webclient
  server.on("/settings.js", HTTP_GET, [](AsyncWebServerRequest *request){ 
    request->send(SPIFFS, "/settings.js", "text/html");                             //send file from SPIFFS to client
  });

  //send webSocket.js to webclient
  server.on("/webSocket.js", HTTP_GET, [](AsyncWebServerRequest *request){    
    request->send(SPIFFS, "/webSocket.js", "text/html");                            //send file from SPIFFS to client
  });

  // Start server 
  server.begin();                                                                   //start webserver

  Serial.println("async web server ready");
}

//notify all clients
void notifyClients(String state) {
  socket.textAll(state);
}

//handle web socket msg
void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;

  if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
    data[len] = 0;
   
    //send state
    if (strcmp((char*)data, "INFO") == 0) {  
       notifyClients(state);                                                       //notify clients state

       if(!lang.equals("ger")){
         notifyClients("eng");                                                     //notify clients languge
       }  
    }

    //if msg is "power" press power button
    //for 300 ms
    else if (strcmp((char*)data, "power") == 0) {  
      Serial.println("press power button");

      digitalWrite(power_button, LOW);                                             //press power button
    }

    //if msg is "one_cup" press one cup button
    //for 300 ms
    else if (strcmp((char*)data, "one_cup") == 0) {  
      Serial.println("press one cup button");

      digitalWrite(oneCup_button, LOW);                                            //press one cup button 
    }

    //if msg is "two_cups" press two cups button
    //for 300 ms
    else if (strcmp((char*)data, "two_cups") == 0) {  
      Serial.println("press two cups button");   

      digitalWrite(twoCups_button, LOW);                                           //press two cups button
    }

    else{
      JsonObject& jObject = DynamicJsonBuffer().parseObject(data);                //get json object
      
      if(jObject.success()){                                                      //check json object success
        if(jObject.containsKey("ssid") && jObject.containsKey("password")){
          Serial.println("get wifi data");
          Serial.println("ssid: " + String(jObject["ssid"].asString()));
          Serial.println("password: " + String(jObject["password"].asString()));

          handleSettingsUpdate(configWifI, jObject);                             //write wifi settings file
          
          delay(1000);
          
          system_restart();                                                      //restart system
        }

        else if(jObject.containsKey("lang")){
          lang = jObject["lang"].asString();                                     //set language
          Serial.println("set languge: " + lang);

          handleSettingsUpdate(configLang, jObject);                             //write wifi settings file         
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
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(),    //client has connected
      client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());        //client has disconnected
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);                                    //handle websocket msg
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}

//set language for website
void setLanguage(){
  File configFile = SPIFFS.open(configLang, "r");                               //open config file

    if(configFile){
      size_t size = configFile.size();
      std::unique_ptr<char[]> buf(new char[size]);
      configFile.readBytes(buf.get(), size);
      configFile.close();

      JsonObject& jObject = DynamicJsonBuffer().parseObject(buf.get());        //get json object

      if(jObject.success())                                                    //check json object
      {
        lang = jObject["lang"].asString();                                     //set lang
      }
    }
}

void setup() {
  Serial.begin(115200);                                                       //init serial

  pinMode(led, OUTPUT);                                                       //init LED

  pinMode(power_button, OUTPUT);                                              //init power button output
  digitalWrite(power_button, HIGH);

  pinMode(oneCup_button, OUTPUT);                                             //init one cup button output
  digitalWrite(oneCup_button, HIGH);

  pinMode(twoCups_button, OUTPUT);                                            //init two cups button output
  digitalWrite(twoCups_button, HIGH);

  pinMode(state_input, INPUT_PULLUP);                                         //init power led input
  
  intiFileSystem();                                                           //init file system

  delay(1000);                                                                //wait

  wifiConnect();                                                              //connect wifi

  delay(1000);                                                                //wait

  setLanguage();                                                              //set language for websocket
  
  initWebSocket();                                                            //init websocket

  startWebServer();                                                           //start webserver
}

void loop() {
  state = senseoState.getSenseoState();                                       //get last state from senseo       

  String senseo_state = senseoState.getState(state_input);                    //check senseo state

  //if senseo has a new state, send the state to all connected clients  
  if(!senseo_state.equals(state)){
    senseoState.setSenseoState(senseo_state);                                 //set new state in senseo
    notifyClients(senseo_state);                                              //send new state
  }

  //it is impossible use delay in websocket threed!

  //hold power button for 300 ms
  if(!digitalRead(power_button)){
    delay(300);                                                               //sleep 300 ms
    digitalWrite(power_button, HIGH);                                         //release power button                                         
  }

  //hold one cup button for 300 ms
  if(!digitalRead(oneCup_button)){
    delay(300);                                                               //sleep 300 ms
    digitalWrite(oneCup_button, HIGH);                                        //release one cup button 
  }
  
  //holdtwo cups button for 300 ms
  if(!digitalRead(twoCups_button)){
    delay(300);                                                               //sleep 300 ms
    digitalWrite(twoCups_button, HIGH);                                       //release two cups button 
  }
}