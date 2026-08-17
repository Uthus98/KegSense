#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>

#include "wifi_setup.h"
#include "config.h"
#include "settings.h"

namespace
{
    constexpr char AP_NAME[] = "KegSense-Setup";
    constexpr uint32_t CONNECT_TIMEOUT_MS = 15000;

    DNSServer dnsServer;
    Preferences wifiPreferences;
    bool portalActive = false;
    bool restartPending = false;
    uint32_t restartAt = 0;

    void startMdns()
    {
        if (!MDNS.begin(DEVICE_HOSTNAME))
        {
            Serial.println("mDNS kunne ikke startes");
            return;
        }

        MDNS.addService("http", "tcp", 80);
        Serial.print("Lokal adresse: http://");
        Serial.print(DEVICE_HOSTNAME);
        Serial.println(".local");
    }

    String escapeHtml(const String& value)
    {
        String result;
        result.reserve(value.length() + 8);
        for (size_t i = 0; i < value.length(); i++)
        {
            switch (value[i])
            {
                case '&': result += "&amp;"; break;
                case '<': result += "&lt;"; break;
                case '>': result += "&gt;"; break;
                case '\"': result += "&quot;"; break;
                case '\'': result += "&#39;"; break;
                default: result += value[i]; break;
            }
        }
        return result;
    }

    void startPortal()
    {
        WiFi.disconnect(true);
        delay(100);
        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_NAME);
        dnsServer.start(53, "*", WiFi.softAPIP());
        portalActive = true;

        Serial.println();
        Serial.println("WiFi-oppsett startet");
        Serial.print("Nettverk: ");
        Serial.println(AP_NAME);
        Serial.print("Oppsettside: http://");
        Serial.println(WiFi.softAPIP());
    }

    String buildPortalPage(const String& message = String())
    {
        String page;
        page.reserve(7000);
        page += F("<!doctype html><html><head><meta charset='UTF-8'>");
        page += F("<meta name='viewport' content='width=device-width,initial-scale=1'>");
        page += F("<title>KegSense - WiFi</title><style>");
        page += F("*{box-sizing:border-box}body{margin:0;background:#181818;color:#fff;font-family:Arial,sans-serif}");
        page += F("main{max-width:600px;margin:auto;padding:24px}.card{background:#252525;border-radius:18px;padding:22px}");
        page += F("label{display:block;color:#bbb;margin-top:17px}select,input{width:100%;padding:13px;margin-top:6px;font-size:18px;border:1px solid #555;border-radius:9px;background:#333;color:#fff}");
        page += F("fieldset{border:1px solid #555;border-radius:12px;margin:22px 0 0;padding:5px 16px 17px}legend{padding:0 8px;font-weight:bold}.toggle{display:flex;align-items:center;gap:11px;color:#fff}.toggle input{width:auto;margin:0}.unavailable{opacity:.55}");
        page += F("button,a{display:block;width:100%;padding:14px;margin-top:20px;border:0;border-radius:10px;text-align:center;font-size:19px;font-weight:bold;text-decoration:none}");
        page += F("button{background:#00d26a;color:#fff}a{background:#444;color:#fff}.note{color:#bbb;line-height:1.5}.message{padding:12px;border-radius:9px;background:#274b39}");
        page += F("</style></head><body><main><h1>KegSense WiFi</h1><div class='card'>");

        if (!message.isEmpty())
        {
            page += F("<div class='message'>");
            page += escapeHtml(message);
            page += F("</div>");
        }

        page += F("<p class='note'>Velg nettverk og funksjonene denne KegSense-enheten skal bruke. Valgene lagres og kan endres senere.</p>");
        page += F("<form method='post' action='/wifi/save'><label>WiFi-nettverk</label><select name='ssid' required>");

        const String savedSsid = wifiPreferences.getString("ssid", "");
        const int count = WiFi.scanNetworks();
        if (count <= 0)
        {
            if (!savedSsid.isEmpty())
            {
                page += F("<option selected value='");
                page += escapeHtml(savedSsid);
                page += F("'>Lagret nettverk: ");
                page += escapeHtml(savedSsid);
                page += F("</option>");
            }
            else
                page += F("<option value=''>Ingen nettverk funnet</option>");
        }
        else
        {
            for (int i = 0; i < count; i++)
            {
                const String ssid = WiFi.SSID(i);
                if (ssid.isEmpty())
                    continue;
                page += F("<option value='");
                page += escapeHtml(ssid);
                page += F("'");
                if (ssid == savedSsid)
                    page += F(" selected");
                page += F(">");
                page += escapeHtml(ssid);
                page += " (" + String(WiFi.RSSI(i)) + " dBm)";
                page += F("</option>");
            }
        }

        WiFi.scanDelete();
        page += F("</select><label>Passord</label><input name='password' type='password' autocomplete='current-password' placeholder='");
        page += savedSsid.isEmpty() ? F("WiFi-passord") : F("La stå tomt for å beholde lagret passord");
        page += F("'>");

        page += F("<fieldset><legend>Enhetsoppsett</legend><label>Antall fat</label><select name='keg_count'>");
        for (size_t count = 1; count <= MAX_KEGS; count++)
        {
            page += F("<option value='");
            page += String(count);
            page += F("'");
            if (count == getActiveKegCount())
                page += F(" selected");
            page += F(">");
            page += String(count);
            page += F(" fat</option>");
        }
        page += F("</select>");
        page += F("<label class='toggle'><input type='checkbox' name='feature_temp'");
        if (isTemperatureFeatureEnabled()) page += F(" checked");
        page += F("> Temperaturmåling</label>");
        page += F("<label class='toggle'><input type='checkbox' name='feature_history'");
        if (isHistoryFeatureEnabled()) page += F(" checked");
        page += F("> Forbrukshistorikk</label>");
        page += F("<label class='toggle'><input type='checkbox' name='feature_remote'");
        if (isRemoteFeatureEnabled()) page += F(" checked");
        page += F("> Cloudflare Remote</label>");
        page += F("<label>Remote enhets-ID</label><input name='remote_id' maxlength='32' value='");
        page += escapeHtml(getRemoteDeviceId());
        page += F("' placeholder='for eksempel kegsense-hjemme'>");
        page += F("<label>Cloudflare Worker-URL</label><input name='remote_url' type='url' maxlength='200' value='");
        page += escapeHtml(getRemoteUrl());
        page += F("' placeholder='https://din-worker.workers.dev/api/telemetry'>");
        page += F("<label>Remote enhetsnøkkel</label><input name='remote_token' type='password' maxlength='128' autocomplete='new-password' placeholder='");
        page += getRemoteToken().isEmpty() ? F("Lim inn enhetsnøkkel") : F("La stå tomt for å beholde lagret nøkkel");
        page += F("'><p class='note'>URL og enhetsnøkkel opprettes i Cloudflare-oppsettet. Nøkkelen vises aldri etter lagring.</p>");
        page += F("</fieldset>");
        page += F("<button type='submit'>Lagre og koble til</button></form>");
        if (!portalActive)
            page += F("<a href='/settings'>Tilbake til innstillinger</a>");
        page += F("</div></main></body></html>");
        return page;
    }

    void redirectToPortal(AsyncWebServerRequest* request)
    {
        request->redirect("/wifi");
    }
}

void wifiSetupBegin()
{
    wifiPreferences.begin("wifi", false);
    const String ssid = wifiPreferences.getString("ssid", "");
    const String password = wifiPreferences.getString("password", "");

    if (ssid.isEmpty())
    {
        startPortal();
        return;
    }

    Serial.println();
    Serial.print("Kobler til WiFi: ");
    Serial.println(ssid);
    WiFi.mode(WIFI_STA);
    WiFi.setHostname(DEVICE_HOSTNAME);
    WiFi.begin(ssid.c_str(), password.c_str());

    const uint32_t startedAt = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - startedAt < CONNECT_TIMEOUT_MS)
    {
        delay(300);
        Serial.print('.');
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println();
        Serial.print("IP: ");
        Serial.println(WiFi.localIP());
        startMdns();
        return;
    }

    Serial.println();
    Serial.println("Kunne ikke koble til lagret WiFi.");
    startPortal();
}

void wifiSetupRegisterRoutes(AsyncWebServer& server)
{
    server.on("/wifi", HTTP_GET, [](AsyncWebServerRequest* request)
    {
        request->send(200, "text/html; charset=utf-8", buildPortalPage());
    });

    server.on("/wifi/save", HTTP_POST, [](AsyncWebServerRequest* request)
    {
        if (!request->hasParam("ssid", true))
            return request->send(400, "text/plain; charset=utf-8", "Mangler nettverksnavn");

        String ssid = request->getParam("ssid", true)->value();
        String password;
        if (request->hasParam("password", true))
            password = request->getParam("password", true)->value();
        ssid.trim();

        if (ssid.isEmpty() || ssid.length() > 32 || password.length() > 63)
            return request->send(400, "text/plain; charset=utf-8", "Ugyldig nettverksnavn eller passord");

        const String savedSsid = wifiPreferences.getString("ssid", "");
        if (password.isEmpty() && ssid == savedSsid)
            password = wifiPreferences.getString("password", "");

        size_t kegCount = MAX_KEGS;
        if (request->hasParam("keg_count", true))
            kegCount = static_cast<size_t>(request->getParam("keg_count", true)->value().toInt());
        if (kegCount < 1 || kegCount > MAX_KEGS)
            return request->send(400, "text/plain; charset=utf-8", "Ugyldig antall fat");

        const bool temperatureEnabled = request->hasParam("feature_temp", true);
        const bool historyEnabled = request->hasParam("feature_history", true);
        const bool remoteEnabled = request->hasParam("feature_remote", true);

        String remoteId;
        String remoteUrl;
        String remoteToken;
        if (request->hasParam("remote_id", true))
            remoteId = request->getParam("remote_id", true)->value();
        if (request->hasParam("remote_url", true))
            remoteUrl = request->getParam("remote_url", true)->value();
        if (request->hasParam("remote_token", true))
            remoteToken = request->getParam("remote_token", true)->value();
        remoteId.trim();
        remoteUrl.trim();
        remoteToken.trim();

        const bool tokenAvailable = !remoteToken.isEmpty() || !getRemoteToken().isEmpty();
        if (remoteEnabled &&
            (remoteId.isEmpty() || remoteId.length() > 32 ||
             !remoteUrl.startsWith("https://") || remoteUrl.length() > 200 ||
             !tokenAvailable || remoteToken.length() > 128))
        {
            return request->send(400, "text/plain; charset=utf-8",
                                 "Remote krever gyldig enhets-ID, HTTPS Worker-URL og enhetsnøkkel");
        }

        wifiPreferences.putString("ssid", ssid);
        wifiPreferences.putString("password", password);
        saveDeviceFeatures(kegCount, temperatureEnabled, historyEnabled, remoteEnabled);
        saveRemoteConfiguration(remoteId, remoteUrl, remoteToken);
        request->send(200, "text/html; charset=utf-8",
            buildPortalPage("Innstillingene er lagret. KegSense starter på nytt. Koble tilbake til hjemmenettverket og åpne http://kegsense.local"));
        restartPending = true;
        restartAt = millis();
    });

    server.on("/wifi/reset", HTTP_POST, [](AsyncWebServerRequest* request)
    {
        wifiPreferences.remove("ssid");
        wifiPreferences.remove("password");
        request->send(200, "text/plain; charset=utf-8",
                      "WiFi er slettet. KegSense starter oppsettsmodus...");
        restartPending = true;
        restartAt = millis();
    });

    server.on("/generate_204", HTTP_GET, redirectToPortal);
    server.on("/hotspot-detect.html", HTTP_GET, redirectToPortal);
    server.on("/ncsi.txt", HTTP_GET, redirectToPortal);
    server.on("/connecttest.txt", HTTP_GET, redirectToPortal);
}

void wifiSetupLoop()
{
    if (portalActive)
        dnsServer.processNextRequest();

    if (restartPending && millis() - restartAt >= 1800)
        ESP.restart();
}

bool wifiSetupIsPortalActive()
{
    return portalActive;
}
