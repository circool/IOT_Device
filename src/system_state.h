/**
 * @file system_state.h
 * @brief Состояние системы (битовая маска)
 */

#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <Arduino.h>

// ================================================================
// БИТОВЫЕ ФЛАГИ СОСТОЯНИЯ УСТРОЙСТВА
// ================================================================

/**
 * @brief Битовые флаги состояния устройства
 * @details Каждый флаг — это состояние всего устройства как целого.
 *          Флаги можно комбинировать (битовая маска).
 */
enum SystemStateBit : uint16_t {
  STATE_NONE = 0,

  // ----- СОСТОЯНИЯ УСТРОЙСТВА (влияют на поведение других слоёв) -----
  STATE_BUTTON_PRESSED = 1 << 0,  // 1    — кнопка нажата
  STATE_WIFI_OK = 1 << 1,         // 2    — WiFi подключён
  STATE_MQTT_OK = 1 << 2,         // 4    — MQTT подключён
  STATE_PROVISIONING = 1 << 3,    // 8    — режим настройки
  STATE_EMERGENCY = 1 << 4,       // 16   — аварийное отключение
  STATE_RESTART = 1 << 5,         // 32   — ожидание перезагрузки

  // Маска всех битов
  STATE_ALL = STATE_BUTTON_PRESSED | STATE_WIFI_OK | STATE_MQTT_OK |
              STATE_PROVISIONING | STATE_EMERGENCY | STATE_RESTART,
};

// ================================================================
// API
// ================================================================

/**
 * @brief Инициализация состояния системы
 */
void system_state_init();

/**
 * @brief Установить бит (добавить состояние)
 */
void system_state_set_bit(uint16_t bit);

/**
 * @brief Снять бит (удалить состояние)
 */
void system_state_clear_bit(uint16_t bit);

/**
 * @brief Проверить факт установки бита
 */
bool system_state_has_bit(uint16_t bit);

/**
 * @brief Получить все биты
 */
uint16_t system_state_get_bits();

#endif