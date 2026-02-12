#include "wifi_setup.h"
#include "sensor.h"
#include "mqtt_broker_wrapper.h"
#include "lcd_display.h"
#include "settings.h"
#include "email_notifier.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#define BMP180


Sensor sensor;
LocalMqttBroker mqtt;
LCDDisplay lcd;  // I2C address 0x27 by default
Settings settings;

struct EmailRequest {
  float temperature;
  char status[16];
};

QueueHandle_t emailQueue = nullptr;
TaskHandle_t emailTaskHandle = nullptr;

unsigned long lastPublish = 0;
const unsigned long publishIntervalMs = 2000;
unsigned long lastTimeUpdate = 0;
const unsigned long timeUpdateIntervalMs = 1000;  // Update time every 1 second
unsigned long lastEmailSend = 0;
bool sensorInitialized = false;
bool temperatureAboveThreshold = false;  // Track alert state for ready_to_print
bool temperatureAboveHighThreshold = false;  // Track alert state for temperature_high
bool temperatureBelowThreshold = false;  // Track alert state for temperature_low
float lastTemperature = 0.0f;
bool lastReadingValid = false;
String lastStatus = "UNKNOWN";
enum PrinterStatus{
  NOT_READY,
  READY,
  TOO_HOT
} printerStatus;
bool printerStatusChanged = false;

void emailTask(void* param) {
  EmailRequest request;
  for (;;) {
    if (xQueueReceive(emailQueue, &request, portMAX_DELAY) == pdTRUE) {
      if (!EmailNotifier::sendStatusEmail(settings, request.temperature, request.status)) {
        Serial.println("Email send attempt failed.");
      }
    }
  }
}

bool enqueueEmail(float temperature, const char* status) {
  if (!emailQueue) {
    return false;
  }

  EmailRequest request;
  request.temperature = temperature;
  strncpy(request.status, status, sizeof(request.status) - 1);
  request.status[sizeof(request.status) - 1] = '\0';

  return xQueueSend(emailQueue, &request, 0) == pdTRUE;
}

void setup() {
  Serial.begin(115200);

  Serial.println("Firmware version " + String(VERSION));

  // Initialize LCD display
  if (!lcd.begin(VERSION)) {
    Serial.println("LCD Initialization failed");
  }

  WifiSetup::connect();
  if (!settings.begin()) {
    Serial.println("Settings initialization failed");
  }

  WifiSetup::syncTimeWithNtp();

  WifiSetup::initWebServer();

  if (!sensor.begin()) {
    Serial.println("Sensor Initialization failed");
    lcd.displayError("Sensor Init Failed");
    // while (true) delay(1000);
  } else {
    sensorInitialized = true;
    // Display humidity status once at startup
    bool isBME280 = (sensor.sensorType == USE_BME280);
    lcd.displayHumidityStatus(isBME280);
  }

  printerStatus = NOT_READY;
  mqtt.begin();

  emailQueue = xQueueCreate(6, sizeof(EmailRequest));
  if (emailQueue) {
    xTaskCreatePinnedToCore(emailTask, "EmailTask", 8192, nullptr, 1, &emailTaskHandle, 0);
  } else {
    Serial.println("Failed to create email queue.");
  }
}

void loop() {
  unsigned long now = millis();
  bool uploading = WifiSetup::isUploading();
  
  if (now - lastPublish >= publishIntervalMs) {
    lastPublish = now;

    float temperature, humidity;
    if (sensor.read(temperature, humidity)) {

      String payload = sensor.getJSONData(temperature, humidity);
      
      // Add printer status to payload for web interface
      String statusStr = "NOT READY";
      if (printerStatus == READY) statusStr = "READY";
      else if (printerStatus == TOO_HOT) statusStr = "TOO HOT";
      
      // Create enhanced payload with status
      String webPayload = payload;
      webPayload.remove(webPayload.length() - 2); // Remove closing brace and space
      webPayload += String(F(", \"status\": \"")) + statusStr + F("\" }");
      
      WifiSetup::events.send(webPayload.c_str(), "sensor_data", millis());
      mqtt.publish("mqtt/sensor", payload.c_str());
      Serial.println(webPayload);

      lastTemperature = temperature;
      lastReadingValid = true;
      lastStatus = statusStr;

      float readyThreshold = settings.getReadyToPrintThreshold();
      float highThreshold = settings.getHighTemperatureThreshold();

      // Check for temperature threshold not reached (temperature_low)
      if (temperature < readyThreshold && !temperatureBelowThreshold) {
        // Temperature just fell below threshold
        temperatureBelowThreshold = true;
        String alertPayload = String(F("{\"alert\": \"temperature_low\", \"temperature\": ")) + 
                            String(temperature, 2) + F(", \"threshold\": ") + 
                            String(readyThreshold, 1) + F("}");
        mqtt.publish("mqtt/alerts", alertPayload.c_str());
        Serial.print("Alert Published: ");
        Serial.println(alertPayload);
        printerStatus = NOT_READY;
        if (!enqueueEmail(lastTemperature, "NOT READY")) {
          Serial.println("Email enqueue failed.");
        }

        printerStatusChanged = true;
      } else if (temperature >= readyThreshold && temperatureBelowThreshold) {
        // Temperature rose back above threshold - reset flag
        temperatureBelowThreshold = false;
      }

      // Check for temperature threshold crossing (ready_to_print)
      if (temperature >= readyThreshold && temperature < highThreshold && (!temperatureAboveThreshold || temperatureAboveHighThreshold)) {
        // Temperature just crossed above threshold
        temperatureAboveThreshold = true;
        String alertPayload = String(F("{\"alert\": \"ready_to_print\", \"temperature\": ")) + 
                            String(temperature, 2) + F(", \"threshold\": ") + 
                            String(readyThreshold, 1) + F("}");
        mqtt.publish("mqtt/alerts", alertPayload.c_str());
        Serial.print("Alert Published: ");
        Serial.println(alertPayload);
        printerStatus = READY;
        if (!enqueueEmail(lastTemperature, "READY")) {
          Serial.println("Email enqueue failed.");
        }
        printerStatusChanged = true;
      } else if (temperature < readyThreshold && temperatureAboveThreshold) {
        // Temperature fell back below threshold - reset flag
        temperatureAboveThreshold = false;
      }
      
      // Check for high temperature threshold crossing (temperature_high)
      if (temperature >= highThreshold && !temperatureAboveHighThreshold) {
        // Temperature just crossed above high threshold
        temperatureAboveHighThreshold = true;
        String alertPayload = String(F("{\"alert\": \"temperature_high\", \"temperature\": ")) + 
                            String(temperature, 2) + F(", \"threshold\": ") + 
                            String(highThreshold, 1) + F("}");
        mqtt.publish("mqtt/alerts", alertPayload.c_str());
        Serial.print("High Temp Alert Published: ");
        Serial.println(alertPayload);
        printerStatus = TOO_HOT;
        if (!enqueueEmail(lastTemperature, "TOO HOT")) {
          Serial.println("Email enqueue failed.");
        }
        printerStatusChanged = true;
      } else if (temperature < highThreshold && temperatureAboveHighThreshold) {
        // Temperature fell back below high threshold - reset flag
        temperatureAboveHighThreshold = false;
      }

      // Update LCD display if not uploading
      if (!uploading) {
        // Display IP address once at first publish
        String ipAddr = WiFi.localIP().toString();
        lcd.displayIPAddress(ipAddr.c_str());
        
        // Update sensor readings
        if (sensorInitialized) {
            bool isBME280 = (sensor.sensorType == USE_BME280);
            lcd.updateTemperature(temperature);
            lcd.updateHumidity(humidity, isBME280);
            if (printerStatusChanged){
              printerStatusChanged = false;
              if (printerStatus == READY) {
                lcd.updateStatus("READY");  // Refresh to show ready status
              } else if (printerStatus == NOT_READY) {
                lcd.updateStatus("NOT READY");  // Refresh to show ready status
              } else if (printerStatus == TOO_HOT) {
                lcd.updateStatus("TOO HOT");  // Refresh to show ready status
              }
            }
            
        }
      }
    } else {
      lcd.displayError("Sensor Error");
    }
    mqtt.loop();
  }

  if (now - lastEmailSend >= settings.getEmailIntervalMs()) {
    lastEmailSend = now;
    if (lastReadingValid && !mqtt.hasSensorSubscribers() && WiFi.status() == WL_CONNECTED) {
      Serial.println("Attempting to send email notification...");
      if (!enqueueEmail(lastTemperature, lastStatus.c_str())) {
        Serial.println("Email enqueue failed.");
      }
    }
  }

  // Update time every second
  if (!uploading && now - lastTimeUpdate >= timeUpdateIntervalMs) {
    lastTimeUpdate = now;
    lcd.updateTime();
  }

  delay(50);

  // Broker loop if required by your library
  // broker.loop();  // depends on implementation
}

