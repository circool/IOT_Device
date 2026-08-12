/**
 * @file common_types.h
 * @brief Единые структуры данных для всех слоёв
 * @note Статус: Рефакторинг
 * @version 0.12
 * @date 10.08.2026
 */

#ifndef COMMON_TYPES_H
#define COMMON_TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include "settings.h"

// ============================================================
// TRANSPORT CONFIG (параметры канала связи)
// ============================================================

/**
 * @brief Конфигурация транспорта - настройки WiFi/MQTT итд
 */
typedef struct {
  char deviceId[32];

#ifdef USE_WIFI
  char wifiSsid[32];
  char wifiPassword[64];
#endif

#ifdef USE_MQTT
  char mqttBroker[64];
  uint16_t mqttPort;
  char mqttUser[32];
  char mqttPassword[64];
  char mqttClientId[24];
#endif

#ifdef USE_ZIGBEE
  uint16_t zigbeePanId;
  uint8_t zigbeeChannel;
  char zigbeeNetworkKey[32];
#endif
} TransportConfig;

// ============================================================
// TRANSPORT STATE (состояние подключения, режима настройки)
// ============================================================
/**
 * @brief Cостояния транспорта - статусы подключения, режима настройки
 */
typedef struct {
  bool link_ok;     ///< Соединение с точкой доступа (WiFi/Zigbee)
  bool gateway_ok;  ///< Соединение с брокером/шлюзом (MQTT/HTTP)
  bool setup_mode;  ///< Режим настройки (AP режим)
} TransportState;


// ============================================================
// DEVICE SETTINGS (настройки устройства, EEPROM)
// ============================================================

/**
 * @brief Настройки устройства - пороги, параметры итд
 */
typedef struct {
#if DEVICE_TYPE == 1
  bool sensorMode;          // TRUE = SENSOR, FALSE = MANUAL
  bool adaptiveMode;        // TRUE = адаптивный режим включён
  float lowTemp;
  float highTemp;
  float lowHum;
  float highHum;
  uint8_t speedPercent;  // 0-100%
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  uint32_t delaySeconds;  // Задержка включения (сек)
  uint32_t maxOnTime;     // Аварийное отключение (сек)
  uint8_t bootState;      // 0 = OFF, 1 = ON
#endif
} DeviceConfig;

typedef enum {
  DEVICE_COMMAND,   // Управление устройством
  STATE_CHANGED,    // Изменение состояния транспорта
  DEVICE_CONFIG,    // Новая конфигурация для устройства
  TRANSPORT_CONFIG  // Новая конфигурация для транспорта
} TransportEvent;



// ============================================================
// RESET REASON (CHECK ENGINE)
// ============================================================

/**
 * @brief Причины перезагрузки устройства
 * @note Используется для диагностики и CHECK ENGINE
 */
typedef enum {
  RESET_REASON_NONE = 0,
  RESET_REASON_POWER_ON,          ///< Включение питания
  RESET_REASON_WATCHDOG,          ///< Срабатывание сторожевого таймера
  RESET_REASON_EXCEPTION,         ///< Исключение CPU (Panic)
  RESET_REASON_SOFT_RESET,        ///< Программная перезагрузка (ESP.restart())
  RESET_REASON_FACTORY_RESET,     ///< Сброс настроек пользователем
  RESET_REASON_BROWNOUT,          ///< Падение напряжения питания
  RESET_REASON_SW_CPU_RESET,      ///< Сброс CPU сторожем задач (Task WDT)
  RESET_REASON_DEEP_SLEEP_AWAKE,  ///< Пробуждение из глубокого сна
  RESET_REASON_EXT_SYS_RST        ///< Сброс внешним сигналом (кнопка RESET)
} ResetReason;

// ============================================================
// DEVICE STATE (оперативное состояние, RAM)
// ============================================================
/**
 * @brief Состояние устройства (оперативное)
 */
typedef struct {
  // ===== СОСТОЯНИЕ АКТУАТОРА =====
  bool isOn;            // Актуатор включён
  uint8_t speed;        // 0-100%
  bool manualMode;      // TRUE = ручной режим (пользователь переключил)
  bool adaptiveMode;  // TRUE = адаптивный режим активен
  bool sensorMode;    // TRUE = режим сенсора активен

  // ===== ТАЙМЕРЫ (остатки) =====
  uint32_t delayRemain;  // Остаток таймера задержки (сек)
  uint32_t maxOnRemain;  // Остаток аварийного таймера (сек) (-1 = Аварийное
                         // отключение произошло)

  // ===== ДАННЫЕ ДАТЧИКА =====
  float temperature;
  float humidity;
  bool sensorValid;

} DeviceState;

// ============================================================
// STATE CHANGE FLAGS (маска изменений)
// ============================================================

/**
 * @brief Битовые флаги для маски изменений оперативного состояния устройства 
 * @details Используются в колбэке DeviceController для указания,
 *          какие поля DeviceState изменились.
 * 
 * @note Флаги можно комбинировать через побитовое ИЛИ (|)
 * @example (changes & STATE_CHANGED_IS_ON) — проверка изменения isOn
 * 
 * @warning Маска 0xFFFFFFFF означает "изменились все поля"
 *          (используется при инициализации)
 */
typedef enum {
    STATE_CHANGED_IS_ON           = (1 << 0),  ///< Изменилось состояние актуатора (isOn)
    STATE_CHANGED_SPEED           = (1 << 1),  ///< Изменилась скорость вентилятора (speed)
    STATE_CHANGED_MANUAL_MODE     = (1 << 2),  ///< Изменился ручной режим (manualMode)
    STATE_CHANGED_ADAPTIVE_MODE   = (1 << 3),  ///< Изменился адаптивный режим (adaptiveMode)
    STATE_CHANGED_SENSOR_MODE     = (1 << 4),  ///< Изменился режим управления по датчику (sensorMode)
    STATE_CHANGED_TEMPERATURE     = (1 << 5),  ///< Изменилась температура (temperature)
    STATE_CHANGED_HUMIDITY        = (1 << 6),  ///< Изменилась влажность (humidity)
    STATE_CHANGED_SENSOR_VALID    = (1 << 7),  ///< Изменилась валидность датчика (sensorValid)
    STATE_CHANGED_DELAY_REMAIN    = (1 << 8),  ///< Изменился остаток таймера задержки (delayRemain)
    STATE_CHANGED_MAX_ON_REMAIN   = (1 << 9),  ///< Изменился остаток аварийного таймера (maxOnRemain)
} StateChangeFlags;

typedef struct {
  TransportEvent event;              // Тип события -
  const TransportState* state;       // Для STATE_CHANGED
  const TransportConfig* transport;  // Для CONFIG (настройки транспорта)
  const DeviceState* deviceState;    // Для STATE/COMMAND (состояние устройства)
  const DeviceConfig* device;        // Для CONFIG (настройки устройства)

} TransportEventData;

// ============================================================
// BUTTON STAGE (стадии нажатия кнопки)
// ============================================================

/**
 * @brief Стадии нажатия кнопки
 */
enum ButtonStage : uint8_t {
  BUTTON_IDLE = 0,   ///< Кнопка отпущена или удержание > 5с
  BUTTON_SHORT = 1,  ///< Нажата < 0.5с и отпущена (событие)
  BUTTON_MID = 2,    ///< Удержание 1-2с (состояние)
  BUTTON_LONG = 3,   ///< Удержание 2-3с (состояние)
  BUTTON_WARN = 4,   ///< Удержание 3-5с (состояние, предупреждение)
  BUTTON_HOLD = 5    ///< Удержание 4-5с и отпущена (событие, сброс)
};

#endif  // COMMON_TYPES_H