#include <Arduino.h>
#include <Preferences.h>

#include "settings.h"
#include "kegmanager.h"
#include "config.h"

Preferences prefs;

void loadKegSettings(int index)
{
    if (index < 0 || index >= static_cast<int>(MAX_KEGS))
        return;

    String prefix = "keg" + String(index) + "_";

    String defaultName = "Fat " + String(index + 1);

    kegs[index].setName(
        prefs.getString(
            (prefix + "name").c_str(),
            defaultName.c_str()
        )
    );

    kegs[index].setEmptyWeight(
        prefs.getFloat(
            (prefix + "empty").c_str(),
            DEFAULT_KEG_EMPTY
        )
    );

    kegs[index].setVolume(
        prefs.getFloat(
            (prefix + "volume").c_str(),
            DEFAULT_KEG_VOLUME
        )
    );

    kegs[index].setCalibration(
        prefs.getFloat(
            (prefix + "cal").c_str(),
            DEFAULT_CALIBRATION
        )
    );
}

void saveKegSettings(int index)
{
    if (index < 0 || index >= static_cast<int>(MAX_KEGS))
        return;

    String prefix = "keg" + String(index) + "_";

    prefs.putString(
        (prefix + "name").c_str(),
        kegs[index].getName()
    );

    prefs.putFloat(
        (prefix + "empty").c_str(),
        kegs[index].getEmptyWeight()
    );

    prefs.putFloat(
        (prefix + "volume").c_str(),
        kegs[index].getVolume()
    );

    prefs.putFloat(
        (prefix + "cal").c_str(),
        kegs[index].getCalibration()
    );
}

void settingsBegin()
{
    prefs.begin("kegsense", false);

    for (size_t i = 0; i < MAX_KEGS; i++)
    {
        loadKegSettings(static_cast<int>(i));
    }
}