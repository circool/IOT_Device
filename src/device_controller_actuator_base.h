/**
 * @file device_controller_actuator_base.h
 * @brief Базовый класс для управления исполнительным механизмом
 */

#ifndef DEVICE_CONTROLLER_ACTUATOR_BASE_H
#define DEVICE_CONTROLLER_ACTUATOR_BASE_H

#include <Arduino.h>
#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Уровень сигнала для включения реле
 * @values HIGH или LOW
 */
#ifndef RELAY_ON_LEVEL
#define RELAY_ON_LEVEL LOW
#endif


class ActuatorBase {
 public:
  ActuatorBase();

  void init(uint8_t pin,
            uint8_t relayOnLevel,
            bool bootState,
            int delaySeconds,
            uint32_t maxOnTime);

  void set(bool on, bool manual = true);
  bool getState() const;
  void update(int delaySeconds, uint32_t maxOnTime);
  void forceStop();

  bool isEmergencyStop() const { return _emergencyStop; }
  unsigned long getStartTime() const { return _startTime; }
  bool isDelayActive() const { return _delayActive; }
  unsigned long getDelayTimer() const { return _delayTimer; }

  // ===== КОЛБЭКИ =====
  void (*onSetPhysicalCallback)(void*, bool);
  void (*onForceStopCallback)(void*);
  void (*onManualCommandCallback)(void*);
  void* callbackContext;

 protected:
  void checkMaxOnTime(uint32_t maxOnTime);
  bool delayTimer(bool start, int delaySeconds);

  uint8_t _pin;
  uint8_t _relayOnLevel;
  bool _state;
  unsigned long _startTime;
  bool _delayActive;
  unsigned long _delayTimer;
  bool _emergencyStop;
};

#endif  // DEVICE_CONTROLLER_ACTUATOR_BASE_H