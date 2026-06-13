#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include "config.h"

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2

/**
 * @brief Модуль работы с датчиком температуры и влажности
 *
 * Поддерживает:
 * - AHT10 (I2C, SENSOR_TYPE=1)
 * - DHT11/DHT22 (1-Wire, SENSOR_TYPE=2)
 *
 * Все функции потокобезопасны (нет прерываний, только опрос в loop)
 */

#if SENSOR_TYPE == 1
#include <Adafruit_AHTX0.h>
#elif SENSOR_TYPE == 2
#include <DHT.h>
#endif

/**
 * @brief Инициализация датчика
 * Вызывается один раз в setup()
 */
void sensor_init();

/**
 * @brief Обновление показаний датчика
 * Вызывается в loop() с интервалом sensorInterval секунд
 * @return true — данные обновились (изменились), false — нет изменений или
 * ошибка
 */
bool sensor_update();

/**
 * @brief Получить текущую температуру
 * @return Температура в градусах Цельсия (при ошибке — 0)
 */
float sensor_getTemperature();

/**
 * @brief Получить текущую влажность
 * @return Влажность в процентах (при ошибке — 0)
 */
float sensor_getHumidity();

/**
 * @brief Проверить работоспособность датчика
 * @return true — датчик отвечает, данные валидны
 */
bool sensor_isOk();

/**
 * @brief Получить текст последней ошибки
 * @return Строка с описанием ошибки (пустая строка, если ошибки нет)
 */
const char* sensor_getError();

/**
 * @brief Получить скорость изменения влажности
 * @return %/сек (положительное — рост, отрицательное — падение)
 */
float sensor_getHumRate();

#endif  // DEVICE_TYPE == 1 || DEVICE_TYPE == 2

#endif  // SENSOR_H