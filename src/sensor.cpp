#include "sensor.h"

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2

#include "config.h"

float currentTemp = 0;
float currentHum = 0;
bool sensorOk = false;
unsigned long lastSensorRead = 0;
String sensorError = "";

#if SENSOR_TYPE == 1
  Adafruit_AHTX0 aht;
#elif SENSOR_TYPE == 2
  DHT dht(SENSOR_PIN, DHT_TYPE);
#endif

void sensor_init() {
  #if SENSOR_TYPE == 1
    if (aht.begin()) {
      // Датчик найден на шине, но пока нет валидных показаний
      sensorOk = false;
      sensorError = "Waiting for first valid reading";
      Serial.println("[SENSOR] AHT10 found, waiting for first valid reading...");
    } else {
      sensorOk = false;
      sensorError = "AHT10 not found";
      Serial.println("[SENSOR] AHT10 not found! Sensor will be disabled.");
    }
  #elif SENSOR_TYPE == 2    
    dht.begin();
    delay(2000);
    // DHT требует времени на первый опрос
    sensorOk = false;
    sensorError = "Waiting for first valid reading";
    Serial.println("[SENSOR] DHT initialized, waiting for first valid reading...");
  #endif
}

bool isSensorValueValid(float temp, float hum) {
  // Проверка диапазонов
  if (temp < -40 || temp > 85) return false;
  if (hum < 0 || hum > 100) return false;
  // Проверка на нулевые значения (признак ошибки чтения)
  if (temp == 0.0 && hum == 0.0) return false;
  return true;
}

void sensor_read() {
  // Если датчик физически отсутствует - не пытаемся читать
  if (sensorError == "AHT10 not found") {
    return;
  }
  
  if (millis() - lastSensorRead < config.sensorInterval * 1000UL) {
    return;
  }
  lastSensorRead = millis();
  
  bool readSuccess = false;
  float temp = 0, hum = 0;
  
  #if SENSOR_TYPE == 1
    sensors_event_t humidity, temperature;
    if (aht.getEvent(&humidity, &temperature)) {
      temp = temperature.temperature;
      hum = humidity.relative_humidity;
      readSuccess = true;
    } else {
      sensorOk = false;
      sensorError = "AHT10 I2C read failed";
      Serial.println("[SENSOR] AHT10 read error!");
    }
  #elif SENSOR_TYPE == 2
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      temp = t;
      hum = h;
      readSuccess = true;
    } else {
      sensorOk = false;
      sensorError = "DHT read failed (NaN)";
      Serial.println("[SENSOR] DHT read error!");
    }
  #endif
  
  if (readSuccess) {
    if (isSensorValueValid(temp, hum)) {
      // Валидные показания - обновляем
      currentTemp = temp;
      currentHum = hum;
      sensorOk = true;
      sensorError = "";
      #ifdef DEBUG_ENABLE
        Serial.printf("[SENSOR] T=%.2f°C, H=%.2f%%\n", currentTemp, currentHum);
      #endif
    } else {
      // Показания получены, но невалидные
      sensorOk = false;
      sensorError = "Out of range (T=" + String(temp, 1) + " H=" + String(hum, 1) + ")";
      #ifdef DEBUG_ENABLE
        Serial.printf("[SENSOR] %s\n", sensorError.c_str());
      #endif
    }
  }
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

#endif // DEVICE_TYPE == 1 || DEVICE_TYPE == 2