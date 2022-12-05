#include <Arduino.h>
#include <NTPClient.h>
#include <WiFiUdp.h>

#define SENSEOTIMER_H

class SenseoTimer{
    private:

    NTPClient* timeClient;                                                              //ntp client

    boolean active;                                                                     //timer is active

    boolean visible;                                                                     //timer is visible

    boolean waitForReady;                                                               //waiting for senseo state "ready"

    boolean powerButtonIsPressed;                                                       //power button is pressed
    
    boolean edgeDetection;                                                              //edge detection for power button

    boolean saveFile;

    int makeCoffeeWhenReady;                                                            //0 = false & reset timer; 1 = false & daly; 2 = one cup; 3 = two cups

    int hour;                                                                           //hour

    int minute;                                                                         //minute

    long startTime;

    public:

    SenseoTimer(){

    }

    SenseoTimer(NTPClient* timeClient){
        this->timeClient = timeClient;
    }

    //get timer is active
    boolean isActive(){
        return active;
    }

    //set timer active
    void setActive(boolean active){
        this->active = active;
    }

    //get timer is visible
    boolean isVisible(){
        return visible;
    }

    //set timer visible
    void setVisible(boolean visible){
        this->visible = visible;
    }

    //get make coffee when ready
    int getCoffeeWhenReady(){
        return makeCoffeeWhenReady;
    }

    //set make coffee when ready
    void setCoffeeWhenReady(int makeCoffeeWhenReady){
        this->makeCoffeeWhenReady = makeCoffeeWhenReady;
    }
    
    //get hour
    int getHour(){
        return hour;
    };

    //get minute
    int getMinute(){
        return minute;
    };

    //set timer
    void setTimer(int hour, int minute){
        this->hour = hour;
        this->minute = minute;
    }
    
    //press power button
    boolean pressPowerOn(String state){
        powerButtonIsPressed = false;

        saveFile = false;

        if(timeClient->getHours() == hour && 
           timeClient->getMinutes() == minute && 
           timeClient->getSeconds() == 0){

            if(!edgeDetection){
              if(makeCoffeeWhenReady > 1){
                Serial.println("wait for ready");

                waitForReady = true;
                startTime = millis();
              }
              
              if(makeCoffeeWhenReady == 0){
                active = false;
                saveFile = true;
              }
              
              if(state.equals("off")){
                powerButtonIsPressed = true;
              }
            }

            edgeDetection = true;
        }else{
          edgeDetection = false;
        }
           
        return powerButtonIsPressed;
    }
    
    //check heating time max (over 2 minutes) 
    boolean isHeatingTimerMax(){
      if(waitForReady && millis() - startTime >= 2 * 60 * 1000){          
        waitForReady = false;
        active = false;
        saveFile = true;
        return true;
      }
      
      return false;
    }

    //press one cup button
    boolean pressOneCupButton(boolean ready){
        if(waitForReady && makeCoffeeWhenReady == 2 && ready){
            waitForReady = false;
            active = false;
            saveFile = true;
            return true;
        }

        return false;
    }

    //press two cups button
    boolean pressTwoCupsButton(boolean ready){
        if(waitForReady && makeCoffeeWhenReady == 3 && ready){
            waitForReady = false;
            active = false;
            saveFile = true;
            return true;
        }

        return false;
    }

    //save file
    boolean isSaveFile(){
      return saveFile;
    }

    DynamicJsonDocument getTimerAsJson(){
      DynamicJsonDocument document(256);

      document["active"] = active;
      document["visible"] = visible;
      document["makeCoffeeWhenReady"] = makeCoffeeWhenReady;
      document["hour"] = hour;
      document["minute"] = minute;

      return document;
    }

    boolean setTimerFromJson(String json){
      boolean success = false;
    
      DynamicJsonDocument document(1024);
      DeserializationError error = deserializeJson(document, json);    //get json object
                
      if(!error){                                                                                  //check json object success
        if(document.containsKey("active") && document.containsKey("makeCoffeeWhenReady") &&
          document.containsKey("visible") && document.containsKey("hour") && document.containsKey("minute")){
   
          if(String(document["active"].as<String>()).length() > 0 && 
             String(document["visible"].as<String>()).length() > 0 &&
             String(document["makeCoffeeWhenReady"].as<String>()).length() > 0 && 
             String(document["hour"].as<String>()).length() > 0 && 
             String(document["minute"].as<String>()).length() > 0){

            boolean active = document["active"].as<boolean>();                                       //timer is active
            boolean visible = document["visible"].as<boolean>();                                     //timer is visible
            int makeCoffeeWhenReady = document["makeCoffeeWhenReady"].as<int>();                     //0 = false & reset timer; 1 = false & daly; 2 = one cup; 3 = two cups
            int hour = document["hour"].as<int>();   
            int minute = document["minute"].as<int>(); 

  
            this->active = active;
            this->visible = visible;
            this->makeCoffeeWhenReady = makeCoffeeWhenReady;
            this->hour = hour;
            this->minute = minute;

            success = true;
          }
        }
      }
      waitForReady = false;
      return success;
    }
};

