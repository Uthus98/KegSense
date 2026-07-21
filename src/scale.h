#pragma once

#include <Arduino.h>
#include <HX711_ADC.h>

#include "config.h"

class Scale
{
public:
    Scale(uint8_t doutPin, uint8_t sckPin);
    Scale(const ScaleConfig& config);

    // Oppstart
    bool begin(float calibration);

    // Oppdater måling
    void update();

    // Tare / kalibrering
    void tare();
    void setCalibration(float calibration);

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
};