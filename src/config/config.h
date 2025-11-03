#pragma once

// Version is injected at compile time via -DFIRMWARE_VERSION="1.2.3"
#ifndef FIRMWARE_VERSION
#define FIRMWARE_VERSION "dev" // Fallback for local builds
#endif

extern const char *FIRMWARE_SIGNING_PUBLIC_KEY;
extern const char *LATEST_RELEASE_URL;
extern const char *DEVICE_MODEL;
extern const char *AP_PASSWORD;
extern const unsigned long WIFI_CHECK_INTERVAL;
extern const unsigned long UPDATE_CHECK_INTERVAL;
