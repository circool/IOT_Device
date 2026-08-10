/**
 * @file led_manager.h
 * @brief Управление светодиодной индикацией
 * @version 0.12
 * @date 10.08.2026
 */

#ifndef LED_MANAGER_H
#define LED_MANAGER_H

#include <Arduino.h>
#include "settings.h"

// ============ Конфигурация ============

#ifndef STATUS_LED_PIN
#define STATUS_LED_PIN 0
#endif

#ifndef LED_INVERTED
#ifdef ESP32
#define LED_INVERTED 0
#else
#define LED_INVERTED 1
#endif
#endif

/**
 * @brief Интервал между началами серий (мс)
 * @details Определяет частоту повторения серий.
 *          Серия всегда длится 1000мс.
 *          1000 = серия каждую секунду (без паузы)
 *          1500 = серия 1с, пауза 0.5с
 *          2000 = серия 1с, пауза 1с
 */
#ifndef LED_SERIES_INTERVAL_MS
#define LED_SERIES_INTERVAL_MS 2000
#endif

/**
 * @brief Режимы светодиодной индикации
 */
enum LedMode : uint8_t {
  LED_OFF = 0,    ///< Постоянно выключен
  LED_ON,         ///< Постоянно включён
  LED_MORZE_E,    ///< 1 вспышка в серии
  LED_MORZE_I,    ///< 2 вспышки в серии
  LED_MORZE_S,    ///< 3 вспышки в серии
  LED_SLOW_BLINK  ///< Медленное мигание (1с ON, 1с OFF)
};

// ============ Основной код ============

#ifdef USE_LED

/**
 * @class LedManager
 * @brief Менеджер светодиодной индикации
 * @details Реализует неблокирующие паттерны мигания.
 *          Получает готовый режим от оркестратора через setMode().
 *          Не содержит логики принятия решений.
 *          Серия всегда длится 1000мс, интервал между сериями настраивается.
 */
class LedManager {
 public:
  /**
   * @brief Инициализация пина светодиода
   * @details Использует константы STATUS_LED_PIN и LED_INVERTED.
   *          Устанавливает начальное состояние в выключенное.
   */
  void init();

  /**
   * @brief Установка режима индикации
   * @param mode Режим из LedMode
   * @details Логирует смену режима на уровне DEBUG.
   */
  void setMode(LedMode mode);

  /**
   * @brief Обновление физического состояния LED
   * @details Вызывается в loop(). Применяет текущий режим.
   *          Использует millis() для неблокирующего мигания.
   *          Состояние пина обновляется только при изменении.
   */
  void update();

 private:
  LedMode _currentMode;  ///< Текущий установленный режим
  bool _state;           ///< Текущее состояние пина (true = включён)

  /**
   * @brief Проверка, должна ли гореть вспышка в текущий момент
   * @param now Текущее время в миллисекундах
   * @return true если вспышка активна
   */
  bool _isPulseActive(unsigned long now) const;

  /**
   * @brief Установка физического состояния пина
   * @param on true = включён, false = выключен
   */
  void _setPinState(bool on);
};

// ============ Заглушки (LED отключён) ============

#else  // USE_LED не определён

/**
 * @brief Заглушка — LED отключён (USE_LED не определён)
 * @details Все методы пустые, не занимают место в прошивке
 */
class LedManager {
 public:
  inline void init() {}
  inline void setMode(LedMode /*mode*/) {}
  inline void update() {}
};

#endif  // USE_LED

#endif  // LED_MANAGER_H