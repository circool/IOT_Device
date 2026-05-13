#include "fan.h"

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3

#include "config.h"
#include "sensor.h"
#include "mqtt.h"

bool fanOn = false;
unsigned long fanStartTime = 0;
unsigned long delayTimer = 0;
bool delayActive = false;

// Переменные стартового импульса
bool startingPulseActive = false;
unsigned long startingPulseStart = 0;

// Переменные адаптивного режима
bool adaptiveActive = false;
float baseTemp = 0;
float baseHum = 0;
unsigned long lastAdaptiveCheck = 0;

// Вспомогательная функция: перевод процентов (0-100) в значение ШИМ для платформы
static int percentToPWMValue(int percent) {
  if (percent <= 0) return 0;
  if (percent >= 100) return 255;
  return map(percent, 0, 100, 0, 255);
}

// Применение ШИМ с указанной скважностью в процентах
void fan_applyPWM(int percent) {
  if (percent <= 0) {
    // Выключено
    #ifdef ESP32
      ledcDetachPin(SWITCH_PIN);
    #endif
    digitalWrite(SWITCH_PIN, LOW);
    if (fanOn && !startingPulseActive) {
      fanOn = false;
      fanStartTime = 0;
      #ifdef DEBUG_ENABLE
        Serial.println("[FAN] Warning: PWM duty 0% with fanOn=true - forcing OFF");
      #endif
    }
    #ifdef DEBUG_ENABLE
      Serial.println("[FAN] PWM: OFF");
    #endif
  } else if (percent >= 100) {
    // Полная мощность
    #ifdef ESP32
      ledcDetachPin(SWITCH_PIN);
    #endif
    digitalWrite(SWITCH_PIN, HIGH);
    #ifdef DEBUG_ENABLE
      Serial.println("[FAN] PWM: FULL POWER (100%)");
    #endif
  } else {
    // ШИМ с заданной скважностью
    int pwmValue = percentToPWMValue(percent);
    #ifdef ESP32
      ledcAttachPin(SWITCH_PIN, 0);
      ledcWrite(0, pwmValue);
    #elif defined(ESP8266)
      analogWrite(SWITCH_PIN, pwmValue);
    #endif
    #ifdef DEBUG_ENABLE
      Serial.printf("[FAN] PWM: %d%% (value %d/255)\n", percent, pwmValue);
    #endif
  }
}

int fan_getCurrentPWMDuty() {
  if (!fanOn) return 0;
  if (startingPulseActive) return 100;
  if (config.pwmDutyPercent >= 100) return 100;
  return config.pwmDutyPercent;
}

void fan_init() {
  pinMode(SWITCH_PIN, OUTPUT);
  
  #ifdef ESP32
    ledcSetup(0, PWM_FREQUENCY, PWM_RESOLUTION);
    ledcAttachPin(SWITCH_PIN, 0);
  #elif defined(ESP8266)
    analogWriteFreq(PWM_FREQUENCY);
    analogWriteRange(255);
  #endif
  
  bool currentPinState = (digitalRead(SWITCH_PIN) == HIGH);
  
  if (config.forceOffOnBoot) {
    fanOn = false;
    fan_applyPWM(0);
    startingPulseActive = false;
    adaptiveActive = false;
    #ifdef DEBUG_ENABLE
      Serial.println("[FAN] Force OFF on boot - forcing OFF");
    #endif
  } else {
    fanOn = currentPinState;
    if (fanOn) {
      if (config.pwmDutyPercent < 100) {
        fan_applyPWM(100);
        startingPulseActive = true;
        startingPulseStart = millis();
        adaptiveActive = false;
        #ifdef DEBUG_ENABLE
          Serial.printf("[FAN] Keep state ON with slow mode - starting pulse for %d ms\n", PWM_STARTING);
        #endif
      } else {
        fan_applyPWM(100);
        startingPulseActive = false;
        #if DEVICE_TYPE == 1
        if (config.adaptiveMode && sensor_isOk()) {
          adaptiveActive = true;
          baseTemp = currentTemp;
          baseHum = currentHum;
          lastAdaptiveCheck = millis();
          #ifdef DEBUG_ENABLE
            Serial.printf("[FAN] Adaptive mode activated: base T=%.2f, H=%.2f\n", baseTemp, baseHum);
          #endif
        } else if (config.adaptiveMode && !sensor_isOk()) {
          #ifdef DEBUG_ENABLE
            Serial.println("[FAN] Adaptive mode waiting for valid sensor readings...");
          #endif
        }
        #endif
      }
    } else {
      startingPulseActive = false;
      adaptiveActive = false;
    }
    #ifdef DEBUG_ENABLE
      Serial.printf("[FAN] Keep state on boot - synced with pin state: %s\n", 
                    fanOn ? "ON" : "OFF");
    #endif
  }
  
  delayActive = false;
  delayTimer = 0;
  fanStartTime = fanOn ? millis() : 0;
  
  #if DEVICE_TYPE == 1
  if (!config.sensorControlMode) {
    #ifdef DEBUG_ENABLE
      Serial.println("[FAN] Starting in MANUAL mode");
    #endif
  }
  #endif
}

void fan_set(bool on) {
  if (fanOn == on) return;
  
  startingPulseActive = false;
  adaptiveActive = false;
  
  fanOn = on;
  
  if (fanOn) {
    fanStartTime = millis();
    if (config.pwmDutyPercent < 100) {
      fan_applyPWM(100);
      startingPulseActive = true;
      startingPulseStart = millis();
      #ifdef DEBUG_ENABLE
        Serial.printf("[FAN] Starting pulse started, duration=%d ms\n", PWM_STARTING);
      #endif
    } else {
      fan_applyPWM(100);
      #if DEVICE_TYPE == 1
      if (config.adaptiveMode && sensor_isOk()) {
        adaptiveActive = true;
        baseTemp = currentTemp;
        baseHum = currentHum;
        lastAdaptiveCheck = millis();
        #ifdef DEBUG_ENABLE
          Serial.printf("[FAN] Adaptive mode activated: base T=%.2f, H=%.2f\n", baseTemp, baseHum);
        #endif
      }
      #endif
      #ifdef DEBUG_ENABLE
        Serial.println("[FAN] Fan turned ON");
      #endif
    }
  } else {
    fan_applyPWM(0);
    fanStartTime = 0;
    adaptiveActive = false;
    
    // Восстанавливаем сохранённую настройку пользователя
    uint16_t oldDuty = config.pwmDutyPercent;
    config.pwmDutyPercent = staticConfig.pwmDutyPercent;
    
    // Публикуем только если скважность изменилась и подключены к MQTT
    if (oldDuty != config.pwmDutyPercent && mqtt_isConnected()) {
      mqttClient.publish(pwmDutyStateTopic, String(config.pwmDutyPercent).c_str());
      #ifdef DEBUG_MQTT
        Serial.printf("[MQTT] PWM duty restored to %d%% (was %d%%)\n", config.pwmDutyPercent, oldDuty);
      #endif
    }
    
    #ifdef DEBUG_ENABLE
      Serial.printf("[FAN] Fan turned OFF, restored PWM duty to %d%%\n", config.pwmDutyPercent);
    #endif
  }
  
  // Публикуем состояние при любом изменении (если подключены к MQTT)
  if (mqtt_isConnected()) {
    mqtt_publishState();
  }
}
bool fan_getState() {
  return fanOn;
}

bool fan_getRealState() {
  return fanOn;
}

void fan_setOverrideMode(bool sensorControl) {
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  config.sensorControlMode = sensorControl;
  if (!sensorControl) {
    delayActive = false;
    adaptiveActive = false;
  } else {
    // При переходе в режим управления сенсором, если вентилятор включён и есть валидные данные
    if (fanOn && config.adaptiveMode && DEVICE_TYPE == 1 && sensor_isOk()) {
      adaptiveActive = true;
      baseTemp = currentTemp;
      baseHum = currentHum;
      lastAdaptiveCheck = millis();
    }
  }
  // НЕ СОХРАНЯЕМ в EEPROM при MQTT команде
  #ifdef DEBUG_ENABLE
    Serial.printf("[FAN] Mode switched to: %s\n", sensorControl ? "SENSOR CONTROL" : "MANUAL");
  #endif
  #endif
}

void fan_checkMaxOnTime() {
  if (fanOn && fanStartTime != 0 && config.maxOnTime > 0 &&
      (millis() - fanStartTime) > config.maxOnTime * 1000UL) {
    #ifdef DEBUG_ENABLE
      Serial.println("[FAN] Max on time exceeded, forcing OFF");
    #endif
    
    #if DEVICE_TYPE == 1
    config.sensorControlMode = false;
    #ifdef DEBUG_ENABLE
      Serial.println("[FAN] Switched to MANUAL mode after safety shutdown");
    #endif
    #endif
    
    fan_set(false);
  }
}

bool fan_delayTimer(bool start) {
  if (start) {
    if (config.delaySeconds > 0) {
      delayActive = true;
      delayTimer = millis() + config.delaySeconds * 1000UL;
      #ifdef DEBUG_ENABLE
        Serial.printf("[FAN] Delay ON timer started: %d seconds\n", config.delaySeconds);
      #endif
      return false;
    }
    delayActive = false;
    return false;
  } else {
    if (delayActive && millis() >= delayTimer) {
      delayActive = false;
      #if DEVICE_TYPE == 1
      config.sensorControlMode = false;
      #ifdef DEBUG_ENABLE
        Serial.println("[FAN] Delay ON timer finished - switching to MANUAL mode (temporary)");
      #endif
      #elif DEVICE_TYPE == 3
      #ifdef DEBUG_ENABLE
        Serial.println("[FAN] Delay ON timer finished - turning ON");
      #endif
      #endif
      return true;
    }
    return false;
  }
}

void fan_adaptiveUpdate() {
    if (!sensor_isOk()) {
        if (adaptiveActive) {
            adaptiveActive = false;
            #ifdef DEBUG_ENABLE
                Serial.println("[FAN] Adaptive mode disabled - sensor error");
            #endif
            if (fanOn && !startingPulseActive) {
                config.pwmDutyPercent = 100;
                fan_applyPWM(100);
            }
        }
        return;
    }
    
    if (!fanOn || startingPulseActive || !config.adaptiveMode || !config.sensorControlMode) {
        if (adaptiveActive) adaptiveActive = false;
        return;
    }
    
    if (!adaptiveActive) {
        adaptiveActive = true;
        baseTemp = currentTemp;
        baseHum = currentHum;
        lastAdaptiveCheck = millis();
        #ifdef DEBUG_ENABLE
            Serial.printf("[FAN] Adaptive active: base T=%.2f, H=%.2f\n", baseTemp, baseHum);
        #endif
        return;
    }
    
    if (millis() - lastAdaptiveCheck < config.sensorInterval * 1000UL) {
        return;
    }
    lastAdaptiveCheck = millis();
    
    float deltaTemp = currentTemp - baseTemp;
    float deltaHum = currentHum - baseHum;
    
    int newDuty = config.pwmDutyPercent;
    bool needChange = false;
    
    // Расчёт шага с учётом скорости
    int step = ADAPTIVE_STEP_SIZE;
    
    if (deltaTemp > ADAPTIVE_EPSILON_TEMP * 2 || deltaHum > ADAPTIVE_EPSILON_HUM * 2) {
        step = step * 2;
    }
    if (deltaTemp > ADAPTIVE_EPSILON_TEMP * 3 || deltaHum > ADAPTIVE_EPSILON_HUM * 3) {
        step = step * 3;
    }
    
    // Скоростной множитель
    float speedMultiplier = 1.0;
    if (humRate > 1.5) {
        speedMultiplier = 2.5;
    } else if (humRate > 0.5) {
        speedMultiplier = 1.5;
    } else if (humRate < -0.5) {
        speedMultiplier = 1.5;
    }
    
    step = step * speedMultiplier;
    if (step > 60) step = 60;
    if (step < 5) step = 5;
    
    if (deltaTemp > ADAPTIVE_EPSILON_TEMP || deltaHum > ADAPTIVE_EPSILON_HUM) {
        newDuty += step;
        if (newDuty > 100) newDuty = 100;
        if (newDuty != config.pwmDutyPercent) {
            needChange = true;
            #ifdef DEBUG_ENABLE
                Serial.printf("[FAN] +%d%% (%.2f/%.2f rate=%.1f) → %d%%\n", 
                              step, deltaTemp, deltaHum, humRate, newDuty);
            #endif
        }
    }
    else if (deltaTemp < -ADAPTIVE_EPSILON_TEMP && deltaHum < -ADAPTIVE_EPSILON_HUM) {
        newDuty -= step;
        if (newDuty < MIN_PWM_DUTY_PERCENT) newDuty = MIN_PWM_DUTY_PERCENT;
        if (newDuty != config.pwmDutyPercent) {
            needChange = true;
            #ifdef DEBUG_ENABLE
                Serial.printf("[FAN] -%d%% (%.2f/%.2f rate=%.1f) → %d%%\n", 
                              step, deltaTemp, deltaHum, humRate, newDuty);
            #endif
        }
    }
    
    if (needChange) {
        config.pwmDutyPercent = newDuty;
        fan_applyPWM(config.pwmDutyPercent);
        
        if (mqtt_isConnected()) {
            mqttClient.publish(pwmDutyStateTopic, String(config.pwmDutyPercent).c_str());
        }
        
        baseTemp = currentTemp;
        baseHum = currentHum;
    }
}
void fan_update() {
  // Обработка завершения стартового импульса
  if (startingPulseActive) {
    if (millis() - startingPulseStart >= PWM_STARTING) {
      if (config.pwmDutyPercent <= 0) {
        fanOn = false;
        fan_applyPWM(0);
        startingPulseActive = false;
        adaptiveActive = false;
        #ifdef DEBUG_ENABLE
          Serial.println("[FAN] Starting pulse finished, but PWM duty is 0% - turning OFF");
        #endif
      } else {
        fan_applyPWM(config.pwmDutyPercent);
        startingPulseActive = false;
        
        // Если адаптивный режим включён, активируем его после стартового импульса (только TYPE 1)
        #if DEVICE_TYPE == 1
        if (config.adaptiveMode && sensor_isOk() && config.sensorControlMode) {
          adaptiveActive = true;
          baseTemp = currentTemp;
          baseHum = currentHum;
          lastAdaptiveCheck = millis();
          #ifdef DEBUG_ENABLE
            Serial.printf("[FAN] Adaptive mode activated after starting pulse: base T=%.2f, H=%.2f, duty=%d%%\n", 
                          baseTemp, baseHum, config.pwmDutyPercent);
          #endif
        }
        #endif
      }
      
      #ifdef DEBUG_ENABLE
        Serial.printf("[FAN] Starting pulse finished, switched to duty=%d%%\n", config.pwmDutyPercent);
      #endif
    }
  }

  #if DEVICE_TYPE == 1
  // Только для TYPE 1: ручной режим
  if (!config.sensorControlMode) {
    fan_checkMaxOnTime();
    if (adaptiveActive) adaptiveActive = false;
    return;
  }
  #endif
  
  #if DEVICE_TYPE == 3
  // Для TYPE 3: всегда проверяем maxOnTime
  fan_checkMaxOnTime();
  #endif
  
  // Адаптивное обновление (только для TYPE 1, в режиме управления сенсором)
  #if DEVICE_TYPE == 1
  if (config.sensorControlMode && fanOn) {
    fan_adaptiveUpdate();
  }
  #endif
  
  // Автоматическое управление по датчикам (только для TYPE 1)
  bool sensorShouldBeOn = false;
  bool timerExpired = false;
  
  #if DEVICE_TYPE == 1
  if (sensor_isOk()) {
    bool tempHigh = (currentTemp >= config.highTemp);
    bool humHigh = (currentHum >= config.highHum);
    bool tempLow = (currentTemp <= config.lowTemp);
    bool humLow = (currentHum <= config.lowHum);
    
    if (tempHigh || humHigh) {
      sensorShouldBeOn = true;
    } else if (tempLow && humLow) {
      sensorShouldBeOn = false;
    } else {
      sensorShouldBeOn = fanOn;
    }
  } else {
    // Если датчик не валиден, не меняем состояние вентилятора
    sensorShouldBeOn = fanOn;
  }
  #endif
  
  // Таймер задержки для TYPE 3 работает всегда
  #if DEVICE_TYPE == 3
  timerExpired = fan_delayTimer(false);
  #else
  timerExpired = fan_delayTimer(false);
  #endif
  
  if (sensorShouldBeOn || timerExpired) {
    if (!fanOn) {
      fan_set(true);
      if (delayActive) {
        delayActive = false;
        #ifdef DEBUG_ENABLE
          Serial.println("[FAN] Timer cancelled - fan turned ON by sensor");
        #endif
      }
    }
  } else {
    if (fanOn) {
      fan_set(false);
      #if DEVICE_TYPE == 1
      if (config.delaySeconds > 0 && !delayActive && config.sensorControlMode) {
        fan_delayTimer(true);
      }
      #elif DEVICE_TYPE == 3
      if (config.delaySeconds > 0 && !delayActive) {
        fan_delayTimer(true);
      }
      #endif
    } else {
      #if DEVICE_TYPE == 1
      if (config.delaySeconds > 0 && !delayActive && !sensorShouldBeOn && config.sensorControlMode) {
        fan_delayTimer(true);
      }
      #elif DEVICE_TYPE == 3
      if (config.delaySeconds > 0 && !delayActive && !sensorShouldBeOn) {
        fan_delayTimer(true);
      }
      #endif
    }
  }
  
  fan_checkMaxOnTime();
}
#endif // DEVICE_TYPE == 1 || DEVICE_TYPE == 3