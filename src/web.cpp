#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include "config.h"
#include "web.h"
#include "html.h"
#include "kegmanager.h"
#include "settings.h"

AsyncWebServer server(80);

static void handleRoot(AsyncWebServerRequest *request);
static void handleApi(AsyncWebServerRequest *request);
static void handleSettings(AsyncWebServerRequest *request);
static void handleSave(AsyncWebServerRequest *request);

void webBegin()
{
    Serial.println();
    Serial.println("Starter WiFi...");

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }

    Serial.println();
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, handleRoot);
    server.on("/api", HTTP_GET, handleApi);
    server.on("/settings", HTTP_GET, handleSettings);
    server.on("/save", HTTP_GET, handleSave);

    server.begin();

    Serial.println("Webserver startet.");
}

void webLoop()
{
}

static void handleRoot(AsyncWebServerRequest *request)
{
    request->send(200, "text/html", MAIN_page);
}

static void handleApi(AsyncWebServerRequest *request)
{
    JsonDocument doc;

    doc["device"] = DEVICE_NAME;
    doc["version"] = VERSION;

    doc["name"] = kegs[0].getName();

    doc["weight"] = kegs[0].getWeight();
    doc["beerWeight"] = kegs[0].getBeerWeight();
    doc["liter"] = kegs[0].getLiters();
    doc["percent"] = kegs[0].getPercent();

    doc["emptyWeight"] = kegs[0].getEmptyWeight();
    doc["kegVolume"] = kegs[0].getVolume();

    doc["wifiRSSI"] = WiFi.RSSI();
    doc["uptime"] = millis() / 1000;

    String json;
    serializeJson(doc, json);

    request->send(200, "application/json", json);
}

static void handleSettings(AsyncWebServerRequest *request)
{
    String html;

    html += "<html><body>";
    html += "<h1>KegSense</h1>";

    html += "<form action='/save'>";

    html += "Navn:<br>";
    html += "<input name='name' value='" + kegs[0].getName() + "'><br><br>";

    html += "Tomvekt:<br>";
    html += "<input name='empty' value='" + String(kegs[0].getEmptyWeight(),2) + "'><br><br>";

    html += "Volum:<br>";
    html += "<input name='volume' value='" + String(kegs[0].getVolume(),1) + "'><br><br>";

    html += "Kalibrering:<br>";
    html += "<input name='cal' value='" + String(kegs[0].getCalibration(),2) + "'><br><br>";

    html += "<input type='submit' value='Lagre'>";

    html += "</form>";

    html += "<br><a href='/'>Tilbake</a>";

    html += "</body></html>";

    request->send(200, "text/html", html);
}

static void handleSave(AsyncWebServerRequest *request)
{
    if(request->hasParam("name"))
        kegs[0].setName(request->getParam("name")->value());

    if(request->hasParam("empty"))
        kegs[0].setEmptyWeight(request->getParam("empty")->value().toFloat());

    if(request->hasParam("volume"))
        kegs[0].setVolume(request->getParam("volume")->value().toFloat());

    if(request->hasParam("cal"))
        kegs[0].setCalibration(request->getParam("cal")->value().toFloat());

    saveKegSettings(0);

    request->redirect("/settings");
}