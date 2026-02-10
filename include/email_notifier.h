#pragma once

#include <Arduino.h>
#include <ESP_Mail_Client.h>
#include <time.h>
#include "settings.h"

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

    SMTPSession smtp;
    Session_Config config;

    config.server.host_name = smtpHost;
    config.server.port = settings.getSmtpPort();
    config.secure.mode = settings.isSmtpSecure() ? esp_mail_secure_mode_ssl_tls : esp_mail_secure_mode_nonsecure;
    config.secure.startTLS = (settings.getSmtpPort() == 587) || (settings.getSmtpPort() == 25);
    config.login.email = smtpUser;
    config.login.password = smtpPassword;
    config.login.user_domain = "";

    SMTP_Message message;
    message.sender.name = senderName;
    message.sender.email = senderEmail;
    message.subject = "Temperature Monitor Status";
    message.addRecipient("Recipient", recipient);

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

    message.text.content = body;
    message.text.charSet = "us-ascii";
    message.text.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

    if (!smtp.connect(&config)) {
        Serial.println("SMTP connection failed.");
        return false;
    }

    if (!MailClient.sendMail(&smtp, &message)) {
        Serial.println("Failed to send email.");
        return false;
    }

    smtp.closeSession();
    Serial.println("Email sent successfully.");

    return true;
    }
};
