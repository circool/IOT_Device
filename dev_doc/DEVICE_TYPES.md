Принято. Исправляю `DEVICE_TYPES.md` с учётом `platformio.ini` и `settings.h`.

---

# DEVICE_TYPES.md

## Типы устройств

---

## 1. Общее

Проект поддерживает **три типа устройств**, выбираемых на этапе компиляции через флаг `-DDEVICE_TYPE`.

| Тип | Назначение | Ключевые компоненты |
|-----|-----------|---------------------|
| **1** | Вентилятор с датчиком | Датчик температуры/влажности + реле с ШИМ |
| **2** | Автономный датчик | Только датчик температуры/влажности |
| **3** | Управляемый выключатель | Реле (ON/OFF), без датчика |

---

## 2. Выбор типа

```cpp
// settings.h
#ifndef DEVICE_TYPE
#define DEVICE_TYPE 1
#endif
```

**В `platformio.ini`:**
```ini
build_flags =
    -DDEVICE_TYPE=1   # 1, 2 или 3
```

---

## 3. TYPE 1 — Вентилятор с датчиком

**Назначение:** автоматическое управление вентиляцией на основе температуры и влажности.

**Компоненты:**
- Датчик температуры/влажности (AHT10 или DHT)
- Реле с ШИМ-управлением (для регулировки скорости)
- Твердотельное реле (для ШИМ)

**Классы:**
```cpp
// main.cpp — при DEVICE_TYPE == 1
fan = new FanActuator();
fan->init(SWITCH_PIN, RELAY_ON_LEVEL, 
          g_configManager.getBootState(),
          g_configManager.getSpeedPercent(),
          g_configManager.getAdaptiveMode(),
          g_configManager.getDelaySeconds(), 
          g_configManager.getMaxOnTime());

statusProvider = new FanWebStatusProvider(fan, &mqttManager);
```

**MQTT-топики:** все топики с `fan/` (см. `TRANSPORT_MQTT.md`)

**Web-страница:** полная (состояние + конфигурация)

**Доступные функции:**
- Управление по порогам температуры/влажности
- ШИМ-регулировка скорости (0-100%)
- Адаптивный режим
- Таймер отложенного включения
- Таймер аварийного отключения
- Ручной режим

---

## 4. TYPE 2 — Автономный датчик

**Назначение:** только сбор и публикация показаний температуры и влажности.

**Компоненты:**
- Датчик температуры/влажности (AHT10 или DHT)
- Реле — **ОТСУТСТВУЕТ**

**Классы:**
```cpp
// main.cpp — при DEVICE_TYPE == 2
// Нет актуатора
statusProvider = new SensorWebStatusProvider(&mqttManager);
```

**MQTT-топики:** только публикация (нет команд):
- `{prefix}/sensor/temperature`
- `{prefix}/sensor/humidity`
- `{prefix}/status`
- `{prefix}/version`
- `{prefix}/rssi`
- `{prefix}/last_reset`

**Web-страница:** упрощённая (только датчик)

**Доступные функции:**
- Чтение датчика
- Публикация показаний
- **Нет** управления актуатором

---

## 5. TYPE 3 — Управляемый выключатель

**Назначение:** управление нагрузкой по командам пользователя (без автоматики).

**Компоненты:**
- Реле (ON/OFF, без ШИМ)
- Датчик — **ОТСУТСТВУЕТ**

**Классы:**
```cpp
// main.cpp — при DEVICE_TYPE == 3
switchActuator = new SwitchActuator();
switchActuator->init(SWITCH_PIN, RELAY_ON_LEVEL,
                     g_configManager.getBootState(),
                     g_configManager.getDelaySeconds(),
                     g_configManager.getMaxOnTime());

statusProvider = new SwitchWebStatusProvider(switchActuator, &mqttManager);
```

**MQTT-топики:** с `switch/` вместо `fan/`:
- `{prefix}/switch/state` (публикация + команда)
- `{prefix}/switch/delaySec`
- `{prefix}/switch/maxOnTime`

**Web-страница:** упрощённая (без датчика)

**Доступные функции:**
- Включение/выключение по команде
- Таймер отложенного включения
- Таймер аварийного отключения
- **Нет** ШИМ, адаптивного режима, управления по датчику

---

## 6. Сравнительная таблица

| Функция | TYPE 1 | TYPE 2 | TYPE 3 |
|---------|--------|--------|--------|
| Датчик | ✅ Есть | ✅ Есть | ❌ Нет |
| Реле | ✅ Есть | ❌ Нет | ✅ Есть |
| ШИМ | ✅ Есть | ❌ Нет | ❌ Нет |
| Автоматика (пороги) | ✅ Есть | ❌ Нет | ❌ Нет |
| Адаптивный режим | ✅ Есть | ❌ Нет | ❌ Нет |
| Ручной режим | ✅ Есть | ❌ Нет | ✅ Есть |
| Таймер отложенного включения | ✅ Есть | ❌ Нет | ✅ Есть |
| Таймер аварийного отключения | ✅ Есть | ❌ Нет | ✅ Есть |
| MQTT-команды | ✅ Да (fan/) | ❌ Нет | ✅ Да (switch/) |
| MQTT-публикация датчика | ✅ Да | ✅ Да | ❌ Нет |

---

## 7. Флаги компиляции по типам

| Флаг | TYPE 1 | TYPE 2 | TYPE 3 |
|------|--------|--------|--------|
| `FEATURE_SENSOR_ENABLED` | 1 | 1 | 0 (принудительно) |
| `PWM_ENABLED` | 1 | 0 | 0 |
| `DEVICE_TYPE` | 1 | 2 | 3 |
| `DEVICE_PREFIX` | `"fan"` | `"sensor"` | `"switch"` |

---

## 8. Примеры сборки

| Платформа | Тип | Транспорт | Команда |
|-----------|-----|-----------|---------|
| ESP8266 | 1 | WiFi/MQTT | `pio run -e esp8266` |
| ESP32 | 1 | WiFi/MQTT + BLE | `pio run -e esp32-ble_only` |
| ESP32-C3 | 3 | WiFi/MQTT | `pio run -e esp32-c3-SuperMini` |
| ESP32-C6 | 1 | WiFi/MQTT | `pio run -e esp32-c6-mqtt` |
| ESP32-C6 | 1 | Zigbee | `pio run -e esp32-c6-zigbee` |

---

*Конец документа*