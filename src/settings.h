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

// Макросы для наглядности
#define TRANSPORT_TYPE_NONE 0
#define TRANSPORT_TYPE_WIFI 1
#define TRANSPORT_TYPE_ZIGBEE 2
#define TRANSPORT_TYPE_THREAD 3

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
// 2. ТРАНСПОРТ (ФИЗИЧЕСКИЙ НОСИТЕЛЬ)
// ============================================================================

/**
 * @brief Тип физического носителя (транспортного уровня)
 * @details Определяет, какой радиостек используется.
 *          Только один транспорт может быть активен одновременно.
 * @values:
 *   - 0: NONE     — без транспорта (автономный режим)
 *   - 1: WIFI     — WiFi (802.11) — для MQTT, Web, Matter over WiFi
 *   - 2: ZIGBEE   — ZigBee (802.15.4 + полный стек)
 *   - 3: THREAD   — Thread (802.15.4 + 6LoWPAN) — для Matter over Thread
 */


#ifndef TRANSPORT_TYPE
// ZigBee по умолчанию для платформ с аппаратной поддержкой
#if PLATFORM_ESP32C6 || PLATFORM_ESP32H2
#define TRANSPORT_TYPE TRANSPORT_TYPE_ZIGBEE
#else
#define TRANSPORT_TYPE TRANSPORT_TYPE_WIFI
#endif
#endif

// ============================================================================
// 3. ПРИКЛАДНЫЕ ПРОТОКОЛЫ (независимые флаги, но с проверками зависимостей)
// ============================================================================

/**
 * @brief Включить MQTT-клиент
 * @details Требует TRANSPORT_TYPE == 1 (WIFI)
 *          Может работать одновременно с FEATURE_WEB_ENABLED
 */
#ifndef FEATURE_MQTT_ENABLED
// По умолчанию включаем, если выбран WiFi
#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI
#define FEATURE_MQTT_ENABLED 1
#else
#define FEATURE_MQTT_ENABLED 0
#endif
#endif

/**
 * @brief Включить веб-интерфейс по умолчанию
 * @details Требует TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI (WIFI)
 *          Может работать одновременно с FEATURE_MQTT_ENABLED
 */
#ifndef FEATURE_WEB_STATUS_ENABLED
#define FEATURE_WEB_STATUS_ENABLED 1
#endif

#if PROVISIONING_METHOD == 2 || PROVISIONING_METHOD == 3
/** @brief Веб-интерфейс нужен для провизионинга
 * @todo: найти место где можно безопасно переиниировать эту константу
 */
#ifndef FEATURE_WEB_ENABLED
#define FEATURE_WEB_ENABLED 1
#endif
#endif  // PROVISIONING_METHOD == 2 || PROVISIONING_METHOD == 3

/**
 * @brief Включить Matter
 * @details Требует TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI (WIFI) ИЛИ 3 (THREAD)
 *          Поддерживается на ESP32-C6/H2 (перспективно)
 */
#ifndef FEATURE_MATTER_ENABLED
#define FEATURE_MATTER_ENABLED 0
#endif

// ============================================================================
// 4. ФУНКЦИОНАЛЬНЫЕ ВОЗМОЖНОСТИ (FEATURES)
// ============================================================================

/**
 * @brief Включить логирование
 * @details Если 0 — весь код логирования исключается.
 *          Настройки (уровень, категории, цвет) определяются в logger.h
 */
#ifndef FEATURE_LOGGER_ENABLED
#define FEATURE_LOGGER_ENABLED 1
#endif

/**
 * @brief Включить светодиодную индикацию
 * @details Если 0 — весь код индикации исключается.
 *          Пин и инверсия определяются в led_manager.h
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
 * @brief Выключить OTA-обновления по умолчанию
 * @details Если 1 — доступно обновление прошивки через веб-интерфейс.
 * @note Требует FEATURE_WEB_ENABLED=1
 */
#ifndef FEATURE_OTA_ENABLED
#define FEATURE_OTA_ENABLED 0
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
 * @note Требует TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI (WIFI)
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
// 5. PROVISIONING (МЕТОД НАСТРОЙКИ)
// ============================================================================

/**
 * @brief Метод комиссионинга (настройки WiFi)
 * @values:
 *   - 0: Нет
 *   - 1: Только BLE
 *   - 2: Только AP (точка доступа)
 *   - 3: BLE + AP (одновременно)
 * @note Требует TRANSPORT_TYPE == 1 (WIFI)
 * @note ESP32-C3 не поддерживает BLE+AP одновременно (значение 3)
 * @note ESP8266 не поддерживает BLE
 */
#ifndef PROVISIONING_METHOD
#define PROVISIONING_METHOD 2  // AP по умолчанию
#endif

/**
 * @brief Включить AP-провизионинг (производная константа)
 */
#if PROVISIONING_METHOD == 2 || PROVISIONING_METHOD == 3
#define USE_AP_PROVISIONING 1
#else
#define USE_AP_PROVISIONING 0
#endif

/**
 * @brief Включить BLE-провизионинг (производная константа)
 */
#if PROVISIONING_METHOD == 1 || PROVISIONING_METHOD == 3
#define USE_BLE_PROVISIONING 1
#else
#define USE_BLE_PROVISIONING 0
#endif

// ============================================================================
// 6. ПРОВЕРКИ ЗАВИСИМОСТЕЙ
// ============================================================================

// ----------------------------------------------------------------------------
// 6.1. Проверка TRANSPORT_TYPE
// ----------------------------------------------------------------------------
#if TRANSPORT_TYPE < 0 || TRANSPORT_TYPE > 3
#error "TRANSPORT_TYPE must be 0 (NONE), 1 (WIFI), 2 (ZIGBEE), or 3 (THREAD)"
#endif

// ----------------------------------------------------------------------------
// 6.2. Проверка платформы для ZigBee и Thread
// ----------------------------------------------------------------------------
#if TRANSPORT_TYPE == TRANSPORT_TYPE_ZIGBEE || TRANSPORT_TYPE == TRANSPORT_TYPE_THREAD
#if !PLATFORM_ESP32C6 && !PLATFORM_ESP32H2
#error \
    "TRANSPORT_TYPE=2 (ZIGBEE) or 3 (THREAD) is only supported on ESP32-C6 and ESP32-H2"
#endif
#endif

// ----------------------------------------------------------------------------
// 6.3. Проверка: прикладные протоколы требуют WiFi
// ----------------------------------------------------------------------------
#if FEATURE_MQTT_ENABLED == 1 && TRANSPORT_TYPE != TRANSPORT_TYPE_WIFI
#error "FEATURE_MQTT_ENABLED requires TRANSPORT_TYPE=TRANSPORT_TYPE_WIFI (WIFI)"
#endif

#if FEATURE_WEB_STATUS_ENABLED == 1 && TRANSPORT_TYPE != TRANSPORT_TYPE_WIFI
#error "FEATURE_WEB_STATUS_ENABLED requires TRANSPORT_TYPE=1 (WIFI)"
#endif

// Matter требует WiFi или Thread
#if FEATURE_MATTER_ENABLED == 1
#if TRANSPORT_TYPE != TRANSPORT_TYPE_WIFI && TRANSPORT_TYPE != TRANSPORT_TYPE_THREAD
#error \
    "FEATURE_MATTER_ENABLED requires TRANSPORT_TYPE=TRANSPORT_TYPE_WIFI or TRANSPORT_TYPE_THREAD"
#endif
#endif

// ----------------------------------------------------------------------------
// 6.4. OTA требует Web
// ----------------------------------------------------------------------------
#if FEATURE_OTA_ENABLED == 1 && FEATURE_WEB_ENABLED == 0
#error "FEATURE_OTA_ENABLED=1 requires FEATURE_WEB_ENABLED=1"
#endif

// ----------------------------------------------------------------------------
// 6.5. Сканирование WiFi требует WiFi
// ----------------------------------------------------------------------------
#if FEATURE_SCANNING_WIFI_ENABLED == 1 && TRANSPORT_TYPE != 1
#error "FEATURE_SCANNING_WIFI_ENABLED requires TRANSPORT_TYPE=1 (WIFI)"
#endif

// ----------------------------------------------------------------------------
// 6.6. Provisioning требует WiFi
// ----------------------------------------------------------------------------
#if PROVISIONING_METHOD != 0 && TRANSPORT_TYPE != 1
#error "PROVISIONING_METHOD requires TRANSPORT_TYPE=1 (WIFI)"
#endif

// ----------------------------------------------------------------------------
// 6.7. ESP32-C3 не поддерживает BLE+AP одновременно
// ----------------------------------------------------------------------------
#if PROVISIONING_METHOD == 3 && PLATFORM_ESP32C3
#error "ESP32-C3 does not support simultaneous BLE+AP (PROVISIONING_METHOD=3)"
#endif

// ----------------------------------------------------------------------------
// 6.8. ESP8266 не поддерживает BLE
// ----------------------------------------------------------------------------
#if PROVISIONING_METHOD == 1 || PROVISIONING_METHOD == 3
#if PLATFORM_ESP8266
#error "ESP8266 does not support BLE (PROVISIONING_METHOD=1 or 3)"
#endif
#endif

// ----------------------------------------------------------------------------
// 6.9. Проверка PROVISIONING_METHOD
// ----------------------------------------------------------------------------
#if PROVISIONING_METHOD < 0 || PROVISIONING_METHOD > 3
#error "PROVISIONING_METHOD must be 0, 1, 2, or 3"
#endif

// ----------------------------------------------------------------------------
// 6.10. DEVICE TYPE требует датчик
// ----------------------------------------------------------------------------
#if DEVICE_TYPE == 1 && FEATURE_SENSOR_ENABLED == 0
#error "DEVICE_TYPE=1 requires FEATURE_SENSOR_ENABLED=1"
#endif

#if DEVICE_TYPE == 2 && FEATURE_SENSOR_ENABLED == 0
#error "DEVICE_TYPE=2 requires FEATURE_SENSOR_ENABLED=1"
#endif

// ----------------------------------------------------------------------------
// 6.11. TYPE 3 не требует датчик (принудительно отключаем)
// ----------------------------------------------------------------------------
#if DEVICE_TYPE == 3 && FEATURE_SENSOR_ENABLED == 1
#warning "DEVICE_TYPE=3 ignores FEATURE_SENSOR_ENABLED"
#undef FEATURE_SENSOR_ENABLED
#define FEATURE_SENSOR_ENABLED 0
#endif

// ----------------------------------------------------------------------------
// 6.12. Проверка DEVICE_TYPE
// ----------------------------------------------------------------------------
#if DEVICE_TYPE < 1 || DEVICE_TYPE > 3
#error "DEVICE_TYPE must be 1, 2, or 3"
#endif

// ----------------------------------------------------------------------------
// 6.13. Предупреждение: WiFi выбран, но ни один протокол не активен
// ----------------------------------------------------------------------------
#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI
#if FEATURE_MQTT_ENABLED == 0 && FEATURE_WEB_STATUS_ENABLED == 0 && \
    FEATURE_MATTER_ENABLED == 0
#warning "TRANSPORT_TYPE=1 (WIFI) selected but no application protocol (MQTT/WEB/MATTER) is enabled"
#endif
#endif

#endif  // SETTINGS_H