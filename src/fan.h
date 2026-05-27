#if DEVICE_TYPE == 1

#ifndef FAN_H
#define FAN_H

#include <Arduino.h>
#include "config.h"



extern bool fanOn;
extern unsigned long fanStartTime;
extern unsigned long delayTimer;
extern bool delayActive;

// Переменные стартового импульса
extern bool startingPulseActive;
extern unsigned long startingPulseStart;

// Переменные адаптивного режима
extern bool adaptiveActive;
extern float baseTemp;
extern float baseHum;
extern unsigned long lastAdaptiveCheck;

void fan_init();
void fan_update();
void fan_set(bool on, bool manual = true);  // ← добавлен параметр manual
bool fan_getState();
void fan_setOverrideMode(bool sensorControl);
void fan_checkMaxOnTime();
bool fan_delayTimer(bool start);
void fan_applySpeed(int percent);
void fan_adaptiveUpdate();



#endif // FAN_H
#endif