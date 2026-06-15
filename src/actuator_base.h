// ============================================================================
// @file actuator_base.h
// @brief Базовый класс для управления исполнительным механизмом
//
// Обеспечивает:
// - Включение/выключение с сохранением состояния
// - Контроль максимального времени работы (аварийное отключение)
// - Виртуальный метод onSetPhysical() для платформозависимой реализации
// - Флаг аварийной остановки для индикации
//
// @note Этот класс не содержит логики ШИМ — только дискретное управление.
//       Для ШИМ используйте FanActuator.
//
// @note maxOnTime = 0 означает, что аварийное отключение выключено.
// ============================================================================

#ifndef ACTUATOR_BASE_H
#define ACTUATOR_BASE_H

#include <Arduino.h>

/**
 * @class ActuatorBase
 * @brief Базовый класс для управления реле/выключателем/вентилятором
 *
 * Реализует общую логику для всех исполнительных механизмов:
 * - Состояние включено/выключено (_state)
 * - Время последнего включения (_startTime) для контроля maxOnTime
 * - Флаг аварийной остановки (_emergencyStop)
 * - Таймер maxOnTime с автоматическим выключением
 *
 * @note Производные классы обязаны реализовать чисто виртуальный метод
 *       onSetPhysical(bool on), который выполняет физическое управление пином
 *       (с учётом инверсии, ШИМ и т.д.).
 *
 * @example
 * class MyActuator : public ActuatorBase {
 * protected:
 *     void onSetPhysical(bool on) override {
 *         digitalWrite(_pin, on ? ACTIVE_LEVEL : !ACTIVE_LEVEL);
 *     }
 * };
 */
class ActuatorBase {
 public:
  /**
   * @brief Конструктор
   *
   * Инициализирует все поля нулями/значениями по умолчанию:
   * - _pin = 0
   * - ACTIVE_LEVEL = LOW
   * - _state = false
   * - _startTime = 0
   * - _maxOnTime = 0
   * - _emergencyStop = false
   */
  ActuatorBase();

  /**
   * @brief Виртуальный деструктор
   *
   * @note Необходим для корректного удаления производных классов
   */
  virtual ~ActuatorBase() {}

  /**
   * @brief Инициализация актуатора
   *
   * Настраивает пин в режим OUTPUT, устанавливает начальное состояние
   * в зависимости от bootState, вызывает onSetPhysical() для применения.
   *
   * @param pin          Номер GPIO для управления
   * @param bootState    Состояние при старте (true = включено)
   * @param maxOnTime    Максимальное время непрерывной работы (сек)
   *                     0 = аварийное отключение выключено
   *
   * @note После init() пин находится в корректном состоянии (ON/OFF)
   * @note _startTime устанавливается в millis() при bootState = true
   */
  void init(uint8_t pin,
            bool bootState,
            uint32_t maxOnTime);

  /**
   * @brief Установить состояние актуатора
   *
   * @param on     true = включить, false = выключить
   * @param manual true = ручная команда (сбрасывает emergency stop),
   *               false = автоматическая (от датчика или таймера)
   *
   * @note При ручной команде сбрасывается флаг _emergencyStop
   * @note Если состояние не меняется, функция ничего не делает
   * @note Вызывает onSetPhysical() для применения изменения
   *
   * @see onSetPhysical()
   */
  void set(bool on, bool manual = true);

  /**
   * @brief Получить текущее состояние
   * @return true = включено, false = выключено
   */
  bool getState() const;

  /**
   * @brief Периодический вызов в loop()
   *
   * Вызывает checkMaxOnTime() для контроля аварийного отключения.
   *
   * @note Производные классы могут переопределять этот метод,
   *       но должны вызывать ActuatorBase::update() для проверки maxOnTime.
   */
  virtual void update();

  /**
   * @brief Проверить, активна ли аварийная остановка
   * @return true = актуатор остановлен из-за превышения maxOnTime
   */
  bool isEmergencyStop() const { return _emergencyStop; }

  /**
   * @brief Получить время последнего включения
   * @return millis() в момент последнего вызова set(true)
   */
  unsigned long getStartTime() const { return _startTime; }

  /**
   * @brief Сбросить флаг аварийной остановки
   *
   * @note Вызывается при ручной команде или перезагрузке.
   *       Не восстанавливает состояние — только сбрасывает флаг.
   */
  void clearEmergencyStop() { _emergencyStop = false; }

 protected:
  /**
   * @brief Чисто виртуальный метод для физического управления пином
   *
   * Вызывается из init() и set() при каждом изменении _state.
   *
   * @param on true = включить физически, false = выключить
   *
   * @note Производный класс обязан реализовать этот метод.
   * @note Должен учитывать ACTIVE_LEVEL и, при необходимости, ШИМ.
   *
   * @example
   * void onSetPhysical(bool on) override {
   *     digitalWrite(_pin, on ? ACTIVE_LEVEL : !ACTIVE_LEVEL);
   * }
   */
  virtual void onSetPhysical(bool on) = 0;

  /**
   * @brief Проверка превышения максимального времени работы
   *
   * Если _state == true, _maxOnTime > 0, и время работы превышает _maxOnTime,
   * вызывает set(false, false) и устанавливает _emergencyStop = true.
   *
   * @note Вызывается из update()
   */
  void checkMaxOnTime();

  // ========== ЗАЩИЩЁННЫЕ ПОЛЯ (доступны производным классам) ==========

  uint8_t _pin;              //!< Номер GPIO
  bool _state;               //!< Текущее состояние (true = включено)
  unsigned long _startTime;  //!< Время последнего включения (millis)
  uint32_t _maxOnTime;       //!< Максимальное время работы (сек), 0 = отключено
  bool _emergencyStop;       //!< Флаг аварийной остановки
};

#endif  // ACTUATOR_BASE_H