#ifndef ACTUATOR_BASE_H
#define ACTUATOR_BASE_H

#include <Arduino.h>
#include "config.h"

class ActuatorBase {
public:
    ActuatorBase();
    
    void init(uint8_t pin, uint8_t relayOnLevel, bool bootState);
    void set(bool on, bool manual = true);
    bool getState() const;
    void update();
    void forceStop();
    
    unsigned long getStartTime() const { return _startTime; }
    bool isDelayActive() const { return _delayActive; }
    unsigned long getDelayTimer() const { return _delayTimer; }
    
    // Методы для переопределения (не virtual, через указатели на функции)
    void (*onSetPhysicalCallback)(void*, bool);
    void (*onForceStopCallback)(void*);
    void* callbackContext;

protected:
    void checkMaxOnTime();
    bool delayTimer(bool start);
    
    uint8_t _pin;
    uint8_t _relayOnLevel;
    bool _state;
    unsigned long _startTime;
    bool _delayActive;
    unsigned long _delayTimer;
};

#endif