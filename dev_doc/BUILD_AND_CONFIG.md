```cpp
/**
 * @file BUILD_AND_CONFIG.md
 * @brief Описание процесса сборки
 * @note Статус: Закончен
 */
```
# BUILD_AND_CONFIG.md

## Оглавление

- [1. Назначение](#1-назначение)
- [2. Платформы и окружения](#2-платформы-и-окружения)
- [3. Структура platformio.ini](#3-структура-platformioini)
  - [3.1. Общие настройки (`[common]`)](#31-общие-настройки-common)
  - [3.2. Окружения](#32-окружения)
    - [3.2.1. ESP8266](#321-esp8266)
    - [3.2.2. ESP32 (AP только)](#322-esp32-ap-только)
    - [3.2.3. ESP32 (BLE только)](#323-esp32-ble-только)
    - [3.2.4. ESP32 (BLE + AP)](#324-esp32-ble--ap)
    - [3.2.5. ESP32-C3 Super Mini](#325-esp32-c3-super-mini)
    - [3.2.6. ESP32-C6 (MQTT)](#326-esp32-c6-mqtt)
    - [3.2.7. ESP32-C6 (Zigbee)](#327-esp32-c6-zigbee)
- [4. Доступные флаги компиляции](#4-доступные-флаги-компиляции)
  - [4.1. Обязательные](#41-обязательные)
  - [4.2. Функциональные](#42-функциональные)
  - [4.3. Provisioning](#43-provisioning)
  - [4.4. Пины](#44-пины)
  - [4.5. Логирование](#45-логирование)
- [5. Зависимости](#5-зависимости)
- [6. Параметры конфигурации (credentials.h)](#6-параметры-конфигурации-credentialsh)
- [7. Примеры сборки](#7-примеры-сборки)
  - [7.1. Базовая сборка](#71-базовая-сборка)
  - [7.2. Сборка с загрузкой](#72-сборка-с-загрузкой)
  - [7.3. Сборка с очисткой](#73-сборка-с-очисткой)
- [8. Варианты сборки (матрица)](#8-варианты-сборки-матрица)
- [9. Требования к железу](#9-требования-к-железу)

## 1. Назначение

Документ описывает процесс сборки проекта для разных платформ и конфигураций. Содержит информацию о:

- Структуре `platformio.ini` и доступных окружениях
- Флагах компиляции и их значениях
- Зависимостях (библиотеках)
- Порядке сборки прошивки для конкретного устройства

---

## 2. Платформы и окружения

Проект поддерживает следующие платформы:

| Платформа | Поддерживаемые транспорты | Особенности |
|-----------|---------------------------|-------------|
| **ESP8266** | WiFi (MQTT) | OTA ограничен, нет BLE |
| **ESP32** | WiFi (MQTT) + BLE | Полная поддержка BLE/AP |
| **ESP32-C3** | WiFi (MQTT) | BLE работает, но AP+BLE не поддерживается |
| **ESP32-C6** | WiFi (MQTT) или Zigbee | Выбор транспорта через флаг |
| **ESP32-H2** | Zigbee/Thread | Планируется |
| **ESP32-S3** | WiFi (MQTT) + BLE | Полная поддержка |

---

## 3. Структура platformio.ini

### 3.1. Общие настройки (`[common]`)

```ini
[common]
framework = arduino
monitor_speed = 115200
upload_speed = 460800
monitor_filters = direct
build_flags =
  -DMONITOR_SPEED=115200
  -DDEVICE_TYPE=1
  -DVERSION=\"1.10.0\"
  -DMAGIC_VALUE=0x5A6D
  -DPB_ENABLE_MALLOC=0
  -DPB_FIELD_16BIT=1
```

| Флаг | Значение | Описание |
|------|----------|----------|
| `DEVICE_TYPE` | 1 | Тип устройства (1, 2, 3) |
| `VERSION` | "1.10.0" | Версия прошивки |
| `MAGIC_VALUE` | 0x5A6D | Идентификатор валидности EEPROM |
| `PB_ENABLE_MALLOC` | 0 | Отключить malloc в Nanopb (экономия RAM) |
| `PB_FIELD_16BIT` | 1 | Использовать 16-битные поля в Nanopb |

---

### 3.2. Окружения

#### 3.2.1. ESP8266

```ini
[env:esp8266]
platform = espressif8266@~4.1.0
board = nodemcu
extends = common
lib_deps = 
    knolleary/PubSubClient @ ^2.8
    adafruit/Adafruit AHTX0 @ ^2.0.5
    ayushsharma82/ElegantOTA @ ^2.2.8
    adafruit/DHT sensor library@^1.4.6
build_flags =
  ${common.build_flags}
    -Wno-sign-compare
    -Wno-uninitialized
    -DTRANSPORT_TYPE=1
    -DSWITCH_PIN=14
    -DPROVISIONING_METHOD=2
    -DFEATURE_RESET_BUTTON_ENABLED=0
```

**Особенности:**
- Нет BLE → `PROVISIONING_METHOD=2` (только AP)
- Кнопка сброса отключена (`FEATURE_RESET_BUTTON_ENABLED=0`)
- Пин управления: `SWITCH_PIN=14`

---

#### 3.2.2. ESP32 (AP только)

```ini
[env:esp32-ap_only]
platform = espressif32@6.4.0
board = esp32dev
extends = common
lib_deps = 
    knolleary/PubSubClient @ ^2.8
    adafruit/Adafruit AHTX0 @ ^2.0.5
    ayushsharma82/ElegantOTA @ ^2.2.5
    adafruit/DHT sensor library@^1.4.6
build_flags =
  ${common.build_flags}
  -DTRANSPORT_TYPE=1
  -DSWITCH_PIN=4
  -DLED_INVERTED=0
  -DSTATUS_LED_PIN=2
  -DPROVISIONING_METHOD=2
```

**Особенности:**
- Только AP-провизионинг (`PROVISIONING_METHOD=2`)
- LED на пине 2 (встроенный), без инверсии (`LED_INVERTED=0`)
- Пин управления: `SWITCH_PIN=4`

---

#### 3.2.3. ESP32 (BLE только)

```ini
[env:esp32-ble_only]
platform = espressif32@6.4.0
board = esp32dev
board_build.partitions = huge_app.csv
extends = common
lib_deps = 
    knolleary/PubSubClient @ ^2.8
    adafruit/Adafruit AHTX0 @ ^2.0.5
    ayushsharma82/ElegantOTA @ ^2.2.5
    adafruit/DHT sensor library@^1.4.6
build_flags =
  ${common.build_flags}
  -DTRANSPORT_TYPE=1
  -DSWITCH_PIN=4
  -DLED_INVERTED=0
  -DSTATUS_LED_PIN=2
  -DPROVISIONING_METHOD=1
  -DCORE_DEBUG_LEVEL=4
```

**Особенности:**
- Только BLE-провизионинг (`PROVISIONING_METHOD=1`)
- Требуется `huge_app.csv` (BLE занимает много места)
- `CORE_DEBUG_LEVEL=4` для отображения списка сетей в ESP BLE Provisioning

---

#### 3.2.4. ESP32 (BLE + AP)

```ini
[env:esp32-ble-ap]
platform = espressif32@6.4.0
board = esp32dev
board_build.partitions = huge_app.csv
extends = common
lib_deps = 
    knolleary/PubSubClient @ ^2.8
    adafruit/Adafruit AHTX0 @ ^2.0.5
    ayushsharma82/ElegantOTA @ ^2.2.5
    adafruit/DHT sensor library@^1.4.6
build_flags =
  ${common.build_flags}
  -DSWITCH_PIN=4
  -DLED_INVERTED=0
  -DSTATUS_LED_PIN=2
  -DTRANSPORT_TYPE=1
  -DPROVISIONING_METHOD=3
  -DCORE_DEBUG_LEVEL=4
  -DFEATURE_MQTT_ENABLED=0
  -DFEATURE_OTA_ENABLED=0
  -DFEATURE_WDT_ENABLED=0
```

**Особенности:**
- BLE + AP одновременно (`PROVISIONING_METHOD=3`)
- MQTT, OTA, WDT отключены (экономия места)
- **Требует ESP32 (не C3!)**

---

#### 3.2.5. ESP32-C3 Super Mini

```ini
[env:esp32-c3-SuperMini]
platform = espressif32@6.4.0
board = esp32-c3-devkitm-1
board_build.partitions = huge_app.csv
extends = common
lib_deps = 
    knolleary/PubSubClient @ ^2.8
    adafruit/Adafruit AHTX0 @ ^2.0.5
    ayushsharma82/ElegantOTA @ ^2.2.5
    adafruit/DHT sensor library@^1.4.6
build_flags =
  ${common.build_flags}
  -DTRANSPORT_TYPE=1
  -DSWITCH_PIN=4
  -DRESET_PIN=9
  -DSTATUS_LED_PIN=8
  -DLED_INVERTED=1
  -DI2C_SDA_PIN=4
  -DI2C_SCL_PIN=5
  -DARDUINO_USB_MODE=1
  -DARDUINO_USB_CDC_ON_BOOT=1
  -DPROVISIONING_METHOD=2
  -DCORE_DEBUG_LEVEL=0
  -DFEATURE_MQTT_ENABLED=1
  -DFEATURE_WEB_STATUS_ENABLED=1
  -DFEATURE_OTA_ENABLED=0
  -DFEATURE_WDT_ENABLED=0
  -DSCANNING_WIFI_ENABLED=0
```

**Особенности:**
- `PROVISIONING_METHOD=2` (только AP, BLE+AP не поддерживается на C3)
- Кнопка сброса на `GPIO9` (кнопка BOOT)
- LED на `GPIO8` (встроенный синий), с инверсией (`LED_INVERTED=1`)
- I2C на `GPIO4` (SDA) и `GPIO5` (SCL)
- USB CDC включён

---

#### 3.2.6. ESP32-C6 (MQTT)

```ini
[env:esp32-c6-mqtt]
platform = espressif32@6.4.0
board = esp32-c6-devkitm-1
framework = espidf
extends = common
lib_deps = 
    knolleary/PubSubClient @ ^2.8
    adafruit/Adafruit AHTX0 @ ^2.0.5
    ayushsharma82/ElegantOTA @ ^2.2.5
    adafruit/DHT sensor library@^1.4.6
build_flags =
  ${common.build_flags}
  -DTRANSPORT_TYPE=1
  -DSWITCH_PIN=4
  -DLED_INVERTED=1
  -DSTATUS_LED_PIN=8
  -DARDUINO_USB_MODE=1
  -DARDUINO_USB_CDC_ON_BOOT=1
  -DPROVISIONING_METHOD=1
```

**Особенности:**
- Использует WiFi для MQTT
- BLE-провизионинг (`PROVISIONING_METHOD=1`)
- Фреймворк `espidf`

---

#### 3.2.7. ESP32-C6 (Zigbee)

```ini
[env:esp32-c6-zigbee]
platform = espressif32@6.4.0
board = esp32-c6-devkitm-1
framework = espidf
extends = common
lib_deps = 
  ; espressif/esp-zigbee-lib @ ^2.0.0
  ; espressif/esp-zigbee-sdk @ ^1.0.0
build_flags =
  ${common.build_flags}
  -DTRANSPORT_TYPE=2
  -DSWITCH_PIN=4
  -DLED_INVERTED=1
  -DSTATUS_LED_PIN=8
  -DARDUINO_USB_MODE=1
  -DARDUINO_USB_CDC_ON_BOOT=1
```

**Особенности:**
- Использует Zigbee (не WiFi)
- `TRANSPORT_TYPE=2` (ZIGBEE)
- Библиотеки ESP-Zigbee-SDK закомментированы (ожидают реализации)

---

## 4. Доступные флаги компиляции

### 4.1. Обязательные

| Флаг | Значения | Описание |
|------|----------|----------|
| `DEVICE_TYPE` | 1, 2, 3 | Тип устройства |
| `TRANSPORT_TYPE` | 0, 1, 2, 3 | Транспорт (NONE/WIFI/ZIGBEE/THREAD) |
| `VERSION` | "x.y.z" | Версия прошивки |

### 4.2. Функциональные

| Флаг | Значения | Описание |
|------|----------|----------|
| `FEATURE_MQTT_ENABLED` | 0, 1 | Включить MQTT-клиент |
| `FEATURE_WEB_STATUS_ENABLED` | 0, 1 | Включить веб-интерфейс |
| `FEATURE_OTA_ENABLED` | 0, 1 | Включить OTA-обновления |
| `FEATURE_WDT_ENABLED` | 0, 1 | Включить сторожевой таймер |
| `FEATURE_SENSOR_ENABLED` | 0, 1 | Включить датчик |
| `FEATURE_LED_ENABLED` | 0, 1 | Включить LED-индикацию |
| `FEATURE_RESET_BUTTON_ENABLED` | 0, 1 | Включить кнопку сброса |

### 4.3. Provisioning

| Флаг | Значения | Описание |
|------|----------|----------|
| `PROVISIONING_METHOD` | 0, 1, 2, 3 | NONE / BLE / AP / BLE+AP |

### 4.4. Пины

| Флаг | Значение | Описание |
|------|----------|----------|
| `SWITCH_PIN` | GPIO номер | Пин управления реле |
| `STATUS_LED_PIN` | GPIO номер | Пин светодиода (0 = отключён) |
| `RESET_PIN` | GPIO номер | Пин кнопки сброса |
| `I2C_SDA_PIN` | GPIO номер | SDA для датчика |
| `I2C_SCL_PIN` | GPIO номер | SCL для датчика |

### 4.5. Логирование

| Флаг | Значения | Описание |
|------|----------|----------|
| `XLOG_LEVEL` | 0-4 | NONE/ERROR/WARN/INFO/DEBUG |
| `XLOG_CATEGORIES` | битовая маска | Категории для фильтрации |
| `XLOG_USE_COLOR` | 0, 1 | Цветной вывод в терминал |

---

## 5. Зависимости

| Библиотека | Версия | Назначение | Лицензия |
|------------|--------|------------|----------|
| `PubSubClient` | ^2.8 | MQTT-клиент | MIT |
| `Adafruit AHTX0` | ^2.0.5 | Датчик AHT10 | BSD |
| `DHT sensor library` | ^1.4.6 | Датчик DHT11/DHT22 | MIT |
| `ElegantOTA` | ^2.2.5 / ^2.2.8 | OTA-обновления | MIT |

---

## 6. Параметры конфигурации (credentials.h)

Файл `credentials.h` (опционально) может содержать заводские настройки:

```cpp
// credentials.h
#ifndef CREDENTIALS_H
#define CREDENTIALS_H

// WiFi
#define DEFAULT_WIFI_SSID     "MyWiFi"
#define DEFAULT_WIFI_PASSWORD "MyPassword"

// MQTT
#define DEFAULT_MQTT_BROKER   "192.168.1.100"
#define DEFAULT_MQTT_PORT     1883
#define DEFAULT_MQTT_USER     ""
#define DEFAULT_MQTT_PASSWORD ""

#endif
```

**Если файл отсутствует** — используются значения по умолчанию из кода (пустые строки).

---

## 7. Примеры сборки

### 7.1. Базовая сборка

```bash
# Сборка для ESP8266
pio run -e esp8266

# Сборка для ESP32 (AP только)
pio run -e esp32-ap_only

# Сборка для ESP32 (BLE только)
pio run -e esp32-ble_only

# Сборка для ESP32-C3
pio run -e esp32-c3-SuperMini

# Сборка для ESP32-C6 (MQTT)
pio run -e esp32-c6-mqtt

# Сборка для ESP32-C6 (Zigbee)
pio run -e esp32-c6-zigbee
```

### 7.2. Сборка с загрузкой

```bash
# Собрать и загрузить
pio run -e esp8266 -t upload

# Собрать, загрузить и открыть монитор
pio run -e esp8266 -t upload && pio device monitor
```

### 7.3. Сборка с очисткой

```bash
# Очистить и собрать заново
pio run -e esp32-ble_only -t clean && pio run -e esp32-ble_only
```

---

## 8. Варианты сборки (матрица)

| Платформа | Тип устройства | Транспорт | Provisioning | Команда |
|-----------|----------------|-----------|--------------|---------|
| ESP8266 | 1 | WiFi/MQTT | AP | `pio run -e esp8266` |
| ESP32 | 1 | WiFi/MQTT | AP | `pio run -e esp32-ap_only` |
| ESP32 | 2 | WiFi/MQTT | BLE | `pio run -e esp32-ble_only` |
| ESP32 | 3 | WiFi/MQTT | BLE+AP | `pio run -e esp32-ble-ap` |
| ESP32-C3 | 1 | WiFi/MQTT | AP | `pio run -e esp32-c3-SuperMini` |
| ESP32-C6 | 1 | WiFi/MQTT | BLE | `pio run -e esp32-c6-mqtt` |
| ESP32-C6 | 1 | Zigbee | — | `pio run -e esp32-c6-zigbee` |

---

## 9. Требования к железу

| Платформа | Flash | RAM | OTA | BLE | Zigbee |
|-----------|-------|-----|-----|-----|--------|
| ESP8266 | ≥1MB | 80kB | ❌ (1MB) | ❌ | ❌ |
| ESP32 | ≥4MB | 520kB | ✅ | ✅ | ❌ |
| ESP32-C3 | ≥4MB | 400kB | ✅ | ⚠️ | ❌ |
| ESP32-C6 | ≥4MB | 512kB | ✅ | ✅ | ✅ |
| ESP32-H2 | ≥4MB | 320kB | ✅ | ✅ | ✅ |

## 10. Валидация на этапе компиляции

Проект содержит встроенные проверки в `settings.h`, которые предотвращают сборку с невалидными комбинациями флагов.

**Примеры проверок:**

| Проверка | Условие | Ошибка |
|----------|---------|--------|
| MQTT требует WiFi | `FEATURE_MQTT_ENABLED=1` и `TRANSPORT_TYPE != WIFI` | `FEATURE_MQTT_ENABLED requires TRANSPORT_TYPE=WIFI` |
| Zigbee требует ESP32-C6/H2 | `TRANSPORT_TYPE=ZIGBEE` и платформа не ESP32-C6/H2 | `TRANSPORT_TYPE=ZIGBEE is only supported on ESP32-C6 and ESP32-H2` |
| ESP32-C3 не поддерживает BLE+AP | `PROVISIONING_METHOD=3` и платформа ESP32-C3 | `ESP32-C3 does not support simultaneous BLE+AP` |
| TYPE 1/2 требует датчик | `DEVICE_TYPE=1/2` и `FEATURE_SENSOR_ENABLED=0` | `DEVICE_TYPE=1/2 requires FEATURE_SENSOR_ENABLED=1` |
| OTA требует Web | `FEATURE_OTA_ENABLED=1` и `FEATURE_WEB_ENABLED=0` | `FEATURE_OTA_ENABLED=1 requires FEATURE_WEB_ENABLED=1` |

**Все проверки выполняются на этапе препроцессинга.** Если комбинация флагов невалидна — сборка прервётся с понятной ошибкой.

**Полный список проверок:** `src/settings.h`, раздел 6.

*Конец документа*