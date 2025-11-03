#include "config.h"

const char *FIRMWARE_SIGNING_PUBLIC_KEY = R"(
-----BEGIN PUBLIC KEY-----
MIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEAqMTNpFtjDxJEYOTK9ohS
L0WrpZbhoIs+YPoc0U2T/jsFg98P6s/rwbATCdMEm8991yU4lFCFM7ZBV467fKDz
WSat9+bk/LNuw75dq7VkDJJMf8ABCK90V1h0LQaPjxRaTEeKO7fPVG4tLv2PsZn7
UC1Bfs/f1cjsgc3A/3Fjr+6jb9DW+jMl0UQi+HO5p5RrA5TU2kTnpD+VCqZhFS0H
7ObZqkII46HMDxBb/QCNRo+GoXD/3IGl8QSRqE++wsmzlDnH2jHblMqnBD9EIK25
/6novnjeVKoAHlz5B8C8dqdPKNWYrw4qOEXmCfqlr6qOMV9awyip7mURQWw5a8xp
+wIDAQAB
-----END PUBLIC KEY-----
)";
const char *LATEST_RELEASE_URL = "https://api.github.com/repos/ayovev/firmware-fun/releases/latest";
const char *DEVICE_MODEL = "P61";
const char *AP_PASSWORD = "";                                           // Open network for easy setup
const unsigned long WIFI_CHECK_INTERVAL = 30000UL;                      // 30 seconds
const unsigned long UPDATE_CHECK_INTERVAL = 6UL * 60UL * 60UL * 1000UL; // 6 hours