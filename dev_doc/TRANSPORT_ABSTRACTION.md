```cpp
/**
 * @file TRANSPORT_ABSTRACTION.md
 * @brief Транспортная абстракция
 * @note Статус: Закончен
 * @todo Необходима актуализация - см STATUS_PROVIDER.md
 */
```
# TRANSPORT_ABSTRACTION.md

## Транспортная абстракция

## 1. Назначение

Документ описывает **единый интерфейс** для всех каналов связи (MQTT, Zigbee, Matter, Web).

**Цель:** Оркестратор работает с транспортом через единый интерфейс `Transport`, не привязываясь к конкретной реализации.

---

## 2. Принципы

| Принцип | Описание |
|---------|----------|
| **Единый интерфейс** | Все транспорты реализуют структуру `Transport` с одинаковыми методами |
| **Указатели на функции** | Вместо `std::function` — экономия RAM |
| **Контекст** | Каждый колбэк принимает `void* context` для передачи состояния |
| **Компиляция** | Выбор транспорта — через `#if` в `transport_factory.h` |
| **Независимость** | Транспорт не знает о бизнес-логике, только о командах и публикациях |

### 2.1 Взаимодействие с оркестратором

Транспорная абстракция сообщает оркестратору о возможности при инициализации - когда все необходимые состаляющие готовы к работе абстракция выхывает необходимые слои и инициирует их работу - подключение, колбеки команд управления или коррекции настроек

При неудачной инициализации транспорта оркестратор начинает процедуру провизионинга, которая формирует предпосылки успешной инициализации транспортной абстракции (регистрацию в mesh-сети или wifi).

## 3. Структура Transport

```cpp
typedef struct Transport {
    // ===== УПРАВЛЕНИЕ =====
    bool (*begin)(Client* client, const ConfigData* config);
    void (*update)();
    bool (*isConnected)();
    void (*disconnect)();
    const char* (*getName)();

    // ===== ПУБЛИКАЦИЯ (устройство → сеть) =====
    void (*publishOnline)();
    void (*publishState)(bool on);
    void (*publishSpeed)(int percent);
    void (*publishDelaySec)(int seconds);
    void (*publishMaxOnTime)(uint32_t seconds);
    void (*publishSensorControlMode)(bool enabled);
    
    void (*publishSensor)(float temp, float hum);          // TYPE 1,2
    void (*publishAdaptiveMode)(bool enabled);              // TYPE 1
    void (*publishThresholds)(float lowTemp, float highTemp, 
                              float lowHum, float highHum); // TYPE 1
    void (*publishRSSI)(int rssi);
    void (*publishVersion)(const char* version);
    void (*publishResetReason)(const char* reason);

    // ===== РЕГИСТРАЦИЯ КОЛБЭКОВ (сеть → устройство) =====
    void (*onState)(TransportBoolCallback callback, void* context);
    void (*onSpeed)(TransportIntCallback callback, void* context);
    void (*onDelaySec)(TransportIntCallback callback, void* context);
    void (*onMaxOnTime)(TransportUintCallback callback, void* context);
    void (*onSensorControlMode)(TransportBoolCallback callback, void* context);
    
    void (*onAdaptiveMode)(TransportBoolCallback callback, void* context);  // TYPE 1
    void (*onLowTemp)(TransportFloatCallback callback, void* context);      // TYPE 1
    void (*onHighTemp)(TransportFloatCallback callback, void* context);     // TYPE 1
    void (*onLowHum)(TransportFloatCallback callback, void* context);       // TYPE 1
    void (*onHighHum)(TransportFloatCallback callback, void* context);      // TYPE 1
    
    void (*onReset)(TransportVoidCallback callback, void* context);         // опционально
} Transport;
```

---

## 4. Типы колбэков

```cpp
// transport_types.h
typedef void (*TransportBoolCallback)(bool value, void* context);
typedef void (*TransportIntCallback)(int value, void* context);
typedef void (*TransportUintCallback)(uint32_t value, void* context);
typedef void (*TransportFloatCallback)(float value, void* context);
typedef void (*TransportVoidCallback)(void* context);
```

**Контейнер для хранения колбэка с контекстом:**

```cpp
template <typename T>
struct TransportCallback {
    T func = nullptr;
    void* context = nullptr;
};
```

Используется в `mqtt_manager.h`:
```cpp
struct BoolCb {
    BoolCallback func = nullptr;
    void* context = nullptr;
};
```

---

## 5. Фабрика транспортов

**Назначение:** выбор реализации на этапе компиляции.

```cpp
// transport_factory.h
inline Transport* createTransport() {
    #if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI
        #if FEATURE_MQTT_ENABLED == 1
            return getMQTTTransport();
        #else
            return nullptr;
        #endif
    #elif TRANSPORT_TYPE == TRANSPORT_TYPE_ZIGBEE
        return getZigbeeTransport();
    #elif TRANSPORT_TYPE == TRANSPORT_TYPE_THREAD
        #warning "Thread transport not implemented yet"
        return nullptr;
    #else
        return nullptr;
    #endif
}
```

**Использование в оркестраторе:**

```cpp
// main.cpp
Transport* g_transport = createTransport();
if (g_transport && g_transport->begin(&wifiClient, config)) {
    g_transport->publishOnline();
}
```

---

| Транспорт | Файл | Статус | Описание |
|-----------|------|--------|----------|
| **MQTT** | `mqtt_manager.h/cpp` | ✅ Реализован | Адаптер над `MQTTManager`, реализует структуру `Transport`. Все команды и публикации реализованы. |
| **Zigbee** | `zigbee_manager.h/cpp` | ⏳ Заглушка | Реализация отсутствует. Ожидает ESP-Zigbee-SDK. |
| **Thread** | — | ⏳ Не реализован | Для Matter over Thread. Ожидает ESP-Matter. |
| **Web** | `web_manager.h/cpp` | ⏳ Частично | HTTP-сервер и провайдеры реализованы. Требует устранения прямых вызовов ConfigManager, sensor_*(), ESP.restart(). |

---

## 7. Web — синхронный транспорт

Web **не реализует** структуру `Transport`, потому что:

| Характеристика | MQTT/Zigbee | Web (HTTP) |
|----------------|-------------|------------|
| **Тип** | Асинхронный (подписка/публикация) | Синхронный (запрос/ответ) |
| **Команды** | Через колбэки (подписка) | Через HTTP-запросы (`/set`, `/save`) |
| **Данные** | Публикуются в топики | Отображаются на страницах |
| **Интерфейс** | `Transport` | `IWebStatusProvider` + флаги |

**Интеграция Web с оркестратором:**

```cpp
// web_manager.h — флаги для команд
extern volatile bool g_webConfigPending;
extern volatile bool g_webRestartPending;
extern ConfigData g_webPendingConfig;

// main.cpp — оркестратор читает флаги в loop()
if (g_webConfigPending) {
    config_save(&g_webPendingConfig);
    g_webConfigPending = false;
    restart_request(1000);
}
```

---

## 8. Потоки данных

### 8.1. Команда от транспорта к устройству

```
1. Внешняя система отправляет команду (MQTT/Zigbee/HTTP)
2. Транспорт парсит команду
3. Транспорт вызывает зарегистрированный колбэк
4. Оркестратор получает колбэк и вызывает DeviceController
5. DeviceController обрабатывает команду
```

**Пример (MQTT):**
```
MQTT брокер → {prefix}/c/state = ON
MQTTManager::callback() → _onState.func(true, _onState.context)
Оркестратор: onStateCommand(true, context) → deviceController->setState(true)
```

### 8.2. Публикация от устройства к транспорту

```
1. DeviceController изменяет состояние
2. DeviceController вызывает _stateCallback(&_state, needSave)
3. Оркестратор получает колбэк
4. Оркестратор вызывает transport->publishState() (или другую публикацию)
5. Транспорт сериализует и отправляет
```

---

## 9. Обработка ошибок

| Сценарий | Действие |
|----------|----------|
| **Транспорт не инициализирован** | `createTransport()` возвращает `nullptr` → оркестратор работает без транспорта |
| **Потеря соединения** | Транспорт пытается переподключиться внутри `update()` |
| **Ошибка публикации** | Транспорт логирует ошибку, но не влияет на работу устройства |
| **Некорректная команда** | Транспорт игнорирует команду и логирует ошибку |

---

## 10. Требования к новым транспортам

При добавлении нового транспорта (например, Matter):

1. **Реализовать все методы структуры `Transport`**
2. **Добавить фабрику в `transport_factory.h`**
3. **Реализовать сериализацию данных** (формат протокола)
4. **Поддерживать колбэки** для команд от сети
5. **Не зависеть от бизнес-логики** (только команды и публикации)

---

## 11. Итоговая таблица

| Транспорт | Интерфейс | Статус | Примечание |
|-----------|-----------|--------|------------|
| **MQTT** | `Transport` | ✅ Реализован | Адаптер над `MQTTManager` |
| **Web** | `IWebStatusProvider` + флаги | ✅ Реализован | Синхронный, отдельный слой |
| **Zigbee** | `Transport` | ⏳ Заглушка |  |
| **Matter** | `Transport` | ⏳ Не реализован |  |

---
