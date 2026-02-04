#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

#define DEFAULT_READY_THRESHOLD 20.0f
#define DEFAULT_HIGH_THRESHOLD 30.0f
#define DEFAULT_CAMERA_URL "http://192.168.0.18:8080/?action=stream"

class Settings {
public:
    bool save() const {
        File file = SPIFFS.open(filePath, "w");
        if (!file) {
            Serial.println("Failed to open settings file for writing");
            return false;
        }

        StaticJsonDocument<256> doc;
        doc["ready_to_print_threshold"] = readyToPrintThreshold;
        doc["temperature_high_threshold"] = highTemperatureThreshold;
        doc["camera_url"] = cameraUrl;

        if (serializeJson(doc, file) == 0) {
            Serial.println("Failed to write settings file");
            file.close();
            return false;
        }

        file.close();
        return true;
    }

    bool load() {
        if (!SPIFFS.exists(filePath)) {
            return save();
        }

        File file = SPIFFS.open(filePath, "r");
        if (!file) {
            Serial.println("Failed to open settings file for reading");
            return false;
        }

        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, file);
        file.close();

        if (error) {
            Serial.println("Failed to parse settings file, restoring defaults");
            readyToPrintThreshold = DEFAULT_READY_THRESHOLD;
            highTemperatureThreshold = DEFAULT_HIGH_THRESHOLD;
            return save();
        }

        readyToPrintThreshold = doc["ready_to_print_threshold"] | DEFAULT_READY_THRESHOLD;
        highTemperatureThreshold = doc["temperature_high_threshold"] | DEFAULT_HIGH_THRESHOLD;
        cameraUrl = doc["camera_url"] | DEFAULT_CAMERA_URL;
        return true;
    }  

    bool begin() {
        if (!SPIFFS.begin(true)) {
            Serial.println("SPIFFS Mount Failed");
            return false;
        }
        return load();
    }
    float getReadyToPrintThreshold() const { return readyToPrintThreshold; }
    float getHighTemperatureThreshold() const { return highTemperatureThreshold; }
    const char* getCameraUrl() const { return cameraUrl.c_str(); }

    void setReadyToPrintThreshold(float value) { readyToPrintThreshold = value; }
    void setHighTemperatureThreshold(float value) { highTemperatureThreshold = value; }
    void setCameraUrl(const String& value) { cameraUrl = value; }
private:
    float readyToPrintThreshold = DEFAULT_READY_THRESHOLD;
    float highTemperatureThreshold = DEFAULT_HIGH_THRESHOLD;
    String cameraUrl = DEFAULT_CAMERA_URL;
    const char* filePath = "/settings.json";
};