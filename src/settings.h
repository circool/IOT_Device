/**
 * @file settings.h
 * @brief Глобальные настройки проекта
 * @details Все макросы конфигурации, определяющие поведение устройства.
 *          Настройки могут быть переопределены через build_flags в
 * platformio.ini.
 *
 *          Принцип именования:
 *          - FEATURE_XXX_ENABLED — функциональные возможности (1 = включена)
 *          - XXX_TYPE — выбор типа/режима
 *          - XXX_METHOD — выбор метода
 *
 *          Все остальные параметры (пины, пороги, таймауты, дефолты,
 *          производные флаги, креденшелы) определяются в соответствующих
 *          слоях, где они используются.
 *
 * @note Этот файл НЕ должен включать другие заголовочные файлы,
 *       чтобы избежать циклических зависимостей.
 */

#ifndef SETTINGS_H
#define SETTINGS_H


// ============================================================================
// 0. ОПРЕДЕЛЕНИЕ ПЛАТФОРМЫ
// ============================================================================

/**
 * @brief Идентификация платформы
 * @details Определяется автоматически на основе макросов компилятора
 *          Используется для выбора дефолтных значений пинов и параметров
 */
#if defined(ESP8266)
#define PLATFORM_ESP8266 1
#define PLATFORM_ESP32 0
#define PLATFORM_ESP32_SERIES 0
#elif defined(ESP32)
#define PLATFORM_ESP8266 0
#define PLATFORM_ESP32 1
#define PLATFORM_ESP32_SERIES 1
// Дополнительное определение для ESP32-C6/H2
#if defined(CONFIG_IDF_TARGET_ESP32C6)
#define PLATFORM_ESP32C6 1
#else
#define PLATFORM_ESP32C6 0
#endif
#if defined(CONFIG_IDF_TARGET_ESP32H2)
#define PLATFORM_ESP32H2 1
#else
#define PLATFORM_ESP32H2 0
#endif
#if defined(CONFIG_IDF_TARGET_ESP32S3)
#define PLATFORM_ESP32S3 1
#else
#define PLATFORM_ESP32S3 0
#endif
#if defined(CONFIG_IDF_TARGET_ESP32C3)
#define PLATFORM_ESP32C3 1
#else
#define PLATFORM_ESP32C3 0
#endif
#else
#define PLATFORM_ESP8266 0
#define PLATFORM_ESP32 0
#define PLATFORM_ESP32_SERIES 0
#define PLATFORM_ESP32C6 0
#define PLATFORM_ESP32H2 0
#define PLATFORM_ESP32S3 0
#define PLATFORM_ESP32C3 0
#endif

// ============================================================================
// 1. ВЕРСИЯ И ТИП УСТРОЙСТВА
// ============================================================================

/**
 * @brief Версия прошивки
 * @details Формат: X.Y.Z
 */
#ifndef VERSION
#define VERSION "1.0"
#endif

/**
 * @brief Тип устройства
 * @values
 *   - 1 – Fan (вентилятор с датчиком)
 *   - 2 – Sensor (автономный датчик)
 *   - 3 – Switch (управляемый выключатель)
 */
#ifndef DEVICE_TYPE
#define DEVICE_TYPE 1
#endif

/**
 * @brief Префикс устройства (автоматически на основе DEVICE_TYPE)
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
// 2. ТРАНСПОРТ (ВЫБОР ПРОТОКОЛА)
// ============================================================================

/**
 * @brief Тип транспортного протокола
 * @values
 *   - 0: MQTT (поверх WiFi)
 *   - 1: ZigBee (только ESP32-C6/H2)
 *   - 2: Matter (перспективный, только ESP32-C6/H2)
 * @details
 *   - MQTT: требует FEATURE_WIFI_ENABLED=1, доступен на всех платформах
 *   - ZigBee: требует FEATURE_WIFI_ENABLED=0, только ESP32-C6/H2
 *   - Matter: требует FEATURE_WIFI_ENABLED=1, только ESP32-C6/H2 (перспективно)
 * @note При TRANSPORT_TYPE=1 (ZigBee) PROVISIONING_METHOD принудительно = 0
 */
#ifndef TRANSPORT_TYPE
// ZigBee по умолчанию для платформ с аппаратной поддержкой
#if PLATFORM_ESP32C6 || PLATFORM_ESP32H2
#define TRANSPORT_TYPE 1  // ZigBee
#else
#define TRANSPORT_TYPE 0  // MQTT
#endif
#endif

// ============================================================================
// 2.1. ПРОИЗВОДНЫЕ ФЛАГИ ТРАНСПОРТА
// ============================================================================

/**
 * @brief Включить MQTT (автоматически на основе TRANSPORT_TYPE)
 */
#ifndef FEATURE_MQTT_ENABLED
#if TRANSPORT_TYPE == 0
#define FEATURE_MQTT_ENABLED 1
#else
#define FEATURE_MQTT_ENABLED 0
#endif
#endif

/**
 * @brief Включить ZigBee (автоматически на основе TRANSPORT_TYPE)
 */
#ifndef FEATURE_ZIGBEE_ENABLED
#if TRANSPORT_TYPE == 1
#define FEATURE_ZIGBEE_ENABLED 1
#else
#define FEATURE_ZIGBEE_ENABLED 0
#endif
#endif

// ============================================================================
// 3. ФУНКЦИОНАЛЬНЫЕ ВОЗМОЖНОСТИ (FEATURES)
// ============================================================================

/**
 * @brief Включить логирование
 * @deprecated Достаточно указать уровень детализации
 * @details Если 0 — весь код логирования исключается.
 *          Настройки (уровень, категории, цвет) определяются в logger.h
 */
#ifndef FEATURE_LOGGER_ENABLED
#define FEATURE_LOGGER_ENABLED 1
#endif

/**
 * @brief Включить светодиодную индикацию
 * @details Если 0 — весь код индикации исключается.
 *          Пин и инверсия определяются в led.h
 */
#ifndef FEATURE_LED_ENABLED
#define FEATURE_LED_ENABLED 1
#endif

/**
 * @brief Включить поддержку датчика
 * @details Если 0 — код датчика исключается.
 *          Для TYPE 1 и TYPE 2 должно быть включено.
 *          Тип датчика и пины определяются в sensor.h
 */
#ifndef FEATURE_SENSOR_ENABLED
#define FEATURE_SENSOR_ENABLED 1
#endif

/**
 * @brief Включить WiFi
 * @details Если 0 — WiFi отключен (для ZigBee режима)
 * @note При TRANSPORT_TYPE=1 (ZigBee) принудительно отключается
 */
#ifndef FEATURE_WIFI_ENABLED
#define FEATURE_WIFI_ENABLED 1
#endif

/**
 * @brief Включить веб-интерфейс
 * @details Если 1 — доступен веб-сервер для настройки и управления.
 * @note Требует FEATURE_WIFI_ENABLED=1
 * @note Недоступен при TRANSPORT_TYPE=1 (ZigBee)
 */
#ifndef FEATURE_WEB_ENABLED
#define FEATURE_WEB_ENABLED 1
#endif

/**
 * @brief Включить OTA-обновления
 * @details Если 1 — доступно обновление прошивки через веб-интерфейс.
 * @note Требует FEATURE_WEB_ENABLED=1
 */
#ifndef FEATURE_OTA_ENABLED
#define FEATURE_OTA_ENABLED 1
#endif

/**
 * @brief Включить Watchdog Timer
 * @details Если 1 — аппаратный сторожевой таймер защищает от зависаний.
 *          Таймаут настраивается в wdt_manager.h
 */
#ifndef FEATURE_WDT_ENABLED
#define FEATURE_WDT_ENABLED 1
#endif

/**
 * @brief Включить сканирование WiFi при старте (отладка)
 * @details Если 1 — устройство сканирует доступные сети при запуске.
 * @note Работает только при TRANSPORT_TYPE=0 (MQTT) и FEATURE_WIFI_ENABLED=1
 */
#ifndef FEATURE_SCANNING_WIFI_ENABLED
#define FEATURE_SCANNING_WIFI_ENABLED 0
#endif

/**
 * @brief Включить сброс настроек через кнопку
 * @details Если 0 — кнопка сброса игнорируется
 * @note Пин кнопки определяется в main.cpp
 */
#ifndef FEATURE_RESET_BUTTON_ENABLED
#define FEATURE_RESET_BUTTON_ENABLED 1
#endif

// ============================================================================
// 4. PROVISIONING (МЕТОД НАСТРОЙКИ)
// ============================================================================



// ============================================================================
// 5. ПРОВЕРКИ ЗАВИСИМОСТЕЙ
// ============================================================================

// ----------------------------------------------------------------------------
// Проверка TRANSPORT_TYPE
// ----------------------------------------------------------------------------
#if TRANSPORT_TYPE < 0 || TRANSPORT_TYPE > 2
#error "TRANSPORT_TYPE must be 0 (MQTT), 1 (ZigBee), or 2 (Matter)"
#endif

// ----------------------------------------------------------------------------
// TRANSPORT_TYPE = 1 (ZigBee)
// ----------------------------------------------------------------------------
#if TRANSPORT_TYPE == 1

// ZigBee требует отключенного WiFi
#if FEATURE_WIFI_ENABLED == 1
#warning "TRANSPORT_TYPE=1 (ZigBee) disables FEATURE_WIFI_ENABLED"
#undef FEATURE_WIFI_ENABLED
#define FEATURE_WIFI_ENABLED 0
#endif

// ZigBee недоступен на неподдерживаемых платформах
#if !PLATFORM_ESP32C6 && !PLATFORM_ESP32H2
#error "TRANSPORT_TYPE=1 (ZigBee) is only supported on ESP32-C6 and ESP32-H2"
#endif

// ZigBee отключает провизионинг
#if PROVISIONING_METHOD != 0
#warning "TRANSPORT_TYPE=1 (ZigBee) disables PROVISIONING_METHOD"
#undef PROVISIONING_METHOD
#define PROVISIONING_METHOD 0
#endif

// ZigBee отключает WEB (не нужен)
#if FEATURE_WEB_ENABLED == 1
#warning "TRANSPORT_TYPE=1 (ZigBee) disables FEATURE_WEB_ENABLED"
#undef FEATURE_WEB_ENABLED
#define FEATURE_WEB_ENABLED 0
#endif

// ZigBee отключает OTA (пока не реализован для ZigBee)
#if FEATURE_OTA_ENABLED == 1
#warning \
    "TRANSPORT_TYPE=1 (ZigBee) disables FEATURE_OTA_ENABLED (not yet supported)"
#undef FEATURE_OTA_ENABLED
#define FEATURE_OTA_ENABLED 0
#endif

// ZigBee отключает сканирование WiFi
#if FEATURE_SCANNING_WIFI_ENABLED == 1
#warning "TRANSPORT_TYPE=1 (ZigBee) disables FEATURE_SCANNING_WIFI_ENABLED"
#undef FEATURE_SCANNING_WIFI_ENABLED
#define FEATURE_SCANNING_WIFI_ENABLED 0
#endif

// ----------------------------------------------------------------------------
// TRANSPORT_TYPE = 0 (MQTT) или 2 (Matter) — требуют WiFi
// ----------------------------------------------------------------------------
#elif TRANSPORT_TYPE == 0 || TRANSPORT_TYPE == 2

// MQTT/Matter требуют WiFi
#if FEATURE_WIFI_ENABLED == 0
#error "TRANSPORT_TYPE=0 (MQTT) or 2 (Matter) requires FEATURE_WIFI_ENABLED=1"
#endif

// Matter требует поддержки (перспективно)
#if TRANSPORT_TYPE == 2
#if !PLATFORM_ESP32C6 && !PLATFORM_ESP32H2
#warning "TRANSPORT_TYPE=2 (Matter) is experimental and only on ESP32-C6/H2"
#endif
#endif

#endif  // TRANSPORT_TYPE == 1

// ----------------------------------------------------------------------------
// ESP32-C3 НЕ ПОДДЕРЖИВАЕТ ОДНОВРЕМЕННУЮ РАБОТУ BLE+AP
// ----------------------------------------------------------------------------
#if PROVISIONING_METHOD == 3 && PLATFORM_ESP32C3
#error "ESP32-C3 does not support simultaneous BLE+AP (PROVISIONING_METHOD=3)."
#endif


// ----------------------------------------------------------------------------
// OTA требует WEB
// ----------------------------------------------------------------------------
#if FEATURE_OTA_ENABLED == 1 && FEATURE_WEB_ENABLED == 0
#error "FEATURE_OTA_ENABLED=1 requires FEATURE_WEB_ENABLED=1"
#endif

// ----------------------------------------------------------------------------
// WEB требует WiFi
// ----------------------------------------------------------------------------
#if FEATURE_WEB_ENABLED == 1 && FEATURE_WIFI_ENABLED == 0
#error "FEATURE_WEB_ENABLED=1 requires FEATURE_WIFI_ENABLED=1"
#endif

// ----------------------------------------------------------------------------
// TYPE 1 требует датчик
// ----------------------------------------------------------------------------
#if DEVICE_TYPE == 1 && FEATURE_SENSOR_ENABLED == 0
#error "DEVICE_TYPE=1 requires FEATURE_SENSOR_ENABLED=1"
#endif

// ----------------------------------------------------------------------------
// TYPE 2 требует датчик
// ----------------------------------------------------------------------------
#if DEVICE_TYPE == 2 && FEATURE_SENSOR_ENABLED == 0
#error "DEVICE_TYPE=2 requires FEATURE_SENSOR_ENABLED=1"
#endif

// ----------------------------------------------------------------------------
// TYPE 3 не требует датчик (принудительно отключаем)
// ----------------------------------------------------------------------------
#if DEVICE_TYPE == 3 && FEATURE_SENSOR_ENABLED == 1
#warning "DEVICE_TYPE=3 ignores FEATURE_SENSOR_ENABLED"
#undef FEATURE_SENSOR_ENABLED
#define FEATURE_SENSOR_ENABLED 0
#endif

// ----------------------------------------------------------------------------
// ESP8266 не поддерживает BLE
// ----------------------------------------------------------------------------
#if PROVISIONING_METHOD == 1 || PROVISIONING_METHOD == 3
#if PLATFORM_ESP8266
#warning "ESP8266 does not support BLE, forcing PROVISIONING_METHOD=2"
#undef PROVISIONING_METHOD
#define PROVISIONING_METHOD 2
#endif
#endif

// ----------------------------------------------------------------------------
// Проверка PROVISIONING_METHOD
// ----------------------------------------------------------------------------
#if PROVISIONING_METHOD < 0 || PROVISIONING_METHOD > 3
#warning "PROVISIONING_METHOD must be 0-3, forcing 0"
#undef PROVISIONING_METHOD
#define PROVISIONING_METHOD 0
#endif

// ----------------------------------------------------------------------------
// Проверка DEVICE_TYPE
// ----------------------------------------------------------------------------
#if DEVICE_TYPE < 1 || DEVICE_TYPE > 3
#error "DEVICE_TYPE must be 1, 2, or 3"
#endif

#endif  // SETTINGS_H