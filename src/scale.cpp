#include "scale.h"

Scale::Scale(uint8_t doutPin, uint8_t sckPin)
    : _cell(doutPin, sckPin)
{
}

Scale::Scale(const ScaleConfig& config)
    : _cell(config.doutPin, config.sckPin)
{
    _enabled = config.enabled;
}

bool Scale::begin(float calibration)
{
    if (!_enabled)
    {
        _online = false;
        return false;
    }

    _cell.begin();

    bool tare = true;
    _cell.start(2000, tare);

    if (_cell.getTareTimeoutFlag())
    {
        _online = false;
        return false;
    }

    _cell.setCalFactor(calibration);

    _online = true;
    _firstSample = true;
    _newData = false;

    return true;
}

void Scale::update()
{
    _newData = false;

    if (!_enabled || !_online)
        return;

    if (!_cell.update())
        return;

    float raw = _cell.getData();

    _newData = true;

    if (_firstSample)
    {
        _filteredWeight = raw;
        _firstSample = false;
    }
    else
    {
        constexpr float alpha = 0.10f;

        _filteredWeight =
            (_filteredWeight * (1.0f - alpha)) +
            (raw * alpha);
    }
}

void Scale::tare()
{
    if (!_enabled || !_online)
        return;

    _cell.tareNoDelay();
}

void Scale::setCalibration(float calibration)
{
    if (!_enabled || !_online)
        return;

    _cell.setCalFactor(calibration);
}

void Scale::setEnabled(bool enabled)
{
    _enabled = enabled;

    if (!_enabled)
    {
        _online = false;
        _newData = false;
    }
}

bool Scale::isEnabled() const
{
    return _enabled;
}

bool Scale::isOnline() const
{
    return _online;
}

bool Scale::hasNewData() const
{
    return _newData;
}

float Scale::getWeight() const
{
    return _filteredWeight;
}