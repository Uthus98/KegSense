#include <Arduino.h>
#include "settings.h"
#include "weight.h"
#include "web.h"
#include "kegmanager.h"
#include "history.h"
#include "temperature.h"
#include "remote.h"
#include "wifi_setup.h"

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
    wifiSetupBegin();
    webBegin();
    historyBegin();
    temperatureBegin();
    remoteBegin();
}

void loop()
{
    weightLoop();
    historyLoop();
    temperatureLoop();
    remoteLoop();
    wifiSetupLoop();
    webLoop();

    delay(20);
}
