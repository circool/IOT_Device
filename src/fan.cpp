#include "fan.h"

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3

#include "config.h"
#include "sensor.h"
#include "mqtt.h"

bool manualOverride = false;
bool fanOn = false;
unsigned long fanStartTime = 0;
unsigned long delayTimer = 0;
bool delayActive = false;

void fan_init() {
  pinMode(SWITCH_PIN, OUTPUT);
  
  #ifdef ESP32
    ledcSetup(0, PWM_FREQUENCY, PWM_RESOLUTION);
  #elif defined(ESP8266)
    analogWriteFreq(PWM_FREQUENCY);
    analogWriteRange(255);
  #endif
  
  bool currentPinState = (digitalRead(SWITCH_PIN) == HIGH);
  
  if (config.forceOffOnBoot) {
    fanOn = false;
    digitalWrite(SWITCH_PIN, LOW);
    #ifdef DEBUG_ENABLE
      Serial.println("[FAN] Force OFF on boot - forcing OFF");
    #endif
  } else {
    fanOn = currentPinState;
    #ifdef DEBUG_ENABLE
      Serial.printf("[FAN] Keep state on boot - synced with pin state: %s\n", 
                    fanOn ? "ON" : "OFF");
    #endif
  }
  
  manualOverride = false;
  delayActive = false;
  delayTimer = 0;
  fanStartTime = 0;
  
  // Запуск таймера отложенного включения при старте
  // Игнорируется, если вентилятор уже включён (forceOffOnBoot = false и пин был HIGH)
  if (config.delaySeconds > 0 && !fanOn && !manualOverride) {
    fan_delayTimer(true);
    #ifdef DEBUG_ENABLE
      Serial.printf("[FAN] Initial delay timer started: %d seconds\n", config.delaySeconds);
    #endif
  }
}

void fan_set(bool on) {
  if (fanOn == on) return;
  
  fanOn = on;
  
  if (fanOn) {
    fanStartTime = millis();
  } else {
    fanStartTime = 0;
  }
  
  if (on) {
    if (config.slowModeEnabled) {
      #ifdef ESP32
        ledcAttachPin(SWITCH_PIN, 0);
        ledcWrite(0, config.slowModeDuty);
      #elif defined(ESP8266)
        analogWrite(SWITCH_PIN, config.slowModeDuty);
      #endif
    } else {
      #ifdef ESP32
        ledcDetachPin(SWITCH_PIN);
        digitalWrite(SWITCH_PIN, HIGH);
      #elif defined(ESP8266)
        digitalWrite(SWITCH_PIN, HIGH);
      #endif
    }
  } else {
    #ifdef ESP32
      ledcDetachPin(SWITCH_PIN);
      digitalWrite(SWITCH_PIN, LOW);
    #elif defined(ESP8266)
      digitalWrite(SWITCH_PIN, LOW);
    #endif
  }
  
  #ifdef DEBUG_ENABLE
    delay(10);
    bool pinState = (digitalRead(SWITCH_PIN) == HIGH);
    if (!config.slowModeEnabled && pinState != on) {
      Serial.printf("[FAN] WARNING: Pin state mismatch! Expected: %s, Real: %s\n", 
                    on ? "ON" : "OFF", pinState ? "ON" : "OFF");
    }
    Serial.println(on ? "[FAN] Fan turned ON" : "[FAN] Fan turned OFF");
  #endif
}

bool fan_getState() {
  return fanOn;
}

bool fan_getRealState() {
  return fanOn;
}

void fan_setOverrideMode(bool override) {
  if (override) {
    manualOverride = true;
    delayActive = false;
    #ifdef DEBUG_ENABLE
      Serial.println("[FAN] Manual override mode ON");
    #endif
  } else {
    manualOverride = false;
    #ifdef DEBUG_ENABLE
      Serial.println("[FAN] Manual override mode OFF (auto mode)");
    #endif
  }
}

void fan_checkMaxOnTime() {
  if (fanOn && fanStartTime != 0 && config.maxOnTime > 0 &&
      (millis() - fanStartTime) > config.maxOnTime * 1000UL) {
    #ifdef DEBUG_ENABLE
      Serial.println("[FAN] Max on time exceeded, forcing OFF");
    #endif
    fan_set(false);
    // Таймер отложенного включения запускается после принудительного выключения
    if (config.delaySeconds > 0 && !manualOverride) {
      fan_delayTimer(true);
    }
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
      manualOverride = true;
      #ifdef DEBUG_ENABLE
        Serial.println("[FAN] Delay ON timer finished - turning ON");
      #endif
      return true;
    }
    return false;
  }
}

void fan_update() {
  // Ручной режим - наивысший приоритет
  if (manualOverride) {
    return;
  }
  
  bool sensorShouldBeOn = false;
  bool timerExpired = false;
  
  // 1. Проверка датчиков (автоматический режим) - только для TYPE 1
  #if DEVICE_TYPE == 1
  if (config.automaticMode && sensor_isOk()) {
    bool tempHigh = (currentTemp >= config.highTemp);
    bool humHigh = (currentHum >= config.highHum);
    bool tempLow = (currentTemp <= config.lowTemp);
    bool humLow = (currentHum <= config.lowHum);
    
    if (tempHigh || humHigh) {
      sensorShouldBeOn = true;
    } else if (tempLow && humLow) {
      sensorShouldBeOn = false;
    } else {
      // Гистерезис - сохраняем текущее состояние
      sensorShouldBeOn = fanOn;
    }
  }
  #endif
  
  // 2. Проверка таймера отложенного включения
  timerExpired = fan_delayTimer(false);
  
  // 3. Логика ИЛИ - если датчик хочет включить ИЛИ таймер истёк
  if (sensorShouldBeOn || timerExpired) {
    if (!fanOn) {
      fan_set(true);
      // Если включились по таймеру или датчику - отменяем таймер
      if (delayActive) {
        delayActive = false;
        #ifdef DEBUG_ENABLE
          Serial.println("[FAN] Timer cancelled - fan turned ON by sensor");
        #endif
      }
    }
  } else {
    // Ни датчик, ни таймер не требуют включения
    if (fanOn) {
      fan_set(false);
      // Запускаем таймер после выключения (если не активен)
      if (config.delaySeconds > 0 && !delayActive && !manualOverride) {
        fan_delayTimer(true);
      }
    } else {
      // Устройство выключено - проверяем нужно ли запустить таймер
      if (config.delaySeconds > 0 && !delayActive && !manualOverride && !sensorShouldBeOn) {
        fan_delayTimer(true);
      }
    }
  }
  
  // 4. Контроль максимального времени работы (работает всегда, даже в ручном режиме)
  fan_checkMaxOnTime();
}

#endif // DEVICE_TYPE == 1 || DEVICE_TYPE == 3