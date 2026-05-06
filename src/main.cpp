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
unsigned long lastAPFallbackCheck = 0;
bool wifiConnected = false;

void checkResetButton() {
  pinMode(RESET_PIN, INPUT_PULLUP);
  delay(50);
  if (digitalRead(RESET_PIN) == LOW) {
    Serial.println("\n[MAIN] Reset button pressed! Clearing configuration...");
    config_clear();
    Serial.println("[MAIN] Configuration cleared. Restarting...");
    delay(1000);
    ESP.restart();
  }
}

void setup_wifi() {
  if (strlen(config.wifiSsid) == 0) {
    Serial.println("No WiFi SSID configured, staying in AP mode");
    return;
  }
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.wifiSsid, config.wifiPassword);
  Serial.print("Connecting to WiFi");
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected, IP: " + WiFi.localIP().toString());
    wifiConnected = true;
    if (apMode) {
      WiFi.softAPdisconnect(true);
      apMode = false;
      Serial.println("AP mode disabled");
    }
  } else {
    Serial.println("\nWiFi connection failed");
    wifiConnected = false;
  }
}

void checkWiFi() {
  if (apMode) return;
  if (strlen(config.wifiSsid) == 0) return;
  if (millis() - lastWiFiCheck < WIFI_CHECK_INTERVAL_MS) return;
  lastWiFiCheck = millis();
  
  if (WiFi.status() != WL_CONNECTED) {
    wifiConnected = false;
    setup_wifi();
  } else {
    wifiConnected = true;
  }
}

void checkAPFallback() {
  if (apMode) return;
  
  if (!configValid && !apMode) {
    Serial.println("Config invalid, starting AP mode");
    web_initAP();
    return;
  }
  
  if (wifiConnected) return;
  
  if (millis() - lastAPFallbackCheck < AP_FALLBACK_TIMEOUT_MS) return;
  lastAPFallbackCheck = millis();
  
  if (!apMode && strlen(config.wifiSsid) > 0 && WiFi.status() != WL_CONNECTED) {
    Serial.println("No WiFi for 2 minutes, starting AP mode");
    web_initAP();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nSmart Fan Device Starting...");
  
  checkResetButton();
  
  config_init();
  config_print();
  
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char deviceMac[18];
  snprintf(deviceMac, sizeof(deviceMac), "%02X:%02X:%02X:%02X:%02X:%02X", 
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  
  if (strlen(config.mqttClientId) == 0) {
    snprintf(config.mqttClientId, sizeof(config.mqttClientId), "fan_%s", deviceMac);
    #ifdef DEBUG_ENABLE
      Serial.printf("[MAIN] Auto-generated MQTT Client ID: %s\n", config.mqttClientId);
    #endif
  }
  mqtt_setupTopics(config.mqttClientId);
  
  fan_init();
  sensor_init();
  mqtt_init();
  
  if (configValid && strlen(config.wifiSsid) > 0) {
    setup_wifi();
    web_init();
  } else {
    Serial.println("No valid config, starting AP mode");
    web_initAP();
  }
  
  if (config.delaySeconds > 0 && config.automaticMode) {
    fan_resetDelayTimer();
    #ifdef DEBUG_ENABLE
      Serial.printf("[MAIN] Delay ON timer started: %d seconds\n", config.delaySeconds);
    #endif
  }
}

void loop() {
  checkAPFallback();
  
  if (apMode) {
    web_update();
    delay(100);
    return;
  }
  
  if (wifiConnected) {
    mqtt_reconnect();
    if (mqtt_isConnected()) {
      mqttClient.loop();
      
      static unsigned long lastKeepAlive = 0;
      if (millis() - lastKeepAlive > MQTT_KEEPALIVE_SEC * 1000) {
        lastKeepAlive = millis();
        mqttClient.publish(lastWillTopic, "Online", true);
      }
    }
  }
  
  checkWiFi();
  sensor_read();
  fan_update();
  
  if (mqtt_isConnected()) {
    static float lastPublishedTemp = 0;
    static float lastPublishedHum = 0;
    
    if (sensor_isOk() && (currentTemp != lastPublishedTemp || currentHum != lastPublishedHum)) {
      mqtt_publishSensor();
      lastPublishedTemp = currentTemp;
      lastPublishedHum = currentHum;
      #ifdef DEBUG_ENABLE
        Serial.println("[MAIN] Sensor published (value changed)");
      #endif
    }
    
    static unsigned long lastStatePublish = 0;
    if (millis() - lastStatePublish > STATE_PUBLISH_INTERVAL_MS) {
      mqtt_publishState();
      lastStatePublish = millis();
    }
  }
  
  web_update();
  delay(100);
}