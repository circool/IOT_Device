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
- [3. Флаги компиляции](#3-флаги-компиляции)
  - [3.1. Обязательные](#31-обязательные)
  - [3.2. Функциональные](#32-функциональные)
  - [3.3. Provisioning](#33-provisioning)
  - [3.4. Пины](#34-пины)
- [4. Проверки совместимости](#4-проверки-совместимости)
- [5. Параметры конфигурации (credentials.h)](#5-параметры-конфигурации-credentialsh)
- [6. Зависимости](#6-зависимости)
- [7. Примеры сборки](#7-примеры-сборки)
- [8. Матрица сборки](#8-матрица-сборки)

---

## 1. Назначение

Документ описывает процесс сборки проекта для разных платформ и конфигураций.

---

## 2. Платформы и окружения

| Платформа | Транспорт | Provisioning | Окружение |
|-----------|-----------|--------------|-----------|
| ESP8266 | WiFi (MQTT) | AP | `esp8266` |
| ESP32 | WiFi (MQTT) | AP / BLE / AP+BLE | `esp32-*` |
| ESP32-C3 | WiFi (MQTT) | AP | `esp32-c3-SuperMini` |
| ESP32-C6 | WiFi (MQTT) / Zigbee | BLE / — | `esp32-c6-*` |

**Список окружений:**

| Окружение | Платформа | Provisioning | Особенности |
|-----------|-----------|--------------|-------------|
| `esp8266` | ESP8266 | AP | Нет BLE |
| `esp32-ap_only` | ESP32 | AP | Только AP |
| `esp32-ble_only` | ESP32 | BLE | Только BLE |
| `esp32-ble-ap` | ESP32 | BLE+AP | Требует ESP32 (не C3) |
| `esp32-c3-SuperMini` | ESP32-C3 | AP | BLE+AP не поддерживается |
| `esp32-c6-mqtt` | ESP32-C6 | BLE | WiFi + MQTT |
| `esp32-c6-zigbee` | ESP32-C6 | — | Zigbee (заглушка) |

---

## 3. Флаги компиляции

### 3.1. Разделение флагов

Флаги делятся на **пользовательские** и **внутренние**.

**Пользовательские** (задаются в `platformio.ini`):
- `FEATURE_XXX_ENABLED` — пользователь запрашивает функцию
- `PROVISIONING_METHOD` — метод настройки
- `DEVICE_TYPE` — тип устройства

**Внутренние** (вычисляются в `settings.h`):
- `USE_XXX` — реально ли функция работает (с учётом аппаратных ограничений)
- Используются в коде для включения/исключения блоков

**Принцип:** Пользователь указывает желания → `settings.h` проверяет возможность → код использует только то, что реально работает.

---

### 3.2. Обязательные

| Флаг | Значения | Описание |
|------|----------|----------|
| `DEVICE_TYPE` | 1, 2, 3 | Тип устройства |
| `TRANSPORT_TYPE` | 0, 1, 2, 3 | NONE / WIFI / ZIGBEE / THREAD |
| `VERSION` | "x.y.z" | Версия прошивки |

### 3.3. Функциональные

| Флаг | Значения | Описание | Зависимости |
|------|----------|----------|-------------|
| `FEATURE_MQTT_ENABLED` | 0, 1 | MQTT-клиент | Требует `TRANSPORT_TYPE=WIFI` |
| `FEATURE_WEB_ENABLED` | 0, 1 | Веб-интерфейс | Требует `TRANSPORT_TYPE=WIFI` |
| `FEATURE_OTA_ENABLED` | 0, 1 | OTA-обновления | Требует `FEATURE_WEB_ENABLED=1` |
| `FEATURE_WDT_ENABLED` | 0, 1 | Сторожевой таймер | — |
| `FEATURE_SENSOR_ENABLED` | 0, 1 | Датчик | Требует `DEVICE_TYPE=1/2` |
| `FEATURE_LED_ENABLED` | 0, 1 | LED-индикация | — |
| `FEATURE_BUTTON_ENABLED` | 0, 1 | Кнопка управления | — |

### 3.4. Provisioning

| Флаг | Значения | Описание |
|------|----------|----------|
| `PROVISIONING_METHOD` | 0, 1, 2, 3 | NONE / BLE / AP / BLE+AP |

### 3.5. Пины

| Флаг | Описание |
|------|----------|
| `SWITCH_PIN` | Пин управления реле |
| `STATUS_LED_PIN` | Пин светодиода (0 = отключён) |
| `RESET_PIN` | Пин кнопки сброса |
| `I2C_SDA_PIN` | SDA для датчика |
| `I2C_SCL_PIN` | SCL для датчика |

---

## 4. Проверки совместимости

Все проверки выполняются на этапе препроцессинга в `settings.h`. При невалидной комбинации — ошибка компиляции.

| Проверка | Условие ошибки |
|----------|----------------|
| MQTT требует WiFi | `FEATURE_MQTT_ENABLED=1` и `TRANSPORT_TYPE != WIFI` |
| Web требует WiFi | `FEATURE_WEB_ENABLED=1` и `TRANSPORT_TYPE != WIFI` |
| OTA требует Web | `FEATURE_OTA_ENABLED=1` и `FEATURE_WEB_ENABLED=0` |
| Zigbee требует ESP32-C6/H2 | `TRANSPORT_TYPE=ZIGBEE` и платформа не C6/H2 |
| BLE+AP требует ESP32 | `PROVISIONING_METHOD=3` и платформа не ESP32 |
| ESP32-C3 не поддерживает BLE+AP | `PROVISIONING_METHOD=3` и платформа C3 |
| TYPE 1/2 требует датчик | `DEVICE_TYPE=1/2` и `FEATURE_SENSOR_ENABLED=0` |

**Полный список:** `src/settings.h`, раздел 6.

---

## 5. Параметры конфигурации (credentials.h)

Файл `credentials.h` (опционально) содержит заводские настройки:

```cpp
// credentials.h
#define DEFAULT_WIFI_SSID     "MyWiFi"
#define DEFAULT_WIFI_PASSWORD "MyPassword"
#define DEFAULT_MQTT_ADDRESS   "192.168.1.100"
#define DEFAULT_MQTT_PORT     1883
#define DEFAULT_MQTT_USER     ""
#define DEFAULT_MQTT_PASSWORD ""
```

**Если файл отсутствует** — используются пустые значения (запускается провизионинг).

---

## 6. Зависимости

| Библиотека | Назначение |
|------------|------------|
| `PubSubClient` | MQTT-клиент |
| `Adafruit AHTX0` | Датчик AHT10 |
| `DHT sensor library` | Датчик DHT11/DHT22 |
| `ElegantOTA` | OTA-обновления |

---

## 7. Примеры сборки

```bash
# Сборка
pio run -e esp32-ap_only

# Сборка + загрузка
pio run -e esp8266 -t upload

# Очистка + сборка
pio run -e esp32-ble_only -t clean && pio run -e esp32-ble_only
```

---

## 8. Матрица сборки

| Платформа | Окружение | Транспорт | Provisioning | Команда |
|-----------|-----------|-----------|--------------|---------|
| ESP8266 | `esp8266` | WiFi/MQTT | AP | `pio run -e esp8266` |
| ESP32 | `esp32-ap_only` | WiFi/MQTT | AP | `pio run -e esp32-ap_only` |
| ESP32 | `esp32-ble_only` | WiFi/MQTT | BLE | `pio run -e esp32-ble_only` |
| ESP32 | `esp32-ble-ap` | WiFi/MQTT | BLE+AP | `pio run -e esp32-ble-ap` |
| ESP32-C3 | `esp32-c3-SuperMini` | WiFi/MQTT | AP | `pio run -e esp32-c3-SuperMini` |
| ESP32-C6 | `esp32-c6-mqtt` | WiFi/MQTT | BLE | `pio run -e esp32-c6-mqtt` |
| ESP32-C6 | `esp32-c6-zigbee` | Zigbee | — | `pio run -e esp32-c6-zigbee` |
