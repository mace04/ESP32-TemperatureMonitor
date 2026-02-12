#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>

#define DEFAULT_READY_THRESHOLD 20.0f
#define DEFAULT_HIGH_THRESHOLD 30.0f
#define DEFAULT_CAMERA_URL "http://192.168.0.18:8080/?action=stream"
#define DEFAULT_EMAIL_ENABLED false
#define DEFAULT_EMAIL_INTERVAL_MINUTES 15
#define DEFAULT_SMTP_HOST "smtp.gmail.com"
#define DEFAULT_SMTP_PORT 587
#define DEFAULT_SMTP_SECURE true
#define DEFAULT_SMTP_USER "mace04021973@gmail.com"
#define DEFAULT_SMTP_PASSWORD "spnufbgdszjcryay"
#define DEFAULT_EMAIL_SENDER "sensors@gmail.com"
#define DEFAULT_EMAIL_SENDER_NAME "ESP32 Temp Monitor"
#define DEFAULT_EMAIL_RECIPIENT "michael@m-software.co.uk"

class Settings {
public:
    bool save() const {
        File file = SPIFFS.open(filePath, "w");
        if (!file) {
            Serial.println("Failed to open settings file for writing");
            return false;
        }

        StaticJsonDocument<768> doc;
        doc["ready_to_print_threshold"] = readyToPrintThreshold;
        doc["temperature_high_threshold"] = highTemperatureThreshold;
        doc["camera_url"] = cameraUrl;
        doc["email_enabled"] = emailEnabled;
        doc["email_interval_minutes"] = emailIntervalMinutes;
        doc["smtp_host"] = smtpHost;
        doc["smtp_port"] = smtpPort;
        doc["smtp_secure"] = smtpSecure;
        doc["smtp_user"] = smtpUser;
        doc["smtp_password"] = smtpPassword;
        doc["email_sender"] = emailSender;
        doc["email_sender_name"] = emailSenderName;
        doc["email_recipient"] = emailRecipient;

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

        StaticJsonDocument<768> doc;
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
        emailEnabled = doc["email_enabled"] | DEFAULT_EMAIL_ENABLED;
        emailIntervalMinutes = doc["email_interval_minutes"] | DEFAULT_EMAIL_INTERVAL_MINUTES;
        smtpHost = doc["smtp_host"] | DEFAULT_SMTP_HOST;
        smtpPort = doc["smtp_port"] | DEFAULT_SMTP_PORT;
        smtpSecure = doc["smtp_secure"] | DEFAULT_SMTP_SECURE;
        smtpUser = doc["smtp_user"] | DEFAULT_SMTP_USER;
        smtpPassword = doc["smtp_password"] | DEFAULT_SMTP_PASSWORD;
        emailSender = doc["email_sender"] | DEFAULT_EMAIL_SENDER;
        emailSenderName = doc["email_sender_name"] | DEFAULT_EMAIL_SENDER_NAME;
        emailRecipient = doc["email_recipient"] | DEFAULT_EMAIL_RECIPIENT;
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
    bool isEmailEnabled() const { return emailEnabled; }
    uint16_t getEmailIntervalMinutes() const { return emailIntervalMinutes; }
    unsigned long getEmailIntervalMs() const {
        uint16_t minutes = emailIntervalMinutes;
        if (minutes == 0) {
            minutes = DEFAULT_EMAIL_INTERVAL_MINUTES;
        }
        return static_cast<unsigned long>(minutes) * 60UL * 1000UL;
    }
    const char* getSmtpHost() const { return smtpHost.c_str(); }
    uint16_t getSmtpPort() const { return smtpPort; }
    bool isSmtpSecure() const { return smtpSecure; }
    const char* getSmtpUser() const { return smtpUser.c_str(); }
    const char* getSmtpPassword() const { return smtpPassword.c_str(); }
    const char* getEmailSender() const { return emailSender.c_str(); }
    const char* getEmailSenderName() const { return emailSenderName.c_str(); }
    const char* getEmailRecipient() const { return emailRecipient.c_str(); }

    String toJson() const {
        StaticJsonDocument<768> doc;
        doc["ready_to_print_threshold"] = readyToPrintThreshold;
        doc["temperature_high_threshold"] = highTemperatureThreshold;
        doc["camera_url"] = cameraUrl;
        doc["email_enabled"] = emailEnabled;
        doc["email_interval_minutes"] = emailIntervalMinutes;
        doc["smtp_host"] = smtpHost;
        doc["smtp_port"] = smtpPort;
        doc["smtp_secure"] = smtpSecure;
        doc["smtp_user"] = smtpUser;
        doc["smtp_password"] = smtpPassword;
        doc["email_sender"] = emailSender;
        doc["email_sender_name"] = emailSenderName;
        doc["email_recipient"] = emailRecipient;

        String json;
        serializeJson(doc, json);
        return json;
    }

    void setReadyToPrintThreshold(float value) { readyToPrintThreshold = value; }
    void setHighTemperatureThreshold(float value) { highTemperatureThreshold = value; }
    void setCameraUrl(const String& value) { cameraUrl = value; }
    void setEmailEnabled(bool value) { emailEnabled = value; }
    void setEmailIntervalMinutes(uint16_t value) { emailIntervalMinutes = value; }
    void setSmtpHost(const String& value) { smtpHost = value; }
    void setSmtpPort(uint16_t value) { smtpPort = value; }
    void setSmtpSecure(bool value) { smtpSecure = value; }
    void setSmtpUser(const String& value) { smtpUser = value; }
    void setSmtpPassword(const String& value) { smtpPassword = value; }
    void setEmailSender(const String& value) { emailSender = value; }
    void setEmailSenderName(const String& value) { emailSenderName = value; }
    void setEmailRecipient(const String& value) { emailRecipient = value; }
private:
    float readyToPrintThreshold = DEFAULT_READY_THRESHOLD;
    float highTemperatureThreshold = DEFAULT_HIGH_THRESHOLD;
    String cameraUrl = DEFAULT_CAMERA_URL;
    bool emailEnabled = DEFAULT_EMAIL_ENABLED;
    uint16_t emailIntervalMinutes = DEFAULT_EMAIL_INTERVAL_MINUTES;
    String smtpHost = DEFAULT_SMTP_HOST;
    uint16_t smtpPort = DEFAULT_SMTP_PORT;
    bool smtpSecure = DEFAULT_SMTP_SECURE;
    String smtpUser = DEFAULT_SMTP_USER;
    String smtpPassword = DEFAULT_SMTP_PASSWORD;
    String emailSender = DEFAULT_EMAIL_SENDER;
    String emailSenderName = DEFAULT_EMAIL_SENDER_NAME;
    String emailRecipient = DEFAULT_EMAIL_RECIPIENT;
    const char* filePath = "/settings.json";
};