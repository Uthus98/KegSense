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
    // Ignore small changes caused by HX711 drift and measurement noise.
    // A real serving is still recorded at its full measured amount once
    // the accumulated decrease exceeds this threshold.
    constexpr float CONSUMPTION_NOISE_TOLERANCE_L = 0.20f;
    constexpr size_t HISTORY_DAYS = 62;
    constexpr uint16_t ARCHIVE_VERSION = 1;

    struct HistoryState
    {
        bool initialized = false;
        bool suspended = false;
        float dayMaximum = 0.0f;
        float lastLevel = 0.0f;
        float carriedConsumption = 0.0f;
        String date;
    };

    struct HistoryArchive
    {
        uint16_t version = ARCHIVE_VERSION;
        uint16_t count = 0;
        DailyConsumptionRecord records[HISTORY_DAYS] = {};
    };

    Preferences historyPrefs;
    HistoryState states[MAX_KEGS];
    HistoryArchive archives[MAX_KEGS];
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

    uint32_t dateNumber(const String& date)
    {
        if (date.length() != 10)
            return 0;

        String compact = date;
        compact.replace("-", "");
        return static_cast<uint32_t>(compact.toInt());
    }

    String archiveKey(size_t index)
    {
        return "archive" + String(index);
    }

    void saveArchive(size_t index)
    {
        historyPrefs.putBytes(
            archiveKey(index).c_str(),
            &archives[index],
            sizeof(HistoryArchive));
    }

    void loadArchive(size_t index)
    {
        HistoryArchive loaded;
        const size_t bytes = historyPrefs.getBytes(
            archiveKey(index).c_str(),
            &loaded,
            sizeof(HistoryArchive));

        if (bytes == sizeof(HistoryArchive) &&
            loaded.version == ARCHIVE_VERSION &&
            loaded.count <= HISTORY_DAYS)
        {
            archives[index] = loaded;
        }
    }

    void archiveDay(size_t index, const String& date, float liters)
    {
        const uint32_t day = dateNumber(date);
        if (day == 0)
            return;

        if (!isfinite(liters) || liters < 0.0f)
            liters = 0.0f;

        HistoryArchive &archive = archives[index];

        if (archive.count > 0 && archive.records[archive.count - 1].date == day)
        {
            archive.records[archive.count - 1].liters = liters;
            saveArchive(index);
            return;
        }

        if (archive.count >= HISTORY_DAYS)
        {
            memmove(
                &archive.records[0],
                &archive.records[1],
                sizeof(DailyConsumptionRecord) * (HISTORY_DAYS - 1));
            archive.count = HISTORY_DAYS - 1;
        }

        archive.records[archive.count++] = {day, liters};
        saveArchive(index);
    }

    float consumptionFor(const HistoryState& state, float currentLiters)
    {
        const float measured = state.dayMaximum - currentLiters;
        const float consumption =
            state.carriedConsumption + (measured > 0.0f ? measured : 0.0f);

        return consumption >= CONSUMPTION_NOISE_TOLERANCE_L
            ? consumption
            : 0.0f;
    }

    void saveState(size_t index)
    {
        historyPrefs.putString(keyFor(index, "date").c_str(), states[index].date);
        historyPrefs.putFloat(keyFor(index, "max").c_str(), states[index].dayMaximum);
        historyPrefs.putFloat(keyFor(index, "base").c_str(), states[index].carriedConsumption);
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
            state.carriedConsumption = historyPrefs.getFloat(keyFor(index, "base").c_str(), 0.0f);
            if (state.dayMaximum < liters)
                state.dayMaximum = liters;
        }
        else
        {
            if (!savedDate.isEmpty())
            {
                const float savedMaximum = historyPrefs.getFloat(keyFor(index, "max").c_str(), liters);
                const float savedBase = historyPrefs.getFloat(keyFor(index, "base").c_str(), 0.0f);
                const float previousConsumption = savedBase + max(0.0f, savedMaximum - liters);
                archiveDay(index, savedDate, previousConsumption);
            }

            state.dayMaximum = liters;
            state.carriedConsumption = 0.0f;
            saveState(index);
        }

        state.initialized = true;
    }
}

void historyBegin()
{
    historyPrefs.begin("history", false);

    for (size_t i = 0; i < MAX_KEGS; i++)
        loadArchive(i);

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

        if (!state.initialized)
        {
            initializeState(i, liters, date);
            continue;
        }

        if (state.date != date)
        {
            archiveDay(i, state.date, consumptionFor(state, liters));

            state.date = date;
            state.dayMaximum = liters;
            state.lastLevel = liters;
            state.carriedConsumption = 0.0f;
            state.suspended = false;
            saveState(i);
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
    state.carriedConsumption = 0.0f;

    if (state.initialized)
        saveState(index);
}

float getConsumptionToday(size_t index)
{
    if (index >= MAX_KEGS || !states[index].initialized || states[index].suspended)
        return 0.0f;

    return consumptionFor(states[index], kegs[index].getLiters());
}

bool isHistoryTimeReady()
{
    return timeReady;
}

size_t getDailyHistoryCount(size_t index)
{
    return index < MAX_KEGS ? archives[index].count : 0;
}

bool getDailyHistoryRecord(size_t index, size_t position, DailyConsumptionRecord& record)
{
    if (index >= MAX_KEGS || position >= archives[index].count)
        return false;

    record = archives[index].records[position];
    return true;
}

uint32_t getHistoryTodayDate()
{
    return dateNumber(currentDate());
}
