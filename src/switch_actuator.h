// ============================================================================
// @file switch_actuator.h
// @brief Управление дискретным выключателем (реле)
//
// Модуль реализует управление простым выключателем/реле без поддержки ШИМ.
// Предназначен для устройств типа DEVICE_TYPE == 3.
//
// @note Этот класс служит примером наследования от ActuatorBase для создания
//       новых типов актуаторов. При добавлении нового устройства (например,
//       освежитель воздуха, увлажнитель, ионизатор) используйте этот класс
//       как шаблон.
//
// @note В отличие от FanActuator, не требует сложной логики ШИМ и стартового
//       импульса. Достаточно просто переключить пин в нужное состояние.
// ============================================================================

#ifndef SWITCH_ACTUATOR_H
#define SWITCH_ACTUATOR_H

#include "actuator_base.h"

/**
 * @class SwitchActuator
 * @brief Управление дискретным выключателем (реле)
 *
 * Обеспечивает:
 * - Включение/выключение нагрузки через реле
 * - Контроль максимального времени работы (наследуется от ActuatorBase)
 * - Аварийное отключение при превышении maxOnTime
 *
 * @note Физическое управление реализовано в методе onSetPhysical():
 *       - При включении: digitalWrite(pin)
 *       - При выключении: digitalWrite(pin)
 *
 * @note Отличие от FanActuator:
 *       - Нет ШИМ (скорость не регулируется)
 *       - Нет стартового импульса
 *       - Мгновенное переключение состояния
 *
 * @example
 * // Использование в main.cpp
 * SwitchActuator relay;
 * relay.init(SWITCH_PIN, ACTIVE_LEVEL, bootState, maxOnTime);
 * relay.set(true);  // включить
 * relay.set(false); // выключить
 */
class SwitchActuator : public ActuatorBase {
 public:
  /**
   * @brief Конструктор выключателя
   *
   * Вызывает конструктор ActuatorBase, который инициализирует:
   * - _pin = 0
   * - ACTIVE_LEVEL = LOW
   * - _state = false
   * - _startTime = 0
   * - _maxOnTime = 0
   * - _emergencyStop = false
   */
  SwitchActuator();

  /**
   * @brief Инициализация выключателя
   *
   * @param pin          Номер GPIO для управления реле

   * @param bootState    Состояние при старте (true = включено)
   * @param maxOnTime    Максимальное время непрерывной работы (сек)
   *                     0 = аварийное отключение выключено
   *
   * @note Вызывает ActuatorBase::init(), которая:
   *       - Настраивает пин в режим OUTPUT
   *       - Устанавливает начальное состояние
   *       - Запускает таймер maxOnTime при необходимости
   *
   * @see ActuatorBase::init()
   */
  void init(uint8_t pin,
            bool bootState,
            uint32_t maxOnTime);

 protected:
  /**
   * @brief Обработка физического включения/выключения
   *
   * Реализация чисто виртуального метода ActuatorBase::onSetPhysical().
   * Выполняет непосредственное управление пином через digitalWrite().
   *
   * @param on true = включить, false = выключить
   *
   * @note Логика:
   *       - При on = true:  digitalWrite(_pin, ACTIVE_LEVEL)
   *       - При on = false: digitalWrite(_pin, !ACTIVE_LEVEL)
   *
   * @note Не требует дополнительных задержек или сложной логики.
   *       Для вентилятора с ШИМ см. FanActuator.
   *
   * @see ActuatorBase::onSetPhysical()
   */
  void onSetPhysical(bool on) override;
};

#endif  // SWITCH_ACTUATOR_H