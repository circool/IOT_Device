/**
 * @file reset_button_manager.h
 * @brief Кнопка сброса
 * @details Обрабатывает нажатие кнопки, определяет стадию удержания.
 *          Устанавливает/снимает STATE_BUTTON_PRESSED в SystemState.
 */

#ifndef RESET_BTN_H
#define RESET_BTN_H

#include <Arduino.h>
#include "settings.h"

#ifndef RESET_PIN
#define RESET_PIN 0
#endif

/**
 * @brief Стадии нажатия кнопки
 */
enum ResetButtonStage : uint8_t {
  RELEASED = 0, /**< Кнопка отпущена */
  PRESSED = 1,  /**< Нажата 0-1с */
  STAGE_1S = 2, /**< Нажата 1-2с */
  STAGE_2S = 3, /**< Нажата 2-3с */
  STAGE_3S = 4  /**< Нажата >3с */
};

#if FEATURE_RESET_BUTTON_ENABLED == 1

/**
 * @brief Инициализация кнопки сброса
 */
void resetBtn_init();

/**
 * @brief Обновление состояния кнопки
 * @details Вызывается в loop(). Обновляет внутреннее состояние
 *          и устанавливает/снимает STATE_BUTTON_PRESSED.
 */
void resetBtn_update();

/**
 * @brief Получить текущую стадию нажатия кнопки
 * @return Стадия из ResetButtonStage
 * @note Вызывать ТОЛЬКО если system_state_has_bit(STATE_BUTTON_PRESSED)
 */
ResetButtonStage resetBtn_get_stage();

#else  // FEATURE_RESET_BUTTON_ENABLED == 0

inline void resetBtn_init() {}
inline void resetBtn_update() {}
inline ResetButtonStage resetBtn_get_stage() {
  return RELEASED;
}

#endif  // FEATURE_RESET_BUTTON_ENABLED

#endif  // RESET_BTN_H