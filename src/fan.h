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

// Переменные адаптивного режима
extern bool adaptiveActive;           // Активна ли адаптация в текущей сессии
extern float baseTemp;                // Базовая температура при включении
extern float baseHum;                 // Базовая влажность при включении
extern unsigned long lastAdaptiveCheck;

void fan_init();
void fan_update();
void fan_set(bool on);
bool fan_getState();
bool fan_getRealState();
void fan_setOverrideMode(bool sensorControl);  // Режим управления датчиком
void fan_checkMaxOnTime();
bool fan_delayTimer(bool start);
void fan_applyPWM(int percent);       // Применение ШИМ в процентах (0-100)
int fan_getCurrentPWMDuty();          // Получить текущую скважность в % (учёт стартового импульса)
void fan_adaptiveUpdate();            // Адаптивный режим

#endif // DEVICE_TYPE == 1 || DEVICE_TYPE == 3

#endif