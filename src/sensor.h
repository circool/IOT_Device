#ifndef SENSOR_H
#define SENSOR_H

#include <Arduino.h>
#include "settings.h"

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2

/**
 * @brief Модуль работы с датчиком температуры и влажности
 *
 * Поддерживает:
 * - AHT10 (I2C, SENSOR_TYPE=1)
 * - DHT11/DHT22 (1-Wire, SENSOR_TYPE=2)
 *
 * Все функции потокобезопасны (нет прерываний, только опрос в loop)
 * Датчик сам управляет минимальным интервалом опроса
 */

// ============================================================================
// КОНСТАНТЫ ИНТЕРВАЛОВ ОПРОСА (могут быть переопределены в platformio.ini)
// ============================================================================

/**
 * @brief Минимальный интервал опроса для AHT10 (по даташиту)
 * @note Значение по умолчанию 100 мс. Увеличение снижает нагрузку на I2C.
 */
#ifndef AHT_MIN_INTERVAL_MS
#define AHT_MIN_INTERVAL_MS 100
#endif

/**
 * @brief Минимальный интервал опроса для DHT11 (по даташиту)
 */
#ifndef DHT11_MIN_INTERVAL_MS
#define DHT11_MIN_INTERVAL_MS 1000
#endif

/**
 * @brief Минимальный интервал опроса для DHT22 (по даташиту)
 */
#ifndef DHT22_MIN_INTERVAL_MS
#define DHT22_MIN_INTERVAL_MS 2000
#endif

// ============================================================================
// ПИНЫ И ТИПЫ (могут быть переопределены в platformio.ini)
// ============================================================================

#if SENSOR_TYPE == 1

#include <Adafruit_AHTX0.h>
#include <Wire.h>

/** @brief Пин I2C SDA (для AHT10) */
#ifndef I2C_SDA_PIN
#ifdef ESP8266
#define I2C_SDA_PIN 4
#elif defined(ESP32)
#define I2C_SDA_PIN 21
#endif
#endif

/** @brief Пин I2C SCL (для AHT10) */
#ifndef I2C_SCL_PIN
#ifdef ESP8266
#define I2C_SCL_PIN 5
#elif defined(ESP32)
#define I2C_SCL_PIN 22
#endif
#endif

#elif SENSOR_TYPE == 2

#include <DHT.h>

/** @brief Пин для DHT датчика (только для SENSOR_TYPE=2) */
#ifndef SENSOR_PIN
#ifdef ESP8266
#define SENSOR_PIN 4
#elif defined(ESP32)
#define SENSOR_PIN 16
#endif
#endif

/** @brief Тип DHT датчика (DHT11 или DHT22) */
#ifndef DHT_TYPE
#define DHT_TYPE DHT22
#endif

#endif  // SENSOR_TYPE...

// ============================================================================
// ПУБЛИЧНЫЙ ИНТЕРФЕЙС
// ============================================================================

/**
 * @brief Инициализация датчика
 * Вызывается один раз в setup()
 */
void sensor_init();

/**
 * @brief Обновление показаний датчика
 * Вызывается в loop() без параметров.
 * Датчик сам определяет, когда можно читать (минимальный интервал опроса)
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