/**
 * @file device_controller_sensor.cpp
 * @brief Реализация датчика температуры и влажности
 */

#include "device_controller_sensor.h"
#include "logger.h"
#include "settings.h"
#include <Arduino.h>

// ============================================================================
// НАСТРОЙКИ ДАТЧИКА (для TYPE 1 и 2)
// ============================================================================

/** Допустимый диапазон времени опроса датчиков */
#ifndef SENSOR_INTERVAL_MIN
#define SENSOR_INTERVAL_MIN 1
#endif
#ifndef SENSOR_INTERVAL_MAX
#define SENSOR_INTERVAL_MAX 50
#endif

/**
 * @brief Тип датчика температуры/влажности
 * @values 1 – AHT10 (I2C)
 *         2 – DHT11/DHT22 (GPIO)
 */
#ifndef SENSOR_TYPE
#define SENSOR_TYPE 1
#endif

#if SENSOR_TYPE == 2

#endif

// ===== ВЫБОР ТИПА ДАТЧИКА =====
#if defined(USE_SENSOR)

#if SENSOR_TYPE == 1
    #include <Adafruit_AHTX0.h>
#elif SENSOR_TYPE == 2
    #include <DHT.h>
#endif

// ===== КОНСТАНТЫ =====
#ifndef SENSOR_INTERVAL_MIN
#define SENSOR_INTERVAL_MIN 1000  // мс
#endif

#ifndef SENSOR_INTERVAL_MAX
#define SENSOR_INTERVAL_MAX 5000  // мс
#endif

#ifndef SENSOR_ERROR_THRESHOLD
#define SENSOR_ERROR_THRESHOLD 3   // Количество ошибок до invalid
#endif



// ============================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================

/**
 * @brief Получить интервал опроса в зависимости от типа датчика
 */
static unsigned long getSensorInterval(uint8_t type) {
    switch (type) {
        case 1:  // AHT10
            return 100;  // 100 мс
        case 2:  // DHT11
            return 1000; // 1 сек
        default:
            return SENSOR_INTERVAL_MIN;
    }
}

/**
 * @brief Проверка валидности показаний
 */
static bool isValidReading(float temperature, float humidity) {
    // Проверка на NaN
    if (isnan(temperature) || isnan(humidity)) {
        return false;
    }
    
    // Проверка диапазона влажности
    if (humidity < 0.0f || humidity > 100.0f) {
        return false;
    }
    
    // Диапазон температуры не проверяем — бизнес-логика сама решит
    return true;
}

// ============================================================
// РЕАЛИЗАЦИЯ ИНТЕРФЕЙСА
// ============================================================

bool sensor_init(Sensor* sensor, uint8_t type, int pin) {
    if (!sensor) {
        return false;
    }
    
    sensor->type = type;
    sensor->pin = pin;
    sensor->valid = false;
    sensor->temperature = 0.0f;
    sensor->humidity = 0.0f;
    sensor->error[0] = '\0';
    sensor->lastReadTime = 0;
    sensor->readErrors = 0;
    sensor->initialized = false;

#if SENSOR_TYPE == 1
    // AHT10 — I2C
    (void)pin;  // pin не используется для I2C
    
    // Инициализация I2C
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    
    // Создаём экземпляр AHT10 (статический, т.к. только один датчик)
    static Adafruit_AHTX0 aht;
    
    if (!aht.begin()) {
        snprintf(sensor->error, sizeof(sensor->error), "AHT10 not found");
        XLOG_ERROR(CAT_SENSOR, "AHT10 init failed");
        return false;
    }
    
    sensor->initialized = true;
    XLOG_INFO(CAT_SENSOR, "AHT10 initialized");
    return true;
    
#elif SENSOR_TYPE == 2
    // DHT11 / DHT22
    static DHT dht(pin, DHT_TYPE);
    dht.begin();
    sensor->initialized = true;
    XLOG_INFO(CAT_SENSOR, "DHT initialized: pin=%d, type=%d", pin, DHT_TYPE);
    return true;
    
#else
    snprintf(sensor->error, sizeof(sensor->error), "Unsupported sensor type: %d", type);
    XLOG_ERROR(CAT_SENSOR, "Unsupported sensor type: %d", type);
    return false;
#endif
}

bool sensor_update(Sensor* sensor) {
    if (!sensor || !sensor->initialized) {
        return false;
    }
    
    // Проверка интервала опроса
    unsigned long now = millis();
    unsigned long interval = getSensorInterval(sensor->type);
    
    if (now - sensor->lastReadTime < interval) {
        return false;  // Слишком рано
    }
    sensor->lastReadTime = now;
    
    float temp = 0.0f;
    float hum = 0.0f;
    bool success = false;
    
#if SENSOR_TYPE == 1
    // ===== AHT10 =====
    static Adafruit_AHTX0 aht;  // Статический экземпляр
    
    sensors_event_t humidityEvent, tempEvent;
    aht.getEvent(&humidityEvent, &tempEvent);
    
    temp = tempEvent.temperature;
    hum = humidityEvent.relative_humidity;
    
    // AHT10: если чтение вернуло 0, значит ошибка
    if (temp == 0.0f && hum == 0.0f) {
        success = false;
    } else {
        success = true;
    }
    
#elif SENSOR_TYPE == 2
    // ===== DHT =====
    static DHT dht(sensor->pin, DHT_TYPE);
    
    temp = dht.readTemperature();
    hum = dht.readHumidity();
    
    // DHT: если чтение вернуло NaN, значит ошибка
    if (isnan(temp) || isnan(hum)) {
        success = false;
    } else {
        success = true;
    }
    
#else
    (void)temp;
    (void)hum;
    success = false;
#endif
    
    // ===== ОБРАБОТКА РЕЗУЛЬТАТА =====
    if (success && isValidReading(temp, hum)) {
        sensor->temperature = temp;
        sensor->humidity = hum;
        sensor->valid = true;
        sensor->readErrors = 0;
        sensor->error[0] = '\0';
        return true;
    }
    
    // ===== ОБРАБОТКА ОШИБКИ =====
    sensor->readErrors++;
    
    if (sensor->readErrors >= SENSOR_ERROR_THRESHOLD) {
        sensor->valid = false;
        snprintf(sensor->error, sizeof(sensor->error), 
                 "Sensor error: %d consecutive failures", 
                 sensor->readErrors);
        XLOG_WARN(CAT_SENSOR, "Sensor invalid: %d errors", sensor->readErrors);
    } else {
        XLOG_DEBUG(CAT_SENSOR, "Sensor read error: %d/%d", 
                   sensor->readErrors, SENSOR_ERROR_THRESHOLD);
    }
    
    return false;
}

#else  // USE_SENSOR не определён

// ============================================================
// ЗАГЛУШКА — ДАТЧИК ОТКЛЮЧЁН
// ============================================================

bool sensor_init(Sensor* sensor, uint8_t type, int pin) {
    (void)sensor;
    (void)type;
    (void)pin;
    return false;
}

bool sensor_update(Sensor* sensor) {
    (void)sensor;
    return false;
}

#endif // USE_SENSOR