/**
 * @file transport_wifi_mqtt.h
 * @brief MQTT-менеджер — полная реализация
 */

#ifndef transport_wifi_mqtt_H
#define transport_wifi_mqtt_H

#include <Arduino.h>
#include "common_types.h"
#include "logger.h"
#include "settings.h"
#include "transport_types.h"

#include <PubSubClient.h>

#ifdef ESP32
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#ifdef USE_MQTT
class MQTTManager {
 public:
  MQTTManager();

  // ===== УПРАВЛЕНИЕ =====
  bool begin(const TransportConfig* config);
  void update();
  bool isConnected();
  void disconnect();
  const char* getName() const;

  // ===== ПУБЛИКАЦИЯ =====
  void publishOnline();
  void publishState(bool on);
  void publishSpeed(int percent);
  void publishDelaySec(int seconds);
  void publishMaxOnTime(uint32_t seconds);
  void publishSensorControlMode(bool enabled);
  void publishSensor(float temp, float hum);
  void publishAdaptiveMode(bool enabled);
  void publishThresholds(float lowTemp,
                         float highTemp,
                         float lowHum,
                         float highHum);
  void publishRSSI(int rssi);
  void publishVersion(const char* version);
  void publishResetReason(const char* reason);

  // ===== КОЛБЭКИ =====
  /**
   * @brief Регистрация колбэка для событий
   * @param callback Функция обратного вызова
   * @param context Контекст для колбэка
   */
  void onEvent(TransportEventCallback callback, void* context);

 private:
  // ===== ВНУТРЕННИЕ МЕТОДЫ =====
  void reconnect();
  void subscribe();
  void callback(char* topic, byte* payload, unsigned int length);

  // ===== ДАННЫЕ =====
  PubSubClient _mqttClient;
  WiFiClient _wifiClient;

  TransportConfig _config;
  char _clientId[24];

  bool _running;
  bool _connected;
  unsigned long _lastReconnectAttempt;

  // ===== ТОПИКИ =====
  struct {
    char online[48];
    char version[48];
    char reset[48];
    char state[48];
    char speed[48];
    char delaySec[48];
    char maxOnTime[48];
    char sensorMode[48];
    char adaptiveMode[48];
    char temperature[48];
    char humidity[48];
    char lowTemp[48];
    char highTemp[48];
    char lowHum[48];
    char highHum[48];
    char rssi[48];
  } _topics;

  // ===== КОЛБЭКИ =====
  TransportEventCallback _eventCallback;
  void* _eventContext;
};

#else  // USE_MQTT == 0

// ============================================================================
// ЗАГЛУШКИ (USE_MQTT == 0)
// ============================================================================

/**
 * @brief Заглушка MQTTManager — MQTT-клиент отключён
 * @details Все методы — пустые заглушки. Используется при FEATURE_MQTT_ENABLED
 * == 0
 */
class MQTTManager {
 public:
  MQTTManager() {}

  // ===== УПРАВЛЕНИЕ =====
  /**
   * @brief Заглушка — MQTT-клиент отключён
   */
  inline bool begin(const TransportConfig* config) {
    (void)config;
    return true;
  }

  /**
   * @brief Заглушка — MQTT-клиент отключён
   */
  inline void update() {}

  /**
   * @brief Заглушка — MQTT-клиент отключён
   */
  inline bool isConnected() { return false; }

  /**
   * @brief Заглушка — MQTT-клиент отключён
   */
  inline void disconnect() {}

  /**
   * @brief Заглушка — MQTT-клиент отключён
   */
  inline const char* getName() const { return "MQTT (stub)"; }

  // ===== ПУБЛИКАЦИЯ =====
  /**
   * @brief Заглушка — MQTT-клиент отключён
   */
  inline void publishOnline() {}

  /**
   * @brief Заглушка — MQTT-клиент отключён
   */
  inline void publishState(bool on) { (void)on; }

  /**
   * @brief Заглушка — MQTT-клиент отключён
   */
  inline void publishSpeed(int percent) { (void)percent; }

  /**
   * @brief Заглушка — MQTT-клиент отключён
   */
  inline void publishDelaySec(int seconds) { (void)seconds; }

  /**
   * @brief Заглушка — MQTT-клиент отключён
   */
  inline void publishMaxOnTime(uint32_t seconds) { (void)seconds; }

  /**
   * @brief Заглушка — MQTT-клиент отключён
   */
  inline void publishSensorControlMode(bool enabled) { (void)enabled; }

  /**
   * @brief Заглушка — MQTT-клиент отключён
   */
  inline void publishSensor(float temp, float hum) {
    (void)temp;
    (void)hum;
  }

  /**
   * @brief Заглушка — MQTT-клиент отключён
   */
  inline void publishAdaptiveMode(bool enabled) { (void)enabled; }

  /**
   * @brief Заглушка — MQTT-клиент отключён
   */
  inline void publishThresholds(float lowTemp,
                                float highTemp,
                                float lowHum,
                                float highHum) {
    (void)lowTemp;
    (void)highTemp;
    (void)lowHum;
    (void)highHum;
  }

  /**
   * @brief Заглушка — MQTT-клиент отключён
   */
  inline void publishRSSI(int rssi) { (void)rssi; }

  /**
   * @brief Заглушка — MQTT-клиент отключён
   */
  inline void publishVersion(const char* version) { (void)version; }

  /**
   * @brief Заглушка — MQTT-клиент отключён
   */
  inline void publishResetReason(const char* reason) { (void)reason; }

  // ===== КОЛБЭКИ =====
  /**
   * @brief Заглушка — MQTT-клиент отключён
   */
  inline void onEvent(TransportEventCallback callback, void* context) {
    (void)callback;
    (void)context;
  }
};

#endif  // USE_MQTT
#endif  // transport_wifi_mqtt_H