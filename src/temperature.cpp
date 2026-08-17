#include <Arduino.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <math.h>

#include "config.h"
#include "temperature.h"
#include "settings.h"

namespace
{
    constexpr uint32_t READ_INTERVAL_MS = 5000;
    constexpr uint32_t CONVERSION_TIME_MS = 800;

    OneWire oneWire(TEMP_SENSOR_PIN);
    DallasTemperature sensors(&oneWire);

    float temperatureC = NAN;
    bool temperatureValid = false;
    bool conversionPending = false;
    uint32_t conversionStartedAt = 0;
    uint32_t lastReadingAt = 0;

    void requestReading()
    {
        sensors.requestTemperatures();
        conversionPending = true;
        conversionStartedAt = millis();
    }
}

void temperatureBegin()
{
    if (!isTemperatureFeatureEnabled())
    {
        Serial.println("Temperatur: deaktivert i enhetsoppsett");
        return;
    }

    sensors.begin();
    sensors.setResolution(12);
    sensors.setWaitForConversion(false);

    Serial.print("DS18B20 sensorer funnet: ");
    Serial.println(sensors.getDeviceCount());

    requestReading();
}

void temperatureLoop()
{
    if (!isTemperatureFeatureEnabled())
        return;

    const uint32_t now = millis();

    if (conversionPending && now - conversionStartedAt >= CONVERSION_TIME_MS)
    {
        const float reading = sensors.getTempCByIndex(0);

        temperatureValid =
            isfinite(reading) &&
            reading != DEVICE_DISCONNECTED_C &&
            reading >= -55.0f &&
            reading <= 125.0f;

        if (temperatureValid)
            temperatureC = reading;

        conversionPending = false;
        lastReadingAt = now;
    }

    if (!conversionPending && now - lastReadingAt >= READ_INTERVAL_MS)
        requestReading();
}

bool isTemperatureValid()
{
    return isTemperatureFeatureEnabled() && temperatureValid;
}

float getTemperatureC()
{
    return temperatureC;
}
