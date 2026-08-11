/**
 * @file device_controller.h
 * @brief Бизнес-логика устройства
 * @version 0.13
 * @date 11.08.2026
 */

#ifndef DEVICE_CONTROLLER_H
#define DEVICE_CONTROLLER_H

#include "common_types.h"
#include "device_controller_fan_actuator.h"
#include "device_controller_sensor.h"
#include "device_controller_switch_actuator.h"



// ============================================================
// КОЛБЭК
// ============================================================

/**
 * @brief Тип функции-колбэка для уведомления об изменении состояния
 * @param changes Битовая маска изменений (StateChangeFlags)
 *
 * @note Само состояние доступно через указатель, полученный в init()
 */
typedef void (*DeviceControllerCallback)(uint32_t changes);

// ============================================================
// КЛАСС DEVICE_CONTROLLER
// ============================================================

class DeviceController {
 public:
  DeviceController();

  /**
   * @brief Инициализация контроллера
   * @param config Указатель на DeviceConfig (из ConfigManager)
   * @param outState Ссылка на указатель, куда будет записан адрес _state
   *
   * @note Вызывается один раз при старте системы.
   *       Сохраняет ссылку на конфиг, инициализирует Sensor и Actuator,
   *       вычисляет начальное состояние DeviceState из config.
   *       После инициализации:
   *       - Записывает адрес _state в outState
   *       - Вызывает notifyChange(0xFFFFFFFF)
   */
  void init(const DeviceConfig* config, const DeviceState*& outState);

  void setOn(bool on);
  void setSpeed(uint8_t percent);
  void setSensorMode(bool on);
  void setAdaptiveMode(bool on);

  void update();
  void onStateChanged(DeviceControllerCallback callback);

 private:
  const DeviceConfig* _config;
  DeviceState _state;
  bool _changed;

  Sensor _sensor;
#if DEVICE_TYPE == 1
  FanActuator _actuator;
#elif DEVICE_TYPE == 3
  SwitchActuator _actuator;
#endif

  DeviceControllerCallback _callback;

  bool _delayTimerRunning;
  unsigned long _delayTimerStart;

  void notifyChange(uint32_t changes);
  void applyStateToActuator();

  int calculateAdaptiveSpeed() const;
  bool updateDelayTimer(uint32_t& changes);
  bool updateTimerRemains(uint32_t& changes);
};

#endif  // DEVICE_CONTROLLER_H