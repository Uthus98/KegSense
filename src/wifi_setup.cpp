#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <Preferences.h>

#include "wifi_setup.h"

namespace
{
    constexpr char AP_NAME[] = "KegSense-Setup";
    constexpr uint32_t CONNECT_TIMEOUT_MS = 15000;

    DNSServer dnsServer;
    Preferences wifiPreferences;
    bool portalActive = false;
    bool restartPending = false;
    uint32_t restartAt = 0;

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
        page += F("button,a{display:block;width:100%;padding:14px;margin-top:20px;border:0;border-radius:10px;text-align:center;font-size:19px;font-weight:bold;text-decoration:none}");
        page += F("button{background:#00d26a;color:#fff}a{background:#444;color:#fff}.note{color:#bbb;line-height:1.5}.message{padding:12px;border-radius:9px;background:#274b39}");
        page += F("</style></head><body><main><h1>KegSense WiFi</h1><div class='card'>");

        if (!message.isEmpty())
        {
            page += F("<div class='message'>");
            page += escapeHtml(message);
            page += F("</div>");
        }

        page += F("<p class='note'>Velg nettverket KegSense skal bruke. Opplysningene lagres i ESP32 og beholdes etter omstart.</p>");
        page += F("<form method='post' action='/wifi/save'><label>WiFi-nettverk</label><select name='ssid' required>");

        const int count = WiFi.scanNetworks();
        if (count <= 0)
        {
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
                page += F("'>");
                page += escapeHtml(ssid);
                page += " (" + String(WiFi.RSSI(i)) + " dBm)";
                page += F("</option>");
            }
        }

        WiFi.scanDelete();
        page += F("</select><label>Passord</label><input name='password' type='password' autocomplete='current-password'>");
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

        wifiPreferences.putString("ssid", ssid);
        wifiPreferences.putString("password", password);
        request->send(200, "text/html; charset=utf-8",
            buildPortalPage("Innstillingene er lagret. KegSense starter på nytt..."));
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

