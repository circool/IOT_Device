```cpp
/**
 * @file STATE_PROVIDER.md
 * @brief Единый источник данных о состоянии устройства
 * @note Статус: Проектирование
 */
```

# STATE_PROVIDER.md

## Оглавление

- [1. Назначение](#1-назначение)
- [2. Архитектурное место](#2-архитектурное-место)
- [3. Структура данных](#3-структура-данных)
- [4. API](#4-api)
- [5. Интеграция со слоями](#5-интеграция-со-слоями)
- [6. Потоки данных](#6-потоки-данных)

---

## 1. Назначение

`StateProvider` — единый источник данных о текущем состоянии устройства. Все слои читают состояние из одного места и обновляют его при изменениях.

**Ответственность:**
- Хранение оперативного состояния устройства (RAM)
- Предоставление доступа к состоянию всем слоям
- Обновление состояния при изменениях (датчик, подключения, режимы)

**Границы:**
- НЕ хранит настройки (ConfigManager)
- НЕ содержит бизнес-логику
- НЕ содержит логику принятия решений

---

## 2. Архитектурное место

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              StateProvider                                  │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                         DeviceState                                 │    │
│  │                                                                     │    │
│  │  • Оперативное состояние (is_on, speed, manual_mode)                │    │
│  │  • Данные датчика (temperature, humidity, sensor_valid)             │    │
│  │  • Состояние подключений (wifi, mqtt, rssi)                         │    │
│  │  • Системные флаги (provisioning, emergency, restart)               │    │
│  │  • Время (uptime, start_time)                                       │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                                                             │
│  Методы:                                                                    │
│  • get_state() → const DeviceState*                                         │
│  • update_*() — для каждого типа данных                                     │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
          ┌─────────────────────────┼─────────────────────────┐
          │                         │                         │
          ▼                         ▼                         ▼
┌──────────────────┐    ┌─────────────────┐    ┌─────────────────────────┐
│  DeviceController│    │  Web            │    │  LED/Transport/         │
│  (пишет)         │    │  (читает)       │    │  (читает)               │
└──────────────────┘    └─────────────────┘    └─────────────────────────┘
          │                         │                         │
          ▼                         ▼                         ▼
┌─────────────────┐    ┌─────────────────┐    ┌─────────────────────────┐
│  WiFi/MQTT/     │    │  Provisioning/  │    │  Sensor/                │
│  (пишут)        │    │  Restart/       │    │  (пишет)                │
│                 │    │  (пишут)        │    │                         │
└─────────────────┘    └─────────────────┘    └─────────────────────────┘
```

---

## 3. Структура данных

### 3.1. DeviceState

```cpp
/**
 * @brief Полное состояние устройства
 */
struct DeviceState {
    // ===== ОПЕРАТИВНОЕ СОСТОЯНИЕ =====
    bool is_on;                  // Актуатор включён
    uint8_t speed;               // Скорость 0-100% (TYPE 1)
    bool manual_mode;            // Ручной режим
    bool timer_mode;             // Режим таймера
    uint32_t delay_remain_sec;   // Остаток таймера
    bool adaptive_active;        // Адаптивный режим активен

    // ===== ДАННЫЕ ДАТЧИКА =====
    float temperature;
    float humidity;
    bool sensor_valid;
    const char* sensor_error;

    // ===== ПОДКЛЮЧЕНИЯ =====
    bool wifi_connected;
    bool mqtt_connected;
    int wifi_rssi;

    // ===== СИСТЕМНЫЕ ФЛАГИ =====
    bool provisioning;
    bool emergency;
    bool restart_pending;
    bool button_pressed;
    uint8_t button_stage;        // 0=RELEASED, 1=PRESSED, 2=STAGE_1S, 3=STAGE_2S, 4=STAGE_3S

    // ===== ВРЕМЯ =====
    unsigned long uptime;
    unsigned long start_time;
};
```

---

## 4. API

### 4.1. Получение экземпляра

```cpp
StateProvider& StateProvider::getInstance();
```

### 4.2. Чтение состояния

```cpp
/**
 * @brief Получить текущее состояние
 * @return Указатель на DeviceState (только для чтения)
 */
const DeviceState* get_state() const;
```

### 4.3. Обновление состояния

```cpp
// ===== ОТ ДАТЧИКА =====
void update_sensor(float temp, float hum, bool valid, const char* error);

// ===== ОТ DEVICECONTROLLER =====
void update_operational(bool on, uint8_t speed, bool manual, bool adaptive);

// ===== ОТ ТРАНСПОРТА =====
void update_connection(bool wifi, bool mqtt, int rssi);

// ===== ОТ СИСТЕМНЫХ СЛОЁВ =====
void set_provisioning(bool active);
void set_emergency(bool active);
void set_restart(bool pending);
void set_button(bool pressed, uint8_t stage);
```

### 4.4. Контроль доступа

| Слой | Читает | Пишет |
|------|--------|-------|
| **DeviceController** | temperature, humidity, sensor_valid | is_on, speed, manual_mode, adaptive_active |
| **Sensor** | ❌ | temperature, humidity, sensor_valid, sensor_error |
| **WiFiManager** | ❌ | wifi_connected, wifi_rssi |
| **MQTTManager** | ❌ | mqtt_connected |
| **ProvisioningManager** | ❌ | provisioning |
| **RestartManager** | ❌ | restart_pending |
| **ResetButton** | ❌ | button_pressed, button_stage |
| **Web** | is_on, speed, manual_mode, temperature, humidity, wifi_connected, mqtt_connected, provisioning, emergency, restart_pending | ❌ |
| **LED** | provisioning, emergency, restart_pending, wifi_connected, mqtt_connected, button_stage | ❌ |
| **Transport** | is_on, speed, manual_mode, temperature, humidity, wifi_connected, mqtt_connected | ❌ |
| **Orchestrator** | wifi_connected, provisioning, button_pressed, button_stage, restart_pending | ❌ |

**Принципы:**
1. Каждый слой пишет ТОЛЬКО свои данные
2. Каждый слой читает ТОЛЬКО то, что ему нужно
3. Оркестратор НЕ пишет в StateProvider — он принимает решения и вызывает методы других слоёв

---

## 5. Интеграция со слоями

### 5.1. DeviceController

DeviceController получает `StateProvider*` через конструктор и работает с ним:

```cpp
class DeviceController {
public:
    void init(ConfigData* config, StateProvider* state);
    void update();  // читает из StateProvider, обновляет StateProvider
private:
    StateProvider* _state;  // только указатель, не владеет
};
```

**В `update()`:**
1. Читает `temperature`, `humidity`, `sensor_valid` из StateProvider
2. Принимает решение
3. Пишет `is_on`, `speed`, `manual_mode`, `adaptive_active` в StateProvider

### 5.2. Sensor

Sensor пишет показания в StateProvider после каждого опроса:

```cpp
void sensor_update() {
    // ... чтение датчика ...
    StateProvider::getInstance().update_sensor(temp, hum, valid, error);
}
```

### 5.3. WiFiManager

WiFiManager пишет статус подключения при изменении:

```cpp
void wifi_manager_update() {
    if (WiFi.status() == WL_CONNECTED) {
        StateProvider::getInstance().update_connection(true, ..., WiFi.RSSI());
    } else {
        StateProvider::getInstance().update_connection(false, ..., 0);
    }
}
```

### 5.4. MQTTManager

MQTTManager пишет статус подключения:

```cpp
void mqtt_manager_update() {
    bool connected = mqttClient.connected();
    StateProvider::getInstance().update_connection(..., connected, ...);
}
```

### 5.5. ProvisioningManager

ProvisioningManager пишет режим настройки:

```cpp
void ProvisioningManager::start() {
    StateProvider::getInstance().set_provisioning(true);
}

void ProvisioningManager::stop() {
    StateProvider::getInstance().set_provisioning(false);
}
```

### 5.6. RestartManager

RestartManager пишет флаг перезагрузки:

```cpp
void restart_request() {
    StateProvider::getInstance().set_restart(true);
}

void restart_update() {
    if (_pending && millis() - _requestTime >= _delayMs) {
        StateProvider::getInstance().set_restart(false);
        ESP.restart();
    }
}
```

### 5.7. ResetButton

ResetButton пишет состояние кнопки:

```cpp
void resetBtn_update() {
    // ... чтение пина и подсчёт времени ...
    StateProvider::getInstance().set_button(pressed, stage);
}
```

### 5.8. Web

Web читает состояние через `get_state()`:

```cpp
String web_build_status_html() {
    const DeviceState* state = StateProvider::getInstance().get_state();
    
    // Отображаем все поля
    // state->is_on, state->speed, state->temperature, ...
}
```

### 5.9. LED

LED читает флаги для индикации:

```cpp
void led_update() {
    const DeviceState* state = StateProvider::getInstance().get_state();
    
    if (state->restart_pending)      → LED_OFF
    else if (state->emergency)       → LED_SLOW_BLINK
    else if (state->provisioning)    → LED_MORZE_S
    else if (state->button_pressed)  → LED_MORZE_*
    else if (!state->wifi_connected) → LED_MORZE_E
    else if (!state->mqtt_connected) → LED_MORZE_I
    else                             → LED_ON
}
```

### 5.10. Transport

Transport читает состояние для публикации:

```cpp
void transport_update() {
    const DeviceState* state = StateProvider::getInstance().get_state();
    
    publish("state", state->is_on);
    publish("speed", state->speed);
    publish("temperature", state->temperature);
    publish("humidity", state->humidity);
    publish("rssi", state->wifi_rssi);
}
```

### 5.11. Orchestrator

Оркестратор читает состояние для принятия решений:

```cpp
void loop() {
    const DeviceState* state = StateProvider::getInstance().get_state();
    
    // Запуск провизионинга
    if (!state->wifi_connected && !state->provisioning) {
        provisioning_start();
    }
    
    // Реакция на кнопку
    if (state->button_pressed && state->button_stage == STAGE_3S) {
        g_configManager.reset();
        restart_request(500);
    }
}
```

---

## 6. Потоки данных

### 6.1. Датчик → StateProvider

```
1. sensor_update() вызывается в loop()
2. sensor_update() читает датчик (I2C)
3. sensor_update() вызывает StateProvider::update_sensor(temp, hum, valid, error)
4. Web/Transport при следующем опросе читают актуальные данные
```

### 6.2. DeviceController → StateProvider

```
1. DeviceController::update() вызывается в loop()
2. DeviceController читает из StateProvider (temperature, humidity)
3. DeviceController принимает решение
4. DeviceController вызывает StateProvider::update_operational(on, speed, manual, adaptive)
5. Web/Transport/LED при следующем опросе читают актуальное состояние
```

### 6.3. WiFiManager → StateProvider

```
1. wifi_manager_update() вызывается в loop()
2. WiFiManager проверяет WiFi.status()
3. WiFiManager вызывает StateProvider::update_connection(wifi_connected, ..., rssi)
4. Web/LED при следующем опросе читают актуальное состояние
```

### 6.4. Web → StateProvider

```
1. web_send_status_page() вызывается при GET /
2. Web вызывает StateProvider::get_state()
3. Web отображает все поля состояния
```

---

*Конец документа*