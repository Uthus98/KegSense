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

bool Scale::begin(float calibration, long tareOffset, bool hasTareOffset)
{
    if (!_enabled)
    {
        _online = false;
        return false;
    }

    _cell.begin();

    // Nullpunkt skal aldri beregnes automatisk ved oppstart. Et fat kan
    // stå på vekten under strømbrudd eller OTA-oppdatering.
    _cell.start(2000, false);

    if (_cell.getTareTimeoutFlag())
    {
        _online = false;
        return false;
    }

    _cell.setCalFactor(calibration);

    if (hasTareOffset)
        _cell.setTareOffset(tareOffset);

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

    if (_calibrationState == CalibrationState::Taring && _cell.getTareStatus())
    {
        _filteredWeight = 0.0f;
        _firstSample = true;
        _calibrationState = CalibrationState::ReadyForMass;
    }

    if (_calibrationState == CalibrationState::Measuring)
    {
        if (_knownMass <= 0.0f || !_cell.refreshDataSet())
        {
            _calibrationState = CalibrationState::Error;
            return;
        }

        _calibrationResult = _cell.getNewCalibration(_knownMass);

        if (!isfinite(_calibrationResult) || _calibrationResult == 0.0f)
        {
            _calibrationState = CalibrationState::Error;
            return;
        }

        _firstSample = true;
        _calibrationState = CalibrationState::Success;
    }

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

bool Scale::startCalibrationTare()
{
    if (!_enabled || !_online)
        return false;

    _calibrationState = CalibrationState::Taring;
    _calibrationResult = 0.0f;
    _knownMass = 0.0f;
    _cell.tareNoDelay();
    return true;
}

bool Scale::startCalibration(float knownMass)
{
    if (!_enabled || !_online ||
        _calibrationState != CalibrationState::ReadyForMass ||
        knownMass <= 0.0f)
    {
        return false;
    }

    _knownMass = knownMass;
    _calibrationState = CalibrationState::Measuring;
    return true;
}

Scale::CalibrationState Scale::getCalibrationState() const
{
    return _calibrationState;
}

float Scale::getCalibrationResult() const
{
    return _calibrationResult;
}

long Scale::getTareOffset()
{
    return _cell.getTareOffset();
}

void Scale::clearCalibrationState()
{
    if (_calibrationState == CalibrationState::Taring ||
        _calibrationState == CalibrationState::Measuring)
    {
        return;
    }

    _calibrationState = CalibrationState::Idle;
}
