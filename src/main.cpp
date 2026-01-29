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

  Serial.println("Firmware version " + String(VERSION));

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

    String payload = sensor.getJSONData();
    WifiSetup::events.send(payload.c_str(), "sensor_data", millis());
    mqtt.publish("mqtt/sensor", payload.c_str());
    Serial.println(payload);
    mqtt.loop();
  }
  delay(50);

  // Broker loop if required by your library
  // broker.loop();  // depends on implementation
}
