#pragma once

#include <Arduino.h>

#define MAX_KEGS 2

class Keg
{
public:
    Keg();

    // Oppdateres fra HX711
    void updateWeight(float rawWeight);

    // Innstillinger
    void setName(const String& value);
    void setEmptyWeight(float value);
    void setVolume(float value);
    void setCalibration(float value);

    // Hent innstillinger
    String getName() const;
    float getEmptyWeight() const;
    float getVolume() const;
    float getCalibration() const;

    // Beregnede verdier
    float getWeight() const;
    float getBeerWeight() const;
    float getLiters() const;
    float getPercent() const;

private:
    String name;

    float weight;
    float beerWeight;
    float liters;
    float percent;

    float emptyWeight;
    float volume;
    float calibration;
};

extern Keg kegs[MAX_KEGS];

void kegManagerBegin();