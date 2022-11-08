#include <Arduino.h>
#include <FS.h>
#include <SPIFFSEditor.h>
#include <ArduinoJson.h>

class ConfigFile
{
private:
    /* data */
public:
    ConfigFile(){
        
    };

    //write json to settings file
    void saveJsonFile(String configFile, DynamicJsonDocument jsonDocument){ 
      File file = SPIFFS.open(configFile, "w");                                           //load file

      serializeJson(jsonDocument, file);                                                  //print json object to file          
      file.close();                                                                       //close file
    }


    DynamicJsonDocument loadJsonFile(String name){
      DynamicJsonDocument jsonDocument(1024);

      File configFile = SPIFFS.open(name, "r");                                           //open config file

      if(configFile){
        size_t size = configFile.size();
        std::unique_ptr<char[]> buf(new char[size]);
        configFile.readBytes(buf.get(), size);
        configFile.close();
    
        deserializeJson(jsonDocument, buf.get());                                       //get json object
      }    
      return jsonDocument;
    }
};