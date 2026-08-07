```cpp
/**
 * @file TRANSPORT_MATTER.md
 * @brief Matter-транспорт — реализация транспортной абстракции для IP-сети
 * @note Статус: Рефакторинг
 * @version 0.11
 * @date 06.08.2026
 */
```

# TRANSPORT_MATTER.md

## Оглавление

- [1. Назначение](#1-назначение)
- [2. Архитектурное место](#2-архитектурное-место)
- [3. Компоненты](#3-компоненты)
- [4. Режимы работы](#4-режимы-работы)
- [5. Жизненный цикл](#5-жизненный-цикл)
- [6. Управление настройкой](#6-управление-настройкой)
- [7. Взаимодействие с компонентами](#7-взаимодействие-с-компонентами)
- [8. Управление флагами состояния](#8-управление-флагами-состояния)
- [9. Флаги компиляции](#9-флаги-компиляции)
- [10. Связь с документацией](#10-связь-с-документацией)

---

## 1. Назначение

`TRANSPORT_MATTER` — реализация транспортной абстракции для Matter-сети (IP-сеть поверх WiFi или Thread). Это **самовосстанавливающаяся сущность**, которая обеспечивает постоянный канал связи между устройством и Matter-контроллером (экосистемой умного дома).

**Matter-транспорт объединяет:**
- **Среду:** WiFi STA или Thread
- **Прикладной протокол:** Matter (на базе ZCL)

**Matter-транспорт управляет:**
- Присоединением к Matter-сети (Commissioning)
- Регистрацией конечных точек (Endpoints) и кластеров
- Подписками (Subscriptions) на атрибуты
- Обработкой входящих Matter-команд

**Matter-транспорт устанавливает флаги в `StateProvider`:**
- `link_ok` — IP-сеть доступна (WiFi/Thread)
- `gateway_ok` — Matter-контроллер доступен
- `setup_mode` — режим настройки активен

**Matter-транспорт НЕ управляет:**
- Бизнес-логикой устройства (это DeviceController)


---

## 2. Архитектурное место

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              ВНЕШНИЙ МИР                                    │
│                                                                             │
│  • Matter-контроллер (Apple Home, Google Home, Amazon Alexa)                │
│  • Thread Border Router (для Thread-среды)                                  │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ Matter (IP)
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                              TRANSPORT_MATTER                               │
│                                                                             │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                      УПРАВЛЕНИЕ СЕТЬЮ                               │    │
│  │                                                                     │    │
│  │  • Commissioning (QR-код / числовой код)                            │    │
│  │  • Мониторинг соединения                                            │    │
│  │  • Восстановление при потере сети                                   │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
│                                    │                                        │
│                                    ▼                                        │
│  ┌─────────────────────────────────────────────────────────────────────┐    │
│  │                      ПРИКЛАДНОЙ УРОВЕНЬ (Matter)                    │    │
│  │                                                                     │    │
│  │  • Endpoint 0 — Aggregator (Basic Information, OTA, Custom)         │    │
│  │  • Endpoint 1 — Extended Color Light (On/Off, Level Control)        │    │
│  │  • Endpoint 2 — Temperature Sensor                                  │    │
│  │  • Endpoint 3 — Humidity Sensor                                     │    │
│  └─────────────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ Реализует интерфейс Transport
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                              ОРКЕСТРАТОР                                    │
│                                                                             │
│  • Работает через интерфейс Transport                                       │
│  • Не знает о внутреннем устройстве Matter-транспорта                       │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 3. Компоненты

| Компонент | Назначение | Активен в обычном режиме | Активен в режиме настройки |
|-----------|------------|--------------------------|---------------------------|
| **Matter Stack** | Управление стеком, commissioning | ✅ | ✅ (ожидание commissioning) |
| **Endpoint 0** | Корневая точка (Basic, OTA, Custom) | ✅ | ❌ |
| **Endpoint 1** | Управление актуатором (On/Off, Level) | ✅ | ❌ |
| **Endpoint 2** | Датчик температуры | ✅ | ❌ |
| **Endpoint 3** | Датчик влажности | ✅ | ❌ |
| **Subscriptions** | Подписки контроллеров на атрибуты | ✅ | ❌ |

---

## 4. Режимы работы

| Режим | Состояние Matter | Назначение |
|-------|------------------|------------|
| **Обычный** | Подключён к контроллеру | Работа в сети |
| **Настройка** | Ожидание Commissioning | Сбор данных |

### 4.1. Обычный режим

- Устройство подключено к Matter-контроллеру
- Конечные точки зарегистрированы
- Атрибуты доступны по подписке
- Команды принимаются через Matter-команды

### 4.2. Режим настройки

- Устройство ожидает Commissioning
- Контроллер сканирует QR-код или вводит числовой код
- После успешного Commissioning — переключение в обычный режим

### 4.3. Переключение между режимами

```
Обычный режим
    │
    │ Потеря связи / нет контроллера
    ▼
Режим настройки (ожидание Commissioning)
    │
    │ Commissioning завершён
    ▼
Обычный режим
```

**Переключение выполняется транспортом автоматически:**
1. При старте: если нет контроллера → режим настройки
2. В процессе: потеря связи → переподключение → если не удаётся → режим настройки
3. После Commissioning: проверка → успех → обычный режим

---

## 5. Жизненный цикл

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ 1. begin(config)                                                            │
│    • Проверить конфигурацию (если есть — использовать)                      │
│    • Если нет контроллера → setMode(true)                                   │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│ 2. update() — вызывается в loop()                                           │
│                                                                             │
│    А. Если режим настройки:                                                 │
│       • Ожидать Commissioning (QR-код / числовой код)                       │
│       • Если Commissioning завершён → setMode(false)                        │
│                                                                             │
│    Б. Если обычный режим:                                                   │
│       • Поддерживать соединение                                             │
│       • Обновлять атрибуты (для подписок)                                   │
│       • Обрабатывать входящие Matter-команды                                │
│       • При потере связи → попытки переподключения                          │
│       • При невозможности → setMode(true)                                   │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 6. Управление настройкой

Matter-транспорт сам управляет процессом настройки (Commissioning).

### 6.1. Кто принимает решение

**Matter-транспорт** — единственный, кто решает, когда переключиться в режим настройки.

**Условия для переключения в режим настройки:**
1. **При старте:** если нет контроллера → `setMode(true)`
2. **В процессе работы:** если потеряна связь → попытки переподключения → если не удаётся → `setMode(true)`


### 6.2. Управление режимами

```cpp
void MatterTransport::setMode(bool setupMode) {
    if (setupMode) {
        // ===== РЕЖИМ НАСТРОЙКИ (ожидание Commissioning) =====
        XLOG_INFO(CAT_MATTER, "Switching to SETUP mode (waiting for commissioning)");
        
        _state = STATE_SETUP_MODE;
        _connected = false;
        
        // Обновляем флаги
        StateProvider::getInstance().set_setup_mode(true);
        StateProvider::getInstance().set_link_ok(false);
        StateProvider::getInstance().set_gateway_ok(false);
        
        // Запускаем Commissioning
        matter_start_setup();
        
    } else {
        // ===== ОБЫЧНЫЙ РЕЖИМ (подключён) =====
        XLOG_INFO(CAT_MATTER, "Switching to NORMAL mode (connected)");
        
        _state = STATE_NORMAL;
        _connected = true;
        
        // Обновляем флаги
        StateProvider::getInstance().set_setup_mode(false);
        StateProvider::getInstance().set_link_ok(true);
        StateProvider::getInstance().set_gateway_ok(true);
        
        // Регистрируем endpoints
        registerEndpoints();
    }
}
```

### 6.3. Инструменты настройки

- **Commissioning** — стандартная процедура Matter
- QR-код или числовой код (11 цифр)
- Контроллер сам передаёт учётные данные сети

### 6.4. Проверка данных

- После Commissioning — транспорт проверяет наличие контроллера
- Проверяет регистрацию endpoints
- Проверяет возможность подписки на атрибуты

### 6.5. Остановка настройки

Транспорт останавливает настройку когда:
- Commissioning успешно завершён
- Endpoints зарегистрированы
- Контроллер подписался на атрибуты

---

## 7. Взаимодействие с компонентами

### 7.1. Управление стеком

Транспорт управляет Matter-стеком через публичный интерфейс:

| Метод | Назначение |
|-------|------------|
| `matter_init()` | Инициализация стека |
| `matter_begin(config)` | Запуск стека с конфигурацией |
| `matter_update()` | Периодическая обработка |
| `matter_start_setup()` | Запуск Commissioning |

### 7.2. Регистрация Endpoints

```cpp
void MatterTransport::registerEndpoints() {
    // Endpoint 0 — Aggregator (все типы)
    matter_register_endpoint(0, 0x000E, &basic_cluster, &custom_cluster);
    
    // Endpoint 1 — Extended Color Light (TYPE 1, 3)
    matter_register_endpoint(1, 0x010D, &on_off_cluster, &level_cluster);
    
    // Endpoint 2 — Temperature Sensor (TYPE 1, 2)
    matter_register_endpoint(2, 0x0302, &temperature_cluster);
    
    // Endpoint 3 — Humidity Sensor (TYPE 1, 2)
    matter_register_endpoint(3, 0x0307, &humidity_cluster);
}
```

### 7.3. Публикация данных

При изменении состояния устройства транспорт обновляет атрибуты для подписок:

```cpp
void MatterTransport::publishState(const DeviceState* state) {
    if (!_connected) return;
    
    // Endpoint 1 — Extended Color Light
    matter_update_attribute(1, 0x0006, 0x0000, state->is_on ? 1 : 0);  // On/Off
    matter_update_attribute(1, 0x0008, 0x0000, state->speed);           // Level
    
    // Endpoint 2 — Temperature Sensor
    matter_update_attribute(2, 0x0402, 0x0000, state->temperature * 100);
    
    // Endpoint 3 — Humidity Sensor
    matter_update_attribute(3, 0x0405, 0x0000, state->humidity * 100);
}
```

### 7.4. Обработка команд

Транспорт принимает команды через Matter и передаёт их в `onCommand()`:

```cpp
void MatterTransport::onMatterCommand(uint16_t endpoint, uint16_t cluster, uint8_t command, void* data) {
    TransportCommand cmd;
    TransportCommandData cmdData;
    
    switch (cluster) {
        case 0x0006:  // On/Off
            cmd = CMD_STATE;
            cmdData.boolVal = (command == 0x01);
            break;
        case 0x0008:  // Level Control
            cmd = CMD_SPEED;
            cmdData.intVal = *(uint8_t*)data;
            break;
        // ...
    }
    
    _commandCallback(cmd, &cmdData);
}
```

### 7.5. Обработка изменения конфигурации

```cpp
void MatterTransport::onConfigUpdate(const TransportConfig* config) {
    // Matter использует commissioning
    // При изменении конфигурации — перезапускаем стек
    if (config->threadNetworkName != _currentNetworkName) {
        _matterStack.reset();
        _matterStack.begin(config);
        setMode(true);  // Перезапускаем commissioning
    }
}
```

---

## 8. Управление флагами состояния

Matter-транспорт устанавливает флаги в `StateProvider`:

| Флаг | Кто устанавливает | Когда |
|------|-------------------|-------|
| `link_ok` | Matter-транспорт | IP-сеть доступна (WiFi/Thread) |
| `gateway_ok` | Matter-транспорт | Контроллер доступен |
| `setup_mode` | Matter-транспорт | Режим настройки активен |

```cpp
void MatterTransport::updateFlags() {
    // Флаги обновляются внутри setMode()
    // Дополнительные проверки для gateway_ok
    bool controllerOk = matter_is_controller_available();
    StateProvider::getInstance().set_gateway_ok(controllerOk);
}
```

---

## 9. Флаги компиляции

| Внутренний флаг | Назначение |
|-----------------|------------|
| `USE_MATTER` | Включить Matter-транспорт |

**Примечание:** Matter использует IP-сеть (WiFi или Thread). Флаг `USE_MATTER` включает Matter-стек поверх выбранной среды.

---

## 10. Связь с документацией

| Компонент | Документ |
|-----------|----------|
| Архитектура транспорта | `TRANSPORT_ABSTRACTION.md` |
| Спецификация Matter | `TRANSPORT_MATTER_SPEC.md` |
| WiFi-транспорт | `TRANSPORT_WIFI.md` |

---

*Конец документа*