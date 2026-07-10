/**
 * @file settings.h
 * @brief Глобальные настройки проекта
 * @details Все макросы конфигурации, определяющие поведение устройства.
 *          Настройки могут быть переопределены через build_flags в
 * platformio.ini.
 */

#ifndef SETTINGS_H
#define SETTINGS_H

// ============================================================================
// ВЕРСИЯ ПРОШИВКИ
// ============================================================================

/**
 * @brief Версия прошивки
 * @details Формат: X.Y.Z, где X — мажорная, Y — минорная, Z — патч
 */
#ifndef VERSION
#define VERSION "1.0"
#endif

// ============================================================================
// ТИП УСТРОЙСТВА
// ============================================================================

/**
 * @brief Тип устройства
 * @values
 *   - 1 – вентилятор с датчиком (Fan)
 *   - 2 – автономный датчик (Sensor)
 *   - 3 – управляемый выключатель (Switch)
 */
#ifndef DEVICE_TYPE
#define DEVICE_TYPE 1
#endif

/**
 * @brief Префикс устройства (формируется автоматически на основе DEVICE_TYPE)
 * @details Используется для:
 *          - Формирования MQTT-топиков
 *          - Имени точки доступа (AP)
 *          - Идентификации устройства
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
// АППАРАТНАЯ КОНФИГУРАЦИЯ
// ============================================================================

/**
 * @brief Пин кнопки сброса настроек
 * @details Обычно GPIO0 (кнопка BOOT/FLASH на большинстве плат)
 *          При длительном нажатии (>3 сек) — сброс к заводским настройкам
 */
#ifndef RESET_PIN
#define RESET_PIN 0
#endif

/**
 * @brief Пин управления реле/вентилятором
 * @details По умолчанию:
 *          - ESP8266: GPIO14
 *          - ESP32: GPIO4
 */
#ifndef SWITCH_PIN
#ifdef ESP8266
#define SWITCH_PIN 14
#elif defined(ESP32)
#define SWITCH_PIN 4
#endif
#endif

/**
 * @brief Пин I2C SDA (для датчиков AHT10)
 * @details По умолчанию:
 *          - ESP8266: GPIO4
 *          - ESP32: GPIO21
 */
#ifndef I2C_SDA_PIN
#ifdef ESP8266
#define I2C_SDA_PIN 4
#elif defined(ESP32)
#define I2C_SDA_PIN 21
#endif
#endif

/**
 * @brief Пин I2C SCL (для датчиков AHT10)
 * @details По умолчанию:
 *          - ESP8266: GPIO5
 *          - ESP32: GPIO22
 */
#ifndef I2C_SCL_PIN
#ifdef ESP8266
#define I2C_SCL_PIN 5
#elif defined(ESP32)
#define I2C_SCL_PIN 22
#endif
#endif

// ============================================================================
// ДАТЧИК (TYPE 1 и 2)
// ============================================================================

/**
 * @brief Тип датчика температуры/влажности
 * @values
 *   - 1 – AHT10 (I2C)
 *   - 2 – DHT11/DHT22 (GPIO)
 */
#ifndef SENSOR_TYPE
#define SENSOR_TYPE 1
#endif

#if SENSOR_TYPE == 2
/**
 * @brief Пин для DHT датчика (только для SENSOR_TYPE=2)
 */
#ifndef SENSOR_PIN
#ifdef ESP8266
#define SENSOR_PIN 4
#elif defined(ESP32)
#define SENSOR_PIN 16
#endif
#endif
#endif

// ============================================================================
// ТРАНСПОРТЫ: MQTT и ZIGBEE
// ============================================================================

/**
 * @brief Включить поддержку MQTT
 * @details По умолчанию:
 *          - ESP8266: включён
 *          - ESP32/ESP32-S2/S3/C3: включён
 *          - ESP32-C6/H2: отключён (требуется явное включение)
 * @note MQTT и ZigBee не могут быть включены одновременно
 */
#ifndef MQTT_ENABLED
#define MQTT_ENABLED 0
#endif

/**
 * @brief Включить поддержку ZigBee
 * @details По умолчанию отключён. Поддерживается только на ESP32-C6 и ESP32-H2.
 *          При включении автоматически отключает MQTT и провизионинг.
 * @warning Только для платформ с аппаратной поддержкой ZigBee!
 */
#ifndef ZIGBEE_ENABLED
#define ZIGBEE_ENABLED 0
#endif

// ============================================================================
// ПРОВЕРКА ПЛАТФОРМЫ ДЛЯ ZIGBEE
// ============================================================================

#if ZIGBEE_ENABLED == 1
/**
 * @brief Проверка: ZigBee доступен только на ESP32-C6 и ESP32-H2
 */
#if !defined(CONFIG_IDF_TARGET_ESP32C6) && !defined(CONFIG_IDF_TARGET_ESP32H2)
#warning \
    "ZIGBEE_ENABLED=1 is only supported on ESP32-C6 and ESP32-H2! Disabling ZigBee."
#undef ZIGBEE_ENABLED
#define ZIGBEE_ENABLED 0
#endif
#endif

// ============================================================================
// АВТОМАТИЧЕСКОЕ ОПРЕДЕЛЕНИЕ ТРАНСПОРТОВ
// ============================================================================

/**
 * @brief Автоматическое определение MQTT_ENABLED
 * @details Если не определён явно, устанавливается на основе платформы:
 *          - ESP8266: включён
 *          - ESP32/ESP32-S2/S3/C3: включён
 *          - ESP32-C6/H2: отключён (пользователь должен выбрать явно)
 *          - Если ZIGBEE_ENABLED=1, MQTT принудительно отключается
 */
#if !defined(MQTT_ENABLED) || MQTT_ENABLED == 0
#if ZIGBEE_ENABLED == 1
// ZigBee включён — MQTT не нужен
#define MQTT_ENABLED 0
#elif defined(ESP8266)
#define MQTT_ENABLED 1
#elif defined(ESP32) && !defined(CONFIG_IDF_TARGET_ESP32C6) && \
    !defined(CONFIG_IDF_TARGET_ESP32H2)
#define MQTT_ENABLED 1
#else
// ESP32-C6/H2 — требуют явного выбора
#define MQTT_ENABLED 0
#endif
#endif

// ============================================================================
// ПРОВЕРКА ВЗАИМОИСКЛЮЧЕНИЯ ТРАНСПОРТОВ
// ============================================================================

/**
 * @brief Проверка: MQTT и ZigBee не могут быть включены одновременно
 */
#if MQTT_ENABLED == 1 && ZIGBEE_ENABLED == 1
#error "MQTT_ENABLED and ZIGBEE_ENABLED cannot be both enabled!"
#endif

// ============================================================================
// ДОПОЛНИТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================================

/**
 * @brief Включить веб-интерфейс
 * @details Работает как в режиме AP (настройка), так и в STA (статус/настройка)
 */
#ifndef WEB_ENABLED
#define WEB_ENABLED 1
#endif

/**
 * @brief Включить OTA-обновления
 * @details Позволяет обновлять прошивку через веб-интерфейс
 * @warning Требует включённого WEB_ENABLED
 */
#ifndef OTA_ENABLED
#define OTA_ENABLED 1
#endif

/**
 * @brief Включить Watchdog Timer
 * @details Защита от зависаний: перезагружает устройство при сбое
 */
#ifndef WDT_ENABLED
#define WDT_ENABLED 1
#endif

/**
 * @brief Включить WiFi
 * @details Если отключён, устройство работает только через ZigBee
 */
#ifndef WIFI_ENABLED
#define WIFI_ENABLED 1
#endif

/**
 * @brief Включить режим сканирования WiFi
 * @details При включении устройство сканирует доступные сети при старте
 */
#ifndef SCANNING_WIFI_ENABLED
#define SCANNING_WIFI_ENABLED 0
#endif

// ============================================================================
// МЕТОД НАСТРОЙКИ (PROVISIONING)
// ============================================================================

/**
 * @brief Режимы провизионинга
 * @values
 *   - 0: Нет (отключён) — для ZigBee или заводской прошивки
 *   - 1: Только BLE
 *   - 2: Только AP (WiFi точка доступа + веб-интерфейс)
 *   - 3: BLE + AP (комбинированный, по умолчанию)
 *
 * @details
 *   - Если не определён, выбирается автоматически:
 *     - ESP8266 (без BLE) → 2 (AP)
 *     - Остальные платформы → 3 (BLE+AP)
 *   - При ZIGBEE_ENABLED=1 принудительно устанавливается в 0
 *
 * @note Для платформ без BLE режимы 1 и 3 автоматически заменяются на 2
 */
#ifndef PROVISIONING_METHOD
#ifdef ESP8266
// ESP8266 без BLE — только AP
#define PROVISIONING_METHOD 2
#else
// Платформы с BLE — BLE+AP
#define PROVISIONING_METHOD 3
#endif
#endif

/**
 * @brief ZIGBEE_ENABLED переопределяет PROVISIONING_METHOD в 0
 * @details Если устройство использует ZigBee, провизионинг не требуется
 */
#if ZIGBEE_ENABLED == 1
#undef PROVISIONING_METHOD
#define PROVISIONING_METHOD 0
#endif

// ============================================================================
// ВЫБОР МЕТОДА НАСТРОЙКИ (на основе PROVISIONING_METHOD)
// ============================================================================

/**
 * @brief Флаг включения BLE-провизионинга
 * @details Автоматически вычисляется на основе PROVISIONING_METHOD
 */
#if PROVISIONING_METHOD == 0
#define USE_BLE_PROVISIONING 0
#define USE_AP_PROVISIONING 0

#elif PROVISIONING_METHOD == 1
// Режим 1: Только BLE
#ifdef ESP8266
// ESP8266 без BLE — принудительно AP
#warning "BLE not supported on ESP8266! Forcing AP mode"
#define USE_BLE_PROVISIONING 0
#define USE_AP_PROVISIONING 1
#else
#define USE_BLE_PROVISIONING 1
#define USE_AP_PROVISIONING 0
#endif

#elif PROVISIONING_METHOD == 2
// Режим 2: Только AP
#define USE_BLE_PROVISIONING 0
#define USE_AP_PROVISIONING 1

#elif PROVISIONING_METHOD == 3
// Режим 3: BLE + AP (комбинированный)
#ifdef ESP8266
// ESP8266 без BLE — только AP
#warning "BLE not supported on ESP8266! Using AP only"
#define USE_BLE_PROVISIONING 0
#define USE_AP_PROVISIONING 1
#else
#define USE_BLE_PROVISIONING 1
#define USE_AP_PROVISIONING 1
#endif

#else
// Неизвестный метод — NONE (режим 0)
#warning "Unknown PROVISIONING_METHOD, using NONE"
#define USE_BLE_PROVISIONING 0
#define USE_AP_PROVISIONING 0
#endif

// ============================================================================
// BLE PROVISIONING
// ============================================================================

/**
 * @brief PIN-код для BLE-сопряжения (PoP — Proof of Possession)
 * @details Используется в приложении ESP BLE Provisioning
 *          для подтверждения права настройки устройства.
 * @warning При изменении необходимо обновить PIN и в приложении!
 */
#ifndef BLE_PROVISIONING_PIN
#define BLE_PROVISIONING_PIN "abcd1234"
#endif

/**
 * @brief Имя устройства для BLE-сопряжения
 * @details Отображается в приложении ESP BLE Provisioning при поиске устройств
 */
#ifndef BLE_DEVICE_NAME
#define BLE_DEVICE_NAME "PROV_123"
#endif

// ============================================================================
// ДИАПАЗОНЫ ДЛЯ ВАЛИДАЦИИ
// ============================================================================

/**
 * @brief Минимальная допустимая температура (°C)
 */
#ifndef TEMP_MIN
#define TEMP_MIN -40.0f
#endif

/**
 * @brief Максимальная допустимая температура (°C)
 */
#ifndef TEMP_MAX
#define TEMP_MAX 85.0f
#endif

/**
 * @brief Минимальная допустимая влажность (%)
 */
#ifndef HUM_MIN
#define HUM_MIN 0.0f
#endif

/**
 * @brief Максимальная допустимая влажность (%)
 */
#ifndef HUM_MAX
#define HUM_MAX 100.0f
#endif

/**
 * @brief Минимальный интервал опроса датчика (сек)
 */
#ifndef SENSOR_INTERVAL_MIN
#define SENSOR_INTERVAL_MIN 1
#endif

/**
 * @brief Максимальный интервал опроса датчика (сек)
 */
#ifndef SENSOR_INTERVAL_MAX
#define SENSOR_INTERVAL_MAX 3600
#endif

/**
 * @brief Минимальная задержка отложенного включения (сек)
 */
#ifndef DELAY_SECONDS_MIN
#define DELAY_SECONDS_MIN 0
#endif

/**
 * @brief Максимальная задержка отложенного включения (сек)
 */
#ifndef DELAY_SECONDS_MAX
#define DELAY_SECONDS_MAX 86400
#endif

/**
 * @brief Минимальное время аварийного отключения (сек)
 */
#ifndef MAX_ON_TIME_MIN
#define MAX_ON_TIME_MIN 0
#endif

/**
 * @brief Максимальное время аварийного отключения (сек)
 */
#ifndef MAX_ON_TIME_MAX
#define MAX_ON_TIME_MAX 86400
#endif

// ============================================================================
// ЗНАЧЕНИЯ ПО УМОЛЧАНИЮ
// ============================================================================

#if DEVICE_TYPE == 1
/**
 * @brief Нижний порог температуры по умолчанию (°C)
 */
#ifndef DEFAULT_LOW_TEMP
#define DEFAULT_LOW_TEMP 27.0f
#endif

/**
 * @brief Верхний порог температуры по умолчанию (°C)
 */
#ifndef DEFAULT_HIGH_TEMP
#define DEFAULT_HIGH_TEMP 29.0f
#endif

/**
 * @brief Нижний порог влажности по умолчанию (%)
 */
#ifndef DEFAULT_LOW_HUM
#define DEFAULT_LOW_HUM 55.0f
#endif

/**
 * @brief Верхний порог влажности по умолчанию (%)
 */
#ifndef DEFAULT_HIGH_HUM
#define DEFAULT_HIGH_HUM 60.0f
#endif

/**
 * @brief Режим управления сенсором по умолчанию
 * @details true — автоматический, false — ручной
 */
#ifndef DEFAULT_SENSOR_CONTROL_MODE
#define DEFAULT_SENSOR_CONTROL_MODE true
#endif
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
/**
 * @brief Задержка отложенного включения по умолчанию (сек)
 */
#ifndef DEFAULT_DELAY_SECONDS
#define DEFAULT_DELAY_SECONDS 60
#endif

/**
 * @brief Адаптивный режим по умолчанию
 * @details true — адаптивный тихий режим включён
 */
#ifndef DEFAULT_ADAPTIVE_MODE
#define DEFAULT_ADAPTIVE_MODE false
#endif

/**
 * @brief Состояние при старте по умолчанию
 * @details true — исполнительный механизм включён
 */
#ifndef BOOT_SWITCH_STATE
#define BOOT_SWITCH_STATE true
#endif

/**
 * @brief Таймер аварийного отключения по умолчанию (сек)
 * @details 0 — отключён
 */
#ifndef MAX_ON_TIME_SEC
#define MAX_ON_TIME_SEC 3600
#endif

/**
 * @brief Включить таймер аварийного отключения
 */
#ifndef EMERGENCY_ENABLED
#define EMERGENCY_ENABLED 1
#endif
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
/**
 * @brief Интервал опроса датчика по умолчанию (сек)
 */
#ifndef SENSOR_DURATION
#define SENSOR_DURATION 10
#endif
#endif

/**
 * @brief Скорость вентилятора по умолчанию (%)
 */
#ifndef DEFAULT_SPEED_PERCENT
#define DEFAULT_SPEED_PERCENT 50
#endif

/**
 * @brief Минимальная скорость вентилятора (%)
 * @details Значения ниже этого порога интерпретируются как команда выключения
 */
#ifndef MIN_SPEED_PERCENT
#define MIN_SPEED_PERCENT 30
#endif

// ============================================================================
// УРОВЕНЬ РЕЛЕ
// ============================================================================

/**
 * @brief Уровень сигнала для включения реле
 * @details HIGH (1) — реле срабатывает по высокому уровню,
 *          LOW (0) — по низкому (обычно для активного LOW)
 */
#ifndef RELAY_ON_LEVEL
#define RELAY_ON_LEVEL HIGH
#endif

// ============================================================================
// MQTT
// ============================================================================

/**
 * @brief Порт MQTT по умолчанию
 */
#ifndef MQTT_PORT
#define MQTT_PORT 1883
#endif

/**
 * @brief Публиковать RSSI в MQTT
 * @details Если включён, устройство публикует уровень сигнала WiFi
 */
#ifndef MQTT_PUBLISH_RSSI
#define MQTT_PUBLISH_RSSI 1
#endif

/**
 * @brief Публиковать версию прошивки в MQTT
 */
#ifndef MQTT_PUBLISH_VERSION
#define MQTT_PUBLISH_VERSION 1
#endif

/**
 * @brief Публиковать причину перезагрузки в MQTT
 */
#ifndef MQTT_PUBLISH_RESET_REASON
#define MQTT_PUBLISH_RESET_REASON 1
#endif

/**
 * @brief Включить команду сброса через MQTT
 */
#ifndef MQTT_RESET_ENABLED
#define MQTT_RESET_ENABLED 1
#endif

/**
 * @brief Интервал публикации статуса в MQTT (мс)
 */
#ifndef STATE_PUBLISH_INTERVAL_MS
#define STATE_PUBLISH_INTERVAL_MS 30000
#endif

// ============================================================================
// WEB
// ============================================================================

/**
 * @brief Включить страницу статуса в веб-интерфейсе
 */
#ifndef WEB_STATUS_ENABLED
#define WEB_STATUS_ENABLED 1
#endif

/**
 * @brief Показывать RSSI на веб-странице
 */
#ifndef WEB_SHOW_RSSI
#define WEB_SHOW_RSSI 1
#endif

/**
 * @brief Включить команду сброса через веб-интерфейс
 */
#ifndef WEB_RESET_ENABLED
#define WEB_RESET_ENABLED 1
#endif

/**
 * @brief Интервал обновления веб-страницы по умолчанию (сек)
 */
#ifndef DEFAULT_WEB_REFRESH
#define DEFAULT_WEB_REFRESH 5
#endif

/**
 * @brief IP-адрес точки доступа (AP) по умолчанию
 */
#ifndef AP_IP_ADDRESS
#define AP_IP_ADDRESS "192.168.4.1"
#endif

/**
 * @brief Таймаут подключения к WiFi (мс)
 */
#ifndef WIFI_CONNECT_TIMEOUT_MS
#define WIFI_CONNECT_TIMEOUT_MS 6000
#endif

/**
 * @brief Время без WiFi до перехода в AP режим (мс)
 */
#ifndef AP_FALLBACK_TIMEOUT_MS
#define AP_FALLBACK_TIMEOUT_MS 12000
#endif

// ============================================================================
// EEPROM
// ============================================================================

/**
 * @brief Магическое число для проверки валидности EEPROM
 */
#ifndef MAGIC_VALUE
#define MAGIC_VALUE 0x5A6D
#endif

/**
 * @brief Размер EEPROM в байтах
 * @details Автоматически вычисляется на основе структуры ConfigData
 */
#ifndef EEPROM_SIZE
#define EEPROM_SIZE 512
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

/**
 * @brief WiFi SSID по умолчанию (заводской)
 */
#ifndef SSID_NAME
#define SSID_NAME ""
#endif

/**
 * @brief WiFi пароль по умолчанию (заводской)
 */
#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD ""
#endif

/**
 * @brief MQTT брокер по умолчанию (заводской)
 */
#ifndef MQTT_ADDRESS
#define MQTT_ADDRESS ""
#endif

/**
 * @brief MQTT пользователь по умолчанию (заводской)
 */
#ifndef MQTT_USER
#define MQTT_USER ""
#endif

/**
 * @brief MQTT пароль по умолчанию (заводской)
 */
#ifndef MQTT_PASSWORD
#define MQTT_PASSWORD ""
#endif

#endif  // SETTINGS_H