// ============================================================================
// @file config.h
// @brief Конфигурация устройства: хранение, валидация, доступ к параметрам
//
// Управляет энергонезависимой памятью (EEPROM), хранит структуру Config,
// обеспечивает валидацию всех параметров через сеттеры.
//
// Приоритет настроек (от высшего к низшему):
//   1. EEPROM (пользовательские настройки)
//   2. credentials.h (заводские настройки, если есть)
//   3. значения по умолчанию (дефайны в этом файле)
// ============================================================================

#ifndef CONFIG_H
#define CONFIG_H

#include <EEPROM.h>

// ============================================================================
// ВЕРСИЯ ПРОШИВКИ
// ============================================================================
#ifndef VERSION
#define VERSION "1.0"
#endif

// ============================================================================
// АППАРАТНАЯ КОНФИГУРАЦИЯ
// ============================================================================

/** @brief Пин кнопки сброса настроек (GPIO0 обычно) */
#ifndef RESET_PIN
#define RESET_PIN 0
#endif

/** @brief Пин светодиода индикации (0 = отключён) */
#ifndef STATUS_LED_PIN
#define STATUS_LED_PIN 0
#endif

// ============================================================================
// ТИП УСТРОЙСТВА
// ============================================================================

/**
 * @brief Тип устройства
 * @values 1 – вентилятор с датчиками
 *         2 – только датчик (бесполезен без MQTT/Web)
 *         3 – управляемый выключатель
 */
#ifndef DEVICE_TYPE
#define DEVICE_TYPE 1
#endif

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

/**
 * @brief Уровень сигнала для включения реле
 * @values HIGH или LOW
 */
#ifndef RELAY_ON_LEVEL
#define RELAY_ON_LEVEL LOW
#endif

// ============================================================================
// ФУНКЦИОНАЛЬНЫЕ ВОЗМОЖНОСТИ (вкл/выкл)
// ============================================================================

/** @brief Включить поддержку WiFi */
#ifndef WIFI_ENABLED
#define WIFI_ENABLED 1
#endif

#if WIFI_ENABLED == 1

#ifndef SCANING_WIFI_ENABLED
#define SCANING_WIFI_ENABLED 0
#endif

/** @brief Мощность WiFi передатчика (0.0 – 20.5 dBm) */
#ifndef WIFI_OUTPUT_POWER
#define WIFI_OUTPUT_POWER 15.0
#endif

/** @brief Включить режим точки доступа (AP) для настройки */
#ifndef AP_ENABLED
#define AP_ENABLED 1
#endif

/** @brief Включить веб-интерфейс */
#ifndef WEB_ENABLED
#define WEB_ENABLED 1
#endif

/** @brief Включить MQTT клиент */
#ifndef MQTT_ENABLED
#define MQTT_ENABLED 1
#endif

/** @brief Включить OTA обновления */
#ifndef OTA_ENABLED
#define OTA_ENABLED 1
#endif

#else
// Если WiFi отключён — отключаем всё, что от него зависит
#define WEB_ENABLED 0
#define MQTT_ENABLED 0
#define OTA_ENABLED 0
#define AP_ENABLED 0
#endif

// ============================================================================
// НАСТРОЙКИ WEB ИНТЕРФЕЙСА
// ============================================================================

#if WEB_ENABLED == 1
/** @brief Показывать страницу состояния (иначе сразу /config) */
#ifndef WEB_STATUS_ENABLED
#define WEB_STATUS_ENABLED 1
#endif

/** @brief Показывать RSSI на странице состояния */
#ifndef WEB_SHOW_RSSI
#define WEB_SHOW_RSSI 1
#endif

/** @brief Включить кнопку сброса настроек в веб-интерфейсе */
#ifndef WEB_RESET_ENABLED
#define WEB_RESET_ENABLED 0
#endif
#endif

// ============================================================================
// WATCHDOG (WDT)
// ============================================================================

/** @brief Включить аппаратный сторожевой таймер */
#ifndef WDT_ENABLED
#define WDT_ENABLED 1
#endif

#if WDT_ENABLED == 1
/** @brief Таймаут WDT в миллисекундах */
#ifndef WDT_TIMER_MS
#define WDT_TIMER_MS 5000
#endif

/** @brief Множитель для расчёта WDT в loop (не используется в текущей версии)
 */
#ifndef LOOP_WATCHDOG_MULTIPLIER
#define LOOP_WATCHDOG_MULTIPLIER 3
#endif

#else

/** @brief Софт-WDT (заглушка, не реализован) */
#ifndef SOFT_WDT_ENABLED
#define SOFT_WDT_ENABLED 1
#endif

#endif

// ============================================================================
// НАСТРОЙКИ WIFI
// ============================================================================

/** @brief Таймаут подключения к WiFi (миллисекунды) */
#ifndef WIFI_CONNECT_TIMEOUT_MS
#define WIFI_CONNECT_TIMEOUT_MS 30000
#endif

/** @brief Интервал проверки WiFi соединения (мс) */
#ifndef WIFI_CHECK_INTERVAL_MS
#define WIFI_CHECK_INTERVAL_MS 10000
#endif

// ============================================================================
// НАСТРОЙКИ MQTT
// ============================================================================

#if MQTT_ENABLED == 1

/** @brief Задержка между попытками переподключения (мс) */
#ifndef MQTT_RECONNECT_DELAY_MS
#define MQTT_RECONNECT_DELAY_MS 5000
#endif

/** @brief Интервал публикации heartbeat (мс) */
#ifndef STATE_PUBLISH_INTERVAL_MS
#define STATE_PUBLISH_INTERVAL_MS 3000
#endif

/** @brief Keep-alive интервал MQTT (секунды) */
#ifndef MQTT_KEEPALIVE_SEC
#define MQTT_KEEPALIVE_SEC 3
#endif

/** @brief Включить MQTT команду сброса настроек */
#ifndef MQTT_RESET_ENABLED
#define MQTT_RESET_ENABLED 1
#endif

/** @brief Публиковать RSSI в MQTT */
#ifndef MQTT_PUBLISH_RSSI
#define MQTT_PUBLISH_RSSI 1
#endif

/** @brief Публиковать версию прошивки в MQTT */
#ifndef MQTT_PUBLISH_VERSION
#define MQTT_PUBLISH_VERSION 1
#endif

/** @brief Публиковать причину перезагрузки (кроме POWER_ON/SOFT_RESTART) */
#ifndef MQTT_PUBLISH_RESET_REASON
#define MQTT_PUBLISH_RESET_REASON 1
#endif

#if MQTT_PUBLISH_RESET_REASON == 1
/** @brief Не публиковать штатные перезагрузки (POWER_ON, SOFT_RESTART) */
#ifndef MQTT_IGNORE_PUBLISH_NORMAL_RESET_REASONS
#define MQTT_IGNORE_PUBLISH_NORMAL_RESET_REASONS 1
#endif
#endif

#else
#define MQTT_RESET_ENABLED 0
#define MQTT_PUBLISH_RSSI 0
#define MQTT_PUBLISH_RESET_REASON 0
#endif

// ============================================================================
// РЕЖИМ ТОЧКИ ДОСТУПА (AP)
// ============================================================================

#if AP_ENABLED == 1
/** @brief IP адрес точки доступа */
#ifndef AP_IP_ADDRESS
#define AP_IP_ADDRESS "192.168.4.1"
#endif

/**
 * @brief Время без WiFi до перехода в режим AP (мс)
 * Если устройство не может подключиться к WiFi дольше этого времени,
 * запускается собственная точка доступа для настройки.
 */
#ifndef AP_FALLBACK_TIMEOUT_MS
#define AP_FALLBACK_TIMEOUT_MS 12000
#endif
#endif

// ============================================================================
// ОТЛАДКА И ЛОГИРОВАНИЕ
// ============================================================================

#ifndef LOG_LEVEL
#define LOG_LEVEL 3  // 0=ERROR, 1=WARN, 2=INFO, 3=DEBUG
#endif

#ifndef LOG_CATEGORIES
#define LOG_CATEGORIES 0xFFFF  // Все категории
#endif

#ifndef LOG_USE_COLOR
#define LOG_USE_COLOR 1
#endif

// ============================================================================
// ШИМ (PWM) — для TYPE 1
// ============================================================================

#if DEVICE_TYPE == 1

/** @brief Включить ШИМ управление скоростью */
#ifndef PWM_ENABLED
#define PWM_ENABLED 1
#endif

#if PWM_ENABLED == 1
/** @brief Частота ШИМ в Герцах */
#ifndef PWM_FREQUENCY
#define PWM_FREQUENCY 5
#endif

/** @brief Разрешение ШИМ (бит) */
#ifndef PWM_RESOLUTION
#define PWM_RESOLUTION 8
#endif

/** @brief Длина стартового импульса для раскрутки вентилятора (мс) */
#ifndef PWM_STARTING
#define PWM_STARTING 200
#endif

/** @brief Скорость по умолчанию (%) */
#ifndef DEFAULT_SPEED_PERCENT
#define DEFAULT_SPEED_PERCENT 50
#endif
#endif

/**
 * @brief Адаптивный тихий режим
 * Автоматически увеличивает скорость при росте температуры/влажности
 */
#if PWM_ENABLED == 1
#ifndef ADAPTIVE_ENABLED
#define ADAPTIVE_ENABLED 1
#endif

#if ADAPTIVE_ENABLED == 1
/** @brief Порог изменения температуры для адаптации (°C) */
#ifndef ADAPTIVE_EPSILON_TEMP
#define ADAPTIVE_EPSILON_TEMP 0.5
#endif

/** @brief Порог изменения влажности для адаптации (%) */
#ifndef ADAPTIVE_EPSILON_HUM
#define ADAPTIVE_EPSILON_HUM 2.0
#endif

/** @brief Чувствительность адаптации (1.0 = нормальная) */
#ifndef ADAPTIVE_SPEED_SENSITIVITY
#define ADAPTIVE_SPEED_SENSITIVITY 0.7
#endif

/** @brief Шаг изменения скорости при адаптации (%) */
#ifndef ADAPTIVE_STEP_SIZE
#define ADAPTIVE_STEP_SIZE 10
#endif

/** @brief Минимальная скорость при адаптации (%) */
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

// ============================================================================
// EEPROM
// ============================================================================

/** @brief Магическое число для проверки валидности EEPROM */
#ifndef MAGIC_VALUE
#define MAGIC_VALUE 0x5A6B
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
// СТРУКТУРА КОНФИГУРАЦИИ (хранится в EEPROM)
// ============================================================================

/**
 * @brief Структура конфигурации, сохраняемая в EEPROM
 *
 * @note Размер структуры не должен превышать размер EEPROM (обычно 512 байт)
 * @note При добавлении полей обязательно увеличить версию и обновить логику CRC
 */
struct Config {
  uint16_t magic;  //!< Магическое число (MAGIC_VALUE) для проверки валидности
  uint16_t crc;    //!< CRC16 от всей структуры (кроме самого поля crc)

// ========== WiFi настройки ==========
#if WIFI_ENABLED == 1
  char wifiSsid[32];      //!< Имя WiFi сети
  char wifiPassword[64];  //!< Пароль WiFi сети
  float wifiOutputPower;  //!< Мощность передатчика (0-20.5 dBm)
#endif

// ========== MQTT настройки ==========
#if MQTT_ENABLED == 1
  char mqttBroker[64];    //!< Адрес MQTT брокера
  uint16_t mqttPort;      //!< Порт MQTT брокера
  char mqttUser[32];      //!< Имя пользователя MQTT
  char mqttPassword[64];  //!< Пароль MQTT
  char mqttClientId[24];  //!< Уникальный ID клиента
#endif

// ========== Настройки вентилятора (только TYPE 1) ==========
#if DEVICE_TYPE == 1
  double lowHum;           //!< Нижний порог влажности
  double highHum;          //!< Верхний порог влажности
  double lowTemp;          //!< Нижний порог температуры
  double highTemp;         //!< Верхний порог температуры
  bool sensorControlMode;  //!< Режим управления сенсором (true = авто)
  uint16_t speedPercent;   //!< Скорость вентилятора (%)
  bool adaptiveMode;       //!< Адаптивный тихий режим
#endif

// ========== Общие настройки для TYPE 1 и TYPE 3 ==========
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  int delaySeconds;    //!< Задержка отложенного включения (сек)
  uint32_t maxOnTime;  //!< Время аварийного отключения (сек)
  bool bootState;      //!< Состояние при старте (true = включено)
#endif

// ========== Настройки датчика для TYPE 1 и TYPE 2 ==========
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  uint16_t sensorInterval;  //!< Интервал опроса датчика (сек)
#endif
};

// ============================================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// ============================================================================

extern char deviceId[12];  //!< Уникальный ID устройства (префикс + MAC)
extern bool apMode;        //!< Флаг режима точки доступа

// ============================================================================
// ИНИЦИАЛИЗАЦИЯ
// ============================================================================

/**
 * @brief Инициализация EEPROM и загрузка конфигурации
 * @note Вызывается один раз в setup()
 */
void config_init();

/**
 * @brief Сгенерировать ID устройства на основе MAC-адреса
 */
void initDeviceId();

// ============================================================================
// ЧТЕНИЕ И ЗАПИСЬ
// ============================================================================

/**
 * @brief Прочитать конфигурацию из EEPROM
 * @note Обычно вызывается из config_init()
 */
void config_read();

/**
 * @brief Записать текущую конфигурацию в EEPROM
 * @return true — успешно, false — ошибка записи или верификации
 */
bool config_write();

/**
 * @brief Сбросить конфигурацию к значениям по умолчанию
 * @return true — успешно, false — ошибка
 */
bool config_clear();

/**
 * @brief Установить значения по умолчанию
 * @note Заполняет структуру _config безопасными значениями
 */
void config_setDefaults();

// ============================================================================
// ДОСТУП К КОНФИГУРАЦИИ
// ============================================================================

/**
 * @brief Получить указатель на текущую конфигурацию
 * @return Указатель на константную структуру Config
 * @note ТОЛЬКО ДЛЯ ЧТЕНИЯ! Используйте сеттеры для изменения.
 */
const Config* config_get();

/**
 * @brief Проверить валидность текущей конфигурации
 * @return true — конфигурация загружена из EEPROM и прошла CRC
 */
bool config_isValid();

/**
 * @brief Получить текст последней ошибки валидации
 * @return Строка с описанием ошибки (пустая, если ошибки нет)
 */
const char* config_getLastError();

/**
 * @brief Получить конфигурацию, сохранённую в EEPROM (без загрузки в рабочую)
 * @return Структура Config с данными из EEPROM
 * @note Используется веб-интерфейсом для отображения сохранённых значений
 */
Config config_getSaved();

// ============================================================================
// СЕТТЕРЫ С ВАЛИДАЦИЕЙ
// ============================================================================
// Все сеттеры возвращают true при успешной установке,
// false при ошибке (текст ошибки доступен через config_getLastError())
// ============================================================================

// --- WiFi настройки ---
bool config_setWifiSsid(
    const char* ssid);  //!< Не может быть пустым, максимум 31 символ
bool config_setWifiPassword(
    const char* password);  //!< Максимум 63 символа, может быть пустым
bool config_setWifiOutputPower(float power);  //!< Диапазон 0.0 – 20.5 dBm

// --- MQTT настройки ---
#if MQTT_ENABLED == 1
bool config_setMqttBroker(
    const char* broker);  //!< Не может быть пустым, максимум 63 символа
bool config_setMqttPort(uint16_t port);             //!< Диапазон 1 – 65535
bool config_setMqttUser(const char* user);          //!< Максимум 31 символ
bool config_setMqttPassword(const char* password);  //!< Максимум 63 символа
bool config_setMqttClientId(
    const char* clientId);  //!< A-Z, a-z, 0-9, _, -, максимум 23 символа
#endif

// --- Таймеры и режимы (TYPE 1 и 3) ---
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
bool config_setDelaySeconds(int seconds);    //!< Диапазон 0 – 86400
bool config_setMaxOnTime(uint32_t seconds);  //!< Диапазон 0 – 86400
bool config_setBootState(bool state);        //!< Без валидации
#endif

// --- Датчик (TYPE 1 и 2) ---
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
bool config_setSensorInterval(uint16_t interval);  //!< Диапазон 1 – 3600
#endif

// --- Вентилятор (TYPE 1) ---
#if DEVICE_TYPE == 1
bool config_setLowTemp(
    double temp);  //!< Диапазон TEMP_MIN..TEMP_MAX, должно быть < highTemp
bool config_setHighTemp(
    double temp);  //!< Диапазон TEMP_MIN..TEMP_MAX, должно быть > lowTemp
bool config_setLowHum(
    double hum);  //!< Диапазон HUM_MIN..HUM_MAX, должно быть < highHum
bool config_setHighHum(
    double hum);  //!< Диапазон HUM_MIN..HUM_MAX, должно быть > lowHum
bool config_setSensorControlMode(bool enabled);  //!< Требует speedPercent > 0
bool config_setSpeedPercent(uint16_t percent);   //!< Диапазон 0 – 100
bool config_setAdaptiveMode(
    bool enabled);  //!< Требует sensorControlMode = true и speedPercent > 0
#endif

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================================

/**
 * @brief Рассчитать CRC16 для блока данных
 * @param data Указатель на данные
 * @param len Длина в байтах
 * @return CRC16 (алгоритм CRC-16/IBM)
 */
uint16_t crc16(const uint8_t* data, size_t len);

// ============================================================================
// ОТЛАДКА
// ============================================================================

/**
 * @brief Вывести текущую конфигурацию в Serial
 */
void config_print();

#endif  // CONFIG_H