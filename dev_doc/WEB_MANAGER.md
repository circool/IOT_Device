```cpp
/**
 * @file WEB_MANAGER.md
 * @brief Веб-интерфейс устройства
 * @note Статус: Закончен
 */
```

# WEB_MANAGER.md

## Оглавление

- [1. Назначение](#1-назначение)
- [2. API](#2-api)
  - [2.1. Отправка страниц](#21-отправка-страниц)
  - [2.2. Рендеринг](#22-рендеринг)
  - [2.3. Обработчики](#23-обработчики)
  - [2.4. Управление](#24-управление)
- [3. Маршруты](#3-маршруты)
- [4. IWebStatusProvider](#4-iwebstatusprovider)
- [5. Команды от Web](#5-команды-от-web)
- [6. Интеграция с OTA](#6-интеграция-с-ota)
- [7. Флаги компиляции](#7-флаги-компиляции)

---

## 1. Назначение

`web_manager` — HTTP-сервер для управления и настройки устройства. Обеспечивает визуальный интерфейс для мониторинга состояния, изменения параметров и оперативного управления.

**Ответственность:**
- Отображение состояния устройства (страница `/`)
- Настройка параметров (страница `/config`, форма `/save`)
- Оперативное управление (команды через `/set`)
- Интеграция с OTA через подсистему `web_ota_manager`

**Взаимодействие с оркестратором:**
- Web-слой устанавливает флаги (`g_webCommandPending`, `g_webConfigPending`, `g_webRestartPending`)
- Оркестратор читает флаги в `loop()` и выполняет соответствующие действия

---

## 2. API

### 2.1. Отправка страниц

| Функция | Назначение |
|---------|------------|
| `web_send_status_page(refreshInterval)` | Отправить страницу состояния |
| `web_send_config_page(send, context, cfg, currentMode, currentSsid, currentIp, refreshSeconds, errorMsg, successMsg)` | Отправить страницу конфигурации |
| `web_send_result_page(action, success)` | Отправить страницу результата операции |

### 2.2. Рендеринг

#### 2.2.1 Единая функция рендеринга

| Функция | Назначение |
|---------|------------|
| `render(buf, size, data)` | Универсальный рендеринг. Перегрузки для всех типов: TextField, NumberField, CheckboxField, TextBlockParams, StatusBlockParams, ButtonParams, ProgressParams, InfoBlockParams |

#### 2.2.2 Структуры для полей

| Структура | Используется для | Поля |
|-----------|------------------|------|
| `TextField` | TEXT / PASSWORD | label, name, value, placeholder, note, hideInput, required |
| `NumberField` | NUMBER / FLOAT | label, name, value, placeholder, note, min, max, step, required |
| `CheckboxField` | CHECKBOX | label, name, note, checked, required |

#### 2.2.3 Структуры для блоков

| Структура | Назначение | Поля |
|-----------|------------|------|
| `TextBlockParams` | Текстовый блок (заголовок + значение + единица) | title, value, unit, colorClass |
| `StatusBlockParams` | Блок статуса (ON/OFF) | isOn, label |
| `ButtonParams` | Кнопка | label, url, colorClass |
| `ProgressParams` | Индикатор прогресса (шкала) | percent |
| `InfoBlockParams` | Информационный блок (многострочный) | title, text, colorClass |

### 2.3. Обработчики

| Функция | Назначение |
|---------|------------|
| `web_handle_save()` | Обработчик POST `/save` |
| `web_handle_set()` | Обработчик GET `/set` |
| `web_handle_reset()` | Обработчик GET `/resetall` (опционально) |

### 2.4. Управление

| Функция | Назначение |
|---------|------------|
| `web_init()` | Инициализация Web-сервера |
| `web_update()` | Периодическая обработка HTTP-запросов |
| `web_register_status_provider(provider)` | Регистрация провайдера статуса |

---

## 3. Маршруты

| URL | Метод | Назначение |
|-----|-------|------------|
| `/` | GET | Страница состояния |
| `/config` | GET | Страница конфигурации |
| `/save` | POST | Сохранение конфигурации |
| `/set` | GET | Оперативная команда |
| `/update` | GET/POST | OTA (регистрируется `web_ota_manager`) |
| `/resetall` | GET | Сброс настроек (опционально) |

---

## 4. IWebStatusProvider

Web-слой получает данные через интерфейс `IWebStatusProvider`.

**Регистрация:**
- Оркестратор создаёт провайдер (в зависимости от `DEVICE_TYPE`)
- Регистрирует через `web_register_status_provider(provider)`

**Реализации:**

| Класс | Тип устройства | Описание |
|-------|----------------|----------|
| `FanWebStatusProvider` | TYPE 1 | Вентилятор с датчиком |
| `SensorWebStatusProvider` | TYPE 2 | Автономный датчик |
| `SwitchWebStatusProvider` | TYPE 3 | Управляемый выключатель |

---

## 5. Команды от Web

### 5.1. Флаги

| Флаг | Назначение |
|------|------------|
| `g_webCommandPending` | Команда от `/set` |
| `g_webConfigPending` | Новые настройки от `/save` |
| `g_webRestartPending` | Запрос перезагрузки от `/resetall` |

### 5.2. Команды `/set`

| Параметр | Тип | Применимость |
|----------|-----|--------------|
| `state` | `on`/`off` | TYPE 1, 3 |
| `speed` | 0-100 | TYPE 1 |
| `manualMode` | `1`/`0` | TYPE 1 |

---

## 6. Интеграция с OTA

OTA реализована как подсистема Web-слоя в отдельном модуле `web_ota_manager`.

**Документация:** `WEB_OTA_MANAGER.md`

**Взаимодействие:**
- `web_ota_manager_init(&server)` — регистрирует маршрут `/update`
- `ota_is_available()` — проверка доступности OTA на этапе выполнения
- Web-слой вызывает `ota_is_available()` для отображения кнопки OTA на странице конфигурации

---

## 7. Флаги компиляции

| Флаг | По умолчанию | Описание |
|------|--------------|----------|
| `FEATURE_WEB_STATUS_ENABLED` | 1 | Включить Web-интерфейс |
| `WEB_SHOW_RSSI` | 1 | Показывать RSSI на странице состояния |
| `DEFAULT_WEB_REFRESH` | 5 | Интервал автообновления (сек) |
| `WEB_RESET_ENABLED` | 0 | Сброс настроек через Web |

---

## 8. Заглушки

При `TRANSPORT_TYPE != TRANSPORT_TYPE_WIFI` или `FEATURE_WEB_STATUS_ENABLED == 0` все методы становятся пустыми.

## 9. Тестирование

### 9.1. Скрипт валидации

В директории `debug_tools/` находится скрипт `test_validation.sh` для проверки серверной валидации формы конфигурации.

**Назначение:**
- Проверка обработки валидных и невалидных данных
- Проверка обязательных полей (`wifiSsid`, `mqttBroker`, `mqttPort`, `mqttClientId`, `confirmSave`)
- Проверка диапазонов значений (порт, температура, скорость)
- Восстановление исходных значений после тестов

**Запуск:**
```bash
./debug_tools/test_validation.sh [IP_ADDRESS]
```

**Принцип работы:**

1. Чтение текущих значений через парсинг страницы `/config`
2. Запуск тестов с валидными и невалидными данными
3. Проверка целостности данных после тестов
4. Восстановление исходных значений

**Тесты:**

| Тест | Что проверяет | Ожидаемый результат |
|------|---------------|---------------------|
| 1 | Валидные данные | Страница успеха |
| 2 | Отсутствует `confirmSave` | Страница ошибки |
| 3 | Порт = 99999 | Страница ошибки |
| 4 | Пустой SSID | Страница ошибки |
| 5 | Температура = 999 | Страница ошибки |
| 6 | Скорость = 150 | Страница ошибки |

**Успешный вывод:**
```
==========================================
VALIDATION TEST SUITE
Target: http://192.168.100.97
==========================================
[INFO] Reading current configuration...
  Current values:
    SSID: iot
    Broker: 192.168.100.223
    Port: 1883
    Client: fan_F860
    Temp: 27.0
    Speed: 50
    Interval: 10

[INFO] Running tests...
  Valid data ... PASS
  No confirmSave ... PASS
  Invalid port (99999) ... PASS
  Empty SSID ... PASS
  Invalid temp (999) ... PASS
  Invalid speed (150) ... PASS

==========================================
SUMMARY: 6/6 passed
ALL TESTS PASSED
==========================================
[INFO] Restoring original values...
```

**Логирование ошибок:**

При запуске скрипта в логах устройства появляются записи:

```log
[WARN] [WEB] Validation error: MQTT Port out of range (port=99999)
[WARN] [WEB] Validation error: WiFi SSID is empty or too long (len=0)
[WARN] [WEB] Validation error: Low Temp out of range (val=999.0)
[WARN] [WEB] Validation error: Speed percent out of range (val=150)
```

**Примечание:** Скрипт использует IP-адрес `192.168.100.97` по умолчанию. Для другого устройства передайте IP как аргумент: `./test_validation.sh 192.168.1.100`
