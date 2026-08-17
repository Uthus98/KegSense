#include "keg.h"
#include "config.h"

Keg::Keg()
{
    _name = "Fat";

    _weight = 0.0f;
    _beerWeight = 0.0f;
    _liters = 0.0f;
    _percent = 0.0f;

    _emptyWeight = DEFAULT_KEG_EMPTY;
    _volume = DEFAULT_KEG_VOLUME;
    _calibration = DEFAULT_CALIBRATION;
}

void Keg::updateWeight(float rawWeight)
{
    _weight = rawWeight;

    _beerWeight = _weight - _emptyWeight;

    if (_beerWeight < 0)
        _beerWeight = 0;

    _liters = _beerWeight / BEER_DENSITY;

    if (_volume > 0)
        _percent = (_liters / _volume) * 100.0f;
    else
        _percent = 0;

    if (_percent < 0)
        _percent = 0;

    if (_percent > 100)
        _percent = 100;
}

void Keg::setName(const String& value)
{
    _name = value;
}

void Keg::setEmptyWeight(float value)
{
    _emptyWeight = value;
}

void Keg::setVolume(float value)
{
    _volume = value;
}

void Keg::setCalibration(float value)
{
    _calibration = value;
}

const String& Keg::getName() const
{
    return _name;
}

float Keg::getWeight() const
{
    return _weight;
}

float Keg::getBeerWeight() const
{
    return _beerWeight;
}

float Keg::getLiters() const
{
    return _liters;
}

float Keg::getPercent() const
{
    return _percent;
}

float Keg::getEmptyWeight() const
{
    return _emptyWeight;
}

float Keg::getVolume() const
{
    return _volume;
}

float Keg::getCalibration() const
{
    return _calibration;
}