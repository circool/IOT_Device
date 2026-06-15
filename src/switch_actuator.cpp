#include "switch_actuator.h"
#include "config.h"
#include "logger.h"

SwitchActuator::SwitchActuator() : ActuatorBase() {}

void SwitchActuator::init(uint8_t pin,
                          bool bootState,
                          uint32_t maxOnTime) {
  ActuatorBase::init(pin, bootState, maxOnTime);
  LOG_INFO(CAT_SWITCH, "Init: maxOnTime=%lu sec, active level=%s",
                    maxOnTime, ACTIVE_LEVEL == LOW ? "LOW" : "HIGH");
}

void SwitchActuator::onSetPhysical(bool on) {
  digitalWrite(_pin, on ? ACTIVE_LEVEL : !ACTIVE_LEVEL);
}