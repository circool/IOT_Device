```cpp
/**
 * @file TRANSPORT_DATA_MODEL.md
 * @brief Модель данных транспортной абстракции
 * @note Статус: Закончен
 * @version 0.11
 * @date 06.08.2026
 */
```

# TRANSPORT_DATA_MODEL.md

## Оглавление

- [1. Назначение](#1-назначение)
- [2. Структуры данных](#2-структуры-данных)
  - [2.1. TransportConfig](#21-transportconfig)
  - [2.2. DeviceConfig](#22-DeviceConfig)
  - [2.3. DeviceState](#23-devicestate)
- [3. Права доступа](#3-права-доступа)
- [4. Жизненный цикл](#4-жизненный-цикл)
- [5. Связь между структурами](#5-связь-между-структурами)

---

## 1. Назначение

Документ описывает **модель данных** транспортной абстракции: структуры, их поля, зоны ответственности и жизненный цикл.

**Цель:** Чётко определить, кто владеет данными и как они передаются между слоями.

---

## 2. Структуры данных

### 2.1. TransportConfig

**Назначение:** Параметры канала связи.

**Владелец:** Транспорт (WiFi/Zigbee/Thread).

**Хранение:** EEPROM (через ConfigManager).

```cpp
typedef struct {
    // ===== ОБЩИЕ ПАРАМЕТРЫ =====
    char deviceId[32];
    
    // ===== WiFi (USE_WIFI == 1) =====
    #ifdef USE_WIFI
        char wifiSsid[32];
        char wifiPassword[64];
    #endif
    
    // ===== MQTT (USE_MQTT == 1) =====
    #ifdef USE_MQTT
        char mqttBroker[64];
        uint16_t mqttPort;
        char mqttUser[32];
        char mqttPassword[64];
        char mqttClientId[24];
    #endif
    
    // ===== Zigbee (USE_ZIGBEE == 1) =====
    #ifdef USE_ZIGBEE
        uint16_t zigbeePanId;
        uint8_t zigbeeChannel;
        char zigbeeNetworkKey[32];
    #endif
    
    // ===== Thread (USE_THREAD == 1) =====
    #if USE_THREAD == 1
        char threadNetworkName[32];
        char threadNetworkKey[32];
        uint16_t threadPanId;
        uint8_t threadChannel;
    #endif
} TransportConfig;
```

**Кто имеет право писать:**
- Оркестратор (при получении `onConfigUpdate()`)
- HTTP-протокол (через `onConfigUpdate()`)

**Кто имеет право читать:**
- Транспорт (WiFi/Zigbee/Thread)
- HTTP-протокол (для отображения на странице настроек)

---

### 2.2. DeviceConfig

**Назначение:** Бизнес-настройки устройства.

**Владелец:** DeviceController.

**Хранение:** EEPROM (через ConfigManager).

```cpp
typedef struct {
    bool sensorMode;  // TRUE = SENSOR, FALSE = MANUAL
    bool adaptiveMode;        // TRUE = адаптивный режим включён
    float lowTemp;
    float highTemp;
    float lowHum;
    float highHum;
    uint32_t delaySec;
    uint32_t maxOnTime;
    uint8_t bootState;        // 0 = OFF, 1 = ON
} DeviceConfig;
```

**Кто имеет право писать:**
- DeviceController (при применении настроек)
- HTTP-протокол (через `onCommand(CMD_SET_ALL_SETTINGS)`)

**Кто имеет право читать:**
- DeviceController (для работы)
- HTTP-протокол (для отображения на странице настроек)
- MQTT-протокол (для публикации)

---

### 2.3. DeviceState

**Назначение:** Оперативное состояние устройства.

**Владелец:** DeviceController.

**Хранение:** RAM (обновляется в реальном времени).

```cpp
typedef struct {
    bool is_on;              // Актуатор включён
    uint8_t speed;           // 0-100%
    float temperature;
    float humidity;
    bool manual_mode;        // TRUE = пользователь переключил вручную
    bool adaptiveMode;    // TRUE = адаптивный режим активен
    uint32_t delay_remain;   // Остаток таймера задержки (сек)
    uint32_t max_on_remain;  // Остаток аварийного таймера (сек)
} DeviceState;
```

**Кто имеет право писать:**
- DeviceController (при изменении состояния)

**Кто имеет право читать:**
- Транспорт (для публикации)
- HTTP-протокол (для отображения)
- MQTT-протокол (для публикации)
- LED (для индикации)
- Оркестратор (для принятия решений)

---

## 3. Права доступа (таблица)

| Структура | Пишет | Читает |
|-----------|-------|--------|
| **TransportConfig** | Оркестратор, HTTP | Транспорт, HTTP |
| **DeviceConfig** | DeviceController, HTTP | DeviceController, HTTP, MQTT |
| **DeviceState** | DeviceController | Транспорт, HTTP, MQTT, LED, Оркестратор |

---

## 4. Жизненный цикл

### 4.1. TransportConfig

```
1. Загрузка из EEPROM (при старте)
2. Используется транспортом для подключения
3. При изменении (через HTTP/MQTT):
   a. Сохраняется в EEPROM
   b. Транспорт перезапускается с новыми параметрами
```

### 4.2. DeviceConfig

```
1. Загрузка из EEPROM (при старте)
2. Передаются в DeviceController
3. При изменении (через HTTP/MQTT):
   a. Применяются в DeviceController
   b. Сохраняются в EEPROM
   c. Не требуют перезапуска транспорта
```

### 4.3. DeviceState

```
1. Инициализируется при старте
2. Обновляется DeviceController в реальном времени
3. При изменении → транспорт публикует через publishState()
4. Не сохраняется в EEPROM
```

---

## 5. Связь между структурами

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              ConfigManager                                  │
│                                                                             │
│  • Хранит TransportConfig и DeviceConfig в EEPROM                        │
│  • Загружает при старте                                                    │
│  • Сохраняет при изменении                                                 │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    │
          ┌─────────────────────────┼─────────────────────────┐
          │                         │                         │
          ▼                         ▼                         ▼
┌───────────────────┐    ┌───────────────────┐    ┌─────────────────────────┐
│  Transport        │    │  DeviceController │    │  HTTP/MQTT (чтение)     │
│                   │    │                   │    │                         │
│  • Читает         │    │  • Читает         │    │  • Читают для           │
│    TransportConfig│    │    DeviceConfig │    │    отображения          │
│  • НЕ читает      │    │  • Пишет          │    │  • НЕ пишут напрямую    │
│    DeviceConfig │    │    DeviceState    │    │                         │
└───────────────────┘    └───────────────────┘    └─────────────────────────┘
```

---

*Конец документа*