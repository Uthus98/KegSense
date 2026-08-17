#pragma once

#include <Arduino.h>

void weightBegin();
void weightLoop();

// Kompatibilitet for Fat 1
float getWeight();
float getBeerWeight();
float getBeerLiters();
float getBeerPercent();

// Status for vektene
bool isScaleEnabled(size_t index);
bool isScaleOnline(size_t index);

bool startScaleCalibrationTare(size_t index);
bool startScaleCalibration(size_t index, float knownMass);
const char* getScaleCalibrationState(size_t index);
float getScaleCalibrationResult(size_t index);
void clearScaleCalibrationState(size_t index);
bool setScaleCalibration(size_t index, float calibration);
