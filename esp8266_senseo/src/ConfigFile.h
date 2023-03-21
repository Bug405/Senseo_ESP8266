#include <Arduino.h>
#include <LittleFS.h>
//#include <SPIFFSEditor.h>
#include <ArduinoJson.h>

#define CONFIGFILE

class ConfigFile
{
private:
    /* data */
public:
    ConfigFile(){
        
    };

    //write json to settings file
    void saveJsonFile(String configFile, DynamicJsonDocument jsonDocument){ 
      File file = LittleFS.open(configFile, "w");                                           //load file
      serializeJson(jsonDocument, file);                                                  //print json object to file          
      
      file.close();                                                                       //close file
    }


    DynamicJsonDocument loadJsonFile(String name){
      DynamicJsonDocument jsonDocument(2048);

      File configFile = LittleFS.open(name, "r");                                           //open config file

      if(configFile){
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        deserializeJson(jsonDocument, buf.get());                                       //get json object
        
        configFile.close();
      }

      return jsonDocument;
    }
};