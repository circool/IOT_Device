```cpp
/**
 * @file TRANSPORT_MQTT_SPEC.md
 * @brief Спецификация MQTT-протокола — топики, форматы, команды
 * @note Статус: Рефакторинг
 * @version 0.11
 * @date 06.08.2026
 */
```

# TRANSPORT_MQTT_SPEC.md

## Оглавление

- [1. Назначение](#1-назначение)
- [2. Формирование топиков](#2-формирование-топиков)
- [3. Форматы данных](#3-форматы-данных)
- [4. Топики публикации](#4-топики-публикации)
- [5. Топики команд](#5-топики-команд)
- [6. Параметры, НЕ доступные через MQTT](#6-параметры-не-доступные-через-mqtt)

---

## 1. Назначение

Документ описывает **внешнее поведение** MQTT-протокола: топики, форматы данных, команды.

**Документ НЕ описывает:**
- Внутреннюю архитектуру MQTT-протокола (см `TRANSPORT_MQTT.md`)
- Механизмы получения данных (см `TRANSPORT_MQTT.md`)

---

## 2. Формирование топиков

Все топики строятся по схеме:

| Тип | Схема | Пример |
|-----|-------|--------|
| **Состояние** | `{clientId}/{параметр}` | `fan_1A2B/state`, `fan_1A2B/temperature` |
| **Настройки** | `{clientId}/config/{параметр}` | `fan_1A2B/config/lowTemp`, `fan_1A2B/config/delaySec` |
| **Команды управления** | `{clientId}/c/{параметр}` | `fan_1A2B/c/state`, `fan_1A2B/c/speed` |
| **Команды настройки** | `{clientId}/c/config/{параметр}` | `fan_1A2B/c/config/lowTemp`, `fan_1A2B/c/config/delaySec` |

**`clientId`** формируется при первом запуске на основе типа устройства и последних четырёх символов MAC-адреса:
- `fan_XXXX` — для TYPE 1 (вентилятор с датчиком)
- `sensor_XXXX` — для TYPE 2 (автономный датчик)
- `switch_XXXX` — для TYPE 3 (управляемый выключатель)

---

## 3. Форматы данных

| Тип | Формат | Пример | Топики |
|-----|--------|--------|--------|
| `ON` / `OFF` | Строка | `ON` | `state`, `adaptiveMode`, `sensorMode` |
| Число | Десятичное | `75` | `speed`, `delaySec`, `maxOnTime`, `rssi` |
| `SENSOR` / `MANUAL` | Строка | `SENSOR` | `sensorMode`, `sensorMode` |
| Число с плавающей точкой | Десятичное | `24.5` | `temperature`, `humidity`, `lowTemp`, `highTemp` |
| Статус | Строка | `Online` / `Offline` | `status` |

---

## 4. Топики публикации

### 4.1. Общие для всех типов

| Топик | Тип | QoS | Retain | Описание |
|-------|-----|-----|--------|----------|
| `{clientId}/status` | LWT | 1 | ✅ | `Online` / `Offline` |
| `{clientId}/version` | static | 0 | ✅ | Версия прошивки |
| `{clientId}/rssi` | dynamic | 0 | ❌ | Сила сигнала WiFi (dBm) |
| `{clientId}/lastReset` | event | 0 | ❌ | Причина последней нештатной перезагрузки |

### 4.2. TYPE 1 (вентилятор с датчиком)

**Состояние (`{clientId}/{параметр}`):**

| Топик | Тип | QoS | Retain | Описание |
|-------|-----|-----|--------|----------|
| `{clientId}/state` | dynamic | 0 | ✅ | `ON` / `OFF` |
| `{clientId}/speed` | dynamic | 0 | ✅ | 0-100% |
| `{clientId}/temperature` | dynamic | 0 | ❌ | °C |
| `{clientId}/humidity` | dynamic | 0 | ❌ | % |
| `{clientId}/sensorMode` | dynamic | 0 | ✅ | `SENSOR` / `MANUAL` |
| `{clientId}/adaptiveMode` | dynamic | 0 | ✅ | `ON` / `OFF` |
| `{clientId}/delay_remain` | dynamic | 0 | ❌ | Остаток задержки (сек) |
| `{clientId}/max_on_remain` | dynamic | 0 | ❌ | Остаток аварийного таймера (сек) |

**Настройки (`{clientId}/config/{параметр}`):**

| Топик | Тип | QoS | Retain | Описание |
|-------|-----|-----|--------|----------|
| `{clientId}/config/lowTemp` | dynamic | 0 | ✅ | Нижний порог температуры |
| `{clientId}/config/highTemp` | dynamic | 0 | ✅ | Верхний порог температуры |
| `{clientId}/config/lowHum` | dynamic | 0 | ✅ | Нижний порог влажности |
| `{clientId}/config/highHum` | dynamic | 0 | ✅ | Верхний порог влажности |
| `{clientId}/config/delaySec` | dynamic | 0 | ✅ | Задержка отложенного включения (сек) |
| `{clientId}/config/maxOnTime` | dynamic | 0 | ✅ | Таймер аварийного отключения (сек) |
| `{clientId}/config/sensorMode` | dynamic | 0 | ✅ | `SENSOR` / `MANUAL` |
| `{clientId}/config/adaptiveMode` | dynamic | 0 | ✅ | `ON` / `OFF` |
| `{clientId}/config/bootState` | dynamic | 0 | ✅ | `ON` / `OFF` |

### 4.3. TYPE 2 (автономный датчик)

**Состояние (`{clientId}/{параметр}`):**

| Топик | Тип | QoS | Retain | Описание |
|-------|-----|-----|--------|----------|
| `{clientId}/temperature` | dynamic | 0 | ❌ | °C |
| `{clientId}/humidity` | dynamic | 0 | ❌ | % |

### 4.4. TYPE 3 (управляемый выключатель)

**Состояние (`{clientId}/{параметр}`):**

| Топик | Тип | QoS | Retain | Описание |
|-------|-----|-----|--------|----------|
| `{clientId}/state` | dynamic | 0 | ✅ | `ON` / `OFF` |
| `{clientId}/delay_remain` | dynamic | 0 | ❌ | Остаток задержки (сек) |
| `{clientId}/max_on_remain` | dynamic | 0 | ❌ | Остаток аварийного таймера (сек) |

**Настройки (`{clientId}/config/{параметр}`):**

| Топик | Тип | QoS | Retain | Описание |
|-------|-----|-----|--------|----------|
| `{clientId}/config/delaySec` | dynamic | 0 | ✅ | Задержка отложенного включения (сек) |
| `{clientId}/config/maxOnTime` | dynamic | 0 | ✅ | Таймер аварийного отключения (сек) |
| `{clientId}/config/bootState` | dynamic | 0 | ✅ | `ON` / `OFF` |

---

## 5. Топики команд

Устройство подписывается на топики с префиксом `{clientId}/c/`.

### 5.1. Команды управления (`{clientId}/c/{параметр}`)

| Топик | Параметр | Тип | Диапазон | Retain | Применимость |
|-------|----------|-----|----------|--------|--------------|
| `{clientId}/c/state` | Состояние | `ON`/`OFF` | — | ❌ | TYPE 1, 3 |
| `{clientId}/c/speed` | Скорость | uint8_t | 0-100 | ❌ | TYPE 1 |
| `{clientId}/c/sensorMode` | Режим | `SENSOR`/`MANUAL` | — | ❌ | TYPE 1 |
| `{clientId}/c/adaptiveMode` | Адаптивный | `ON`/`OFF` | — | ❌ | TYPE 1 |

### 5.2. Команды настройки (`{clientId}/c/config/{параметр}`)

| Топик | Параметр | Тип | Диапазон | Retain | Применимость |
|-------|----------|-----|----------|--------|--------------|
| `{clientId}/c/config/lowTemp` | Нижний порог температуры | float | -40..85 | ❌ | TYPE 1 |
| `{clientId}/c/config/highTemp` | Верхний порог температуры | float | -40..85 | ❌ | TYPE 1 |
| `{clientId}/c/config/lowHum` | Нижний порог влажности | float | 0..100 | ❌ | TYPE 1 |
| `{clientId}/c/config/highHum` | Верхний порог влажности | float | 0..100 | ❌ | TYPE 1 |
| `{clientId}/c/config/delaySec` | Задержка включения | int | 0-86400 | ❌ | TYPE 1, 3 |
| `{clientId}/c/config/maxOnTime` | Аварийное отключение | uint32_t | 0-86400 | ❌ | TYPE 1, 3 |
| `{clientId}/c/config/bootState` | Состояние при старте | `ON`/`OFF` | — | ❌ | TYPE 1, 3 |
| `{clientId}/c/config/sensorMode` | Режим SENSOR/MANUAL | `SENSOR`/`MANUAL` | — | ❌ | TYPE 1 |
| `{clientId}/c/config/adaptiveMode` | Адаптивный режим | `ON`/`OFF` | — | ❌ | TYPE 1 |

---

## 6. Параметры, НЕ доступные через MQTT

| Параметр | Причина |
|----------|---------|
| `wifiSsid` | Изменение может привести к потере связи |
| `wifiPassword` | Изменение может привести к потере связи |
| `mqttBroker` | Изменение может привести к потере связи |
| `mqttPort` | Изменение может привести к потере связи |
| `mqttUser` | Изменение может привести к потере связи |
| `mqttPassword` | Изменение может привести к потере связи |
| `mqttClientId` | Изменение может привести к конфликту в брокере |

**Эти параметры изменяются ТОЛЬКО через веб-интерфейс (HTTP).**

---

*Конец документа*