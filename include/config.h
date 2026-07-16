#pragma once

#define DEVICE_NAME "KegSense"
#define VERSION     "1.1.0"

//=========================
// HX711
//=========================

constexpr uint8_t HX711_DOUT = 32;
constexpr uint8_t HX711_SCK  = 33;

//=========================
// Kalibrering
//=========================

constexpr float CALIBRATION_FACTOR = 23786.25f;

//=========================
// Fat
//=========================

// Tomvekt Corneliusfat
constexpr float KEG_EMPTY = 4.90f;

// Liter når fullt
constexpr float KEG_VOLUME = 20.0f;

// Tetthet øl
constexpr float BEER_DENSITY = 0.997f;

//=========================
// WiFi
//=========================

constexpr char WIFI_SSID[] = "Marstadvegen 36";
constexpr char WIFI_PASSWORD[] = "juli2022";