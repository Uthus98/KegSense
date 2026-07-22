#pragma once

#include <Arduino.h>

void historyBegin();
void historyLoop();
void historyResetKeg(size_t index);

float getConsumptionToday(size_t index);
bool isHistoryTimeReady();

