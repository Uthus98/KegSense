#include <Arduino.h>

#include "config.h"
#include "weight.h"
#include "kegmanager.h"
#include "scale.h"
#include "settings.h"

// ============================================================
// Vekter
//
// Hver Scale opprettes direkte fra SCALE_CONFIGS i config.h.
// enabled + DOUT + SCK ligger dermed kun definert ett sted.
// ============================================================

Scale scales[MAX_KEGS] =
{
    Scale(SCALE_CONFIGS[0]),
    Scale(SCALE_CONFIGS[1])
};


// ============================================================
// Oppstart
// ============================================================

void weightBegin()
{
    Serial.println();
    Serial.println("==========================");
    Serial.println(" Starter vekter");
    Serial.println("==========================");

    for (size_t i = 0; i < MAX_KEGS; i++)
    {
        Serial.print("Fat ");
        Serial.print(i + 1);
        Serial.print(" - ");

        // Vekten er deaktivert i config.h
        if (!SCALE_CONFIGS[i].enabled)
        {
            Serial.println("DEAKTIVERT");
            continue;
        }

        // Aktiver Scale-objektet iht. config
        scales[i].setEnabled(true);

        // Start HX711 med kalibreringsfaktoren som tilhører fatet
        if (scales[i].begin(kegs[i].getCalibration()))
        {
            Serial.print("OK");

            Serial.print("  DOUT=");
            Serial.print(SCALE_CONFIGS[i].doutPin);

            Serial.print("  SCK=");
            Serial.println(SCALE_CONFIGS[i].sckPin);
        }
        else
        {
            Serial.println("OFFLINE");
        }
    }

    Serial.println("==========================");
    Serial.println();
}


// ============================================================
// Hovedloop for vektene
// ============================================================

void weightLoop()
{
    // --------------------------------------------------------
    // Oppdater alle aktive vekter
    // --------------------------------------------------------

    for (size_t i = 0; i < MAX_KEGS; i++)
    {
        if (!SCALE_CONFIGS[i].enabled)
            continue;

        if (!scales[i].isOnline())
            continue;

        scales[i].update();

        // Oppdater Keg kun når HX711 faktisk har nye data
        if (scales[i].hasNewData())
        {
            kegs[i].updateWeight(scales[i].getWeight());
        }

        if (scales[i].getCalibrationState() == Scale::CalibrationState::Success)
        {
            const float calibration = scales[i].getCalibrationResult();

            if (kegs[i].getCalibration() != calibration)
            {
                kegs[i].setCalibration(calibration);
                saveKegSettings(static_cast<int>(i));
            }
        }
    }


    // --------------------------------------------------------
    // Debug-utskrift én gang per sekund
    // --------------------------------------------------------

    static unsigned long lastPrint = 0;

    if (millis() - lastPrint < 1000)
        return;

    lastPrint = millis();

    Serial.println("========== KEGS ==========");

    for (size_t i = 0; i < MAX_KEGS; i++)
    {
        Serial.print("Fat ");
        Serial.print(i + 1);

        Serial.print(" (");
        Serial.print(kegs[i].getName());
        Serial.print(")");

        // Deaktivert i config.h
        if (!SCALE_CONFIGS[i].enabled)
        {
            Serial.println("  DEAKTIVERT");
            continue;
        }

        // Aktivert, men HX711 kunne ikke startes
        if (!scales[i].isOnline())
        {
            Serial.println("  OFFLINE");
            continue;
        }

        Serial.print("  ");
        Serial.print(kegs[i].getWeight(), 2);
        Serial.print(" kg");

        Serial.print("  ");

        Serial.print(kegs[i].getLiters(), 2);
        Serial.print(" L");

        Serial.print("  ");

        Serial.print(kegs[i].getPercent(), 1);
        Serial.println(" %");
    }

    Serial.println();
}


// ============================================================
// Kompatibilitetsfunksjoner for Fat 1
//
// Disse beholdes foreløpig for eventuell eldre kode som fortsatt
// bruker getWeight(), getBeerLiters() osv.
// De kan fjernes senere når hele prosjektet bruker kegs[] direkte.
// ============================================================

float getWeight()
{
    return kegs[0].getWeight();
}

float getBeerWeight()
{
    return kegs[0].getBeerWeight();
}

float getBeerLiters()
{
    return kegs[0].getLiters();
}

float getBeerPercent()
{
    return kegs[0].getPercent();
}
bool isScaleEnabled(size_t index)
{
    if (index >= MAX_KEGS)
        return false;

    return SCALE_CONFIGS[index].enabled;
}

bool isScaleOnline(size_t index)
{
    if (index >= MAX_KEGS)
        return false;

    return scales[index].isOnline();
}

bool startScaleCalibrationTare(size_t index)
{
    return index < MAX_KEGS && scales[index].startCalibrationTare();
}

bool startScaleCalibration(size_t index, float knownMass)
{
    return index < MAX_KEGS && scales[index].startCalibration(knownMass);
}

const char* getScaleCalibrationState(size_t index)
{
    if (index >= MAX_KEGS)
        return "error";

    switch (scales[index].getCalibrationState())
    {
        case Scale::CalibrationState::Idle: return "idle";
        case Scale::CalibrationState::Taring: return "taring";
        case Scale::CalibrationState::ReadyForMass: return "ready";
        case Scale::CalibrationState::Measuring: return "measuring";
        case Scale::CalibrationState::Success: return "success";
        case Scale::CalibrationState::Error: return "error";
    }

    return "error";
}

float getScaleCalibrationResult(size_t index)
{
    if (index >= MAX_KEGS)
        return 0.0f;

    return scales[index].getCalibrationResult();
}

void clearScaleCalibrationState(size_t index)
{
    if (index < MAX_KEGS)
        scales[index].clearCalibrationState();
}

bool setScaleCalibration(size_t index, float calibration)
{
    if (index >= MAX_KEGS || calibration == 0.0f || !scales[index].isOnline())
        return false;

    scales[index].setCalibration(calibration);
    return true;
}
