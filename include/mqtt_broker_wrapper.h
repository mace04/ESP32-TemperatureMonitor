#pragma once
#include "PicoMQTT.h"   // From your chosen broker library

class LocalMqttBroker {
public:
  void begin() {
    broker.begin();
  }

  void publish(const char* topic, const char* payload) {
    broker.publish(topic, payload);
  }

  void loop() {
    broker.loop();
  }

private:
  PicoMQTT::Server broker;
};
