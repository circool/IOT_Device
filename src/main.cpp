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
    Serial.println("\n[MAIN] Reset button pressed! Clearing configuration...");
    config_clear();
    Serial.println("[MAIN] Configuration cleared. Restarting...");
    delay(1000);
    ESP.restart();
  }
}

// Асинхронное подключение к WiFi (неблокирующее)
void wifi_beginAsync() {
  if (strlen(config.wifiSsid) == 0) {
    Serial.println("[WIFI] No SSID configured");
    return;
  }
  
  if (wifiConnected || wifiConnecting) return;
  
  Serial.printf("[WIFI] Starting async connection to %s\n", config.wifiSsid);
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
    
    // Отключаем AP если он был активен
    if (apMode) {
      WiFi.softAPdisconnect(true);
      apMode = false;
    }
    
    // Запускаем MQTT после успешного WiFi
    mqtt_init();
    
  } else if (millis() - wifiConnectStartTime > 30000) {
    // Таймаут 30 секунд
    Serial.println("[WIFI] Connection timeout");
    wifiConnecting = false;
    wifiConnected = false;
  }
}

void mqtt_checkAsync() {
  if (!wifiConnected) return;
  if (!configValid) return;
  
  // Попытка подключения к MQTT не чаще чем раз в 5 секунд
  if (!mqtt_isConnected() && (millis() - lastMQTTAttempt > 5000)) {
    lastMQTTAttempt = millis();
    mqtt_reconnect();
  }
  
  // Если подключены - обслуживаем MQTT
  if (mqtt_isConnected()) {
    mqttClient.loop();
    
    // Публикация статуса Online каждые 10 секунд
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
    const float EPSILON = 0.05;  // порог 0.05°C
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
  // Только если есть конфигурация WiFi и не в AP режиме
  if (strlen(config.wifiSsid) == 0) return;
  if (apMode) return;
  if (wifiConnected) {
    wifiLostTime = 0;
    return;
  }
  
  // Если WiFi не подключён и нет попытки подключения
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
  
  // 1. Инициализация EEPROM и загрузка конфигурации
  config_init();
  config_print();
  
  // 2. Определяем тип запуска
  bool hasValidConfig = (configValid && strlen(config.wifiSsid) > 0);
  
  if (hasValidConfig) {
    // ========== ОБЫЧНЫЙ РЕЖИМ ==========
    Serial.println("[MAIN] Normal mode - starting with saved config");
    
    // Генерация MQTT Client ID если пуст
    if (strlen(config.mqttClientId) == 0) {
      uint8_t mac[6];
      WiFi.macAddress(mac);
      snprintf(config.mqttClientId, sizeof(config.mqttClientId), 
               "device_%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
      config_write();
    }
    
    mqtt_setupTopics(config.mqttClientId);
    
    // Инициализация железа (НЕ БЛОКИРУЕТСЯ)
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    fan_init();
    #endif
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    sensor_init();
    #endif
    
    // Запускаем асинхронное подключение к WiFi
    wifi_beginAsync();
    
    // Запускаем веб-сервер в КЛИЕНТСКОМ режиме
    web_init();
    
  } else {
    // ========== РЕЖИМ НАСТРОЙКИ ==========
    Serial.println("[MAIN] Configuration mode - starting AP for setup");
    
    // Запускаем AP режим для настройки (defaults уже загружены в config_read)
    web_initAP();
  }
}

void loop() {
  // Режим настройки (нет валидной конфигурации)
  if (!configValid || strlen(config.wifiSsid) == 0) {
    web_update();
    delay(100);
    return;
  }
  
  // ========== ОСНОВНАЯ ФУНКЦИОНАЛЬНОСТЬ (ВЫПОЛНЯЕТСЯ ВСЕГДА) ==========
  
  // Управление вентилятором (не зависит от WiFi/MQTT)
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  fan_update();
  #endif
  
  // Чтение датчика (не зависит от WiFi/MQTT)
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  sensor_read();
  #endif
  
  // ========== ВТОРОСТЕПЕННАЯ ФУНКЦИОНАЛЬНОСТЬ (АСИНХРОННО) ==========
  
  // Асинхронное подключение к WiFi
  wifi_checkAsync();
  
  // Проверка необходимости перехода в AP режим при потере WiFi
  checkWiFiFallbackToAP();
  
  // Асинхронное подключение к MQTT и публикация данных
  if (wifiConnected) {
    mqtt_checkAsync();
    publishDataIfNeeded();
  }
  
  // Обслуживание веб-сервера
  web_update();
  
  // Небольшая задержка для стабильности
  delay(50);
}