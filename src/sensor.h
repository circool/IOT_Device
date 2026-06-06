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

// ========== НОВЫЙ API (ГЕТТЕРЫ) ==========

/**
 * Инициализация датчика. Вызывается один раз при старте.
 */
void sensor_init();

/**
 * Обновление показаний датчика. Вызывается в loop().
 * @return true если данные были обновлены (изменились), false если нет.
 */
bool sensor_update();

/**
 * @return текущая температура в градусах Цельсия
 */
float sensor_getTemperature();

/**
 * @return текущая влажность в процентах
 */
float sensor_getHumidity();

/**
 * @return true если датчик работает корректно
 */
bool sensor_isOk();

/**
 * @return текст последней ошибки (или пустую строку)
 */
const char* sensor_getError();

/**
 * @return скорость изменения влажности (%/сек)
 */
float sensor_getHumRate();

#endif // DEVICE_TYPE == 1 || DEVICE_TYPE == 2

#endif // SENSOR_H