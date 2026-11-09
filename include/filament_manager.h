#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <vector>

constexpr uint8_t FILAMENT_BUTTON_PIN = 13;
constexpr unsigned long BUTTON_DEBOUNCE_MS = 50;

struct FilamentProfile {
    String name;
    float lowest_temperature;
    float highest_temperature;
    float optimum_temperature;
};

class FilamentManager {
public:
    // Call after SPIFFS is mounted (i.e. after Settings::begin())
    bool begin() {
        pinMode(FILAMENT_BUTTON_PIN, INPUT_PULLUP);
        return loadProfiles();
    }

    // Call every loop iteration to handle button debouncing and cyclic selection
    void update() {
        bool pressed = (digitalRead(FILAMENT_BUTTON_PIN) == LOW);
        unsigned long now = millis();

        if (pressed && !_lastButtonState) {
            _debounceStart = now;
        } else if (pressed && _lastButtonState && !_buttonHandled
                   && (now - _debounceStart >= BUTTON_DEBOUNCE_MS)) {
            cycleFilament();
            _buttonHandled = true;
        }

        if (!pressed) {
            _buttonHandled = false;
        }

        _lastButtonState = pressed;
    }

    // Returns true once when the selected filament has changed, then resets the flag
    bool consumeChange() {
        if (_changed) {
            _changed = false;
            return true;
        }
        return false;
    }

    // Ready to print: temperature is over lowest_temperature and below highest_temperature
    bool isReadyToPrint(float temperature) const {
        if (_profiles.empty()) return false;
        const FilamentProfile& f = current();
        return temperature > f.lowest_temperature && temperature < f.highest_temperature;
    }

    // Too hot: temperature is over highest_temperature
    bool isTooHot(float temperature) const {
        if (_profiles.empty()) return false;
        return temperature > current().highest_temperature;
    }

    // Fan ON: temperature is above optimum + 0.5
    bool shouldFanTurnOn(float temperature) const {
        if (_profiles.empty()) return false;
        return temperature > (current().optimum_temperature + 0.5f);
    }

    // Fan OFF: temperature is below optimum - 0.5
    bool shouldFanTurnOff(float temperature) const {
        if (_profiles.empty()) return false;
        return temperature < (current().optimum_temperature - 0.5f);
    }

    const FilamentProfile& current() const {
        return _profiles[_currentIndex];
    }

    const String& currentName() const {
        static const String unknown = "UNKNOWN";
        return _profiles.empty() ? unknown : _profiles[_currentIndex].name;
    }

    size_t profileCount() const { return _profiles.size(); }

    // Advance to the next filament profile (simulates a button press)
    void cycle() {
        cycleFilament();
    }

private:
    std::vector<FilamentProfile> _profiles;
    size_t _currentIndex = 0;
    bool _lastButtonState = false;
    bool _buttonHandled = false;
    bool _changed = false;
    unsigned long _debounceStart = 0;

    void cycleFilament() {
        if (_profiles.empty()) return;
        _currentIndex = (_currentIndex + 1) % _profiles.size();
        _changed = true;
        Serial.printf("Filament selected: %s (index %d)\n",
                      _profiles[_currentIndex].name.c_str(), (int)_currentIndex);
    }

    bool loadProfiles() {
        const char* path = "/filament_settings.json";
        if (!SPIFFS.exists(path)) {
            Serial.println("filament_settings.json not found in SPIFFS");
            return false;
        }

        File file = SPIFFS.open(path, "r");
        if (!file) {
            Serial.println("Failed to open filament_settings.json");
            return false;
        }

        DynamicJsonDocument doc(1024);
        DeserializationError err = deserializeJson(doc, file);
        file.close();

        if (err) {
            Serial.printf("Failed to parse filament_settings.json: %s\n", err.c_str());
            return false;
        }

        _profiles.clear();
        for (JsonObject obj : doc.as<JsonArray>()) {
            FilamentProfile p;
            p.name                = obj["name"]                | "UNKNOWN";
            p.lowest_temperature  = obj["lowest_temperature"]  | 0.0f;
            p.highest_temperature = obj["highest_temperature"] | 100.0f;
            p.optimum_temperature = obj["optimum_temperature"] | 25.0f;
            _profiles.push_back(p);
        }

        Serial.printf("Loaded %d filament profile(s). Active: %s\n",
                      (int)_profiles.size(),
                      _profiles.empty() ? "none" : _profiles[0].name.c_str());
        return !_profiles.empty();
    }
};
