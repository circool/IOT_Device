#include "sensor.h"
#include "config.h"

float currentTemp = 0;
float currentHum = 0;
bool sensorOk = false;
unsigned long lastSensorRead = 0;

#if SENSOR_TYPE == 1
  Adafruit_AHTX0 aht;
#elif SENSOR_TYPE == 2
  DHT dht(SENSOR_PIN, DHT_TYPE);
#endif

void sensor_init() {
  #if SENSOR_TYPE == 1
    if (aht.begin()) {
      sensorOk = true;
      Serial.println("[SENSOR] AHT10 initialized");
    } else {
      sensorOk = false;
      Serial.println("[SENSOR] AHT10 not found! Sensor will be disabled.");
    }
  #elif SENSOR_TYPE == 2
    dht.begin();
    sensorOk = true;
    Serial.println("[SENSOR] DHT initialized");
  #else
    #error "Unknown SENSOR_TYPE! Define SENSOR_TYPE as 1 (AHT10) or 2 (DHT11/22)"
  #endif
}

void sensor_read() {
  // Если датчик не инициализирован - не читаем
  if (!sensorOk) return;
  
  // Проверяем интервал
  if (millis() - lastSensorRead < config.sensorInterval * 1000UL) {
    return;
  }
  lastSensorRead = millis();
  
  #if SENSOR_TYPE == 1
    sensors_event_t humidity, temperature;
    if (aht.getEvent(&humidity, &temperature)) {
      currentTemp = temperature.temperature;
      currentHum = humidity.relative_humidity;
      sensorOk = true;
      #ifdef DEBUG_ENABLE
        Serial.printf("[SENSOR] AHT10: T=%.2f°C, H=%.2f%%\n", currentTemp, currentHum);
      #endif
    } else {
      sensorOk = false;
      Serial.println("[SENSOR] AHT10 read error!");
    }
  #elif SENSOR_TYPE == 2
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      currentTemp = t;
      currentHum = h;
      sensorOk = true;
      #ifdef DEBUG_ENABLE
        Serial.printf("[SENSOR] DHT: T=%.2f°C, H=%.2f%%\n", currentTemp, currentHum);
      #endif
    } else {
      sensorOk = false;
      Serial.println("[SENSOR] DHT read error!");
    }
  #endif
}

bool sensor_isOk() {
  return sensorOk;
}

float sensor_getTemperature() {
  return currentTemp;
}

float sensor_getHumidity() {
  return currentHum;
}