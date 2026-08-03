```cpp
/**
 * @file CONFIG_AND_STATE.md
 * @brief Конфигурация и состояние устройства
 * @deprecated See STATUS_PROVIDER.md
 */
```

# CONFIG_AND_STATE.md

## Конфигурация и состояние устройства

## 1. Назначение

Документ описывает **две ключевые структуры данных** устройства:

- **`ConfigData`** — постоянные параметры, хранящиеся в EEPROM
- **`OperationalState`** — оперативное состояние, хранящееся в RAM

А также правила их изменения, синхронизации и сохранения.

---

## 2. Принцип разделения

| Характеристика | `ConfigData` | `OperationalState` |
|----------------|--------------|-------------------|
| **Хранилище** | EEPROM (энергонезависимое) | RAM (энергозависимое) |
| **Переживает перезагрузку** | Да | Нет |
| **Изменяется через MQTT/Web** | Да (постоянные параметры) | Да (оперативные параметры) |
| **Изменяется внутренней логикой** | Нет | Да |
| **Кто владеет** | `ConfigManager` | `DeviceController` |
| **Кто сохраняет** | `ConfigManager::save()` | Не сохраняется |

---

## 3. Структура ConfigData (EEPROM)

### 3.1. Полное описание полей

```cpp
struct ConfigData {
    // ===== Служебные =====
    uint16_t magic;            // 0x5A6D — маркер валидности
    uint16_t crc;              // Контрольная сумма (CRC16)
    
    // ===== WiFi (только если TRANSPORT_TYPE == WIFI) =====
    char wifiSsid[32];         // Имя WiFi сети (SSID)
    char wifiPassword[64];     // Пароль WiFi
    
    // ===== MQTT (только если FEATURE_MQTT_ENABLED == 1) =====
    char mqttBroker[64];       // Адрес брокера (IP или домен)
    uint16_t mqttPort;         // Порт (по умолчанию 1883)
    char mqttUser[32];         // Имя пользователя (может быть пустым)
    char mqttPassword[64];     // Пароль (может быть пустым)
    char mqttClientId[24];     // Уникальный ID клиента
    
    // ===== Zigbee (только если TRANSPORT_TYPE == ZIGBEE) =====
    char zigbeeNetworkKey[32]; // Сетевой ключ
    uint16_t zigbeePanId;      // PAN ID сети
    uint8_t zigbeeChannel;     // Канал (11-26)
    
    // ===== Общие для TYPE 1 и TYPE 3 =====
    int delaySeconds;          // Задержка отложенного включения (сек), 0 = отключено
    uint32_t maxOnTime;        // Аварийное отключение (сек), 0 = отключено
    bool bootState;            // Состояние при старте (true = включено)
    
    // ===== TYPE 1 (вентилятор с датчиком) =====
    float lowTemp;             // Нижний порог температуры (°C)
    float highTemp;            // Верхний порог температуры (°C)
    float lowHum;              // Нижний порог влажности (%)
    float highHum;             // Верхний порог влажности (%)
    bool sensorControlMode;    // Режим AUTO (true) / MANUAL (false)
    uint16_t speedPercent;     // Скорость по умолчанию (0-100%)
    bool adaptiveMode;         // Адаптивный режим включён (true/false)
    
    // ===== TYPE 1 и TYPE 2 (датчик) =====
    uint16_t sensorInterval;   // Интервал опроса датчика (сек)
};
```

### 3.2. Диапазоны валидации

| Поле | Минимум | Максимум | Значение по умолчанию |
|------|---------|----------|----------------------|
| `lowTemp` | -40°C | `highTemp` - 0.1 | 27.0°C |
| `highTemp` | `lowTemp` + 0.1 | 85°C | 29.0°C |
| `lowHum` | 0% | `highHum` - 0.1 | 55% |
| `highHum` | `lowHum` + 0.1 | 100% | 60% |
| `speedPercent` | 0 | 100 | 50% |
| `delaySeconds` | 0 | 86400 | 60 сек |
| `maxOnTime` | 0 | 86400 | 3600 сек |
| `sensorInterval` | 1 | 3600 | 10 сек |
| `mqttPort` | 1 | 65535 | 1883 |

### 3.3. Значения по умолчанию (Factory Defaults)

```cpp
// ConfigManager::setDefaults()
magic = 0x5A6D;
crc = 0;

#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI
strcpy(wifiSsid, DEFAULT_WIFI_SSID);      // из credentials.h или ""
strcpy(wifiPassword, DEFAULT_WIFI_PASSWORD);
#endif

#if FEATURE_MQTT_ENABLED == 1
strcpy(mqttBroker, DEFAULT_MQTT_BROKER);   // из credentials.h или ""
mqttPort = MQTT_PORT;                      // 1883
mqttUser[0] = '\0';
mqttPassword[0] = '\0';
strcpy(mqttClientId, deviceId);
#endif

delaySeconds = DEFAULT_DELAY_SECONDS;      // 60
maxOnTime = MAX_ON_TIME_SEC;               // 3600
bootState = DEFAULT_BOOT_SWITCH_STATE;     // true

#if DEVICE_TYPE == 1
lowTemp = DEFAULT_LOW_TEMP;                // 27.0
highTemp = DEFAULT_HIGH_TEMP;              // 29.0
lowHum = DEFAULT_LOW_HUM;                  // 55.0
highHum = DEFAULT_HIGH_HUM;                // 60.0
sensorControlMode = DEFAULT_SENSOR_CONTROL_MODE; // true
speedPercent = DEFAULT_SPEED_PERCENT;      // 50
adaptiveMode = DEFAULT_ADAPTIVE_MODE;      // false
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
sensorInterval = DEFAULT_SENSOR_DURATION;  // 10
#endif
```

---

## 4. Структура OperationalState (RAM)

### 4.1. Полное описание полей

```cpp
struct OperationalState {
    // ===== Управляемые пользователем (команды) =====
    bool isOn;                   // Текущее состояние актуатора
    uint8_t speed;               // Текущая скорость (0-100%)
    ControlMode mode;            // MANUAL или AUTOMATIC
    bool adaptiveModeActive;     // Адаптивный режим включён (пользовательский флаг)
    
    // ===== Системные (управляются внутренней логикой) =====
    bool timerMode;              // Работа по таймеру отложенного включения
    uint32_t delayRemainSec;     // Остаток до срабатывания таймера (сек)
    
    // ===== Флаг для оркестратора =====
    bool configDirty;            // true — нужно сохранить Config в EEPROM
};
```

### 4.2. Поля, которые НЕ входят в OperationalState

Эти поля **вычисляются** в других слоях и передаются в DeviceController через интерфейсы:

| Поле | Где вычисляется | Как получить |
|------|-----------------|--------------|
| `emergency` | Actuator | `actuator.isEmergencyStop()` |
| `onTimeSec` | Actuator | `actuator.getOnTimeSec()` |
| `sensorValid` | Sensor | `sensor.isOk()` |
| `temperature` | Sensor | `sensor.getTemperature()` |
| `humidity` | Sensor | `sensor.getHumidity()` |
| `sensorError` | Sensor | `sensor.getError()` |

---

## 5. Разделение параметров по категориям

| Категория | Параметры | Хранилище | Изменение через MQTT/Web | Сохранение в EEPROM |
|-----------|-----------|-----------|--------------------------|---------------------|
| **Постоянные (Config)** | `bootState`, `maxOnTime`, `lowTemp`, `highTemp`, `lowHum`, `highHum`, `sensorControlMode`, `adaptiveMode`, `delaySeconds`, `speedPercent`, `sensorInterval`, `wifiSsid`, `wifiPassword`, `mqttBroker`, `mqttPort`, `mqttUser`, `mqttPassword`, `mqttClientId`, `zigbee...` | EEPROM | **ДА** (через `DeviceController::setPermanentParam()`) | **ДА** (немедленно) |
| **Оперативные (State)** | `isOn`, `speed` (ручная), `mode`, `adaptiveModeActive`, `timerMode`, `delayRemainSec` | RAM | **ДА** (через `DeviceController::setOperationalParam()`) | **НЕТ** |
| **Вычисляемые** | `emergency`, `onTimeSec`, `sensorValid`, `temperature`, `humidity`, `sensorError` | RAM | **НЕТ** (вычисляются в Actuator/Sensor) | **НЕТ** |

---

## 6. Кто и как меняет параметры

### 6.1. Постоянные параметры (изменяются через `DeviceController::setPermanentParam()`)

```cpp
void DeviceController::setPermanentParam(ParamType type, float value) {
    switch(type) {
        case PARAM_DELAY_SEC:
            _state.config.delaySeconds = (int)value;
            break;
        case PARAM_MAX_ON_TIME:
            _state.config.maxOnTime = (uint32_t)value;
            break;
        case PARAM_LOW_TEMP:
            _state.config.lowTemp = value;
            break;
        case PARAM_HIGH_TEMP:
            _state.config.highTemp = value;
            break;
        case PARAM_LOW_HUM:
            _state.config.lowHum = value;
            break;
        case PARAM_HIGH_HUM:
            _state.config.highHum = value;
            break;
        case PARAM_SENSOR_CONTROL_MODE:
            _state.config.sensorControlMode = (bool)value;
            break;
        case PARAM_ADAPTIVE_MODE:
            _state.config.adaptiveMode = (bool)value;
            break;
        case PARAM_SPEED_PERCENT:
            _state.config.speedPercent = (uint16_t)value;
            break;
        case PARAM_BOOT_STATE:
            _state.config.bootState = (bool)value;
            break;
        default:
            return;
    }
    _state.configDirty = true;  // Устанавливаем флаг
    if (_stateCallback) {
        _stateCallback(&_state, true);  // needSave = true
    }
}
```

### 6.2. Оперативные параметры (изменяются через `DeviceController::setOperationalParam()`)

```cpp
void DeviceController::setOperationalParam(ParamType type, float value) {
    switch(type) {
        case PARAM_STATE:
            _state.op.isOn = (bool)value;
            if (_state.op.isOn) {
                _actuator->on();
            } else {
                _actuator->off();
            }
            break;
        case PARAM_SPEED:
            _state.op.speed = (uint8_t)value;
            _actuator->setSpeed(_state.op.speed);
            // Ручное изменение скорости отключает адаптивный режим
            if (_state.op.adaptiveModeActive) {
                _state.op.adaptiveModeActive = false;
                XLOG_INFO(CAT_ACTUATOR, "Adaptive mode disabled by manual speed change");
            }
            break;
        case PARAM_MODE:
            _state.op.mode = (ControlMode)value;
            break;
        case PARAM_ADAPTIVE_MODE_ACTIVE:
            _state.op.adaptiveModeActive = (bool)value;
            break;
        default:
            return;
    }
    // configDirty НЕ меняется
    if (_stateCallback) {
        _stateCallback(&_state, false);  // needSave = false
    }
}
```

---

## 7. Потоки данных

### 7.1. Старт устройства (инициализация State из Config)

```
1. Оркестратор вызывает config_load(&config)
2. Если EEPROM пуст или повреждён → config = Factory Defaults
3. Оркестратор создаёт DeviceController и передаёт Config
4. DeviceController::init(&config):
   a. Копирует config в _state.config
   b. Копирует начальные значения в _state.op:
      - isOn = config.bootState
      - speed = config.speedPercent
      - mode = config.sensorControlMode ? AUTOMATIC : MANUAL
      - adaptiveModeActive = config.adaptiveMode
      - timerMode = false
      - delayRemainSec = 0
      - configDirty = false
5. Actuator инициализируется с параметрами из Config:
   - maxOnTime = config.maxOnTime
   - delaySeconds = config.delaySeconds
   - bootState = config.bootState
```

### 7.2. Изменение постоянного параметра (MQTT/Web)

```
1. Пользователь отправляет команду: {prefix}/c/delaySec = 120
2. Transport → колбэк в оркестраторе
3. Оркестратор вызывает DeviceController::setPermanentParam(PARAM_DELAY_SEC, 120)
4. DeviceController:
   a. Меняет _state.config.delaySeconds = 120
   b. Устанавливает _state.configDirty = true
   c. Вызывает _stateCallback(&_state, true)
5. Оркестратор в колбэке:
   a. Вызывает config_save(&_state.config) → запись в EEPROM
   b. Вызывает transport->publish() → публикация нового состояния
6. configDirty сбрасывается в false после сохранения
```

### 7.3. Изменение оперативного параметра (MQTT/Web)

```
1. Пользователь отправляет команду: {prefix}/c/state = ON
2. Transport → колбэк в оркестраторе
3. Оркестратор вызывает DeviceController::setOperationalParam(PARAM_STATE, true)
4. DeviceController:
   a. Меняет _state.op.isOn = true
   b. Вызывает _actuator->on()
   c. configDirty НЕ меняется (остаётся false)
   d. Вызывает _stateCallback(&_state, false)
5. Оркестратор в колбэке:
   a. НЕ вызывает config_save()
   b. Вызывает transport->publish() → публикация нового состояния
```

---

## 8. Синхронизация Actuator → Config (emergency)

`emergency` не является частью `ConfigData` или `OperationalState`. Это состояние актуатора.

```cpp
// Actuator вычисляет emergency
void Actuator::checkMaxOnTime(uint32_t maxOnTime) {
    if (maxOnTime == 0) return;
    if (_isOn && !_emergency) {
        _onTimeSec += (millis() - _lastTick) / 1000;
        if (_onTimeSec >= maxOnTime) {
            _emergency = true;
            _isOn = false;
            digitalWrite(_pin, LOW);
            if (onForceStopCallback) {
                onForceStopCallback(callbackContext);
            }
        }
    }
}

// Оркестратор получает колбэк об emergency
void onForceStopCallback(void* context) {
    // 1. Устанавливаем STATE_EMERGENCY в SystemState
    system_state_set_bit(STATE_EMERGENCY);
    
    // 2. Обновляем LED
    led_set_mode(LED_SLOW_BLINK);
    
    // 3. Публикуем статус через транспорт
    if (g_transport && g_transport->isConnected()) {
        g_transport->publishState(false);
        g_transport->publishResetReason("Emergency stop: maxOnTime exceeded");
    }
}
```

---

## 9. Итоговая таблица: параметры и их свойства

| Параметр | Структура | Категория | Хранилище | Изменяется через | Сохраняется в EEPROM |
|----------|-----------|-----------|-----------|------------------|---------------------|
| `wifiSsid` | Config | Постоянный | EEPROM | Web (/save) |   Да |
| `wifiPassword` | Config | Постоянный | EEPROM | Web (/save) |   Да |
| `mqttBroker` | Config | Постоянный | EEPROM | Web (/save) |   Да |
| `mqttPort` | Config | Постоянный | EEPROM | Web (/save) |   Да |
| `mqttUser` | Config | Постоянный | EEPROM | Web (/save) |   Да |
| `mqttPassword` | Config | Постоянный | EEPROM | Web (/save) |   Да |
| `mqttClientId` | Config | Постоянный | EEPROM | Web (/save) |   Да |
| `bootState` | Config | Постоянный | EEPROM | MQTT/Web |   Да |
| `maxOnTime` | Config | Постоянный | EEPROM | MQTT/Web |   Да |
| `delaySeconds` | Config | Постоянный | EEPROM | MQTT/Web |   Да |
| `lowTemp` | Config | Постоянный | EEPROM | MQTT/Web |   Да |
| `highTemp` | Config | Постоянный | EEPROM | MQTT/Web |   Да |
| `lowHum` | Config | Постоянный | EEPROM | MQTT/Web |   Да |
| `highHum` | Config | Постоянный | EEPROM | MQTT/Web |   Да |
| `sensorControlMode` | Config | Постоянный | EEPROM | MQTT/Web |   Да |
| `speedPercent` | Config | Постоянный | EEPROM | MQTT/Web |   Да |
| `adaptiveMode` | Config | Постоянный | EEPROM | MQTT/Web |   Да |
| `sensorInterval` | Config | Постоянный | EEPROM | Web (/save) |   Да |
| `isOn` | OperationalState | Оперативный | RAM | MQTT/Web |   Нет |
| `speed` (рук.) | OperationalState | Оперативный | RAM | MQTT/Web |   Нет |
| `mode` | OperationalState | Оперативный | RAM | MQTT/Web |   Нет |
| `adaptiveModeActive` | OperationalState | Оперативный | RAM | MQTT/Web |   Нет |
| `timerMode` | OperationalState | Системный | RAM | Внутренняя логика |   Нет |
| `delayRemainSec` | OperationalState | Системный | RAM | Внутренняя логика |   Нет |
| `configDirty` | OperationalState | Служебный | RAM | DeviceController |   Нет |
| `emergency` | — | Вычисляемый | RAM | Actuator |   Нет |
| `onTimeSec` | — | Вычисляемый | RAM | Actuator |   Нет |
| `sensorValid` | — | Вычисляемый | RAM | Sensor |   Нет |
| `temperature` | — | Вычисляемый | RAM | Sensor |   Нет |
| `humidity` | — | Вычисляемый | RAM | Sensor |   Нет |

---
