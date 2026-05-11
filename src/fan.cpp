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
    #ifdef ESP32
      ledcDetachPin(SWITCH_PIN);
    #endif
    digitalWrite(SWITCH_PIN, LOW);
    startingPulseActive = false;
    #ifdef DEBUG_ENABLE
      Serial.println("[FAN] Force OFF on boot - forcing OFF");
    #endif
  } else {
    // Сохраняем состояние пина, но для гарантии запуска в slow mode делаем стартовый импульс,
    // даже если пин был HIGH (чтобы раскрутить мотор после перезагрузки)
    fanOn = currentPinState;
    if (fanOn) {
      if (config.slowModeEnabled) {
        // Запускаем стартовый импульс: полная мощность на PWM_STARTING мс
        #ifdef ESP32
          ledcDetachPin(SWITCH_PIN);
        #endif
        digitalWrite(SWITCH_PIN, HIGH);
        startingPulseActive = true;
        startingPulseStart = millis();
        #ifdef DEBUG_ENABLE
          Serial.printf("[FAN] Keep state ON with slow mode - starting pulse for %d ms\n", PWM_STARTING);
        #endif
      } else {
        digitalWrite(SWITCH_PIN, HIGH);
      }
    } else {
      startingPulseActive = false;
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
  if (!config.automaticMode) {
    #ifdef DEBUG_ENABLE
      Serial.println("[FAN] Starting in MANUAL mode");
    #endif
  }
  #endif
  
  // Запуск таймера отложенного включения при старте
  if (config.delaySeconds > 0 && !fanOn 
      #if DEVICE_TYPE == 1
      && config.automaticMode
      #endif
     ) {
    fan_delayTimer(true);
    #ifdef DEBUG_ENABLE
      Serial.printf("[FAN] Initial delay timer started: %d seconds\n", config.delaySeconds);
    #endif
  }
}

void fan_set(bool on) {
  if (fanOn == on) return;
  
  // При изменении состояния сбрасываем стартовый импульс
  startingPulseActive = false;
  
  fanOn = on;
  
  if (fanOn) {
    fanStartTime = millis();
    if (config.slowModeEnabled) {
      // Начинаем стартовый импульс полной мощности
      #ifdef ESP32
        ledcDetachPin(SWITCH_PIN);
      #endif
      digitalWrite(SWITCH_PIN, HIGH);
      startingPulseActive = true;
      startingPulseStart = millis();
      #ifdef DEBUG_ENABLE
        Serial.printf("[FAN] Starting pulse started, duration=%d ms\n", PWM_STARTING);
      #endif
    } else {
      // Обычное включение
      #ifdef ESP32
        ledcDetachPin(SWITCH_PIN);
        digitalWrite(SWITCH_PIN, HIGH);
      #elif defined(ESP8266)
        digitalWrite(SWITCH_PIN, HIGH);
      #endif
      #ifdef DEBUG_ENABLE
        Serial.println("[FAN] Fan turned ON (full power)");
      #endif
    }
  } else {
    // Выключение
    #ifdef ESP32
      ledcDetachPin(SWITCH_PIN);
    #endif
    #ifdef ESP8266
      analogWrite(SWITCH_PIN, 0);
    #endif
    digitalWrite(SWITCH_PIN, LOW);
    fanStartTime = 0;
    #ifdef DEBUG_ENABLE
      Serial.println("[FAN] Fan turned OFF");
    #endif
  }
}

bool fan_getState() {
  return fanOn;
}

bool fan_getRealState() {
  // Реальная логическая единица, даже если мы в стартовом импульсе
  return fanOn;
}

void fan_setOverrideMode(bool automatic) {
  #if DEVICE_TYPE == 1
  config.automaticMode = automatic;
  if (!automatic) {
    delayActive = false;
  }
  config_write();
  #ifdef DEBUG_ENABLE
    Serial.printf("[FAN] Mode switched to: %s\n", automatic ? "AUTO" : "MANUAL");
  #endif
  #endif
}

void fan_checkMaxOnTime() {
  if (fanOn && fanStartTime != 0 && config.maxOnTime > 0 &&
      (millis() - fanStartTime) > config.maxOnTime * 1000UL) {
    #ifdef DEBUG_ENABLE
      Serial.println("[FAN] Max on time exceeded, forcing OFF");
    #endif
    fan_set(false);
    
    #if DEVICE_TYPE == 1
    config.automaticMode = false;
    config_write();
    #ifdef DEBUG_ENABLE
      Serial.println("[FAN] Switched to MANUAL mode after safety shutdown");
    #endif
    #endif
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
      config.automaticMode = false;
      #ifdef DEBUG_ENABLE
        Serial.println("[FAN] Delay ON timer finished - switching to MANUAL mode (temporary)");
      #endif
      #else
      #ifdef DEBUG_ENABLE
        Serial.println("[FAN] Delay ON timer finished - turning ON");
      #endif
      #endif
      return true;
    }
    return false;
  }
}

void fan_update() {
  // Обработка завершения стартового импульса
  if (startingPulseActive) {
    if (millis() - startingPulseStart >= PWM_STARTING) {
      // Переключаемся на ШИМ с заданной скважностью
      #ifdef ESP32
        ledcAttachPin(SWITCH_PIN, 0);
        ledcWrite(0, config.slowModeDuty);
      #elif defined(ESP8266)
        analogWrite(SWITCH_PIN, config.slowModeDuty);
      #endif
      startingPulseActive = false;
      #ifdef DEBUG_ENABLE
        Serial.printf("[FAN] Starting pulse finished, switched to PWM duty=%d\n", config.slowModeDuty);
      #endif
    }
    // Пока идёт стартовый импульс, остальная логика управления не прерывается,
    // но изменение состояния (выключение) сбросило бы startingPulseActive.
  }

  #if DEVICE_TYPE == 1
  // Ручной режим - только проверка maxOnTime
  if (!config.automaticMode) {
    fan_checkMaxOnTime();
    return;
  }
  #endif
  
  // Автоматический режим
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
  }
  #endif
  
  timerExpired = fan_delayTimer(false);
  
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
      if (config.delaySeconds > 0 && !delayActive) {
        fan_delayTimer(true);
      }
    } else {
      if (config.delaySeconds > 0 && !delayActive && !sensorShouldBeOn) {
        fan_delayTimer(true);
      }
    }
  }
  
  fan_checkMaxOnTime();
}

#endif // DEVICE_TYPE == 1 || DEVICE_TYPE == 3