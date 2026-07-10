#include "actuator_base.h"
#include "logger.h"

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
ActuatorBase::ActuatorBase()
    : onSetPhysicalCallback(nullptr),
      onForceStopCallback(nullptr),
      onManualCommandCallback(nullptr),
      callbackContext(nullptr),
      _pin(0),
      _relayOnLevel(LOW),
      _state(false),
      _startTime(0),
      _delayActive(false),
      _delayTimer(0),
      _emergencyStop(false) {}

void ActuatorBase::init(uint8_t pin, uint8_t relayOnLevel, bool bootState) {
  _pin = pin;
  _relayOnLevel = relayOnLevel;

  pinMode(_pin, OUTPUT);
  _delayActive = false;
  _delayTimer = 0;
  _startTime = 0;
  _emergencyStop = false;

  if (bootState) {
    if (onSetPhysicalCallback)
      onSetPhysicalCallback(callbackContext, true);
    _state = true;
    _startTime = millis();
  } else {
    if (onSetPhysicalCallback)
      onSetPhysicalCallback(callbackContext, false);
    _state = false;
    _startTime = 0;
  }

  XLOG_INFO(CAT_ACTUATOR, "Init: pin=%d, state=%s, bootState=%s", _pin,
           _state ? "ON" : "OFF", bootState ? "ON" : "OFF");
}

void ActuatorBase::set(bool on, bool manual) {
  // Обработка ручного режима выполняется всегда, даже если состояние не
  // меняется
  if (manual) {
    if (onManualCommandCallback) {
      onManualCommandCallback(callbackContext);
    }

    if (_delayActive) {
      _delayActive = false;
      XLOG_INFO(CAT_ACTUATOR, "Manual - delay cancelled");
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
  }

  if (_state) {
    _startTime = millis();
    XLOG_INFO(CAT_ACTUATOR, "ON");
  } else {
    _startTime = 0;
    XLOG_INFO(CAT_ACTUATOR, "OFF");
  }
}

bool ActuatorBase::getState() const {
  return _state;
}

void ActuatorBase::update() {
  checkMaxOnTime();

  bool timerExpired = delayTimer(false);
  if (timerExpired && !_state) {
    set(true, true);
  }

  if (!_state && !_delayActive && g_configManager.getDelaySeconds() > 0) {
    delayTimer(true);
  }
}

void ActuatorBase::forceStop() {
  XLOG_INFO(CAT_ACTUATOR, "Force stop!");
  set(false, true);
  _emergencyStop = true;
  if (onForceStopCallback)
    onForceStopCallback(callbackContext);
}

void ActuatorBase::checkMaxOnTime() {
  if (!_state)
    return;
  if (g_configManager.getMaxOnTime() == 0)
    return;
  if (_startTime == 0)
    return;

  if ((millis() - _startTime) > g_configManager.getMaxOnTime() * 1000UL) {
    forceStop();
  }
}

bool ActuatorBase::delayTimer(bool start) {
  if (start) {
    uint16_t delaySec = g_configManager.getDelaySeconds();
    if (delaySec > 0 && !_delayActive && !_state) {
      _delayActive = true;
      _delayTimer = millis() + delaySec * 1000UL;
      XLOG_INFO(CAT_ACTUATOR, "Delay start: %d sec", delaySec);
    }
    return false;
  } else {
    if (_delayActive && millis() >= _delayTimer) {
      _delayActive = false;
      XLOG_INFO(CAT_ACTUATOR, "Delay expired");
      return true;
    }
    return false;
  }
}
#endif