#include "sensor.h"
#include "ansi.h"

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2

#include "config.h"
#include "mqtt.h"

float currentTemp = 0;
float currentHum = 0;
bool sensorOk = false;
unsigned long lastSensorRead = 0;
char sensorError[64] = "";  // Было String sensorError = ""
float humRate = 0;  

#if SENSOR_TYPE == 1
  Adafruit_AHTX0 aht;
#elif SENSOR_TYPE == 2
  DHT dht(SENSOR_PIN, DHT_TYPE);
#endif

void sensor_init() {
  #if SENSOR_TYPE == 1
    
    if (aht.begin()) {
      sensorOk = false;
      strcpy(sensorError, "Waiting for first valid reading");
      
      #if LOG_SENSOR == 1
        Serial.println("[SENSOR] AHT10 found, waiting for first valid reading...");
      #endif
    } else {
      sensorOk = false;
      strcpy(sensorError, "AHT10 not found");
      Serial.print(ANSI_BRIGHT_RED);
      Serial.println("[SENSOR] AHT10 not found! Sensor will be disabled.");
      Serial.print(ANSI_RESET);
    }
  #elif SENSOR_TYPE == 2    
    dht.begin();
    delay(2000);
    sensorOk = false;
    strcpy(sensorError, "Waiting for first valid reading");
    #if LOG_SENSOR == 1
      Serial.println("[SENSOR] DHT initialized, waiting for first valid reading...");
    #endif
  #endif
  
  humRate = 0;  
}

bool isSensorValueValid(float temp, float hum) {
  if (temp < -40 || temp > 85) return false;
  if (hum < 0 || hum > 100) return false;
  if (temp == 0.0 && hum == 0.0) return false;
  return true;
}

void sensor_read() {
  if (strcmp(sensorError, "AHT10 not found") == 0) {
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
      strcpy(sensorError, "AHT10 I2C read failed");
      Serial.print(ANSI_BRIGHT_RED);
      Serial.println("[SENSOR] AHT10 read error!");
      Serial.print(ANSI_RESET);
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
      strcpy(sensorError, "DHT read failed (NaN)");
      Serial.print(ANSI_BRIGHT_RED);
      Serial.println("[SENSOR] DHT read error!");
      Serial.print(ANSI_RESET);
    }
  #endif
  
  if (readSuccess) {
    if (isSensorValueValid(temp, hum)) {
      // расчёт скорости изменения влажности
      static unsigned long lastHumTime = 0;
      static float lastHumValue = 0;
      
      if (lastHumTime > 0) {
        float dt = (millis() - lastHumTime) / 1000.0;
        if (dt > 0.1) {  // Защита от деления на ноль
          humRate = (hum - lastHumValue) / dt;
          // Ограничиваем экстремальные значения
          if (humRate > 5.0) humRate = 5.0;
          if (humRate < -5.0) humRate = -5.0;
        }
      } else {
        humRate = 0;
      }
      
      lastHumValue = hum;
      lastHumTime = millis();
      
      currentTemp = temp;
      currentHum = hum;
      sensorOk = true;
      sensorError[0] = '\0';
      
      #if LOG_SENSOR == 1
        Serial.printf("[SENSOR] T=%.2f°C, H=%.2f%% (rate=%.2f%%/s)\n", 
                      currentTemp, currentHum, humRate);
      #endif

    } else {
      sensorOk = false;
      snprintf(sensorError, sizeof(sensorError), "Out of range (T=%.1f H=%.1f)", temp, hum);
      humRate = 0;
      
      #if LOG_SENSOR == 1
        Serial.printf("[SENSOR] %s\n", sensorError);
      #endif
    }
  } else {
    humRate = 0;
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

#endif