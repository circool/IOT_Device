/**
 * @file button_manager.h
 * @brief Кнопка управления - обработка нажатий
 * @version 0.12
 * @date 10.08.2026
 */

#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <Arduino.h>
#include "common_types.h"

// ============ Конфигурация ============

#ifndef BUTTON_PIN
#define BUTTON_PIN 0
#endif

#ifndef BUTTON_INVERTED
#define BUTTON_INVERTED 1  // 1 = LOW активен (подтяжка к GND)
#endif

// ============ Основной код ============

#ifdef USE_BUTTON

/**
 * @class ButtonManager
 * @brief Менеджер кнопки с определением стадий удержания
 * @details Алгоритм определения стадий:
 *          - BUTTON_IDLE: кнопка не нажата или удержание > 5с
 *          - BUTTON_SHORT: нажата < 0.5с и отпущена (событие)
 *          - BUTTON_MID: удерживается 1-2с (состояние)
 *          - BUTTON_LONG: удерживается 2-3с (состояние)
 *          - BUTTON_WARN: удерживается 3-5с (состояние, предупреждение)
 *          - BUTTON_HOLD: удерживалась 4-5с и отпущена (событие, сброс)
 */
class ButtonManager {
 public:
  /**
   * @brief Инициализация кнопки
   * @details Настраивает GPIO, сбрасывает внутреннее состояние.
   *          Использует константы BUTTON_PIN и BUTTON_INVERTED.
   */
  void init();

  /**
   * @brief Обновление состояния кнопки
   * @details Вызывается в loop(). Детектирует нажатие/удержание/отпускание
   *          и обновляет внутреннюю стадию.
   */
  void update();

  /**
   * @brief Получить текущую стадию нажатия
   * @return ButtonStage:
   *         - BUTTON_IDLE: кнопка не нажата или удержание > 5с
   *         - BUTTON_SHORT: нажатие < 0.5с (событие после отпускания)
   *         - BUTTON_MID: удержание 1-2с (активное состояние)
   *         - BUTTON_LONG: удержание 2-3с (активное состояние)
   *         - BUTTON_WARN: удержание 3-5с (активное состояние, предупреждение)
   *         - BUTTON_HOLD: удержание 4-5с и отпущена (событие, сброс)
   */
  ButtonStage getStage() const;

  /**
   * @brief Сброс событийных стадий
   * @details Сбрасывает BUTTON_SHORT и BUTTON_HOLD в BUTTON_IDLE.
   *          Используется оркестратором для однократной обработки событий.
   *          Стадии-состояния (MID, LONG, WARN) не сбрасываются.
   */
  void clearEvent();

 private:
  bool _isPressed;                ///< Текущее состояние (true = нажата)
  unsigned long _pressStartTime;  ///< Время начала нажатия
  ButtonStage _currentStage;      ///< Текущая стадия
  bool _lastState;  ///< Предыдущее состояние для детектирования фронтов

  /**
   * @brief Вычисление стадии по длительности (для удержания)
   * @param duration Длительность нажатия в мс
   * @return Стадия для текущего момента удержания
   */
  ButtonStage _calculateHoldStage(unsigned long duration) const;

  /**
   * @brief Вычисление финальной стадии при отпускании
   * @param duration Длительность нажатия в мс
   * @return Стадия, которая должна быть установлена после отпускания
   */
  ButtonStage _calculateReleaseStage(unsigned long duration) const;
};

// ============ Заглушки (кнопка отключена) ============

#else  // USE_BUTTON не определён

/**
 * @brief Заглушка — кнопка отключена (USE_BUTTON не определён)
 * @details Все методы пустые, getStage() всегда возвращает BUTTON_IDLE
 */
class ButtonManager {
 public:
  inline void init() {}
  inline void update() {}
  inline ButtonStage getStage() const { return BUTTON_IDLE; }
  inline void clearEvent() {}
};

#endif  // USE_BUTTON

#endif  // BUTTON_MANAGER_H