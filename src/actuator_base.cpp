#include "actuator_base.h"
#include "logger.h"

ActuatorBase::ActuatorBase()
    : _pin(0),
      _state(false),
      _startTime(0),
      _maxOnTime(0),
      _emergencyStop(false) {}

void ActuatorBase::init(uint8_t pin,
                        bool bootState,
                        uint32_t maxOnTime) {
  _pin = pin;
  _maxOnTime = maxOnTime;
  _emergencyStop = false;

  pinMode(_pin, OUTPUT);

  if (bootState) {
    _state = true;
    _startTime = millis();
    onSetPhysical(true);
    LOG_INFO(CAT_ACTUATOR, "Init: pin=%d, state=ON, maxOnTime=%lu sec", _pin,
             _maxOnTime);
  } else {
    _state = false;
    _startTime = 0;
    onSetPhysical(false);
    LOG_INFO(CAT_ACTUATOR, "Init: pin=%d, state=OFF, maxOnTime=%lu sec", _pin,
             _maxOnTime);
  }
}

void ActuatorBase::set(bool on, bool manual) {
  if (manual && _emergencyStop) {
    LOG_INFO(CAT_ACTUATOR, "Manual command resets emergency stop");
    _emergencyStop = false;
  }

  if (_state == on)
    return;

  _state = on;
  onSetPhysical(_state);

  if (_state) {
    _startTime = millis();
    LOG_INFO(CAT_ACTUATOR, "ON");
  } else {
    _startTime = 0;
    LOG_INFO(CAT_ACTUATOR, "OFF");
  }
}

bool ActuatorBase::getState() const {
  return _state;
}

void ActuatorBase::update() {
  checkMaxOnTime();
}

void ActuatorBase::checkMaxOnTime() {
  if (!_state)
    return;
  if (_maxOnTime == 0)
    return;
  if (_emergencyStop)
    return;
  if (_startTime == 0)
    return;

  if ((millis() - _startTime) > _maxOnTime * 1000UL) {
    LOG_WARN(CAT_ACTUATOR, "MaxOnTime exceeded (%lu sec)! Emergency stop.",
             _maxOnTime);
    _emergencyStop = true;
    set(false, false);
  }
}