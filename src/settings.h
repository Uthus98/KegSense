#pragma once

#include <Arduino.h>

void settingsBegin();

void loadKegSettings(int index);
void saveKegSettings(int index);

bool hasScaleTareOffset(int index);
long loadScaleTareOffset(int index);
void saveScaleTareOffset(int index, long offset);

size_t getActiveKegCount();
bool isTemperatureFeatureEnabled();
bool isHistoryFeatureEnabled();
bool isRemoteFeatureEnabled();
void saveDeviceFeatures(size_t kegCount, bool temperatureEnabled,
                        bool historyEnabled, bool remoteEnabled);

const String& getRemoteDeviceId();
const String& getRemoteUrl();
const String& getRemoteToken();
bool isRemoteConfigured();
void saveRemoteConfiguration(const String& deviceId, const String& url,
                             const String& token);

const String& getOtaUsername();
const String& getOtaPassword();
bool isOtaConfigured();
void saveOtaCredentials(const String& username, const String& password);
