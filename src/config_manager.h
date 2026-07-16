/**
 * @file config_manager.h
 * @brief Менеджер конфигурации — единственный источник правды для всех настроек
 * @details Хранит настройки в EEPROM, обеспечивает валидацию и доступ.
 *
 * @note Для ESP8266: используется EEPROM для совместимости с обеими
 * платформами. Синглтон с прямым доступом — осознанное отступление для экономии
 * RAM.
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <Arduino.h>
#include <EEPROM.h>
#include "settings.h"

// ============================================================================
// КОНСТАНТЫ
// ============================================================================

// /**
//  * @brief Префикс устройства (для MQTT топиков и AP SSID)
//  * @details Формируется автоматически на основе DEVICE_TYPE
//  */
// #if DEVICE_TYPE == 1
// #define DEVICE_PREFIX "fan"
// #elif DEVICE_TYPE == 2
// #define DEVICE_PREFIX "sensor"
// #elif DEVICE_TYPE == 3
// #define DEVICE_PREFIX "switch"
// #else
// #define DEVICE_PREFIX "device"
// #endif




// ============================================================================
// ИСПОЛНИТЕЛЬНЫЙ МЕХАНИЗМ (для TYPE 1 и 3)
// ============================================================================

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

// ============================================================================
// ДИАПАЗОНЫ ДЛЯ ВАЛИДАЦИИ
// ============================================================================

/** @brief Минимальная допустимая температура (°C) */
#ifndef TEMP_MIN
#define TEMP_MIN -40.0f
#endif

/** @brief Максимальная допустимая температура (°C) */
#ifndef TEMP_MAX
#define TEMP_MAX 85.0f
#endif

/** @brief Минимальная допустимая влажность (%) */
#ifndef HUM_MIN
#define HUM_MIN 0.0f
#endif

/** @brief Максимальная допустимая влажность (%) */
#ifndef HUM_MAX
#define HUM_MAX 100.0f
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
/** @brief Нижний порог температуры по умолчанию (°C) */
#ifndef DEFAULT_LOW_TEMP
#define DEFAULT_LOW_TEMP 27.0f
#endif

/** @brief Верхний порог температуры по умолчанию (°C) */
#ifndef DEFAULT_HIGH_TEMP
#define DEFAULT_HIGH_TEMP 29.0f
#endif

/** @brief Нижний порог влажности по умолчанию (%) */
#ifndef DEFAULT_LOW_HUM
#define DEFAULT_LOW_HUM 55.0f
#endif

/** @brief Верхний порог влажности по умолчанию (%) */
#ifndef DEFAULT_HIGH_HUM
#define DEFAULT_HIGH_HUM 60.0f
#endif

/** @brief Режим управления сенсором по умолчанию */
#ifndef DEFAULT_SENSOR_CONTROL_MODE
#define DEFAULT_SENSOR_CONTROL_MODE true
#endif
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
/** @brief Задержка отложенного включения по умолчанию (сек) */
#ifndef DEFAULT_DELAY_SECONDS
#define DEFAULT_DELAY_SECONDS 60
#endif

/** @brief Адаптивный режим по умолчанию */
#ifndef DEFAULT_ADAPTIVE_MODE
#define DEFAULT_ADAPTIVE_MODE false
#endif

/** @brief Состояние при старте по умолчанию */
#ifndef DEFAULT_BOOT_SWITCH_STATE
#define DEFAULT_BOOT_SWITCH_STATE true
#endif
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
/** @brief Интервал опроса датчика по умолчанию (сек) */
#ifndef DEFAULT_SENSOR_DURATION
#define DEFAULT_SENSOR_DURATION 10
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

/** @brief Размер EEPROM в байтах */
#ifndef EEPROM_SIZE
#define EEPROM_SIZE sizeof(ConfigData)
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












// ============================================================================
// СТРУКТУРА КОНФИГУРАЦИИ
// ============================================================================

/**
 * @brief Структура конфигурации, сохраняемая в EEPROM
 * @note Все поля выровнены для минимизации размера
 */
struct ConfigData {
  uint16_t magic; /**< Магическое число для проверки валидности */
  uint16_t crc;   /**< CRC16 от всей структуры (кроме самого поля crc) */

  // ========== WiFi ==========
#if FEATURE_WIFI_ENABLED == 1
  char wifiSsid[32];     /**< Имя WiFi сети (SSID) */
  char wifiPassword[64]; /**< Пароль WiFi */
#endif

  // ========== MQTT ==========
#if FEATURE_MQTT_ENABLED == 1
  char mqttBroker[64];   /**< Адрес MQTT брокера */
  uint16_t mqttPort;     /**< Порт MQTT брокера */
  char mqttUser[32];     /**< Имя пользователя MQTT */
  char mqttPassword[64]; /**< Пароль MQTT */
  char mqttClientId[24]; /**< Уникальный ID клиента */
#endif

  // ========== Вентилятор (TYPE 1) ==========
#if DEVICE_TYPE == 1
  float lowHum;           /**< Нижний порог влажности (%) */
  float highHum;          /**< Верхний порог влажности (%) */
  float lowTemp;          /**< Нижний порог температуры (°C) */
  float highTemp;         /**< Верхний порог температуры (°C) */
  bool sensorControlMode; /**< Режим управления сенсором (true=авто) */
  uint16_t speedPercent;  /**< Скорость вентилятора (0-100%) */
  bool adaptiveMode;      /**< Адаптивный тихий режим */
#endif

  // ========== Общие для TYPE 1 и TYPE 3 ==========
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  int delaySeconds;   /**< Задержка отложенного включения (сек) */
  uint32_t maxOnTime; /**< Таймер аварийного отключения (сек) */
  bool bootState;     /**< Состояние при старте (true=включено) */
#endif

  // ========== Датчик (TYPE 1 и TYPE 2) ==========
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  uint16_t sensorInterval; /**< Интервал опроса датчика (сек) */
#endif

  // ========== Zigbee ==========
#if FEATURE_ZIGBEE_ENABLED == 1
  char zigbeeNetworkKey[32]; /**< Сетевой ключ Zigbee */
  uint16_t zigbeePanId;      /**< PAN ID Zigbee сети */
  uint8_t zigbeeChannel;     /**< Канал Zigbee (11-26) */
#endif
};

// ============================================================================
// КЛАСС CONFIG MANAGER
// ============================================================================

/**
 * @brief Синглтон-менеджер конфигурации
 * @details Единственный источник правды для всех настроек.
 *          Инкапсулирует чтение/запись EEPROM и валидацию.
 *
 * @note Синглтон с прямым доступом — осознанное отступление для embedded:
 *       - Экономит RAM (нет лишних указателей)
 *       - Упрощает доступ из любого места
 *       - Приемлемо для проекта такого размера
 */
class ConfigManager {
 public:
  // ========================================================================
  // ДОСТУП К ЭКЗЕМПЛЯРУ
  // ========================================================================

  /**
   * @brief Получить экземпляр синглтона
   * @return Ссылка на единственный экземпляр
   */
  static ConfigManager& getInstance();

  // ========================================================================
  // УПРАВЛЕНИЕ
  // ========================================================================

  

  /**
   * @brief Инициализация менеджера
   * @details Вызывается один раз в setup().
   *          Читает конфигурацию из EEPROM.
   */
  void init();

  /**
   * @brief Получить текущую конфигурацию (только для чтения)
   * @return Указатель на структуру ConfigData
   */
  const ConfigData* get() const;

  /**
   * @brief Проверить валидность конфигурации
   * @return true — конфигурация валидна (есть в EEPROM и CRC совпадает)
   */
  bool isValid() const;

  /**
   * @brief Получить текст последней ошибки
   * @return Строка с описанием ошибки или пустая строка
   */
  const char* getLastError() const;

  /**
   * @brief Сохранить текущую конфигурацию в EEPROM
   * @return true — успешно, false — ошибка записи или верификации
   */
  bool save();

  /**
   * @brief Сбросить конфигурацию и очистить EEPROM
   * @return true — успешно, false — ошибка стирания EEPROM
   */
  bool reset();

  /**
   * @brief Вывести текущую конфигурацию в лог
   */
  void print() const;

  // ========================================================================
  // ГЕТТЕРЫ
  // ========================================================================

  /** @brief Получить SSID WiFi */
  const char* getWifiSsid() const;

  /** @brief Получить пароль WiFi */
  const char* getWifiPassword() const;

  /** @brief Получить адрес MQTT брокера */
  const char* getMqttBroker() const;

  /** @brief Получить порт MQTT брокера */
  uint16_t getMqttPort() const;

  /** @brief Получить имя пользователя MQTT */
  const char* getMqttUser() const;

  /** @brief Получить пароль MQTT */
  const char* getMqttPassword() const;

  /** @brief Получить Client ID MQTT */
  const char* getMqttClientId() const;

  /** @brief Получить интервал опроса датчика (сек) */
  uint16_t getSensorInterval() const;

  /** @brief Получить нижний порог температуры (°C) */
  double getLowTemp() const;

  /** @brief Получить верхний порог температуры (°C) */
  double getHighTemp() const;

  /** @brief Получить нижний порог влажности (%) */
  double getLowHum() const;

  /** @brief Получить верхний порог влажности (%) */
  double getHighHum() const;

  /** @brief Получить состояние сенсорного режима (true=авто) */
  bool getSensorControlMode() const;

  /** @brief Получить скорость вентилятора (0-100%) */
  uint16_t getSpeedPercent() const;

  /** @brief Получить состояние адаптивного режима */
  bool getAdaptiveMode() const;

  /** @brief Получить задержку отложенного включения (сек) */
  int getDelaySeconds() const;

  /** @brief Получить таймер аварийного отключения (сек) */
  uint32_t getMaxOnTime() const;

  /** @brief Получить состояние при старте (true=включено) */
  bool getBootState() const;

  /** @brief Получить сетевой ключ Zigbee */
  const char* getZigbeeNetworkKey() const;

  /** @brief Получить PAN ID Zigbee */
  uint16_t getZigbeePanId() const;

  /** @brief Получить канал Zigbee (11-26) */
  uint8_t getZigbeeChannel() const;

  /** @brief Получить ID устройства (например, "fan_1A2B") */
  const char* getDeviceId() const;

  // ========================================================================
  // СЕТТЕРЫ (с валидацией)
  // ========================================================================

  /**
   * @brief Установить SSID WiFi
   * @param ssid Имя сети (не может быть пустым)
   * @return true — успешно, false — ошибка валидации
   */
  bool setWifiSsid(const char* ssid);

  /**
   * @brief Установить пароль WiFi
   * @param password Пароль (может быть пустым для открытых сетей)
   * @return true — успешно, false — ошибка валидации
   */
  bool setWifiPassword(const char* password);

  /**
   * @brief Установить адрес MQTT брокера
   * @param broker IP или домен (не может быть пустым)
   * @return true — успешно, false — ошибка валидации
   */
  bool setMqttBroker(const char* broker);

  /**
   * @brief Установить порт MQTT брокера
   * @param port Порт (1-65535)
   * @return true — успешно, false — ошибка валидации
   */
  bool setMqttPort(uint16_t port);

  /**
   * @brief Установить имя пользователя MQTT
   * @param user Имя пользователя (может быть пустым)
   * @return true — успешно, false — ошибка валидации
   */
  bool setMqttUser(const char* user);

  /**
   * @brief Установить пароль MQTT
   * @param password Пароль (может быть пустым)
   * @return true — успешно, false — ошибка валидации
   */
  bool setMqttPassword(const char* password);

  /**
   * @brief Установить Client ID MQTT
   * @param clientId Уникальный ID (только a-z, A-Z, 0-9, _, -)
   * @return true — успешно, false — ошибка валидации
   */
  bool setMqttClientId(const char* clientId);

  /**
   * @brief Установить интервал опроса датчика
   * @param interval Секунды (1-3600)
   * @return true — успешно, false — ошибка валидации
   * @deprecated Нет необходимости установки этого значения
   */
  // bool setSensorInterval(uint16_t interval);

  /**
   * @brief Установить нижний порог температуры
   * @param temp Температура в °C (-40..85)
   * @return true — успешно, false — ошибка валидации
   */
  bool setLowTemp(double temp);

  /**
   * @brief Установить верхний порог температуры
   * @param temp Температура в °C (-40..85)
   * @return true — успешно, false — ошибка валидации
   */
  bool setHighTemp(double temp);

  /**
   * @brief Установить нижний порог влажности
   * @param hum Влажность в % (0..100)
   * @return true — успешно, false — ошибка валидации
   */
  bool setLowHum(double hum);

  /**
   * @brief Установить верхний порог влажности
   * @param hum Влажность в % (0..100)
   * @return true — успешно, false — ошибка валидации
   */
  bool setHighHum(double hum);

  /**
   * @brief Включить/выключить сенсорный режим
   * @param enabled true — авто, false — ручной
   * @return true — успешно, false — ошибка валидации
   */
  bool setSensorControlMode(bool enabled);

  /**
   * @brief Установить скорость вентилятора
   * @param percent 0-100%
   * @return true — успешно, false — ошибка валидации
   */
  bool setSpeedPercent(uint16_t percent);

  /**
   * @brief Включить/выключить адаптивный режим
   * @param enabled true — включён
   * @return true — успешно, false — ошибка валидации
   */
  bool setAdaptiveMode(bool enabled);

  /**
   * @brief Установить задержку отложенного включения
   * @param seconds 0-86400 (0 = отключено)
   * @return true — успешно, false — ошибка валидации
   */
  bool setDelaySeconds(int seconds);

  /**
   * @brief Установить таймер аварийного отключения
   * @param seconds 0-86400 (0 = отключено)
   * @return true — успешно, false — ошибка валидации
   */
  bool setMaxOnTime(uint32_t seconds);

  /**
   * @brief Установить состояние при старте
   * @param state true — включено после перезагрузки
   * @return true — успешно
   */
  bool setBootState(bool state);

  /**
   * @brief Установить сетевой ключ Zigbee
   * @param key Строка с ключом
   * @return true — успешно, false — ошибка валидации
   */
  bool setZigbeeNetworkKey(const char* key);

  /**
   * @brief Установить PAN ID Zigbee
   * @param panId 0-65535
   * @return true — успешно
   */
  bool setZigbeePanId(uint16_t panId);

  /**
   * @brief Установить канал Zigbee
   * @param channel 11-26
   * @return true — успешно, false — ошибка валидации
   */
  bool setZigbeeChannel(uint8_t channel);

  

 private:
  // ========================================================================
  // КОНСТРУКТОРЫ (закрытые для синглтона)
  // ========================================================================

  ConfigManager() = default;
  ~ConfigManager() = default;
  ConfigManager(const ConfigManager&) = delete;
  ConfigManager& operator=(const ConfigManager&) = delete;

  // ========================================================================
  // ВНУТРЕННИЕ МЕТОДЫ
  // ========================================================================

  /**
   * @brief Установить значения по умолчанию
   */
  void setDefaults();

  /**
   * @brief Загрузить заводские настройки из credentials.h
   */
  void loadFromCredentials();

  /**
   * @brief Прочитать конфигурацию из EEPROM
   */
  void readFromEEPROM();

  /**
   * @brief Применить прочитанную конфигурацию через сеттеры
   * @param raw Сырые данные из EEPROM
   * @return true — все поля успешно применены
   * @deprecated - избыточность и дублирование
   */
  bool validateAndApply(const ConfigData& raw);

  /**
   * @brief Рассчитать CRC16 для конфигурации
   * @param config Структура конфигурации
   * @return 16-битный CRC
   */
  uint16_t calculateCRC(const ConfigData& config) const;

  /**
   * @brief Сгенерировать ID устройства на основе MAC-адреса
   */
  void initDeviceId();

  /**
   * @brief Установить текст последней ошибки
   * @param msg Текст ошибки
   */
  void setError(const char* msg);

  // ========================================================================
  // ДАННЫЕ
  // ========================================================================

  ConfigData _config;        /**< Текущая конфигурация */
  bool _configValid = false; /**< Флаг валидности */
  char _lastError[64] = "";  /**< Текст последней ошибки */
  char _deviceId[12] = "";   /**< ID устройства (например, "fan_1A2B") */
  bool _initialized = false; /**< Флаг инициализации */
};

// ============================================================================
// ГЛОБАЛЬНЫЙ ДОСТУП (для обратной совместимости)
// ============================================================================

/**
 * @brief Глобальный экземпляр ConfigManager для упрощённого доступа
 * @note Embedded-отступление: прямой доступ к синглтону экономит RAM
 *       и упрощает код на мелких проектах
 */
extern ConfigManager& g_configManager;

#endif  // CONFIG_MANAGER_H