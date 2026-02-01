#include "wifi_setup.h"
#include "sensor.h"
#include "mqtt_broker_wrapper.h"
#include "lcd_display.h"
#define BMP180


Sensor sensor;
LocalMqttBroker mqtt;
LCDDisplay lcd;  // I2C address 0x27 by default

unsigned long lastPublish = 0;
const unsigned long publishIntervalMs = 2000;
unsigned long lastTimeUpdate = 0;
const unsigned long timeUpdateIntervalMs = 1000;  // Update time every 1 second
bool sensorInitialized = false;

void setup() {
  Serial.begin(115200);

  Serial.println("Firmware version " + String(VERSION));

  // Initialize LCD display
  if (!lcd.begin()) {
    Serial.println("LCD Initialization failed");
  }

  WifiSetup::connect();
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

  mqtt.begin();
}

void loop() {
  unsigned long now = millis();
  bool uploading = WifiSetup::isUploading();
  
  if (now - lastPublish >= publishIntervalMs) {
    lastPublish = now;

    String payload = sensor.getJSONData();
    WifiSetup::events.send(payload.c_str(), "sensor_data", millis());
    mqtt.publish("mqtt/sensor", payload.c_str());
    Serial.println(payload);
    
    if (!uploading) {
      // Display IP address once at first publish
      String ipAddr = WiFi.localIP().toString();
      lcd.displayIPAddress(ipAddr.c_str());
      
      // Update sensor readings
      if (sensorInitialized) {
        float temperature, humidity;
        if (sensor.read(temperature, humidity)) {
          bool isBME280 = (sensor.sensorType == USE_BME280);
          lcd.updateTemperature(temperature);
          lcd.updateHumidity(humidity, isBME280);
        } else {
          lcd.displayError("Sensor Error");
        }
      }
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

