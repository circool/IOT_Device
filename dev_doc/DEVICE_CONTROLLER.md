```cpp
/**
 * @file DEVICE_CONTROLLER.md
 * @brief Бизнес-логика устройства
 * @note Статус: Закончен
 * @todo Необходима актуализация - см STATUS_PROVIDER.md
 */
```
# DEVICE_CONTROLLER.md

## Бизнес-логика устройства

## 1. Назначение

`DeviceController` — центральный слой, содержащий **всю бизнес-логику** устройства. Он изолирует принятие решений от транспорта (MQTT/Web/Zigbee/Matter), железа (Actuator/Sensor) и инфраструктуры (ConfigManager).

**Статус:** ✅ **Реализован** (базовый функционал). В `update()` реализована логика для всех трёх типов устройств (TYPE 1, 2, 3). Обработка всех команд реализована через `handle_command()`. 

---

## 2. Ответственность

| Область | Ответственность |
|---------|-----------------|
| **Хранение состояния** | `OperationalState` (RAM) — текущее состояние устройства |
| **Принятие решений** | На основе показаний датчика, порогов из Config, режимов |
| **Управление актуатором** | Через вызовы методов `Actuator` |
| **Обработка команд** | Изменение состояния по командам от транспорта |
| **Оповещение** | Вызов колбэка при изменении состояния (с флагом `needSave`) |

**DeviceController НЕ ЗНАЕТ:**
- О ConfigManager (получает Config через `init()`)
- О транспорте (MQTT, Web, Zigbee, Matter)
- О EEPROM
- О пинах (управляет через Actuator)

**Важно:** Все команды поступают через колбэки, зарегистрированные оркестратором. Какой именно транспорт используется (MQTT, Zigbee, Matter) — не важно для DeviceController. Преобразование протокольных команд в универсальные — ответственность транспорта.

**DeviceController ЗНАЕТ:**
- Структуру `ConfigData` (для чтения порогов, таймеров)
- Структуру `OperationalState` (для хранения состояния)
- Интерфейсы `Sensor` и `Actuator`

---

## 3. Структуры данных

### 3.1. OperationalState (RAM)

```cpp
struct OperationalState {
    // ===== Управляемые пользователем =====
    bool isOn;                   // Состояние актуатора
    uint8_t speed;               // Текущая скорость (0-100%)
    bool manualMode;             // Ручной режим (true = ручное управление)
    bool timerMode;              // Работа по таймеру отложенного включения
    uint32_t delayRemainSec;     // Остаток до срабатывания таймера (сек)
};
```

**Примечания:**
- `adaptiveMode` хранится в `Config`, а не в `OperationalState`
- Ручной режим — оперативное состояние (RAM), при его включении Config **НЕ МЕНЯЕТСЯ**
- Скорость `speed` — текущая оперативная скорость, при выключении сбрасывается до `config.speedPercent`

### 3.2. Поля, которые DeviceController НЕ хранит

| Поле | Где вычисляется | Как получить |
|------|-----------------|--------------|
| `emergency` | Actuator | `actuator.isEmergencyStop()` |
| `onTimeSec` | Actuator | `actuator.getOnTimeSec()` |
| `sensorValid` | Sensor | `sensor.isOk()` |
| `temperature` | Sensor | `sensor.getTemperature()` |
| `humidity` | Sensor | `sensor.getHumidity()` |

---

## 4. API

### 4.1. Инициализация

```cpp
void init(const ConfigData* config, Sensor* sensor, Actuator* actuator);
```

**Действия:**
1. Сохранить указатели на `Sensor` и `Actuator`
2. Скопировать `Config` в `_state.config`
3. Инициализировать `OperationalState` из Config:
   - `isOn = config.bootState`
   - `speed = config.speedPercent`
   - `manualMode = !config.sensorControlMode` (если AUTO выключен → ручной режим)
   - `timerMode = false`
   - `delayRemainSec = 0`

### 4.2. Основной цикл

```cpp
void update();
```

**Действия (выполняются в порядке приоритета):**

1. **Чтение датчика** — через `sensor->getReading()`
2. **Проверка аварийного отключения** — через `actuator->isEmergencyStop()`
3. **Адаптивный режим** (если включён в Config и НЕ manualMode) — вычисление скорости
4. **Управление по порогам** (если НЕ manualMode и датчик валиден)
5. **Таймер отложенного включения** — отсчёт и включение
6. **Вызов колбэка** при изменении состояния

### 4.3. Команды

**Постоянные параметры (сохраняются в EEPROM):**

```cpp
void setPermanentParam(ParamType type, float value);
```

| Параметр | Тип | Диапазон | Действие |
|----------|-----|----------|----------|
| `PARAM_DELAY_SEC` | int | 0-86400 | Меняет `_state.config.delaySeconds`. Вызывает колбэк с `needSave = true` |
| `PARAM_MAX_ON_TIME` | uint32_t | 0-86400 | Меняет `_state.config.maxOnTime`. Вызывает колбэк с `needSave = true` |
| `PARAM_LOW_TEMP` | float | -40..85 | Меняет `_state.config.lowTemp`. Вызывает колбэк с `needSave = true` |
| `PARAM_HIGH_TEMP` | float | -40..85 | Меняет `_state.config.highTemp`. Вызывает колбэк с `needSave = true` |
| `PARAM_LOW_HUM` | float | 0..100 | Меняет `_state.config.lowHum`. Вызывает колбэк с `needSave = true` |
| `PARAM_HIGH_HUM` | float | 0..100 | Меняет `_state.config.highHum`. Вызывает колбэк с `needSave = true` |
| `PARAM_SENSOR_CONTROL_MODE` | bool | true/false | Меняет `_state.config.sensorControlMode`. Вызывает колбэк с `needSave = true` |
| `PARAM_ADAPTIVE_MODE` | bool | true/false | Меняет `_state.config.adaptiveMode`. Вызывает колбэк с `needSave = true` |
| `PARAM_SPEED_PERCENT` | uint16_t | 0-100 | Меняет `_state.config.speedPercent`. Вызывает колбэк с `needSave = true` |
| `PARAM_BOOT_STATE` | bool | true/false | Меняет `_state.config.bootState`. Вызывает колбэк с `needSave = true` |

**Действие:** меняет `_state.config`, вызывает колбэк с `needSave = true`. Оркестратор сохраняет Config в EEPROM.

---

**Оперативные параметры (только RAM):**

```cpp
void setOperationalParam(ParamType type, float value);
```

| Параметр | Тип | Диапазон | Действие | `needSave` |
|----------|-----|----------|----------|------------|
| `PARAM_STATE` | bool | true/false | Меняет `isOn`, вызывает `actuator->set()`. При любой команде (ON или OFF) устанавливает `manualMode = true`. Config **НЕ МЕНЯЕТСЯ**. | `false` |
| `PARAM_SPEED` | uint8_t | 0-100 | Меняет `speed`. Если `manualMode == false` — включает `manualMode`. Config **НЕ МЕНЯЕТСЯ**. | `false` |
| `PARAM_MANUAL_MODE` | bool | true/false | Включает/выключает ручной режим. Config **НЕ МЕНЯЕТСЯ**. | `false` |

**Важно:** Любая команда управления состоянием (`PARAM_STATE`) автоматически переводит устройство в ручной режим (`manualMode = true`). Это соответствует принципу «ручной режим — наивысший приоритет» из PROJECT.md. Config при этом **НЕ МЕНЯЕТСЯ**.

---

### 4.4. Чтение состояния

```cpp
const DeviceState* getState() const;
```

Возвращает указатель на полное состояние (`Config + OperationalState`).

### 4.5. Колбэк

```cpp
typedef void (*StateCallback)(const DeviceState* state, bool needSave);
void setStateCallback(StateCallback callback);
```

- `needSave = true` — изменился постоянный параметр (нужно сохранить в EEPROM)
- `needSave = false` — изменился только оперативный параметр (только публикация)

---

## 5. Бизнес-логика (подробно)

### 5.1. Приоритет режимов

| Приоритет | Режим | Описание |
|-----------|-------|----------|
| **1 (высший)** | `EMERGENCY` | Аварийное отключение (блокирует всё). Работает во всех режимах. |
| **2** | `manualMode = true` | Ручное управление (игнорирует датчик и таймеры). Таймер аварийного отключения продолжает работать. |
| **3** | `timerMode` | Отложенное включение (при срабатывании → `manualMode = true`) |
| **4 (низший)** | `manualMode = false` | Автоматическое управление по порогам датчика |

**Важно:**
- Любая команда управления состоянием (`PARAM_STATE`) автоматически переводит устройство в ручной режим (`manualMode = true`)
- Ручной режим — **оперативное состояние** (RAM). Config **НЕ МЕНЯЕТСЯ** при его включении/выключении
- Таймер аварийного отключения (`maxOnTime`) продолжает работать **во всех режимах**, включая ручной
- Данные датчиков продолжают публиковаться **во всех режимах**
- При перезагрузке устройства состояние восстанавливается из Config (ручной режим сбрасывается)

### 5.2. Автоматический режим (пороги)

**Условия (только если `manualMode == false` и датчик валиден):**

```
Включение:  (temperature >= highTemp) ИЛИ (humidity >= highHum)
Выключение: (temperature < lowTemp) И (humidity < lowHum)
```

**При выключении по порогам:**
- Скорость устанавливается в `config.speedPercent` (значение из Config)
- Публикуется новое состояние (isOn = false, speed = config.speedPercent)

### 5.3. Адаптивный режим

**Условия работы:**
- `config.adaptiveMode == true` (включён в конфигурации)
- `manualMode == false` (ручной режим выключен)
- `isOn == true`
- **`speed < 100`** — адаптивный режим работает только при активном тихом режиме
- Датчик валиден

**Логика:**
1. Стартовая скорость — `config.speedPercent` (должна быть < 100)
2. При ухудшении показаний — увеличение скорости (шаг 10%)
3. При улучшении — снижение (но не ниже `config.speedPercent`)
4. При достижении 100% — адаптивный режим отключается (тихий режим выключен)
5. Интервал проверки — `config.sensorInterval`

**Блокировка (игнорирование, НЕ отключение в Config):**

Адаптивный режим **игнорируется** (но не отключается в Config) при:
- Включении ручного режима (`manualMode = true`)
- Любой команде управления состоянием (`PARAM_STATE`)
- Ручном изменении скорости (`PARAM_SPEED`)
- Установке скорости 100% (полная мощность — тихий режим выключен)

**Важно:** При выходе из ручного режима (`manualMode = false`) адаптивный режим автоматически возобновляет работу, если `config.adaptiveMode == true`.

### 5.4. Таймер отложенного включения

```
1. При старте, если config.delaySeconds > 0:
   - Запускается таймер
   - Актуатор выключен

2. По истечении delaySeconds:
   - Включается актуатор
   - manualMode = true (переход в ручной режим)
   - timerMode = false

3. В ручном режиме таймер игнорируется

4. Возврат в автоматический режим — только по команде пользователя или перезагрузке
```

### 5.5. Аварийное отключение (emergency)

- Вычисляется в **Actuator** (на основе `maxOnTime`)
- DeviceController **читает** `actuator.isEmergencyStop()`
- При `emergency == true`:
  - Блокируется включение актуатора
  - Устанавливается `STATE_EMERGENCY` в SystemState (через колбэк)
  - Сброс — только при ручном включении

**Важно:** Аварийное отключение работает **во всех режимах**, включая ручной. Даже если пользователь включил вентилятор вручную, `maxOnTime` продолжает отсчитываться и при превышении выключит нагрузку.

### 5.6. Тихий режим (Slow Mode)

Тихий режим — это работа вентилятора на скорости ниже 100% с использованием ШИМ.

**Условия:**
- `speed < 100` — тихий режим активен
- `speed = 100` — тихий режим выключен (полная мощность)
- `speed = 0` — вентилятор выключен

**Адаптивный режим работает только при активном тихом режиме** (`speed < 100`).

### 5.7. Восстановление скорости

При выключении вентилятора (по любой причине) скорость всегда устанавливается в значение по умолчанию из Config:

```
speed = config.speedPercent
```

**Сценарии:**
1. **Выключение по порогам** — `speed = config.speedPercent`
2. **Выключение по таймеру аварийного отключения** — `speed = config.speedPercent`
3. **Ручное выключение** — `speed = config.speedPercent` (при следующем включении)
4. **Выход из ручного режима** — `speed = config.speedPercent`
5. **Перезагрузка** — `speed = config.speedPercent` (из Config)
6. **Команда state = OFF через MQTT/Web** — `speed = config.speedPercent`

**Исключение:** если вентилятор был выключен с сохранением текущей скорости (например, через MQTT), при следующем включении используется `config.speedPercent`, а не запомненная скорость.

---

## 6. Потоки данных

### 6.1. Инициализация при старте

```
1. Оркестратор загружает Config из EEPROM
2. Оркестратор создаёт DeviceController
3. Оркестратор вызывает deviceController.init(&config, &sensor, &actuator)
4. DeviceController копирует config в _state.config
5. DeviceController инициализирует _state.op из config
6. DeviceController сохраняет указатели на sensor и actuator
```

### 6.2. Изменение постоянного параметра

```
1. Команда через MQTT/Web/Zigbee/Matter → оркестратор
2. Оркестратор → deviceController.setPermanentParam(PARAM_DELAY_SEC, 120)
3. DeviceController:
   a. Меняет _state.config.delaySeconds = 120
   b. Вызывает _stateCallback(&_state, true)
4. Оркестратор в колбэке:
   a. config_save(&_state.config) → запись в EEPROM
   b. transport->publish() → публикация
```

### 6.3. Изменение оперативного параметра (команда state)

```
1. Команда через MQTT/Web/Zigbee/Matter: state = ON → оркестратор
2. Оркестратор → deviceController.setOperationalParam(PARAM_STATE, true)
3. DeviceController:
   a. Меняет _state.op.isOn = true
   b. Вызывает actuator->set(true)
   c. Устанавливает _state.op.manualMode = true
   d. Вызывает _stateCallback(&_state, false)   // needSave = false, Config не менялся
4. Оркестратор в колбэке:
   a. НЕ вызывает config_save()
   b. transport->publish() → публикация
```

**При выключении (state = OFF):**
- `speed` устанавливается в `config.speedPercent`
- Публикуется состояние со `speed = config.speedPercent`
- `manualMode = true` (ручной режим включается даже при выключении)

### 6.4. Изменение оперативного параметра (скорость)

```
1. Команда через MQTT/Web/Zigbee/Matter: speed = 75 → оркестратор
2. Оркестратор → deviceController.setOperationalParam(PARAM_SPEED, 75)
3. DeviceController:
   a. Меняет _state.op.speed = 75
   b. Если manualMode == false:
      - Устанавливает manualMode = true
   c. Вызывает actuator->setSpeed(75)
   d. Вызывает _stateCallback(&_state, false)   // needSave = false
4. Оркестратор в колбэке:
   a. НЕ вызывает config_save()
   b. transport->publish() → публикация
```

### 6.5. Команда через Zigbee/Matter (общий поток)

```
1. Zigbee-координатор / Matter-контроллер отправляет команду
2. ZigbeeTransport / MatterTransport получает команду (ZCL-команда / Matter-команда)
3. Транспорт преобразует команду в универсальный вызов колбэка (TransportBoolCallback / TransportIntCallback и т.д.)
4. Оркестратор получает колбэк
5. Оркестратор вызывает DeviceController.setOperationalParam() или setPermanentParam()
6. DeviceController обрабатывает команду (без знания о том, откуда она пришла)
```

**Важно:** DeviceController не знает о Zigbee или Matter. Он работает только с универсальными командами (`PARAM_STATE`, `PARAM_SPEED`, `PARAM_LOW_TEMP` и т.д.). Преобразование протокольных команд (ZCL/Matter) в универсальные — ответственность транспорта.

---

## 7. Интеграция с оркестратором

```cpp
// main.cpp

static DeviceController deviceController;
static Sensor* g_sensor;
static Actuator* g_actuator;

void onDeviceStateChange(const DeviceState* state, bool needSave) {
    if (needSave) {
        Config config;
        config_syncFromState(state, &config);
        config_save(&config);
    }
    transport->publish(buildPublicationData(state));
}

void setup() {
    // ... инициализация Logger, ConfigManager, Sensor, Actuator ...
    
    deviceController.init(&config, &sensor, &actuator);
    deviceController.setStateCallback(onDeviceStateChange);
}

void loop() {
    // ... другие update() ...
    
    deviceController.update();
    
    // ... остальное ...
}
```

---

## 8. Требования к реализации

| # | Задача | Приоритет |
|---|--------|-----------|
| 1 | Вынести бизнес-логику из `main.cpp` в `DeviceController` | 🔴 Критично |
| 2 | Реализовать `OperationalState` как отдельную структуру | 🔴 Критично |
| 3 | Реализовать обработку всех команд | 🔴 Критично |
| 4 | Реализовать колбэк с `needSave` | 🟠 Высокий |
| 5 | Реализовать адаптивный режим | 🟠 Высокий |
| 6 | Реализовать таймер отложенного включения | 🟠 Высокий |
| 7 | Добавить юнит-тесты для бизнес-логики | 🟡 Средний |

---

*Конец документа*