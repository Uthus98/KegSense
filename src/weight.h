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