#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include <Update.h>
#include "sensor.h"
#include "lcd_display.h"
#include "settings.h"

extern Sensor sensor;
extern LCDDisplay lcd;
extern Settings settings;

namespace WifiSetup {
    const char* SSID = "SKYPGFYX";
    const char* PASSWORD = "HDXvtRKbPi8i";
    const char* HOSTNAME = "esp32-temp-mon";

    AsyncWebServer server(80);
    AsyncEventSource events("/events");
    static bool uploadInProgress = false;

    bool isUploading() {
        return uploadInProgress;
    }

    void handleGetUpdate(AsyncWebServerRequest *request) {
        File file = SPIFFS.open("/update.html", "r");
        if (!file) {
            request->send(404, "text/plain", "File not found");
            return;
        }

        String html = file.readString();
        file.close();

        request->send(200, "text/html", html);
    }

    void handlePostUpdate(AsyncWebServerRequest *request) {
        String response = (Update.hasError()) ? "Update Failed" : "Update Successful. Rebooting...";
        request->send(200, "text/plain", response);
        delay(3000);
        ESP.restart();
    }

    void handlePostUpload(AsyncWebServerRequest *request, const String& filename, size_t index, 
                        uint8_t *data, size_t len, bool final) {
        static uint32_t updateSize = 0;
        static String uploadType = "";

        if (index == 0) {
            Serial.printf("Upload Start: %s\n", filename.c_str());
            updateSize = 0;
            uploadInProgress = true;

            // Determine upload type from HTTP argument
            if (request->hasArg("uploadType")) {
                uploadType = request->arg("uploadType");
            } else {
                uploadType = "firmware"; // default to firmware
            }

            Serial.printf("Upload Type: %s\n", uploadType.c_str());

            // Handle firmware update
            if (uploadType == "firmware") {
                lcd.displayUploadMessage("Uploading firmware");
                if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) {
                    Update.printError(Serial);
                    uploadInProgress = false;
                    request->send(500, "text/plain", "OTA begin failed");
                    return;
                }
                Serial.println("Started firmware update...");
            }
            // Handle filesystem upload
            else if (uploadType == "filesystem") {
                lcd.displayUploadMessage("Uploading filesystem");
                // Backup settings.json if the upload type is filesystem
                if (uploadType == "filesystem") {
                    if (SPIFFS.exists("/settings.json")) {
                        // settings.loadSettings();
                        // Serial.println("settings.json backed up successfully.");
                    }
                }            
                if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_SPIFFS)) {
                    Update.printError(Serial);
                    uploadInProgress = false;
                    request->send(500, "text/plain", "OTA begin failed");
                    return;
                }
                Serial.println("Started filesystem update...");
            }
            else {
                uploadInProgress = false;
                request->send(400, "text/plain", "Invalid uploadType parameter. Use 'firmware' or 'filesystem'");
                return;
            }
        }

        if (!Update.hasError()) {
            if (Update.write(data, len) != len) {
                Update.printError(Serial);
            }
            updateSize += len;
        }

        if (final) {
            if (Update.end(true)) {
                Serial.printf("Update Success: %u bytes\nRebooting...\n", updateSize);

                // Restore settings.json if the upload type is filesystem
                if (uploadType == "filesystem") {
                    // settings.saveSettings();
                    // Serial.println("settings.json restored successfully.");
                }
                // request->send(200, "text/html",
                //     "<html><body><p>Firmware update successful!</p>"
                //     "<p>Rebooting ESP32...</p>"
                //     "<meta http-equiv='refresh' content='3; url=/' />"
                //     "</body></html>");
                delay(3000);
                ESP.restart();
            } else {
                Update.printError(Serial);
                uploadInProgress = false;
                request->send(500, "text/plain", "OTA end failed");
            }
        }
    }

    void connect() {
        WiFi.mode(WIFI_STA);
        WiFi.setHostname(HOSTNAME);
        WiFi.begin(SSID, PASSWORD);
        Serial.print("Connecting to WiFi");
        for (int i = 0; i < 10 && WiFi.status() != WL_CONNECTED; i++) {
            delay(500);
            Serial.print(".");
        }

        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("\nWiFi connection failed. Starting hotspot...");
            WiFi.softAP("ESP32_Hotspot", "12345678");
            Serial.println("Hotspot started. IP address: " + WiFi.softAPIP().toString());
        } else {
            Serial.println("\nWiFi connected. IP address: " + WiFi.localIP().toString());
        }
    }

    void initWebServer() {
        if (!SPIFFS.begin(true)) {
            Serial.println("SPIFFS Mount Failed");
            return;
        }
    // Serve root
        server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/index.html", "text/html");
        });

        server.on("/index", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/index.html", "text/html");
        });

            // Serve style.css for GET /style.css
        server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/style.css", "text/css");
        });

        // Serve common.js for GET /index.js
        server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request) {
            request->send(SPIFFS, "/script.js", "application/javascript");
        });

        // Serve update.html for GET /update
        server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
            WifiSetup::handleGetUpdate(request);
        });

        // Handle POST /update for OTA firmware update
        server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
            WifiSetup::handlePostUpdate(request);
        }, [](AsyncWebServerRequest *request, const String& filename, size_t index, uint8_t *data, size_t len, bool final) {
            WifiSetup::handlePostUpload(request, filename, index, data, len, final);
        });

        // Settings file GET
        server.on("/settings", HTTP_GET, [](AsyncWebServerRequest *request) {
            if (!SPIFFS.exists("/settings.json")) {
                settings.save();
            }
            request->send(SPIFFS, "/settings.json", "application/json");
        });

        // Settings file POST (replace existing file)
        server.on(
            "/settings",
            HTTP_POST,
            [](AsyncWebServerRequest *request) {
                request->send(200, "application/json", "{\"status\": \"ok\"}");
            },
            nullptr,
            [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
                static File file;
                if (index == 0) {
                    file = SPIFFS.open("/settings.json", "w");
                    if (!file) {
                        request->send(500, "application/json", "{\"error\": \"Failed to open settings file\"}");
                        return;
                    }
                }

                if (file) {
                    file.write(data, len);
                }

                if (index + len == total) {
                    if (file) {
                        file.close();
                    }
                    settings.load();
                }
            }
        );

        // Request for the latest sensor readings
        server.on("/readings", HTTP_GET, [](AsyncWebServerRequest *request){
            String json = sensor.getJSONData ();
            request->send(200, "application/json", json);
            json = String();
        });

        // Serve firmware version
        server.on("/version", HTTP_GET, [](AsyncWebServerRequest *request){
             
            request->send(200, "text/json", "{\"version\":\"" + String(VERSION) +  "\"}");
        });

        server.addHandler(&events);

        server.begin();
    }    
}
