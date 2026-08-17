#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#include "config.h"
#include "remote.h"
#include "kegmanager.h"
#include "weight.h"
#include "history.h"
#include "temperature.h"
#include "settings.h"

namespace
{
    uint32_t lastAttempt = 0;
    bool firstUpload = true;

    bool configurationReady()
    {
        return isRemoteFeatureEnabled() && isRemoteConfigured();
    }

    String createPayload()
    {
        JsonDocument doc;
        doc["deviceId"] = getRemoteDeviceId();
        doc["device"] = DEVICE_NAME;
        doc["version"] = VERSION;
        doc["uptime"] = millis() / 1000;
        doc["temperatureValid"] = isTemperatureValid();

        if (isTemperatureValid())
            doc["temperatureC"] = getTemperatureC();
        else
            doc["temperatureC"] = nullptr;

        JsonArray kegArray = doc["kegs"].to<JsonArray>();
        for (size_t i = 0; i < getActiveKegCount(); i++)
        {
            JsonObject keg = kegArray.add<JsonObject>();
            keg["index"] = i;
            keg["name"] = kegs[i].getName();
            keg["enabled"] = isScaleEnabled(i);
            keg["online"] = isScaleOnline(i);
            keg["weight"] = kegs[i].getWeight();
            keg["liters"] = kegs[i].getLiters();
            keg["percent"] = kegs[i].getPercent();
            keg["consumptionToday"] = getConsumptionToday(i);
        }

        String payload;
        serializeJson(doc, payload);
        return payload;
    }

    void uploadTelemetry()
    {
        WiFiClientSecure client;
        // Cloudflare sertifikater roteres. HTTPS krypterer trafikken, mens
        // enhetsnøkkelen valideres av Worker. CA-validering kan legges til
        // senere uten å endre API-et.
        client.setInsecure();

        HTTPClient http;
        if (!http.begin(client, getRemoteUrl()))
        {
            Serial.println("KegSense Remote: ugyldig URL");
            return;
        }

        http.setTimeout(10000);
        http.addHeader("Content-Type", "application/json");
        http.addHeader("Authorization", "Bearer " + getRemoteToken());

        const int status = http.POST(createPayload());
        if (status >= 200 && status < 300)
            Serial.println("KegSense Remote: data sendt");
        else
        {
            Serial.print("KegSense Remote: HTTP-feil ");
            Serial.println(status);
        }

        http.end();
    }
}

void remoteBegin()
{
    if (!isRemoteFeatureEnabled())
    {
        Serial.println("KegSense Remote: deaktivert");
        return;
    }

    if (!configurationReady())
        Serial.println("KegSense Remote: mangler URL eller enhetsnøkkel");
}

void remoteLoop()
{
    if (!configurationReady() || WiFi.status() != WL_CONNECTED)
        return;

    const uint32_t now = millis();
    if (!firstUpload && now - lastAttempt < REMOTE_INTERVAL_MS)
        return;

    // Vent litt etter oppstart slik at vekter og temperatur rekker å stabilisere seg.
    if (firstUpload && now < 15000)
        return;

    firstUpload = false;
    lastAttempt = now;
    uploadTelemetry();
}
