#ifndef SWITCH_ACTUATOR_H
#define SWITCH_ACTUATOR_H

#include "actuator_base.h"

/**
 * @brief Управление выключателем без ШИМ (только вкл/выкл)
 *
 * Прокси-класс над ActuatorBase.
 * Используется при DEVICE_TYPE == 3 (управляемый выключатель)
 */
class SwitchActuator {
 public:
  SwitchActuator();

  /**
   * @brief Инициализация выключателя
   * @param pin GPIO для управления реле
   * @param relayOnLevel Уровень включения (HIGH/LOW)
   * @param bootState Состояние при старте (true=вкл)
   * @param delaySeconds Задержка отложенного включения (сек)
   * @param maxOnTime Таймер аварийного отключения (сек)
   */
  void init(uint8_t pin,
            uint8_t relayOnLevel,
            bool bootState,
            int delaySeconds,
            uint32_t maxOnTime);

  /**
   * @brief Периодический вызов в loop()
   * @param delaySeconds Текущая задержка отложенного включения (сек)
   * @param maxOnTime Текущее время аварийного отключения (сек)
   */
  void update(int delaySeconds, uint32_t maxOnTime);

  /**
   * @brief Установить состояние
   * @param on true — включить, false — выключить
   * @param manual true — ручная команда
   */
  void set(bool on, bool manual = true);

  /**
   * @brief Получить текущее состояние
   */
  bool getState() const;

  /**
   * @brief Обновить конфигурацию во время работы
   * @param delaySeconds Новая задержка отложенного включения (сек)
   * @param maxOnTime Новое время аварийного отключения (сек)
   */
  void updateConfig(int delaySeconds, uint32_t maxOnTime);

  // Прокси-методы для доступа к таймерам базового класса
  unsigned long getStartTime() const { return _base.getStartTime(); }
  bool isDelayActive() const { return _base.isDelayActive(); }
  unsigned long getDelayTimer() const { return _base.getDelayTimer(); }
  bool isEmergencyStop() const { return _base.isEmergencyStop(); }

  // Статические колбэки
  static void onSetPhysicalCallback(void* context, bool on);
  static void onForceStopCallback(void* context);

 private:
  ActuatorBase _base;  // Делегирование базовому классу
  uint8_t _pin;
  uint8_t _relayOnLevel;
  int _delaySeconds;
  uint32_t _maxOnTime;
};

#endif