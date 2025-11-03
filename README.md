# Firmware Fun

## Overview

Firmware Fun is an embedded firmware project designed for the ESP32-S3 microcontroller. The firmware provides functionality for:

- **Thermocouple temperature sensing**: Reads data from multiple MAX31856 thermocouple amplifiers.
- **WiFi provisioning**: Allows the device to connect to WiFi networks via a captive portal.
- **Over-the-Air (OTA) firmware updates**: Securely downloads and verifies firmware updates from GitHub releases.
- **Web Serial API communication**: Enables communication with the device via a browser-based serial interface.
- **JSON-based command protocol**: Processes commands sent to the device in JSON format.
- **Status LEDs**: Provides visual feedback for connection and data transmission states.

## Features

### 1. **Thermocouple Reading**

- Uses the Adafruit MAX31856 library to interface with up to 4 thermocouple channels.
- Reads and transmits temperature data at a configurable sampling rate.

### 2. **WiFi Provisioning**

- Implements a captive portal for easy WiFi setup.
- Supports both Access Point (AP) mode and Station (STA) mode.
- Monitors WiFi connection status and reconnects if disconnected.

### 3. **OTA Firmware Updates**

- Downloads firmware updates from GitHub releases.
- Verifies firmware integrity using RSA PKCS#1 v1.5 signature verification with SHA256.
- Streams firmware to flash memory to avoid RAM exhaustion.

### 4. **Web Serial Communication**

- Implements a JSON-based protocol for sending and receiving commands via the Web Serial API.
- Provides real-time temperature data and device status.

### 5. **Status LEDs**

- Blue LED indicates WiFi connection status.
- Yellow LED indicates data transmission activity.

## Repository Structure

The repository is organized as follows:

```
firmware-fun/
├── src/
│   ├── main.cpp                # Entry point of the firmware
│   ├── config.h                # Configuration constants (extern declarations)
│   ├── config.cpp              # Configuration constants (definitions)
│   ├── wifi/                   # WiFi-related functionality
│   │   ├── wifi_manager.h      # WiFi manager header
│   │   └── wifi_manager.cpp    # WiFi manager implementation
│   ├── ota/                    # OTA update functionality
│   │   ├── ota_update.h        # OTA update header
│   │   └── ota_update.cpp      # OTA update implementation
│   ├── sensors/                # Sensor-related functionality
│   │   ├── thermocouple.h      # Thermocouple header
│   │   └── thermocouple.cpp    # Thermocouple implementation
│   ├── web/                    # Web server and captive portal
│   │   ├── setup_portal_html.h # HTML for the captive portal
│   │   └── web_server.cpp      # Web server implementation
│   ├── utils/                  # Utility headers
│   │   ├── connection_state.h  # Connection state enum
│   │   └── helpers.h           # Miscellaneous helper functions
├── platformio.ini              # PlatformIO project configuration
├── SETUP.md                    # Setup instructions for the project
├── CONTRIBUTING.md             # Contribution guidelines
├── boards/                     # Board-specific configuration files
│   └── esp32-s3-devkitc-1-n16r8.json
├── data/                       # Data files (e.g., SPIFFS)
│   └── default_16MB.csv
└── README.md                   # Project documentation (this file)
```

## Key Components

### 1. **`main.cpp`**

- Orchestrates the firmware's functionality.
- Handles setup and loop logic.
- Initializes peripherals, WiFi, and OTA updates.

### 2. **`config.h` and `config.cpp`**

- Centralized configuration constants.
- `config.h` declares `extern` constants, and `config.cpp` defines them.

### 3. **WiFi Manager**

- Located in `src/wifi/`.
- Handles WiFi connection, AP mode, and captive portal.

### 4. **OTA Updates**

- Located in `src/ota/`.
- Implements secure OTA updates with signature verification.

### 5. **Thermocouple Reading**

- Located in `src/sensors/`.
- Interfaces with MAX31856 thermocouple amplifiers.

### 6. **Web Server**

- Located in `src/web/`.
- Hosts the captive portal for WiFi provisioning.

### 7. **Utilities**

- Located in `src/utils/`.
- Includes shared enums and helper functions.

## Build and Upload

This project uses PlatformIO for building and uploading firmware.

### Prerequisites

- Install [PlatformIO](https://platformio.org/).
- Ensure the ESP32-S3 board is connected to your computer.

### Build

Run the following command in the project root:

```bash
pio run
```

### Upload

To upload the firmware to the ESP32-S3, run:

```bash
pio run --target upload
```

### Monitor

To monitor the serial output, run:

```bash
pio device monitor
```

## OTA Update Process

1. The device checks for updates at regular intervals (default: every 6 hours).
2. It downloads the latest firmware from the GitHub releases API.
3. The firmware is verified using RSA signature verification.
4. If valid, the firmware is flashed, and the device reboots.

## Contribution

Contributions are welcome! Please see `CONTRIBUTING.md` for guidelines.

## License

This project is licensed under the MIT License. See `LICENSE` for details.

---

Happy coding!
