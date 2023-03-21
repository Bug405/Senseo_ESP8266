#include <Arduino.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "ESPAsyncWebServer.h"
#include <ESPAsyncTCP.h>

class WiFiConnection
{
private:
    const char *password;                                                            //AP password
    const char *mySsid;                                                              //AP ssid

    String hostname;                                                                 //hostname

    String configWifI;                                                               //wifi config file name

    long starttime;

public:
    WiFiConnection(/* args */);
    ~WiFiConnection();

    void setAP(const char* ssid, const char* password, String hostname){
       this->mySsid = ssid;
       this->password = password;
       this->hostname = hostname;
    }

    void setConfigFile(String file){
       configWifI = file;
    }
    
    /*
    connect WiFi
    if wifi not avaible, open AP for wifi config
    */
    void wifiConnect(){
      if(LittleFS.exists(configWifI)){                                                      //check config file exist
        const char* ssid = "";
        const char* password = "";
  
        File configFile = LittleFS.open(configWifI, "r");                                   //open config file

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

            Serial.print("connecting Wifi");
            
            unsigned long startTime = millis();

            while (WiFi.status() != WL_CONNECTED){
              Serial.print(".");

              digitalWrite(LED_BUILTIN,!digitalRead(LED_BUILTIN));                        //blink led
        
              if ((unsigned long)(millis() - startTime) >= 10000){ 
                break;                                                                    //break if no connection is possible
              }

              delay(500);
           }
          }
        }
      }

      if (WiFi.status() != WL_CONNECTED){
        IPAddress local_ip(192,168,0,1);                                                      //local ip
        IPAddress gateway(192,168,0,2);                                                       //gateway
        IPAddress netmask(255,255,255,0);                                                     //netmask

        this->starttime = millis();

        WiFi.mode(WIFI_AP);                                                                   //else set wifi AP mode
        WiFi.softAPConfig(local_ip, gateway, netmask);                                        //config AP
        WiFi.softAP(mySsid, password);                                                        //start AP

        digitalWrite(LED_BUILTIN, HIGH);                                                       //set led off
      } else{
        digitalWrite(LED_BUILTIN, LOW);                                                      //set led on
      }

      Serial.println("");
      WiFi.printDiag(Serial);                                                                 //print connection state
      Serial.print("ip ");
      Serial.println(WiFi.localIP());
    }

    //reconnect wifi after get new settings
    void wifiReconnect(){    
      if(WiFi.isConnected()){
        WiFi.disconnect();                                                                    //if wifi is connected disconnect wifi 
      }
      
      delay(1000);

      wifiConnect();                                                                          //connect wifi
    }
    

    //close ap afer 15 minutes
    void AP_Connection(){
      if(WiFi.getMode() == WIFI_AP && millis() - starttime >= 15 * 60 * 1000){
        WiFi.mode(WIFI_OFF);                                                                 //turn wifi off
        Serial.println("disconnect ap");     
      }
    }
}; 

WiFiConnection::WiFiConnection(/* args */)
{
}

WiFiConnection::~WiFiConnection()
{
}
