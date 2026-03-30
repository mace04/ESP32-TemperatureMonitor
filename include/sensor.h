#pragma once
#include <Arduino.h>
#include <Adafruit_BMP085.h>
#include <Adafruit_BME280.h>
#include "settings.h"

extern Settings settings;

enum SensorType {
  USE_BMP180,
  USE_BME280,
  USE_DEBUG
};

class Sensor {
public:
  SensorType sensorType;
  bool begin() {
    if (bmp.begin()) {
      Serial.println("BMP085/BMP180 sensor found!");
      sensorType = USE_BMP180;
      return true;
    }
    if (bme.begin(0x76)) {
      Serial.println("BME280 sensor found at 0x76!");
      sensorType = USE_BME280;
      return true;
    }
    else{
      Serial.println("No sensor found!");
      sensorType = USE_DEBUG;
    }
    return false;
  }

  bool read(float& temperature, float& humidity) {
    if(sensorType == USE_BMP180) {
      temperature = bmp.readTemperature();
      humidity  = 0.0; // BMP180 does not provide humidity
    } else if(sensorType == USE_BME280) {
      temperature = bme.readTemperature();
      humidity = bme.readHumidity();
    } else if(sensorType == USE_DEBUG) {
      const float upperBound = settings.getHighTemperatureThreshold() + 1.5f;
      const float lowerBound = settings.getReadyToPrintThreshold() - 1.5f;
      const float step = random(50, 151) / 100.0f;

      temperature = debug_temp;
      humidity = debug_hum;

      if (debugIncreasing) {
        debug_temp += step;
        if (debug_temp >= upperBound) {
          debug_temp = upperBound;
          debugIncreasing = false;
        }
      } else {
        debug_temp -= step;
        if (debug_temp <= lowerBound) {
          debug_temp = lowerBound;
          debugIncreasing = true;
        }
      }

      debug_hum += random(-50,50) / 100.00;
    } else {
      return false;
    }
    return true;
  }

  String getJSONData() {
    float t, h;
    static int debug_count = 0;
    if (read(t,h)) {
      String payload = String(F("{\"temperature\": ")) + String(t,2);
      if (sensorType == USE_BME280) {
        payload += String(F(", \"humidity\": ")) + String(h,2);
      } 
      else if(sensorType == USE_DEBUG) {
        debug_count++;
        // Send humidity every 5th reading for debug sensor
        if (debug_count <= 10) {
          payload += String(F(", \"humidity\": ")) + String(h,2);
        } 
        else if(debug_count > 20) {
          debug_count = 0;
        }
        payload += String(F(", \"isDebug\": true"));
      }
      payload += F(" }");
      return payload;
    }
    return String(F("{\"error\": \"Sensor read failed\"}"));
  }
  String getJSONData(float& temperature, float& humidity, bool fanState = false) {
    float t, h;
    static int debug_count = 0;
    String payload = String(F("{\"temperature\": ")) + String(temperature,2);
    if (sensorType == USE_BME280) {
      payload += String(F(", \"humidity\": ")) + String(humidity,2);
    } 
    else if(sensorType == USE_DEBUG) {
      debug_count++;
      // Send humidity every 5th reading for debug sensor
      if (debug_count <= 10) {
        payload += String(F(", \"humidity\": ")) + String(humidity ,2);
      } 
      else if(debug_count > 20) {
        debug_count = 0;
      }
      payload += String(F(", \"isDebug\": true"));
      payload += String(F(", \"fan\": ")) + (fanState ? "\"ON\"" : "\"OFF\"");
    }
    payload += String(F(", \"fan\": ")) + (fanState ? "\"ON\"" : "\"OFF\"");
    payload += F(" }");
    return payload;
  }
private:
  Adafruit_BMP085 bmp;
  Adafruit_BME280 bme;
  float debug_temp = 8.0;
  float debug_hum = 50.0;
  bool debugIncreasing = true;
};
