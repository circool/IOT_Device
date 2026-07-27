/**
 * @file mqtt.h
 * @brief MQTT транспорт для умного дома
 */

#ifndef MQTT_H
#define MQTT_H

#include <Arduino.h>
#include <PubSubClient.h>

// ============================================================================
// НАСТРОЙКИ
// ============================================================================

#ifndef FEATURE_MQTT_ENABLED
#define FEATURE_MQTT_ENABLED 1
#endif

// ============================================================================
// ТИПЫ КОЛБЭКОВ (всегда определены, даже если MQTT выключен)
// ============================================================================

/** @brief Колбэк с булевым параметром */
typedef void (*BoolCallback)(bool value, void* context);

/** @brief Колбэк с целочисленным параметром */
typedef void (*IntCallback)(int value, void* context);

/** @brief Колбэк с 32-битным беззнаковым параметром */
typedef void (*UintCallback)(uint32_t value, void* context);

/** @brief Колбэк с float-параметром */
typedef void (*FloatCallback)(float value, void* context);

/** @brief Колбэк без параметров */
typedef void (*VoidCallback)(void* context);

// ============================================================================
// КОНТЕЙНЕРЫ ДЛЯ КОЛБЭКОВ (без std::function и без шаблонов)
// ============================================================================

/**
 * @brief Структура для хранения колбэка с булевым параметром
 */
struct BoolCb {
  BoolCallback func = nullptr;
  void* context = nullptr;
};

/**
 * @brief Структура для хранения колбэка с целочисленным параметром
 */
struct IntCb {
  IntCallback func = nullptr;
  void* context = nullptr;
};

/**
 * @brief Структура для хранения колбэка с 32-битным беззнаковым параметром
 */
struct UintCb {
  UintCallback func = nullptr;
  void* context = nullptr;
};

/**
 * @brief Структура для хранения колбэка с float-параметром
 */
struct FloatCb {
  FloatCallback func = nullptr;
  void* context = nullptr;
};

/**
 * @brief Структура для хранения колбэка без параметров
 */
struct VoidCb {
  VoidCallback func = nullptr;
  void* context = nullptr;
};

// ============================================================================
// MQTT (только если включён)
// ============================================================================

#if FEATURE_MQTT_ENABLED == 1

/** @brief По умолчанию публиковать причину перезагрузки */
#ifndef MQTT_PUBLISH_RESET_REASON
#define MQTT_PUBLISH_RESET_REASON 1
#endif

/** @brief Не публиковать штатные перезагрузки (POWER_ON, SOFT_RESTART) */
#ifndef MQTT_IGNORE_PUBLISH_NORMAL_RESET_REASONS
#define MQTT_IGNORE_PUBLISH_NORMAL_RESET_REASONS 1
#endif

/** @brief По умолчанию публиковать RSSI */
#ifndef MQTT_PUBLISH_RSSI
#define MQTT_PUBLISH_RSSI 1
#endif

/** @brief По умолчанию публиковать версию прошивки */
#ifndef MQTT_PUBLISH_VERSION
#define MQTT_PUBLISH_VERSION 1
#endif

/** @brief MQTT порт по умолчанию */
#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

/** @brief Задержка между попытками переподключения */
#ifndef MQTT_RECONNECT_DELAY_MS
#define MQTT_RECONNECT_DELAY_MS 5000
#endif

/** @brief Интервал публикации heartbeat */
#ifndef STATE_PUBLISH_INTERVAL_MS
#define STATE_PUBLISH_INTERVAL_MS 3000
#endif

/** @brief Keep-alive интервал MQTT */
#ifndef MQTT_KEEPALIVE_SEC
#define MQTT_KEEPALIVE_SEC 3
#endif

// ============================================================================
// КЛАСС MQTTManager
// ============================================================================

class MQTTManager {
 public:
  MQTTManager();
  ~MQTTManager();

  // ========================================================================
  // УПРАВЛЕНИЕ ПОДКЛЮЧЕНИЕМ
  // ========================================================================

  bool begin(Client& client,
             const char* broker,
             uint16_t port,
             const char* clientId,
             const char* user = nullptr,
             const char* password = nullptr);

  void process();
  bool isConnected();
  void disconnect();

  // ========================================================================
  // ПУБЛИКАЦИЯ ДАННЫХ (устройство → брокер)
  // ========================================================================

  void publishOnline();
  void publishState(bool on);
  void publishSpeed(int percent);
  void publishDelaySec(int seconds);
  void publishMaxOnTime(uint32_t seconds);
  void publishSensorControlMode(bool enabled);

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  void publishSensor(float temp, float hum);
#endif

#if DEVICE_TYPE == 1
  void publishAdaptiveMode(bool enabled);
  void publishThresholds(float lowTemp,
                         float highTemp,
                         float lowHum,
                         float highHum);
#endif

#if MQTT_PUBLISH_RSSI == 1
  void publishRSSI(int rssi);
#endif

#if MQTT_PUBLISH_VERSION == 1
  void publishVersion(const char* version);
#endif

#if MQTT_PUBLISH_RESET_REASON == 1
  void publishResetReason(const char* reason);
#endif

  // ========================================================================
  // РЕГИСТРАЦИЯ КОЛБЭКОВ (брокер → устройство)
  // ========================================================================

  void onState(BoolCallback callback, void* context);
  void onSpeed(IntCallback callback, void* context);
  void onDelaySec(IntCallback callback, void* context);
  void onMaxOnTime(UintCallback callback, void* context);
  void onSensorControlMode(BoolCallback callback, void* context);

#if DEVICE_TYPE == 1
  void onAdaptiveMode(BoolCallback callback, void* context);
  void onLowTemp(FloatCallback callback, void* context);
  void onHighTemp(FloatCallback callback, void* context);
  void onLowHum(FloatCallback callback, void* context);
  void onHighHum(FloatCallback callback, void* context);
#endif

#if MQTT_RESET_ENABLED == 1
  void onReset(VoidCallback callback, void* context);
#endif

 private:
  // ========================================================================
  // ВНУТРЕННИЕ МЕТОДЫ
  // ========================================================================

  void reconnect();
  void setupTopics();
  void subscribe();
  void callback(char* topic, byte* payload, unsigned int length);
  static void staticCallback(char* topic, byte* payload, unsigned int length);
  void handleCommand(const char* topic, const String& payload);
  bool firstAttempt = true;

  // ========================================================================
  // ДАННЫЕ
  // ========================================================================

  PubSubClient _mqttClient;

  // Топики
  struct {
    char online[48];
    char version[48];
    char reset[48];
    char state[48];
    char control[48];
    char speed[48];
    char speedControl[48];
    char delaySec[48];
    char delaySecControl[48];
    char maxOnTime[48];
    char maxOnTimeControl[48];
    char sensorControlMode[48];
    char sensorControlModeControl[48];

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    char temperature[48];
    char humidity[48];
#endif

#if DEVICE_TYPE == 1
    char adaptiveMode[48];
    char adaptiveModeControl[48];
    char lowTemp[48], highTemp[48], lowHum[48], highHum[48];
    char lowTempControl[48], highTempControl[48];
    char lowHumControl[48], highHumControl[48];
#endif

#if MQTT_PUBLISH_RSSI == 1
    char rssi[48];
#endif
  } _topics;

  char _clientId[24];
  bool _initialized = false;
  unsigned long _lastReconnectAttempt = 0;

  // Параметры для переподключения
  char _broker[64];
  uint16_t _port = 1883;
  char _user[32];
  char _password[64];

  // ========================================================================
  // КОЛБЭКИ (без std::function!)
  // ========================================================================

  BoolCb _onState;
  IntCb _onSpeed;
  IntCb _onDelaySec;
  UintCb _onMaxOnTime;
  BoolCb _onSensorControlMode;

#if DEVICE_TYPE == 1
  BoolCb _onAdaptiveMode;
  FloatCb _onLowTemp;
  FloatCb _onHighTemp;
  FloatCb _onLowHum;
  FloatCb _onHighHum;
#endif

#if MQTT_RESET_ENABLED == 1
  VoidCb _onReset;
#endif
};

extern MQTTManager mqttManager;

#endif  // FEATURE_MQTT_ENABLED == 1

#endif  // MQTT_H