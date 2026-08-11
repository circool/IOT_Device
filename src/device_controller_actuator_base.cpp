/**
 * @file device_controller_actuator_base.cpp
 * @brief Реализация базового класса актуатора
 */

#include "device_controller_actuator_base.h"
#include "logger.h"




ActuatorBase::ActuatorBase()
    : _pin(0),
      _relayOnLevel(LOW),
      _state(false),
      _startTime(0),
      _delayActive(false),
      _delayTimer(0),
      _emergencyStop(false),
      onSetPhysicalCallback(nullptr),
      onForceStopCallback(nullptr),
      onManualCommandCallback(nullptr),
      callbackContext(nullptr) {}

void ActuatorBase::init(uint8_t pin,
                        uint8_t relayOnLevel,
                        bool bootState,
                        int delaySeconds,
                        uint32_t maxOnTime) {
  (void)delaySeconds;
  (void)maxOnTime;

  _pin = pin;
  _relayOnLevel = relayOnLevel;
  _delayActive = false;
  _delayTimer = 0;
  _startTime = 0;
  _emergencyStop = false;

  pinMode(_pin, OUTPUT);

  if (bootState) {
    if (onSetPhysicalCallback) {
      onSetPhysicalCallback(callbackContext, true);
    }
    _state = true;
    _startTime = millis();
  } else {
    if (onSetPhysicalCallback) {
      onSetPhysicalCallback(callbackContext, false);
    }
    _state = false;
    _startTime = 0;
  }

  XLOG_INFO(CAT_ACTUATOR, "Init: pin=%d, state=%s", _pin,
            _state ? "ON" : "OFF");
}

void ActuatorBase::set(bool on, bool manual) {
  if (manual) {
    if (onManualCommandCallback) {
      onManualCommandCallback(callbackContext);
    }
    if (_delayActive) {
      _delayActive = false;
      XLOG_DEBUG(CAT_ACTUATOR, "Manual command: delay cancelled");
    }
  }

  if (_state == on)
    return;

  _state = on;

  if (onSetPhysicalCallback) {
    onSetPhysicalCallback(callbackContext, _state);
  }

  if (_state) {
    _emergencyStop = false;
    _startTime = millis();
    // XLOG_DEBUG(CAT_ACTUATOR, "ON");
  } else {
    _startTime = 0;
    // XLOG_DEBUG(CAT_ACTUATOR, "OFF");
  }
}

bool ActuatorBase::getState() const {
  return _state;
}

void ActuatorBase::update(int delaySeconds, uint32_t maxOnTime) {
  checkMaxOnTime(maxOnTime);

  bool timerExpired = delayTimer(false, delaySeconds);
  if (timerExpired && !_state) {
    set(true, true);
  }

  if (!_state && !_delayActive && delaySeconds > 0) {
    delayTimer(true, delaySeconds);
  }
}

void ActuatorBase::forceStop() {
  XLOG_WARN(CAT_ACTUATOR, "Force stop!");
  set(false, true);
  _emergencyStop = true;
  if (onForceStopCallback) {
    onForceStopCallback(callbackContext);
  }
}

void ActuatorBase::checkMaxOnTime(uint32_t maxOnTime) {
  if (!_state)
    return;
  if (maxOnTime == 0)
    return;
  if (_startTime == 0)
    return;

  if ((millis() - _startTime) > maxOnTime * 1000UL) {
    forceStop();
  }
}

bool ActuatorBase::delayTimer(bool start, int delaySeconds) {
  if (start) {
    if (delaySeconds > 0 && !_delayActive && !_state) {
      _delayActive = true;
      _delayTimer = millis() + delaySeconds * 1000UL;
      XLOG_INFO(CAT_ACTUATOR, "Delay started: %d sec", delaySeconds);
    }
    return false;
  } else {
    if (_delayActive && millis() >= _delayTimer) {
      _delayActive = false;
      XLOG_DEBUG(CAT_ACTUATOR, "Delay expired");
      return true;
    }
    return false;
  }
}