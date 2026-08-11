```cpp
/**
 * @file DEVICE_CONTROLLER.md
 * @brief Бизнес-логика устройства
 * @version 0.12
 * @date 11.08.2026
 */
```

# DEVICE_CONTROLLER.md

## Оглавление

- [1. Назначение](#1-назначение)
- [2. Архитектурное место](#2-архитектурное-место)
- [3. Ответственность](#3-ответственность)
- [4. Структуры данных](#4-структуры-данных)
- [5. Публичный интерфейс](#5-публичный-интерфейс)
- [6. Приватные поля](#6-приватные-поля)
- [7. Логика работы](#7-логика-работы)
  - [7.1. Инициализация](#71-инициализация)
  - [7.2. Сеттеры](#72-сеттеры)
  - [7.3. Периодическая обработка (update)](#73-периодическая-обработка-update)
  - [7.4. Внутренние методы](#74-внутренние-методы)
- [8. Типы устройств](#8-типы-устройств)
- [9. Потоки данных](#9-потоки-данных)
- [10. Интеграция с оркестратором](#10-интеграция-с-оркестратором)
- [11. Константы](#11-константы)
- [12. Флаги компиляции](#12-флаги-компиляции)
- [13. Связь с документацией](#13-связь-с-документацией)
- [14. Изменения по сравнению с предыдущей версией](#14-изменения-по-сравнению-с-предыдущей-версией)

---

## 1. Назначение

`DeviceController` — центральный слой, содержащий **всю бизнес-логику** устройства.

**Статус:** ✅ **Реализован** (v0.13)

---

## 2. Архитектурное место

```
┌────────────────────────────────────────────────────────────┐
│                              ОРКЕСТРАТОР                   │
│                                                            │
│  • Инициализирует DeviceController                         │
│  • Передаёт ссылку на DeviceConfig                         │
│  • Получает указатель на состояние через init()            │
│  • Получает уведомления через колбэк (маска изменений)     │
│  • Передаёт команды через сеттеры                          │
└────────────────────────────────────────────────────────────┘
          │                    │                    │
          ▼                    ▼                    ▼
┌──────────────────┐  ┌───────────────────┐  ┌───────────────┐
│  ConfigManager   │  │ DeviceController  │  │   Transport   │
│                  │  │                   │  │               │
│ • Владелец       │  │ • Владелец        │  │ • Публикует   │
│   DeviceConfig   │  │   DeviceState     │  │   состояние   │
│ • Предоставляет  │  │ • Бизнес-логика   │  │ • Принимает   │
│   const ссылку   │  │ • Управляет       │  │   команды     │
│                  │  │   Actuator        │  │               │
└──────────────────┘  └───────────────────┘  └───────────────┘
```

---

## 3. Ответственность

| Область | Ответственность |
|---------|-----------------|
| **Хранение состояния** | `DeviceState` (RAM) — текущее состояние устройства |
| **Принятие решений** | На основе показаний датчика, порогов из Config, режимов |
| **Управление актуатором** | Через внутренний компонент `Actuator` |
| **Обработка команд** | Изменение состояния по командам от транспорта (сеттеры) |
| **Оповещение** | Вызов колбэка с маской изменений |

**DeviceController НЕ ЗНАЕТ:**
- О ConfigManager (получает ссылку на Config через `init()`)
- О транспорте (MQTT, Web, Zigbee, Matter)
- О StateProvider
- О пинах (управляет через Actuator)

---

## 4. Структуры данных

### 4.1. DeviceState (RAM) — оперативное состояние

```cpp
typedef struct {
    bool isOn;
    uint8_t speed;
    bool manualMode;
    bool adaptiveMode;
    bool sensorMode;
    uint32_t delayRemain;
    uint32_t maxOnRemain;
    float temperature;
    float humidity;
    bool sensorValid;
} DeviceState;
```

### 4.2. DeviceConfig (EEPROM) — настройки (только чтение!)

```cpp
typedef struct {
#if DEVICE_TYPE == 1
    bool sensorControlMode;
    bool adaptiveMode;
    float lowTemp;
    float highTemp;
    float lowHum;
    float highHum;
    uint8_t speedPercent;
#endif
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    uint32_t delaySeconds;
    uint32_t maxOnTime;
    uint8_t bootState;
#endif
} DeviceConfig;
```

**Важно:** DeviceController **НЕ ХРАНИТ КОПИЮ** DeviceConfig — только указатель!

```cpp
const DeviceConfig* _config;  // указатель!
```

### 4.3. StateChangeFlags — маска изменений

Опредена в `common_types.h`

```cpp
typedef enum {
    STATE_CHANGED_IS_ON           = (1 << 0),
    STATE_CHANGED_SPEED           = (1 << 1),
    STATE_CHANGED_MANUAL_MODE     = (1 << 2),
    STATE_CHANGED_ADAPTIVE_MODE   = (1 << 3),
    STATE_CHANGED_SENSOR_MODE     = (1 << 4),
    STATE_CHANGED_TEMPERATURE     = (1 << 5),
    STATE_CHANGED_HUMIDITY        = (1 << 6),
    STATE_CHANGED_SENSOR_VALID    = (1 << 7),
    STATE_CHANGED_DELAY_REMAIN    = (1 << 8),
    STATE_CHANGED_MAX_ON_REMAIN   = (1 << 9),
} StateChangeFlags;
```

---

## 5. Публичный интерфейс

```cpp
class DeviceController {
public:
    DeviceController();

    /**
     * @brief Инициализация контроллера
     * @param config Указатель на DeviceConfig (из ConfigManager)
     * @param outState Ссылка на указатель, куда будет записан адрес _state
     * 
     * @note После инициализации записывает адрес _state в outState
     */
    void init(const DeviceConfig* config, const DeviceState*& outState);

    void setOn(bool on);
    void setSpeed(uint8_t percent);
    void setSensorMode(bool on);
    void setAdaptiveMode(bool on);

    void update();

    /**
     * @brief Регистрация колбэка для уведомления об изменениях
     * @param callback Функция-колбэк (принимает маску изменений)
     */
    void onStateChanged(DeviceControllerCallback callback);
};
```

### 5.1. Тип колбэка

```cpp
/**
 * @brief Тип функции-колбэка для уведомления об изменении состояния
 * @param changes Битовая маска изменений (StateChangeFlags)
 * 
 * @note Состояние доступно через указатель, полученный в init()
 */
typedef void (*DeviceControllerCallback)(uint32_t changes);
```

---

## 6. Приватные поля

```cpp
class DeviceController {
private:
    const DeviceConfig* _config;   // Указатель на конфиг (НЕ копия!)
    DeviceState _state;            // Текущее состояние
    bool _changed;                 // Флаг изменения

    Sensor _sensor;
#if DEVICE_TYPE == 1
    FanActuator _actuator;
#elif DEVICE_TYPE == 3
    SwitchActuator _actuator;
#endif

    DeviceControllerCallback _callback;

    bool _delayTimerRunning;
    unsigned long _delayTimerStart;

    void notifyChange(uint32_t changes);
    void applyStateToActuator();
    int calculateAdaptiveSpeed() const;
    bool updateDelayTimer(uint32_t& changes);
    bool updateTimerRemains(uint32_t& changes);
};
```

---

## 7. Логика работы

### 7.1. Инициализация (`init()`)

1. Сохраняет указатель на `DeviceConfig` (НЕ копирует!)
2. Инициализирует `Sensor` и `Actuator`
3. Вычисляет начальное состояние из `config`
4. Применяет к актуатору
5. Записывает `&_state` в `outState` (передаёт указатель наружу)
6. Устанавливает `_changed = true` и вызывает `notifyChange(0xFFFFFFFF)`

### 7.2. Сеттеры

| Сеттер | Действие | Маска |
|--------|----------|-------|
| **`setOn()`** | Меняет `isOn` | `IS_ON | MANUAL_MODE | SENSOR_MODE | ADAPTIVE_MODE` (если изменились) |
| **`setSpeed()`** | Меняет `speed` | `SPEED | MANUAL_MODE | ADAPTIVE_MODE` (если изменились) |
| **`setSensorMode()`** | Меняет `sensorMode` | `SENSOR_MODE | MANUAL_MODE` (если изменился) |
| **`setAdaptiveMode()`** | Меняет `adaptiveMode` | `ADAPTIVE_MODE | MANUAL_MODE | SPEED` (если изменились) |

### 7.3. Периодическая обработка (`update()`)

1. Чтение датчика
2. Проверка аварийного отключения
3. Таймер отложенного включения
4. Автоматическое управление по порогам
5. Адаптивный режим
6. Обновление остатков таймеров
7. Применение к актуатору и уведомление

### 7.4. Внутренние методы

- **`calculateAdaptiveSpeed()`** — вычисляет новую скорость в адаптивном режиме
- **`updateDelayTimer()`** — обновляет таймер отложенного включения
- **`updateTimerRemains()`** — обновляет остаток аварийного таймера
- **`applyStateToActuator()`** — применяет состояние к актуатору
- **`notifyChange()`** — вызывает колбэк с маской изменений

---

## 8. Типы устройств

### 8.1. TYPE 1 — Вентилятор с датчиком
**Компоненты:** Sensor + FanActuator

### 8.2. TYPE 2 — Автономный датчик
**Компоненты:** Sensor

### 8.3. TYPE 3 — Управляемый выключатель
**Компоненты:** SwitchActuator

---

## 9. Потоки данных

### 9.1. Инициализация

```
Оркестратор: DeviceController.init(&deviceConfig, g_statePtr)
DeviceController: 
  1. Сохраняет _config = &deviceConfig
  2. Вычисляет _state из config
  3. Записывает &_state в outState
  4. notifyChange(0xFFFFFFFF)
Оркестратор: onStateChanged(0xFFFFFFFF)
```

### 9.2. Команда от пользователя

```
Транспорт/кнопка → Оркестратор → DeviceController.setOn(true)
DeviceController: 
  1. Меняет _state
  2. Собирает маску
  3. notifyChange(changes)
Оркестратор: onStateChanged(changes)
```

### 9.3. Чтение состояния

```
Оркестратор: g_statePtr = &_state (получен при инициализации)
Оркестратор: читает g_statePtr->isOn в любой момент
```

---

## 10. Интеграция с оркестратором

```cpp
// main.cpp
const DeviceState* g_statePtr = nullptr;

void onDeviceStateChanged(uint32_t changes) {
    if (changes & STATE_CHANGED_IS_ON) {
        XLOG_DEBUG(CAT_MAIN, "isOn: %d", g_statePtr->isOn);
    }
}

void setup() {
    g_deviceController.onStateChanged(onDeviceStateChanged);
    g_deviceController.init(&g_deviceConfig, g_statePtr);
}

void loop() {
    g_deviceController.update();
    
    if (stage == BUTTON_SHORT) {
        g_deviceController.setOn(!g_statePtr->isOn);
    }
}
```

---

## 11. Константы

| Константа | Значение | Описание | Файл |
|-----------|----------|----------|------|
| `PWM_FREQUENCY` | 500 Гц | Частота ШИМ | `device_controller_fan_actuator.h` |
| `PWM_RESOLUTION` | 8 бит | Разрешение ШИМ | `device_controller_fan_actuator.h` |
| `PWM_STARTING` | 200 мс | Стартовый импульс | `device_controller_fan_actuator.h` |
| `MIN_SPEED_PERCENT` | 10% | Мин. скорость | `device_controller_fan_actuator.h` |
| `ADAPTIVE_STEP` | 10% | Шаг адаптивного | `device_controller.cpp` |
| `TIMER_UPDATE_INTERVAL` | 1000 мс | Интервал таймеров | `device_controller.cpp` |

---

## 12. Флаги компиляции

| Флаг | Назначение |
|------|------------|
| `DEVICE_TYPE` | 1 = Fan, 2 = Sensor, 3 = Switch |
| `USE_SENSOR` | Включить датчик |
| `SENSOR_TYPE` | 1 = AHT10, 2 = DHT |

---

## 13. Связь с документацией

| Документ | Описание |
|----------|----------|
| `ARCHITECTURE.md` | Общая архитектура |
| `common_types.h` | Структуры данных |
| `device_controller_sensor.h` | Датчик |
| `device_controller_fan_actuator.h` | Вентилятор |
| `device_controller_switch_actuator.h` | Выключатель |
| `ORCHESTRATOR.md` | Оркестратор |


---

*Конец документа*