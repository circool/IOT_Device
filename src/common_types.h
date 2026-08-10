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
// 1. TRANSPORT CONFIG (параметры канала связи)
// ============================================================

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
// 2. DEVICE SETTINGS (настройки устройства, EEPROM)
// ============================================================

typedef struct {
#if DEVICE_TYPE == 1
  bool sensorControlMode;  // TRUE = SENSOR, FALSE = MANUAL
  bool adaptiveMode;       // TRUE = адаптивный режим включён
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
// 3. DEVICE STATE (оперативное состояние, RAM)
// ============================================================

typedef struct {
  // ===== СОСТОЯНИЕ АКТУАТОРА =====
  bool isOn;            // Актуатор включён
  uint8_t speed;        // 0-100%
  bool manualMode;      // TRUE = ручной режим (пользователь переключил)
  bool adaptiveActive;  // TRUE = адаптивный режим активен

  // ===== ТАЙМЕРЫ (остатки) =====
  uint32_t delayRemain;  // Остаток таймера задержки (сек)
  uint32_t maxOnRemain;  // Остаток аварийного таймера (сек)

  // ===== ДАННЫЕ ДАТЧИКА =====
  float temperature;
  float humidity;
  bool sensorValid;

  // ===== СИСТЕМНЫЕ ФЛАГИ =====
  bool emergency;           // Аварийное отключение
  ResetReason resetReason;  // CHECK ENGINE (причина нештатной перезагрузки)
} DeviceState;

// ============================================================
// 4. BUTTON STAGE (стадии нажатия кнопки)
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