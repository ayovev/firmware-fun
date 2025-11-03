/*
 * P61 Data Bridge - Integrated Firmware
 * Features:
 * - 4-channel MAX31856 thermocouple reading
 * - WiFi provisioning with captive portal
 * - OTA firmware updates
 * - Web Serial API communication
 * - JSON-based command protocol
 * - Status LEDs
 */

#include <USB.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <DNSServer.h>
#include <HTTPClient.h>
#include <Update.h>
#include <SPI.h>
// TEMP - START
#include <Adafruit_MAX31855.h>
// TEMP - END
// #include <Adafruit_MAX31856.h>
#include <ArduinoJson.h>
#include "config/config.h"
#include "ota/ota_update.h"
#include "wifi/wifi_manager.h"
#include "common/connection_state.h"
#include "common/roast_state.h"

// ============================================================================
// CONFIGURATION
// ============================================================================

// SPI Pins for MAX31856 (ESP32-S3 default SPI)
#define SPI_MOSI 11
#define SPI_MISO 13
#define SPI_SCK 12

// Chip Select pins for 4 MAX31856 channels
#define CS_PIN_1 5
#define CS_PIN_2 10
#define CS_PIN_3 15
#define CS_PIN_4 16

// Status LED pins (active LOW)
#define LED_CONN 4 // Connection status (blue)
#define LED_DATA 6 // Data transmission (yellow/amber)

// Button pins
#define BOOT_BTN 0

// ============================================================================
// GLOBAL OBJECTS
// ============================================================================

// TEMP - START
Adafruit_MAX31855 thermocouple1(CS_PIN_1);
// TEMP - END

// Create 4 MAX31856 instances
// Adafruit_MAX31856 thermocouple1(CS_PIN_1);
// Adafruit_MAX31856 thermocouple2(CS_PIN_2);
// Adafruit_MAX31856 thermocouple3(CS_PIN_3);
// Adafruit_MAX31856 thermocouple4(CS_PIN_4);

Preferences preferences;
WebServer server(80);
DNSServer dnsServer;

// ============================================================================
// STATE VARIABLES
// ============================================================================

// Device identification
String deviceId;
String deviceSerialNumber;

// WiFi state
bool wifiConfigured = false;
bool apModeActive = false;
unsigned long lastWiFiCheck = 0;

// Temperature reading state
int samplingRateMs = 1000; // Default 1 second
unsigned long lastReadingTime = 0;

// Serial command processing
String inputBuffer = "";

// OTA update state
unsigned long lastUpdateCheck = 0;
bool updateAvailable = false;
String pendingFirmwareVersion = "";

// Roast state tracking
RoastState currentRoastState = IDLE;
unsigned long roastStartTime = 0;
unsigned long lastActivityTime = 0;
const unsigned long ACTIVITY_TIMEOUT = 60000; // 60 seconds of no high temps = idle

// Connection state
ConnectionState currentConnectionState = DISCONNECTED;

// ============================================================================
// FUNCTION DECLARATIONS
// ============================================================================

void generateDeviceIds();
void initializeThermocouples();
void readAndTransmitTemperatures();
void readChannel(JsonArray &channels, int channelNum, Adafruit_MAX31855 &tc);
void handleSerialCommands();
void processCommand(String command);
void sendReadyMessage();
void setConnectionState(ConnectionState state);
void blinkSetupLED();
void checkFactoryReset();
int calculateLoopDelay(int samplingRate);

// ============================================================================
// SETUP
// ============================================================================

void setup()
{
  Serial.begin(115200);
  delay(1000);

// Custom USB device identification (optional)
#if ARDUINO_USB_CDC_ON_BOOT
  USB.manufacturerName("PuckPrep, Inc.");
  USB.productName("P61");
  USB.begin();
#endif

  Serial.println("\n\n==================================");
  Serial.println("     Data Bridge Initializing");
  Serial.println("==================================");
  Serial.printf("Model: %s\n", DEVICE_MODEL);
  Serial.printf("Firmware: v%s\n", FIRMWARE_VERSION);

  // Initialize LED pins
  pinMode(LED_CONN, OUTPUT);
  pinMode(LED_DATA, OUTPUT);
  digitalWrite(LED_CONN, HIGH); // OFF (active LOW)
  digitalWrite(LED_DATA, HIGH); // OFF (active LOW)

  // Initialize button
  pinMode(BOOT_BTN, INPUT_PULLUP);

  // Generate device IDs
  generateDeviceIds();
  Serial.printf("Device ID: %s\n", deviceId.c_str());
  Serial.printf("Serial Number: %s\n", deviceSerialNumber.c_str());

  // Initialize preferences
  preferences.begin("config", false);

  // Load sampling rate if saved
  samplingRateMs = preferences.getInt("sampling_rate", 5000);
  Serial.printf("Sampling Rate: %d ms\n", samplingRateMs);

  // Initialize SPI
  SPI.begin(SPI_SCK, SPI_MISO, SPI_MOSI);

  // Initialize all 4 thermocouples
  initializeThermocouples();

  // Try to connect to saved WiFi
  String savedSSID = preferences.getString("ssid", "");
  String savedPassword = preferences.getString("password", "");

  if (savedSSID.length() > 0)
  {
    Serial.println("\nAttempting WiFi connection...");
    wifiConfigured = connectToWiFi(savedSSID.c_str(), savedPassword.c_str());

    if (wifiConfigured)
    {
      Serial.println("✓ WiFi Connected");
      setConnectionState(CONNECTED);

      // Check for firmware updates on startup
      checkForFirmwareUpdate();
    }
    else
    {
      Serial.println("✗ WiFi connection failed");
    }
  }

  // If not connected, start AP mode
  if (!wifiConfigured)
  {
    Serial.println("\nStarting WiFi Setup Mode");
    startAPMode();
  }

  // Send initial ready message
  sendReadyMessage();

  Serial.println("\n=================================");
  Serial.println("        Data Bridge Ready");
  Serial.println("=================================\n");
}

// ============================================================================
// MAIN LOOP
// ============================================================================

void loop()
{
  unsigned long currentTime = millis();

  // Handle AP mode operations
  if (apModeActive)
  {
    dnsServer.processNextRequest();
    server.handleClient();
    blinkSetupLED();
  }

  // Handle WiFi connection monitoring
  if (wifiConfigured && !apModeActive)
  {
    monitorWiFiConnection();

    // Periodic firmware update check
    if (currentTime - lastUpdateCheck > UPDATE_CHECK_INTERVAL)
    {
      checkForFirmwareUpdate();
      lastUpdateCheck = currentTime;
    }
  }

  // Read and transmit temperature data
  if (currentTime - lastReadingTime >= samplingRateMs)
  {
    lastReadingTime = currentTime;
    readAndTransmitTemperatures();
  }

  // Handle serial commands
  handleSerialCommands();

  // Check for factory reset button press (hold BOOT for 5 seconds)
  checkFactoryReset();

  // Dynamic loop delay based on sampling rate
  int loopDelay = calculateLoopDelay(samplingRateMs);
  delay(loopDelay);
}

// ============================================================================
// DEVICE IDENTIFICATION
// ============================================================================

void generateDeviceIds()
{
  uint64_t mac = ESP.getEfuseMac();

  // Full serial number for database
  char serialBuf[20];
  sprintf(serialBuf, "P61-%012llX", mac);
  deviceSerialNumber = String(serialBuf);

  // Short device ID for display
  char idBuf[20];
  sprintf(idBuf, "%06X", (uint32_t)(mac & 0xFFFFFF));
  deviceId = String(idBuf);

  // Store in preferences if not already set
  if (!preferences.isKey("serial_number"))
  {
    preferences.putString("serial_number", deviceSerialNumber);
  }
}

// ============================================================================
// THERMOCOUPLE INITIALIZATION
// ============================================================================

void initializeThermocouples()
{
  Serial.println("\nInitializing thermocouples...");

  bool success = true;

  if (!thermocouple1.begin())
  {
    Serial.println("✗ Channel 1 (MAX31856) initialization failed!");
    success = false;
  }
  else
  {
    // thermocouple1.setThermocoupleType(MAX31856_TCTYPE_K);
    Serial.println("✓ Channel 1 ready (K-type)");
  }

  // if (!thermocouple2.begin()) {
  //   Serial.println("✗ Channel 2 (MAX31856) initialization failed!");
  //   success = false;
  // } else {
  //   thermocouple2.setThermocoupleType(MAX31856_TCTYPE_K);
  //   Serial.println("✓ Channel 2 ready (K-type)");
  // }

  // if (!thermocouple3.begin()) {
  //   Serial.println("✗ Channel 3 (MAX31856) initialization failed!");
  //   success = false;
  // } else {
  //   thermocouple3.setThermocoupleType(MAX31856_TCTYPE_K);
  //   Serial.println("✓ Channel 3 ready (K-type)");
  // }

  // if (!thermocouple4.begin()) {
  //   Serial.println("✗ Channel 4 (MAX31856) initialization failed!");
  //   success = false;
  // } else {
  //   thermocouple4.setThermocoupleType(MAX31856_TCTYPE_K);
  //   Serial.println("✓ Channel 4 ready (K-type)");
  // }

  if (success)
  {
    Serial.println("All thermocouples initialized successfully");
  }
}

// ============================================================================
// TEMPERATURE READING
// ============================================================================

void readAndTransmitTemperatures()
{
  setConnectionState(TRANSMITTING);

  JsonDocument doc;
  doc["type"] = "data";
  doc["device_id"] = deviceSerialNumber;
  doc["firmware_version"] = FIRMWARE_VERSION;

  JsonObject meta = doc["metadata"].to<JsonObject>();
  meta["timestamp"] = millis();
  meta["sampling_rate_ms"] = samplingRateMs;

  JsonArray channels = doc["channels"].to<JsonArray>();

  // Read all 4 channels
  readChannel(channels, 1, thermocouple1);
  // readChannel(channels, 2, thermocouple2);
  // readChannel(channels, 3, thermocouple3);
  // readChannel(channels, 4, thermocouple4);

  serializeJson(doc, Serial);
  Serial.println();

  // Brief LED blink to indicate transmission
  digitalWrite(LED_DATA, LOW);
  delay(50);
  digitalWrite(LED_DATA, HIGH);

  setConnectionState(wifiConfigured ? CONNECTED : DISCONNECTED);
}

void readChannel(JsonArray &channels, int channelNum, Adafruit_MAX31855 &tc)
{
  JsonObject channel = channels.add<JsonObject>();
  channel["channel"] = channelNum;

  // uint8_t fault = tc.readFault();

  // if (fault) {
  //   channel["status"] = "error";
  //   channel["fault_code"] = fault;

  //   // Decode fault
  //   JsonArray faults = channel.createNestedArray("faults");
  //   if (fault & MAX31856_FAULT_CJRANGE) faults.add("cold_junction_range");
  //   if (fault & MAX31856_FAULT_TCRANGE) faults.add("thermocouple_range");
  //   if (fault & MAX31856_FAULT_CJHIGH) faults.add("cold_junction_high");
  //   if (fault & MAX31856_FAULT_CJLOW) faults.add("cold_junction_low");
  //   if (fault & MAX31856_FAULT_TCHIGH) faults.add("thermocouple_high");
  //   if (fault & MAX31856_FAULT_TCLOW) faults.add("thermocouple_low");
  //   if (fault & MAX31856_FAULT_OVUV) faults.add("over_under_voltage");
  //   if (fault & MAX31856_FAULT_OPEN) faults.add("open_circuit");

  //   channel["temperature_c"] = nullptr;
  //   channel["temperature_f"] = nullptr;
  // } else {
  channel["status"] = "ok";
  // float tempC = tc.readThermocoupleTemperature();
  float tempC = tc.readCelsius();

  channel["temperature_c"] = tempC;
  // channel["cold_junction_c"] = tc.readCJTemperature();
  // }
}

// ============================================================================
// SERIAL COMMAND HANDLING
// ============================================================================

void handleSerialCommands()
{
  while (Serial.available())
  {
    char c = Serial.read();

    if (c == '\n' || c == '\r')
    {
      if (inputBuffer.length() > 0)
      {
        processCommand(inputBuffer);
        inputBuffer = "";
      }
    }
    else
    {
      inputBuffer += c;
    }
  }
}

void processCommand(String command)
{
  JsonDocument docIn;
  DeserializationError error = deserializeJson(docIn, command);

  JsonDocument docOut;
  docOut["device_id"] = deviceSerialNumber;
  JsonObject meta = docOut["metadata"].to<JsonObject>();
  meta["timestamp"] = millis();
  JsonObject payload = docOut["payload"].to<JsonObject>();

  if (error)
  {
    docOut["type"] = "error";
    payload["error"] = "Invalid JSON command";
    payload["details"] = command;
    serializeJson(docOut, Serial);
    Serial.println();
    return;
  }

  // Handle commands
  if (docIn["update_connection_status"].is<const char *>())
  {
    String status = docIn["update_connection_status"];

    if (status == "connected")
    {
      setConnectionState(CONNECTED);
    }
    else if (status == "disconnected")
    {
      setConnectionState(DISCONNECTED);
    }

    docOut["type"] = "configuration";
    payload["result"] = "status_updated";
    payload["connection_state"] = status;
  }
  else if (docIn["update_sampling_rate"].is<int>())
  {
    int newRate = docIn["update_sampling_rate"];

    if (newRate >= 1000 && newRate <= 60000)
    {
      samplingRateMs = newRate;
      preferences.putInt("sampling_rate", samplingRateMs);

      docOut["type"] = "configuration";
      payload["result"] = "sampling_rate_updated";
      payload["new_rate_ms"] = samplingRateMs;
    }
    else
    {
      docOut["type"] = "error";
      payload["error"] = "Invalid sampling rate. Must be between 1000-60000ms";
      payload["requested_rate"] = newRate;
    }
  }
  else if (docIn["get_device_info"].is<bool>())
  {
    docOut["type"] = "device_info";
    payload["serial_number"] = deviceSerialNumber;
    payload["device_id"] = deviceId;
    payload["firmware_version"] = FIRMWARE_VERSION;
    payload["model"] = DEVICE_MODEL;
    payload["wifi_configured"] = wifiConfigured;
    payload["sampling_rate_ms"] = samplingRateMs;

    if (wifiConfigured)
    {
      payload["wifi_ssid"] = WiFi.SSID();
      payload["wifi_rssi"] = WiFi.RSSI();
      payload["ip_address"] = WiFi.localIP().toString();
    }
  }
  else if (docIn["trigger_ota_update"].is<bool>())
  {
    docOut["type"] = "configuration";
    payload["result"] = "ota_update_triggered";
    serializeJson(docOut, Serial);
    Serial.println();

    // Trigger OTA update
    performOTAUpdate();
    return;
  }
  // else if (docIn.containsKey("set_thermocouple_type")) {
  //   int channel = docIn["channel"];
  //   String type = docIn["set_thermocouple_type"];

  //   max31856_thermocoupletype_t tcType;

  //   if (type == "K") tcType = MAX31856_TCTYPE_K;
  //   else if (type == "J") tcType = MAX31856_TCTYPE_J;
  //   else if (type == "T") tcType = MAX31856_TCTYPE_T;
  //   else if (type == "N") tcType = MAX31856_TCTYPE_N;
  //   else if (type == "S") tcType = MAX31856_TCTYPE_S;
  //   else if (type == "E") tcType = MAX31856_TCTYPE_E;
  //   else if (type == "B") tcType = MAX31856_TCTYPE_B;
  //   else if (type == "R") tcType = MAX31856_TCTYPE_R;
  //   else {
  //     docOut["type"] = "error";
  //     payload["error"] = "Invalid thermocouple type";
  //     payload["requested_type"] = type;
  //     serializeJson(docOut, Serial);
  //     Serial.println();
  //     return;
  //   }

  //   bool success = false;
  //   // if (channel == 1) { thermocouple1.setThermocoupleType(tcType); success = true; }
  //   // else if (channel == 2) { thermocouple2.setThermocoupleType(tcType); success = true; }
  //   // else if (channel == 3) { thermocouple3.setThermocoupleType(tcType); success = true; }
  //   // else if (channel == 4) { thermocouple4.setThermocoupleType(tcType); success = true; }

  //   if (success) {
  //     docOut["type"] = "configuration";
  //     payload["result"] = "thermocouple_type_updated";
  //     payload["channel"] = channel;
  //     payload["type"] = type;
  //   } else {
  //     docOut["type"] = "error";
  //     payload["error"] = "Invalid channel number (1-4)";
  //     payload["requested_channel"] = channel;
  //   }
  // }
  else
  {
    docOut["type"] = "error";
    payload["error"] = "Unknown command";
    payload["received"] = command;
  }

  serializeJson(docOut, Serial);
  Serial.println();
}

void sendReadyMessage()
{
  JsonDocument doc;
  doc["type"] = "ready";
  doc["device_id"] = deviceSerialNumber;
  doc["firmware_version"] = FIRMWARE_VERSION;
  doc["model"] = DEVICE_MODEL;

  JsonObject meta = doc["metadata"].to<JsonObject>();
  meta["timestamp"] = millis();
  meta["sampling_rate_ms"] = samplingRateMs;

  serializeJson(doc, Serial);
  Serial.println();
}

// ============================================================================
// LED STATUS MANAGEMENT
// ============================================================================

void setConnectionState(ConnectionState state)
{
  currentConnectionState = state;

  switch (state)
  {
  case DISCONNECTED:
    digitalWrite(LED_CONN, HIGH); // OFF
    digitalWrite(LED_DATA, HIGH); // OFF
    break;

  case SETUP_MODE:
    // Handled by blinkSetupLED() in loop
    digitalWrite(LED_CONN, HIGH); // OFF
    break;

  case CONNECTED:
    digitalWrite(LED_CONN, LOW);  // ON (solid)
    digitalWrite(LED_DATA, HIGH); // OFF
    break;

  case TRANSMITTING:
    digitalWrite(LED_CONN, LOW); // ON
    digitalWrite(LED_DATA, LOW); // ON (brief)
    break;
  }
}

void blinkSetupLED()
{
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 500)
  {
    digitalWrite(LED_DATA, !digitalRead(LED_DATA));
    lastBlink = millis();
  }
}

void checkFactoryReset()
{
  static unsigned long bootPressStart = 0;
  static bool bootPressed = false;

  if (digitalRead(BOOT_BTN) == LOW)
  {
    if (!bootPressed)
    {
      bootPressed = true;
      bootPressStart = millis();
    }

    // Check if held for 5 seconds
    if (millis() - bootPressStart > 5000)
    {
      Serial.println("\n=== FACTORY RESET ===");

      // Clear all preferences
      preferences.clear();

      // Notify via serial
      JsonDocument doc;
      doc["type"] = "factory_reset";
      doc["message"] = "All settings cleared, rebooting...";
      serializeJson(doc, Serial);
      Serial.println();

      // Blink LEDs rapidly
      for (int i = 0; i < 10; i++)
      {
        digitalWrite(LED_CONN, !digitalRead(LED_CONN));
        digitalWrite(LED_DATA, !digitalRead(LED_DATA));
        delay(100);
      }

      delay(1000);
      ESP.restart();
    }
  }
  else
  {
    bootPressed = false;
  }
}

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

int calculateLoopDelay(int samplingRate)
{
  if (samplingRate <= 2000)
    return 10;
  else if (samplingRate <= 10000)
    return 50;
  else if (samplingRate <= 30000)
    return 100;
  else
    return 250;
}