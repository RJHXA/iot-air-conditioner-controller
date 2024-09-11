#include <Pangodream_18650_CL.h>
#include <Arduino.h>

#define VBAT_PIN 35
#define BATTV_MAX    4.1     // maximum voltage of battery
#define BATTV_MIN    3.2     // what we regard as an empty battery
#define BATTV_LOW    3.4     // voltage considered to be low battery

void get_battery_level(){
    float battv = ((float)analogRead(VBAT_PIN) / 4095) * 3.3 * 2 * 1.05;

    Serial.print("batt: ");
    Serial.print(battv);
}