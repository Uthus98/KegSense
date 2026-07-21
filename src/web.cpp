#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include "config.h"
#include "web.h"
#include "html.h"
#include "kegmanager.h"
#include "settings.h"
#include "weight.h"

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
    // AsyncWebServer krever ingen behandling her.
}


// ============================================================
// Hovedside
// ============================================================

static void handleRoot(AsyncWebServerRequest *request)
{
    request->send(200, "text/html", MAIN_page);
}


// ============================================================
// API
// ============================================================

static void handleApi(AsyncWebServerRequest *request)
{
    JsonDocument doc;

    doc["device"] = DEVICE_NAME;
    doc["version"] = VERSION;
    doc["wifiRSSI"] = WiFi.RSSI();
    doc["uptime"] = millis() / 1000;

    JsonArray kegArray = doc["kegs"].to<JsonArray>();

    for (size_t i = 0; i < MAX_KEGS; i++)
    {
        JsonObject keg = kegArray.add<JsonObject>();

        keg["index"] = i;
        keg["name"] = kegs[i].getName();

        keg["enabled"] = isScaleEnabled(i);
        keg["online"] = isScaleOnline(i);

        keg["weight"] = kegs[i].getWeight();
        keg["beerWeight"] = kegs[i].getBeerWeight();
        keg["liter"] = kegs[i].getLiters();
        keg["percent"] = kegs[i].getPercent();

        keg["emptyWeight"] = kegs[i].getEmptyWeight();
        keg["volume"] = kegs[i].getVolume();
        keg["calibration"] = kegs[i].getCalibration();
    }

    String json;
    serializeJson(doc, json);

    request->send(200, "application/json", json);
}


// ============================================================
// Innstillinger
// ============================================================

static void handleSettings(AsyncWebServerRequest *request)
{
    String html;

    html.reserve(5000);

    html += "<!DOCTYPE html>";
    html += "<html>";
    html += "<head>";

    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";

    html += "<title>KegSense - Innstillinger</title>";

    html += "<style>";

    html += "body{";
    html += "margin:0;";
    html += "background:#181818;";
    html += "color:white;";
    html += "font-family:Arial,sans-serif;";
    html += "}";

    html += ".container{";
    html += "max-width:600px;";
    html += "margin:auto;";
    html += "padding:20px;";
    html += "}";

    html += "h1{text-align:center;}";

    html += ".keg{";
    html += "background:#252525;";
    html += "padding:20px;";
    html += "border-radius:16px;";
    html += "margin-bottom:20px;";
    html += "}";

    html += ".offline{opacity:.55;}";

    html += "label{";
    html += "display:block;";
    html += "margin-top:15px;";
    html += "color:#bbb;";
    html += "}";

    html += "input{";
    html += "width:100%;";
    html += "box-sizing:border-box;";
    html += "font-size:18px;";
    html += "padding:10px;";
    html += "margin-top:5px;";
    html += "border-radius:8px;";
    html += "border:1px solid #555;";
    html += "background:#333;";
    html += "color:white;";
    html += "}";

    html += "button{";
    html += "width:100%;";
    html += "font-size:20px;";
    html += "padding:14px;";
    html += "margin-top:20px;";
    html += "border:0;";
    html += "border-radius:10px;";
    html += "background:#00d26a;";
    html += "color:white;";
    html += "font-weight:bold;";
    html += "}";

    html += "a{";
    html += "display:block;";
    html += "text-align:center;";
    html += "color:#00d26a;";
    html += "font-size:20px;";
    html += "text-decoration:none;";
    html += "margin:25px;";
    html += "}";

    html += ".status{";
    html += "font-size:14px;";
    html += "color:#aaa;";
    html += "margin-bottom:10px;";
    html += "}";

    html += "</style>";

    html += "</head>";

    html += "<body>";

    html += "<div class='container'>";

    html += "<h1>⚙️ KegSense</h1>";

    html += "<form action='/save' method='get'>";

    for (size_t i = 0; i < MAX_KEGS; i++)
    {
        bool enabled = isScaleEnabled(i);

        html += "<div class='keg";

        if (!enabled)
            html += " offline";

        html += "'>";

        html += "<h2>🍺 Fat ";
        html += String(i + 1);
        html += "</h2>";

        html += "<div class='status'>";

        if (!enabled)
        {
            html += "⚫ Deaktivert i config.h";
        }
        else if (isScaleOnline(i))
        {
            html += "🟢 Vekt tilkoblet";
        }
        else
        {
            html += "🔴 Vekt offline";
        }

        html += "</div>";

        String prefix = "keg" + String(i) + "_";

        html += "<label>Fatnavn</label>";
        html += "<input type='text' name='";
        html += prefix;
        html += "name' value='";
        html += kegs[i].getName();
        html += "'>";

        html += "<label>Tomvekt (kg)</label>";
        html += "<input type='number' step='0.01' name='";
        html += prefix;
        html += "empty' value='";
        html += String(kegs[i].getEmptyWeight(), 2);
        html += "'>";

        html += "<label>Fatvolum (L)</label>";
        html += "<input type='number' step='0.1' name='";
        html += prefix;
        html += "volume' value='";
        html += String(kegs[i].getVolume(), 1);
        html += "'>";

        html += "<label>Kalibreringsfaktor</label>";
        html += "<input type='number' step='0.01' name='";
        html += prefix;
        html += "cal' value='";
        html += String(kegs[i].getCalibration(), 2);
        html += "'>";

        html += "</div>";
    }

    html += "<button type='submit'>💾 Lagre innstillinger</button>";

    html += "</form>";

    html += "<a href='/'>⬅ Tilbake til dashboard</a>";

    html += "</div>";

    html += "</body>";
    html += "</html>";

    request->send(200, "text/html", html);
}


// ============================================================
// Lagre innstillinger
// ============================================================

static void handleSave(AsyncWebServerRequest *request)
{
    for (size_t i = 0; i < MAX_KEGS; i++)
    {
        String prefix = "keg" + String(i) + "_";

        String nameKey = prefix + "name";
        String emptyKey = prefix + "empty";
        String volumeKey = prefix + "volume";
        String calKey = prefix + "cal";

        if (request->hasParam(nameKey))
        {
            kegs[i].setName(
                request->getParam(nameKey)->value()
            );
        }

        if (request->hasParam(emptyKey))
        {
            kegs[i].setEmptyWeight(
                request->getParam(emptyKey)->value().toFloat()
            );
        }

        if (request->hasParam(volumeKey))
        {
            kegs[i].setVolume(
                request->getParam(volumeKey)->value().toFloat()
            );
        }

        if (request->hasParam(calKey))
        {
            kegs[i].setCalibration(
                request->getParam(calKey)->value().toFloat()
            );
        }

        saveKegSettings(i);
    }

    request->redirect("/settings");
}