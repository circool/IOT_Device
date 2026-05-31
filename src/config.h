#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

#ifndef VERSION
  #define VERSION "1.0"
#endif

// Пин кнопки сброса настроек
#ifndef RESET_PIN
  #define RESET_PIN 0
#endif

// ======================== НАСТРОЙКИ УСТРОЙСТВА ========================

// Тип устройства
#ifndef DEVICE_TYPE
  #define DEVICE_TYPE 1           // 1 – вентилятор с датчиками, 2 – только датчик, 3 – управляемый выключатель 
#endif

extern char deviceId[12];

// Префикс устройства (для MQTT или AP SSID)
#if DEVICE_TYPE==1
  #define DEVICE_PREFIX "Fan"
#elif DEVICE_TYPE==2
  #define DEVICE_PREFIX "Sensor"
#elif DEVICE_TYPE==3
  #define DEVICE_PREFIX "Switch"
#else
  #define DEVICE_PREFIX "Device"
#endif

// Тип датчика (для вентилятора или сенсора)
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  
  #ifndef SENSOR_TYPE
    #define SENSOR_TYPE 1           // 1 – AHT10 (I2C), 2 – DHT11/22 (GPIO)
  #endif

  #if SENSOR_TYPE == 2
    #ifndef SENSOR_PIN
      #define SENSOR_PIN 2
    #endif
    
    #ifndef DHT_TYPE
      #define DHT_TYPE DHT11
    #endif
  #endif
#endif

// Исполнительный механизм (для вентилятора или выключателя)
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  // Пин управления 
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    #ifndef SWITCH_PIN
      #ifdef ESP8266
        #define SWITCH_PIN 14
      #elif defined(ESP32)
        #define SWITCH_PIN 4
      #endif
    #endif
  #endif

  // Управляюшие уровни
  #ifndef RELAY_ON_LEVEL
    #define RELAY_ON_LEVEL LOW
  #endif
  
  // TODO: Удалить - достаточно RELAY_ON_LEVEL
  // #if RELAY_ON_LEVEL == LOW
  //   #define RELAY_OFF_LEVEL HIGH
  // #else
  //   #define RELAY_OFF_LEVEL LOW
  // #endif



#endif


// ======================== ОБЩИЙ ФУНКЦИОНАЛЬНЫЙ СОСТАВ ========================

// Поддержка WIFI
#ifndef WIFI_ENABLED
  #define WIFI_ENABLED 1
#endif
void initDeviceId();

#if WIFI_ENABLED == 1

  #ifndef WIFI_OUTPUT_POWER
    #define WIFI_OUTPUT_POWER 15.0   // от 0 до 20.5, по умолчанию 15.0
  #endif


  // Поддержка Access point
  #ifndef AP_ENABLED
    #define AP_ENABLED 1
  #endif

  // Поддержка WEB
  #ifndef WEB_ENABLED
    #define WEB_ENABLED 1
  #endif

  // Поддержка MQTT
  #ifndef MQTT_ENABLED
    #define MQTT_ENABLED 1
  #endif

  // Поддержка OTA
  #ifndef OTA_ENABLED
    #define OTA_ENABLED 1
  #endif

#else
  #define WEB_ENABLED 0
  #define MQTT_ENABLED 0
  #define OTA_ENABLED 0
  #define AP_ENABLED 0
#endif

// Подчиненные параметры

// WEB
#if WEB_ENABLED == 1
  #ifndef WEB_STATUS_ENABLED 
    #define WEB_STATUS_ENABLED 1
  #endif
  
  #ifndef WEB_SHOW_RSSI 
    #define WEB_SHOW_RSSI 1
  #endif
  
  #ifndef WEB_RESET_ENABLED 
    #define WEB_RESET_ENABLED 1
  #endif

#endif

// Стабильность
#ifndef WDT_ENABLED
  #define WDT_ENABLED 1
#endif

// Индикация состояния
#ifndef STATUS_LED_PIN
  #define STATUS_LED_PIN 2
#endif

#ifndef LOG_LED
  #if DEBUG_ENABLED == 1
    #define LOG_LED 1
  #else
    #define LOG_LED 0
  #endif
#endif




#if WDT_ENABLED == 1
  
  #ifndef WDT_TIMER_MS
    #define WDT_TIMER_MS 5000               // Таймаут WDT в миллисекундах
  #endif

  #ifndef LOOP_WATCHDOG_MULTIPLIER
    #define LOOP_WATCHDOG_MULTIPLIER 3      // Множитель STATE_PUBLISH_INTERVAL_MS для watchdog
  #endif

#else
  
  #ifndef SOFT_WDT_ENABLED
    #define SOFT_WDT_ENABLED 1
  #endif

#endif



#if WIFI_ENABLED
  #ifndef WIFI_CONNECT_TIMEOUT_MS
    #define WIFI_CONNECT_TIMEOUT_MS 30000   // Таймаут подключения WiFi (30 секунд)
  #endif
#endif

// MQTT
#if MQTT_ENABLED == 1

  #ifndef MQTT_RECONNECT_DELAY_MS
    #define MQTT_RECONNECT_DELAY_MS 5000
  #endif

  //@deprecated
  #ifndef STATE_PUBLISH_INTERVAL_MS
    #define STATE_PUBLISH_INTERVAL_MS 3000
  #endif
  // TODO: Вероятно лучше эту величину вычислять из периодичности публикации
  #ifndef MQTT_KEEPALIVE_SEC
    #define MQTT_KEEPALIVE_SEC 3
  #endif

  #ifndef MQTT_RESET_ENABLED 
    #define MQTT_RESET_ENABLED 1
  #endif
  
  #ifndef MQTT_PUBLISH_RSSI 
    #define MQTT_PUBLISH_RSSI 1
  #endif
  
  #if WDT_ENABLED == 1
    #ifndef MQTT_PUBLISH_RESET_REASON 
      #define MQTT_PUBLISH_RESET_REASON 1
    #endif
  #else
    #define MQTT_PUBLISH_RESET_REASON 1
  #endif

  #if MQTT_PUBLISH_RESET_REASON == 1
    #ifndef MQTT_IGNORE_PUBLISH_NORMAL_RESET_REASONS
      #define MQTT_IGNORE_PUBLISH_NORMAL_RESET_REASONS 1
    #endif
  #endif

#else
  #define MQTT_RESET_ENABLED 0
  #define MQTT_PUBLISH_RSSI 0
  #define MQTT_PUBLISH_RESET_REASON 0
#endif

// AP
#if AP_ENABLED == 1
  
  // Значения по умолчанию
  #ifndef AP_IP_ADDRESS
    #define AP_IP_ADDRESS "192.168.4.1"
  #endif

  // Время в мсек до перехода в режим точки доступа при потере WiFi
  #ifndef AP_FALLBACK_TIMEOUT_MS
    #define AP_FALLBACK_TIMEOUT_MS 12000  
  #endif


#endif

// Отладка 
#ifndef DEBUG_ENABLED
  #define DEBUG_ENABLED 0
#endif

#if DEBUG_ENABLED == 1

  #ifndef LOG_SENSOR
    #define LOG_SENSOR 1
  #endif

  #ifndef LOG_CONFIG
    #define LOG_CONFIG 1
  #endif

  #ifndef LOG_FAN
    #define LOG_FAN 1
  #endif

  #ifndef LOG_SWITCH
    #define LOG_SWITCH 1
  #endif

  #ifndef LOG_MQTT
    #define LOG_MQTT 1
  #endif

  #ifndef LOG_WIFI
    #define LOG_WIFI 1
  #endif

  #ifndef LOG_WEB
    #define LOG_WEB 1
  #endif

  #ifndef LOG_AP
    #define LOG_AP 1
  #endif

  #ifndef LOG_OTA
    #define LOG_OTA 1
  #endif

#else
  #define LOG_SENSOR 0
  #define LOG_CONFIG 0
  #define LOG_FAN 0
  #define LOG_MQTT 0
  #define LOG_WIFI 0
  #define LOG_WEB 0
  #define LOG_AP 0
  #define LOG_OTA 0
#endif

// ======================== ЗАВИСИМЫЙ ФУНКЦИОНАЛЬНЫЙ СОСТАВ ========================

// Регулировка скорости вращения вентилятора
#if DEVICE_TYPE == 1
  
  // Задействовать управление скоростью
  #ifndef PWM_ENABLED
    #define PWM_ENABLED 1
  #endif
  
  // Значения управления скоростью по умолчанию
  #if PWM_ENABLED == 1
    
    // Частота импульсов ШИМ в Гц
    #ifndef PWM_FREQUENCY
      #define PWM_FREQUENCY 5
    #endif

    // Разрешение ШИМ
    #ifndef PWM_RESOLUTION
      #define PWM_RESOLUTION 8
    #endif
    
    // Стартовый импульс для первоначальний раскрутки в мс
    #ifndef PWM_STARTING
      #define PWM_STARTING 200      
    #endif

    // Скорость в %% по умолчанию
    #ifndef DEFAULT_SPEED_PERCENT
      #define DEFAULT_SPEED_PERCENT 50  
    #endif




  #endif
  
  // Адаптивное управлени скоростью
  #if PWM_ENABLED == 1
    #ifndef APAPTIVE_ENABLED
      #define APAPTIVE_ENABLED 1
    #endif

    // Параметры адаптивного управления по умолчанию
    #if APAPTIVE_ENABLED == 1  
      
      // Порог изменения температуры для адаптации (°C)
      #ifndef ADAPTIVE_EPSILON_TEMP
        #define ADAPTIVE_EPSILON_TEMP 0.5   
      #endif
      
      // Порог изменения влажности для адаптации (%)
      #ifndef ADAPTIVE_EPSILON_HUM
        #define ADAPTIVE_EPSILON_HUM 2.0    
      #endif
      
      // Зависимость шага адаптации от изменения влажности или температуры
      #ifndef ADAPTIVE_SPEED_SENSITIVITY
        #define ADAPTIVE_SPEED_SENSITIVITY 0.7   
      #endif
      
      // Шаг изменения скорости при адаптации (%)
      #ifndef ADAPTIVE_STEP_SIZE
        #define ADAPTIVE_STEP_SIZE 10       
      #endif

      #ifndef MIN_SPEED_PERCENT
        #define MIN_SPEED_PERCENT 1
      #endif


    #endif
  #endif

#else 
  #ifndef PWM_ENABLED
    #define PWM_ENABLED 0
  #endif
#endif

// Работа с датчиком
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  #ifndef SENSOR_DURATION
    #define SENSOR_DURATION 10
  #endif
#endif

// Работа с актуатором
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  
  // Таймер аварийного отключения
  #ifndef EMERGENCY_ENABLED
    #define EMERGENCY_ENABLED 1
  #endif

  #if EMERGENCY_ENABLED == 1 
    // Значение по умолчанию
    #ifndef MAX_ON_TIME_SEC
      #define MAX_ON_TIME_SEC 3600
    #endif
  #endif

#endif

// 

#include <EEPROM.h>

















// ======================== НАСТРОЙКИ ШИМ ДЛЯ ТИХОГО РЕЖИМА ========================
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  
  
  



  

  

 

  

  

  
   

#endif

// ======================== НАСТРОЙКИ ПОВЕДЕНИЯ ========================





// ======================== НАСТРОЙКИ СЕТИ ========================





// Интервал проверки WiFi соединения (мс)
#ifndef WIFI_CHECK_INTERVAL_MS
  #define WIFI_CHECK_INTERVAL_MS 10000
#endif



// ======================== ПОРОГИ ПО УМОЛЧАНИЮ ========================
#if DEVICE_TYPE == 1
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
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  
  #ifndef DEFAULT_DELAY_SECONDS
    #define DEFAULT_DELAY_SECONDS 60
  #endif

  #ifndef DEFAULT_ADAPTIVE_MODE
    #define DEFAULT_ADAPTIVE_MODE false   
  #endif

  #ifndef BOOT_SWITCH_STATE
    #define BOOT_SWITCH_STATE true
  #endif
#endif



// ======================== ДЛЯ ОТЛАДКИ СОСТОЯНИЯ EEPROM ========================
#ifndef MAGIC_VALUE
  #define MAGIC_VALUE 0x5A6B
#endif


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
struct Config {
  uint16_t magic;
  uint16_t crc;
  
  #if WIFI_ENABLED == 1
    char wifiSsid[32];
    char wifiPassword[64];
    float wifiOutputPower;
  #endif

  #if MQTT_ENABLED == 1
    char mqttBroker[64];
    uint16_t mqttPort;
    char mqttUser[32];
    char mqttPassword[64];
    char mqttClientId[24];
  #endif

  // Вентилятор
  #if DEVICE_TYPE == 1
    double lowHum;
    double highHum;
    double lowTemp;
    double highTemp;
    bool sensorControlMode;
    uint16_t speedPercent;
    bool adaptiveMode;
  #endif
  
  // Вентилятор или выключатель
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    int delaySeconds;   
    uint32_t maxOnTime;
    bool bootState;
  #endif
  
  // Вентилятор или сенсор
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    uint16_t sensorInterval;
  #endif

};

#if AP_ENABLED == 1
  extern bool apMode;
#endif


extern Config config;
extern Config staticConfig;
extern bool configValid;
extern String configLastError;   

void config_init();
void config_read();
void config_write();
void config_setDefaults();
uint16_t crc16(const uint8_t* data, size_t len);
void config_print();
bool config_clear();
bool config_validate();           

#endif