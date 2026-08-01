# WIFI_MANAGER.md

## Управление WiFi (STA-режим)

Управление WiFi-подключением устройства в режиме клиента (STA).

---

## Оглавление

- [1. Назначение](#1-назначение)
- [2. Принцип работы](#2-принцип-работы)
- [3. API](#3-api)
  - [3.1. wifi_manager_init()](#31-wifi_manager_init)
  - [3.2. wifi_manager_connect()](#32-wifi_manager_connect)
  - [3.3. wifi_manager_update()](#33-wifi_manager_update)
  - [3.4. wifi_get_local_ip()](#34-wifi_get_local_ip)
  - [3.5. wifi_get_rssi()](#35-wifi_get_rssi)
  - [3.6. wifi_scan_and_log()](#36-wifi_scan_and_log)
- [4. Интеграция с SystemState](#4-интеграция-с-systemstate)
- [5. Интеграция с оркестратором](#5-интеграция-с-оркестратором)
- [6. Интеграция с Provisioning](#6-интеграция-с-provisioning)
- [7. Примеры использования](#7-примеры-использования)
- [8. Настройка через build_flags](#8-настройка-через-build_flags)
- [9. Автономное переключение из AP в STA](#9-автономное-переключение-из-ap-в-sta)
- [10. Особенности реализации](#10-особенности-реализации)

---

## 1. Назначение

Слой `wifi_manager` обеспечивает **только STA-режим** (клиент):

- **Подключение к WiFi** — подключение к существующей сети
- **Мониторинг** — отслеживание состояния подключения
- **Автопереподключение** — восстановление связи при потере
- **Интеграцию** — управление флагом `STATE_WIFI_OK` для других слоёв

**Важно:** AP-режим (точка доступа) вынесен в слой `provisioning`, так как используется только для настройки устройства.

---

## 2. Принцип работы

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         wifi_manager_init()                                 │
│                                                                             │
│  1. Инициализация WiFi-стека                                                │
│  2. Настройка режима STA (клиент)                                           │
│  3. Регистрация обработчиков событий WiFi                                  │
│  4. Запуск автоматического подключения (если есть SSID)                    │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         wifi_manager_update()                               │
│                         (вызывается в loop)                                 │
│                                                                             │
│  1. Проверка состояния подключения                                          │
│  2. При потере связи — попытка переподключения                             │
│  3. Управление STATE_WIFI_OK                                               │
│  4. Обработка событий WiFi (подключение/отключение)                        │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                           ОРКЕСТРАТОР                                       │
│                                                                             │
│  if (STATE_WIFI_OK) {                                                      │
│      // Запустить Web-сервер, MQTT-транспорт                               │
│  } else {                                                                  │
│      // Запустить провизионинг                                             │
│  }                                                                          │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 3. API

### 3.1. wifi_manager_init()

```cpp
void wifi_manager_init();
```

**Назначение:** инициализация WiFi-менеджера. Вызывается один раз в `setup()`.

**Параметры:** отсутствуют.

**Возвращаемое значение:** отсутствует (`void`).

**Поведение:**
- Настраивает WiFi-стек в режим STA
- Регистрирует обработчики событий
- Если в Config есть SSID — запускает подключение
- Если SSID пуст — ничего не делает (ожидает провизионинг)

**Пример:**
```cpp
void setup() {
    // ...
    wifi_manager_init();
    // ...
}
```

---

### 3.2. wifi_manager_connect()

```cpp
void wifi_manager_connect(const char* ssid, const char* password);
```

**Назначение:** подключиться к WiFi-сети.

**Параметры:**

| Параметр | Тип | Описание |
|----------|-----|----------|
| `ssid` | `const char*` | Имя сети (не может быть пустым) |
| `password` | `const char*` | Пароль (может быть пустым для открытых сетей) |

**Возвращаемое значение:** отсутствует (`void`).

**Поведение:**
- Запускает асинхронное подключение
- Не блокирует выполнение (не ждёт подключения)
- Результат можно проверить через `STATE_WIFI_OK` или `wifi_get_local_ip()`

**Пример:**
```cpp
wifi_manager_connect("MyWiFi", "my_password");
```

---

### 3.3. wifi_manager_update()

```cpp
void wifi_manager_update();
```

**Назначение:** периодическая обработка WiFi. Вызывается в `loop()`.

**Параметры:** отсутствуют.

**Возвращаемое значение:** отсутствует (`void`).

**Поведение:**
- Проверяет состояние подключения
- При потере связи — пытается переподключиться
- Обновляет `STATE_WIFI_OK`

**Пример:**
```cpp
void loop() {
    wifi_manager_update();  // ← ОБЯЗАТЕЛЬНО
    // ...
}
```

---

### 3.4. wifi_get_local_ip()

```cpp
String wifi_get_local_ip();
```

**Назначение:** получить локальный IP-адрес устройства.

**Параметры:** отсутствуют.

**Возвращаемое значение:** IP-адрес в виде строки (например, `"192.168.1.100"`).

**Поведение:**
- Если подключение активно — возвращает IP
- Если нет — возвращает `"0.0.0.0"`

**Пример:**
```cpp
String ip = wifi_get_local_ip();
XLOG_INFO(CAT_WIFI, "IP address: %s", ip.c_str());
```

---

### 3.5. wifi_get_rssi()

```cpp
int wifi_get_rssi();
```

**Назначение:** получить уровень сигнала WiFi (RSSI).

**Параметры:** отсутствуют.

**Возвращаемое значение:** RSSI в dBm (отрицательное число, например `-55`).

**Поведение:**
- Если подключение активно — возвращает RSSI
- Если нет — возвращает `0`

**Пример:**
```cpp
int rssi = wifi_get_rssi();
XLOG_DEBUG(CAT_WIFI, "RSSI: %d dBm", rssi);
```

---

### 3.6. wifi_scan_and_log()

```cpp
int wifi_scan_and_log(const char* targetSsid);
```

**Назначение:** выполнить сканирование WiFi-сетей и вывести результат в лог. Используется для отладки.

**Параметры:**
- `targetSsid` — SSID для отметки в логе (если `nullptr` — без отметки)

**Возвращаемое значение:**
- Количество найденных сетей (положительное число)
- `-1` при ошибке или если сканирование уже выполняется

**Поведение:**
- Синхронное сканирование (блокирует выполнение на 2-5 секунд)
- Выводит список сетей в лог с уровнями сигнала

**Пример:**
```cpp
// Отладка — при старте показать доступные сети
wifi_scan_and_log(g_configManager.getWifiSsid());
```

---

## 4. Интеграция с SystemState

Слой `wifi_manager` управляет битом `STATE_WIFI_OK`:

```cpp
// wifi_manager.cpp — внутри wifi_manager_update()
if (WiFi.status() == WL_CONNECTED) {
    if (!_wasConnected) {
        system_state_set_bit(STATE_WIFI_OK);
        _wasConnected = true;
        XLOG_INFO(CAT_WIFI, "WiFi connected, IP: %s", WiFi.localIP().toString().c_str());
    }
} else {
    if (_wasConnected) {
        system_state_clear_bit(STATE_WIFI_OK);
        _wasConnected = false;
        XLOG_WARN(CAT_WIFI, "WiFi disconnected");
    }
}
```

---

## 5. Интеграция с оркестратором

### В `setup()`:

```cpp
void setup() {
    // ... инициализация Logger, ConfigManager ...
    
    wifi_manager_init();  // ← ИНИЦИАЛИЗАЦИЯ
    
    // Проверка наличия SSID
    if (strlen(g_configManager.getWifiSsid()) > 0) {
        wifi_manager_connect(
            g_configManager.getWifiSsid(),
            g_configManager.getWifiPassword()
        );
    } else {
        XLOG_INFO(CAT_MAIN, "No WiFi config, waiting for provisioning");
    }
}
```

### В `loop()`:

```cpp
void loop() {
    wdt_feed();
    
    wifi_manager_update();  // ← ОБНОВЛЕНИЕ СОСТОЯНИЯ
    
    // Проверка потери WiFi
    if (!(bits & STATE_WIFI_OK) && !(bits & STATE_PROVISIONING)) {
        if (wifi_fail_start == 0) {
            wifi_fail_start = millis();
        } else if (millis() - wifi_fail_start > WIFI_FALLBACK_TIMEOUT_MS) {
            startProvisioning();  // ← ПЕРЕХОД В AP (через provisioning)
        }
    } else {
        wifi_fail_start = 0;
    }
    
    // ... остальной код
}
```

---

## 6. Интеграция с Provisioning

AP-режим полностью вынесен в `provisioning`:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      PROVISIONING (AP + HTTP)                              │
│                                                                             │
│  • Запускает AP: wifi_start_ap() — через WiFi API напрямую                │
│  • Останавливает AP: wifi_stop_ap() — через WiFi API напрямую             │
│  • Принимает SSID/пароль от пользователя                                   │
│  • Передаёт данные оркестратору                                            │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ После получения данных
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                           ОРКЕСТРАТОР                                       │
│                                                                             │
│  1. Сохраняет SSID/пароль в Config                                          │
│  2. Вызывает provisioning_stop()                                            │
│  3. Снимает STATE_PROVISIONING                                              │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ Следующий цикл loop()
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         WIFI_MANAGER (STA)                                  │
│                                                                             │
│  1. wifi_manager_update() замечает, что STATE_PROVISIONING снят            │
│  2. Читает SSID/пароль из ConfigManager                                    │
│  3. Самостоятельно подключается к сети                                     │
│  4. Устанавливает STATE_WIFI_OK                                            │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 7. Примеры использования

### 7.1. Проверка состояния WiFi

```cpp
if (system_state_has_bit(STATE_WIFI_OK)) {
    XLOG_INFO(CAT_MAIN, "WiFi is connected, IP: %s", wifi_get_local_ip().c_str());
} else {
    XLOG_WARN(CAT_MAIN, "WiFi is not connected");
}
```

### 7.2. Получение RSSI для отладки

```cpp
int rssi = wifi_get_rssi();
XLOG_DEBUG(CAT_WIFI, "Signal strength: %d dBm", rssi);

// RSSI интерпретация
if (rssi > -50) {
    XLOG_DEBUG(CAT_WIFI, "Excellent signal");
} else if (rssi > -60) {
    XLOG_DEBUG(CAT_WIFI, "Good signal");
} else if (rssi > -70) {
    XLOG_DEBUG(CAT_WIFI, "Fair signal");
} else {
    XLOG_DEBUG(CAT_WIFI, "Weak signal");
}
```

### 7.3. Сканирование сетей при запуске (отладка)

```cpp
#if FEATURE_SCANNING_WIFI_ENABLED == 1
void setup() {
    // ...
    wifi_manager_init();
    wifi_scan_and_log(g_configManager.getWifiSsid());
    // ...
}
#endif
```

---

## 8. Настройка через build_flags

В `platformio.ini` можно переопределить настройки:

```ini
build_flags =
    -D WIFI_FALLBACK_TIMEOUT_MS=10000    # Время до запуска провизионинга при потере WiFi (мс)
    -D FEATURE_SCANNING_WIFI_ENABLED=0   # 1 = включить сканирование при старте
```

Если `TRANSPORT_TYPE != TRANSPORT_TYPE_WIFI`, все функции становятся заглушками:

```cpp
// Заглушка — WiFi отключён (TRANSPORT_TYPE != TRANSPORT_TYPE_WIFI)
inline void wifi_manager_init() {}
inline void wifi_manager_connect(const char*, const char*) {}
inline void wifi_manager_update() {}
inline String wifi_get_local_ip() { return "0.0.0.0"; }
inline int wifi_get_rssi() { return 0; }
inline int wifi_scan_and_log(const char*) { return -1; }
```

---

## 9. Автономное переключение из AP в STA

**Ключевое архитектурное решение:** WiFi-менеджер **не получает команду** на переключение в STA. Это его собственная ответственность.

### 9.1. Механизм

```cpp
// wifi_manager.cpp — внутри wifi_manager_update()

static bool _wasProvisioning = false;

void wifi_manager_update() {
    bool isProvisioning = system_state_has_bit(STATE_PROVISIONING);
    
    // --- ОБНАРУЖЕНИЕ: провизионинг только что завершился ---
    if (_wasProvisioning && !isProvisioning) {
        XLOG_INFO(CAT_WIFI, "Provisioning finished, switching to STA mode");
        
        // 1. Получить SSID и пароль из ConfigManager
        const char* ssid = g_configManager.getWifiSsid();
        const char* password = g_configManager.getWifiPassword();
        
        // 2. Подключиться к сохранённой сети
        if (strlen(ssid) > 0) {
            wifi_manager_connect(ssid, password);
        } else {
            XLOG_ERROR(CAT_WIFI, "No SSID in config after provisioning!");
        }
    }
    
    _wasProvisioning = isProvisioning;
    
    // --- ОБЫЧНАЯ ЛОГИКА: мониторинг STA ---
    if (!isProvisioning) {
        // Проверка состояния подключения, переподключение и т.д.
        // ...
    }
}
```

### 9.2. Почему это важно

| Подход | Проблема |
|--------|----------|
| ❌ Оркестратор вызывает `wifi_manager_connect()` | Нарушает инкапсуляцию, оркестратор знает детали WiFi |
| ✅ WiFi-менеджер сам переключается | Инкапсуляция, оркестратор только управляет `STATE_PROVISIONING` |

### 9.3. Последовательность

```
1. Оркестратор: provisioning_stop()
   → снимает STATE_PROVISIONING

2. Следующий вызов loop():
   → wifi_manager_update()
   → обнаруживает: STATE_PROVISIONING снят
   → читает SSID/пароль из ConfigManager
   → подключается к сети

3. WiFi подключается:
   → STATE_WIFI_OK устанавливается
   → Transport (MQTT) запускается
```

**Результат:** перезагрузка **не требуется**. Устройство продолжает работу в обычном режиме.

---

## 10. Особенности реализации

### 10.1. Асинхронное подключение

```cpp
static bool _connecting = false;
static unsigned long _connectStartTime = 0;

void wifi_manager_connect(const char* ssid, const char* password) {
    WiFi.begin(ssid, password);
    _connecting = true;
    _connectStartTime = millis();
}

void wifi_manager_update() {
    if (_connecting) {
        if (WiFi.status() == WL_CONNECTED) {
            _connecting = false;
            system_state_set_bit(STATE_WIFI_OK);
        } else if (millis() - _connectStartTime > WIFI_CONNECT_TIMEOUT_MS) {
            _connecting = false;
            XLOG_WARN(CAT_WIFI, "WiFi connection timeout");
        }
    }
    // ...
}
```

### 10.2. Обработка событий WiFi

На ESP32 можно использовать обработчики событий:

```cpp
void onWiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case SYSTEM_EVENT_STA_CONNECTED:
            XLOG_INFO(CAT_WIFI, "Connected to AP");
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            XLOG_INFO(CAT_WIFI, "Got IP: %s", WiFi.localIP().toString().c_str());
            system_state_set_bit(STATE_WIFI_OK);
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            XLOG_WARN(CAT_WIFI, "Disconnected from AP");
            system_state_clear_bit(STATE_WIFI_OK);
            break;
        default:
            break;
    }
}

void wifi_manager_init() {
    WiFi.onEvent(onWiFiEvent);
    // ...
}
```

### 10.3. Автопереподключение

```cpp
static unsigned long _lastReconnectAttempt = 0;

void wifi_manager_update() {
    if (WiFi.status() != WL_CONNECTED && !_connecting) {
        if (millis() - _lastReconnectAttempt > WIFI_RECONNECT_INTERVAL_MS) {
            XLOG_WARN(CAT_WIFI, "Attempting to reconnect...");
            WiFi.reconnect();
            _lastReconnectAttempt = millis();
            _connecting = true;
        }
    }
    // ...
}
```

### 10.4. Защита от watchdog при сканировании

```cpp
int wifi_scan_and_log(const char* targetSsid) {
    wdt_stop();  // ← Остановить WDT, так как сканирование может занять > WDT_TIMER_MS
    
    int networks = WiFi.scanNetworks();
    
    wdt_start();  // ← Восстановить WDT
    
    // ... обработка результатов
    return networks;
}
```

---

## 11. Требования к доработке

| # | Проблема | Статус | Решение |
|---|----------|--------|---------|
| 1 | **AP-логика в wifi_manager** | 🔴 | **Вынести в provisioning** (сейчас смешано) |
| 2 | **Зависимость от ConfigManager** | ✅ | Убраны прямые вызовы `g_configManager` |
| 3 | **Нет единого интерфейса `init()`/`update()`** | ⬜ | Привести к единому стандарту слоёв |

---

*Конец документа*
