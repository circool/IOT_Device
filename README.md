# Universal Smart Home Device

[![Platform](https://img.shields.io/badge/platform-ESP8266%20%7C%20ESP32-blue)](https://github.com/)
[![Framework](https://img.shields.io/badge/framework-Arduino-red)](https://www.arduino.cc/)
[![License](https://img.shields.io/badge/license-MIT-green)](LICENSE)
[![MQTT](https://img.shields.io/badge/MQTT-v3.1.1-orange)](https://mqtt.org/)

Универсальное устройство для умного дома с поддержкой датчиков температуры/влажности, ШИМ-управления вентилятором, MQTT и веб-интерфейсом. Идеально подходит для автоматизации вытяжной вентиляции в ванной и туалете.

## Особенности

- **Три типа устройств** в одной прошивке:
  - Вентилятор с датчиками (автоматическое управление)
  - Автономный датчик температуры/влажности
  - Управляемый выключатель

- **Smart-управление**:
  - Автоматическое включение по температуре/влажности
  - Ручное управление через MQTT и Web
  - Таймер отложенного включения
  - Аварийное отключение по таймеру

- **Тихий режим (ШИМ)**:
  - Регулировка скорости 0-100%
  - **Адаптивный режим** — автоматическая подстройка мощности
  - Стартовый импульс для надёжного запуска

- **Поддержка платформ**:
  - ESP8266 (NodeMCU, ESP-12F)
  - ESP32 (любые версии)

- **Датчики**:
  - AHT10 (I2C)
  - DHT11/DHT22 (1-Wire)

- **Связь и управление**:
  - MQTT с LWT и autodiscovery-ready топиками
  - Веб-интерфейс для настройки
  - OTA обновления прошивки
  - Точка доступа (AP) для первоначальной настройки
  - Сброс настроек кнопкой / MQTT / Web

- **Надёжность**:
  - Хранение настроек в EEPROM с CRC16
  - Watchdog Timer (WDT)
  - Автоматическое восстановление после потери связи

## Содержание

- [Поддерживаемые устройства](#-поддерживаемые-устройства)
- [Аппаратные требования](#-аппаратные-требования)
- [Схема подключения](#-схема-подключения)
- [Установка прошивки](#-установка-прошивки)
- [Конфигурация](#-конфигурация)
- [MQTT топики](#-mqtt-топики)
- [Логика работы](#-логика-работы)
- [TODO](#-todo)
- [Лицензия](#-лицензия)

## Поддерживаемые устройства

| Тип | DEVICE_TYPE | Компоненты | Применение |
|-----|-------------|-----------|------------|
| **Вентилятор** | 1 | Датчик + ШИМ + реле | Вытяжка в ванной/туалете |
| **Датчик** | 2 | Только датчик | Мониторинг микроклимата |
| **Выключатель** | 3 | Реле (без датчика) | Управление освещением/нагрузкой |

## 🛠 Аппаратные требования

### Компоненты (максимальная конфигурация)

| Компонент | Модель | Примечание |
|-----------|--------|------------|
| Микроконтроллер | ESP8266 (NodeMCU) или ESP32 | Любая плата с WiFi |
| Датчик (опционально) | AHT10 или DHT11/DHT22 | I2C или 1-Wire |
| Исполнительное реле | Твердотельное реле G3MB-202P | Должно поддерживать ШИМ! |
| Питание | AC-DC 220V → 3.3V | Подключается параллельно освещению |
| Кнопка сброса | Тактовая на замыкание | GPIO0 → GND |

### Назначение пинов

| Функция | ESP8266 | ESP32 |
|---------|---------|-------|
| Управление (PWM) | GPIO14 | GPIO4 |
| AHT10 SDA | GPIO4 | GPIO21 |
| AHT10 SCL | GPIO5 | GPIO22 |
| DHT11/22 | GPIO2 | GPIO2 |
| Кнопка сброса | GPIO0 | GPIO0 |

**Важно:** Для работы ШИМ-регулировки обязательно используйте **твердотельное реле (SSR)**, а не механическое!

## Схема подключения

```
220V AC →→→ AC-DC 3.3V →→→ ESP
           ↓
         Свет
           
220V AC →→→ SSR (реле) →→→ Вентилятор
           ↑
        GPIO (PWM)
           
ESP →→→ I2C →→→ AHT10 (SDA/SCL)
```

**Принцип работы:**
1. Свет включён → устройство получает питание
2. Устройство решает: включать вентилятор или нет (по датчикам)
3. Если нужно — подаёт ШИМ-сигнал на реле
4. Свет выключен → устройство обесточено → вентилятор выключен

## Установка прошивки

### Способ 1: PlatformIO (рекомендуется)

```bash
# Клонировать репозиторий
git clone https://github.com/yourname/smart-fan-controller.git
cd smart-fan-controller

# Собрать для ESP8266
pio run -e esp8266 -t upload

# Или для ESP32
pio run -e esp32 -t upload
```

### Способ 2: Arduino IDE

1. Установите поддержку ESP8266/ESP32 через Boards Manager
2. Установите библиотеки:
   - PubSubClient by Nick O'Leary
   - Adafruit AHTX0
   - DHT sensor library
   - ElegantOTA
3. Откройте `main.cpp`, выберите плату и загрузите

### Первоначальная настройка

1. **Первый запуск** — устройство создаст точку доступа `Fan_XXXX` (или `Sensor_XXXX`/`Switch_XXXX`)
2. **Подключитесь** к этой WiFi сети (пароль отсутствует)
3. **Откройте браузер** по адресу `http://192.168.4.1`
4. **Заполните настройки:**
   - WiFi SSID и пароль
   - MQTT брокер (адрес и порт)
   - Параметры управления (при необходимости)
5. **Сохраните** — устройство перезагрузится и подключится к вашей сети

## Конфигурация

### Настройки в коде (config.h)

```cpp
// Тип устройства
#define DEVICE_TYPE 1  // 1=Fan, 2=Sensor, 3=Switch

// Датчик (для TYPE 1,2)
#define SENSOR_TYPE 1  // 1=AHT10, 2=DHT

// Аппаратные пины
#define SWITCH_PIN 4     // Пин управления (ESP32)
#define STATUS_LED_PIN 2 // LED индикации (0 = отключён)
#define RESET_PIN 0      // Кнопка сброса

// Функционал
#define WIFI_ENABLED 1
#define MQTT_ENABLED 1
#define WEB_ENABLED 1
#define OTA_ENABLED 1
#define WDT_ENABLED 1

// Тайминги
#define WDT_TIMER_MS 5000           // Watchdog таймер
#define AP_FALLBACK_TIMEOUT_MS 12000 // Таймаут до перехода в AP

// ШИМ параметры (для TYPE 1)
#define PWM_FREQUENCY 5      // Частота ШИМ (Гц)
#define PWM_RESOLUTION 8     // Разрешение (бит)
#define PWM_STARTING 200     // Стартовый импульс (мс)
#define DEFAULT_SPEED_PERCENT 50 // Скорость по умолчанию (%)

// Адаптивный режим
#define ADAPTIVE_EPSILON_TEMP 0.5   // Чувствительность температуры
#define ADAPTIVE_EPSILON_HUM 2.0    // Чувствительность влажности
#define ADAPTIVE_STEP_SIZE 10       // Шаг адаптации (%)
#define MIN_SPEED_PERCENT 10        // Минимальная скорость при адаптации

// Пороги по умолчанию (для TYPE 1)
#define DEFAULT_LOW_TEMP 27.0
#define DEFAULT_HIGH_TEMP 29.0
#define DEFAULT_LOW_HUM 55.0
#define DEFAULT_HIGH_HUM 60.0
```

Вот подробный подраздел про настройки в `platformio.ini`:

```markdown
### Настройки в среде (platformio.ini)

Файл `platformio.ini` содержит все параметры сборки для разных платформ. Раздел разделён на общие настройки (`[common]`) и специфичные для каждой платформы (`[env:esp32]`, `[env:esp8266]`).

#### Структура файла

```ini
[common]                           # Общие настройки для всех платформ
framework = arduino                # Фреймворк
monitor_speed = 115200             # Скорость Serial порта
upload_speed = 460800              # Скорость загрузки прошивки
build_flags =                      # Флаги компиляции (общие)
    -DMAGIC_VALUE=0x5A6B          # Магическое число для EEPROM
    -DDEVICE_TYPE=1               # Тип устройства (1=Fan, 2=Sensor, 3=Switch)
    -DVERSION=\"1.4\"             # Версия прошивки
    -DSTATUS_LED_PIN=2            # Пин статусного LED (0=отключён)

[env:esp32]                        # Сборка для ESP32
platform = espressif32             # Платформа
board = esp32dev                   # Тип платы
extends = common                   # Наследование общих настроек
lib_deps = ...                     # Зависимости (библиотеки)
build_flags =                      # Специфичные флаги для ESP32
    ${common.build_flags}         # Включить общие флаги
    -DSWITCH_PIN=4                # Пин управления для ESP32
    -DLED_INVERTED=0              # Инверсия LED (0=нормальная логика)

[env:esp8266]                      # Сборка для ESP8266
platform = espressif8266           # Платформа
board = nodemcu                    # Тип платы
extends = common                   # Наследование общих настроек
build_flags = 
    ${common.build_flags}
    -DSWITCH_PIN=14               # Пин управления для ESP8266
    -DLED_INVERTED=1              # Инверсия LED (1=инвертированная)
board_build.flash_size = 4MB      # Размер flash памяти
board_build.flash_mode = dout     # Режим flash (для ESP8266)
```

#### Основные параметры сборки

| Параметр | Значение | Описание |
|----------|----------|----------|
| `-DMAGIC_VALUE` | `0x5A6B` | Идентификатор корректной конфигурации в EEPROM |
| `-DDEVICE_TYPE` | `1`, `2`, `3` | Тип устройства (см. таблицу ниже) |
| `-DVERSION` | `"1.4"` | Версия прошивки (отображается в веб-интерфейсе) |
| `-DSTATUS_LED_PIN` | `2` | Пин для LED индикации (`0` = отключить) |
| `-DSWITCH_PIN` | `4` (ESP32) / `14` (ESP8266) | Пин управления реле/вентилятором |
| `-DLED_INVERTED` | `0` или `1` | Инверсия логики LED (`1` = HIGH выключает) |

#### Типы устройств (DEVICE_TYPE)

```ini
# Вентилятор с датчиками (полная функциональность)
-DDEVICE_TYPE=1

# Только датчик (без управления нагрузкой)
-DDEVICE_TYPE=2

# Управляемый выключатель (без датчика)
-DDEVICE_TYPE=3
```

#### Включение/отключение функционала

```ini
# Основные модули (по умолчанию ВСЕ включены)
-DWIFI_ENABLED=1          # WiFi поддержка
-DMQTT_ENABLED=1          # MQTT клиент
-DWEB_ENABLED=1           # Веб-сервер
-DOTA_ENABLED=1           # OTA обновления
-DWDT_ENABLED=1           # Watchdog Timer

# Отключение ненужного функционала (экономия памяти)
; -DMQTT_ENABLED=0        # Пример отключения MQTT
; -DWIFI_ENABLED=0        # Полностью отключить WiFi
; -DWDT_ENABLED=0         # Отключить Watchdog (только для отладки)
```

#### Настройка отладки (логирование)

```ini
# Глобальное включение отладки
-DDEBUG_ENABLED=1

# Детальное логирование компонентов (работает только при DEBUG_ENABLED=1)
-DLOG_SENSOR=1            # Логи датчика
-DLOG_CONFIG=1            # Логи конфигурации
-DLOG_FAN=1               # Логи вентилятора
-DLOG_MQTT=1              # Логи MQTT
-DLOG_WIFI=1              # Логи WiFi
-DLOG_WEB=1               # Логи веб-сервера
-DLOG_AP=1                # Логи режима точки доступа
-DLOG_OTA=1               # Логи OTA обновлений
-DLOG_LED=1               # Логи LED индикации
```

**Пример включения полной отладки:**
```ini
build_flags =
    ${common.build_flags}
    -DDEBUG_ENABLED=1
    -DLOG_SENSOR=1
    -DLOG_MQTT=1
    -DLOG_WIFI=1
```

#### Настройка MQTT функционала

```ini
# MQTT возможности (требуют MQTT_ENABLED=1)
-DMQTT_RESET_ENABLED=1            # Разрешить сброс через MQTT
-DMQTT_PUBLISH_RSSI=1             # Публиковать RSSI WiFi
-DMQTT_PUBLISH_RESET_REASON=1     # Публиковать причину перезагрузки
-DMQTT_IGNORE_PUBLISH_NORMAL_RESET_REASONS=1  # Не публиковать штатные перезагрузки

# Настройки MQTT соединения
-DMQTT_RECONNECT_DELAY_MS=5000    # Задержка переподключения (мс)
-DMQTT_KEEPALIVE_SEC=3            # Keepalive интервал (сек)
```

#### Настройка веб-интерфейса

```ini
# WEB возможности (требуют WEB_ENABLED=1)
-DWEB_STATUS_ENABLED=1            # Страница статуса
-DWEB_SHOW_RSSI=1                 # Отображать RSSI на странице
-DWEB_RESET_ENABLED=1             # Кнопка сброса в веб-интерфейсе

# Настройки точки доступа (AP)
-DAP_ENABLED=1                    # Включить режим AP
-DAP_FALLBACK_TIMEOUT_MS=12000    # Таймаут до перехода в AP (мс)
-DAP_IP_ADDRESS="192.168.4.1"     # IP адрес в режиме AP
```

#### Настройки Watchdog (WDT)

```ini
# Параметры Watchdog (требуют WDT_ENABLED=1)
-DWDT_TIMER_MS=5000               # Таймаут WDT (мс)
-DLOOP_WATCHDOG_MULTIPLIER=3      # Множитель для основного цикла
```

#### Настройки датчиков

```ini
# Тип датчика (для DEVICE_TYPE=1 или 2)
-DSENSOR_TYPE=1                   # 1 = AHT10 (I2C), 2 = DHT11/22 (GPIO)

# Параметры для DHT (если SENSOR_TYPE=2)
-DSENSOR_PIN=2                    # GPIO пин для DHT
-DDHT_TYPE=DHT11                  # Тип DHT (DHT11, DHT22, DHT21)

# Интервал опроса датчика (сек)
-DSENSOR_DURATION=10
```

#### Настройки ШИМ (для вентилятора)

```ini
# ШИМ параметры (для DEVICE_TYPE=1)
-DPWM_ENABLED=1                   # Включить ШИМ
-DPWM_FREQUENCY=5                 # Частота ШИМ (Гц) — 5 Гц оптимально для SSR
-DPWM_RESOLUTION=8                # Разрешение ШИМ (бит) — 8 бит = 0-255
-DPWM_STARTING=200                # Длительность стартового импульса (мс)

# Адаптивный режим (требует PWM_ENABLED=1)
-DAPAPTIVE_ENABLED=1              # Включить адаптивный режим
-DDEFAULT_SPEED_PERCENT=50        # Скорость по умолчанию (%)
-DMIN_SPEED_PERCENT=10            # Минимальная скорость при адаптации
-DADAPTIVE_EPSILON_TEMP=0.5       # Чувствительность температуры (°C)
-DADAPTIVE_EPSILON_HUM=2.0        # Чувствительность влажности (%)
-DADAPTIVE_STEP_SIZE=10           # Шаг изменения скорости (%)
-DADAPTIVE_SPEED_SENSITIVITY=0.7  # Чувствительность к скорости изменения
```

#### Настройки аварийной защиты

```ini
# Таймеры безопасности (для DEVICE_TYPE=1 или 3)
-DEMERGENCY_ENABLED=1             # Включить аварийное отключение
-DMAX_ON_TIME_SEC=3600            # Максимальное время работы (сек) — 1 час

# Таймер отложенного включения
-DDEFAULT_DELAY_SECONDS=60        # Задержка по умолчанию (сек)
-DBOOT_SWITCH_STATE=0             # Состояние при старте (0=выкл, 1=вкл)
```

#### Настройки WiFi

```ini
# Параметры WiFi (требуют WIFI_ENABLED=1)
-DWIFI_CONNECT_TIMEOUT_MS=30000   # Таймаут подключения (мс)
-DWIFI_OUTPUT_POWER=15.0          # Мощность передатчика (0-20.5 dBm)

# Включить сканирование сетей при старте (только для отладки)
-DDEBUG_WIFI_ENABLED=1
```

#### Примеры готовых конфигураций

**1. Минимальная конфигурация (только датчик, без WiFi):**
```ini
[env:esp32]
build_flags =
    -DDEVICE_TYPE=2
    -DWIFI_ENABLED=0
    -DMQTT_ENABLED=0
    -DWEB_ENABLED=0
    -DSENSOR_TYPE=1
    -DSTATUS_LED_PIN=0
```

**2. Полная конфигурация с отладкой:**
```ini
[env:esp32]
build_flags =
    ${common.build_flags}
    -DDEBUG_ENABLED=1
    -DLOG_SENSOR=1
    -DLOG_MQTT=1
    -DLOG_WIFI=1
    -DMQTT_PUBLISH_RSSI=1
    -DWEB_SHOW_RSSI=1
```

**3. Выключатель с таймером (без датчика):**
```ini
[env:esp8266]
build_flags =
    ${common.build_flags}
    -DDEVICE_TYPE=3
    -DSWITCH_PIN=14
    -DDEFAULT_DELAY_SECONDS=30
    -DMAX_ON_TIME_SEC=1800
    -DBOOT_SWITCH_STATE=0
```

**4. Тихий режим с адаптацией:**
```ini
[env:esp32]
build_flags =
    ${common.build_flags}
    -DDEVICE_TYPE=1
    -DPWM_ENABLED=1
    -DAPAPTIVE_ENABLED=1
    -DPWM_FREQUENCY=5
    -DDEFAULT_SPEED_PERCENT=70
    -DMIN_SPEED_PERCENT=20
    -DADAPTIVE_STEP_SIZE=5
```

#### Важные замечания

1. **Приоритет флагов:** Специфичные для платформы флаги переопределяют общие
2. **Экономия памяти:** Отключайте ненужный функционал (`MQTT_ENABLED=0`, `WEB_ENABLED=0`) для ESP8266 (особенно важно при ограниченной flash)
3. **Частота ШИМ:** Для твердотельных реле оптимальна 5-10 Гц. Более высокая частота может вызвать перегрев реле
4. **ESP8266 нюансы:**
   - Требуется `-Wno-sign-compare` и `-Wno-uninitialized` для подавления предупреждений
   - Режим flash `dout` обязателен для некоторых плат
   - LED часто инвертирован (`LED_INVERTED=1`)

#### Изменение параметров без перекомпиляции

Некоторые параметры можно изменить через веб-интерфейс или MQTT без перепрошивки:
- Пороги температуры/влажности
- Скорость вентилятора
- Таймеры (задержка, аварийное отключение)
- Режимы работы (ручной/автоматический)

Параметры, требующие перекомпиляции:
- Тип устройства (`DEVICE_TYPE`)
- Назначение пинов (`SWITCH_PIN`, `STATUS_LED_PIN`)
- Частота ШИМ (`PWM_FREQUENCY`)
- Включение/отключение модулей (`MQTT_ENABLED`, `WEB_ENABLED`)
```

### Файл credentials.h (опционально)

Создайте этот файл для заводских настроек:

```cpp
// credentials.h
#define SSID_NAME "MyWiFi"
#define WIFI_PASSWORD "MyPassword"
#define MQTT_ADDRESS "192.168.1.100"
#define MQTT_PORT 1883
#define MQTT_USER "user"
#define MQTT_PASSWORD "pass"
```

## MQTT топики

Устройство использует префикс вида `{Type}_{ID}` (например `Fan_D84C`).

### Общие топики

| Топик | Направление | Формат | Описание |
|-------|------------|--------|----------|
| `{prefix}/status` | Dev → Broker | `Online`/`Offline` | Статус (LWT) |
| `{prefix}/version` | Dev → Broker | `1.4` | Версия прошивки |
| `{prefix}/c/system/reset` | Broker → Dev | `1` | Сброс настроек |

### Для вентилятора (TYPE 1)

| Топик | Направление | Описание |
|-------|------------|----------|
| `{prefix}/fan/state` | Dev → Broker | `ON`/`OFF` |
| `{prefix}/c/fan/state` | Broker → Dev | Управление |
| `{prefix}/fan/speed` | Dev → Broker | Скорость (0-100%) |
| `{prefix}/c/fan/speed` | Broker → Dev | Установка скорости |
| `{prefix}/fan/adaptiveMode` | Dev → Broker | Адаптивный режим |
| `{prefix}/fan/delaySec` | Dev → Broker | Задержка включения |
| `{prefix}/fan/maxOnTime` | Dev → Broker | Аварийное отключение |
| `{prefix}/fan/sensorControlMode` | Dev → Broker | Режим датчика |
| `{prefix}/sensor/temperature` | Dev → Broker | Температура |
| `{prefix}/sensor/humidity` | Dev → Broker | Влажность |
| `{prefix}/sensor/lowTemp` | Dev → Broker | Нижний порог T |
| `{prefix}/c/sensor/highTemp` | Broker → Dev | Верхний порог T |

### Для датчика (TYPE 2)

| Топик | Направление | Описание |
|-------|------------|----------|
| `{prefix}/sensor/temperature` | Dev → Broker | Температура |
| `{prefix}/sensor/humidity` | Dev → Broker | Влажность |

### Для выключателя (TYPE 3)

| Топик | Направление | Описание |
|-------|------------|----------|
| `{prefix}/switch/state` | Dev → Broker | Состояние |
| `{prefix}/c/switch/state` | Broker → Dev | Управление |
| `{prefix}/switch/delaySec` | Dev → Broker | Задержка включения |
| `{prefix}/switch/maxOnTime` | Dev → Broker | Аварийное отключение |

## Логика работы

### Режимы работы

1. **Режим датчика** (`sensorControlMode = 1`):
   - Включение: температура ≥ `highTemp` **ИЛИ** влажность ≥ `highHum`
   - Выключение: температура ≤ `lowTemp` **И** влажность ≤ `lowHum`

2. **Ручной режим** (`sensorControlMode = 0`):
   - Игнорирует датчик
   - Управление только через MQTT/Web

3. **Адаптивный режим** (при `speedPercent < 100`):
   - Автоматически повышает/понижает скорость
   - Старается удержать температуру/влажность на уровне момента включения

### Таймеры

- **Отложенное включение** (`delaySeconds`):
  - Запускается при выключении вентилятора
  - По истечении — принудительное включение

- **Аварийное отключение** (`maxOnTime`):
  - Защита от перегрева
  - По истечении — выключение + переход в ручной режим

### Приоритеты команд

1. **Ручное управление** (MQTT/Web) — наивысший приоритет
2. **Таймер отложенного включения**
3. **Режим управления датчиком**

## Компиляция из исходников

### Структура проекта

```
├── main.cpp           # Основной цикл
├── config.h/cpp       # Настройки и EEPROM
├── mqtt.h/cpp         # MQTT клиент
├── web.h/cpp          # Веб-сервер
├── sensor.h/cpp       # Датчики
├── fan.h/cpp          # Управление вентилятором
├── switch.h/cpp       # Управление выключателем
├── led.h/cpp          # LED индикация
├── ansi.h             # Цветной вывод в Serial
├── web_strings.h      # HTML строки
└── platformio.ini     # Конфигурация сборки
```

### PlatformIO конфигурация

```ini
[env:esp32]
platform = espressif32
board = esp32dev
build_flags = 
    -DDEVICE_TYPE=1
    -DMQTT_ENABLED=1
    -DWEB_ENABLED=1
lib_deps = 
    knolleary/PubSubClient @ ^2.8
    me-no-dev/AsyncTCP @ ^1.1.1
    https://github.com/me-no-dev/ESPAsyncWebServer.git
    adafruit/Adafruit AHTX0 @ ^2.0.5
    ayushsharma82/ElegantOTA @ ^3.1.0
```

### Отладка

```cpp
// В platformio.ini
build_flags = 
    -DDEBUG_ENABLED=1           // Включить отладку
    -DLOG_SENSOR=1              // Логи датчика
    -DLOG_MQTT=1                // Логи MQTT
    -DLOG_WIFI=1                // Логи WiFi
    -DLOG_WEB=1                 // Логи Web
```



## TODO

- [ ] Добавить поддержку BME280 (давление)
- [ ] Реализовать MQTT Discovery для Home Assistant
- [ ] Добавить поддержку ESP32-C6 (ZigBee/Matter)
- [ ] Оптимизировать энергопотребление для батарейного питания
- [ ] Добавить графики температуры в веб-интерфейс
- [ ] Реализовать экспорт логов через Web


## 📄 Лицензия

Распространяется под лицензией MIT. Смотрите файл `LICENSE` для деталей.

---

