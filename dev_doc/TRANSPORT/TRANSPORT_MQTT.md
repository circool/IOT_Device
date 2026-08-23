```cpp
/**
 * @file TRANSPORT_MQTT.md
 * @brief MQTT-протокол — прикладной протокол WiFi-транспорта
 * @note Статус: Рефакторинг
 * @version 0.11
 * @date 06.08.2026
 */
```

# TRANSPORT_MQTT.md

## Оглавление

- [1. Назначение](#1-назначение)
- [2. Архитектурное место](#2-архитектурное-место)
- [3. Принципы](#3-принципы)
- [4. Режимы работы](#4-режимы-работы)
- [5. Интерфейс](#5-интерфейс)
- [6. Получение данных](#6-получение-данных)
- [7. Формирование топиков](#7-формирование-топиков)
- [8. Публикация](#8-публикация)
- [9. Подписка и команды](#9-подписка-и-команды)
- [10. Переподключение](#10-переподключение)
- [11. LWT (Last Will and Testament)](#11-lwt-last-will-and-testament)
- [12. Флаги компиляции](#12-флаги-компиляции)
- [13. Связь с документацией](#13-связь-с-документацией)

---

## 1. Назначение

`TRANSPORT_MQTT` — прикладной протокол WiFi-транспорта, обеспечивающий:
- Асинхронную публикацию состояния устройства в MQTT-брокер
- Приём команд управления от систем умного дома
- Публикацию настроек и показаний датчиков

**MQTT-протокол является частью WiFi-транспорта и работает только в обычном режиме (STA).**

**MQTT управляет флагом `gateway_ok` в `StateProvider`:** он единственный знает, есть ли соединение с брокером.

---

## 2. Архитектурное место

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              ВНЕШНИЙ МИР                                    │
│                                                                             │
│  • Системы умного дома (Home Assistant, OpenHAB и т.д.)                     │
│  • MQTT-брокер (Mosquitto, EMQX и т.д.)                                     │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ MQTT (асинхронный: публикация/подписка)
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                              MQTT-ПРОТОКОЛ                                  │
│                                                                             │
│  • Прикладной протокол WiFi-транспорта                                      │
│  • Работает поверх WiFi (только в STA)                                      │
│  • Получает данные через publishState() / publishSettings()                 │
│  • Публикует данные в топики                                                │
│  • Принимает команды через подписку                                         │
│  • Управляет флагом gateway_ok в StateProvider                              │
│                                                                             │
│  НЕ УПРАВЛЯЕТ ДРУГИМИ ФЛАГАМИ:                                              │
│  • link_ok — устанавливается WiFi-транспортом                               │
│  • setup_mode — устанавливается WiFi-транспортом                            │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ Управляется WiFi-транспортом
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                              WiFi-ТРАНСПОРТ                                 │
│                                                                             │
│  • Управляет MQTT-протоколом через публичный интерфейс                      │
│  • Включает/выключает MQTT при переключении режимов                         │
│  • Передаёт данные через publishState()                                     │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Принципы

| Принцип | Описание |
|---------|----------|
| **Единый контракт** | MQTT получает данные через `publishState()` и `publishSettings()` — как и HTTP |
| **Асинхронность** | MQTT — асинхронный протокол. Данные публикуются по событию, команды приходят через подписку |
| **Управление gateway_ok** | MQTT-протокол единственный, кто устанавливает флаг `gateway_ok` |
| **Пассивность команд** | MQTT-протокол не знает о бизнес-логике — команды передаются через `onCommand()` |
| **Автоматическое переподключение** | MQTT-протокол сам восстанавливает соединение при потере связи |

---

## 4. Режимы работы

| Режим | Состояние MQTT | Назначение | Когда активен |
|-------|----------------|------------|---------------|
| **NORMAL** | Активен (подключён/переподключается) | Публикация и приём команд | Обычный режим WiFi-транспорта (STA) |
| **SETUP** | Приостановлен | Не работает | Режим настройки WiFi-транспорта (AP) |

### 4.1. Режим NORMAL

- MQTT-клиент активен
- Пытается подключиться к брокеру
- При подключении — подписывается на команды
- Публикует состояние при изменениях
- При потере связи — автоматически переподключается

### 4.2. Режим SETUP

- MQTT-клиент приостановлен
- Не пытается подключиться
- Не публикует данные
- Не принимает команды

### 4.3. Переключение режимов

Режимы переключаются **WiFi-транспортом** через публичный интерфейс:

```cpp
// WiFi-транспорт управляет MQTT-протоколом
_mqtt.setEnabled(false);  // Остановить MQTT (режим настройки)
_mqtt.setEnabled(true);   // Запустить MQTT (обычный режим)
```

---

## 5. Интерфейс

MQTT-протокол предоставляет WiFi-транспорту публичный интерфейс для управления:

### 5.1. Управление жизненным циклом

| Метод | Назначение | Вызов |
|-------|------------|-------|
| `init()` | Инициализация MQTT-клиента | Один раз в `setup()` |
| `begin(config)` | Запуск MQTT-клиента с конфигурацией | После инициализации |
| `update()` | Периодическая обработка (поддержание соединения) | В `loop()` |

### 5.2. Управление режимами

| Метод | Назначение |
|-------|------------|
| `set_mode(mode)` | Переключить режим (ENABLED / DISABLED) |

### 5.3. Получение данных от транспорта

| Метод | Назначение |
|-------|------------|
| `update_state(state)` | Опубликовать состояние |
| `update_settings(settings)` | Опубликовать настройки |

---

## 6. Получение данных

MQTT-протокол получает данные через единый механизм публикации транспортной абстракции.

```cpp
// MQTT-протокол получает данные через publishState()
void MqttProtocol::publishState(const DeviceState* state) {
    if (!_enabled || !_connected) return;
    
    publish("state", state->is_on ? "ON" : "OFF");
    publish("speed", String(state->speed));
    publish("temperature", String(state->temperature));
    publish("humidity", String(state->humidity));
    publish("sensorMode", state->sensorMode ? "SENSOR" : "MANUAL");
    publish("adaptiveMode", state->adaptiveMode ? "ON" : "OFF");
    publish("delay_remain", String(state->delay_remain));
    publish("max_on_remain", String(state->max_on_remain));
    
    // CHECK ENGINE: публикуем только если есть причина
    if (state->reset_reason && strlen(state->reset_reason) > 0) {
        publish("lastReset", state->reset_reason);
    }
}

void MqttProtocol::publishSettings(const DeviceConfig* settings) {
    if (!_enabled || !_connected) return;
    
    // Настройки — {clientId}/config/{параметр}
    publish("config/lowTemp", String(settings->lowTemp));
    publish("config/highTemp", String(settings->highTemp));
    publish("config/lowHum", String(settings->lowHum));
    publish("config/highHum", String(settings->highHum));
    publish("config/delaySec", String(settings->delaySec));
    publish("config/maxOnTime", String(settings->maxOnTime));
    publish("config/sensorMode", settings->sensorMode ? "SENSOR" : "MANUAL");
    publish("config/adaptiveMode", settings->adaptiveMode ? "ON" : "OFF");
    publish("config/bootState", settings->bootState ? "ON" : "OFF");
}
```

**Детальное описание всех топиков — в `TRANSPORT_MQTT_SPEC.md`.**

---

## 7. Формирование топиков

Все топики строятся по схеме:

| Тип | Схема | Пример |
|-----|-------|--------|
| **Состояние** | `{clientId}/{параметр}` | `fan_1A2B/state` |
| **Настройки** | `{clientId}/config/{параметр}` | `fan_1A2B/config/lowTemp` |
| **Команды управления** | `{clientId}/c/{параметр}` | `fan_1A2B/c/state` |
| **Команды настройки** | `{clientId}/c/config/{параметр}` | `fan_1A2B/c/config/lowTemp` |

**`clientId`** формируется при первом запуске на основе типа устройства и последних четырёх символов MAC-адреса:
- `fan_XXXX` — для TYPE 1 (вентилятор с датчиком)
- `sensor_XXXX` — для TYPE 2 (автономный датчик)
- `switch_XXXX` — для TYPE 3 (управляемый выключатель)

**Детальное описание всех топиков — в `TRANSPORT_MQTT_SPEC.md`.**

---

## 8. Публикация

MQTT-протокол публикует данные в топики при получении через `publishState()` и `publishSettings()`.

```cpp
void MqttProtocol::publish(const char* topic, const char* value) {
    char fullTopic[128];
    snprintf(fullTopic, sizeof(fullTopic), "%s/%s", _clientId, topic);
    _mqttClient.publish(fullTopic, value);
}
```

**Все топики публикации описаны в `TRANSPORT_MQTT_SPEC.md`.**

---

## 9. Подписка и команды

Устройство подписывается на топики с префиксом `{clientId}/c/`.

### 9.1. Подписка

```cpp
void MqttProtocol::subscribe() {
    // Команды управления — {clientId}/c/{параметр}
    subscribe("c/state");
    subscribe("c/speed");
    subscribe("c/sensorMode");
    subscribe("c/adaptiveMode");
    
    // Команды настройки — {clientId}/c/config/{параметр}
    subscribe("c/config/lowTemp");
    subscribe("c/config/highTemp");
    subscribe("c/config/lowHum");
    subscribe("c/config/highHum");
    subscribe("c/config/delaySec");
    subscribe("c/config/maxOnTime");
    subscribe("c/config/bootState");
    subscribe("c/config/sensorMode");
    subscribe("c/config/adaptiveMode");
}

void MqttProtocol::subscribe(const char* topic) {
    char fullTopic[128];
    snprintf(fullTopic, sizeof(fullTopic), "%s/%s", _clientId, topic);
    _mqttClient.subscribe(fullTopic);
}
```

### 9.2. Обработка команд

```cpp
void MqttProtocol::callback(char* topic, byte* payload, unsigned int length) {
    // Преобразуем payload в строку
    String message = String((char*)payload).substring(0, length);
    TransportCommand cmd;
    TransportCommandData data;
    
    // Определяем команду по топику
    if (strstr(topic, "/c/state") != nullptr) {
        cmd = CMD_STATE;
        data.boolVal = (message == "ON");
    } else if (strstr(topic, "/c/speed") != nullptr) {
        cmd = CMD_SPEED;
        data.intVal = message.toInt();
    } else if (strstr(topic, "/c/sensorMode") != nullptr) {
        cmd = CMD_SET_SENSOR_CONTROL_MODE;
        data.boolVal = (message == "SENSOR");
    } else if (strstr(topic, "/c/adaptiveMode") != nullptr) {
        cmd = CMD_SET_ADAPTIVE_MODE;
        data.boolVal = (message == "ON");
    } else if (strstr(topic, "/c/config/lowTemp") != nullptr) {
        cmd = CMD_SET_LOW_TEMP;
        data.floatVal = message.toFloat();
    } else if (strstr(topic, "/c/config/highTemp") != nullptr) {
        cmd = CMD_SET_HIGH_TEMP;
        data.floatVal = message.toFloat();
    } else if (strstr(topic, "/c/config/lowHum") != nullptr) {
        cmd = CMD_SET_LOW_HUM;
        data.floatVal = message.toFloat();
    } else if (strstr(topic, "/c/config/highHum") != nullptr) {
        cmd = CMD_SET_HIGH_HUM;
        data.floatVal = message.toFloat();
    } else if (strstr(topic, "/c/config/delaySec") != nullptr) {
        cmd = CMD_SET_DELAY_SECONDS;
        data.u32Val = message.toInt();
    } else if (strstr(topic, "/c/config/maxOnTime") != nullptr) {
        cmd = CMD_SET_MAX_ON_TIME;
        data.u32Val = message.toInt();
    } else if (strstr(topic, "/c/config/bootState") != nullptr) {
        cmd = CMD_SET_BOOT_STATE;
        data.boolVal = (message == "ON");
    } else if (strstr(topic, "/c/config/sensorMode") != nullptr) {
        cmd = CMD_SET_SENSOR_CONTROL_MODE;
        data.boolVal = (message == "SENSOR");
    } else if (strstr(topic, "/c/config/adaptiveMode") != nullptr) {
        cmd = CMD_SET_ADAPTIVE_MODE;
        data.boolVal = (message == "ON");
    } else {
        XLOG_WARN(CAT_MQTT, "Unknown topic: %s", topic);
        return;
    }
    
    // Вызываем колбэк
    if (_commandCallback) {
        _commandCallback(cmd, &data);
    }
}
```

**Все топики команд описаны в `TRANSPORT_MQTT_SPEC.md`.**

---

## 10. Переподключение

При потере соединения:

1. `mqtt_update()` обнаруживает `!isConnected()`
2. Вызывается `reconnect()` с задержкой `MQTT_RECONNECT_DELAY_MS`
3. При успешном подключении:
   - Публикуется `Online` в `{clientId}/status`
   - Выполняется подписка на управляющие топики
   - Устанавливается `gateway_ok = true` в StateProvider
4. При неудаче — попытка повторяется через задержку

```cpp
void MqttProtocol::reconnect() {
    if (!_enabled) return;
    
    // Формируем LWT
    char statusTopic[64];
    snprintf(statusTopic, sizeof(statusTopic), "%s/status", _clientId);
    
    bool connected = _mqttClient.connect(_clientId, _user, _password, 
                                          statusTopic, 1, true, "Offline");
    
    if (connected) {
        _connected = true;
        StateProvider::getInstance().set_gateway_ok(true);
        
        // Публикуем Online и подписываемся
        publish("status", "Online");
        subscribe();
        
        // Публикуем версию
        publish("version", VERSION);
    } else {
        StateProvider::getInstance().set_gateway_ok(false);
    }
}
```

---

## 11. LWT (Last Will and Testament)

При подключении устройство отправляет Will-сообщение:

```cpp
_mqttClient.connect(_clientId, _user, _password, _topics.status, 1, true, "Offline");
```

- **Топик:** `{clientId}/status`
- **QoS:** 1
- **Retain:** true
- **Сообщение:** `Offline`

При штатном отключении устройство публикует `Offline` самостоятельно.

```cpp
void MqttProtocol::disconnect() {
    if (_connected) {
        publish("status", "Offline");
        _mqttClient.disconnect();
    }
    _connected = false;
    StateProvider::getInstance().set_gateway_ok(false);
}
```

---

## 12. Флаги компиляции

| Внутренний флаг | Назначение |
|-----------------|------------|
| `USE_MQTT` | Включить MQTT-протокол |

**Примечание:** `USE_MQTT` = 1 для WiFi-транспорта, если включена поддержка MQTT.

---

## 13. Связь с документацией

| Компонент | Документ |
|-----------|----------|
| Архитектура транспорта | `TRANSPORT_ABSTRACTION.md` |
| WiFi-транспорт | `TRANSPORT_WIFI.md` |
| Спецификация MQTT | `TRANSPORT_MQTT_SPEC.md` |
| HTTP-протокол | `TRANSPORT_HTTP.md` |

---

*Конец документа*