# ESP32 Temperature Monitor

An IoT temperature and pressure monitoring system built on the ESP32 microcontroller using PlatformIO. This project integrates a BMP180 sensor for environmental readings, hosts a local MQTT broker, provides a web-based dashboard with real-time gauges, sends email notifications on status changes, and supports over-the-air (OTA) updates for firmware and filesystem.

## Project Summary

The ESP32 Temperature Monitor is a comprehensive IoT solution for environmental sensing and data visualization. It automatically detects and interfaces with BMP180 or BME280 sensors to collect temperature, pressure, and humidity data. The device establishes WiFi connectivity for remote access, hosts its own MQTT broker for local data publishing, and serves a responsive web dashboard featuring real-time animated gauges. Real-time updates are delivered via Server-Sent Events (SSE), ensuring live data visualization without page refreshes. The system supports over-the-air (OTA) updates for both firmware and filesystem, enabling seamless remote maintenance. It also syncs time via NTP for accurate timestamps, uses email alerts for printer readiness changes, and controls a fan connected to GPIO04 based on dynamic temperature thresholds. All web assets and configurations are stored in the ESP32's SPIFFS flash filesystem, making it a self-contained, standalone monitoring solution.

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
- **Sensor Integration**: Automatically detects and reads environmental data from BMP180 (temperature), BME280 (temperature and humidity), or provides simulated data in debug mode. Readings are taken at configurable intervals (default: 2 seconds).
- **WiFi Connectivity**: Connects to a specified WiFi network for remote access.
- **WiFi Fallback Hotspot**: Starts a local AP if the primary WiFi connection fails.
- **Local MQTT Broker**: Hosts an internal MQTT server for publishing sensor data and alerts.
- **Web Dashboard**: Serves a responsive web interface with animated gauges, printer status, and camera stream support.
- **Real-Time Updates**: Uses Server-Sent Events (SSE) to push live sensor readings and printer status to web clients.
- **OTA Updates**: Supports remote firmware and filesystem updates via a web form.
- **LCD Display (I2C 20x4)**: Shows temperature, humidity (if supported), IP address, printer status, and a live clock. The display updates efficiently (only when values change), and shows a dedicated message during firmware/filesystem uploads.
- **SPIFFS Storage**: Stores web assets and configuration files in the ESP32's flash filesystem.
- **Settings File**: Persists thresholds and camera URL in `/settings.json`, with automatic creation on first boot.
- **Email Notifications**: Sends SMTP alerts on printer status changes and periodic status emails when no MQTT subscribers are connected.
- **Fan Control**: Controls a fan on GPIO04 using dynamic hysteresis thresholds derived from printer thresholds and exposes HTTP control/status endpoints.
- **Fan Temperature-Rise Alert**: Publishes a dedicated MQTT warning when the fan is running and temperature is still rising.

### Sensor Capabilities
- **Auto-Detection**: Automatically detects BMP180 or BME280 sensors on startup.
- **Multi-Sensor Support**: Compatible with BMP180 (temperature and pressure) and BME280 (temperature, humidity, and pressure).
- **Debug Mode**: Provides simulated sensor data when no physical sensor is detected, useful for development and testing. Debug temperature now follows a cyclic ramp:
   - Increases by a random step between `0.5` and `1.5` °C per reading until it reaches `temperature_high_threshold + 1.5`.
   - Then decreases by a random step between `0.5` and `1.5` °C per reading until it reaches `ready_to_print_threshold - 1.5`.
   - Then repeats the same increase/decrease cycle continuously.
- **Configurable Sampling**: Adjustable reading interval (default: 2 seconds) for balancing responsiveness and power consumption.

### Connectivity and Communication
- **WiFi Management**: Robust WiFi connection handling with status monitoring.
- **NTP Time Sync**: Syncs device time for accurate timestamps in email notifications.
- **MQTT Publishing**: Publishes JSON-formatted sensor data to configurable MQTT topics.
- **Server-Sent Events**: Real-time data streaming to web clients without polling.
- **Web Server**: Asynchronous web server handling multiple concurrent connections.

### Data Handling
- Publishes sensor data as JSON payloads to MQTT topics and SSE events.
- Logs readings to the serial console for debugging.
- Structured data format: `{"temperature": 25.00, "humidity": 60.00}` (humidity only when available).
- Emits printer status with SSE payloads for UI updates.
- Sends periodic status emails every 15 minutes when enabled and no MQTT subscribers are connected.
- Fan rise alert interval is configurable through `fan_alert_interval_minutes` in `/settings.json`.

### Web Interface
- Temperature gauge: Linear display (0-40°C) with visual alerts.
- Humidity gauge: For BME280 sensors, displays humidity levels (hidden otherwise).
- **3D Printer Environment Status**: Shows NOT READY / READY / TOO HOT based on thresholds.
- **3D Printer Camera**: Expandable card that streams from a configurable URL.
- Automatic updates without page refreshes.
- Responsive design compatible with desktop and mobile devices.

### Maintenance and Updates
- **OTA Firmware Updates**: Upload new firmware binaries remotely.
- **Filesystem Updates**: Update web assets and configurations via OTA.
- **Serial Monitoring**: Detailed logging for troubleshooting and status monitoring.
- **Partition Management**: Optimized flash partitioning for app and filesystem storage.

## Hardware Requirements
- ESP32 DevKit v1 (or compatible board)
- BMP180 temperature and pressure sensor (connected via I2C)
- 20x4 I2C LCD display (typical address 0x27 or 0x3F)
- Fan connected to GPIO04 through a suitable driver stage (for example, transistor/MOSFET + external fan supply)
- USB cable for programming and power
- Optional: External power supply for standalone operation

## Software Requirements
- PlatformIO IDE or CLI
- Arduino framework for ESP32
- Libraries (automatically installed via PlatformIO):
  - [Adafruit BMP085 Library](https://github.com/adafruit/Adafruit_BMP085_Library)
  - [Adafruit BME280 Library](https://github.com/adafruit/Adafruit_BME280_Library)
  - [PicoMQTT](https://github.com/mlesniew/PicoMQTT)
  - [ESPAsyncWebServer](https://github.com/esp32async/ESPAsyncWebServer)
  - [LiquidCrystal_I2C](https://github.com/marcoschwartz/LiquidCrystal_I2C)
   - [ArduinoJson](https://github.com/bblanchon/ArduinoJson)

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
- In debug mode, `ready_to_print_threshold` and `temperature_high_threshold` also control the simulated temperature bounds described above.

### Settings File
Stored in `/settings.json` on SPIFFS. Created automatically if missing. Includes thresholds, camera URL, and SMTP settings for email notifications.

Example:
```
{
   "ready_to_print_threshold": 20.0,
   "temperature_high_threshold": 30.0,
   "camera_url": "http://192.168.0.18:8080/?action=stream",
   "email_enabled": true,
   "email_interval_minutes": 15,
   "smtp_host": "smtp.example.com",
   "smtp_port": 587,
   "smtp_secure": true,
   "smtp_user": "user@example.com",
   "smtp_password": "app-password",
   "email_sender": "monitor@example.com",
   "email_sender_name": "ESP32 Temp Monitor",
   "email_recipient": "recipient@example.com",
   "fan_alert_interval_minutes": 1
}
```

Fan-related settings:
- `fan_alert_interval_minutes`: Minimum interval between repeated `fan_on_temperature_increasing` MQTT alerts while fan is ON and temperature is rising.

### Partition Scheme
- Uses `default_1.5MBapp_spiffs768KB.csv` for 1.5MB app space and 768KB SPIFFS.
- Located in PlatformIO's framework directory.

## Usage

1. Power on the ESP32 after uploading.
2. The device will connect to WiFi and initialize the sensor and MQTT broker.
3. Access the web dashboard at `http://<ESP32-IP>/` (find IP via serial logs or router admin).
4. Sensor data is published to MQTT and sent via SSE every 2 seconds.
5. Use the `/update` page for OTA updates.

### LCD Display Behavior
- Temperature updates only when it changes.
- Humidity updates only when it changes (and only if a humidity sensor is present).
- IP address is displayed once after boot.
- Time updates every second.
- Printer status updates when thresholds are crossed.
- During OTA uploads, the LCD shows "Uploading firmware" or "Uploading filesystem" and suppresses other updates.

### Fan Control Behavior
- Fan pin: `GPIO04`.
- Fan default state on boot: `OFF`.
- Thresholds are recalculated whenever `ready_to_print_threshold` or `temperature_high_threshold` changes:
   - `midpoint = (ready_to_print_threshold + temperature_high_threshold) / 2`
   - `fan_on_threshold = midpoint + ((temperature_high_threshold - midpoint) * 0.65)`
   - `fan_off_threshold = midpoint - ((midpoint - ready_to_print_threshold) * 0.35)`
- Control logic:
   - Fan turns ON when `temperature > fan_on_threshold`.
   - Fan turns OFF when `temperature < fan_off_threshold`.
- While fan is ON, if the current temperature is greater than the previous reading, the firmware publishes `fan_on_temperature_increasing` alerts to MQTT topic `mqtt/alert`, limited by `fan_alert_interval_minutes`.
- `POST /fan` can force fan ON/OFF, but automatic loop logic may override it on subsequent sensor updates depending on temperature.

### Serial Output
- Monitors connection status, sensor readings, and errors.

## Web Interface

- **Dashboard (`/` or `/index`)**: Displays temperature gauge, humidity gauge (if available), printer status, and camera card.
- **Update Page (`/update`)**: Form for uploading firmware or filesystem updates.
- **Static Files**: CSS and JS served from SPIFFS.

## MQTT Broker

- **Sensor Topic**: `mqtt/sensor`
   - Payload: `{"temperature": 25.00, "humidity": 60.00, "status": "READY"}` (humidity only when available)
- **Alert Topic**: `mqtt/alerts`
   - Payloads:
      - `{"alert": "temperature_low", "temperature": 18.50, "threshold": 20.0}`
      - `{"alert": "ready_to_print", "temperature": 21.20, "threshold": 20.0}`
      - `{"alert": "temperature_high", "temperature": 39.10, "threshold": 30.0}`
- **Fan Alert Topic**: `mqtt/alert`
   - Payload: `{"alert": "fan_on_temperature_increasing", "temperature": 66.20, "previous_temperature": 65.80, "fan": "on"}`
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
- `GET /script.js`: Serves JavaScript.
- `GET /update`: Serves the update form.
- `POST /update`: Handles file uploads for OTA.
- `GET /events`: SSE endpoint for real-time data.
- `GET /readings`: Returns the latest sensor readings as JSON.
- `GET /version`: Returns firmware version as JSON.
- `GET /settings`: Returns `settings.json`.
- `POST /settings`: Replaces `settings.json`.
- `GET /fan`: Returns current fan state as JSON boolean text:
   - Example: `{"fan_active": "true"}` or `{"fan_active": "false"}`
- `POST /fan`: Sets fan state.
   - Accepted `state` values: `on`, `off`
   - `state` may be sent as a query parameter or form parameter.
   - Example: `POST /fan?state=on`

Note about web payloads:
- Fan state is always available from `/fan`.
- In current firmware, the `fan` field in sensor JSON payloads (`/readings` and SSE `sensor_data`) is always present and reflects the current fan state.

## Known Issues

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