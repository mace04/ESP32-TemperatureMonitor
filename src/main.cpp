#include "wifi_setup.h"
#include "sensor.h"
#include "mqtt_broker_wrapper.h"
#include "lcd_display.h"
#include "settings.h"
#include "email_notifier.h"
#include "filament_manager.h"
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#define BMP180


Sensor sensor;
LocalMqttBroker mqtt;
LCDDisplay lcd;  // I2C address 0x27 by default
Settings settings;
FilamentManager filamentManager;

struct EmailRequest {
  float temperature;
  char status[16];
};

QueueHandle_t emailQueue = nullptr;
TaskHandle_t emailTaskHandle = nullptr;

constexpr uint8_t FAN_PIN = 4;

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
volatile bool fanState = false;
String lastFilament = "";
float previousTemperatureForFanTrend = 0.0f;
bool hasPreviousTemperatureForFanTrend = false;
unsigned long lastFanIncreaseAlert = 0;
unsigned long filamentDisplayUntil = 0;
enum PrinterStatus{
  NOT_READY,
  READY,
  TOO_HOT
} printerStatus;
bool printerStatusChanged = false;

bool setFanState(bool on) {
  digitalWrite(FAN_PIN, on ? HIGH : LOW);
  fanState = on;
  Serial.printf("Fan state set to %s\n", on ? "ON" : "OFF");
  return true;
}

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

  pinMode(FAN_PIN, OUTPUT);
  setFanState(false);
  filamentManager.begin();

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
    lcd.displayHumidityStatus(isBME280, filamentManager.currentName().c_str());
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

  filamentManager.update();
  if (filamentManager.consumeChange()) {
    temperatureBelowThreshold = false;
    temperatureAboveThreshold = false;
    temperatureAboveHighThreshold = false;
    filamentDisplayUntil = now + 3000;
    Serial.printf("Filament changed to %s, resetting threshold states\n",
                  filamentManager.currentName().c_str());
  }

  if(!lastFilament.equals(filamentManager.currentName())) {
    lastFilament = filamentManager.currentName();
    lcd.displayHumidityStatus(false, filamentManager.currentName().c_str());
  }

  if (now - lastPublish >= publishIntervalMs) {
    lastPublish = now;

    float temperature, humidity;
    if (sensor.read(temperature, humidity)) {

      if (!fanState && filamentManager.shouldFanTurnOn(temperature)) {
        setFanState(true);
      } else if (fanState && filamentManager.shouldFanTurnOff(temperature)) {
        setFanState(false);
      }

      if (fanState && hasPreviousTemperatureForFanTrend && temperature > previousTemperatureForFanTrend) {
        unsigned long intervalMs = settings.getFanAlertIntervalMs();
        if (now - lastFanIncreaseAlert >= intervalMs) {
          String alertPayload = String(F("{\"alert\": \"fan_on_temperature_increasing\", \"temperature\": ")) +
                                String(temperature, 2) + F(", \"previous_temperature\": ") +
                                String(previousTemperatureForFanTrend, 2) + F(", \"fan\": \"on\"}");
          mqtt.publish("mqtt/alert", alertPayload.c_str());
          Serial.print("Fan Alert Published: ");
          Serial.println(alertPayload);
          lastFanIncreaseAlert = now;
        }
      }

      previousTemperatureForFanTrend = temperature;
      hasPreviousTemperatureForFanTrend = true;

      String payload = sensor.getJSONData(temperature, humidity, fanState);
      
      // Add printer status to payload for web interface
      String statusStr = "NOT READY";
      if (printerStatus == READY) statusStr = "READY";
      else if (printerStatus == TOO_HOT) statusStr = "TOO HOT";
      
      // Create enhanced payload with status
      String webPayload = payload;
      webPayload.remove(webPayload.length() - 2); // Remove closing brace and space
      webPayload += String(F(", \"status\": \"")) + statusStr +
            String(F("\", \"filament\": \"")) + filamentManager.currentName() +
            + F("\" }");
      
      WifiSetup::events.send(webPayload.c_str(), "sensor_data", millis());
      mqtt.publish("mqtt/sensor", payload.c_str());
      Serial.println(webPayload);

      lastTemperature = temperature;
      lastReadingValid = true;
      lastStatus = statusStr;

      // Check for temperature not ready (below lowest_temperature of selected filament)
      if (!filamentManager.isReadyToPrint(temperature) && !filamentManager.isTooHot(temperature) && !temperatureBelowThreshold) {
        temperatureBelowThreshold = true;
        String alertPayload = String(F("{\"alert\": \"temperature_low\", \"temperature\": ")) + 
                            String(temperature, 2) + F(", \"threshold\": ") + 
                            String(filamentManager.current().lowest_temperature, 1) + F("}");
        mqtt.publish("mqtt/alerts", alertPayload.c_str());
        Serial.print("Alert Published: ");
        Serial.println(alertPayload);
        printerStatus = NOT_READY;
        if (!enqueueEmail(lastTemperature, "NOT READY")) {
          Serial.println("Email enqueue failed.");
        }
        printerStatusChanged = true;
      } else if ((filamentManager.isReadyToPrint(temperature) || filamentManager.isTooHot(temperature)) && temperatureBelowThreshold) {
        temperatureBelowThreshold = false;
      }

      // Check for ready to print (over lowest_temperature and below highest_temperature)
      if (filamentManager.isReadyToPrint(temperature) && (!temperatureAboveThreshold || temperatureAboveHighThreshold)) {
        temperatureAboveThreshold = true;
        String alertPayload = String(F("{\"alert\": \"ready_to_print\", \"temperature\": ")) + 
                            String(temperature, 2) + F(", \"threshold\": ") + 
                            String(filamentManager.current().lowest_temperature, 1) + F("}");
        mqtt.publish("mqtt/alerts", alertPayload.c_str());
        Serial.print("Alert Published: ");
        Serial.println(alertPayload);
        printerStatus = READY;
        if (!enqueueEmail(lastTemperature, "READY")) {
          Serial.println("Email enqueue failed.");
        }
        printerStatusChanged = true;
      } else if (!filamentManager.isReadyToPrint(temperature) && temperatureAboveThreshold) {
        temperatureAboveThreshold = false;
      }

      // Check for too hot (over highest_temperature of selected filament)
      if (filamentManager.isTooHot(temperature) && !temperatureAboveHighThreshold) {
        temperatureAboveHighThreshold = true;
        String alertPayload = String(F("{\"alert\": \"temperature_high\", \"temperature\": ")) + 
                            String(temperature, 2) + F(", \"threshold\": ") + 
                            String(filamentManager.current().highest_temperature, 1) + F("}");
        mqtt.publish("mqtt/alerts", alertPayload.c_str());
        Serial.print("High Temp Alert Published: ");
        Serial.println(alertPayload);
        printerStatus = TOO_HOT;
        if (!enqueueEmail(lastTemperature, "TOO HOT")) {
          Serial.println("Email enqueue failed.");
        }
        printerStatusChanged = true;
      } else if (!filamentManager.isTooHot(temperature) && temperatureAboveHighThreshold) {
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

