#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>
#include "settings.h"

// ============================================================================
// КОНСТАНТЫ (перенесены из config.h)
// ============================================================================

// ============================================================================
// ПРЕФИКС УСТРОЙСТВА
// ============================================================================

/**
 * @brief Префикс устройства (для MQTT топиков и AP SSID)
 * Формируется автоматически на основе DEVICE_TYPE
 */
#if DEVICE_TYPE == 1
#define DEVICE_PREFIX "fan"
#elif DEVICE_TYPE == 2
#define DEVICE_PREFIX "sensor"
#elif DEVICE_TYPE == 3
#define DEVICE_PREFIX "switch"
#else
#define DEVICE_PREFIX "device"
#endif

// ============================================================================
// I2C (для датчика AHT10)
// ============================================================================

/** @brief Пин I2C SDA (для AHT10) */
#ifndef I2C_SDA_PIN
#ifdef ESP8266
#define I2C_SDA_PIN 4
#elif defined(ESP32)
#define I2C_SDA_PIN 21
#endif
#endif

/** @brief Пин I2C SCL (для AHT10) */
#ifndef I2C_SCL_PIN
#ifdef ESP8266
#define I2C_SCL_PIN 5
#elif defined(ESP32)
#define I2C_SCL_PIN 22
#endif
#endif

// ============================================================================
// ДАТЧИК (для TYPE 1 и 2)
// ============================================================================

/**
 * @brief Тип датчика температуры/влажности
 * @values 1 – AHT10 (I2C)
 *         2 – DHT11/DHT22 (GPIO)
 */
#ifndef SENSOR_TYPE
#define SENSOR_TYPE 1
#endif

#if SENSOR_TYPE == 2
/** @brief Пин для DHT датчика (только для SENSOR_TYPE=2) */
#ifndef SENSOR_PIN
#ifdef ESP8266
#define SENSOR_PIN 4
#elif defined(ESP32)
#define SENSOR_PIN 16
#endif
#endif
#endif

// ============================================================================
// ИСПОЛНИТЕЛЬНЫЙ МЕХАНИЗМ (для TYPE 1 и 3)
// ============================================================================

/** @brief Пин управления реле/вентилятором */
#ifndef SWITCH_PIN
#ifdef ESP8266
#define SWITCH_PIN 14
#elif defined(ESP32)
#define SWITCH_PIN 4
#endif
#endif

// ============================================================================
// ДИАПАЗОНЫ ДЛЯ ВАЛИДАЦИИ
// ============================================================================

/** @brief Минимальная допустимая температура (°C) */
#ifndef TEMP_MIN
#define TEMP_MIN -40.0
#endif

/** @brief Максимальная допустимая температура (°C) */
#ifndef TEMP_MAX
#define TEMP_MAX 85.0
#endif

/** @brief Минимальная допустимая влажность (%) */
#ifndef HUM_MIN
#define HUM_MIN 0.0
#endif

/** @brief Максимальная допустимая влажность (%) */
#ifndef HUM_MAX
#define HUM_MAX 100.0
#endif

/** @brief Минимальный интервал опроса датчика (сек) */
#ifndef SENSOR_INTERVAL_MIN
#define SENSOR_INTERVAL_MIN 1
#endif

/** @brief Максимальный интервал опроса датчика (сек) */
#ifndef SENSOR_INTERVAL_MAX
#define SENSOR_INTERVAL_MAX 3600
#endif

/** @brief Минимальная задержка включения (сек) */
#ifndef DELAY_SECONDS_MIN
#define DELAY_SECONDS_MIN 0
#endif

/** @brief Максимальная задержка включения (сек) */
#ifndef DELAY_SECONDS_MAX
#define DELAY_SECONDS_MAX 86400
#endif

/** @brief Минимальное время аварийного отключения (сек) */
#ifndef MAX_ON_TIME_MIN
#define MAX_ON_TIME_MIN 0
#endif

/** @brief Максимальное время аварийного отключения (сек) */
#ifndef MAX_ON_TIME_MAX
#define MAX_ON_TIME_MAX 86400
#endif

/** @brief Минимальная скорость (%) */
#ifndef SPEED_PERCENT_MIN
#define SPEED_PERCENT_MIN 0
#endif

/** @brief Максимальная скорость (%) */
#ifndef SPEED_PERCENT_MAX
#define SPEED_PERCENT_MAX 100
#endif

// ============================================================================
// ЗНАЧЕНИЯ ПО УМОЛЧАНИЮ
// ============================================================================

#if DEVICE_TYPE == 1
/** @brief Нижний порог температуры по умолчанию (°C) — ниже этого выключаем */
#ifndef DEFAULT_LOW_TEMP
#define DEFAULT_LOW_TEMP 27.0
#endif

/** @brief Верхний порог температуры по умолчанию (°C) — выше этого включаем */
#ifndef DEFAULT_HIGH_TEMP
#define DEFAULT_HIGH_TEMP 29.0
#endif

/** @brief Нижний порог влажности по умолчанию (%) — ниже этого выключаем */
#ifndef DEFAULT_LOW_HUM
#define DEFAULT_LOW_HUM 55.0
#endif

/** @brief Верхний порог влажности по умолчанию (%) — выше этого включаем */
#ifndef DEFAULT_HIGH_HUM
#define DEFAULT_HIGH_HUM 60.0
#endif

/** @brief Режим управления сенсором по умолчанию (вкл = авто) */
#ifndef DEFAULT_SENSOR_CONTROL_MODE
#define DEFAULT_SENSOR_CONTROL_MODE true
#endif
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
/** @brief Задержка отложенного включения по умолчанию (сек) */
#ifndef DEFAULT_DELAY_SECONDS
#define DEFAULT_DELAY_SECONDS 60
#endif

/** @brief Адаптивный режим по умолчанию (вкл/выкл) */
#ifndef DEFAULT_ADAPTIVE_MODE
#define DEFAULT_ADAPTIVE_MODE false
#endif

/** @brief Состояние при старте (вкл = нагрузка включена после перезагрузки) */
#ifndef BOOT_SWITCH_STATE
#define BOOT_SWITCH_STATE true
#endif
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
/** @brief Интервал опроса датчика по умолчанию (сек) */
#ifndef SENSOR_DURATION
#define SENSOR_DURATION 10
#endif
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
/** @brief Включить таймер аварийного отключения */
#ifndef EMERGENCY_ENABLED
#define EMERGENCY_ENABLED 1
#endif

#if EMERGENCY_ENABLED == 1
/** @brief Время аварийного отключения по умолчанию (сек) */
#ifndef MAX_ON_TIME_SEC
#define MAX_ON_TIME_SEC 3600
#endif
#endif
#endif

/** @brief Скорость по умолчанию (%) */
#ifndef DEFAULT_SPEED_PERCENT
#define DEFAULT_SPEED_PERCENT 50
#endif

// ============================================================================
// EEPROM
// ============================================================================

/** @brief Магическое число для проверки валидности EEPROM */
#ifndef MAGIC_VALUE
#define MAGIC_VALUE 0x5A6D
#endif

// ============================================================================
// CREDENTIALS (заводские настройки)
// ============================================================================

#ifdef __has_include
#if __has_include("credentials.h")
#include "credentials.h"
#define HAS_CREDENTIALS 1
#else
#define HAS_CREDENTIALS 0
#endif
#else
#ifdef CREDENTIALS_AVAILABLE
#include "credentials.h"
#define HAS_CREDENTIALS 1
#else
#define HAS_CREDENTIALS 0
#endif
#endif

/** @brief WiFi SSID по умолчанию (заводской) */
#ifndef SSID_NAME
#define SSID_NAME ""
#endif

/** @brief WiFi пароль по умолчанию (заводской) */
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

/** @brief MQTT брокер по умолчанию (заводской) */
#ifndef MQTT_ADDRESS
#define MQTT_ADDRESS ""
#endif

/** @brief MQTT порт по умолчанию */
#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

/** @brief MQTT пользователь по умолчанию (заводской) */
#ifndef MQTT_USER
#define MQTT_USER ""
#endif

/** @brief MQTT пароль по умолчанию (заводской) */
#ifndef MQTT_PASSWORD
#define MQTT_PASSWORD ""
#endif

// ============================================================================
// СТРУКТУРЫ ДАННЫХ
// ============================================================================

/**
 * @brief Структура конфигурации, сохраняемая в EEPROM
 */
struct ConfigData {
  uint16_t magic;  //!< Магическое число для проверки валидности
  uint16_t crc;    //!< CRC16 от всей структуры (кроме самого поля crc)

  // ========== WiFi настройки ==========
#if WIFI_ENABLED == 1
  char wifiSsid[32];
  char wifiPassword[64];
#endif

  // ========== MQTT настройки ==========
#if MQTT_ENABLED == 1
  char mqttBroker[64];
  uint16_t mqttPort;
  char mqttUser[32];
  char mqttPassword[64];
  char mqttClientId[24];
#endif

  // ========== Настройки вентилятора (TYPE 1) ==========
#if DEVICE_TYPE == 1
  double lowHum;
  double highHum;
  double lowTemp;
  double highTemp;
  bool sensorControlMode;
  uint16_t speedPercent;
  bool adaptiveMode;
#endif

  // ========== Общие настройки для TYPE 1 и TYPE 3 ==========
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  int delaySeconds;
  uint32_t maxOnTime;
  bool bootState;
#endif

  // ========== Настройки датчика для TYPE 1 и TYPE 2 ==========
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  uint16_t sensorInterval;
#endif

  // ========== Zigbee настройки ==========
#if ZIGBEE_ENABLED == 1
  char zigbeeNetworkKey[32];
  uint16_t zigbeePanId;
  uint8_t zigbeeChannel;
#endif
};



// ============================================================================
// КЛАСС CONFIG MANAGER
// ============================================================================

/**
 * @brief Менеджер конфигурации - единственный источник правды для всех настроек
 *
 * Инкапсулирует:
 * - Чтение/запись EEPROM
 * - Валидацию всех параметров
 * - Уведомления об изменениях
 * - Доступ к настройкам через геттеры/сеттеры
 */
class ConfigManager {
 public:
  /**
   * @brief Получить экземпляр (синглтон)
   */
  static ConfigManager& getInstance();

  /**
   * @brief Инициализация (вызывается один раз в setup)
   */
  void begin();

  /**
   * @brief Получить текущую конфигурацию (только для чтения)
   */
  const ConfigData* get() const;

  /**
   * @brief Проверить валидность конфигурации
   */
  bool isValid() const;

  /**
   * @brief Получить текст последней ошибки
   */
  const char* getLastError() const;

  /**
   * @brief Записать текущую конфигурацию в EEPROM
   * @return true — успешно
   */
  bool save();

  /**
   * @brief Сбросить конфигурацию к значениям по умолчанию
   * @return true — успешно
   */
  bool reset();

  // ========================================================================
  // ГЕТТЕРЫ
  // ========================================================================

  // --- WiFi ---
  const char* getWifiSsid() const;
  const char* getWifiPassword() const;

  // --- MQTT ---
  const char* getMqttBroker() const;
  uint16_t getMqttPort() const;
  const char* getMqttUser() const;
  const char* getMqttPassword() const;
  const char* getMqttClientId() const;

  // --- Sensor (TYPE 1 и 2) ---
  uint16_t getSensorInterval() const;

  // --- Fan (TYPE 1) ---
  double getLowTemp() const;
  double getHighTemp() const;
  double getLowHum() const;
  double getHighHum() const;
  bool getSensorControlMode() const;
  uint16_t getSpeedPercent() const;
  bool getAdaptiveMode() const;

  // --- Общие (TYPE 1 и 3) ---
  int getDelaySeconds() const;
  uint32_t getMaxOnTime() const;
  bool getBootState() const;

  // --- Zigbee ---
  const char* getZigbeeNetworkKey() const;
  uint16_t getZigbeePanId() const;
  uint8_t getZigbeeChannel() const;

  // --- Device ---
  const char* getDeviceId() const;

  // ========================================================================
  // СЕТТЕРЫ (с валидацией)
  // ========================================================================

  // --- WiFi ---
  bool setWifiSsid(const char* ssid);
  bool setWifiPassword(const char* password);

  // --- MQTT ---
  bool setMqttBroker(const char* broker);
  bool setMqttPort(uint16_t port);
  bool setMqttUser(const char* user);
  bool setMqttPassword(const char* password);
  bool setMqttClientId(const char* clientId);

  // --- Sensor (TYPE 1 и 2) ---
  bool setSensorInterval(uint16_t interval);

  // --- Fan (TYPE 1) ---
  bool setLowTemp(double temp);
  bool setHighTemp(double temp);
  bool setLowHum(double hum);
  bool setHighHum(double hum);
  bool setSensorControlMode(bool enabled);
  bool setSpeedPercent(uint16_t percent);
  bool setAdaptiveMode(bool enabled);

  // --- Общие (TYPE 1 и 3) ---
  bool setDelaySeconds(int seconds);
  bool setMaxOnTime(uint32_t seconds);
  bool setBootState(bool state);

  // --- Zigbee ---
  bool setZigbeeNetworkKey(const char* key);
  bool setZigbeePanId(uint16_t panId);
  bool setZigbeeChannel(uint8_t channel);

  /**
   * @brief Вывести текущую конфигурацию в лог
   */
  void print() const;

  // ========================================================================
  // УВЕДОМЛЕНИЯ ОБ ИЗМЕНЕНИЯХ
  // ========================================================================

  using ChangeCallback = std::function<void()>;

  /**
   * @brief Зарегистрировать колбэк на изменение конфигурации
   * @param callback Функция, вызываемая при изменении любых настроек
   */
  void onConfigChanged(ChangeCallback callback);

 private:
  ConfigManager() = default;
  ~ConfigManager() = default;
  ConfigManager(const ConfigManager&) = delete;
  ConfigManager& operator=(const ConfigManager&) = delete;

  // ========================================================================
  // ВНУТРЕННИЕ МЕТОДЫ
  // ========================================================================

  void setDefaults();
  void loadFromCredentials();
  void readFromEEPROM();
  bool validateAndApply(const ConfigData& raw);
  // bool isConfigValid(const ConfigData& config) const;
  uint16_t calculateCRC(const ConfigData& config) const;
  void initDeviceId();

  // ========================================================================
  // ДАННЫЕ
  // ========================================================================

  ConfigData _config;
  bool _configValid = false;
  char _lastError[64] = "";
  char _deviceId[12] = "";
  bool _initialized = false;
  ChangeCallback _changeCallback = nullptr;

  // Константы
  static constexpr uint16_t MAGIC = MAGIC_VALUE;
  static constexpr size_t EEPROM_SIZE = sizeof(ConfigData);
};

// ============================================================================
// ГЛОБАЛЬНЫЙ ДОСТУП (для обратной совместимости)
// ============================================================================

extern ConfigManager& g_configManager;

#endif  // CONFIG_MANAGER_H