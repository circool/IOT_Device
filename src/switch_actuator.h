#ifndef SWITCH_ACTUATOR_H
#define SWITCH_ACTUATOR_H

#include "actuator_base.h"

class SwitchActuator {
public:
    SwitchActuator();
    
    void init(uint8_t pin, uint8_t relayOnLevel, bool bootState);
    void update();
    void set(bool on, bool manual = true);
    bool getState() const;
    
    static void onSetPhysicalCallback(void* context, bool on);
    static void onForceStopCallback(void* context);
    
private:
    ActuatorBase _base;
    uint8_t _pin;
    uint8_t _relayOnLevel;
};

#endif