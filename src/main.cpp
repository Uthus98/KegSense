#include <Arduino.h>
#include "settings.h"
#include "weight.h"
#include "web.h"
#include "kegmanager.h"
#include "history.h"

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("----------------------");
    Serial.println(" KegSense v2.0");
    Serial.println("----------------------");

    kegManagerBegin();
    settingsBegin();
    weightBegin();
    webBegin();
    historyBegin();
}

void loop()
{
    weightLoop();
    historyLoop();
    webLoop();

    delay(20);
}
