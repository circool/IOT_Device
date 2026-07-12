#include "switch_actuator.h"
#include "logger.h"

SwitchActuator::SwitchActuator()
    : _pin(0), _relayOnLevel(LOW), _delaySeconds(0), _maxOnTime(0) {
  _base.onSetPhysicalCallback = SwitchActuator::onSetPhysicalCallback;
  _base.onForceStopCallback = SwitchActuator::onForceStopCallback;
  _base.onManualCommandCallback = nullptr;
  _base.callbackContext = this;
}

void SwitchActuator::init(uint8_t pin,
                          uint8_t relayOnLevel,
                          bool bootState,
                          int delaySeconds,
                          uint32_t maxOnTime) {
  _pin = pin;
  _relayOnLevel = relayOnLevel;
  _delaySeconds = delaySeconds;
  _maxOnTime = maxOnTime;
  _base.init(pin, relayOnLevel, bootState, delaySeconds, maxOnTime);
}

void SwitchActuator::update(int delaySeconds, uint32_t maxOnTime) {
  _delaySeconds = delaySeconds;
  _maxOnTime = maxOnTime;
  _base.update(delaySeconds, maxOnTime);
}

void SwitchActuator::set(bool on, bool manual) {
  _base.set(on, manual);
}

bool SwitchActuator::getState() const {
  return _base.getState();
}

void SwitchActuator::updateConfig(int delaySeconds, uint32_t maxOnTime) {
  _delaySeconds = delaySeconds;
  _maxOnTime = maxOnTime;
  XLOG_DEBUG(CAT_SWITCH, "Config updated: delay=%d, maxOnTime=%lu",
             delaySeconds, maxOnTime);
}

void SwitchActuator::onSetPhysicalCallback(void* context, bool on) {
  SwitchActuator* self = (SwitchActuator*)context;
  if (!self)
    return;
  digitalWrite(self->_pin, on ? self->_relayOnLevel : !self->_relayOnLevel);
}

void SwitchActuator::onForceStopCallback(void* context) {
  XLOG_INFO(CAT_ACTUATOR, "Force stop due to maxOnTime");
}