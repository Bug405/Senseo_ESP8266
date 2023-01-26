#include <Arduino.h>

#define MAKECOFFEE_H

class MakeCoffee{
    private:

    boolean waitForReady;                                                               //waiting for senseo state "ready"

    boolean powerButtonIsPressed;                                                       //power button is pressed
    
    boolean edgeDetection;                                                              //edge detection for power button

    boolean makeOneCup;                                                                 //make one cup coffee

    boolean makeTwoCups;                                                                //make two cups coffee

    long startTime;

    public:

    MakeCoffee(){

    }

    //make one cup coffee
    void makeOneCupCoffee(boolean makeOneCup){
        this->makeOneCup = makeOneCup;
        this->makeTwoCups = false;
    }

    //make two cups coffee
    void makeTwoCupsCoffee(boolean makeTwoCups){
        this->makeTwoCups = makeTwoCups;
        this->makeOneCup = false;
    }

    //chancel make coffee
    void chancel(){
      this->makeOneCup = false;
      this->makeTwoCups = false;
      this->waitForReady = false;
    }
      
    //press power button
    boolean pressPowerOn(String state){
        powerButtonIsPressed = false;

        if(makeOneCup || makeTwoCups){
          if(!edgeDetection){
            waitForReady = true;
            startTime = millis();
              
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
        makeOneCup = false;
        makeTwoCups = false;
        
        return true;
      }
      
      return false;
    }

    //press one cup button
    boolean pressOneCupButton(boolean ready){
        if(waitForReady && makeOneCup && ready){
            waitForReady = false;
            makeOneCup = false;

            return true;
        }

        return false;
    }

    //press two cups button
    boolean pressTwoCupsButton(boolean ready){
        if(waitForReady && makeTwoCups && ready){
            waitForReady = false;
            makeTwoCups = false;

            return true;
        }

        return false;
    }
};

