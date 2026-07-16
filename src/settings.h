#pragma once

#include <Arduino.h>

void settingsBegin();

String getKegName();
void setKegName(String name);

float getKegEmpty();
void setKegEmpty(float value);

float getKegVolume();
void setKegVolume(float value);

float getCalFactor();
void setCalFactor(float value);