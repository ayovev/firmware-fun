#include <Preferences.h>
#include <HTTPClient.h>
#include <Update.h>
#include <mbedtls/sha256.h>
#include <mbedtls/pk.h>
#include <mbedtls/rsa.h>
#include <ArduinoJson.h>
#include "ota_update.h"
#include "config/config.h"

extern Preferences preferences;
extern String deviceSerialNumber;
extern String deviceId;
extern bool wifiConfigured;

void checkForFirmwareUpdate()
{
    if (WiFi.status() != WL_CONNECTED)
        return;

    Serial.println("Checking for firmware updates...");

    HTTPClient http;
    http.begin(LATEST_RELEASE_URL);
    http.addHeader("Accept", "application/vnd.github+json");

    int httpCode = http.GET();

    if (httpCode == 200)
    {
        JsonDocument doc;
        deserializeJson(doc, http.getStream());

        const char *latestVersion = doc["tag_name"];

        Serial.printf("Latest version: %s (current: %s)\n", latestVersion, FIRMWARE_VERSION);

        if (isNewerVersion(latestVersion))
        {
            // Find firmware files in assets
            String firmwareUrl, signatureUrl;

            JsonArray assets = doc["assets"];
            for (JsonObject asset : assets)
            {
                String name = asset["name"].as<String>();
                String url = asset["browser_download_url"].as<String>();

                if (name == "firmware.bin")
                    firmwareUrl = url;
                if (name == "firmware.bin.sig")
                    signatureUrl = url;
            }

            // Notify user via Serial
            JsonDocument notif;
            notif["type"] = "update_available";
            notif["version"] = latestVersion;
            notif["firmware_url"] = firmwareUrl;
            notif["signature_url"] = signatureUrl;
            notif["changelog"] = doc["body"];

            serializeJson(notif, Serial);
            Serial.println();
        }
    }

    http.end();
}

void performOTAUpdate()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Cannot update: WiFi not connected");
        return;
    }

    Serial.println("Starting OTA firmware update...");

    // Step 1: Fetch latest release info
    String firmwareUrl, signatureUrl;
    HTTPClient http;
    http.begin(LATEST_RELEASE_URL);
    http.addHeader("Accept", "application/vnd.github+json");
    int httpCode = http.GET();

    if (httpCode == 200)
    {
        JsonDocument releaseDoc;
        DeserializationError err = deserializeJson(releaseDoc, http.getStream());
        if (!err)
        {
            JsonArray assets = releaseDoc["assets"];
            for (JsonObject asset : assets)
            {
                String name = asset["name"].as<String>();
                String url = asset["browser_download_url"].as<String>();
                if (name == "firmware.bin")
                    firmwareUrl = url;
                if (name == "firmware.bin.sig")
                    signatureUrl = url;
            }
        }
    }
    http.end();

    if (firmwareUrl.length() == 0 || signatureUrl.length() == 0)
    {
        Serial.println("✗ Firmware or signature not found in release");
        return;
    }

    // Step 2: Download signature (small - 256 bytes)
    Serial.println("Downloading signature...");
    HTTPClient sigClient;
    sigClient.begin(signatureUrl);
    sigClient.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    sigClient.setUserAgent("ESP32-OTA-Updater");

    std::vector<uint8_t> signature;
    int sigCode = sigClient.GET();
    if (sigCode == 200)
    {
        WiFiClient *stream = sigClient.getStreamPtr();
        while (stream->available())
        {
            signature.push_back(stream->read());
        }
    }
    sigClient.end();

    if (signature.size() != 256)
    {
        Serial.printf("✗ Invalid signature size: %d bytes (expected 256)\n", signature.size());
        return;
    }

    Serial.println("✓ Signature downloaded");

    // Step 3: Download firmware while computing hash
    Serial.println("Downloading and hashing firmware...");
    HTTPClient fwClient;
    fwClient.begin(firmwareUrl);
    fwClient.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    fwClient.setUserAgent("ESP32-OTA-Updater");
    fwClient.setTimeout(30000); // 30 second timeout for large download

    int fwCode = fwClient.GET();
    if (fwCode != 200)
    {
        Serial.printf("✗ Firmware download failed: %d\n", fwCode);
        fwClient.end();
        return;
    }

    int contentLength = fwClient.getSize();
    Serial.printf("Firmware size: %d bytes\n", contentLength);

    // Initialize SHA256 context for incremental hashing
    mbedtls_sha256_context sha256_ctx;
    mbedtls_sha256_init(&sha256_ctx);
    mbedtls_sha256_starts(&sha256_ctx, 0); // 0 = SHA256 (not SHA224)

    // Begin OTA update
    if (!Update.begin(contentLength))
    {
        Serial.println("✗ Not enough space for OTA");
        fwClient.end();
        mbedtls_sha256_free(&sha256_ctx);
        return;
    }

    // Stream firmware in chunks, updating hash as we go
    WiFiClient *stream = fwClient.getStreamPtr();
    uint8_t buffer[512];
    size_t totalWritten = 0;

    // Download based on content length, not stream->available()
    while (totalWritten < contentLength)
    {
        // Wait for data to be available (with timeout)
        unsigned long timeout = millis();
        while (!stream->available() && (millis() - timeout < 5000))
        {
            delay(1);
        }

        if (!stream->available())
        {
            Serial.println("✗ Stream timeout - no data available");
            break;
        }

        size_t bytesToRead = min((size_t)(contentLength - totalWritten), sizeof(buffer));
        size_t bytesRead = stream->readBytes(buffer, bytesToRead);

        if (bytesRead == 0)
        {
            Serial.println("✗ Read returned 0 bytes");
            break;
        }

        // Update hash
        mbedtls_sha256_update(&sha256_ctx, buffer, bytesRead);

        // Write to OTA partition
        size_t written = Update.write(buffer, bytesRead);
        if (written != bytesRead)
        {
            Serial.println("✗ Write error during OTA");
            Update.abort();
            fwClient.end();
            mbedtls_sha256_free(&sha256_ctx);
            return;
        }

        totalWritten += written;

        // Progress indicator
        if (totalWritten % 51200 == 0) // Every 50KB
        {
            Serial.printf("Progress: %d/%d bytes (%.1f%%)\n",
                          totalWritten, contentLength,
                          (totalWritten * 100.0) / contentLength);
        }
    }

    fwClient.end();

    if (totalWritten != contentLength)
    {
        Serial.printf("✗ Incomplete download: %d/%d bytes\n", totalWritten, contentLength);
        Update.abort();
        mbedtls_sha256_free(&sha256_ctx);
        return;
    }

    // Finalize hash computation
    uint8_t computedHash[32];
    mbedtls_sha256_finish(&sha256_ctx, computedHash);
    mbedtls_sha256_free(&sha256_ctx);

    Serial.println("✓ Firmware downloaded and hashed");
    Serial.print("Computed hash: ");
    for (int i = 0; i < 32; i++)
    {
        Serial.printf("%02x", computedHash[i]);
    }
    Serial.println();

    // Step 4: Verify signature
    Serial.println("\n========================================");
    Serial.println("SIGNATURE VERIFICATION STARTING");
    Serial.println("========================================");
    delay(100);

    mbedtls_pk_context pk;
    mbedtls_pk_init(&pk);

    int pkParse = mbedtls_pk_parse_public_key(&pk,
                                              (const unsigned char *)FIRMWARE_SIGNING_PUBLIC_KEY,
                                              strlen(FIRMWARE_SIGNING_PUBLIC_KEY) + 1);

    Serial.printf("Public key parse result: %d\n", pkParse);
    delay(100);

    if (pkParse != 0)
    {
        Serial.printf("✗ Public key parse failed: -0x%04x\n", -pkParse);
        Update.abort();
        mbedtls_pk_free(&pk);
        return;
    }

    // Get RSA context and set padding
    mbedtls_rsa_context *rsa = mbedtls_pk_rsa(pk);
    mbedtls_rsa_set_padding(rsa, MBEDTLS_RSA_PKCS_V15, MBEDTLS_MD_SHA256);

    int verify = mbedtls_rsa_pkcs1_verify(rsa,
                                          NULL, // RNG function (not needed for verify)
                                          NULL, // RNG parameter (not needed for verify)
                                          MBEDTLS_RSA_PUBLIC,
                                          MBEDTLS_MD_SHA256,
                                          32,
                                          computedHash,
                                          signature.data());

    Serial.printf("Signature verification result: %d\n", verify);
    delay(100);

    mbedtls_pk_free(&pk);

    if (verify != 0)
    {
        Serial.printf("✗ Firmware signature verification failed: -0x%04x\n", -verify);
        Serial.println("✗ Aborting update for security reasons");
        Update.abort();
        return;
    }

    Serial.println("✓ Signature verified successfully!");
    Serial.println("========================================\n");

    // Step 5: Commit the update
    if (Update.end())
    {
        if (Update.isFinished())
        {
            Serial.println("✓ OTA update complete! Rebooting...");
            delay(2000);
            ESP.restart();
        }
        else
        {
            Serial.println("✗ OTA update not finished");
        }
    }
    else
    {
        Serial.printf("✗ OTA update failed: %s\n", Update.errorString());
    }
}

bool isNewerVersion(const char *version)
{
    // Remove leading 'v' (or 'V') prefix if present
    const char *verA = version;
    const char *verB = FIRMWARE_VERSION;

    if (verA[0] == 'v' || verA[0] == 'V')
        verA++;
    if (verB[0] == 'v' || verB[0] == 'V')
        verB++;

    return strcmp(verA, verB) > 0;
}
