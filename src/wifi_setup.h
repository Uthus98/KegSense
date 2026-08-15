#pragma once

#include <ESPAsyncWebServer.h>

void wifiSetupBegin();
void wifiSetupLoop();
void wifiSetupRegisterRoutes(AsyncWebServer& server);
bool wifiSetupIsPortalActive();

