#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>

#include "config.h"
#include "web.h"
#include "html.h"
#include "icons.h"
#include "kegmanager.h"
#include "settings.h"
#include "weight.h"

AsyncWebServer server(80);

static void handleRoot(AsyncWebServerRequest *request);
static void handleApi(AsyncWebServerRequest *request);
static void handleSettings(AsyncWebServerRequest *request);
static void handleSave(AsyncWebServerRequest *request);
static void handleCalibrationStatus(AsyncWebServerRequest *request);
static void handleCalibrationTare(AsyncWebServerRequest *request);
static void handleCalibrationApply(AsyncWebServerRequest *request);
static void handleCalibrationClear(AsyncWebServerRequest *request);

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
    server.on("/apple-touch-icon.png", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(200, "image/png", APPLE_TOUCH_ICON_180, APPLE_TOUCH_ICON_180_LEN);
    });
    server.on("/apple-touch-icon-152.png", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(200, "image/png", APPLE_TOUCH_ICON_152, APPLE_TOUCH_ICON_152_LEN);
    });
    server.on("/apple-touch-icon-120.png", HTTP_GET, [](AsyncWebServerRequest *request)
    {
        request->send(200, "image/png", APPLE_TOUCH_ICON_120, APPLE_TOUCH_ICON_120_LEN);
    });
    server.on("/api", HTTP_GET, handleApi);
    server.on("/settings", HTTP_GET, handleSettings);
    server.on("/save", HTTP_GET, handleSave);
    // Ikke legg status under /api. Enkelte ESPAsyncWebServer-versjoner
    // lar den eksisterende /api-ruten fange opp /api/calibration.
    server.on("/calibration/status", HTTP_GET, handleCalibrationStatus);
    server.on("/api/calibration/tare", HTTP_POST, handleCalibrationTare);
    server.on("/api/calibration/apply", HTTP_POST, handleCalibrationApply);
    server.on("/api/calibration/clear", HTTP_POST, handleCalibrationClear);

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

        if (enabled)
        {
            html += "<div class='calibration' data-index='";
            html += String(i);
            html += "'>";
            html += "<button type='button' onclick='startTare(";
            html += String(i);
            html += ")'>1. Nullstill tom plattform</button>";
            html += "<label>Kjent testvekt (kg)</label>";
            html += "<input type='number' min='0.01' step='0.001' id='known-";
            html += String(i);
            html += "' placeholder='For eksempel 2.677'>";
            html += "<button type='button' onclick='applyCalibration(";
            html += String(i);
            html += ")'>2. Kalibrer med testvekten</button>";
            html += "<div class='status' id='cal-status-";
            html += String(i);
            html += "'>Kalibrering ikke startet</div>";
            html += "</div>";
        }

        html += "</div>";
    }

    html += "<button type='submit'>💾 Lagre innstillinger</button>";

    html += "</form>";

    html += R"rawliteral(
<script>
async function post(url, values) {
  const body = new URLSearchParams(values || {});
  const response = await fetch(url, {method:'POST', body});
  const data = await response.json();
  if (!response.ok) throw new Error(data.message || 'Ukjent feil');
  return data;
}

async function startTare(index) {
  try {
    await post('/api/calibration/tare', {index});
    document.getElementById('cal-status-' + index).textContent =
      'Nullstiller. Hold plattformen helt tom...';
  } catch (error) {
    alert(error.message);
  }
}

async function applyCalibration(index) {
  const mass = document.getElementById('known-' + index).value;
  try {
    await post('/api/calibration/apply', {index, mass});
    document.getElementById('cal-status-' + index).textContent =
      'Måler testvekten...';
  } catch (error) {
    alert(error.message);
  }
}

async function refreshCalibration() {
  try {
    const response = await fetch(
      '/calibration/status?_=' + Date.now(),
      {cache: 'no-store'}
    );

    if (!response.ok)
      throw new Error('HTTP ' + response.status);

    const data = await response.json();

    if (!data || !Array.isArray(data.scales))
      throw new Error('ugyldig svar fra ESP32');

    data.scales.forEach(scale => {
      const el = document.getElementById('cal-status-' + scale.index);
      if (!el) return;
      const messages = {
        idle: 'Kalibrering ikke startet',
        taring: 'Nullstiller. Hold plattformen tom...',
        ready: 'Klar: legg på kjent vekt, skriv vekten og trykk steg 2.',
        measuring: 'Måler testvekten...',
        success: 'Ferdig. Ny faktor er lagret: ' + scale.result.toFixed(2),
        error: 'Kalibrering feilet. Prøv på nytt.'
      };
      el.textContent = messages[scale.state] || scale.state;
    });
  } catch (error) {
    document.querySelectorAll('[id^="cal-status-"]').forEach(el => {
      el.textContent = 'Kunne ikke hente kalibreringsstatus: ' + error.message;
    });
  }
}

setInterval(refreshCalibration, 700);
refreshCalibration();
</script>
)rawliteral";

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
            const float calibration = request->getParam(calKey)->value().toFloat();
            if (calibration != 0.0f)
            {
                kegs[i].setCalibration(calibration);
                setScaleCalibration(i, calibration);
            }
        }

        saveKegSettings(i);
    }

    request->redirect("/settings");
}

static bool readScaleIndex(AsyncWebServerRequest *request, size_t &index)
{
    if (!request->hasParam("index", true))
        return false;

    const int parsed = request->getParam("index", true)->value().toInt();
    if (parsed < 0 || parsed >= static_cast<int>(MAX_KEGS))
        return false;

    index = static_cast<size_t>(parsed);
    return true;
}

static void sendCalibrationError(AsyncWebServerRequest *request, int code, const char *message)
{
    JsonDocument doc;
    doc["message"] = message;
    String json;
    serializeJson(doc, json);
    request->send(code, "application/json", json);
}

static void handleCalibrationStatus(AsyncWebServerRequest *request)
{
    JsonDocument doc;
    JsonArray scales = doc["scales"].to<JsonArray>();

    for (size_t i = 0; i < MAX_KEGS; i++)
    {
        JsonObject scale = scales.add<JsonObject>();
        scale["index"] = i;
        scale["enabled"] = isScaleEnabled(i);
        scale["online"] = isScaleOnline(i);
        scale["state"] = getScaleCalibrationState(i);
        scale["result"] = getScaleCalibrationResult(i);
    }

    String json;
    serializeJson(doc, json);

    AsyncWebServerResponse *response =
        request->beginResponse(200, "application/json", json);

    response->addHeader("Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "0");

    request->send(response);
}

static void handleCalibrationTare(AsyncWebServerRequest *request)
{
    size_t index;
    if (!readScaleIndex(request, index))
        return sendCalibrationError(request, 400, "Ugyldig fatnummer");

    clearScaleCalibrationState(index);
    if (!startScaleCalibrationTare(index))
        return sendCalibrationError(request, 409, "Vekten er deaktivert eller offline");

    request->send(202, "application/json", "{\"message\":\"Tare startet\"}");
}

static void handleCalibrationApply(AsyncWebServerRequest *request)
{
    size_t index;
    if (!readScaleIndex(request, index) || !request->hasParam("mass", true))
        return sendCalibrationError(request, 400, "Mangler fatnummer eller kjent vekt");

    const float mass = request->getParam("mass", true)->value().toFloat();
    if (mass <= 0.0f)
        return sendCalibrationError(request, 400, "Kjent vekt må være større enn null");

    if (!startScaleCalibration(index, mass))
        return sendCalibrationError(request, 409, "Fullfør nullstilling før kalibrering");

    request->send(202, "application/json", "{\"message\":\"Kalibrering startet\"}");
}

static void handleCalibrationClear(AsyncWebServerRequest *request)
{
    size_t index;
    if (!readScaleIndex(request, index))
        return sendCalibrationError(request, 400, "Ugyldig fatnummer");

    clearScaleCalibrationState(index);
    request->send(200, "application/json", "{\"message\":\"Status nullstilt\"}");
}
