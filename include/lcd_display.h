#pragma once
#include <Arduino.h>
#include <LiquidCrystal_I2C.h>

class LCDDisplay {
public:
  // Default I2C address for 20x4 LCD is 0x27 (common) or 0x3F
  LCDDisplay(uint8_t address = 0x27) : lcd(address, 20, 4), 
    lastTemp(-999.0), lastHumidity(-999.0), ipDisplayed(false) {}

  bool begin() {
    // Initialize I2C LCD
    lcd.init();
    lcd.backlight();
    
    // Display startup message
    lcd.setCursor(0, 0);
    lcd.print("Temperature Monitor");
    lcd.setCursor(0, 1);
    lcd.print("Initializing...");
    
    delay(2000);
    lcd.clear();
    return true;
  }

  // Display IP address once at boot time
  void displayIPAddress(const char* ipAddress) {
    if (!ipDisplayed) {
      lcd.setCursor(0, 2);
      lcd.print("                    ");  // Clear line
      lcd.setCursor(0, 2);
      if (ipAddress && ipAddress[0] != '\0') {
        lcd.print("IP: ");
        lcd.print(ipAddress);
      } else {
        lcd.print("IP: Connecting...");
      }
      ipDisplayed = true;
    }
  }

  // Update temperature only if it changed
  void updateTemperature(float temperature) {
    // Only update if temperature changed by at least 0.1 degrees
    if (fabsf(temperature - lastTemp) >= 0.1) {
      lastTemp = temperature;
      lcd.setCursor(0, 0);
      lcd.print("Temp: ");
      lcd.print(temperature, 1);
      lcd.print("C  ");  // Extra spaces to clear old text
    }
  }

  // Update humidity only if it changed and sensor supports it
  void updateHumidity(float humidity, bool isBME280) {
    if (!isBME280) {
      // Don't update humidity line if sensor doesn't support it
      return;
    }
    
    // Only update if humidity changed by at least 1%
    if (fabsf(humidity - lastHumidity) >= 1.0) {
      lastHumidity = humidity;
      lcd.setCursor(0, 1);
      lcd.print("Humidity: ");
      lcd.print(humidity, 1);
      lcd.print("%   ");  // Extra spaces to clear old text
    }
  }

  // Display humidity sensor status (called once or when sensor type changes)
  void displayHumidityStatus(bool isBME280) {
    if (!isBME280) {
      lcd.setCursor(0, 1);
      lcd.print("(No Humidity Sensor)");
    }
  }

  // Update time every second
  void updateTime() {
    lcd.setCursor(0, 3);
    unsigned long seconds = millis() / 1000;
    unsigned long minutes = seconds / 60;
    unsigned long secs = seconds % 60;
    
    lcd.print("Time: ");
    if (minutes < 10) lcd.print("0");
    lcd.print(minutes);
    lcd.print(":");
    if (secs < 10) lcd.print("0");
    lcd.print(secs);
    lcd.print("  ");  // Extra spaces to clear old text
  }

  void displayError(const char* errorMsg) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("ERROR");
    lcd.setCursor(0, 1);
    lcd.print(errorMsg);
    lcd.setCursor(0, 3);
    lcd.print("Check connections");
  }

  void displayUploadMessage(const char* message) {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(message);
    lcd.setCursor(0, 2);
    lcd.print("Please wait...");
  }

private:
  LiquidCrystal_I2C lcd;
  float lastTemp;
  float lastHumidity;
  bool ipDisplayed;
};
