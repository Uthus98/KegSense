#pragma once

#include <Arduino.h>

void historyBegin();
void historyLoop();
void historyResetKeg(size_t index);

float getConsumptionToday(size_t index);
bool isHistoryTimeReady();

struct DailyConsumptionRecord
{
    uint32_t date;
    float liters;
};

size_t getDailyHistoryCount(size_t index);
bool getDailyHistoryRecord(size_t index, size_t position, DailyConsumptionRecord& record);
uint32_t getHistoryTodayDate();
