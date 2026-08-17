#pragma once

#include <Arduino.h>
#include <HX711_ADC.h>

#include "config.h"

class Scale
{
public:
    enum class CalibrationState : uint8_t
    {
        Idle,
        Taring,
        ReadyForMass,
        Measuring,
        Success,
        Error
    };

    Scale(uint8_t doutPin, uint8_t sckPin);
    Scale(const ScaleConfig& config);

    // Oppstart
    bool begin(float calibration, long tareOffset, bool hasTareOffset);

    // Oppdater måling
    void update();

    // Tare / kalibrering
    void tare();
    void setCalibration(float calibration);
    bool startCalibrationTare();
    bool startCalibration(float knownMass);
    CalibrationState getCalibrationState() const;
    float getCalibrationResult() const;
    long getTareOffset();
    void clearCalibrationState();

    // Aktiv / deaktivert
    void setEnabled(bool enabled);
    bool isEnabled() const;

    // Status
    bool isOnline() const;
    bool hasNewData() const;

    // Målt vekt
    float getWeight() const;

private:
    HX711_ADC _cell;

    bool _enabled = true;
    bool _online = false;
    bool _newData = false;

    float _filteredWeight = 0.0f;
    bool _firstSample = true;

    CalibrationState _calibrationState = CalibrationState::Idle;
    float _knownMass = 0.0f;
    float _calibrationResult = 0.0f;
};
