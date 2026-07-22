#pragma once

#include <Arduino.h>

constexpr char DEVICE_NAME[] = "KegSense";
constexpr char VERSION[] = "2.0.0-alpha";

//=========================
// Antall fat
//=========================

constexpr size_t MAX_KEGS = 2;

//=========================
// Konfigurasjon av vekter
//=========================

struct ScaleConfig
{
    bool enabled;
    uint8_t doutPin;
    uint8_t sckPin;
};

constexpr ScaleConfig SCALE_CONFIGS[MAX_KEGS] =
{
    // Aktiv, DOUT, SCK
    { true,  32, 33 },   // Fat 1
    { false, 25, 26 }    // Fat 2 (ikke tilkoblet enda)
};

//=========================
// Standardverdier
//=========================

constexpr float DEFAULT_CALIBRATION = 23786.25f;
constexpr float DEFAULT_KEG_EMPTY   = 4.90f;
constexpr float DEFAULT_KEG_VOLUME  = 20.0f;

constexpr float BEER_DENSITY = 0.997f;

//=========================
// WiFi
//=========================

constexpr char WIFI_SSID[] = "Marstadvegen 36";
constexpr char WIFI_PASSWORD[] = "juli2022";

//=========================
// OTA-oppdatering
//=========================

// Endre disse før systemet tas i permanent bruk.
constexpr char OTA_USERNAME[] = "admin";
constexpr char OTA_PASSWORD[] = "kegsense";
