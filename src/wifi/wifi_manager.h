#pragma once
#include <Arduino.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "common/connection_state.h"

bool connectToWiFi(const char *ssid, const char *password);
void monitorWiFiConnection();
void startAPMode();
void handleRoot();
void handleScan();
void handleConnect();

extern WebServer server;
extern DNSServer dnsServer;
extern bool wifiConfigured;
extern bool apModeActive;
extern void setConnectionState(ConnectionState state);
