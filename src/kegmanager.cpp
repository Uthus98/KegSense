#include "kegmanager.h"

#define BEER_DENSITY 0.998

Keg kegs[MAX_KEGS];

Keg::Keg()
{
    name = "Fat";

    weight = 0.0;
    beerWeight = 0.0;
    liters = 0.0;
    percent = 0.0;

    emptyWeight = 4.90;
    volume = 20.0;
    calibration = 23786.25;
}

void Keg::updateWeight(float rawWeight)
{
    weight = rawWeight;

    beerWeight = weight - emptyWeight;

    if (beerWeight < 0)
        beerWeight = 0;

    liters = beerWeight / BEER_DENSITY;

    percent = (liters / volume) * 100.0;

    if (percent < 0)
        percent = 0;

    if (percent > 100)
        percent = 100;
}

void Keg::setName(const String& value)
{
    name = value;
}

void Keg::setEmptyWeight(float value)
{
    emptyWeight = value;
}

void Keg::setVolume(float value)
{
    volume = value;
}

void Keg::setCalibration(float value)
{
    calibration = value;
}

String Keg::getName() const
{
    return name;
}

float Keg::getEmptyWeight() const
{
    return emptyWeight;
}

float Keg::getVolume() const
{
    return volume;
}

float Keg::getCalibration() const
{
    return calibration;
}

float Keg::getWeight() const
{
    return weight;
}

float Keg::getBeerWeight() const
{
    return beerWeight;
}

float Keg::getLiters() const
{
    return liters;
}

float Keg::getPercent() const
{
    return percent;
}

void kegManagerBegin()
{
    kegs[0].setName("Fat 1");
    kegs[1].setName("Fat 2");
}