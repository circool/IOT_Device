#ifndef FAN_ACTUATOR_H
#define FAN_ACTUATOR_H

#include "actuator_base.h"
#include "sensor.h"

class FanActuator {
public:
    FanActuator();
    
    void init(uint8_t pin, uint8_t relayOnLevel, bool bootState, uint16_t defaultSpeed);
    void update();
    
    void set(bool on, bool manual = true);
    bool getState() const;
    
    void setSpeed(int percent, bool manual = true);
    int getSpeed() const;
    
    void setAdaptiveMode(bool enabled);
    bool getAdaptiveMode() const;
    
    // Статические回调-функции
    static void onSetPhysicalCallback(void* context, bool on);
    static void onForceStopCallback(void* context);
    
private:
    void applySpeed(int percent);
    void enablePWM();
    void disablePWM();
    void adaptiveUpdate();
    int calculateAdaptiveStep(float deltaTemp, float deltaHum, float humRate);
    
    ActuatorBase _base;
    
    uint8_t _pin;
    uint8_t _relayOnLevel;
    int _currentSpeed;
    bool _adaptiveMode;
    bool _pwmActive;
    
    bool _adaptiveActive;
    float _baseTemp;
    float _baseHum;
    unsigned long _lastAdaptiveCheck;
    
    bool _startingPulseActive;
    unsigned long _startingPulseStart;
};

#endif