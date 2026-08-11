/**
 * @file device_controller_switch_actuator.cpp
 * @brief Реализация управления выключателем (только ON/OFF)
 */

#include "device_controller_switch_actuator.h"
#include "logger.h"
#include "settings.h"

#if DEVICE_TYPE == 3

// ============================================================
// КОНСТРУКТОР
// ============================================================

SwitchActuator::SwitchActuator()
    : _pin(0)
    , _relayOnLevel(LOW)
    , _delaySeconds(0)
    , _maxOnTime(0) {
    
    _base.onSetPhysicalCallback = SwitchActuator::onSetPhysicalCallback;
    _base.onForceStopCallback = SwitchActuator::onForceStopCallback;
    _base.onManualCommandCallback = nullptr;  // SwitchActuator не использует
    _base.callbackContext = this;
}

// ============================================================
// ПУБЛИЧНЫЕ МЕТОДЫ
// ============================================================

void SwitchActuator::init(uint8_t pin, uint8_t relayOnLevel, bool bootState,
                          int delaySeconds, uint32_t maxOnTime) {
    _pin = pin;
    _relayOnLevel = relayOnLevel;
    _delaySeconds = delaySeconds;
    _maxOnTime = maxOnTime;

    _base.init(pin, relayOnLevel, bootState, delaySeconds, maxOnTime);
    
    XLOG_INFO(CAT_SWITCH, "Init: pin=%d, state=%s, delay=%d, maxOn=%lu",
              pin, bootState ? "ON" : "OFF", delaySeconds, maxOnTime);
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
    XLOG_DEBUG(CAT_SWITCH, "Config updated: delay=%d, maxOn=%lu",
               delaySeconds, maxOnTime);
}

// ============================================================
// СТАТИЧЕСКИЕ КОЛБЭКИ
// ============================================================

void SwitchActuator::onSetPhysicalCallback(void* context, bool on) {
    SwitchActuator* self = (SwitchActuator*)context;
    if (!self) return;
    
    digitalWrite(self->_pin, on ? self->_relayOnLevel : !self->_relayOnLevel);
    XLOG_DEBUG(CAT_SWITCH, "Physical set: %s", on ? "ON" : "OFF");
}

void SwitchActuator::onForceStopCallback(void* context) {
    (void)context;
    XLOG_WARN(CAT_SWITCH, "Force stop triggered (maxOnTime exceeded)");
}

#endif // DEVICE_TYPE == 3