#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "ESPAsyncWebServer.h"
#include <ESPAsyncTCP.h>

class WebSocket
{
private:

public:
    WebSocket();
    ~WebSocket();

    /*
    add requst handler 
    and start webserver
    */
    static void startWebServer(AsyncWebServer *server) {
      extern const char* user;
      extern const char* pwd;

      // send index.html to webclient
      server->on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        if (!request->authenticate(user, pwd)){
          return request->requestAuthentication(); // check authentification
        }

        request->send(LittleFS, "/index.html", "text/html"); // send file from SPIFFS to client
      });

      // send settings.html to webclient
      server->on("/settings", HTTP_GET, [](AsyncWebServerRequest *request){
        if (!request->authenticate(user, pwd)){
          return request->requestAuthentication(); // check authentification
        }

        request->send(LittleFS, "/settings.html", "text/html"); // send file from SPIFFS to client
      });

      // send timer.html to webclient
      server->on("/timer", HTTP_GET, [](AsyncWebServerRequest *request){
        if (!request->authenticate(user, pwd)){
          return request->requestAuthentication(); // check authentification
        }

        request->send(LittleFS, "/timer.html", "text/html"); // send file from SPIFFS to client
      });

      // send webSocket.js to webclient
      server->on("/webSocket.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/webSocket.js", "text/html"); // send file from SPIFFS to client
      });

      // send senseo.jpg to webclient
      server->on("/senseo.jpg", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(LittleFS, "/senseo.jpg", "text/css"); // send file from SPIFFS to client
      });

      // Start server
      server->begin(); // start webserver

      Serial.println("async web server ready");
    }
};



WebSocket::WebSocket()
{
}

WebSocket::~WebSocket()
{
}
