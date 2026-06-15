// ============================================================================
// @file config.h
// @brief Конфигурация устройства: хранение, загрузка, валидация
// ============================================================================

#ifndef CONFIG_H
#define CONFIG_H
#include <Arduino.h>
#include <EEPROM.h>

// ============================================================================
// ВЕРСИЯ ПРОШИВКИ
// ============================================================================

#ifndef VERSION
#define VERSION "1.0"
#endif

// ============================================================================
// ТИП УСТРОЙСТВА
// ============================================================================

#ifndef DEVICE_TYPE
#define DEVICE_TYPE 1
#endif

// ============================================================================
// АППАРАТНАЯ КОНФИГУРАЦИЯ
// ============================================================================

#ifndef RESET_PIN
#define RESET_PIN 0
#endif

#ifndef STATUS_LED_PIN
#define STATUS_LED_PIN 0
#endif

#ifndef I2C_SDA_PIN
#ifdef ESP8266
#define I2C_SDA_PIN 4
#elif defined(ESP32)
#define I2C_SDA_PIN 21
#endif
#endif

#ifndef I2C_SCL_PIN
#ifdef ESP8266
#define I2C_SCL_PIN 5
#elif defined(ESP32)
#define I2C_SCL_PIN 22
#endif
#endif

#ifndef SWITCH_PIN
#ifdef ESP8266
#define SWITCH_PIN 14
#elif defined(ESP32)
#define SWITCH_PIN 4
#endif
#endif

#ifndef ACTIVE_LEVEL
#define ACTIVE_LEVEL LOW
#endif

// ============================================================================
// ПРЕФИКС УСТРОЙСТВА
// ============================================================================

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
// ТИП ДАТЧИКА
// ============================================================================

#ifndef SENSOR_TYPE
#define SENSOR_TYPE 1
#endif

#if SENSOR_TYPE == 2
#ifndef SENSOR_PIN
#ifdef ESP8266
#define SENSOR_PIN 4
#elif defined(ESP32)
#define SENSOR_PIN 16
#endif
#endif
#endif

// ============================================================================
// ФУНКЦИОНАЛЬНЫЕ ВОЗМОЖНОСТИ
// ============================================================================

#ifndef WIFI_ENABLED
#define WIFI_ENABLED 1
#endif

#ifndef SCANING_WIFI_ENABLED
#define SCANING_WIFI_ENABLED 0
#endif

#ifndef AP_ENABLED
#define AP_ENABLED 1
#endif

#ifndef WEB_STATUS_ENABLED
#define WEB_STATUS_ENABLED 1
#endif

#ifndef MQTT_ENABLED
#define MQTT_ENABLED 1
#endif

#ifndef OTA_ENABLED
#define OTA_ENABLED 1
#endif

#ifndef WDT_ENABLED
#define WDT_ENABLED 1
#endif

#ifndef PWM_ENABLED
#define PWM_ENABLED 1
#endif

#ifndef ADAPTIVE_ENABLED
#define ADAPTIVE_ENABLED 1
#endif

#ifndef EMERGENCY_ENABLED
#define EMERGENCY_ENABLED 1
#endif

// ============================================================================
// ПАРАМЕТРЫ WDT
// ============================================================================

#ifndef WDT_TIMER_MS
#define WDT_TIMER_MS 5000
#endif

// ============================================================================
// ПАРАМЕТРЫ ШИМ
// ============================================================================

#ifndef PWM_FREQUENCY
#define PWM_FREQUENCY 5
#endif

#ifndef PWM_RESOLUTION
#define PWM_RESOLUTION 8
#endif

#ifndef PWM_STARTING
#define PWM_STARTING 200
#endif

// ============================================================================
// ПАРАМЕТРЫ АДАПТАЦИИ
// ============================================================================

#ifndef ADAPTIVE_EPSILON_TEMP
#define ADAPTIVE_EPSILON_TEMP 0.5
#endif

#ifndef ADAPTIVE_EPSILON_HUM
#define ADAPTIVE_EPSILON_HUM 2.0
#endif

#ifndef ADAPTIVE_SPEED_SENSITIVITY
#define ADAPTIVE_SPEED_SENSITIVITY 0.7
#endif

#ifndef ADAPTIVE_STEP_SIZE
#define ADAPTIVE_STEP_SIZE 10
#endif

#ifndef MIN_SPEED_PERCENT
#define MIN_SPEED_PERCENT 1
#endif

// ============================================================================
// ПАРАМЕТРЫ ЛОГИРОВАНИЯ
// ============================================================================

#ifndef LOG_LEVEL
#define LOG_LEVEL 3
#endif

#ifndef LOG_CATEGORIES
#define LOG_CATEGORIES 0xFFFF
#endif

#ifndef LOG_USE_COLOR
#define LOG_USE_COLOR 1
#endif

// ============================================================================
// ПАРАМЕТРЫ MQTT
// ============================================================================

#ifndef MQTT_RECONNECT_DELAY_MS
#define MQTT_RECONNECT_DELAY_MS 5000
#endif

#ifndef STATE_PUBLISH_INTERVAL_MS
#define STATE_PUBLISH_INTERVAL_MS 3000
#endif

#ifndef MQTT_KEEPALIVE_SEC
#define MQTT_KEEPALIVE_SEC 3
#endif

#ifndef MQTT_RESET_ENABLED
#define MQTT_RESET_ENABLED 1
#endif

#ifndef MQTT_PUBLISH_RSSI
#define MQTT_PUBLISH_RSSI 1
#endif

#ifndef MQTT_PUBLISH_VERSION
#define MQTT_PUBLISH_VERSION 1
#endif

#ifndef MQTT_PUBLISH_RESET_REASON
#define MQTT_PUBLISH_RESET_REASON 1
#endif

// ============================================================================
// ПАРАМЕТРЫ AP
// ============================================================================

#ifndef AP_IP_ADDRESS
#define AP_IP_ADDRESS "192.168.4.1"
#endif

#ifndef AP_FALLBACK_TIMEOUT_MS
#define AP_FALLBACK_TIMEOUT_MS 12000
#endif

#ifndef WIFI_CONNECT_TIMEOUT_MS
#define WIFI_CONNECT_TIMEOUT_MS 30000
#endif

// ============================================================================
// ПАРАМЕТРЫ WEB
// ============================================================================

#ifndef WEB_STATUS_ENABLED
#define WEB_STATUS_ENABLED 1
#endif

#ifndef WEB_SHOW_RSSI
#define WEB_SHOW_RSSI 1
#endif

#ifndef DEFAULT_WEB_REFRESH
#define DEFAULT_WEB_REFRESH 5
#endif

// ============================================================================
// МАКСИМАЛЬНЫЕ ДЛИНЫ
// ============================================================================

#ifndef DEVICE_ID_MAX_LEN
#define DEVICE_ID_MAX_LEN 24
#endif

#ifndef WIFI_SSID_MAX_LEN
#define WIFI_SSID_MAX_LEN 32
#endif

#ifndef WIFI_PASSWORD_MAX_LEN
#define WIFI_PASSWORD_MAX_LEN 64
#endif

#ifndef MQTT_BROKER_MAX_LEN
#define MQTT_BROKER_MAX_LEN 64
#endif

#ifndef MQTT_USER_MAX_LEN
#define MQTT_USER_MAX_LEN 32
#endif

#ifndef MQTT_PASSWORD_MAX_LEN
#define MQTT_PASSWORD_MAX_LEN 64
#endif

// ============================================================================
// ДИАПАЗОНЫ ВАЛИДАЦИИ
// ============================================================================

#ifndef TEMP_MIN
#define TEMP_MIN -40.0
#endif

#ifndef TEMP_MAX
#define TEMP_MAX 85.0
#endif

#ifndef HUM_MIN
#define HUM_MIN 0.0
#endif

#ifndef HUM_MAX
#define HUM_MAX 100.0
#endif

#ifndef SENSOR_INTERVAL_MIN
#define SENSOR_INTERVAL_MIN 1
#endif

#ifndef SENSOR_INTERVAL_MAX
#define SENSOR_INTERVAL_MAX 3600
#endif

#ifndef DELAY_SECONDS_MIN
#define DELAY_SECONDS_MIN 0
#endif

#ifndef DELAY_SECONDS_MAX
#define DELAY_SECONDS_MAX 86400
#endif

#ifndef MAX_ON_TIME_MIN
#define MAX_ON_TIME_MIN 0
#endif

#ifndef MAX_ON_TIME_MAX
#define MAX_ON_TIME_MAX 86400
#endif

#ifndef SPEED_PERCENT_MIN
#define SPEED_PERCENT_MIN 0
#endif

#ifndef SPEED_PERCENT_MAX
#define SPEED_PERCENT_MAX 100
#endif

// ============================================================================
// ЗНАЧЕНИЯ ПО УМОЛЧАНИЮ
// ============================================================================

#ifndef DEFAULT_WIFI_SSID
#define DEFAULT_WIFI_SSID ""
#endif

#ifndef DEFAULT_WIFI_PASSWORD
#define DEFAULT_WIFI_PASSWORD ""
#endif

#ifndef DEFAULT_MQTT_BROKER
#define DEFAULT_MQTT_BROKER ""
#endif

#ifndef DEFAULT_MQTT_PORT
#define DEFAULT_MQTT_PORT 1883
#endif

#ifndef DEFAULT_MQTT_USER
#define DEFAULT_MQTT_USER ""
#endif

#ifndef DEFAULT_MQTT_PASSWORD
#define DEFAULT_MQTT_PASSWORD ""
#endif

#ifndef DEFAULT_SENSOR_INTERVAL
#define DEFAULT_SENSOR_INTERVAL 10
#endif

#ifndef DEFAULT_LOW_TEMP
#define DEFAULT_LOW_TEMP 27.0
#endif

#ifndef DEFAULT_HIGH_TEMP
#define DEFAULT_HIGH_TEMP 29.0
#endif

#ifndef DEFAULT_LOW_HUM
#define DEFAULT_LOW_HUM 55.0
#endif

#ifndef DEFAULT_HIGH_HUM
#define DEFAULT_HIGH_HUM 60.0
#endif

#ifndef DEFAULT_SENSOR_CONTROL_MODE
#define DEFAULT_SENSOR_CONTROL_MODE true
#endif

#ifndef DEFAULT_SPEED_PERCENT
#define DEFAULT_SPEED_PERCENT 50
#endif

#ifndef DEFAULT_ADAPTIVE_MODE
#define DEFAULT_ADAPTIVE_MODE false
#endif

#ifndef DEFAULT_DELAY_SECONDS
#define DEFAULT_DELAY_SECONDS 60
#endif

#ifndef DEFAULT_MAX_ON_TIME
#define DEFAULT_MAX_ON_TIME 3600
#endif

#ifndef DEFAULT_BOOT_STATE
#define DEFAULT_BOOT_STATE true
#endif

// ============================================================================
// CREDENTIALS
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

#ifndef SSID_NAME
#define SSID_NAME ""
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

#ifndef MQTT_ADDRESS
#define MQTT_ADDRESS ""
#endif

#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

#ifndef MQTT_USER
#define MQTT_USER ""
#endif

#ifndef MQTT_PASSWORD
#define MQTT_PASSWORD ""
#endif

// ============================================================================
// ЗАГЛУШКИ ДЛЯ ПОЛЕЙ WIFI (когда WIFI_ENABLED == 0)
// ============================================================================


// ============================================================================
// EEPROM
// ============================================================================

#ifndef MAGIC_VALUE
#define MAGIC_VALUE 0x5A6B
#endif

// ============================================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// ============================================================================
extern bool apMode;

// ============================================================================
// ТИП ТРАНСПОРТА (MQTT/Zigbee/Matter)
// ============================================================================

enum TransportType {
  TRANSPORT_NONE = 0,
  TRANSPORT_MQTT = 1,
  TRANSPORT_ZIGBEE = 2,
  TRANSPORT_MATTER = 3
};

#ifndef TRANSPORT_TYPE
#define TRANSPORT_TYPE TRANSPORT_NONE
#endif

// ============================================================================
// СТРУКТУРА КОНФИГУРАЦИИ
// ============================================================================

// ============================================================================
// СТРУКТУРА КОНФИГУРАЦИИ
// ============================================================================

struct Config {
  // ========== СИСТЕМНЫЕ ПОЛЯ (всегда есть) ==========
  uint16_t magic;                    //!< Магическое число (MAGIC_VALUE)
  uint16_t crc;                      //!< CRC16 от всех полей
  char deviceId[DEVICE_ID_MAX_LEN];  //!< Уникальный ID устройства

  // ========== WIFI ПОЛЯ (всегда есть) ==========
  char wifiSsid[WIFI_SSID_MAX_LEN];          //!< Имя WiFi сети
  char wifiPassword[WIFI_PASSWORD_MAX_LEN];  //!< Пароль WiFi

  // ========== ТРАНСПОРТНЫЕ ПОЛЯ (зависят от TRANSPORT_TYPE) ==========
#if TRANSPORT_TYPE == TRANSPORT_MQTT
  char mqttBroker[MQTT_BROKER_MAX_LEN];      //!< Адрес MQTT брокера
  uint16_t mqttPort;                         //!< Порт MQTT брокера (1-65535)
  char mqttUser[MQTT_USER_MAX_LEN];          //!< Имя пользователя MQTT
  char mqttPassword[MQTT_PASSWORD_MAX_LEN];  //!< Пароль MQTT
#elif TRANSPORT_TYPE == TRANSPORT_ZIGBEE
  uint16_t zigbeePanId;     //!< Zigbee PAN ID
  uint8_t zigbeeChannel;    //!< Zigbee канал (11-26)
  char zigbeeDeviceId[16];  //!< Zigbee идентификатор устройства
#elif TRANSPORT_TYPE == TRANSPORT_MATTER
  char matterSetupCode[32];    //!< Matter setup code
  char matterVendorName[32];   //!< Имя вендора
  char matterProductName[32];  //!< Имя продукта
#endif

  // ========== НАСТРОЙКИ ДАТЧИКА (TYPE 1 и 2) ==========
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  uint16_t sensorInterval;  //!< Интервал опроса датчика (сек)
#endif

  // ========== НАСТРОЙКИ ВЕНТИЛЯТОРА (TYPE 1) ==========
#if DEVICE_TYPE == 1
  double lowHum;           //!< Нижний порог влажности (%)
  double highHum;          //!< Верхний порог влажности (%)
  double lowTemp;          //!< Нижний порог температуры (°C)
  double highTemp;         //!< Верхний порог температуры (°C)
  bool sensorControlMode;  //!< Режим управления сенсором
  uint16_t speedPercent;   //!< Скорость вентилятора (%)
  bool adaptiveMode;       //!< Адаптивный тихий режим
#endif

  // ========== ОБЩИЕ НАСТРОЙКИ (TYPE 1 и 3) ==========
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  int delaySeconds;    //!< Задержка отложенного включения (сек)
  uint32_t maxOnTime;  //!< Таймер аварийного отключения (сек)
  bool bootState;      //!< Состояние при старте
#endif

  // ========== РАНТАЙМ-ПОЛЯ (НЕ СОХРАНЯЮТСЯ) ==========
  bool valid;      //!< Флаг валидности конфига
  char error[64];  //!< Текст ошибки (при valid == false)

  Config();
};
// ============================================================================
// ПУБЛИЧНЫЕ ФУНКЦИИ
// ============================================================================

void config_init();
Config config_load();
bool config_save(const Config& cfg);
Config config_getDefaults();
static bool isValidString(const char* str, bool allowEmpty);
bool config_validate(Config& cfg);
uint16_t crc16(const uint8_t* data, size_t len);
void config_print(const Config& cfg);

// ============================================================================
// СЕТТЕРЫ С ВАЛИДАЦИЕЙ И ЛОГИРОВАНИЕМ
// ============================================================================

bool config_setDeviceId(const char* deviceId);
bool config_setWifiSsid(const char* ssid);
bool config_setWifiPassword(const char* password);

#if MQTT_ENABLED == 1
bool config_setMqttBroker(const char* broker);
bool config_setMqttPort(uint16_t port);
bool config_setMqttUser(const char* user);
bool config_setMqttPassword(const char* password);
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
bool config_setDelaySeconds(int seconds);
bool config_setMaxOnTime(uint32_t seconds);
bool config_setBootState(bool state);
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
bool config_setSensorInterval(uint16_t interval);
#endif

#if DEVICE_TYPE == 1
bool config_setLowTemp(double temp);
bool config_setHighTemp(double temp);
bool config_setLowHum(double hum);
bool config_setHighHum(double hum);
bool config_setSensorControlMode(bool enabled);
bool config_setSpeedPercent(uint16_t percent);
bool config_setAdaptiveMode(bool enabled);
#endif



#endif  // CONFIG_H