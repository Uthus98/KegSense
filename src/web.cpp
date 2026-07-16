#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include "config.h"
#include "weight.h"
#include "settings.h"
#include "web.h"
#include "html.h"

AsyncWebServer server(80);

void handleRoot(AsyncWebServerRequest *request);
void handleApi(AsyncWebServerRequest *request);
void handleSettings(AsyncWebServerRequest *request);
void handleSave(AsyncWebServerRequest *request);
void handleTare(AsyncWebServerRequest *request);

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
    Serial.print("IP-adresse: ");
    Serial.println(WiFi.localIP());

    server.on("/", HTTP_GET, handleRoot);
    server.on("/api", HTTP_GET, handleApi);
    server.on("/settings", HTTP_GET, handleSettings);
    server.on("/save", HTTP_GET, handleSave);
    server.on("/tare", HTTP_GET, handleTare);

    server.begin();

    Serial.println("Webserver startet.");
}

void webLoop()
{
    // AsyncWebServer trenger ingenting her
}

void handleRoot(AsyncWebServerRequest *request)
{
    request->send_P(200, "text/html", MAIN_page);
}

void handleApi(AsyncWebServerRequest *request)
{
    JsonDocument doc;

    doc["device"] = DEVICE_NAME;
    doc["version"] = VERSION;

    doc["name"] = getKegName();

    doc["weight"] = getWeight();
    doc["beerWeight"] = getBeerWeight();
    doc["liter"] = getBeerLiters();
    doc["percent"] = getBeerPercent();

    doc["emptyWeight"] = getKegEmpty();
    doc["kegVolume"] = getKegVolume();

    doc["wifiRSSI"] = WiFi.RSSI();
    doc["uptime"] = millis() / 1000;

    String json;
    serializeJsonPretty(doc, json);

    request->send(200, "application/json", json);
}

void handleSettings(AsyncWebServerRequest *request)
{
    String html;

    html += "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";

    html += "<style>";
    html += "body{background:#181818;color:white;font-family:Arial;text-align:center;}";
    html += "input{font-size:22px;width:220px;padding:8px;margin:8px;}";
    html += "button{font-size:22px;padding:10px 25px;margin:15px;}";
    html += "a{color:#00d26a;font-size:22px;text-decoration:none;}";
    html += "</style>";

    html += "</head><body>";

    html += "<h1>⚙️ KegSense</h1>";

    html += "<form action='/save'>";

    html += "<p>Fatnavn</p>";
    html += "<input type='text' name='name' value='";
    html += getKegName();
    html += "'>";

    html += "<p>Tomvekt (kg)</p>";
    html += "<input type='number' step='0.01' name='empty' value='";
    html += String(getKegEmpty(),2);
    html += "'>";

    html += "<p>Fatvolum (L)</p>";
    html += "<input type='number' step='0.1' name='volume' value='";
    html += String(getKegVolume(),1);
    html += "'>";

    html += "<p>Kalibreringsfaktor</p>";
    html += "<input type='number' step='0.01' name='cal' value='";
    html += String(getCalFactor(),2);
    html += "'>";

    html += "<br><br>";

    html += "<button type='submit'>💾 Lagre</button>";

    html += "</form>";

    html += "<br>";

    html += "<form action='/tare'>";
    html += "<button type='submit'>⚖️ Sett nåværende vekt som tomt fat</button>";
    html += "</form>";

    html += "<br><br>";

    html += "<a href='/'>⬅ Tilbake</a>";

    html += "</body></html>";

    request->send(200, "text/html", html);
}

void handleSave(AsyncWebServerRequest *request)
{
    if(request->hasParam("name"))
        setKegName(request->getParam("name")->value());

    if(request->hasParam("empty"))
        setKegEmpty(request->getParam("empty")->value().toFloat());

    if(request->hasParam("volume"))
        setKegVolume(request->getParam("volume")->value().toFloat());

    if(request->hasParam("cal"))
        setCalFactor(request->getParam("cal")->value().toFloat());

    request->redirect("/settings");
}

void handleTare(AsyncWebServerRequest *request)
{
    setKegEmpty(getWeight());

    request->redirect("/settings");
}