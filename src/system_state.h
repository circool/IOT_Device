/**
 * @file system_state.h
 * @brief Состояния системы
 * @details Единый источник правды для всех слоёв.
 *          LED, Restart Manager и другие слои читают это состояние.
 */

#ifndef SYSTEM_STATE_H
#define SYSTEM_STATE_H

#include <Arduino.h>

/**
 * @brief Состояния системы
 * @details Каждое состояние определяет поведение LED индикации
 */
enum class SystemState : uint8_t {
  INIT,           /**< Инициализация (setup) — LED выключен */
  NORMAL,         /**< Нормальная работа — LED горит постоянно */
  MODE_1,         /**< Нет WiFi ИЛИ кнопка 0-1с — 1 вспышка/сек */
  MODE_2,         /**< Нет MQTT ИЛИ кнопка 1-2с — 2 вспышки/сек */
  MODE_3,         /**< Provisioning (AP/BLE) ИЛИ кнопка 2-3с — 3 вспышки/сек */
  MODE_4,         /**< Аварийное отключение — медленное мигание */
  RESTART_PENDING /**< Перезагрузка (ожидание ESP.restart()) — LED выключен */
};

/**
 * @brief Установить состояние системы
 * @param state Новое состояние
 * @details Логирует изменение состояния и обновляет g_systemState.
 *          Используется вместо прямого присвоения g_systemState.
 */
void system_state_set(SystemState state);

/**
 * @brief Получить текущее состояние системы
 * @return Текущее состояние
 */
SystemState system_state_get();

/**
 * @brief Глобальное состояние системы
 * @details Определено в system_state.cpp.
 *          Доступно всем слоям через extern.
 * @note Для чтения используйте system_state_get(),
 *       для записи — system_state_set().
 */
extern SystemState g_systemState;

#endif  // SYSTEM_STATE_H