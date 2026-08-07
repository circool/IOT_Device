```cpp
/**
 * @file REFERENCE_WIFI_IMPLEMENTATION.md
 * @brief Пример реализации WiFi-транспорта (reference implementation)
 * @note Статус: Закончен
 * @version 0.11
 * @date 06.08.2026
 */
```

# REFERENCE_WIFI_IMPLEMENTATION.md

## Оглавление

- [1. Назначение](#1-назначение)
- [2. Структура WiFiTransport](#2-структура-wifitransport)
- [3. begin() — инициализация](#3-begin--инициализация)
- [4. update() — периодическая обработка](#4-update--периодическая-обработка)
- [5. setMode() — управление режимами](#5-setmode--управление-режимами)
- [6. Публикация данных](#6-публикация-данных)
- [7. Обработка команд](#7-обработка-команд)

---

## 1. Назначение

Документ показывает **реальный код** WiFi-транспорта с комментариями, объясняющими архитектурные решения.

**Цель:** Дать разработчикам готовый пример, на который можно опираться при реализации других транспортов.

---

## 2. Структура WiFiTransport

```cpp
// transport_wifi.h

#ifndef TRANSPORT_WIFI_H
#define TRANSPORT_WIFI_H

#include "transport.h"
#include "transport_types.h"

#if USE_HTTP == 1
    #include "transport_http.h"
#endif

#if USE_MQTT == 1
    #include "transport_mqtt.h"
#endif

#if USE_BLE_SETUP == 1
    #include "transport_ble.h"
#endif

typedef enum {
    STATE_INIT,
    STATE_NORMAL,
    STATE_SETUP_MODE,
    STATE_CONNECTING,
    STATE_RECONNECTING
} WifiState;

typedef struct WiFiTransport {
    // ===== СОСТОЯНИЕ =====
    bool initialized;
    WifiState state;
    const TransportConfig* config;
    int reconnectAttempts;
    unsigned long stateStartTime;
    
    // ===== ПРОТОКОЛЫ (экземпляры) =====
    #if USE_HTTP == 1
        HttpProtocol http;
    #endif
    
    #if USE_MQTT == 1
        MqttProtocol mqtt;
    #endif
    
    #if USE_BLE_SETUP == 1
        BleServer ble;
    #endif
    
    // ===== КОЛБЭКИ =====
    TransportCommandCallback commandCallback;
    TransportConfigCallback configCallback;
} WiFiTransport;

// ===== ПУБЛИЧНАЯ ФУНКЦИЯ =====
Transport* getWiFiTransport();

#endif  // TRANSPORT_WIFI_H
```

---

## 3. begin() — инициализация

```cpp
// transport_wifi.cpp

static bool transport_begin(const TransportConfig* config) {
    if (!config) {
        XLOG_ERROR(CAT_WIFI, "config is NULL");
        return false;
    }
    
    g_wifiTransport.config = config;
    g_wifiTransport.initialized = true;
    g_wifiTransport.reconnectAttempts = 0;
    g_wifiTransport.stateStartTime = millis();
    
    // ===== ИНИЦИАЛИЗАЦИЯ ПРОТОКОЛОВ =====
    #if USE_HTTP == 1
        g_wifiTransport.http.init();
    #endif
    
    #if USE_MQTT == 1
        g_wifiTransport.mqtt.init();
    #endif
    
    #if USE_BLE_SETUP == 1
        g_wifiTransport.ble.init();
    #endif
    
    // ===== ПРОВЕРКА КОНФИГУРАЦИИ =====
    if (strlen(config->wifiSsid) == 0) {
        // Нет SSID — сразу в режим настройки
        XLOG_INFO(CAT_WIFI, "No SSID, entering setup mode");
        setMode(true);
        return true;
    }
    
    // ===== ПОПЫТКА ПОДКЛЮЧЕНИЯ =====
    XLOG_INFO(CAT_WIFI, "Connecting to SSID: %s", config->wifiSsid);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(config->wifiSsid, config->wifiPassword);
    g_wifiTransport.state = STATE_CONNECTING;
    g_wifiTransport.stateStartTime = millis();
    
    return true;
}
```

---

## 4. update() — периодическая обработка

```cpp
static void transport_update() {
    if (!g_wifiTransport.initialized) {
        return;
    }
    
    // ===== ПЕРИОДИЧЕСКАЯ ОБРАБОТКА ПРОТОКОЛОВ =====
    #if USE_HTTP == 1
        g_wifiTransport.http.update();
    #endif
    
    #if USE_MQTT == 1
        g_wifiTransport.mqtt.update();
    #endif
    
    #if USE_BLE_SETUP == 1
        g_wifiTransport.ble.update();
    #endif
    
    // ===== ОБРАБОТКА СОСТОЯНИЙ =====
    switch (g_wifiTransport.state) {
        case STATE_SETUP_MODE:
            handleSetupMode();
            break;
            
        case STATE_NORMAL:
            handleNormalMode();
            break;
            
        case STATE_CONNECTING:
            handleConnecting();
            break;
            
        case STATE_RECONNECTING:
            handleReconnecting();
            break;
            
        default:
            break;
    }
    
    // ===== ОБНОВЛЕНИЕ ФЛАГОВ =====
    updateStateProvider();
}

// ----- ОБРАБОТЧИКИ СОСТОЯНИЙ -----

static void handleConnecting() {
    if (WiFi.status() == WL_CONNECTED) {
        XLOG_INFO(CAT_WIFI, "Connected to WiFi");
        setMode(false);
        return;
    }
    
    // Таймаут подключения
    if (millis() - g_wifiTransport.stateStartTime > WIFI_CONNECT_TIMEOUT_MS) {
        XLOG_WARN(CAT_WIFI, "Connection timeout, entering setup mode");
        setMode(true);
    }
}

static void handleNormalMode() {
    // Поддерживаем соединение
    if (WiFi.status() != WL_CONNECTED) {
        XLOG_WARN(CAT_WIFI, "WiFi lost, reconnecting...");
        g_wifiTransport.state = STATE_RECONNECTING;
        g_wifiTransport.reconnectAttempts = 0;
        g_wifiTransport.stateStartTime = millis();
        return;
    }
    
    // Обновляем RSSI
    static int lastRssi = 0;
    int rssi = WiFi.RSSI();
    if (rssi != lastRssi) {
        lastRssi = rssi;
        StateProvider::getInstance().set_link_quality(rssi);
    }
}

static void handleReconnecting() {
    if (WiFi.status() == WL_CONNECTED) {
        XLOG_INFO(CAT_WIFI, "Reconnected to WiFi");
        setMode(false);
        return;
    }
    
    // Попытки переподключения
    if (++g_wifiTransport.reconnectAttempts > MAX_RECONNECT_ATTEMPTS) {
        XLOG_WARN(CAT_WIFI, "Max reconnect attempts reached, entering setup mode");
        setMode(true);
        return;
    }
    
    // Задержка между попытками
    if (millis() - g_wifiTransport.stateStartTime > WIFI_RECONNECT_DELAY_MS) {
        g_wifiTransport.stateStartTime = millis();
        WiFi.begin(g_wifiTransport.config->wifiSsid, 
                   g_wifiTransport.config->wifiPassword);
    }
}

static void handleSetupMode() {
    // Обработка настройки
    #if USE_AP_SETUP == 1
        // AP активен, HTTP показывает страницу ввода
    #endif
    
    #if USE_BLE_SETUP == 1
        // BLE активен
    #endif
}
```

---

## 5. setMode() — управление режимами

```cpp
// ============================================================
// УПРАВЛЕНИЕ РЕЖИМАМИ (единый метод)
// ============================================================

static void setMode(bool setupMode) {
    if (setupMode) {
        // ===== РЕЖИМ НАСТРОЙКИ (AP) =====
        XLOG_INFO(CAT_WIFI, "Switching to SETUP mode (AP)");
        
        // 1. Переключаем WiFi в AP
        WiFi.mode(WIFI_AP);
        WiFi.softAP(g_wifiTransport.config->deviceId);
        
        // 2. Управляем протоколами
        #if USE_HTTP == 1
            g_wifiTransport.http.setMode(HTTP_MODE_SETUP);
        #endif
        
        #if USE_MQTT == 1
            g_wifiTransport.mqtt.setEnabled(false);
        #endif
        
        #if USE_BLE_SETUP == 1
            g_wifiTransport.ble.start(g_wifiTransport.config->deviceId);
        #endif
        
        // 3. Обновляем состояние и флаги
        g_wifiTransport.state = STATE_SETUP_MODE;
        g_wifiTransport.connected = false;
        StateProvider::getInstance().set_setup_mode(true);
        StateProvider::getInstance().set_link_ok(false);
        
    } else {
        // ===== ОБЫЧНЫЙ РЕЖИМ (STA) =====
        XLOG_INFO(CAT_WIFI, "Switching to NORMAL mode (STA)");
        
        // 1. Переключаем WiFi в STA
        WiFi.mode(WIFI_STA);
        WiFi.begin(g_wifiTransport.config->wifiSsid, 
                   g_wifiTransport.config->wifiPassword);
        
        // 2. Управляем протоколами
        #if USE_HTTP == 1
            g_wifiTransport.http.setMode(HTTP_MODE_NORMAL);
        #endif
        
        #if USE_MQTT == 1
            g_wifiTransport.mqtt.setEnabled(true);
            g_wifiTransport.mqtt.begin(g_wifiTransport.config);
        #endif
        
        #if USE_BLE_SETUP == 1
            g_wifiTransport.ble.stop();
        #endif
        
        // 3. Обновляем состояние и флаги
        g_wifiTransport.state = STATE_CONNECTING;
        g_wifiTransport.reconnectAttempts = 0;
        g_wifiTransport.stateStartTime = millis();
        StateProvider::getInstance().set_setup_mode(false);
        // link_ok будет установлен после успешного подключения
    }
}
```

---

## 6. Публикация данных

```cpp
static void transport_publishState(const DeviceState* state) {
    if (!g_wifiTransport.connected) {
        return;
    }
    
    XLOG_DEBUG(CAT_WIFI, "publishState() called");
    
    #if USE_HTTP == 1
        g_wifiTransport.http.updateState(state);
    #endif
    
    #if USE_MQTT == 1
        g_wifiTransport.mqtt.updateState(state);
    #endif
}

static void transport_publishSettings(const DeviceSettings* settings) {
    if (!g_wifiTransport.connected) {
        return;
    }
    
    XLOG_DEBUG(CAT_WIFI, "publishSettings() called");
    
    #if USE_HTTP == 1
        g_wifiTransport.http.updateSettings(settings);
    #endif
    
    #if USE_MQTT == 1
        g_wifiTransport.mqtt.updateSettings(settings);
    #endif
}
```

---

## 7. Обработка команд

```cpp
// Входящая команда от протокола (HTTP/MQTT)
static void onProtocolCommand(TransportCommand cmd, const TransportCommandData* data) {
    if (g_wifiTransport.commandCallback) {
        g_wifiTransport.commandCallback(cmd, data);
    }
}

// Изменение конфигурации от протокола (HTTP)
static void onProtocolConfigUpdate(const TransportConfig* config) {
    if (g_wifiTransport.configCallback) {
        g_wifiTransport.configCallback(config);
    }
}
```

---

## 8. Сборка структуры Transport

```cpp
// ===== ИНИЦИАЛИЗАЦИЯ СТРУКТУРЫ TRANSPORT =====

static Transport g_transportImpl = {
    .begin = transport_begin,
    .update = transport_update,
    .publishState = transport_publishState,
    .publishSettings = transport_publishSettings,
    .onCommand = transport_onCommand,
    .onConfigUpdate = transport_onConfigUpdate,
};

// ===== ПУБЛИЧНАЯ ФУНКЦИЯ =====

Transport* getWiFiTransport() {
    return &g_transportImpl;
}
```

---

*Конец документа*