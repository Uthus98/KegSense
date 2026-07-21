#include <Arduino.h>
#include <HX711_ADC.h>

#include "config.h"
#include "settings.h"
#include "weight.h"
#include "kegmanager.h"

HX711_ADC LoadCell(HX711_DOUT, HX711_SCK);

static float filteredWeight = 0.0f;

void weightBegin()
{
    Serial.println("Starter HX711...");

    LoadCell.begin();

    bool tare = true;
    LoadCell.start(2000, tare);

    if (LoadCell.getTareTimeoutFlag())
    {
        Serial.println("HX711 Timeout!");
        while (1);
    }

    LoadCell.setCalFactor(kegs[0].getCalibration());

    Serial.println("HX711 klar.");
}

void weightLoop()
{
    if (!LoadCell.update())
        return;

    float raw = LoadCell.getData();

    static bool firstSample = true;

    if (firstSample)
    {
        filteredWeight = raw;
        firstSample = false;
    }
    else
    {
        constexpr float alpha = 0.10f;
        filteredWeight = filteredWeight * (1.0f - alpha) + raw * alpha;
    }

    kegs[0].updateWeight(filteredWeight);

    static unsigned long lastPrint = 0;

    if (millis() - lastPrint >= 1000)
    {
        lastPrint = millis();

        Serial.print("RAW: ");
        Serial.print(raw, 2);

        Serial.print("  Filter: ");
        Serial.print(filteredWeight, 2);

        Serial.print("  Liter: ");
        Serial.print(kegs[0].getLiters(), 2);

        Serial.print("  %: ");
        Serial.println(kegs[0].getPercent(), 1);
    }
}

float getWeight()
{
    return kegs[0].getWeight();
}

float getBeerWeight()
{
    return kegs[0].getBeerWeight();
}

float getBeerLiters()
{
    return kegs[0].getLiters();
}

float getBeerPercent()
{
    return kegs[0].getPercent();
}