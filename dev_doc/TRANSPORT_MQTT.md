# TRANSPORT_MQTT.md

## MQTT-транспорт

---

## 1. Назначение

MQTT — основной протокол интеграции устройства в экосистемы умного дома. Устройство подключается к заданному брокеру, публикует своё состояние и принимает команды управления.

---

## 2. Реализация

### 2.1. Файлы

| Файл | Назначение |
|------|------------|
| `mqtt_manager.h/cpp` | Реализация MQTT-клиента (`MQTTManager`) |
| `transport_mqtt.h/cpp` | Адаптер над `MQTTManager`, реализующий структуру `Transport` |

### 2.2. Зависимости

- `PubSubClient` — MQTT-клиент
- `WiFiClient` — транспортный слой (TCP/IP)

### 2.3. Особенности реализации

| Особенность | Описание |
|-------------|----------|
| **Колбэки** | Без `std::function` — структуры с указателями на функции + контекст (экономия RAM) |
| **Переподключение** | Автоматическое с задержкой (`MQTT_RECONNECT_DELAY_MS`) |
| **LWT** | Публикация `Offline` в топик `{prefix}/status` при потере связи |
| **Keep-Alive** | `MQTT_KEEPALIVE_SEC` (по умолчанию 3 сек) |
| **Буфер** | 512 байт для JSON-сообщений |

---

## 3. Формирование топиков

Все топики строятся по схеме: `{prefix}/{группа}/{параметр}`

**Префикс (`prefix`)** — это `mqttClientId`, который формируется при первом запуске на основе типа устройства и последних четырёх символов MAC-адреса:
- `fan_XXXX` — для TYPE 1 (вентилятор с датчиком)
- `sensor_XXXX` — для TYPE 2 (автономный датчик)
- `switch_XXXX` — для TYPE 3 (управляемый выключатель)

**Топики управления** (команды от брокера к устройству) имеют префикс `c/` в пути: `{prefix}/c/{группа}/{параметр}`

---

## 4. Публикация (устройство → брокер)

### 4.1. Общие для всех типов

| Топик | Тип | Описание |
|-------|-----|----------|
| `{prefix}/status` | LWT | `Online` / `Offline` |
| `{prefix}/version` | static | Версия прошивки (публикуется один раз при подключении) |
| `{prefix}/rssi` | dynamic | Сила сигнала WiFi (dBm) |
| `{prefix}/last_reset` | event | Причина последней перезагрузки (только нештатные) |

### 4.2. TYPE 1 (вентилятор с датчиком)

| Топик | Тип | Описание |
|-------|-----|----------|
| `{prefix}/fan/state` | dynamic | `ON` / `OFF` |
| `{prefix}/fan/speed` | dynamic | Скорость (0-100%) |
| `{prefix}/fan/delaySec` | dynamic | Задержка отложенного включения (сек) |
| `{prefix}/fan/maxOnTime` | dynamic | Таймер аварийного отключения (сек) |
| `{prefix}/fan/sensorControlMode` | dynamic | `1` / `0` (AUTO / MANUAL) |
| `{prefix}/fan/adaptiveMode` | dynamic | `1` / `0` (адаптивный режим) |
| `{prefix}/sensor/temperature` | dynamic | Температура (°C) |
| `{prefix}/sensor/humidity` | dynamic | Влажность (%) |
| `{prefix}/sensor/lowTemp` | dynamic | Нижний порог температуры |
| `{prefix}/sensor/highTemp` | dynamic | Верхний порог температуры |
| `{prefix}/sensor/lowHum` | dynamic | Нижний порог влажности |
| `{prefix}/sensor/highHum` | dynamic | Верхний порог влажности |

### 4.3. TYPE 2 (автономный датчик)

| Топик | Тип | Описание |
|-------|-----|----------|
| `{prefix}/sensor/temperature` | dynamic | Температура (°C) |
| `{prefix}/sensor/humidity` | dynamic | Влажность (%) |

### 4.4. TYPE 3 (управляемый выключатель)

| Топик | Тип | Описание |
|-------|-----|----------|
| `{prefix}/switch/state` | dynamic | `ON` / `OFF` |
| `{prefix}/switch/delaySec` | dynamic | Задержка отложенного включения (сек) |
| `{prefix}/switch/maxOnTime` | dynamic | Таймер аварийного отключения (сек) |

---

## 5. Команды управления (брокер → устройство)

Устройство принимает команды через топики с префиксом `{prefix}/c/`.

### 5.1. Общие для TYPE 1 и TYPE 3

| Топик | Параметр | Тип | Диапазон |
|-------|----------|-----|----------|
| `{prefix}/c/fan/state` (TYPE 1) | Состояние | bool | `ON` / `OFF` |
| `{prefix}/c/switch/state` (TYPE 3) | Состояние | bool | `ON` / `OFF` |
| `{prefix}/c/fan/speed` (TYPE 1) | Скорость | uint8_t | 0-100 |
| `{prefix}/c/fan/delaySec` | Задержка включения | int | 0-86400 |
| `{prefix}/c/fan/maxOnTime` | Аварийное отключение | uint32_t | 0-86400 |
| `{prefix}/c/fan/sensorControlMode` | Режим AUTO/MANUAL | bool | `1` / `0` |

### 5.2. Только TYPE 1 (вентилятор с датчиком)

| Топик | Параметр | Тип | Диапазон |
|-------|----------|-----|----------|
| `{prefix}/c/fan/adaptiveMode` | Адаптивный режим | bool | `1` / `0` |
| `{prefix}/c/sensor/lowTemp` | Нижний порог температуры | float | -40..85 |
| `{prefix}/c/sensor/highTemp` | Верхний порог температуры | float | -40..85 |
| `{prefix}/c/sensor/lowHum` | Нижний порог влажности | float | 0..100 |
| `{prefix}/c/sensor/highHum` | Верхний порог влажности | float | 0..100 |

### 5.3. Опционально (если `MQTT_RESET_ENABLED == 1`)

| Топик | Параметр | Тип | Описание |
|-------|----------|-----|----------|
| `{prefix}/c/system/reset` | Сброс | bool | `1` / `ON` / `reset` — сброс к заводским настройкам |

---

## 6. Параметры, НЕ доступные через MQTT

| Параметр | Причина |
|----------|---------|
| `wifiSsid` | Изменение может привести к потере связи с устройством |
| `wifiPassword` | Изменение может привести к потере связи с устройством |
| `mqttBroker` | Изменение может привести к потере связи с устройством |
| `mqttPort` | Изменение может привести к потере связи с устройством |
| `mqttUser` | Изменение может привести к потере связи с устройством |
| `mqttPassword` | Изменение может привести к потере связи с устройством |
| `mqttClientId` | Изменение может привести к конфликту в брокере |
| `sensorInterval` | Технически возможно, но пока не реализовано |

**Эти параметры изменяются ТОЛЬКО через веб-интерфейс.**

---

## 7. Форматы данных

### 7.1. Простые значения

Большинство топиков передают простые значения:

| Тип | Пример | Топики |
|-----|--------|--------|
| `ON` / `OFF` | `ON` | `state` |
| Число | `75` | `speed`, `delaySec`, `maxOnTime`, `rssi` |
| `1` / `0` | `1` | `sensorControlMode`, `adaptiveMode` |
| Число с плавающей точкой | `24.5` | `temperature`, `humidity`, `lowTemp`, `highTemp` |

### 7.2. Исключения

| Топик | Формат | Пример |
|-------|--------|--------|
| `{prefix}/status` | Строка | `Online` / `Offline` |
| `{prefix}/version` | Строка | `1.8.0` |
| `{prefix}/last_reset` | Строка | `WATCHDOG` / `POWER_ON` |

---

## 8. Обработка команд в коде

### 8.1. Поток команды

```
1. Брокер отправляет сообщение в топик {prefix}/c/fan/state = ON
2. MQTTManager::callback() получает сообщение
3. MQTTManager::handleCommand() парсит топик и payload
4. Вызывается зарегистрированный колбэк (например, _onState.func(true, context))
5. Оркестратор получает колбэк и вызывает DeviceController::setState(true)
6. DeviceController обрабатывает команду
```

### 8.2. Регистрация колбэков

```cpp
// В оркестраторе (main.cpp)
mqttManager.onState(onStateCommand, nullptr);

// Колбэк в оркестраторе
void onStateCommand(bool value, void* context) {
    deviceController->setState(value);
}
```

---

## 9. Переподключение

При потере соединения:

1. `MQTTManager::update()` обнаруживает `!isConnected()`
2. Вызывается `reconnect()` с задержкой `MQTT_RECONNECT_DELAY_MS` (по умолчанию 5000 мс)
3. При успешном подключении:
   - Публикуется `Online` в `{prefix}/status`
   - Выполняется подписка на управляющие топики
   - Устанавливается `STATE_MQTT_OK` в SystemState
4. При неудаче — попытка повторяется через задержку

---

## 10. LWT (Last Will and Testament)

При подключении устройство отправляет `Will`-сообщение:

```cpp
_mqttClient.connect(_clientId, _user, _password, _topics.online, 1, true, "Offline");
```

- **Топик:** `{prefix}/status`
- **QoS:** 1
- **Retain:** true
- **Сообщение:** `Offline`

При штатном отключении устройство публикует `Offline` самостоятельно.

---

## 11. Флаги компиляции

| Флаг | Значение по умолчанию | Описание |
|------|----------------------|----------|
| `FEATURE_MQTT_ENABLED` | 1 | Включить MQTT-клиент |
| `MQTT_PORT` | 1883 | Порт MQTT брокера |
| `MQTT_KEEPALIVE_SEC` | 3 | Keep-Alive интервал (сек) |
| `MQTT_RECONNECT_DELAY_MS` | 5000 | Задержка между попытками переподключения (мс) |
| `MQTT_PUBLISH_RSSI` | 1 | Публиковать RSSI |
| `MQTT_PUBLISH_VERSION` | 1 | Публиковать версию прошивки |
| `MQTT_PUBLISH_RESET_REASON` | 1 | Публиковать причину перезагрузки |
| `MQTT_IGNORE_PUBLISH_NORMAL_RESET_REASONS` | 1 | Не публиковать штатные перезагрузки (POWER_ON, SOFT_RESTART) |
| `MQTT_RESET_ENABLED` | 0 | Включить команду сброса через MQTT |

---