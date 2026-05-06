#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>
#include <EEPROM.h>

// ======================== НАСТРОЙКИ УСТРОЙСТВА ========================

// Тип устройства
#ifndef DEVICE_TYPE
  #define DEVICE_TYPE 1           // 1 – вентилятор с датчиками, 2 – только датчик, 3 – управляемый выключатель
#endif

// Тип датчика
#ifndef SENSOR_TYPE
  #define SENSOR_TYPE 1           // 1 – AHT10 (I2C), 2 – DHT11/22 (GPIO)
#endif

// Пин для DHT11/22 (если SENSOR_TYPE == 2)
#ifndef SENSOR_PIN
  #define SENSOR_PIN 2
#endif

// Тип DHT (DHT11, DHT22, DHT21)
#ifndef DHT_TYPE
  #define DHT_TYPE DHT11
#endif

// Пин управления реле / ШИМ
#ifndef SWITCH_PIN
  #ifdef ESP8266
    #define SWITCH_PIN 14
  #elif defined(ESP32)
    #define SWITCH_PIN 14
  #endif
#endif

// Пин кнопки сброса настроек
#ifndef RESET_PIN
  #define RESET_PIN 0
#endif

// ======================== НАСТРОЙКИ ШИМ ДЛЯ SLOW MODE ========================

// Частота ШИМ для Slow Mode (Гц)
#ifndef PWM_FREQUENCY
  #define PWM_FREQUENCY 10
#endif

// Разрешение ШИМ (бит) - 8 бит = 0..255
#ifndef PWM_RESOLUTION
  #define PWM_RESOLUTION 8
#endif

// Скважность Slow Mode по умолчанию (0-255)
#ifndef SLOW_MODE_DUTY_CYCLE
  #define SLOW_MODE_DUTY_CYCLE 128
#endif

// ======================== НАСТРОЙКИ ПОВЕДЕНИЯ ========================

// Интервал опроса датчика (секунд)
#ifndef SENSOR_DURATION
  #define SENSOR_DURATION 10
#endif

// Максимальное время непрерывной работы вентилятора (секунд)
#ifndef MAX_ON_TIME_SEC
  #define MAX_ON_TIME_SEC 3600
#endif

// ======================== НАСТРОЙКИ СЕТИ ========================

// IP адрес точки доступа (AP режим)
#ifndef AP_IP_ADDRESS
  #define AP_IP_ADDRESS "192.168.4.1"
#endif

// ======================== ПОРОГИ ПО УМОЛЧАНИЮ ========================

#ifndef DEFAULT_LOW_TEMP
  #define DEFAULT_LOW_TEMP 27.0
#endif

#ifndef DEFAULT_HIGH_TEMP
  #define DEFAULT_HIGH_TEMP 29.0
#endif

#ifndef DEFAULT_LOW_HUM
  #define DEFAULT_LOW_HUM 60.0
#endif

#ifndef DEFAULT_HIGH_HUM
  #define DEFAULT_HIGH_HUM 70.0
#endif

#ifndef DEFAULT_DELAY_SECONDS
  #define DEFAULT_DELAY_SECONDS 60
#endif

#ifndef DEFAULT_AUTOMATIC_MODE
  #define DEFAULT_AUTOMATIC_MODE true
#endif

#ifndef DEFAULT_SLOW_MODE
  #define DEFAULT_SLOW_MODE false
#endif

#ifndef DEFAULT_SET_SWITCH_OFF
  #define DEFAULT_SET_SWITCH_OFF true
#endif

// ======================== НАСТРОЙКИ MQTT ========================

#ifndef STATE_PUBLISH_INTERVAL_MS
  #define STATE_PUBLISH_INTERVAL_MS 3000
#endif

#ifndef MQTT_KEEPALIVE_SEC
  #define MQTT_KEEPALIVE_SEC 4
#endif

#ifndef MQTT_RECONNECT_DELAY_MS
  #define MQTT_RECONNECT_DELAY_MS 5000
#endif

#ifndef WIFI_CHECK_INTERVAL_MS
  #define WIFI_CHECK_INTERVAL_MS 10000
#endif

#ifndef AP_FALLBACK_TIMEOUT_MS
  #define AP_FALLBACK_TIMEOUT_MS 120000
#endif

// ======================== ОТЛАДКА ========================

// Раскомментировать для включения отладки
// #define DEBUG_ENABLE
// #define DEBUG_MQTT
// #define DEBUG_EEPROM

// ======================== ПОДКЛЮЧЕНИЕ CREDENTIALS ========================

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

// ======================== СТРУКТУРА КОНФИГУРАЦИИ ========================

struct ScheduleEntry {
  uint8_t hour;
  uint8_t minute;
  bool state;
};

struct Config {
  uint16_t magic;
  uint16_t crc;
  char wifiSsid[32];
  char wifiPassword[64];
  char mqttBroker[64];
  uint16_t mqttPort;
  char mqttUser[32];
  char mqttPassword[64];
  char mqttClientId[24];
  double lowHum;
  double highHum;
  double lowTemp;
  double highTemp;
  int delaySeconds;
  bool automaticMode;
  bool slowModeEnabled;
  uint16_t slowModeDuty;
  uint16_t sensorInterval;
  uint32_t maxOnTime;
  bool setSwitchOff;
  uint8_t scheduleCount;
  ScheduleEntry schedule[10];
  uint8_t reserved[31];
};

extern Config config;
extern bool configValid;
extern bool apMode;

void config_init();
void config_read();
void config_write();
void config_setDefaults();
bool config_isValid();
uint16_t crc16(const uint8_t* data, size_t len);
void config_print();
void config_clear();
void config_loadFromCredentials();

#endif