#include "wifi_setup.h"
#include "sensor.h"
#include "mqtt_broker_wrapper.h"
#define BMP180


Sensor sensor;
LocalMqttBroker mqtt;

unsigned long lastPublish = 0;
const unsigned long publishIntervalMs = 2000;

void setup() {
  Serial.begin(115200);

  WifiSetup::connect();
  WifiSetup::initWebServer();

  if (!sensor.begin()) {
    Serial.println("Sesnor Initialisationt failed");
    // while (true) delay(1000);
  }

  mqtt.begin();
}

void loop() {
  unsigned long now = millis();
  if (now - lastPublish >= publishIntervalMs) {
    lastPublish = now;

    float t, h;
    if (sensor.read(t,h)) {
      String payload = String(F("{\"temperature\": ")) + String(t,2) + F(", \"humidity\": ") + String(h,2) + F("}");

      if(sensor.sensorType == USE_BMP180) {
        mqtt.publish("sensors/bmp180", payload.c_str());
        WifiSetup::events.send(payload.c_str(), "sensor_bmp180", millis());
      } else if(sensor.sensorType == USE_BME280) {
        mqtt.publish("sensors/bme280", payload.c_str());
        WifiSetup::events.send(payload.c_str(), "sensor_bme280", millis());
      } else {
        mqtt.publish("sensors/debug", payload.c_str());
        WifiSetup::events.send(payload.c_str(), "sensor_debug", millis());
      }
      Serial.println(payload);
    }
  }

  // Broker loop if required by your library
  // broker.loop();  // depends on implementation
}
