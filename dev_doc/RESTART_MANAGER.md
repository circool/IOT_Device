# RESTART_MANAGER.md

## Менеджер перезагрузок

Централизованное управление перезагрузкой устройства. Единственный слой, который вызывает `ESP.restart()`.

---

## Оглавление

- [1. Назначение](#1-назначение)
- [2. Принцип работы](#2-принцип-работы)
- [3. API](#3-api)
  - [3.1. restart_request()](#31-restart_request)
  - [3.2. restart_update()](#32-restart_update)
  - [3.3. restart_is_pending()](#33-restart_is_pending-опционально)
  - [3.4. restart_cancel()](#34-restart_cancel-опционально)
- [4. Интеграция с оркестратором](#4-интеграция-с-оркестратором)
- [5. Примеры использования](#5-примеры-использования)
- [6. Особенности реализации](#6-особенности-реализации)
- [7. Рекомендации по использованию](#7-рекомендации-по-использованию)
- [8. Проверка причин перезагрузки](#8-проверка-причин-перезагрузки)

---

## 1. Назначение

Предотвращает хаотичные перезагрузки из разных мест кода. Все слои, которым нужно перезагрузить устройство, вызывают `restart_request()`, а фактический вызов `ESP.restart()` происходит централизованно в `restart_update()`.

**Цели:**
- **Единая точка управления** — легко добавить логирование, задержки, отмену
- **Предотвращение коллизий** — защита от множественных вызовов `ESP.restart()`
- **Гарантированная отправка логов** — задержка перед перезагрузкой даёт время на отправку данных

---

## 2. Принцип работы

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          ЛЮБОЙ СЛОЙ КОДА                                    │
│                                                                             │
│  • ConfigManager: ошибка EEPROM                                             │
│  • Web: пользователь нажал "Save"                                           │                       
│                                                                             │
│  • MQTT: команда reset                                                      │
│  • Кнопка: удержание 3 секунды                                              │
│  • OTA: обновление завершено                                               
│
│  • И т.д.                                                                   │
│                                                                             │
│  ВСЕ ВЫЗЫВАЮТ: restart_request(500)                                         │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         restart_manager.cpp                                 │
│                                                                             │
│  static bool _pending = false;                                              │
│  static unsigned long _requestTime = 0;                                     │
│  static unsigned long _delayMs = 0;                                         │
│                                                                             │
│  void restart_request(delayMs) {                                            │
│      if (!_pending) {                                                      │
│          _pending = true;                                                  │
│          _requestTime = millis();                                           │
│          _delayMs = delayMs;                                               │
│          XLOG_INFO(CAT_RESTART, "Restart requested in %lu ms", delayMs);    │
│      }                                                                      │
│  }                                                                          │
│                                                                             │
│  void restart_update() {                                                    │
│      if (_pending && millis() - _requestTime >= _delayMs) {                │
│          XLOG_INFO(CAT_RESTART, "Executing restart...");                    │
│          ESP.restart();   // ← ЕДИНСТВЕННЫЙ ВЫЗОВ В ПРОЕКТЕ                 │
│      }                                                                      │
│  }                                                                          │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                              ESP.restart()                                  │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 3. API

### 3.1. restart_request()

```cpp
void restart_request(unsigned long delayMs = 500);
```

**Назначение:** запросить перезагрузку устройства с задержкой.

**Параметры:**

| Параметр | Тип | Описание | Значение по умолчанию |
|----------|-----|----------|----------------------|
| `delayMs` | `unsigned long` | Задержка перед перезагрузкой в миллисекундах | 500 |

**Возвращаемое значение:** отсутствует (`void`).

**Поведение:**
- Если перезагрузка ещё не запрошена — устанавливает флаг и запоминает время
- Если перезагрузка уже запрошена — игнорирует вызов (защита от дублирования)
- Логирует запрос с указанием задержки

**Пример:**
```cpp
// Запросить перезагрузку с задержкой 500 мс (по умолчанию)
restart_request();

// Запросить перезагрузку с задержкой 2 секунды
restart_request(2000);
```

---

### 3.2. restart_update()

```cpp
void restart_update();
```

**Назначение:** периодическая проверка необходимости перезагрузки. Вызывается в `loop()`.

**Параметры:** отсутствуют.

**Возвращаемое значение:** отсутствует (`void`).

**Поведение:**
- Если перезагрузка запрошена и задержка истекла — вызывает `ESP.restart()`
- **Единственное место в проекте**, где вызывается `ESP.restart()`

**Пример:**
```cpp
void loop() {
    // ... все update() слоёв
    restart_update();  // ← ПОСЛЕДНИЙ ВЫЗОВ В ЦИКЛЕ
}
```

---

### 3.3. restart_is_pending() (опционально)

```cpp
bool restart_is_pending();
```

**Назначение:** проверить, запрошена ли перезагрузка.

**Параметры:** отсутствуют.

**Возвращаемое значение:**
- `true` — перезагрузка запрошена и ожидает выполнения
- `false` — перезагрузка не запрошена

**Пример:**
```cpp
if (restart_is_pending()) {
    XLOG_DEBUG(CAT_MAIN, "Restart is pending, skipping some operations");
}
```

---

### 3.4. restart_cancel() (опционально)

```cpp
void restart_cancel();
```

**Назначение:** отменить запланированную перезагрузку.

**Параметры:** отсутствуют.

**Возвращаемое значение:** отсутствует (`void`).

**Поведение:**
- Сбрасывает флаг `_pending`
- Логирует отмену перезагрузки

**Пример:**
```cpp
// Внезапно отменили перезагрузку
if (some_condition) {
    restart_cancel();
    XLOG_INFO(CAT_MAIN, "Restart cancelled due to condition");
}
```

---

## 4. Интеграция с оркестратором

### В `setup()`:

```cpp
void setup() {
    // ... инициализация всех слоёв
    // restart_manager не требует инициализации
}
```

### В `loop()`:

```cpp
void loop() {
    // ... все update() слоёв
    restart_update();  // ← ПОСЛЕДНИЙ ВЫЗОВ В ЦИКЛЕ
}
```

**Почему последний?** Чтобы все остальные операции (отправка логов, сохранение данных) успели завершиться до перезагрузки.

---

## 5. Примеры использования

### 5.1. Из Web-слоя (сохранение настроек)

```cpp
// web_manager.cpp — обработчик /save
void web_handle_save() {
    if (g_configManager.save()) {
        g_webRestartPending = true;
        // Оркестратор в loop() вызовет restart_request(500)
    }
}
```

### 5.2. Из ConfigManager (CRC ошибка)

```cpp
// config_manager.cpp
bool ConfigManager::init() {
    if (!isValid()) {
        XLOG_ERROR(CAT_CONFIG, "Config corrupted, resetting to defaults");
        setDefaults();
        save();
        restart_request(1000);  // ← перезагрузка через 1 секунду
        return false;
    }
    return true;
}
```

### 5.3. Из кнопки сброса

```cpp
// main.cpp — обработка кнопки
if (stage == STAGE_3S) {
    XLOG_WARN(CAT_MAIN, "Reset button held 3s - factory reset");
    g_configManager.reset();
    restart_request(500);
}
```

### 5.4. Из OTA (после обновления)

```cpp
// ota.cpp
void web_ota_manager_update() {
    ElegantOTA.loop();
    if (ElegantOTA.isFinished()) {
        XLOG_INFO(CAT_OTA, "OTA update complete, restarting...");
        restart_request(500);
    }
}
```

---

## 6. Особенности реализации

### 6.1. Защита от множественных вызовов

```cpp
static bool _pending = false;

void restart_request(unsigned long delayMs) {
    if (!_pending) {
        _pending = true;
        _requestTime = millis();
        _delayMs = delayMs;
        XLOG_INFO(CAT_RESTART, "Restart requested in %lu ms", delayMs);
    } else {
        XLOG_DEBUG(CAT_RESTART, "Restart already pending, ignoring duplicate");
    }
}
```

### 6.2. Логирование перед перезагрузкой

```cpp
void restart_update() {
    if (_pending && millis() - _requestTime >= _delayMs) {
        XLOG_INFO(CAT_RESTART, "Executing restart...");
        delay(100);  // Дать время на отправку логов
        ESP.restart();
    }
}
```

### 6.3. Остановка WDT перед перезагрузкой

```cpp
void restart_update() {
    if (_pending && millis() - _requestTime >= _delayMs) {
        wdt_stop();  // Остановить WDT, чтобы не сработал во время перезагрузки
        XLOG_INFO(CAT_RESTART, "Executing restart...");
        delay(100);
        ESP.restart();
    }
}
```

### 6.4. Внутреннее состояние (restart_manager.cpp)

```cpp
static bool _pending = false;
static unsigned long _requestTime = 0;
static unsigned long _delayMs = 0;
```

---

## 7. Рекомендации по использованию

| Ситуация | Рекомендуемая задержка |
|----------|------------------------|
| Обычная перезагрузка | 500 мс |
| После сохранения EEPROM | 500-1000 мс |
| После OTA-обновления | 1000-2000 мс |
| Сброс настроек (factory reset) | 500 мс |
| Аварийная перезагрузка | 100 мс (минимальная) |

**Общее правило:** задержка должна быть достаточной для отправки всех логов и завершения операций записи.

---

## 8. Проверка причин перезагрузки

При запуске можно определить, почему устройство перезагрузилось:

```cpp
// В setup()
void setup() {
    esp_reset_reason_t reason = esp_reset_reason();
    switch (reason) {
        case ESP_RST_POWERON:
            XLOG_INFO(CAT_SYSTEM, "Reset reason: Power ON");
            break;
        case ESP_RST_WDT:
            XLOG_WARN(CAT_SYSTEM, "Reset reason: Watchdog timeout");
            break;
        case ESP_RST_SW:
            XLOG_INFO(CAT_SYSTEM, "Reset reason: Software restart (restart_manager)");
            break;
        // ... и т.д.
    }
}
```

