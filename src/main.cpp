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
    static int debug_count = 0;
    if (sensor.read(t,h)) {
      String payload = String(F("{\"temperature\": ")) + String(t,2);
      if (sensor.sensorType == USE_BME280) {
        payload += String(F(", \"humidity\": ")) + String(h,2);
      } else if(sensor.sensorType == USE_DEBUG) {
        debug_count++;
        // Send humidity every 5th reading for debug sensor
        if (debug_count <= 10) {
          payload += String(F(", \"humidity\": ")) + String(h,2);
        } else if(debug_count > 20) {
          debug_count = 0;
        }
      }
      payload += F(" }");
      WifiSetup::events.send(payload.c_str(), "sensor_data", millis());

      if(sensor.sensorType == USE_BMP180) {
        mqtt.publish("sensors/bmp180", payload.c_str());
      } else if(sensor.sensorType == USE_BME280) {
        mqtt.publish("sensors/bme280", payload.c_str());
      } else {
        mqtt.publish("sensors/debug", payload.c_str());
      }
      Serial.println(payload);
    }
  }

  // Broker loop if required by your library
  // broker.loop();  // depends on implementation
}
