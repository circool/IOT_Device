#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include "config.h"

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2

#if SENSOR_TYPE == 1
  #include <Adafruit_AHTX0.h>
#elif SENSOR_TYPE == 2
  #include <DHT.h>
#endif

extern float currentTemp;
extern float currentHum;
extern bool sensorOk;
extern unsigned long lastSensorRead;
extern char sensorError[64];            
extern float humRate;                   // скорость изменения влажности (%/сек)

void sensor_init();
void sensor_read();
bool sensor_isOk();
float sensor_getTemperature();
float sensor_getHumidity();

#endif // DEVICE_TYPE == 1 || DEVICE_TYPE == 2

#endif