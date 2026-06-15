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

/**
 * @brief Инициализация датчика
 * @param cfg Указатель на активный конфиг (для получения sensorInterval и
 * других параметров)
 */
void sensor_init(const Config* cfg);

/**
 * @brief Обновление показаний датчика
 * @return true — данные обновились (изменились), false — нет изменений или
 * ошибка
 */
bool sensor_update();

/**
 * @brief Получить текущую температуру
 */
float sensor_getTemperature();

/**
 * @brief Получить текущую влажность
 */
float sensor_getHumidity();

/**
 * @brief Проверить работоспособность датчика
 */
bool sensor_isOk();

/**
 * @brief Получить текст последней ошибки
 */
const char* sensor_getError();

/**
 * @brief Получить скорость изменения влажности (%/сек)
 */
float sensor_getHumRate();

#endif  // DEVICE_TYPE == 1 || DEVICE_TYPE == 2

#endif  // SENSOR_H