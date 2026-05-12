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
      sensorOk = true;
      sensorError = "";
      Serial.println("[SENSOR] AHT10 initialized");
    } else {
      sensorOk = false;
      sensorError = "AHT10 not found";
      Serial.println("[SENSOR] AHT10 not found! Sensor will be disabled.");
    }
  #elif SENSOR_TYPE == 2    
    dht.begin();
    delay(2000);
    sensorOk = true;
    sensorError = "";
    Serial.println("[SENSOR] DHT initialized");
  #endif
}

void sensor_read() {
  if (!sensorOk && sensorError.length() > 0 && sensorError == "AHT10 not found") {
    return;  // Датчик не найден при инициализации — не пытаемся читать
  }
  
  if (millis() - lastSensorRead < config.sensorInterval * 1000UL) {
    return;
  }
  lastSensorRead = millis();
  
  #if SENSOR_TYPE == 1
    sensors_event_t humidity, temperature;
    if (aht.getEvent(&humidity, &temperature)) {
      currentTemp = temperature.temperature;
      currentHum = humidity.relative_humidity;
      
      if (currentTemp < -40 || currentTemp > 85 || 
          currentHum < 0 || currentHum > 100) {
        sensorOk = false;
        sensorError = "AHT10 out of range (T=" + String(currentTemp, 1) + " H=" + String(currentHum, 1) + ")";
        #ifdef DEBUG_ENABLE
          Serial.printf("[SENSOR] %s\n", sensorError.c_str());
        #endif
      } else if (currentTemp == 0.0 && currentHum == 0.0) {
        sensorOk = false;
        sensorError = "AHT10 returned zero values";
        #ifdef DEBUG_ENABLE
          Serial.printf("[SENSOR] %s\n", sensorError.c_str());
        #endif
      } else {
        sensorOk = true;
        sensorError = "";
        #ifdef DEBUG_ENABLE
          Serial.printf("[SENSOR] AHT10: T=%.2f°C, H=%.2f%%\n", currentTemp, currentHum);
        #endif
      }
    } else {
      sensorOk = false;
      sensorError = "AHT10 I2C read failed";
      Serial.println("[SENSOR] AHT10 read error!");
    }
  #elif SENSOR_TYPE == 2
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      currentTemp = t;
      currentHum = h;
      
      if (currentTemp < -40 || currentTemp > 85 || 
          currentHum < 0 || currentHum > 100) {
        sensorOk = false;
        sensorError = "DHT out of range (T=" + String(currentTemp, 1) + " H=" + String(currentHum, 1) + ")";
        #ifdef DEBUG_ENABLE
          Serial.printf("[SENSOR] %s\n", sensorError.c_str());
        #endif
      } else {
        sensorOk = true;
        sensorError = "";
        #ifdef DEBUG_ENABLE
          Serial.printf("[SENSOR] DHT: T=%.2f°C, H=%.2f%%\n", currentTemp, currentHum);
        #endif
      }
    } else {
      sensorOk = false;
      sensorError = "DHT read failed (NaN)";
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

#endif // DEVICE_TYPE == 1 || DEVICE_TYPE == 2