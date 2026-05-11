#include <Arduino.h>

#ifdef ESP32
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif

#include "config.h"
#include "sensor.h"
#include "fan.h"
#include "mqtt.h"
#include "web.h"

unsigned long lastWiFiCheck = 0;
unsigned long lastMQTTAttempt = 0;
unsigned long wifiLostTime = 0;
bool wifiConnected = false;
bool wifiConnecting = false;
unsigned long wifiConnectStartTime = 0;

void checkResetButton() {
  pinMode(RESET_PIN, INPUT_PULLUP);
  delay(50);
  
  if (digitalRead(RESET_PIN) == LOW) {
    #ifdef DEBUG_ENABLE
      Serial.println("\n[MAIN] Reset button pressed...");
      Serial.println("[MAIN] Hold for 3 seconds to confirm reset...");
    #endif
    unsigned long pressStart = millis();
    
    while (digitalRead(RESET_PIN) == LOW) {
      if (millis() - pressStart >= 3000) {
        #ifdef DEBUG_ENABLE
          Serial.println("[MAIN] Reset confirmed! Waiting for button release...");
        #endif

        while (digitalRead(RESET_PIN) == LOW) {
          delay(10);
        }
        
        #ifdef DEBUG_ENABLE
          Serial.println("[MAIN] Clearing configuration...");
        #endif

        if (config_clear()) {
          Serial.println("[MAIN] Configuration cleared. Restarting...");
          delay(1000);
          ESP.restart();  
        } else {
          Serial.println("[MAIN] Clear failed — not restarting");
        }
        return;
      }
      delay(10);
    }
    #ifdef DEBUG_ENABLE
      Serial.println("[MAIN] Button released too early — reset cancelled");
    #endif
  }
}

void wifi_beginAsync() {
  if (strlen(config.wifiSsid) == 0) {
    Serial.println("[WIFI] No SSID configured");
    return;
  }
  
  if (wifiConnected || wifiConnecting) return;
  #ifdef DEBUG_ENABLE
    Serial.printf("[WIFI] Starting async connection to %s\n", config.wifiSsid);
  #endif
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.wifiSsid, config.wifiPassword);
  wifiConnecting = true;
  wifiConnectStartTime = millis();
  wifiLostTime = 0;
}

void wifi_checkAsync() {
  if (!wifiConnecting) return;
  
  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    wifiConnecting = false;
    wifiLostTime = 0;
    Serial.println("[WIFI] Connected! IP: " + WiFi.localIP().toString());
    
    if (apMode) {
      WiFi.softAPdisconnect(true);
      apMode = false;
    }
    
    mqtt_init();
    
  } else if (millis() - wifiConnectStartTime > 30000) {
    Serial.println("[WIFI] Connection timeout");
    wifiConnecting = false;
    wifiConnected = false;
  }
}

void mqtt_checkAsync() {
  if (!wifiConnected) return;
  if (!configValid) return;
  
  if (!mqtt_isConnected() && (millis() - lastMQTTAttempt > 5000)) {
    lastMQTTAttempt = millis();
    mqtt_reconnect();
  }
  
  if (mqtt_isConnected()) {
    mqttClient.loop();
    
    static unsigned long lastOnline = 0;
    if (millis() - lastOnline > 10000) {
      mqtt_publishOnline();
      lastOnline = millis();
    }
  }
}

void publishDataIfNeeded() {
  if (!mqtt_isConnected()) return;
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  if (sensor_isOk()) {
    static float lastTemp = 0, lastHum = 0;
    const float EPSILON = 0.05;
    if (fabs(currentTemp - lastTemp) > EPSILON || fabs(currentHum - lastHum) > EPSILON) {
        mqtt_publishSensor();
        lastTemp = currentTemp;
        lastHum = currentHum;
    }
  }
  #endif
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  static bool lastState = false;
  if (fan_getState() != lastState) {
    mqtt_publishState();
    lastState = fan_getState();
  }
  #endif
}

void checkWiFiFallbackToAP() {
  if (strlen(config.wifiSsid) == 0) return;
  if (apMode) return;
  if (wifiConnected) {
    wifiLostTime = 0;
    return;
  }
  
  if (!wifiConnected && !wifiConnecting) {
    if (wifiLostTime == 0) {
      wifiLostTime = millis();
      Serial.println("[MAIN] WiFi lost, starting fallback timer");
    } else if (millis() - wifiLostTime > AP_FALLBACK_TIMEOUT_MS) {
      Serial.println("[MAIN] WiFi lost for too long, switching to AP mode");
      web_initAP();
      wifiLostTime = 0;
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n==========================================");
  Serial.println("Device starting...");
  Serial.println("==========================================");
  
  checkResetButton();
  
  config_init();
  config_print();
  
  bool hasValidConfig = (configValid && strlen(config.wifiSsid) > 0);
  
  if (hasValidConfig) {
    Serial.println("[MAIN] Normal mode - starting with saved config");
    
    mqtt_setupTopics(config.mqttClientId);
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    fan_init();
    #endif
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    sensor_init();
    #endif
    
    wifi_beginAsync();
    web_init();
    
  } else {
    Serial.println("[MAIN] Configuration mode - starting AP for setup");
    web_initAP();
  }
}

void loop() {
  checkResetButton();

  if (!configValid || strlen(config.wifiSsid) == 0) {
    web_update();
    delay(100);
    return;
  }
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  fan_update();
  #endif
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  sensor_read();
  #endif
  
  wifi_checkAsync();
  checkWiFiFallbackToAP();
  
  if (wifiConnected) {
    mqtt_checkAsync();
    publishDataIfNeeded();
  }
  
  web_update();
  delay(50);
}