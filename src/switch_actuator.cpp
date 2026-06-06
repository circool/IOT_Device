#include "switch_actuator.h"
#include "ansi.h"

SwitchActuator::SwitchActuator()
    : _pin(0), _relayOnLevel(LOW) {
    _base.onSetPhysicalCallback = SwitchActuator::onSetPhysicalCallback;
    _base.onForceStopCallback = SwitchActuator::onForceStopCallback;
    _base.callbackContext = this;
}

void SwitchActuator::init(uint8_t pin, uint8_t relayOnLevel, bool bootState) {
    _pin = pin;
    _relayOnLevel = relayOnLevel;
    _base.init(pin, relayOnLevel, bootState);
}

void SwitchActuator::update() {
    _base.update();
}

void SwitchActuator::set(bool on, bool manual) {
    _base.set(on, manual);
}

bool SwitchActuator::getState() const {
    return _base.getState();
}

void SwitchActuator::onSetPhysicalCallback(void* context, bool on) {
    SwitchActuator* self = (SwitchActuator*)context;
    if (!self) return;
    digitalWrite(self->_pin, on ? self->_relayOnLevel : !self->_relayOnLevel);
}

void SwitchActuator::onForceStopCallback(void* context) {
    #if LOG_SWITCH == 1
        Serial.println("[SWITCH] Force stop due to maxOnTime");
    #endif
}