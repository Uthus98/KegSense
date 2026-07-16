#include <Preferences.h>
#include "settings.h"

Preferences prefs;

// Standardverdier
String kegName = "Mitt fat";
float kegEmpty = 4.90;
float kegVolume = 20.0;
float calFactor = 23786.25;

void settingsBegin()
{
    prefs.begin("kegsense", false);

    kegName   = prefs.getString("kegName", "Mitt fat");
    kegEmpty  = prefs.getFloat("empty", 4.90);
    kegVolume = prefs.getFloat("volume", 20.0);
    calFactor = prefs.getFloat("cal", 23786.25);
}

String getKegName()
{
    return kegName;
}

float getKegEmpty()
{
    return kegEmpty;
}

float getKegVolume()
{
    return kegVolume;
}

float getCalFactor()
{
    return calFactor;
}

void setKegName(String name)
{
    kegName = name;
    prefs.putString("kegName", name);
}

void setKegEmpty(float value)
{
    kegEmpty = value;
    prefs.putFloat("empty", value);
}

void setKegVolume(float value)
{
    kegVolume = value;
    prefs.putFloat("volume", value);
}

void setCalFactor(float value)
{
    calFactor = value;
    prefs.putFloat("cal", value);
}