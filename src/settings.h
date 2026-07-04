#ifndef SETTINGS_H
#define SETTINGS_H
// ============================================================================
// ВЕРСИЯ ПРОШИВКИ
// ============================================================================
#ifndef VERSION
#define VERSION "1.0"
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

// ============================================================================
// АППАРАТНАЯ КОНФИГУРАЦИЯ
// ============================================================================

/** @brief Пин кнопки сброса настроек (GPIO0 обычно) */
#ifndef RESET_PIN
#define RESET_PIN 0
#endif

// ============================================================================
// ПРОВЕРКА ПЛАТФОРМЫ ДЛЯ ZIGBEE
// ============================================================================

/**
 * @brief Проверка: ZigBee доступен только на ESP32-C6 и ESP32-H2
 */
#if ZIGBEE_ENABLED == 1
#if defined(ESP32) && !defined(CONFIG_IDF_TARGET_ESP32C6)
#error \
    "ZigBee is only supported on ESP32-C6 and ESP32-H2. For ESP32/ESP32-C3 use MQTT_ENABLED=1"
#endif
#endif

/**
 * @brief Проверка: MQTT и ZigBee не могут быть включены одновременно
 */
#if MQTT_ENABLED == 1 && ZIGBEE_ENABLED == 1
#error "MQTT_ENABLED and ZIGBEE_ENABLED cannot be both enabled"
#endif

// ============================================================================
// АВТОМАТИЧЕСКОЕ ОПРЕДЕЛЕНИЕ ТИПА ТРАНСПОРТА (для обратной совместимости)
// ============================================================================

#ifndef MQTT_ENABLED
// Если не задан явно, включаем MQTT по умолчанию (для ESP32/ESP32-C3)
#if defined(ESP32) && !defined(CONFIG_IDF_TARGET_ESP32C6)
#define MQTT_ENABLED 1
#define ZIGBEE_ENABLED 0
#else
// Для ESP32-C6/H2 по умолчанию выключаем оба — пользователь должен выбрать явно
#define MQTT_ENABLED 0
#define ZIGBEE_ENABLED 0
#endif
#endif

#ifndef WEB_ENABLED
#define WEB_ENABLED 1
#endif

#ifndef OTA_ENABLED
#define OTA_ENABLED 1
#endif

#ifndef WDT_ENABLED
#define WDT_ENABLED 1
#endif
#ifndef WIFI_ENABLED
#define WIFI_ENABLED 1
#endif

// ============================================================================
// МЕТОД НАСТРОЙКИ (PROVISIONING)
// ============================================================================

/**
 * @brief Метод комиссионинга (первоначальной настройки)
 *
 * Определяется через build_flags в platformio.ini:
 *   -DPROVISIONING_METHOD=1   — BLE-комиссионинг (ESP32-C3/C6/H2)
 *   -DPROVISIONING_METHOD=2   — WiFi AP + Web (ESP8266 или fallback)
 *   -DPROVISIONING_METHOD=0   — AUTO (автоматический выбор)
 *
 * Если не определён, используется AUTO (0)
 */
#ifndef PROVISIONING_METHOD
#define PROVISIONING_METHOD 0  // 0=AUTO, 1=BLE, 2=AP
#endif

// ============================================================================
// ВЫБОР МЕТОДА НАСТРОЙКИ (на основе PROVISIONING_METHOD)
// ============================================================================

#if PROVISIONING_METHOD == 1
// Явно задан BLE
#define USE_BLE_PROVISIONING 1
#define USE_AP_PROVISIONING 0

#elif PROVISIONING_METHOD == 2
// Явно задан AP
#define USE_BLE_PROVISIONING 0
#define USE_AP_PROVISIONING 1

#else  // PROVISIONING_METHOD == 0 (AUTO)
// Автоматический выбор на основе платформы
#if defined(ESP8266)
// ESP8266 — нет BLE, только AP
#define USE_BLE_PROVISIONING 0
#define USE_AP_PROVISIONING 1

#elif defined(CONFIG_IDF_TARGET_ESP32C6)
// ESP32-C6/H2 — используем BLE (основной метод)
#define USE_BLE_PROVISIONING 1
#define USE_AP_PROVISIONING 0

#elif defined(CONFIG_IDF_TARGET_ESP32C3) || \
    defined(CONFIG_IDF_TARGET_ESP32S3) || defined(CONFIG_IDF_TARGET_ESP32)
// ESP32-C3/S3/классический ESP32 — есть BLE, но для стабильности оставляем AP
// При желании можно переключить на BLE через PROVISIONING_METHOD=1
#define USE_BLE_PROVISIONING 0
#define USE_AP_PROVISIONING 1

#else
// Неизвестная платформа — AP как fallback
#define USE_BLE_PROVISIONING 0
#define USE_AP_PROVISIONING 1
#endif
#endif

// ============================================================================
// BLE PROVISIONING
// ============================================================================

/**
 * @brief Максимальное количество попыток подключения к WiFi при
 * BLE-комиссионинге
 *
 * При вводе неверного пароля или SSID устройство делает до 3 попыток,
 * после чего переходит в состояние ошибки.
 *
 * Изменить можно в ble_server.h (MAX_WIFI_ATTEMPTS)
 */
#ifndef BLE_PROVISIONING_MAX_ATTEMPTS
#define BLE_PROVISIONING_MAX_ATTEMPTS 3
#endif

/**
 * @brief PIN-код для BLE-сопряжения (PoP — Proof of Possession)
 *
 * Используется в приложении ESP BLE Provisioning для подтверждения
 * права настройки устройства.
 */
#ifndef BLE_PROVISIONING_PIN
#define BLE_PROVISIONING_PIN "12345678"
#endif

/**
 * @brief Таймаут BLE-провизионинга (миллисекунды)
 *
 * Общее время, отведенное на процесс настройки.
 * По истечении таймаута устройство переходит в состояние ошибки.
 */
#ifndef BLE_PROVISIONING_TIMEOUT_MS
#define BLE_PROVISIONING_TIMEOUT_MS 120000  // 2 минуты
#endif

#endif  // SETTINGS_H