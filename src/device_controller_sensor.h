/**
 * @file device_controller_sensor.h
 * @brief Датчик температуры и влажности (внутренняя подсистема DeviceController)
 */

#ifndef DEVICE_CONTROLLER_SENSOR_H
#define DEVICE_CONTROLLER_SENSOR_H

#include <stdbool.h>
#include <stdint.h>

/** @brief Пин I2C SDA (для AHT10) */
#ifndef I2C_SDA_PIN
#ifdef ESP8266
#define I2C_SDA_PIN 4
#elif defined(ESP32)
#define I2C_SDA_PIN 21
#endif
#endif

/** @brief Пин для DHT датчика (только для SENSOR_TYPE=2) */
#ifndef SENSOR_PIN
#ifdef ESP8266
#define SENSOR_PIN 4
#elif defined(ESP32)
#define SENSOR_PIN 16
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

// ===== СТРУКТУРА ДАТЧИКА =====
typedef struct {
    // ===== ПУБЛИЧНЫЕ ПОЛЯ (для чтения) =====
    bool valid;
    float temperature;
    float humidity;
    char error[32];
    
    // ===== ВНУТРЕННИЕ ПОЛЯ (не трогать) =====
    unsigned long lastReadTime;
    uint8_t readErrors;
    bool initialized;
    uint8_t type;            // 1 = AHT10, 2 = DHT
    int pin;                 // GPIO для DHT или -1 для I2C
} Sensor;

// ===== ПУБЛИЧНЫЙ ИНТЕРФЕЙС =====

/**
 * @brief Инициализация датчика
 * @param sensor Указатель на Sensor
 * @param type Тип датчика (1 = AHT10, 2 = DHT)
 * @param pin GPIO пин (для DHT) или -1 (для I2C)
 * @return true при успехе
 */
bool sensor_init(Sensor* sensor, uint8_t type, int pin);

/**
 * @brief Обновление показаний датчика
 * @param sensor Указатель на Sensor
 * @return true если данные обновились
 */
bool sensor_update(Sensor* sensor);

/**
 * @brief Проверка валидности данных
 */
static inline bool sensor_isOk(const Sensor* sensor) {
    return sensor->valid;
}

/**
 * @brief Получить температуру
 */
static inline float sensor_getTemperature(const Sensor* sensor) {
    return sensor->temperature;
}

/**
 * @brief Получить влажность
 */
static inline float sensor_getHumidity(const Sensor* sensor) {
    return sensor->humidity;
}

/**
 * @brief Получить текст ошибки
 */
static inline const char* sensor_getError(const Sensor* sensor) {
    return sensor->error;
}

#endif // DEVICE_CONTROLLER_SENSOR_H