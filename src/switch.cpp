#if DEVICE_TYPE == 3
#include "Arduino.h"
#include "config.h"
#include "switch.h"

// Глобальные переменные
bool switchOn = false;
unsigned long switchStartTime = 0;
unsigned long delayTimer = 0;
bool delayActive = false;

void switch_init() {
    pinMode(SWITCH_PIN, OUTPUT);
    
    delayActive = false;
    delayTimer = 0;
    switchStartTime = 0;
    
    // >>> ИЗМЕНЕНИЕ: config.bootState → config_get()->bootState
    if (config_get()->bootState) {
        digitalWrite(SWITCH_PIN, RELAY_ON_LEVEL);
        switchOn = true;
        
        #if LOG_SWITCH == 1
            Serial.printf("[SWITCH] Boot: set to ON (pin %d = %s)\n", 
                          SWITCH_PIN, RELAY_ON_LEVEL == HIGH ? "HIGH" : "LOW");
        #endif
    } else {
        digitalWrite(SWITCH_PIN, !RELAY_ON_LEVEL);
        switchOn = false;
        
        #if LOG_SWITCH == 1
            Serial.printf("[SWITCH] Boot: set to OFF (pin %d = %s)\n", 
                          SWITCH_PIN, RELAY_ON_LEVEL == HIGH ? "LOW" : "HIGH");
        #endif
    }
    
       
    #if LOG_SWITCH == 1
        Serial.printf("[SWITCH] Init complete: switchOn=%s, bootState=%s\n", 
                      switchOn ? "ON" : "OFF", 
                      config_get()->bootState ? "ON" : "OFF");
    #endif
}

void switch_set(bool on, bool manual) {
    #if LOG_SWITCH == 1
        Serial.printf("[SWITCH] switch_set(%s, manual=%s), current=%s\n", 
                      on ? "ON" : "OFF", 
                      manual ? "true" : "false", 
                      switchOn ? "ON" : "OFF");
    #endif
    
    if (switchOn == on) return;
    
    // Ручная команда — отключаем таймер отложенного включения
    if (manual && delayActive) {
        delayActive = false;
        #if LOG_SWITCH == 1
            Serial.println("[SWITCH] Manual control - delay timer cancelled");
        #endif
    }
    
    switchOn = on;
    
    if (switchOn) {
        // Включение
        digitalWrite(SWITCH_PIN, RELAY_ON_LEVEL);
        switchStartTime = millis();
        
        #if LOG_SWITCH == 1
            Serial.println("[SWITCH] Turned ON");
        #endif
    } else {
        // Выключение
        digitalWrite(SWITCH_PIN, !RELAY_ON_LEVEL);
        switchStartTime = 0;
        
        #if LOG_SWITCH == 1
            Serial.println("[SWITCH] Turned OFF");
        #endif
    }
    
}

bool switch_getState() {
    return switchOn;
}

void switch_checkMaxOnTime() {
    if (!switchOn) return;
    // >>> ИЗМЕНЕНИЕ: config.maxOnTime → config_get()->maxOnTime
    if (config_get()->maxOnTime == 0) return;
    if (switchStartTime == 0) return;
    
    if ((millis() - switchStartTime) > config_get()->maxOnTime * 1000UL) {
        #if LOG_SWITCH == 1
            Serial.println("[SWITCH] Max on time exceeded, forcing OFF");
        #endif
        
        // Принудительное выключение (ручное, чтобы отключить таймер)
        switch_set(false, true);
    }
}

bool switch_delayTimer(bool start) {
    if (start) {
        // Запуск таймера отложенного включения
        // >>> ИЗМЕНЕНИЕ: config.delaySeconds → config_get()->delaySeconds
        if (config_get()->delaySeconds > 0 && !delayActive && !switchOn) {
            delayActive = true;
            delayTimer = millis() + config_get()->delaySeconds * 1000UL;
            
            #if LOG_SWITCH == 1
                Serial.printf("[SWITCH] Delay timer started: %d seconds\n", config_get()->delaySeconds);
            #endif
        }
        return false;
    } else {
        // Проверка таймера
        if (delayActive && millis() >= delayTimer) {
            delayActive = false;
            
            #if LOG_SWITCH == 1
                Serial.println("[SWITCH] Delay timer expired - turning ON");
            #endif
            return true;  // Таймер сработал — нужно включить
        }
        return false;
    }
}

void switch_update() {
    // 1. Проверка аварийного отключения
    switch_checkMaxOnTime();
    
    // 2. Проверка таймера отложенного включения
    bool timerExpired = switch_delayTimer(false);
    
    // 3. Управление по таймеру
    if (timerExpired && !switchOn) {
        // Включаем без ручного флага (чтобы не отменять таймер повторно)
        switch_set(true, false);
    }
    
    // 4. Запуск таймера при необходимости
    // >>> ИЗМЕНЕНИЕ: config.delaySeconds → config_get()->delaySeconds
    if (!switchOn && !delayActive && config_get()->delaySeconds > 0) {
        switch_delayTimer(true);
    }
}

#endif // DEVICE_TYPE == 3