#include <Preferences.h>

#include "settings.h"
#include "kegmanager.h"
#include "config.h"

Preferences prefs;

void loadKegSettings(int index)
{
    String prefix = "keg" + String(index) + "_";

    kegs[index].setName(
        prefs.getString(
            (prefix + "name").c_str(),
            ("Fat " + String(index + 1)).c_str()));

    kegs[index].setEmptyWeight(
        prefs.getFloat(
            (prefix + "empty").c_str(),
            DEFAULT_KEG_EMPTY));

    kegs[index].setVolume(
        prefs.getFloat(
            (prefix + "volume").c_str(),
            DEFAULT_KEG_VOLUME));

    kegs[index].setCalibration(
        prefs.getFloat(
            (prefix + "cal").c_str(),
            DEFAULT_CALIBRATION));
}

void saveKegSettings(int index)
{
    String prefix = "keg" + String(index) + "_";

    prefs.putString(
        (prefix + "name").c_str(),
        kegs[index].getName());

    prefs.putFloat(
        (prefix + "empty").c_str(),
        kegs[index].getEmptyWeight());

    prefs.putFloat(
        (prefix + "volume").c_str(),
        kegs[index].getVolume());

    prefs.putFloat(
        (prefix + "cal").c_str(),
        kegs[index].getCalibration());
}

void settingsBegin()
{
    prefs.begin("kegsense", false);

    for (size_t i = 0; i < MAX_KEGS; i++)
    {
        loadKegSettings(i);
    }
}