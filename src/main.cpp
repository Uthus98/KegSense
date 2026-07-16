#include <Arduino.h>
#include "settings.h"
#include "weight.h"
#include "web.h"

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("----------------------");
    Serial.println(" KegSense v1.0");
    Serial.println("----------------------");
    
    settingsBegin();
    weightBegin();
    webBegin();
}

void loop()
{
    weightLoop();
    webLoop();

    delay(20);
}