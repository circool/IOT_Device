#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include "config.h"          // <- ДОБАВИТЬ ЭТУ СТРОКУ! SENSOR_TYPE будет определён

#if SENSOR_TYPE == 1
  #include <Adafruit_AHTX0.h>
#elif SENSOR_TYPE == 2
  #include <DHT.h>
#endif

// Глобальные переменные для доступа из других модулей
extern float currentTemp;
extern float currentHum;
extern bool sensorOk;
extern unsigned long lastSensorRead;

// Функции датчика
void sensor_init();
void sensor_read();
bool sensor_isOk();
float sensor_getTemperature();
float sensor_getHumidity();

#endif