#include "sensor.h"
#include "logger.h"

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2

#include "config.h"

// ========== СТАТИЧЕСКИЕ ПЕРЕМЕННЫЕ ==========
static float _currentTemp = 0;
static float _currentHum = 0;
static bool _sensorOk = false;
static unsigned long _lastSensorRead = 0;
static char _sensorError[64] = "";
static float _humRate = 0;

static unsigned long _lastHumTime = 0;
static float _lastHumValue = 0;

static const Config* _cfg = nullptr;  // указатель на активный конфиг

#if SENSOR_TYPE == 1
static Adafruit_AHTX0 _aht;
#elif SENSOR_TYPE == 2
static DHT _dht(SENSOR_PIN, DHT_TYPE);
#endif

// ========== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ==========
static bool isSensorValueValid(float temp, float hum) {
  if (temp < TEMP_MIN || temp > TEMP_MAX)
    return false;
  if (hum < 0 || hum > 100)
    return false;
  if (temp == 0.0 && hum == 0.0)
    return false;
  return true;
}

// ========== ПУБЛИЧНЫЕ ФУНКЦИИ ==========

void sensor_init(const Config* cfg) {
  _cfg = cfg;

#if SENSOR_TYPE == 1
  Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
  _sensorOk = _aht.begin();
  if (_sensorOk) {
    strcpy(_sensorError, "Waiting for first valid reading");
    LOG_DEBUG(CAT_SENSOR, "AHT10 found, waiting for first valid reading...");
    sensors_event_t humidity, temperature;
    if (_aht.getEvent(&humidity, &temperature)) {
      _currentTemp = temperature.temperature;
      _currentHum = humidity.relative_humidity;
      _sensorError[0] = '\0';
      _lastSensorRead = millis();
      LOG_INFO(CAT_SENSOR, "First reading: T=%.2f°C, H=%.2f%%", _currentTemp,
               _currentHum);
    }
  } else {
    strcpy(_sensorError, "AHT10 not found");
    LOG_ERROR(CAT_SENSOR, "AHT10 not found! Sensor will be disabled.");
  }

#elif SENSOR_TYPE == 2
  _dht.begin();
  delay(2000);
  _sensorOk = false;
  strcpy(_sensorError, "Waiting for first valid reading");
  LOG_INFO(CAT_SENSOR, "DHT initialized, waiting for first valid reading...");
#endif

  _humRate = 0;
  _lastHumTime = 0;
  _lastHumValue = 0;
}

bool sensor_update() {
  if (!_cfg)
    return false;

  if (strcmp(_sensorError, "AHT10 not found") == 0) {
    return false;
  }

  if (millis() - _lastSensorRead < _cfg->sensorInterval * 1000UL) {
    return false;
  }
  _lastSensorRead = millis();

  bool readSuccess = false;
  float temp = 0, hum = 0;

#if SENSOR_TYPE == 1
  sensors_event_t humidity, temperature;
  if (_aht.getEvent(&humidity, &temperature)) {
    temp = temperature.temperature;
    hum = humidity.relative_humidity;
    readSuccess = true;
  } else {
    _sensorOk = false;
    strcpy(_sensorError, "AHT10 I2C read failed");
    LOG_ERROR(CAT_SENSOR, "AHT10 read error!");
  }
#elif SENSOR_TYPE == 2
  float t = _dht.readTemperature();
  float h = _dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    temp = t;
    hum = h;
    readSuccess = true;
  } else {
    _sensorOk = false;
    strcpy(_sensorError, "DHT read failed (NaN)");
    LOG_ERROR(CAT_SENSOR, "DHT read error!");
  }
#endif

  if (readSuccess) {
    if (isSensorValueValid(temp, hum)) {
      if (_lastHumTime > 0) {
        float dt = (millis() - _lastHumTime) / 1000.0;
        if (dt > 0.1) {
          _humRate = (hum - _lastHumValue) / dt;
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

      bool changed =
          (fabs(_currentTemp - temp) > 0.05 || fabs(_currentHum - hum) > 0.05);

      _currentTemp = temp;
      _currentHum = hum;
      _sensorOk = true;
      _sensorError[0] = '\0';

      LOG_DEBUG(CAT_SENSOR, "T=%.2f°C, H=%.2f%% (rate=%.2f%%/s)", _currentTemp,
                _currentHum, _humRate);

      return changed;
    } else {
      _sensorOk = false;
      snprintf(_sensorError, sizeof(_sensorError),
               "Out of range (T=%.1f H=%.1f)", temp, hum);
      _humRate = 0;
      LOG_ERROR(CAT_SENSOR, "%s", _sensorError);
      return false;
    }
  } else {
    _humRate = 0;
    return false;
  }
}

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