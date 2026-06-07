#include "actuator_base.h"
#include "ansi.h"

ActuatorBase::ActuatorBase()
    : onSetPhysicalCallback(nullptr)
    , onForceStopCallback(nullptr)
    , onManualCommandCallback(nullptr)  
    , callbackContext(nullptr)
    , _pin(0)
    , _relayOnLevel(LOW)
    , _state(false)
    , _startTime(0)
    , _delayActive(false)
    , _delayTimer(0) {
}

void ActuatorBase::init(uint8_t pin, uint8_t relayOnLevel, bool bootState) {
    _pin = pin;
    _relayOnLevel = relayOnLevel;
    
    pinMode(_pin, OUTPUT);
    _delayActive = false;
    _delayTimer = 0;
    _startTime = 0;
    
    if (bootState) {
        if (onSetPhysicalCallback) onSetPhysicalCallback(callbackContext, true);
        _state = true;
        _startTime = millis();
    } else {
        if (onSetPhysicalCallback) onSetPhysicalCallback(callbackContext, false);
        _state = false;
        _startTime = 0;
    }
    
    #if LOG_ACTUATOR == 1
        Serial.printf("[ACTUATOR] Init: pin=%d, state=%s, bootState=%s\n", 
                      _pin, _state ? "ON" : "OFF", bootState ? "ON" : "OFF");
    #endif
}

void ActuatorBase::set(bool on, bool manual) {
    // Обработка ручного режима выполняется всегда, даже если состояние не меняется
    if (manual) {
        if (onManualCommandCallback) {
            onManualCommandCallback(callbackContext);
        }
        
        if (_delayActive) {
            _delayActive = false;
            #if LOG_ACTUATOR == 1
                Serial.println("[ACTUATOR] Manual - delay cancelled");
            #endif
        }
    }
       
    if (_state == on) return;

    _state = on;
    
    if (onSetPhysicalCallback) {
        onSetPhysicalCallback(callbackContext, _state);
    }
    
    if (_state) {
        _startTime = millis();
        #if LOG_ACTUATOR == 1
            Serial.println("[ACTUATOR] ON");
        #endif
    } else {
        _startTime = 0;
        #if LOG_ACTUATOR == 1
            Serial.println("[ACTUATOR] OFF");
        #endif
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
    
    if (!_state && !_delayActive && config_get()->delaySeconds > 0) {
        delayTimer(true);
    }
}

void ActuatorBase::forceStop() {
    #if LOG_ACTUATOR == 1
        Serial.println("[ACTUATOR] Force stop!");
    #endif
    set(false, true);
    if (onForceStopCallback) onForceStopCallback(callbackContext);
}

void ActuatorBase::checkMaxOnTime() {
    if (!_state) return;
    if (config_get()->maxOnTime == 0) return;
    if (_startTime == 0) return;
    
    if ((millis() - _startTime) > config_get()->maxOnTime * 1000UL) {
        forceStop();
    }
}

bool ActuatorBase::delayTimer(bool start) {
    if (start) {
        uint16_t delaySec = config_get()->delaySeconds;
        if (delaySec > 0 && !_delayActive && !_state) {
            _delayActive = true;
            _delayTimer = millis() + delaySec * 1000UL;
            #if LOG_ACTUATOR == 1
                Serial.printf("[ACTUATOR] Delay start: %d sec\n", delaySec);
            #endif
        }
        return false;
    } else {
        if (_delayActive && millis() >= _delayTimer) {
            _delayActive = false;
            #if LOG_ACTUATOR == 1
                Serial.println("[ACTUATOR] Delay expired");
            #endif
            return true;
        }
        return false;
    }
}