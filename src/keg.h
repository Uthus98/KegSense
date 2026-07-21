#pragma once

#include <Arduino.h>

class Keg
{
public:

    Keg();

    void updateWeight(float rawWeight);

    void setName(const String& value);
    void setEmptyWeight(float value);
    void setVolume(float value);
    void setCalibration(float value);

    const String& getName() const;

    float getWeight() const;
    float getBeerWeight() const;
    float getLiters() const;
    float getPercent() const;

    float getEmptyWeight() const;
    float getVolume() const;
    float getCalibration() const;

private:

    String _name;

    float _weight;
    float _beerWeight;
    float _liters;
    float _percent;

    float _emptyWeight;
    float _volume;
    float _calibration;
};