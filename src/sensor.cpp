#include "sensor.h"
#include "logger.h"

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2

// ========== СТАТИЧЕСКИЕ ПЕРЕМЕННЫЕ (СКРЫТЫЕ) ==========
static float _currentTemp = 0;
static float _currentHum = 0;
static bool _sensorOk = false;
static unsigned long _lastSensorRead = 0;
static char _sensorError[64] = "";
static float _humRate = 0;

// Статические переменные для расчёта humRate
static unsigned long _lastHumTime = 0;
static float _lastHumValue = 0;

// Минимальный интервал опроса (зависит от типа датчика)
// Устанавливается в sensor_init() на основе констант из sensor.h
static unsigned long _minIntervalMs = 0;

#if SENSOR_TYPE == 1
static Adafruit_AHTX0 _aht;
#elif SENSOR_TYPE == 2
static DHT _dht(SENSOR_PIN, DHT_TYPE);
#endif

// ========== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ==========

/**
 * @brief Проверить, прошло ли достаточно времени для следующего опроса
 */
static bool isReadyToRead() {
  return (millis() - _lastSensorRead) >= _minIntervalMs;
}

/**
 * @brief Проверить, что показания не содержат явного мусора
 * @note Проверяем только NaN и влажность вне физического диапазона.
 *       Температуру не проверяем — пусть бизнес-логика решает, что с ней
 * делать.
 */
static bool isSensorDataValid(float temp, float hum) {
  // Ошибка библиотеки — не число
  if (isnan(temp) || isnan(hum)) {
    return false;
  }
  // Влажность физически не может быть < 0 или > 100%
  if (hum < 0.0f || hum > 100.0f) {
    return false;
  }
  return true;
}

// ========== ПУБЛИЧНЫЕ ФУНКЦИИ ==========

void sensor_init() {
#if SENSOR_TYPE == 1
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  _minIntervalMs = AHT_MIN_INTERVAL_MS;

  _sensorOk = _aht.begin();
  if (_sensorOk) {
    _sensorError[0] = '\0';
    XLOG_INFO(CAT_SENSOR, "AHT10 found (min interval: %lu ms)", _minIntervalMs);

    // Попытка первого чтения для получения валидных данных
    sensors_event_t humidity, temperature;
    if (_aht.getEvent(&humidity, &temperature)) {
      if (isSensorDataValid(temperature.temperature,
                            humidity.relative_humidity)) {
        _currentTemp = temperature.temperature;
        _currentHum = humidity.relative_humidity;
        _lastSensorRead = millis();
        XLOG_INFO(CAT_SENSOR, "First reading: T=%.2f°C, H=%.2f%%", _currentTemp,
                  _currentHum);
      } else {
        _sensorOk = false;
        strcpy(_sensorError,
               "First reading out of range (NaN or invalid humidity)");
        XLOG_WARN(CAT_SENSOR, "%s", _sensorError);
      }
    } else {
      // Датчик найден, но не отвечает на чтение
      _sensorOk = false;
      strcpy(_sensorError, "AHT10 not responding to read");
      XLOG_WARN(CAT_SENSOR, "%s", _sensorError);
    }
  } else {
    strcpy(_sensorError, "AHT10 not found");
    XLOG_ERROR(CAT_SENSOR, "AHT10 not found! Sensor will be disabled.");
  }

#elif SENSOR_TYPE == 2
// Выбор интервала в зависимости от типа DHT
#if DHT_TYPE == DHT11
  _minIntervalMs = DHT11_MIN_INTERVAL_MS;
#elif DHT_TYPE == DHT22
  _minIntervalMs = DHT22_MIN_INTERVAL_MS;
#else
  _minIntervalMs = DHT22_MIN_INTERVAL_MS;  // fallback
#endif

  _dht.begin();
  delay(1000);

  // DHT не имеет метода begin() с возвратом статуса.
  // Попытка первого чтения для проверки наличия датчика.
  float t = _dht.readTemperature();
  float h = _dht.readHumidity();
  if (!isnan(t) && !isnan(h) && h >= 0 && h <= 100) {
    _sensorOk = true;
    _currentTemp = t;
    _currentHum = h;
    _lastSensorRead = millis();
    _sensorError[0] = '\0';
    XLOG_INFO(CAT_SENSOR,
              "DHT found: T=%.2f°C, H=%.2f%% (min interval: %lu ms)",
              _currentTemp, _currentHum, _minIntervalMs);
  } else {
    _sensorOk = false;
    strcpy(_sensorError, "DHT not found or not responding");
    XLOG_ERROR(CAT_SENSOR, "DHT not found! Sensor will be disabled.");
  }
#endif

  _humRate = 0;
  _lastHumTime = 0;
  _lastHumValue = 0;
}

bool sensor_update() {
  // Если датчик не найден при инициализации — не пытаемся читать повторно
  if (!_sensorOk && strstr(_sensorError, "not found") != nullptr) {
    return false;
  }

  // Проверка минимального интервала опроса (паспортные характеристики)
  if (!isReadyToRead()) {
    return false;
  }
  _lastSensorRead = millis();

  float temp = 0, hum = 0;
  bool readSuccess = false;

#if SENSOR_TYPE == 1
  sensors_event_t humidity, temperature;
  if (_aht.getEvent(&humidity, &temperature)) {
    temp = temperature.temperature;
    hum = humidity.relative_humidity;
    readSuccess = true;
  } else {
    // Временная ошибка чтения — датчик может восстановиться
    _sensorOk = false;
    strcpy(_sensorError, "AHT10 read failed (I2C error)");
    XLOG_ERROR(CAT_SENSOR, "AHT10 read error!");
    return false;
  }
#elif SENSOR_TYPE == 2
  float t = _dht.readTemperature();
  float h = _dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    temp = t;
    hum = h;
    readSuccess = true;
  } else {
    // DHT вернул NAN — либо нарушен интервал опроса, либо датчик отвалился
    _sensorOk = false;
    strcpy(_sensorError, "DHT read failed (NaN)");
    XLOG_ERROR(CAT_SENSOR, "DHT read error!");
    return false;
  }
#endif

  if (!readSuccess) {
    _humRate = 0;
    return false;
  }

  // Валидация: только NaN и влажность вне физического диапазона
  if (!isSensorDataValid(temp, hum)) {
    // Сохраняем ошибку, но не обновляем кэш
    _sensorOk = false;
    if (isnan(temp) || isnan(hum)) {
      strcpy(_sensorError, "NaN from sensor");
    } else {
      snprintf(_sensorError, sizeof(_sensorError),
               "Invalid data: H=%.1f%% (must be 0-100)", hum);
    }
    XLOG_ERROR(CAT_SENSOR, "%s", _sensorError);
    _humRate = 0;
    return false;
  }

  // ========== ДАННЫЕ ВАЛИДНЫ — ОБНОВЛЯЕМ КЭШ ==========

  // расчёт скорости изменения влажности
  if (_lastHumTime > 0) {
    float dt = (millis() - _lastHumTime) / 1000.0;
    if (dt > 0.1) {
      _humRate = (hum - _lastHumValue) / dt;
      // Ограничение для фильтрации выбросов
      if (_humRate > 5.0)
        _humRate = 5.0;
      if (_humRate < -5.0)
        _humRate = -5.0;
    }
  } else {
    _humRate = 0;
  }

  _lastHumValue = hum;
  _lastHumTime = millis();

  // Проверяем, изменились ли данные (для возврата true/false)
  bool changed =
      (fabs(_currentTemp - temp) > 0.05 || fabs(_currentHum - hum) > 0.05);

  _currentTemp = temp;
  _currentHum = hum;
  _sensorOk = true;
  _sensorError[0] = '\0';

  XLOG_DEBUG(CAT_SENSOR, "T=%.2f°C, H=%.2f%% (rate=%.2f%%/s)", _currentTemp,
             _currentHum, _humRate);

  return changed;
}

// ========== ГЕТТЕРЫ ==========

float sensor_getTemperature() {
  return _currentTemp;
}

float sensor_getHumidity() {
  return _currentHum;
}

bool sensor_isOk() {
  return _sensorOk;
}

const char* sensor_getError() {
  return _sensorError;
}

float sensor_getHumRate() {
  return _humRate;
}

#endif  // DEVICE_TYPE == 1 || DEVICE_TYPE == 2