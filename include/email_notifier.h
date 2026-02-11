#pragma once

#include <Arduino.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <ESP_SSLClient.h>
#define ENABLE_SMTP
#include <ReadyMail.h>
#include <time.h>
#include "settings.h"

static ESP_SSLClient* gStartTlsClient = nullptr;
static void startTlsCallback(bool &success) {
    success = gStartTlsClient && gStartTlsClient->connectSSL();
}

class EmailNotifier {
public:
  static bool sendStatusEmail(const Settings& settings, float temperature, const char* status){
    if (!settings.isEmailEnabled()) {
        return false;
    }

    const char* smtpHost = settings.getSmtpHost();
    const char* smtpUser = settings.getSmtpUser();
    const char* smtpPassword = settings.getSmtpPassword();
    const char* senderEmail = settings.getEmailSender();
    const char* senderName = settings.getEmailSenderName();
    const char* recipient = settings.getEmailRecipient();

    if (strlen(smtpHost) == 0 || strlen(smtpUser) == 0 || strlen(smtpPassword) == 0 ||
        strlen(senderEmail) == 0 || strlen(recipient) == 0) {
        Serial.println("Email settings are incomplete; skipping email send.");
        return false;
    }

    auto statusCallback = [](SMTPStatus status) {
        Serial.println(status.text);
    };

    SMTPMessage message;
    String fromHeader = String(senderName) + " <" + senderEmail + ">";
    String toHeader = String("Recipient <") + recipient + ">";
    message.headers.add(rfc822_from, fromHeader.c_str());
    message.headers.add(rfc822_to, toHeader.c_str());
    message.headers.add(rfc822_subject, "Temperature Monitor Status");

    char timeBuffer[32] = "unknown";
    time_t now;
    time(&now);
    if (now > 1609459200) {
        struct tm timeinfo;
        if (gmtime_r(&now, &timeinfo)) {
            strftime(timeBuffer, sizeof(timeBuffer), "%Y-%m-%d %H:%M:%S UTC", &timeinfo);
        }
    }

    char body[256];
    snprintf(body, sizeof(body),
            "Status: %s\nTemperature: %.2f C\nReading Time: %s",
            status, temperature, timeBuffer);

    message.text.body(body);
    message.timestamp = time(nullptr);

    const uint16_t smtpPort = settings.getSmtpPort();
    const bool useSecure = settings.isSmtpSecure();
    bool sent = false;

    if (useSecure) {
        if (smtpPort == 587) {
            WiFiClient basicClient;
            ESP_SSLClient sslClient;
            gStartTlsClient = &sslClient;
            sslClient.setClient(&basicClient, false);
            sslClient.setInsecure();
            SMTPClient smtp(sslClient, startTlsCallback, true);
            smtp.connect(smtpHost, smtpPort, statusCallback);
            if (smtp.isConnected()) {
                smtp.authenticate(smtpUser, smtpPassword, readymail_auth_password);
                sent = smtp.send(message);
            }
            gStartTlsClient = nullptr;
        } else {
            WiFiClientSecure sslClient;
            sslClient.setInsecure();
            SMTPClient smtp(sslClient);
            smtp.connect(smtpHost, smtpPort, statusCallback);
            if (smtp.isConnected()) {
                smtp.authenticate(smtpUser, smtpPassword, readymail_auth_password);
                sent = smtp.send(message);
            }
        }
    } else {
        WiFiClient plainClient;
        SMTPClient smtp(plainClient);
        smtp.connect(smtpHost, smtpPort, statusCallback, false);
        if (smtp.isConnected()) {
            smtp.authenticate(smtpUser, smtpPassword, readymail_auth_password);
            sent = smtp.send(message);
        }
    }

    if (!sent) {
        Serial.println("Failed to send email.");
        return false;
    }

    Serial.println("Email sent successfully.");

    return true;
    }
};
