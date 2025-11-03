#include <WiFi.h>
#include <Preferences.h>
#include "wifi_manager.h"
#include "config/config.h"
#include "common/connection_state.h"
#include "setup_portal_html.h"

extern Preferences preferences;
extern String deviceId;
extern String deviceSerialNumber;
extern bool wifiConfigured;
extern bool apModeActive;
extern void setConnectionState(ConnectionState state);
extern WebServer server;
extern DNSServer dnsServer;

bool connectToWiFi(const char *ssid, const char *password)
{
    Serial.printf("Connecting to WiFi: %s\n", ssid);

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30)
    {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.printf("✓ Connected! IP: %s, RSSI: %d dBm\n",
                      WiFi.localIP().toString().c_str(), WiFi.RSSI());
        return true;
    }

    Serial.println("✗ Connection failed");
    return false;
}

void monitorWiFiConnection()
{
    extern unsigned long lastWiFiCheck;
    // extern const unsigned long WIFI_CHECK_INTERVAL;
    if (millis() - lastWiFiCheck < WIFI_CHECK_INTERVAL)
    {
        return;
    }
    lastWiFiCheck = millis();

    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("WiFi disconnected, attempting reconnect...");
        setConnectionState(DISCONNECTED);

        String savedSSID = preferences.getString("ssid", "");
        String savedPassword = preferences.getString("password", "");

        if (!connectToWiFi(savedSSID.c_str(), savedPassword.c_str()))
        {
            Serial.println("Reconnection failed, entering setup mode");
            startAPMode();
        }
        else
        {
            setConnectionState(CONNECTED);
        }
    }
}

void startAPMode()
{
    apModeActive = true;
    setConnectionState(SETUP_MODE);

    char apName[32];
    sprintf(apName, "PuckPrep P61-%s", deviceId.c_str());

    Serial.printf("Starting AP: %s\n", apName);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName, AP_PASSWORD);

    IPAddress apIP = WiFi.softAPIP();
    Serial.printf("AP IP: %s\n", apIP.toString().c_str());

    dnsServer.start(53, "*", apIP);

    server.on("/", handleRoot);
    server.on("/scan", handleScan);
    server.on("/connect", HTTP_POST, handleConnect);
    server.onNotFound(handleRoot);

    server.begin();
    Serial.println("Setup portal ready at http://192.168.4.1");
}

void handleRoot()
{
    extern String deviceSerialNumber;
    String html = String(setupPortalHtml);
    html.replace("%SERIAL_NUMBER%", deviceSerialNumber);
    server.send(200, "text/html", html);
}

void handleScan()
{
    int n = WiFi.scanNetworks();
    String json = "[";
    for (int i = 0; i < n; i++)
    {
        if (i > 0)
            json += ",";
        json += "{\"ssid\":\"" + WiFi.SSID(i) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"secure\":" + String(WiFi.encryptionType(i) != WIFI_AUTH_OPEN) + "}";
    }
    json += "]";
    server.send(200, "application/json", json);
    WiFi.scanDelete();
}

void handleConnect()
{
    String ssid = server.arg("ssid");
    String password = server.arg("password");

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid.c_str(), password.c_str());

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30)
    {
        delay(500);
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        preferences.putString("ssid", ssid);
        preferences.putString("password", password);

        String json = "{\"success\":true,\"ip\":\"" + WiFi.localIP().toString() + "\"}";
        server.send(200, "application/json", json);

        delay(1000);

        apModeActive = false;
        wifiConfigured = true;
        setConnectionState(CONNECTED);

        dnsServer.stop();
        server.stop();
        WiFi.softAPdisconnect(true);
    }
    else
    {
        server.send(200, "application/json", "{\"success\":false}");
        WiFi.mode(WIFI_AP);
    }
}
