#include <Arduino.h>

class SenseoState{
  
  private:

  String senseo_state = "off";                           //state of senseo

  bool flanke;                                           //flanke
  
  long prev = 0;                                         //prev time

  //get new state of input power LED
  bool valueListener(int input){
    bool value = digitalRead(input);                     //read input
  
    if(value != flanke){                                 //is value changed
      flanke = value; 
      return true;
    }
  
    return false;
  }

  
  public:

  //get state
  String getSenseoState(){
    return senseo_state;
  }

  //set state
  void setSenseoState(String state){
    senseo_state = state;
  }
  
  //get state of senseo  
  String getState(int input){
  
    bool value = digitalRead(input);                     //read input
    bool valueChange = valueListener(input);             //is value changed
  
    if(valueChange && value == true){
      prev = millis();                                   //reset timer
    }
  
    long time = millis() - prev;                         //time differnce
   
    if(value){
      if(time > 1500){
        return "ready";                                  //ready
      }
    } 
    
    else {
      if(time > 3000){
        return "off";                                    //off
      }
    }
  
    if(valueChange && !value){   
      time = millis() - prev;
  
      if(time < 0){
        return senseo_state;                             //falure (time < 0)
      }
      
      else if(time > 400){            
        return "busy";                                   //heating or make coffe 
      }
  
      else if(time < 400){
        return "failure";                                //no wather
      }
    }
  
    return senseo_state;
  }
};