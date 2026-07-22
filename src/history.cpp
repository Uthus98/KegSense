#include <Arduino.h>
#include <Preferences.h>
#include <time.h>

#include "config.h"
#include "history.h"
#include "kegmanager.h"
#include "weight.h"

namespace
{
    constexpr uint32_t SAMPLE_INTERVAL_MS = 10000;
    constexpr float REMOVAL_DROP_L = 2.0f;
    constexpr float RETURN_TOLERANCE_L = 0.30f;

    struct HistoryState
    {
        bool initialized = false;
        bool suspended = false;
        float dayMaximum = 0.0f;
        float lastLevel = 0.0f;
        String date;
    };

    Preferences historyPrefs;
    HistoryState states[MAX_KEGS];
    uint32_t lastSample = 0;
    bool timeReady = false;

    String currentDate()
    {
        struct tm now;
        if (!getLocalTime(&now, 10))
            return String();

        char buffer[11];
        strftime(buffer, sizeof(buffer), "%Y-%m-%d", &now);
        return String(buffer);
    }

    String keyFor(size_t index, const char *suffix)
    {
        return "h" + String(index) + "_" + suffix;
    }

    void saveState(size_t index)
    {
        historyPrefs.putString(keyFor(index, "date").c_str(), states[index].date);
        historyPrefs.putFloat(keyFor(index, "max").c_str(), states[index].dayMaximum);
    }

    void initializeState(size_t index, float liters, const String& date)
    {
        HistoryState &state = states[index];
        state.date = date;
        state.lastLevel = liters;
        state.suspended = false;

        const String savedDate = historyPrefs.getString(keyFor(index, "date").c_str(), "");
        if (savedDate == date)
        {
            state.dayMaximum = historyPrefs.getFloat(keyFor(index, "max").c_str(), liters);
            if (state.dayMaximum < liters)
                state.dayMaximum = liters;
        }
        else
        {
            state.dayMaximum = liters;
            saveState(index);
        }

        state.initialized = true;
    }
}

void historyBegin()
{
    historyPrefs.begin("history", false);

    // Norsk tid, inkludert automatisk sommer-/vintertid.
    configTzTime("CET-1CEST,M3.5.0,M10.5.0/3", "pool.ntp.org", "time.nist.gov");
}

void historyLoop()
{
    if (millis() - lastSample < SAMPLE_INTERVAL_MS)
        return;

    lastSample = millis();

    const String date = currentDate();
    if (date.isEmpty())
        return;

    timeReady = true;

    for (size_t i = 0; i < MAX_KEGS; i++)
    {
        if (!isScaleEnabled(i) || !isScaleOnline(i))
            continue;

        HistoryState &state = states[i];
        const float liters = kegs[i].getLiters();
        const float weight = kegs[i].getWeight();

        // En fjernet plattform/fat skal ikke registreres som forbruk.
        if (weight < kegs[i].getEmptyWeight() - 0.20f)
        {
            state.suspended = true;
            continue;
        }

        if (!state.initialized || state.date != date)
        {
            initializeState(i, liters, date);
            continue;
        }

        if (state.suspended)
        {
            if (liters >= state.lastLevel - RETURN_TOLERANCE_L)
            {
                state.suspended = false;
                state.lastLevel = liters;
            }
            continue;
        }

        if (state.lastLevel - liters > REMOVAL_DROP_L)
        {
            state.suspended = true;
            continue;
        }

        state.lastLevel = liters;

        if (liters > state.dayMaximum + 0.05f)
        {
            state.dayMaximum = liters;
            saveState(i);
        }
    }
}

void historyResetKeg(size_t index)
{
    if (index >= MAX_KEGS)
        return;

    HistoryState &state = states[index];
    const String date = currentDate();

    state.initialized = !date.isEmpty();
    state.suspended = false;
    state.date = date;
    state.dayMaximum = kegs[index].getLiters();
    state.lastLevel = state.dayMaximum;

    if (state.initialized)
        saveState(index);
}

float getConsumptionToday(size_t index)
{
    if (index >= MAX_KEGS || !states[index].initialized || states[index].suspended)
        return 0.0f;

    const float consumption = states[index].dayMaximum - kegs[index].getLiters();
    return consumption > 0.0f ? consumption : 0.0f;
}

bool isHistoryTimeReady()
{
    return timeReady;
}

