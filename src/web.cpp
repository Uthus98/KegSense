#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <Update.h>

#include "config.h"
#include "web.h"
#include "html.h"
#include "icons.h"
#include "kegmanager.h"
#include "settings.h"
#include "weight.h"
#include "history.h"
#include "temperature.h"
#include "wifi_setup.h"

AsyncWebServer server(80);

static void handleRoot(AsyncWebServerRequest *request);
static void handleApi(AsyncWebServerRequest *request);
static void handleSettings(AsyncWebServerRequest *request);
static void handleSave(AsyncWebServerRequest *request);
static String escapeHtml(const String& value);
static void handleNewKeg(AsyncWebServerRequest *request);
static void handleNewKegSave(AsyncWebServerRequest *request);
static void handleUpdatePage(AsyncWebServerRequest *request);
static void handleUpdateComplete(AsyncWebServerRequest *request);
static void handleUpdateUpload(AsyncWebServerRequest *request, String filename,
                               size_t index, uint8_t *data, size_t len, bool final);
static void handleRestart(AsyncWebServerRequest *request);
static void handleHistoryPage(AsyncWebServerRequest *request);
static void handleHistoryData(AsyncWebServerRequest *request);
static void handleCalibrationStatus(AsyncWebServerRequest *request);
static void handleCalibrationTare(AsyncWebServerRequest *request);
static void handleCalibrationApply(AsyncWebServerRequest *request);
static void handleCalibrationClear(AsyncWebServerRequest *request);

static bool restartAfterUpdate = false;
static uint32_t restartRequestedAt = 0;
static bool updateFileAccepted = false;

void webBegin()
{
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
    server.on("/new-keg", HTTP_GET, handleNewKeg);
    server.on("/new-keg/save", HTTP_POST, handleNewKegSave);
    server.on("/update", HTTP_GET, handleUpdatePage);
    server.on("/update", HTTP_POST, handleUpdateComplete, handleUpdateUpload);
    server.on("/restart", HTTP_POST, handleRestart);
    server.on("/daily-history", HTTP_GET, handleHistoryPage);
    server.on("/history-data", HTTP_GET, handleHistoryData);
    // Ikke legg status under /api. Enkelte ESPAsyncWebServer-versjoner
    // lar den eksisterende /api-ruten fange opp /api/calibration.
    server.on("/calibration/status", HTTP_GET, handleCalibrationStatus);
    server.on("/api/calibration/tare", HTTP_POST, handleCalibrationTare);
    server.on("/api/calibration/apply", HTTP_POST, handleCalibrationApply);
    server.on("/api/calibration/clear", HTTP_POST, handleCalibrationClear);
    wifiSetupRegisterRoutes(server);

    server.begin();

    Serial.println("Webserver startet.");
}

void webLoop()
{
    if (restartAfterUpdate && millis() - restartRequestedAt >= 1200)
        ESP.restart();
}

static String escapeHtml(const String& value)
{
    String escaped;
    escaped.reserve(value.length() + 8);

    for (size_t i = 0; i < value.length(); i++)
    {
        switch (value[i])
        {
            case '&': escaped += "&amp;"; break;
            case '<': escaped += "&lt;"; break;
            case '>': escaped += "&gt;"; break;
            case '\"': escaped += "&quot;"; break;
            case '\'': escaped += "&#39;"; break;
            default: escaped += value[i]; break;
        }
    }

    return escaped;
}


// ============================================================
// Hovedside
// ============================================================

static void handleRoot(AsyncWebServerRequest *request)
{
    if (wifiSetupIsPortalActive())
        return request->redirect("/wifi");

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
    doc["timeReady"] = isHistoryTimeReady();
    doc["temperatureValid"] = isTemperatureValid();
    if (isTemperatureValid())
        doc["temperatureC"] = getTemperatureC();
    else
        doc["temperatureC"] = nullptr;

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
        keg["halfLiters"] = static_cast<int>(kegs[i].getLiters() / 0.5f);

        keg["emptyWeight"] = kegs[i].getEmptyWeight();
        keg["volume"] = kegs[i].getVolume();
        keg["calibration"] = kegs[i].getCalibration();
        keg["consumptionToday"] = getConsumptionToday(i);
    }

    String json;
    serializeJson(doc, json);

    request->send(200, "application/json", json);
}


// ============================================================
// Daglig forbrukshistorikk
// ============================================================

static void handleHistoryPage(AsyncWebServerRequest *request)
{
    static const char page[] PROGMEM = R"HISTORY(
<!doctype html><html><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>KegSense - Historikk</title><style>
*{box-sizing:border-box}body{margin:0;background:#181818;color:#fff;font-family:Arial,sans-serif}
.wrap{max-width:1050px;margin:auto;padding:22px}.toolbar{display:flex;gap:10px;margin-bottom:18px}
button,a.back{border:0;border-radius:10px;padding:12px 17px;font-size:17px;font-weight:bold;text-decoration:none;color:#fff;background:#414141}
button.active{background:#00b85c}.card{background:#252525;border-radius:18px;padding:20px;margin:18px 0}
.chart{height:220px;display:flex;align-items:flex-end;gap:8px;overflow-x:auto;padding:18px 4px 4px;border-bottom:1px solid #555}
.day{min-width:42px;text-align:center}.bar{width:30px;min-height:2px;margin:auto;background:#00d26a;border-radius:7px 7px 0 0}
.value{font-size:12px;margin-bottom:5px}.date{font-size:11px;color:#aaa;margin-top:6px;transform:rotate(-45deg);height:38px;white-space:nowrap}
.empty{color:#aaa;padding:25px 0}.summary{color:#bbb}.back{display:block!important;text-align:center;margin-top:22px}
</style></head><body><main class="wrap"><h1>Daglig forbruk</h1>
<div class="toolbar"><button id="d30" class="active" onclick="loadHistory(30)">30 dager</button>
<button id="d62" onclick="loadHistory(62)">62 dager</button></div>
<div id="content">Laster...</div><a class="back" href="/">Tilbake til dashboard</a></main><script>
function dateLabel(n){const s=String(n);return s.length===8?s.slice(6,8)+'.'+s.slice(4,6):s;}
function kegCard(keg){
 const card=document.createElement('section');card.className='card';
 const total=keg.records.reduce((sum,r)=>sum+r.liters,0);
 const maximum=Math.max(0.5,...keg.records.map(r=>r.liters));
 card.innerHTML=`<h2>${keg.name}</h2><div class="summary">Totalt i perioden: ${total.toFixed(2)} L</div>`;
 if(!keg.records.length){card.innerHTML+='<div class="empty">Ingen historikk ennå.</div>';return card;}
 const chart=document.createElement('div');chart.className='chart';
 keg.records.forEach(r=>{const day=document.createElement('div');day.className='day';
   const height=Math.max(2,(r.liters/maximum)*140);
   day.innerHTML=`<div class="value">${r.liters.toFixed(2)}</div><div class="bar" style="height:${height}px"></div><div class="date">${dateLabel(r.date)}</div>`;
   chart.appendChild(day);
 });
 card.appendChild(chart);setTimeout(()=>{chart.scrollLeft=chart.scrollWidth;},0);return card;
}
async function loadHistory(days){
 document.getElementById('d30').classList.toggle('active',days===30);
 document.getElementById('d62').classList.toggle('active',days===62);
 const content=document.getElementById('content');content.textContent='Laster...';
 try{const response=await fetch('/history-data?days='+days+'&_='+Date.now(),{cache:'no-store'});
   if(!response.ok)throw new Error('HTTP '+response.status);const data=await response.json();content.innerHTML='';
   data.kegs.forEach(k=>content.appendChild(kegCard(k)));
 }catch(error){content.textContent='Kunne ikke hente historikk: '+error.message;}
}
loadHistory(30);
</script></body></html>
)HISTORY";

    AsyncWebServerResponse *response = request->beginResponse(200, "text/html; charset=utf-8", page);
    response->addHeader("Cache-Control", "no-store");
    request->send(response);
}

static void handleHistoryData(AsyncWebServerRequest *request)
{
    int days = 30;
    if (request->hasParam("days"))
        days = constrain(request->getParam("days")->value().toInt(), 1, 62);

    JsonDocument doc;
    JsonArray kegArray = doc["kegs"].to<JsonArray>();

    for (size_t i = 0; i < MAX_KEGS; i++)
    {
        JsonObject keg = kegArray.add<JsonObject>();
        keg["index"] = i;
        keg["name"] = kegs[i].getName();
        JsonArray records = keg["records"].to<JsonArray>();

        const size_t count = getDailyHistoryCount(i);
        const size_t start = count > static_cast<size_t>(days - 1)
            ? count - static_cast<size_t>(days - 1)
            : 0;

        for (size_t position = start; position < count; position++)
        {
            DailyConsumptionRecord record;
            if (!getDailyHistoryRecord(i, position, record))
                continue;

            JsonObject item = records.add<JsonObject>();
            item["date"] = record.date;
            item["liters"] = record.liters;
        }

        const uint32_t today = getHistoryTodayDate();
        if (today != 0)
        {
            JsonObject item = records.add<JsonObject>();
            item["date"] = today;
            item["liters"] = getConsumptionToday(i);
        }
    }

    String json;
    serializeJson(doc, json);
    AsyncWebServerResponse *response = request->beginResponse(200, "application/json", json);
    response->addHeader("Cache-Control", "no-store");
    request->send(response);
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

async function restartDevice() {
  if (!confirm('Starte KegSense på nytt?')) return;

  try {
    const response = await fetch('/restart', {method:'POST'});
    if (!response.ok) throw new Error('HTTP ' + response.status);

    document.querySelectorAll('[id^="cal-status-"]').forEach(el => {
      el.textContent = 'KegSense starter på nytt...';
    });

    setTimeout(waitForDevice, 2500);
  } catch (error) {
    alert('Kunne ikke starte på nytt: ' + error.message);
  }
}

async function waitForDevice() {
  try {
    const response = await fetch('/?_=' + Date.now(), {cache:'no-store'});
    if (response.ok) {
      location.replace('/');
      return;
    }
  } catch (_) {}

  setTimeout(waitForDevice, 1000);
}

setInterval(refreshCalibration, 700);
refreshCalibration();
</script>
)rawliteral";

    html += "<a href='/wifi'>Endre WiFi</a>";
    html += "<button type='button' style='background:#d35454' onclick='restartDevice()'>Start KegSense på nytt</button>";
    html += "<a href='/update'>Oppdater firmware</a>";
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

// ============================================================
// OTA-oppdatering fra nettleser
// ============================================================

static void handleRestart(AsyncWebServerRequest *request)
{
    request->send(200, "text/plain; charset=utf-8", "KegSense starter på nytt");
    restartAfterUpdate = true;
    restartRequestedAt = millis();
}

static bool authenticateUpdate(AsyncWebServerRequest *request)
{
    if (request->authenticate(OTA_USERNAME, OTA_PASSWORD))
        return true;

    request->requestAuthentication();
    return false;
}

static void handleUpdatePage(AsyncWebServerRequest *request)
{
    if (!authenticateUpdate(request))
        return;

    static const char page[] PROGMEM = R"rawliteral(
<!doctype html><html><head><meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>KegSense - Oppdatering</title><style>
body{margin:0;background:#181818;color:#fff;font-family:Arial,sans-serif}
.box{max-width:560px;margin:auto;padding:24px}.card{background:#252525;border-radius:18px;padding:22px}
input{display:block;width:100%;box-sizing:border-box;margin:20px 0;padding:12px;background:#333;color:#fff;border:1px solid #555;border-radius:9px}
button,a{display:block;width:100%;box-sizing:border-box;padding:14px;border:0;border-radius:10px;text-align:center;font-size:18px;font-weight:bold;text-decoration:none}
button{background:#00d26a;color:#fff}a{margin-top:15px;background:#444;color:#fff}
progress{width:100%;height:24px;margin-top:20px}.note{color:#bbb;line-height:1.5}#status{margin-top:15px}
</style></head><body><main class="box"><h1>Oppdater KegSense</h1><div class="card">
<p class="note">Velg bare en firmwarefil med endelsen <strong>.bin</strong>. Ikke koble fra strømmen under oppdateringen.</p>
<input id="file" type="file" accept=".bin,application/octet-stream">
<button onclick="upload()">Last opp og installer</button>
<progress id="progress" value="0" max="100"></progress><div id="status"></div>
<a href="/settings">Avbryt</a></div></main><script>
function upload(){
 const file=document.getElementById('file').files[0], status=document.getElementById('status');
 if(!file){status.textContent='Velg en .bin-fil først.';return;}
 if(!file.name.toLowerCase().endsWith('.bin')){status.textContent='Filen må ende med .bin.';return;}
 if(!confirm('Installere '+file.name+'?'))return;
 const data=new FormData();data.append('firmware',file);
 const xhr=new XMLHttpRequest();xhr.open('POST','/update');
 xhr.upload.onprogress=e=>{if(e.lengthComputable)document.getElementById('progress').value=(e.loaded/e.total)*100;};
 xhr.onload=()=>{
   status.textContent=xhr.responseText;
   if(xhr.status===200)setTimeout(waitForRestart,2500);
 };
 xhr.onerror=()=>{status.textContent='Nettverksfeil under opplasting.';};
 status.textContent='Laster opp...';xhr.send(data);
}
async function waitForRestart(){
 const status=document.getElementById('status');
 status.textContent='Venter på at KegSense skal starte...';
 try{
   const response=await fetch('/?_='+Date.now(),{cache:'no-store'});
   if(response.ok){location.replace('/');return;}
 }catch(error){}
 setTimeout(waitForRestart,1000);
}
</script></body></html>
)rawliteral";

    AsyncWebServerResponse *response = request->beginResponse(200, "text/html; charset=utf-8", page);
    response->addHeader("Cache-Control", "no-store");
    request->send(response);
}

static void handleUpdateComplete(AsyncWebServerRequest *request)
{
    if (!authenticateUpdate(request))
        return;

    if (!updateFileAccepted || Update.hasError())
    {
        updateFileAccepted = false;
        return request->send(400, "text/plain; charset=utf-8",
                             "Oppdateringen feilet. ESP32 starter ikke på nytt.");
    }

    request->send(200, "text/plain; charset=utf-8",
                  "Oppdatering fullført. KegSense starter på nytt...");
    restartAfterUpdate = true;
    restartRequestedAt = millis();
    updateFileAccepted = false;
}

static void handleUpdateUpload(AsyncWebServerRequest *request, String filename,
                               size_t index, uint8_t *data, size_t len, bool final)
{
    if (!request->authenticate(OTA_USERNAME, OTA_PASSWORD))
        return;

    if (index == 0)
    {
        updateFileAccepted = filename.endsWith(".bin") || filename.endsWith(".BIN");

        if (!updateFileAccepted || !Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH))
        {
            updateFileAccepted = false;
            Update.printError(Serial);
            return;
        }

        Serial.print("OTA starter: ");
        Serial.println(filename);
    }

    if (updateFileAccepted && len > 0 && Update.write(data, len) != len)
    {
        updateFileAccepted = false;
        Update.printError(Serial);
        return;
    }

    if (final && updateFileAccepted)
    {
        if (!Update.end(true))
        {
            updateFileAccepted = false;
            Update.printError(Serial);
        }
        else
        {
            Serial.printf("OTA ferdig: %u bytes\n", static_cast<unsigned>(index + len));
        }
    }
}

// ============================================================
// Nytt fat
// ============================================================

static void handleNewKeg(AsyncWebServerRequest *request)
{
    if (!request->hasParam("index"))
        return request->send(400, "text/plain", "Mangler fatnummer");

    const int parsed = request->getParam("index")->value().toInt();
    if (parsed < 0 || parsed >= static_cast<int>(MAX_KEGS))
        return request->send(400, "text/plain", "Ugyldig fatnummer");

    const size_t index = static_cast<size_t>(parsed);
    if (!isScaleEnabled(index))
        return request->send(409, "text/plain", "Dette fatet er deaktivert");

    String html;
    html.reserve(4000);
    html += "<!doctype html><html><head><meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width,initial-scale=1'>";
    html += "<title>KegSense - Nytt fat</title><style>";
    html += "body{margin:0;background:#181818;color:#fff;font-family:Arial,sans-serif}";
    html += ".box{max-width:560px;margin:auto;padding:24px}";
    html += ".card{background:#252525;border-radius:18px;padding:22px}";
    html += "label{display:block;color:#bbb;margin-top:17px}";
    html += "input{width:100%;box-sizing:border-box;margin-top:6px;padding:12px;font-size:19px;border:1px solid #555;border-radius:9px;background:#333;color:#fff}";
    html += "button,.back{display:block;width:100%;box-sizing:border-box;margin-top:22px;padding:14px;border:0;border-radius:10px;text-align:center;font-size:19px;font-weight:bold;text-decoration:none}";
    html += "button{background:#00d26a;color:#fff}.back{background:#444;color:#fff}";
    html += ".note{color:#bbb;line-height:1.45}.measure{margin:18px 0;padding:13px;border-radius:10px;background:#303030}";
    html += "</style></head><body><main class='box'><h1>Nytt fat</h1><div class='card'>";
    html += "<p class='note'>Kalibreringen beholdes. Sett gjerne det fylte fatet p&aring; vekten f&oslash;r du lagrer.</p>";
    html += "<div class='measure'>N&aring;v&aelig;rende totalvekt: <strong>";
    html += String(kegs[index].getWeight(), 2);
    html += " kg</strong></div>";
    html += "<form action='/new-keg/save' method='post' onsubmit=\"return confirm('Starte nytt fat med disse innstillingene?')\">";
    html += "<input type='hidden' name='index' value='" + String(index) + "'>";
    html += "<label>Fatnavn / &oslash;ltype</label><input name='name' maxlength='32' required value='";
    html += escapeHtml(kegs[index].getName());
    html += "'>";
    html += "<label>Tomvekt for fatet (kg)</label><input type='number' name='empty' min='0' max='50' step='0.01' required value='";
    html += String(kegs[index].getEmptyWeight(), 2);
    html += "'>";
    html += "<label>Faktisk fylt volum / 100 % (L)</label><input type='number' name='volume' min='0.1' max='100' step='0.1' required value='";
    html += String(kegs[index].getVolume(), 1);
    html += "'>";
    html += "<button type='submit'>Bekreft nytt fat</button></form>";
    html += "<a class='back' href='/'>Avbryt</a></div></main></body></html>";

    request->send(200, "text/html; charset=utf-8", html);
}

static void handleNewKegSave(AsyncWebServerRequest *request)
{
    if (!request->hasParam("index", true) ||
        !request->hasParam("name", true) ||
        !request->hasParam("empty", true) ||
        !request->hasParam("volume", true))
    {
        return request->send(400, "text/plain", "Mangler opplysninger");
    }

    const int parsed = request->getParam("index", true)->value().toInt();
    if (parsed < 0 || parsed >= static_cast<int>(MAX_KEGS))
        return request->send(400, "text/plain", "Ugyldig fatnummer");

    const size_t index = static_cast<size_t>(parsed);
    if (!isScaleEnabled(index))
        return request->send(409, "text/plain", "Dette fatet er deaktivert");

    String name = request->getParam("name", true)->value();
    name.trim();
    const float emptyWeight = request->getParam("empty", true)->value().toFloat();
    const float volume = request->getParam("volume", true)->value().toFloat();

    if (name.isEmpty() || name.length() > 32 ||
        emptyWeight < 0.0f || emptyWeight > 50.0f ||
        volume < 0.1f || volume > 100.0f)
    {
        return request->send(400, "text/plain", "Ugyldige verdier");
    }

    // Kalibreringen endres ikke ved fatbytte.
    kegs[index].setName(name);
    kegs[index].setEmptyWeight(emptyWeight);
    kegs[index].setVolume(volume);
    kegs[index].updateWeight(kegs[index].getWeight());
    saveKegSettings(static_cast<int>(index));
    historyResetKeg(index);

    request->redirect("/");
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
