#include <Arduino.h>
#include <Preferences.h>

#include "settings.h"
#include "kegmanager.h"
#include "config.h"

Preferences prefs;

namespace
{
    size_t activeKegCount = MAX_KEGS;
    bool temperatureFeatureEnabled = true;
    bool historyFeatureEnabled = true;
    bool remoteFeatureEnabled = true;
    String remoteDeviceId;
    String remoteUrl;
    String remoteToken;
    String otaUsername;
    String otaPassword;
}

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

    activeKegCount = constrain(
        static_cast<size_t>(prefs.getUChar("keg_count", MAX_KEGS)),
        static_cast<size_t>(1), MAX_KEGS);
    temperatureFeatureEnabled = prefs.getBool("feature_temp", true);
    historyFeatureEnabled = prefs.getBool("feature_history", true);
    remoteFeatureEnabled = prefs.getBool("feature_remote", true);
    remoteDeviceId = prefs.getString("remote_id", REMOTE_DEVICE_ID);
    remoteUrl = prefs.getString("remote_url", REMOTE_URL);
    remoteToken = prefs.getString("remote_token", REMOTE_DEVICE_TOKEN);
    otaUsername = prefs.getString("ota_user", "admin");
    otaPassword = prefs.getString("ota_pass", "");

    for (size_t i = 0; i < MAX_KEGS; i++)
    {
        loadKegSettings(static_cast<int>(i));
    }
}

size_t getActiveKegCount()
{
    return activeKegCount;
}

bool isTemperatureFeatureEnabled()
{
    return temperatureFeatureEnabled;
}

bool isHistoryFeatureEnabled()
{
    return historyFeatureEnabled;
}

bool isRemoteFeatureEnabled()
{
    return remoteFeatureEnabled;
}

void saveDeviceFeatures(size_t kegCount, bool temperatureEnabled,
                        bool historyEnabled, bool remoteEnabled)
{
    activeKegCount = constrain(kegCount, static_cast<size_t>(1), MAX_KEGS);
    temperatureFeatureEnabled = temperatureEnabled;
    historyFeatureEnabled = historyEnabled;
    remoteFeatureEnabled = remoteEnabled;

    prefs.putUChar("keg_count", static_cast<uint8_t>(activeKegCount));
    prefs.putBool("feature_temp", temperatureFeatureEnabled);
    prefs.putBool("feature_history", historyFeatureEnabled);
    prefs.putBool("feature_remote", remoteFeatureEnabled);
}

const String& getRemoteDeviceId()
{
    return remoteDeviceId;
}

const String& getRemoteUrl()
{
    return remoteUrl;
}

const String& getRemoteToken()
{
    return remoteToken;
}

bool isRemoteConfigured()
{
    return !remoteDeviceId.isEmpty() && remoteUrl.startsWith("https://") &&
           !remoteToken.isEmpty() && remoteToken != "BYTT-MEG" &&
           remoteUrl.indexOf("DIN-WORKER") < 0;
}

void saveRemoteConfiguration(const String& deviceId, const String& url,
                             const String& token)
{
    remoteDeviceId = deviceId;
    remoteUrl = url;
    if (!token.isEmpty())
        remoteToken = token;

    prefs.putString("remote_id", remoteDeviceId);
    prefs.putString("remote_url", remoteUrl);
    if (!token.isEmpty())
        prefs.putString("remote_token", remoteToken);
}

const String& getOtaUsername()
{
    return otaUsername;
}

const String& getOtaPassword()
{
    return otaPassword;
}

bool isOtaConfigured()
{
    return !otaUsername.isEmpty() && otaPassword.length() >= 8;
}

void saveOtaCredentials(const String& username, const String& password)
{
    otaUsername = username;
    if (!password.isEmpty())
        otaPassword = password;
    prefs.putString("ota_user", otaUsername);
    if (!password.isEmpty())
        prefs.putString("ota_pass", otaPassword);
}

bool hasScaleTareOffset(int index)
{
    if (index < 0 || index >= static_cast<int>(MAX_KEGS))
        return false;

    const String key = "keg" + String(index) + "_offset";
    return prefs.isKey(key.c_str());
}

long loadScaleTareOffset(int index)
{
    if (!hasScaleTareOffset(index))
        return 0;

    const String key = "keg" + String(index) + "_offset";
    return prefs.getLong(key.c_str(), 0);
}

void saveScaleTareOffset(int index, long offset)
{
    if (index < 0 || index >= static_cast<int>(MAX_KEGS))
        return;

    const String key = "keg" + String(index) + "_offset";
    prefs.putLong(key.c_str(), offset);
}
