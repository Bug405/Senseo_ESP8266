#include <Arduino.h>
#include <AsyncMqttClient.h>

#ifdef MAKECOFFEE_H
#else
  #include <MakeCoffee.h>
#endif

class MqttMessage
{
private:
    /* data */
public:
    MqttMessage(/* args */);
    ~MqttMessage();

    static void onMqttMessage(char* topic, char* payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total) {
        extern MakeCoffee makeCoffee;
        
        String press_button = String(payload);

        if(len < press_button.length()){
            press_button = press_button.substring(0, len);
        }

        // if msg is "power" press power button
        // for 300 ms
        if (press_button.equals("power")){
            extern int power_button;

            makeCoffee.chancel();            // chancel make coffe when ready
            digitalWrite(power_button, LOW); // press power button
        }

        // if msg is "one_cup" press one cup button
        // for 300 ms
        else if (press_button.equals("one_cup")){
            extern int oneCup_button;
            digitalWrite(oneCup_button, LOW); // press one cup button
        }

        // if msg is "two_cups" press two cups button
        // for 300 ms
        else if (press_button.equals("two_cups")){
            extern int twoCups_button;
            digitalWrite(twoCups_button, LOW); // press two cups button
        }

        // if msg is "one_cup_whenReady" make one cup when ready
        else if (press_button.equals("one_cup_whenReady")){
            makeCoffee.makeOneCupCoffee(true); // make one cup when ready

            Serial.println("make one cup coffe when ready");
        }

        // if msg is "two_cups_whenReady" make two cups when ready
        else if (press_button.equals("two_cups_whenReady")){
            makeCoffee.makeTwoCupsCoffee(true); // make two cups when ready

            Serial.println("make two cups coffe when ready");
        }
    }
};

MqttMessage::MqttMessage(/* args */)
{
}

MqttMessage::~MqttMessage()
{
}
