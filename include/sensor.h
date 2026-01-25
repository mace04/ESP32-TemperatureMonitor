#pragma once
#include <Arduino.h>
#include <Adafruit_BMP085.h>
#include <Adafruit_BME280.h>

enum SensorType {
  USE_BMP180,
  USE_BME280,
  USE_DEBUG
};

class Sensor {
public:
  SensorType sensorType;
  bool begin() {
    Serial.begin(115200);
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
      // Provide dummy values for debugging
      temperature = debug_temp;
      humidity = debug_hum;
      static bool stabilised = false;
      if(!stabilised && debug_temp < 20.0) {
        debug_temp += random(1,50) / 100.00;
      } else {
        stabilised = true;
      }
      if (stabilised) debug_temp += random(-40,40) / 100.00;
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
      }
      payload += F(" }");
      return payload;
    }
    return String(F("{\"error\": \"Sensor read failed\"}"));
  }
private:
  Adafruit_BMP085 bmp;
  Adafruit_BME280 bme;
  float debug_temp = 8.0;
  float debug_hum = 50.0;
};
