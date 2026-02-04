#include "wifi_setup.h"
#include "sensor.h"
#include "mqtt_broker_wrapper.h"
#include "lcd_display.h"
#include "settings.h"
#define BMP180


Sensor sensor;
LocalMqttBroker mqtt;
LCDDisplay lcd;  // I2C address 0x27 by default
Settings settings;

unsigned long lastPublish = 0;
const unsigned long publishIntervalMs = 2000;
unsigned long lastTimeUpdate = 0;
const unsigned long timeUpdateIntervalMs = 1000;  // Update time every 1 second
bool sensorInitialized = false;
bool temperatureAboveThreshold = false;  // Track alert state for ready_to_print
bool temperatureAboveHighThreshold = false;  // Track alert state for temperature_high
bool temperatureBelowThreshold = false;  // Track alert state for temperature_low
enum PrinterStatus{
  NOT_READY,
  READY,
  TOO_HOT
} printerStatus;
bool printerStatusChanged = false;

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

  WifiSetup::initWebServer();

  if (!sensor.begin()) {
    Serial.println("Sesnor Initialisationt failed");
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
}

void loop() {
  unsigned long now = millis();
  bool uploading = WifiSetup::isUploading();
  
  if (now - lastPublish >= publishIntervalMs) {
    lastPublish = now;

    float temperature, humidity;
    if (sensor.read(temperature, humidity)) {

      String payload = sensor.getJSONData(temperature, humidity);
      WifiSetup::events.send(payload.c_str(), "sensor_data", millis());
      mqtt.publish("mqtt/sensor", payload.c_str());
      Serial.println(payload);

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

  // Update time every second
  if (!uploading && now - lastTimeUpdate >= timeUpdateIntervalMs) {
    lastTimeUpdate = now;
    lcd.updateTime();
  }

  delay(50);

  // Broker loop if required by your library
  // broker.loop();  // depends on implementation
}

