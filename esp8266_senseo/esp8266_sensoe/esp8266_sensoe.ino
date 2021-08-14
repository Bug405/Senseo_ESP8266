#include "ESP8266WiFi.h"
#include "SenseoState.h"

const char* ssid = "your-ssid";                                       //the ssid of wifi
const char* password = "your-password";                               //the password of wifi

WiFiServer server(8266);                                              //start server on port 8266

WiFiClient clients[3];                                                //clients array

String hostname = "Senseo";                                           //hostname

int clientNumber;                                                     //number of client in array

int state_input = 4;                                                  //gpio 4 (D2) power LED senseo

int power_button = 14;                                                //gpio 14 (D5) power button

int oneCup_button = 12;                                               //gpio 12 (D6) one cup button

int twoCups_button = 13;                                              //gpio 13 (D7) two cups button

SenseoState senseoState;                                              //intialisize the senseo state class


//setup program
void setup() { 

  pinMode(LED_BUILTIN, OUTPUT);                                       //intialisize LED

  pinMode(power_button, OUTPUT);                                      //intialisize power button output
  digitalWrite(power_button, HIGH);

  pinMode(oneCup_button, OUTPUT);                                     //intialisize one cup button output
  digitalWrite(oneCup_button, HIGH);

  pinMode(twoCups_button, OUTPUT);                                    //intialisize two cups button output
  digitalWrite(twoCups_button, HIGH);

  pinMode(state_input, INPUT_PULLUP);                                 //intialisize power led input

  Serial.begin(115200);                                               //intialisize serial output

  delay(1000);                                                        //wait

  WiFi.begin(ssid, password);                                         //intialisize WiFi

  WiFi.hostname(hostname);                                            //set WiFi hostname

  while(WiFi.status() != WL_CONNECTED){                               //wait for WiFi connection
    delay(1000);
    Serial.println("connecting..");
  }

  Serial.print("Connected WiFi with IP: ");

  Serial.println(WiFi.localIP());

  server.begin();                                                     //intialisize server
}

//read msg from client
void readClient(WiFiClient client){

  if(client.connected()){
    String msg;

    //read msg from client
    while(client.available() > 0)
    {
      msg = client.readStringUntil('\n');                             //read line from msg
      //Serial.println(msg);      
    }

    //if msg is "power" press power button
    //for 300 ms
    if(msg == "power"){
      Serial.println("press power button");

      digitalWrite(power_button, LOW);                                //press power button         
      delay(300);                                                     //sleep 300 ms
      digitalWrite(power_button, HIGH);                               //release power button
    } 

    //if msg is "one_cup" press one cup button
    //for 300 ms
    if(msg == "one_cup"){
      Serial.println("press one cup button");

      digitalWrite(oneCup_button, LOW);                               //press one cup button 
      delay(300);                                                     //sleep 300 ms
      digitalWrite(oneCup_button, HIGH);                              //release one cup button
    } 

    //if msg is "two_cups" press two cups button
    //for 300 ms
    if(msg == "two_cups"){
      Serial.println("press two cups button");   

      digitalWrite(twoCups_button, LOW);                              //press two cups button
      delay(300);                                                     //sleep 300 ms
      digitalWrite(twoCups_button, HIGH);                             //release two cups button
    }

    //if msg is "disconnect" disconnect client
    else if(msg == "disconnect"){
      client.stop();                                                  //disconnect client
      Serial.println("client disconnected");
    }

    delay(10);
  }else{
    //close client
    client.stop();                                                    //disconnect client
    Serial.println("client disconnected");
  } 
}   
    
void loop() {
  //wait for client connected
  WiFiClient client = server.available();                             //new client connected

  String senseo_state = senseoState.getSenseoState();                 //get last state from senseo           

  //set new client
  if(client){
    Serial.println("client has connected");

    //send state to client
    client.println(senseo_state);                                     //send state
    client.flush();                                                   //flush msg

    //clear client
    clients[clientNumber].stop();                                     //disconnect client
    
    //add client in cliets array
    clients[clientNumber] = client;                                   //set new client in array

    //set new position for next client 
    clientNumber++;

    //set next client number
    if(clientNumber >= 3){
      clientNumber = 0;
    }
  }

  //read new msg from clients
  for(int z = 0; z < 3; z++){
    if(clients[z]){
      readClient(clients[z]);                                         //read msg
    }
  }  

  //check senseo state
  String state = senseoState.getState(state_input);

  //if senseo has a new state, send the state to all connected clients  
  if(senseo_state != state){
    senseoState.setSenseoState(state);                               //set new state in senseo

    for(int z = 0; z < 3; z++){
      if(clients[z]){
        clients[z].println(state);                                   //send new state to clients
        Serial.println("send state " + state + " to client");
      }
    }    
  }
}
