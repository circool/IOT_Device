#ifndef FAN_H
#define FAN_H

#include <Arduino.h>

extern bool manualOverride;
extern bool fanShouldBeOn;
extern unsigned long fanOnStartTime;
extern unsigned long delayOnTimer;
extern bool delayOnActive;
extern unsigned long lastRelayCheck;
extern bool lastRelayError;

void fan_init();
void fan_update();
void fan_set(bool on);
void fan_toggle();
bool fan_getState();
bool fan_getRealState();
bool fan_getRealStateForMqtt();
void fan_applyPwmOrDigital(bool on);
void fan_checkMaxOnTime();
void fan_resetDelayTimer();
bool fan_cancelDelayTimer();
void fan_checkRelayConsistency();
void fan_checkDelayTimer();

#endif