#include <Arduino.h>
#include <HX711_ADC.h>

#include "settings.h"
#include "config.h"
#include "weight.h"

HX711_ADC LoadCell(HX711_DOUT, HX711_SCK);

float weight = 0.0;
float beerWeight = 0.0;
float beerLiters = 0.0;
float beerPercent = 0.0;

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

    LoadCell.setCalFactor(getCalFactor());

    Serial.println("HX711 klar.");
}

void weightLoop()
{
   if (LoadCell.update())
{
    float raw = LoadCell.getData();

    static bool firstSample = true;

    if (firstSample)
    {
        weight = raw;
        firstSample = false;
    }
    else
    {
        const float alpha = 0.10f;
        weight = weight * (1.0f - alpha) + raw * alpha;
        beerWeight = weight - getKegEmpty();

if (beerWeight < 0)
    beerWeight = 0;

beerLiters = beerWeight / BEER_DENSITY;

beerPercent = (beerLiters / getKegVolume()) * 100.0f;

if (beerPercent < 0)
    beerPercent = 0;

if (beerPercent > 100)
    beerPercent = 100;
    }

        static unsigned long lastPrint = 0;

        if (millis() - lastPrint > 1000)
        {
            lastPrint = millis();

            Serial.print("RAW: ");
            Serial.print(raw, 3);

            Serial.print("   Filtrert: ");
            Serial.print(weight, 3);

            Serial.print(" kg   Liter: ");
            Serial.print(getBeerLiters(), 2);

            Serial.print("   %: ");
            Serial.println(getBeerPercent(), 1);
        }
    }
}

float getWeight()
{
    return weight;
}

float getBeerLiters()
{
    return beerLiters;
}

float getBeerPercent()
{
    return beerPercent;
}

float getBeerWeight()
{
    return beerWeight;
}