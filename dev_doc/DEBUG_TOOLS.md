```cpp
/**
 * @file DEBUG_TOOLS.md
 * @brief Вспомогательные функции для диагностики и отладки устройства
 * @note Статус: Закончен
 */
```
# DEBUG_TOOLS.md

## Отладочные утилиты

Вспомогательные функции для диагностики и отладки устройства.

## Оглавление

- [1. Назначение](#1-назначение)
- [2. API](#2-api)
  - [2.1. printSystemInfo()](#21-printSystemInfo)
  - [2.2. getResetReason()](#22-getresetreason)
  - [2.3. getChipTemperature()](#23-getchiptemperature)
- [3. Особенности реализации](#3-особенности-реализации)
  - [3.1. ESP32](#31-esp32)
  - [3.2. ESP8266](#32-esp8266)
- [4. Требования к доработке](#4-требования-к-доработке)

## 1. Назначение

Слой `debug_tools` предоставляет утилиты для получения диагностической информации об устройстве:

- **Информация о системе** — чип, память, флеш, архитектура, протоколы
- **Причина перезагрузки** — определение причины последнего сброса
- **Температура чипа** — для ESP32 (на ESP8266 недоступно)

Слой **не имеет состояния** и **не требует инициализации или периодического вызова**.

**Важно:** слой предназначен **только для отладки**. В production-сборках вызов `printSystemInfo()` может быть закомментирован.

## 2. API

### 2.1. printSystemInfo()

```cpp
void printSystemInfo();
```

**Назначение:** вывести в лог полную информацию об устройстве.

**Параметры:** отсутствуют.

**Возвращаемое значение:** отсутствует (`void`).

**Пример вывода (ESP32):**
```
Chip: ESP32-S3 (revision v0.1)
Cores: 2, Frequency: 240 MHz
Chip ID: 7C9A3D4B8E2F
Flash: 16 MB (80 MHz, mode 0)
PSRAM: 8 MB (free: 7.2 MB)
Heap: 294 KB free (min: 256 KB, max alloc: 128 KB)
ESP-IDF: v4.4.5
Architecture: Xtensa
Protocols: WiFi, BLE
  WiFi: Yes
  BLE: Yes
  IEEE 802.15.4: No
Chip temperature: 42.5 °C / 108.5 °F
==========================================
Sketch size: 1234567 bytes
Free sketch space: 4567890 bytes
Free heap: 294 KB
Flash chip size: 16777216 bytes
Firmware ver. 1.0.0
Reset reason: POWER_ON
```

**Пример вывода (ESP8266):**
```
Platform: ESP8266
Chip ID: 4C11A3
Core: 2.7.4, CPU: 160 MHz
Free heap: 45678 bytes
Flash: 4 MB (80 MHz, mode 3)
SDK: 3.0.0
==========================================
Sketch size: 1234567 bytes
Free sketch space: 4567890 bytes
Free heap: 45678 bytes
Flash chip size: 4194304 bytes
Firmware ver. 1.0.0
Reset reason: POWER_ON
```

**Когда использовать:**
- При старте устройства (в `setup()`)
- При подозрении на проблемы с памятью или перегревом

### 2.2. getResetReason()

```cpp
const char* getResetReason();
```

**Назначение:** получить причину последней перезагрузки устройства.

**Параметры:** отсутствуют.

**Возвращаемое значение:** строка с описанием причины.

**ESP32:**
| Возвращаемое значение | Описание |
|-----------------------|----------|
| `POWER_ON` | Включение питания |
| `EXT_RESET` | Сброс от внешнего сигнала |
| `SOFT_RESTART` | Программная перезагрузка (ESP.restart()) |
| `PANIC_CRASH` | Паника (аппаратное исключение) |
| `INT_WDT_CRASH` | Прерывание от WDT |
| `TASK_WDT_CRASH` | Задача WDT |
| `WDT_CRASH` | Сторожевой таймер |
| `DEEP_SLEEP_WAKE` | Пробуждение из глубокого сна |
| `UNKNOWN` | Неизвестная причина |

**ESP8266:**
| Возвращаемое значение | Описание |
|-----------------------|----------|
| `POWER_ON` | Включение питания |
| `WATCHDOG_CRASH` | Сторожевой таймер |
| `EXCEPTION_CRASH` | Исключение |
| `SOFT_WDT_CRASH` | Программный WDT |
| `SOFT_RESTART` | Программная перезагрузка |
| `DEEP_SLEEP_WAKE` | Пробуждение из глубокого сна |
| `EXT_RESET` | Внешний сброс |
| `UNKNOWN` | Неизвестная причина |

**Когда использовать:**
Внутренний метод для printSystemInfo() - в проекте не используется

**Пример:**
```cpp
const char* reason = getResetReason();
XLOG_INFO(CAT_SYSTEM, "Device booted after: %s", reason);

// Анализ причины
if (strcmp(reason, "WDT_CRASH") == 0) {
    XLOG_WARN(CAT_SYSTEM, "Previous crash was due to WDT timeout!");
    // Возможно, стоит увеличить WDT_TIMER_MS
}
```

---

### 2.3. getChipTemperature()

```cpp
float getChipTemperature();
```

**Назначение:** получить температуру чипа.

**Параметры:** отсутствуют.

**Возвращаемое значение:**
- **ESP32** — температура в градусах Цельсия (°C)
- **ESP8266** — `-273.15°C` (не поддерживается)

**Когда использовать:**
Внутренний метод для printSystemInfo() - в проекте не используется

**Пример:**
```cpp
float temp = getChipTemperature();
if (temp > -50.0f && temp < 150.0f) {
    XLOG_INFO(CAT_SYSTEM, "Chip temperature: %.1f°C", temp);
    if (temp > 85.0f) {
        XLOG_WARN(CAT_SYSTEM, "Temperature is high! Consider cooling.");
    }
} else {
    XLOG_DEBUG(CAT_SYSTEM, "Temperature sensor not available");
}
```

---

## 3. Особенности реализации

### 3.1. ESP32

| Функция | API |
|---------|-----|
| **Чип** | `esp_chip_info_t` |
| **Температура** | `temperatureRead()` (встроенный датчик) |
| **Причина сброса** | `esp_reset_reason()` |
| **Flash** | `ESP.getFlashChipSize()` |
| **PSRAM** | `ESP.getPsramSize()` |

**Определение имени чипа:**
```cpp
switch (chip_info.model) {
    case CHIP_ESP32:   return "ESP32";
    case CHIP_ESP32S2: return "ESP32-S2";
    case CHIP_ESP32S3: return "ESP32-S3";
    case CHIP_ESP32C3: return "ESP32-C3";
    case CHIP_ESP32C6: return "ESP32-C6";
    case CHIP_ESP32H2: return "ESP32-H2";
    default:           return "Unknown";
}
```

**Определение архитектуры:**
```cpp
#if defined(CONFIG_IDF_TARGET_ESP32C6) || \
    defined(CONFIG_IDF_TARGET_ESP32H2) || \
    defined(CONFIG_IDF_TARGET_ESP32C3)
    return "RISC-V";
#else
    return "Xtensa";
#endif
```

**Определение протоколов:**
```cpp
bool hasWifi = chip_info.features & CHIP_FEATURE_WIFI_BGN;
bool hasBle = chip_info.features & CHIP_FEATURE_BLE;
#ifdef CHIP_FEATURE_IEEE802154
    bool has802154 = chip_info.features & CHIP_FEATURE_IEEE802154;
#endif
```

### 3.2. ESP8266

| Функция | API |
|---------|-----|
| **Чип** | `ESP.getChipId()` |
| **Температура** | ❌ Не поддерживается (возвращает -273.15) |
| **Причина сброса** | `system_get_rst_info()` |
| **Flash** | `ESP.getFlashChipRealSize()` |
| **SDK** | `system_get_sdk_version()` |


## 4. Требования к доработке

Не целесообразно
