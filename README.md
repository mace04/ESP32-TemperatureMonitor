# ESP32 Temperature Monitor

An IoT temperature and pressure monitoring system built on the ESP32 microcontroller using PlatformIO. This project integrates a BMP180 sensor for environmental readings, hosts a local MQTT broker, provides a web-based dashboard with real-time gauges, and supports over-the-air (OTA) updates for firmware and filesystem.

## Table of Contents
- [Features](#features)
- [Hardware Requirements](#hardware-requirements)
- [Software Requirements](#software-requirements)
- [Installation](#installation)
- [Configuration](#configuration)
- [Usage](#usage)
- [Web Interface](#web-interface)
- [MQTT Broker](#mqtt-broker)
- [OTA Updates](#ota-updates)
- [API Endpoints](#api-endpoints)
- [Known Issues](#known-issues)
- [Contributing](#contributing)
- [License](#license)

## Features

### Core Capabilities
- **Sensor Integration**: Reads temperature (°C) and atmospheric pressure (Pa) from a BMP180 sensor every 2 seconds.
- **WiFi Connectivity**: Connects to a specified WiFi network for remote access.
- **Local MQTT Broker**: Hosts an internal MQTT server for publishing sensor data.
- **Web Dashboard**: Serves a responsive web interface with animated gauges for real-time data visualization.
- **Real-Time Updates**: Uses Server-Sent Events (SSE) to push live sensor readings to web clients.
- **OTA Updates**: Supports remote firmware and filesystem updates via a web form.
- **SPIFFS Storage**: Stores web assets and configuration files in the ESP32's flash filesystem.

### Data Handling
- Publishes sensor data as JSON payloads to MQTT topics and SSE events.
- Logs readings to the serial console for debugging.

### Web Interface
- Temperature gauge: Linear display (0-40°C) with visual alerts above 30°C.
- Pressure gauge: Radial display (0-100% simulated, though actual data is pressure).
- Automatic updates without page refreshes.

## Hardware Requirements
- ESP32 DevKit v1 (or compatible board)
- BMP180 temperature and pressure sensor (connected via I2C)
- USB cable for programming and power
- Optional: External power supply for standalone operation

## Software Requirements
- PlatformIO IDE or CLI
- Arduino framework for ESP32
- Libraries (automatically installed via PlatformIO):
  - Adafruit BMP085 Library
  - PicoMQTT
  - ESPAsyncWebServer

## Installation

1. **Clone or Download the Project**:
   ```
   git clone <repository-url>
   cd ESP32-TemperatureMonitor
   ```

2. **Install Dependencies**:
   - Open the project in PlatformIO.
   - PlatformIO will automatically download the required libraries based on `platformio.ini`.

3. **Build the Project**:
   ```
   pio run
   ```

4. **Upload to ESP32**:
   - Connect your ESP32 via USB.
   - Upload firmware:
     ```
     pio run --target upload
     ```
   - Upload filesystem (SPIFFS):
     ```
     pio run --target uploadfs
     ```

5. **Monitor Serial Output** (optional):
   ```
   pio device monitor
   ```

## Configuration

### WiFi Settings
Edit `include/wifi_setup.h`:
- Update `SSID` and `PASSWORD` with your WiFi credentials.

### Sensor Settings
- The BMP180 sensor is configured for default I2C pins (SDA: GPIO 21, SCL: GPIO 22 on ESP32).
- Adjust `publishIntervalMs` in `src/main.cpp` to change the reading interval (default: 2000ms).

### Partition Scheme
- Uses `default_1.5MBapp_spiffs768KB.csv` for 1.5MB app space and 768KB SPIFFS.
- Located in PlatformIO's framework directory.

## Usage

1. Power on the ESP32 after uploading.
2. The device will connect to WiFi and initialize the sensor and MQTT broker.
3. Access the web dashboard at `http://<ESP32-IP>/` (find IP via serial logs or router admin).
4. Sensor data is published to MQTT and sent via SSE every 2 seconds.
5. Use the `/update` page for OTA updates.

### Serial Output
- Monitors connection status, sensor readings, and errors.

## Web Interface

- **Dashboard (`/` or `/index`)**: Displays temperature and pressure gauges.
- **Update Page (`/update`)**: Form for uploading firmware or filesystem updates.
- **Static Files**: CSS and JS served from SPIFFS.

## MQTT Broker

- **Topic**: `sensors/bmp180`
- **Payload**: `{"temperature": 25.00, "pressure": 101325.00}`
- Connect local MQTT clients to the ESP32's IP on port 1883 (default MQTT port).

## OTA Updates

1. Navigate to `http://<ESP32-IP>/update`.
2. Select "Firmware" or "Filesystem".
3. Choose a `.bin` file (for firmware) or archive (for SPIFFS).
4. Upload and wait for reboot.

## API Endpoints

- `GET /`: Serves the main dashboard.
- `GET /index`: Alias for dashboard.
- `GET /style.css`: Serves CSS styles.
- `GET /script.js`: Serves JavaScript (note: server maps to `/common.js`).
- `GET /update`: Serves the update form.
- `POST /update`: Handles file uploads for OTA.
- `GET /events`: SSE endpoint for real-time data.

## Known Issues

- **Web Server Not Initialized**: `initWebServer()` is defined but not called in `main.cpp`. Add `WifiSetup::initWebServer();` after `connect()` to enable the web interface.
- **Data Mismatch**: Sensor provides temperature and pressure, but the web interface expects temperature and humidity. Update `script.js` to handle pressure instead of humidity.
- **Missing Endpoint**: JS attempts to fetch `/readings` on load, but no handler exists. Implement a GET `/readings` endpoint to return current sensor data.
- **MQTT Loop**: `broker.loop()` is commented out; uncomment if required by PicoMQTT.
- **File Naming**: Server serves `/common.js`, but file is `script.js`. Rename or update references.
- **Hardcoded Credentials**: WiFi settings are not configurable at runtime.
- **No Authentication**: Web and MQTT access are unsecured.

## Contributing

1. Fork the repository.
2. Create a feature branch.
3. Make changes and test thoroughly.
4. Submit a pull request with a description of changes.

## License

This project is licensed under the MIT License. See LICENSE file for details.</content>
<parameter name="filePath">d:\dev\Microcontrollers\PlatformIO\ESP32\ESP32-TemperatureMonitor\README.md