#pragma once
#include <vector>
#include "PicoMQTT.h"   // From your chosen broker library

class TrackingMqttBroker : public PicoMQTT::Server {
public:
  using PicoMQTT::Server::Server;

  size_t sensorSubscriberCount() const {
    return sensorSubscribers.size();
  }

  bool hasSensorSubscribers() const {
    return !sensorSubscribers.empty();
  }

protected:
  void on_subscribe(const char* client_id, const char* topic) override {
    if (matchesSensorTopic(topic)) {
      updateClientSubscription(client_id, 1);
    }
  }

  void on_unsubscribe(const char* client_id, const char* topic) override {
    if (matchesSensorTopic(topic)) {
      updateClientSubscription(client_id, -1);
    }
  }

  void on_disconnected(const char* client_id) override {
    removeClient(client_id);
  }

private:
  struct ClientSubscription {
    String clientId;
    uint16_t matchCount;
  };

  std::vector<ClientSubscription> sensorSubscribers;

  bool matchesSensorTopic(const char* topic_filter) const {
    return PicoMQTT::Subscriber::topic_matches(topic_filter, "mqtt/sensor");
  }

  int findClientIndex(const char* client_id) const {
    for (size_t i = 0; i < sensorSubscribers.size(); ++i) {
      if (sensorSubscribers[i].clientId == client_id) {
        return static_cast<int>(i);
      }
    }
    return -1;
  }

  void updateClientSubscription(const char* client_id, int delta) {
    int index = findClientIndex(client_id);
    if (index < 0) {
      if (delta > 0) {
        sensorSubscribers.push_back({String(client_id), static_cast<uint16_t>(delta)});
      }
      return;
    }

    int newCount = static_cast<int>(sensorSubscribers[index].matchCount) + delta;
    if (newCount <= 0) {
      sensorSubscribers.erase(sensorSubscribers.begin() + index);
    } else {
      sensorSubscribers[index].matchCount = static_cast<uint16_t>(newCount);
    }
  }

  void removeClient(const char* client_id) {
    int index = findClientIndex(client_id);
    if (index >= 0) {
      sensorSubscribers.erase(sensorSubscribers.begin() + index);
    }
  }
};

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

  bool hasSensorSubscribers() const {
    return broker.hasSensorSubscribers();
  }

private:
  TrackingMqttBroker broker;
};
