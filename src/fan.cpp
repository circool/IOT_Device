#include "fan.h"

#if DEVICE_TYPE == 1

#include "config.h"
#include "sensor.h"

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

// Флаг, используется ли ШИМ для текущего состояния
static bool pwmActive = false;

// Вспомогательная функция: перевод процентов (0-100) в значение ШИМ для платформы
static int percentToPWMValue(int percent) {
  if (percent <= 0) return 0;
  if (percent >= 100) return 255;
  return map(percent, 0, 100, 0, 255);
}

// Отключить ШИМ и перейти в режим обычного GPIO
static void disablePWM() {
  if (!pwmActive) return;
  
  #ifdef ESP32
    ledcDetachPin(SWITCH_PIN);
  #elif defined(ESP8266)
    analogWrite(SWITCH_PIN, 1024);
    delayMicroseconds(10);
    pinMode(SWITCH_PIN, OUTPUT);
  #endif
  pwmActive = false;
  
  #if LOG_FAN == 1
    Serial.println("[FAN] PWM disabled, switched to GPIO mode");
  #endif
}

// Включить ШИМ режим
static void enablePWM() {
  if (pwmActive) return;
  
  #ifdef ESP32
    ledcAttachPin(SWITCH_PIN, 0);
  #endif
  pwmActive = true;
  
  #if LOG_FAN == 1
    Serial.println("[FAN] PWM enabled");
  #endif
}

// Применение ШИМ с указанной скважностью в процентах
void fan_applySpeed(int percent) {
  if (percent <= 0) {
    disablePWM();
    digitalWrite(SWITCH_PIN, !RELAY_ON_LEVEL);
    if (fanOn && !startingPulseActive) {
      fanOn = false;
      fanStartTime = 0;
      
      #if LOG_FAN == 1
        Serial.println("[FAN] Warning: Speed 0% with fanOn=true - forcing OFF");
      #endif
    }
    
    #if LOG_FAN == 1
      Serial.println("[FAN] Speed: OFF");
    #endif
  
  } else if (percent >= 100) {
    disablePWM();
    digitalWrite(SWITCH_PIN, RELAY_ON_LEVEL);
    
    #if LOG_FAN == 1
      Serial.println("[FAN] Speed: FULL POWER (100%)");
    #endif
  
  } else {
    int pwmValue = percentToPWMValue(percent);
    
    #if RELAY_ON_LEVEL == LOW
      pwmValue = 255 - pwmValue;
    #endif
    
    enablePWM();
    
    #ifdef ESP32
      ledcWrite(0, pwmValue);
    #elif defined(ESP8266)
      analogWrite(SWITCH_PIN, pwmValue);
    #endif
    
    #if LOG_FAN == 1
      Serial.printf("[FAN] Speed: %d%% (value %d/255)\n", percent, pwmValue);
    #endif
  }
}

void fan_init() {
  pinMode(SWITCH_PIN, OUTPUT);
  pwmActive = false;
  #if DEVICE_TYPE == 1
    #ifdef ESP32
      ledcSetup(0, PWM_FREQUENCY, PWM_RESOLUTION);
    #elif defined(ESP8266)
      analogWriteFreq(PWM_FREQUENCY);
      analogWriteRange(255);
    #endif
  #endif

  delayActive = false;
  delayTimer = 0;
  startingPulseActive = false;
  adaptiveActive = false;
  fanStartTime = 0;
  
  // >>> ИЗМЕНЕНИЕ: config.bootState → config_get()->bootState
  if (config_get()->bootState) {
    disablePWM();
    digitalWrite(SWITCH_PIN, RELAY_ON_LEVEL);
    fanOn = true;
    
    #if LOG_FAN == 1
      Serial.printf("[FAN] Boot: set to ON (pin %d = %s)\n", SWITCH_PIN, RELAY_ON_LEVEL == HIGH ? "HIGH" : "LOW");
    #endif

  } else {
    disablePWM();
    digitalWrite(SWITCH_PIN, !RELAY_ON_LEVEL);
    fanOn = false;
    #if LOG_FAN == 1
      Serial.printf("[FAN] Boot: set to OFF (pin %d = %s)\n", SWITCH_PIN, RELAY_ON_LEVEL == HIGH ? "HIGH" : "LOW");
    #endif
  }
  
  #if LOG_FAN == 1
    Serial.printf("[FAN] Init complete: fanOn=%s, bootState=%s, RELAY_ON_LEVEL=%s\n", 
                  fanOn ? "ON" : "OFF", 
                  config_get()->bootState ? "ON" : "OFF",
                  RELAY_ON_LEVEL == LOW ? "LOW" : "HIGH");
  #endif
}

void fan_set(bool on, bool manual) {
  #if LOG_FAN == 1
    Serial.printf("[FAN] fan_set(%s, manual=%s) called, current fanOn=%s\n", 
                  on ? "ON" : "OFF", 
                  manual ? "true" : "false", 
                  fanOn ? "ON" : "OFF");
  #endif
  
  if (fanOn == on) return;
  
  // >>> ИЗМЕНЕНИЯ: чтение и запись через геттеры/сеттеры
  if (manual) {
    if (config_get()->sensorControlMode) {
      config_setSensorControlMode(false);

      #if LOG_FAN == 1
        Serial.println("[FAN] Manual control - sensor control mode disabled");
      #endif
    }
    
    if (config_get()->adaptiveMode) {
      config_setAdaptiveMode(false);
      adaptiveActive = false;

      #if LOG_FAN == 1
        Serial.println("[FAN] Manual control - adaptive mode disabled");
      #endif
    }

    if (delayActive) {
      delayActive = false;
      #if LOG_FAN == 1
        Serial.println("[FAN] Manual control - delay timer cancelled");
      #endif
    }
  }
  
  startingPulseActive = false;
  adaptiveActive = false;
  
  fanOn = on;
  
  if (fanOn) {
    fanStartTime = millis();
    
    // >>> ИЗМЕНЕНИЕ: config.speedPercent → config_get()->speedPercent
    uint16_t currentSpeed = config_get()->speedPercent;
    
    if (currentSpeed < 100 && currentSpeed > 0) {
      fan_applySpeed(100);
      startingPulseActive = true;
      startingPulseStart = millis();
      
      #if LOG_FAN == 1
        Serial.printf("[FAN] Starting pulse started, duration=%d ms\n", PWM_STARTING);
      #endif
    
    } else if (currentSpeed >= 100) {
      disablePWM();
      digitalWrite(SWITCH_PIN, RELAY_ON_LEVEL);
      
      #if LOG_FAN == 1
        Serial.printf("[FAN] FULL ON: pin=%d, level=RELAY_ON_LEVEL (forced)\n", SWITCH_PIN);
      #endif
      
      if (config_get()->adaptiveMode && sensor_isOk()) {
        adaptiveActive = true;
        baseTemp = sensor_getTemperature();
        baseHum = sensor_getHumidity();
        lastAdaptiveCheck = millis();
        
        #if LOG_FAN == 1
          Serial.printf("[FAN] Adaptive mode activated: base T=%.2f, H=%.2f\n", baseTemp, baseHum);
        #endif
      }

      #if LOG_FAN == 1
        Serial.println("[FAN] Fan turned ON");
      #endif

    } else {
      fanOn = false;
      fan_applySpeed(0);
      
      #if LOG_FAN == 1
        Serial.println("[FAN] Speed percent is 0% - cannot turn ON");
      #endif
      
      return;
    }
  } else {
    fan_applySpeed(0);
    fanStartTime = 0;
    adaptiveActive = false;
    
    // >>> ИСПРАВЛЕНИЕ ОШИБКИ: правильный синтаксис #if и объявление savedConfig
    Config savedConfig = config_getSaved();
    #if LOG_FAN == 1
    uint16_t oldSpeed = config_get()->speedPercent;
    #endif
    
    config_setSpeedPercent(savedConfig.speedPercent);
    
    #if LOG_FAN == 1
      Serial.printf("[FAN] Fan turned OFF, restored speed to %d%% (was %d%%)\n", 
                    config_get()->speedPercent, oldSpeed);
    #endif
  }
  
}

bool fan_getState() {
  return fanOn;
}

void fan_setOverrideMode(bool sensorControl) {
  // >>> ИЗМЕНЕНИЕ: использование сеттера
  config_setSensorControlMode(sensorControl);
  if (!sensorControl) {
    delayActive = false;
    adaptiveActive = false;
  } else {
    #if DEVICE_TYPE == 1
    if (fanOn && config_get()->adaptiveMode && sensor_isOk()) {
      adaptiveActive = true;
      baseTemp = sensor_getTemperature();
      baseHum = sensor_getHumidity();
      lastAdaptiveCheck = millis();
    }
    #endif
  }

  #if LOG_FAN == 1
    Serial.printf("[FAN] Mode switched to: %s\n", sensorControl ? "SENSOR CONTROL" : "MANUAL");
  #endif
}

void fan_checkMaxOnTime() {
  // >>> ИЗМЕНЕНИЕ: config.maxOnTime → config_get()->maxOnTime
  if (fanOn && fanStartTime != 0 && config_get()->maxOnTime > 0 &&
      (millis() - fanStartTime) > config_get()->maxOnTime * 1000UL) {
    
    #if LOG_FAN == 1
      Serial.println("[FAN] Max on time exceeded, forcing OFF");
    #endif
    
    #if DEVICE_TYPE == 1
      config_setSensorControlMode(false);
      #if LOG_FAN == 1
        Serial.println("[FAN] Switched to MANUAL mode after safety shutdown");
      #endif
    #endif
    
    fan_set(false);
  }
}

bool fan_delayTimer(bool start) {
  // >>> ИЗМЕНЕНИЕ: config.delaySeconds → config_get()->delaySeconds
  if (start) {
    uint16_t delaySec = config_get()->delaySeconds;
    if (delaySec > 0) {
      delayActive = true;
      delayTimer = millis() + delaySec * 1000UL;
      
      #if LOG_FAN == 1
        Serial.printf("[FAN] Delay ON timer started: %d seconds\n", delaySec);
      #endif
      return false;
    }
    delayActive = false;
    return false;
  } else {
    if (delayActive && millis() >= delayTimer) {
      delayActive = false;
      #if DEVICE_TYPE == 1
        config_setSensorControlMode(false);
        #if LOG_FAN == 1
          Serial.println("[FAN] Delay ON timer finished - switching to MANUAL mode (temporary)");
        #endif
      #elif DEVICE_TYPE == 3
        #if LOG_FAN == 1
          Serial.println("[FAN] Delay ON timer finished - turning ON");
        #endif
      #endif
      return true;
    }
    return false;
  }
}

// Расчёт шага адаптации
static int calculateAdaptiveStep(float deltaTemp, float deltaHum, float humRate) {
  int step = ADAPTIVE_STEP_SIZE;
  
  if (deltaTemp > ADAPTIVE_EPSILON_TEMP * 2 || deltaHum > ADAPTIVE_EPSILON_HUM * 2) {
    step *= 2;
  }
  if (deltaTemp > ADAPTIVE_EPSILON_TEMP * 3 || deltaHum > ADAPTIVE_EPSILON_HUM * 3) {
    step *= 3;
  }
  
  #if DEVICE_TYPE == 1
    float speedMultiplier = 1.0 + (humRate / ADAPTIVE_SPEED_SENSITIVITY);
    speedMultiplier = constrain(speedMultiplier, 0.5, 3.0);
    step = step * speedMultiplier;
  #endif
  
  return constrain(step, 5, 60);
}

void fan_adaptiveUpdate() {
  #if DEVICE_TYPE == 1
  if (!sensor_isOk()) {
    if (adaptiveActive) {
      adaptiveActive = false;
      
      #if LOG_FAN == 1
        Serial.println("[FAN] Adaptive mode disabled - sensor error");
      #endif

      if (fanOn && !startingPulseActive) {
        config_setSpeedPercent(100);
        fan_applySpeed(100);
      }
    }
    return;
  }
  
  // >>> ИЗМЕНЕНИЕ: чтение через геттеры
  if (!fanOn || startingPulseActive || !config_get()->adaptiveMode || !config_get()->sensorControlMode) {
    if (adaptiveActive) adaptiveActive = false;
    return;
  }
  
  if (!adaptiveActive) {
    adaptiveActive = true;
    baseTemp = sensor_getTemperature();
    baseHum = sensor_getHumidity();
    lastAdaptiveCheck = millis();

    #if LOG_FAN == 1
      Serial.printf("[FAN] Adaptive active: base T=%.2f, H=%.2f\n", baseTemp, baseHum);
    #endif
    return;
  }
  
  // >>> ИЗМЕНЕНИЕ: config.sensorInterval → config_get()->sensorInterval
  if (millis() - lastAdaptiveCheck < config_get()->sensorInterval * 1000UL) {
    return;
  }
  lastAdaptiveCheck = millis();
  
  float deltaTemp = sensor_getTemperature() - baseTemp;
  float deltaHum = sensor_getHumidity() - baseHum;
  
  int newSpeed = config_get()->speedPercent;
  bool needChange = false;
  
  int step = calculateAdaptiveStep(deltaTemp, deltaHum, sensor_getHumRate());
  
  if (deltaTemp > ADAPTIVE_EPSILON_TEMP || deltaHum > ADAPTIVE_EPSILON_HUM) {
    newSpeed += step;
    if (newSpeed > 100) newSpeed = 100;
    if (newSpeed != config_get()->speedPercent) {
      needChange = true;
      
      #if LOG_FAN == 1
        Serial.printf("[FAN] Adaptive step +%d%% (ΔT=%.2f ΔH=%.2f rate=%.1f) → %d%%\n", 
                      step, deltaTemp, deltaHum, sensor_getHumRate(), newSpeed);
      #endif
    }
  }
  
  if (needChange) {
    config_setSpeedPercent(newSpeed);
    fan_applySpeed(config_get()->speedPercent);
    baseTemp = sensor_getTemperature();
    baseHum = sensor_getHumidity();
  }
  #endif // DEVICE_TYPE == 1
}

void fan_update() {
  // Обработка завершения стартового импульса
  if (startingPulseActive) {
    if (millis() - startingPulseStart >= PWM_STARTING) {
      if (config_get()->speedPercent <= 0) {
        fanOn = false;
        fan_applySpeed(0);
        startingPulseActive = false;
        adaptiveActive = false;

        #if LOG_FAN == 1
          Serial.println("[FAN] Starting pulse finished, but speed is 0% - turning OFF");
        #endif
      } else {
        fan_applySpeed(config_get()->speedPercent);
        startingPulseActive = false;
        
        #if DEVICE_TYPE == 1
        if (config_get()->adaptiveMode && sensor_isOk() && config_get()->sensorControlMode) {
          adaptiveActive = true;
          baseTemp = sensor_getTemperature();
          baseHum = sensor_getHumidity();
          lastAdaptiveCheck = millis();
          
          #if LOG_FAN == 1
            Serial.printf("[FAN] Adaptive mode activated after starting pulse: base T=%.2f, H=%.2f, speed=%d%%\n", 
                          baseTemp, baseHum, config_get()->speedPercent);
          #endif
        }
        #endif
      }
      
      #if LOG_FAN == 1
        Serial.printf("[FAN] Starting pulse finished, switched to speed=%d%%\n", config_get()->speedPercent);
      #endif
    }
  }

  #if DEVICE_TYPE == 1
  // >>> ИЗМЕНЕНИЕ: чтение через геттер
  if (!config_get()->sensorControlMode) {
    fan_checkMaxOnTime();
    if (adaptiveActive) adaptiveActive = false;
    return;
  }
  #endif
  
  #if DEVICE_TYPE == 3
  fan_checkMaxOnTime();
  #endif
  
  #if DEVICE_TYPE == 1
  if (config_get()->sensorControlMode && fanOn) {
    fan_adaptiveUpdate();
  }
  #endif
  
  bool sensorShouldBeOn = false;
  bool timerExpired = false;
  
  #if DEVICE_TYPE == 1
  if (sensor_isOk()) {
    // >>> ИЗМЕНЕНИЕ: чтение порогов через геттеры
    bool tempHigh = (sensor_getTemperature() >= config_get()->highTemp);
    bool humHigh = (sensor_getHumidity() >= config_get()->highHum);
    bool tempLow = (sensor_getTemperature() <= config_get()->lowTemp);
    bool humLow = (sensor_getHumidity() <= config_get()->lowHum);
    
    if (tempHigh || humHigh) {
      sensorShouldBeOn = true;
    } else if (tempLow && humLow) {
      sensorShouldBeOn = false;
    } else {
      sensorShouldBeOn = fanOn;
    }
  } else {
    sensorShouldBeOn = fanOn;
  }
  #endif
  
  timerExpired = fan_delayTimer(false);
  
  if (sensorShouldBeOn || timerExpired) {
    if (!fanOn) {
      fan_set(true, false);  
      if (delayActive) {
        delayActive = false;
        #if LOG_FAN == 1
          Serial.println("[FAN] Timer cancelled - fan turned ON by sensor");
        #endif
      }
    }
  } else {
    if (fanOn) {
      fan_set(false, false);
      #if DEVICE_TYPE == 1
      // >>> ИЗМЕНЕНИЕ: чтение через геттер
      if (config_get()->delaySeconds > 0 && !delayActive && config_get()->sensorControlMode) {
        fan_delayTimer(true);
      }
      #elif DEVICE_TYPE == 3
      if (config_get()->delaySeconds > 0 && !delayActive) {
        fan_delayTimer(true);
      }
      #endif
    } else {
      #if DEVICE_TYPE == 1
      if (config_get()->delaySeconds > 0 && !delayActive && !sensorShouldBeOn && config_get()->sensorControlMode) {
        fan_delayTimer(true);
      }
      #elif DEVICE_TYPE == 3
      if (config_get()->delaySeconds > 0 && !delayActive && !sensorShouldBeOn) {
        fan_delayTimer(true);
      }
      #endif
    }
  }
  
  fan_checkMaxOnTime();
}

#endif // DEVICE_TYPE == 1