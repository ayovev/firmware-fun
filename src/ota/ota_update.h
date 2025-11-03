#pragma once
#include <Arduino.h>

void checkForFirmwareUpdate();
void performOTAUpdate();
bool isNewerVersion(const char *version);
