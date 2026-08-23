```cpp
/**
 * @file HOW_TO_ADD_NEW_TRANSPORT.md
 * @brief Инструкция по добавлению нового транспорта
 * @note Статус: Закончен
 * @version 0.11
 * @date 06.08.2026
 */
```

# HOW_TO_ADD_NEW_TRANSPORT.md

## Оглавление

- [1. Назначение](#1-назначение)
- [2. Шаг 1: Создать структуру транспорта](#2-шаг-1-создать-структуру-транспорта)
- [3. Шаг 2: Реализовать методы управления](#3-шаг-2-реализовать-методы-управления)
- [4. Шаг 3: Реализовать публикацию](#4-шаг-3-реализовать-публикацию)
- [5. Шаг 4: Реализовать колбэки](#5-шаг-4-реализовать-колбэки)
- [6. Шаг 5: Добавить в фабрику](#6-шаг-5-добавить-в-фабрику)
- [7. Шаг 6: Добавить флаги компиляции](#7-шаг-6-добавить-флаги-компиляции)
- [8. Шаг 7: Добавить в документацию](#8-шаг-7-добавить-в-документацию)

---

## 1. Назначение

Документ описывает процесс добавления нового транспорта в проект.

**Транспорт** — это реализация транспортной абстракции для конкретной среды (WiFi, Zigbee, Thread, LoRa и т.д.).

**Перед началом:** Убедитесь, что вы знакомы с `TRANSPORT_ABSTRACTION.md` и `TRANSPORT_WIFI.md`.

---

## 2. Шаг 1: Создать структуру транспорта

Создайте файлы `transport_<name>.h` и `transport_<name>.cpp` в директории `src/transport/`.

### transport_<name>.h

```cpp
#ifndef TRANSPORT_<NAME>_H
#define TRANSPORT_<NAME>_H

#include "transport.h"
#include "transport_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    // ===== СОСТОЯНИЕ =====
    bool initialized;
    bool connected;
    const TransportConfig* config;
    
    // ===== КОЛБЭКИ (хранятся для вызова) =====
    TransportCommandCallback commandCallback;
    TransportConfigCallback configCallback;
    
    // ===== ВНУТРЕННИЕ ДАННЫЕ (специфичные для транспорта) =====
    // Добавьте свои поля здесь
    // ...
} <Name>Transport;

// ===== ПУБЛИЧНАЯ ФУНКЦИЯ =====
Transport* get<Name>Transport();

#ifdef __cplusplus
}
#endif

#endif  // TRANSPORT_<NAME>_H
```

---

## 3. Шаг 2: Реализовать методы управления

### transport_<name>.cpp

```cpp
#include "transport_<name>.h"
#include "logger.h"

// ===== СТАТИЧЕСКИЙ ЭКЗЕМПЛЯР =====
static <Name>Transport g_<name>Transport;

// ===== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ =====
static const char* getTransportName() {
    return "<Name>";
}

// ===== УПРАВЛЕНИЕ =====

static bool transport_begin(const TransportConfig* config) {
    if (!config) {
        XLOG_ERROR(CAT_TRANSPORT, "[%s] config is NULL", getTransportName());
        return false;
    }
    
    g_<name>Transport.config = config;
    g_<name>Transport.initialized = true;
    
    XLOG_INFO(CAT_TRANSPORT, "[%s] begin() called", getTransportName());
    
    // 1. Проверяем конфигурацию
    if (/* нет конфигурации для среды */) {
        // Переключаемся в режим настройки
        StateProvider::getInstance().set_setup_mode(true); // @deprecated StateProvider see TransportState
        g_<name>Transport.connected = false;
        return true;
    }
    
    // 2. Пытаемся подключиться к среде
    bool result = /* подключение к среде */;
    g_<name>Transport.connected = result;
    
    // 3. Обновляем флаги
    StateProvider::getInstance().set_link_ok(result);// @deprecated StateProvider see TransportState
    
    if (result) {
        StateProvider::getInstance().set_setup_mode(false);// @deprecated StateProvider see TransportState
    }
    
    return result;
}

static void transport_update() {
    if (!g_<name>Transport.initialized) {
        return;
    }
    
    const DeviceState* state = StateProvider::getInstance().get_state();// @deprecated StateProvider see TransportState
    
    // ===== РЕЖИМ НАСТРОЙКИ =====
    if (state->setup_mode) {
        // Обработка настройки (сбор данных)
        // ...
        return;
    }
    
    // ===== ОБЫЧНЫЙ РЕЖИМ =====
    // 1. Поддерживать соединение
    // 2. Обновлять флаги состояния
    // 3. При потере связи → переключиться в режим настройки
    // ...
}

static bool transport_isConnected() {
    return g_<name>Transport.connected;
}

static void transport_disconnect() {
    XLOG_INFO(CAT_TRANSPORT, "[%s] disconnect() called", getTransportName());
    g_<name>Transport.connected = false;
    StateProvider::getInstance().set_link_ok(false);// @deprecated @deprecated StateProvider see TransportState
}

static const char* transport_getName() {
    return getTransportName();
}
```

---

## 4. Шаг 3: Реализовать публикацию

```cpp
// ===== ПУБЛИКАЦИЯ =====

static void transport_publishState(const DeviceState* state) {
    XLOG_DEBUG(CAT_TRANSPORT, "[%s] publishState() called", getTransportName());
    // Отправить состояние в сеть
}

static void transport_publishSettings(const DeviceConfig* settings) {
    XLOG_DEBUG(CAT_TRANSPORT, "[%s] publishSettings() called", getTransportName());
    // Отправить настройки в сеть
}
```

---

## 5. Шаг 4: Реализовать колбэки

```cpp
// ===== КОЛБЭКИ =====

static void transport_onCommand(TransportCommandCallback callback) {
    XLOG_DEBUG(CAT_TRANSPORT, "[%s] onCommand() registered", getTransportName());
    g_<name>Transport.commandCallback = callback;
}

static void transport_onConfigUpdate(TransportConfigCallback callback) {
    XLOG_DEBUG(CAT_TRANSPORT, "[%s] onConfigUpdate() registered", getTransportName());
    g_<name>Transport.configCallback = callback;
}
```

---

## 6. Шаг 5: Собрать структуру Transport

```cpp
// ===== ИНИЦИАЛИЗАЦИЯ СТРУКТУРЫ TRANSPORT =====

static Transport g_transportImpl = {
    .begin = transport_begin,
    .update = transport_update,
    .isConnected = transport_isConnected,
    .disconnect = transport_disconnect,
    .getName = transport_getName,
    
    .publishState = transport_publishState,
    .publishSettings = transport_publishSettings,
    
    .onCommand = transport_onCommand,
    .onConfigUpdate = transport_onConfigUpdate,
};

// ===== ПУБЛИЧНАЯ ФУНКЦИЯ =====

Transport* get<Name>Transport() {
    return &g_transportImpl;
}
```

---

## 7. Шаг 6: Добавить в фабрику

В `transport_factory.h`:

```cpp
#include "settings.h"
#include "transport.h"

#ifdef USE_<NAME>
    #include "transport_<name>.h"
#endif

inline Transport* createTransport() {
    #ifdef USE_WIFI
        return getWiFiTransport();
    #elif USE_ZIGBEE == 1
        return getZigbeeTransport();
    #elif USE_THREAD == 1
        return getThreadTransport();
    #elif USE_<NAME> == 1
        return get<Name>Transport();
    #else
        #error "No transport selected!"
        return nullptr;
    #endif
}
```

---

## 8. Шаг 7: Добавить флаги компиляции

В `settings.h`:

```cpp
// ===== ВНУТРЕННИЕ ФЛАГИ =====
#ifndef USE_<NAME>
#define USE_<NAME> 0
#endif
```

В `platformio.ini`:

```ini
[env:new-transport]
build_flags =
    -DUSE_<NAME>=1
    -DTRANSPORT_TYPE=<ID>
```

---

## 9. Шаг 8: Добавить в документацию

Обновить `TRANSPORT_ABSTRACTION.md`:

```markdown
### <Name>-транспорт

**Среда:** <Name>

**Протоколы:** <Protocol>

**Статус:** Реализован / Заглушка

**Документация:** `TRANSPORT_<NAME>.md`
```

Обновить раздел 15. Связь со спецификациями:

```markdown
**Реализации транспортов:**

| Транспорт | Документ |
|-----------|----------|
| WiFi | `TRANSPORT_WIFI.md` |
| Zigbee | `TRANSPORT_ZIGBEE.md` |
| Thread | `TRANSPORT_MATTER.md` |
| <Name> | `TRANSPORT_<NAME>.md` |
```

---

## 10. Чек-лист для разработчика

- [ ] Созданы файлы `transport_<name>.h` и `transport_<name>.cpp`
- [ ] Реализованы все методы структуры `Transport`
- [ ] Добавлена публичная функция `get<Name>Transport()`
- [ ] Добавлен `#include` и условие в `transport_factory.h`
- [ ] Добавлен флаг `USE_<NAME>` в `settings.h`
- [ ] Добавлено окружение в `platformio.ini`
- [ ] Обновлена документация (`TRANSPORT_ABSTRACTION.md`)
- [ ] Проверена сборка с новым транспортом
- [ ] Проверена сборка без нового транспорта (не должен включаться)

---

*Конец документа*