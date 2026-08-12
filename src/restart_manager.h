/**
 * @file restart_manager.h
 * @brief Централизованное управление перезагрузкой
 * @version 0.12
 * @date 10.08.2026
 */

#ifndef RESTART_MANAGER_H
#define RESTART_MANAGER_H

#include <Arduino.h>
#include <stdbool.h>
#include "settings.h"

// ============ Основной код ============

#ifdef USE_RESTART

/**
 * @class RestartManager
 * @brief Менеджер перезагрузки — единая точка вызова ESP.restart()
 * @details Предотвращает хаотичные перезагрузки из разных мест кода.
 *          Все слои вызывают request(), фактический вызов ESP.restart()
 *          происходит централизованно в update().
 *          Не содержит логики работы с RTC — это задача debug_tools.
 */
class RestartManager {
 public:
  /**
   * @brief Запрос перезагрузки с задержкой
   * @param delayMs Задержка перед перезагрузкой в мс (по умолчанию 500)
   * @details Если перезагрузка уже запрошена — игнорирует вызов.
   *          Логирует запрос с указанием задержки.
   */
  void request(unsigned long delayMs = 500);

  /**
   * @brief Периодическая проверка необходимости перезагрузки
   * @details Вызывается в loop() последним.
   *          Если перезагрузка запрошена и задержка истекла — вызывает
   * ESP.restart(). Единственное место в проекте, где вызывается ESP.restart().
   */
  void update();

  /**
   * @brief Проверка, запрошена ли перезагрузка
   * @return true если перезагрузка запрошена и ожидает выполнения
   */
  bool isPending() const;

  /**
   * @brief Отмена запланированной перезагрузки
   * @details Сбрасывает флаг _pending, логирует отмену.
   */
  void cancel();

 private:
  bool _pending;               ///< Флаг запрошенной перезагрузки
  unsigned long _requestTime;  ///< Время запроса
  unsigned long _delayMs;      ///< Задержка перед перезагрузкой (мс)
};

// ============ Заглушки (RESTART отключён) ============

#else  // USE_RESTART не определён

/**
 * @brief Заглушка — RESTART отключён (USE_RESTART не определён)
 * @details Все методы пустые, isPending() всегда возвращает false
 */
class RestartManager {
 public:
  inline void request(unsigned long /*delayMs*/ = 500) {}
  inline void update() {}
  inline bool isPending() const { return false; }
  inline void cancel() {}
};

#endif  // USE_RESTART

#endif  // RESTART_MANAGER_H