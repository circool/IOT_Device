#ifndef FAN_H
#define FAN_H

#include <Arduino.h>
#include "config.h"

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3

extern bool fanOn;
extern unsigned long fanStartTime;
extern unsigned long delayTimer;
extern bool delayActive;

// Переменные стартового импульса
extern bool startingPulseActive;
extern unsigned long startingPulseStart;

void fan_init();
void fan_update();
void fan_set(bool on);
bool fan_getState();
bool fan_getRealState();
void fan_setOverrideMode(bool automatic);
void fan_checkMaxOnTime();
bool fan_delayTimer(bool start);

#endif // DEVICE_TYPE == 1 || DEVICE_TYPE == 3

#endif