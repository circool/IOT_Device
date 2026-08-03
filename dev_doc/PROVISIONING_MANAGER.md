```cpp
/**
 * @file PROVISIONING_MANAGER.md
 * @brief Провизионинг
 * @note Статус: Закончен
  * @todo Необходима актуализация - см STATUS_PROVIDER.md
 */
```
# PROVISIONING_MANAGER.md

## Провизионинг (первоначальная настройка)


## Оглавление

- [1. Назначение](#1-назначение)
- [2. Когда запускается Provisioning](#2-когда-запускается-provisioning)
- [3. Методы Provisioning](#3-методы-provisioning)
- [4. Описание процессов провизионинга](#4-описание-процессов-провизионинга)
  - [4.1. AP-Provisioning (точка доступа)](#41-ap-provisioning-точка-доступа)
    - [4.1.1. Принцип работы](#411-принцип-работы)
    - [4.1.2. Страница AP-провизионинга](#412-страница-ap-провизионинга)
    - [4.1.3. Поток данных (AP)](#413-поток-данных-ap)
  - [4.2. BLE-Provisioning (ESP32)](#42-ble-provisioning-esp32)
    - [4.2.1. Принцип работы](#421-принцип-работы)
    - [4.2.2. Совместимые приложения](#422-совместимые-приложения)
  - [4.3. Zigbee-Provisioning (ESP32-C6 / ESP32-H2)](#43-zigbee-provisioning-esp32-c6--esp32-h2)
    - [4.3.1. Принцип работы](#431-принцип-работы)
    - [4.3.2. Требования к координатору](#432-требования-к-координатору)
    - [4.3.3. Реализация](#433-реализация)
    - [4.3.4. Сброс настроек](#434-сброс-настроек)
- [5. Требования и описания ответственности](#5-требования-и-описания-ответственности)
  - [5.1. Файлы](#51-файлы)
  - [5.2. Менеджер провизионинга](#52-менеджер-провизионинга)
    - [Назначение](#назначение)
    - [Архитектура взаимодействия](#архитектура-взаимодействия)
    - [Взаимодействие менеджер ↔ серверы](#взаимодействие-менеджер--серверы)
    - [Публичный интерфейс менеджера (для оркестратора)](#публичный-интерфейс-менеджера-для-оркестратора)
    - [Глобальные функции-обёртки](#глобальные-функции-обёртки)
    - [Поток данных (единый для всех серверов)](#поток-данных-единый-для-всех-серверов)
    - [Защита от дублирования](#защита-от-дублирования)
    - [Требования к слою](#требования-к-слою)
- [6. HTTP-сервер AP-провизионинга](#6-http-сервер-ap-провизионинга)
  - [6.1. Маршруты](#61-маршруты)
  - [6.2. Валидация](#62-валидация)
  - [6.3. Сообщения пользователю](#63-сообщения-пользователю)
- [7. Интеграция с оркестратором](#7-интеграция-с-оркестратором)
  - [7.1. Запуск в setup()](#71-запуск-в-setup)
  - [7.2. Обработка в loop()](#72-обработка-в-loop)
  - [7.3. Колбэк оркестратора](#73-колбэк-оркестратора)
- [8. Флаги компиляции](#8-флаги-компиляции)
  - [8.1. Пользовательские флаги](#81-пользовательские-флаги-задаются-в-platformioini)
  - [8.2. Внутренние (производные) флаги](#82-внутренние-производные-флаги)
  - [8.3. Таблица соответствия](#83-таблица-соответствия)
  - [8.4. Примеры использования в коде](#84-примеры-использования-в-коде)
- [9. Требования к доработке](#9-требования-к-доработке)
- [10. Структура серверов](#10-структура-серверов)
  - [10.1. Единый интерфейс](#101-единый-интерфейс)
  - [10.2. Детали рефакторинга BLE-сервера](#102-детали-рефакторинга-ble-сервера)
  - [10.3. Детали рефакторинга AP-сервера](#103-детали-рефакторинга-ap-сервера)
  - [10.4. Детали рефакторинга Zigbee-сервера](#104-детали-рефакторинга-zigbee-сервера)

---

## 1. Назначение

Provisioning — процесс первоначальной настройки, в ходе которого устройство получает сетевые учётные данные: Wi-Fi SSID/пароль для подключения к роутеру, либо проходит безопасную регистрацию в Mesh-сети Zigbee/Matter.

**Цель:** Устройство получает доступ к сети и становится управляемым через веб-интерфейс, а после настройки брокера — и посредством MQTT.

Для устройств, использующих **WiFi/MQTT** интеграцию, необходимо два этапа настройки; устройствам, использующим **Zigbee/Matter**, достаточно первого:

1. **Провизионинг** — получение WiFi SSID/пароля (через BLE или AP) или регистрация в Mesh-сети.
2. **Полная настройка параметров** — через веб-интерфейс (MQTT брокер, пороги, таймеры, режимы). Является отдельной процедурой и не рассматривается в этом документе.

---

## 2. Когда запускается Provisioning

| Условие | Действие |
|---------|----------|
| **Первый запуск** (EEPROM пуст) | Запускается provisioning |
| **Конфигурация повреждена** (CRC ошибка) | Запускается provisioning |
| **Нет WiFi соединения** (после таймаута) | Запускается provisioning |
| **Пользователь сбросил настройки** (кнопка 3 сек) | Запускается provisioning |

---

## 3. Методы Provisioning

| Метод | Описание | Поддерживаемые платформы |
|-------|----------|--------------------------|
| **ZIGBEE** | Автоматическое присоединение к Zigbee Mesh-сети через координатора (требуется активный Permit Join на хабе) | ESP32-C6, ESP32-H2 |
| **BLE** | Настройка через приложение ESP BLE Provisioning | ESP32, ESP32-S3, ESP32-C3, ESP32-C6, ESP32-H2 |
| **AP** | Настройка через WiFi-точку доступа и веб-интерфейс | Все платформы (ESP8266, ESP32) |
| **AP+BLE** | Одновременная работа обоих методов | ESP32 (кроме C3) |

**Выбор метода** задаётся флагом `PROVISIONING_METHOD` в `settings.h`:

```cpp
#define PROVISIONING_METHOD 2   // 0 = ZigBee, 1 = BLE, 2 = AP, 3 = AP+BLE
```

---

## 4. Описание процессов провизионинга### 4.1. AP-Provisioning (точка доступа)

#### 4.1.1. Принцип работы

1. Устройство создаёт WiFi-точку доступа с SSID `{prefix}_XXXX`, где `XXXX` — последние 4 символа MAC-адреса.
2. Пользователь подключается к этой точке доступа, открывает браузер по адресу `192.168.4.1`.
3. Пользователь вводит SSID и пароль своей WiFi-сети и отправляет форму.
4. Встроенный HTTP-сервер (`ApProvisioningServer`) принимает данные, валидирует их и уведомляет `ProvisioningManager`.
5. Менеджер передаёт данные оркестратору через колбэк.
6. Оркестратор сохраняет данные в Config и вызывает `provisioning_stop()`.
7. Менеджер останавливает `ApProvisioningServer`: HTTP-сервер уничтожается, точка доступа выключается.
8. WiFi-менеджер (уже в `loop()`) переключается в STA-режим и подключается к сохранённой сети.
9. **Перезагрузка не требуется.** Устройство продолжает работу в обычном режиме.

#### 4.1.2. Страница AP-провизионинга

```
┌─────────────────────────────────────────────────────────────┐
│                     WiFi Setup                              │
│                                                             │
│   WiFi SSID:   [________________________]                   │
│   WiFi Password: [________________________]                 │
│                                                             │
│   [ Save and Connect ]                                      │
│                                                             │
│   ℹ️ Device will connect to your WiFi network.              │
└─────────────────────────────────────────────────────────────┘
```

#### 4.1.3. Поток данных (AP)

1. Пользователь заполняет форму на `192.168.4.1`.
2. POST-запрос на `/savewifi` с параметрами `wifiSsid` и `wifiPassword`.
3. `ApProvisioningServer` обрабатывает запрос, валидирует данные.
4. **Сервер уведомляет менеджера:** вызывает `_manager->onDataReceived(data)`.
5. Менеджер вызывает колбэк оркестратора с полученными данными.
6. Оркестратор сохраняет данные в Config и вызывает `provisioning_stop()`.
7. Менеджер останавливает сервер (`_apServer.stop()`), снимает `STATE_PROVISIONING`.

---

### 4.2. BLE-Provisioning (ESP32)

#### 4.2.1. Принцип работы

1. Устройство запускает BLE-сервер с именем `{prefix}_XXXX`.
2. Пользователь открывает приложение **ESP BLE Provisioning** (Android/iOS), подключается к устройству и вводит SSID/пароль.
3. `BleProvisioningServer` получает данные через события SDK.
4. **Сервер уведомляет менеджера:** вызывает `_manager->onDataReceived(data)`.
5. Менеджер вызывает колбэк оркестратора с полученными данными.
6. Оркестратор сохраняет данные в Config и вызывает `provisioning_stop()`.
7. Менеджер останавливает `BleProvisioningServer` (очищает события), снимает `STATE_PROVISIONING`.
8. **Перезагрузка не требуется.** WiFi-менеджер подключается к сети.

#### 4.2.2. Совместимые приложения

| Приложение | Платформа | Ссылка |
|------------|-----------|--------|
| **ESP BLE Provisioning** | Android | [Play Store](https://play.google.com/store/apps/details?id=com.espressif.bleprov) |
| **ESP BLE Provisioning** | iOS | [App Store](https://apps.apple.com/app/esp-ble-provisioning/id1473590141) |

---

### 4.3. Zigbee-Provisioning (ESP32-C6 / ESP32-H2)

#### 4.3.1. Принцип работы

1. Устройство при запуске автоматически инициализирует Zigbee-стек и сканирует доступные сети.
2. При обнаружении координатора с активным Permit Join устройство автоматически присоединяется к сети.
3. Координатор передаёт устройству сетевой ключ.
4. `ZigbeeProvisioningServer` уведомляет `ProvisioningManager` об успешном присоединении (вызов `_manager->onDataReceived()`).
5. Менеджер передаёт данные оркестратору через колбэк.
6. Оркестратор сохраняет параметры сети в Config, вызывает `provisioning_stop()`.
7. Менеджер останавливает `ZigbeeProvisioningServer` (опционально), снимает `STATE_PROVISIONING`.

#### 4.3.2. Требования к координатору

На координаторе (хабе) должен быть активирован режим Permit Join (разрешение на добавление новых устройств).

#### 4.3.3. Реализация

Процесс полностью автоматизирован библиотекой `Zigbee.h`:

```cpp
Zigbee.begin();        // запуск поиска сети
Zigbee.connected();    // ожидание подключения
```

#### 4.3.4. Сброс настроек

При необходимости пользователь может сбросить устройство к заводским настройкам через кнопку очистки параметров (если установлен флаг `FEATURE_RESET_BUTTON_ENABLED`) или API (вызов `Zigbee.factoryReset()`). После сброса устройство перезагружается и повторяет процесс провизионинга.

---

## 5. Требования и описания ответственности

### 5.1. Файлы

| Файл | Назначение | Статус |
|------|------------|--------|
| `provisioning_manager.h/cpp` | `ProvisioningManager` — менеджер провизионинга | 🔄 Требуется рефакторинг |
| `ble_server.h/cpp` | `BleProvisioningServer` — BLE-сервер для ESP32 | ✅ Реализован |
| `ap_server.h/cpp` | `ApProvisioningServer` — AP-сервер для WiFi (ESP32/8266) | ⬜ Вынести в отдельный файл |
| `zigbee_server.h/cpp` | `ZigbeeProvisioningServer` — Zigbee-сервер для ESP32-C6 | ⬜ В проекте |
| `web_common.h/cpp` | Рендеринг страниц (используется provisioning совместно с другими слоями) | ✅ Реализован |

---

## 5.2. Менеджер провизионинга

### Назначение

Слой `ProvisioningManager` инкапсулирует всю логику первоначальной настройки устройства. Абстрагирует от оркестратора способы получения учётных данных (AP, BLE, Zigbee) и предоставляет единый интерфейс для управления процессом.

Менеджер управляет подчинёнными серверами (`ApProvisioningServer`, `BleProvisioningServer`, `ZigbeeProvisioningServer`) — получает от них данные и передаёт результат оркестратору через колбэк.

### Архитектура взаимодействия

**Ключевое архитектурное решение:** взаимодействие между менеджером и серверами построено по принципу **«сервер → менеджер»** — серверы уведомляют менеджера о получении данных через вызов его метода, а не через колбэк и не через опрос.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                              ORCHESTRATOR                                   │
│  • Анализ состояния системы (есть ли WiFi, валидна ли конфигурация)         │
│  • ПРИНЯТИЕ РЕШЕНИЯ: запускать провизионинг или нет                         │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ оркестратор взаимодействует только с менеджером
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│  provisioning_start(callback) → передаёт колбэк менеджеру                   │
│  provisioning_update()        → вызывает update() менеджера                 │
│  provisioning_stop()          → вызывает stop() менеджера                   │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    │ менеджер → серверы (управление)
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         PROVISIONING MANAGER                                │
│                                                                             │
│  • Хранит колбэк, переданный оркестратором                                  │
│  • Управляет серверами (start/stop/update)                                  │
│  • Принимает уведомления от серверов через ЕДИНЫЙ метод                     │
│  • Вызывает колбэк оркестратора при получении данных или ошибке             │
│  • Управляет STATE_PROVISIONING                                             │
│                                                                             │
│  ┌───────────────────────────────────────────────────────────────────────┐  │
│  │                    ВНУТРЕННИЕ СЕРВЕРЫ                                 │  │
│  │                                                                       │  │
│  │  • ApProvisioningServer   — HTTP-сервер + точка доступа               │  │
│  │  • BleProvisioningServer  — BLE-сервер                                │  │
│  │  • ZigbeeProvisioningServer — Zigbee-стек                             │  │
│  │                                                                       │  │
│  │  ВСЕ СЕРВЕРЫ УВЕДОМЛЯЮТ МЕНЕДЖЕРА ОДИНАКОВО:                          │  │
│  │  _manager->onDataReceived(data)                                       │  │
│  └───────────────────────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────────────────────┘
                                    ▲
                                    │ серверы → менеджер (ЕДИНЫЙ МЕХАНИЗМ)
                                    │ _manager->onDataReceived(data)
                                    │
          ┌─────────────────────────┼─────────────────────────┐
          │                         │                         │
          ▼                         ▼                         ▼
┌───────────────────┐    ┌───────────────────┐    ┌─────────────────────────┐
│  AP SERVER        │    │  BLE SERVER       │    │  ZIGBEE SERVER          │
│                   │    │                   │    │                         │
│  • Получил данные │    │  • Получил данные │    │  • Получил данные       │
│  • Валидировал    │    │  • Обработал      │    │  • Присоединился к сети │
│  • УВЕДОМИЛ       │    │  • УВЕДОМИЛ       │    │  • УВЕДОМИЛ             │
│    менеджера      │    │    менеджера      │    │    менеджера            │
└───────────────────┘    └───────────────────┘    └─────────────────────────┘
```

### Взаимодействие менеджер ↔ серверы

| Направление | Механизм | Описание |
|-------------|----------|----------|
| **Менеджер → серверы** | Прямой вызов методов | `server->start()`, `server->stop()`, `server->update()` |
| **Серверы → менеджер** | Прямой вызов ЕДИНОГО метода менеджера | `_manager->onDataReceived(data)` |

### Публичный интерфейс менеджера (для оркестратора)

```cpp
class ProvisioningManager {
public:
    static ProvisioningManager& getInstance();

    void init();
    void update();
    void start(ProvisioningCallback callback = nullptr, void* context = nullptr);
    void stop();

private:
    // ===== МЕТОДЫ ДЛЯ СЕРВЕРОВ (доступны через friend) =====
    friend class ApProvisioningServer;
    friend class BleProvisioningServer;
    friend class ZigbeeProvisioningServer;

    void onDataReceived(const ProvisioningData& data);
    void onError();
    void stopServers();

    // ===== ДАННЫЕ =====
    bool _started = false;
    bool _complete = false;
    ProvisioningData _data;
    ProvisioningCallback _callback = nullptr;
    void* _context = nullptr;

#if USE_AP_PROVISIONING == 1
    ApProvisioningServer _apServer;
#endif
#if USE_BLE_PROVISIONING == 1
    BleProvisioningServer _bleServer;
#endif
#if USE_ZIGBEE_PROVISIONING == 1
    ZigbeeProvisioningServer _zigbeeServer;
#endif
};
```

**Инкапсуляция:** методы `onDataReceived()` и `onError()` — приватные. Доступ к ним имеют только серверы (объявлены как `friend`). Оркестратор не знает об их существовании.

### Глобальные функции-обёртки

```cpp
void provisioning_init();
void provisioning_update();
void provisioning_start(ProvisioningCallback callback = nullptr, void* context = nullptr);
void provisioning_stop();
```

### Поток данных (единый для всех серверов)

```
1. Оркестратор → менеджер: provisioning_start(callback)
2. Менеджер → серверы: server->start(this)
3. Серверы хранят указатель на менеджер

4. Пользователь вводит данные через ЛЮБОЙ метод (AP/BLE/Zigbee)
5. Сервер → менеджер: _manager->onDataReceived(data)  // ЕДИНЫЙ МЕХАНИЗМ
6. Менеджер:
   а) проверяет, не получены ли уже данные (_complete)
   б) сохраняет данные
   в) вызывает stopServers() — останавливает все серверы
   г) вызывает callback(true, &data, context) — уведомляет оркестратора

7. Оркестратор получает данные в колбэке
8. Оркестратор → менеджер: provisioning_stop() (если не вызван автоматически)
9. Менеджер снимает STATE_PROVISIONING и очищает ресурсы
```

### Защита от дублирования

Менеджер игнорирует повторные уведомления от серверов:

```cpp
void ProvisioningManager::onDataReceived(const ProvisioningData& data) {
    if (_complete) {
        // Уже получили данные — игнорируем
        return;
    }
    // ... обработка
}
```

### Требования к слою

| # | Требование |
|---|------------|
| 1 | Публичный интерфейс: `init()`, `update()`, `start()`, `stop()` |
| 2 | Глобальные обёртки в стиле `{слой}_{действие}` |
| 3 | Слой самостоятельно управляет `STATE_PROVISIONING` |
| 4 | Оркестратор не знает о внутренней реализации |
| 5 | Серверы уведомляют менеджера через ЕДИНЫЙ метод |
| 6 | Менеджер — единственная точка входа для данных от всех серверов |
| 7 | `stop()` полностью уничтожает все ресурсы |
| 8 | Данные передаются через колбэк, опрос отсутствует |
| 9 | Защита от дублирования данных |

---

## 6. HTTP-сервер AP-провизионинга

*Данный раздел описывает внутреннюю реализацию `ApProvisioningServer`. Эти детали скрыты от оркестратора.*

### 6.1. Маршруты

| URL | Метод | Назначение |
|-----|-------|------------|
| `/` | GET | Страница ввода WiFi-настроек |
| `/savewifi` | POST | Сохранение WiFi-настроек |
| `/config` | GET | Редирект на `/` |
| `/favicon.ico` | GET | 404 |

### 6.2. Валидация

**Клиентская (HTML-форма):**
- `required` — SSID обязателен
- `maxlength="31"` — SSID не длиннее 31 символа
- `maxlength="63"` — пароль не длиннее 63 символов

**Серверная (защита от обхода):**
- Пустой SSID → ошибка
- SSID > 31 символов → ошибка
- Пароль > 63 символов → ошибка

### 6.3. Сообщения пользователю

| Ситуация | Сообщение |
|----------|-----------|
| Успешное сохранение | `"WiFi credentials saved. Device will connect to your WiFi network."` |
| Пустой SSID | `"WiFi SSID cannot be empty."` |
| SSID слишком длинный | `"WiFi SSID is too long (max 31 chars)."` |
| Пароль слишком длинный | `"WiFi password is too long (max 63 chars)."` |

---

## 7. Интеграция с оркестратором

### 7.1. Запуск в setup()

```cpp
// main.cpp
void setup() {
    // ... инициализация Logger, ConfigManager ...
    provisioning_init();
    
    // ОРКЕСТРАТОР ПРИНИМАЕТ РЕШЕНИЕ
    if (strlen(g_configManager.getWifiSsid()) < 1) {
        XLOG_INFO(CAT_MAIN, "Starting provisioning (no WiFi config)");
        provisioning_start(onProvisioningComplete, nullptr);
    }
}
```

### 7.2. Обработка в loop()

```cpp
// main.cpp
void loop() {
    // ... другие update() ...
    
    if (system_state_has_bit(STATE_PROVISIONING)) {
        provisioning_update();  // менеджер обновляет свои серверы
    }
    
    // ... остальные update() ...
}
```

### 7.3. Колбэк оркестратора

```cpp
// main.cpp
static void onProvisioningComplete(bool success, const ProvisioningData* data, void* context) {
    if (success && data) {
        XLOG_INFO(CAT_MAIN, "Provisioning completed, SSID: %s", data->wifiSsid);
        
        g_configManager.setWifiSsid(data->wifiSsid);
        g_configManager.setWifiPassword(data->wifiPassword);
        
        if (g_configManager.save()) {
            provisioning_stop();  // слой сам очистит ресурсы и снимет STATE_PROVISIONING
        }
    } else {
        XLOG_ERROR(CAT_MAIN, "Provisioning failed!");
        // Слой уже снял STATE_PROVISIONING и очистил ресурсы
    }
}
```

---

## 8. Флаги компиляции

Флаги разделены на две категории: **пользовательские** (задаются в `platformio.ini`) и **автоматические** (вычисляются в `settings.h` на основе пользовательских).

### 8.1. Пользовательские флаги (задаются в platformio.ini)

| Флаг | Значения | Описание |
|------|----------|----------|
| `TRANSPORT_TYPE` | 0 = NONE, 1 = WIFI, 2 = ZIGBEE, 3 = THREAD | Физический транспорт устройства |
| `PROVISIONING_METHOD` | 0 = нет, 1 = BLE, 2 = AP, 3 = AP+BLE | Метод провизионинга (только для WiFi) |
| `BLE_PROVISIONING_PIN` | строка (по умолчанию "abcd1234") | PIN-код для BLE-сопряжения (опционально) |

### 8.2. Внутренние (производные) флаги

Эти флаги **не задаются** пользователем. Они автоматически вычисляются на основе `TRANSPORT_TYPE` и `PROVISIONING_METHOD`:

| Флаг | Условие | Значение | Описание |
|------|---------|----------|----------|
| `USE_BLE_PROVISIONING` | `PROVISIONING_METHOD == 1` или `3` | 0 или 1 | Включить BLE-провизионинг |
| `USE_AP_PROVISIONING` | `PROVISIONING_METHOD == 2` или `3` | 0 или 1 | Включить AP-провизионинг |
| `USE_ZIGBEE_PROVISIONING` | `TRANSPORT_TYPE == 2` (ZIGBEE) | 0 или 1 | Включить Zigbee-провизионинг |

### 8.3. Таблица соответствия

| TRANSPORT_TYPE | PROVISIONING_METHOD | USE_BLE | USE_AP | USE_ZIGBEE | Что включено |
|----------------|---------------------|---------|--------|------------|--------------|
| WIFI (1) | 0 | 0 | 0 | 0 | Нет провизионинга |
| WIFI (1) | 1 | 1 | 0 | 0 | Только BLE |
| WIFI (1) | 2 | 0 | 1 | 0 | Только AP |
| WIFI (1) | 3 | 1 | 1 | 0 | BLE + AP |
| ZIGBEE (2) | любое | 0 | 0 | 1 | Только Zigbee (автоматическое присоединение) |

### 8.4. Примеры использования в коде

```cpp
// provisioning_manager.cpp — автоматическое вычисление флагов
#if PROVISIONING_METHOD == 0
#define USE_BLE_PROVISIONING 0
#define USE_AP_PROVISIONING 0
#elif PROVISIONING_METHOD == 1
#define USE_BLE_PROVISIONING 1
#define USE_AP_PROVISIONING 0
#elif PROVISIONING_METHOD == 2
#define USE_BLE_PROVISIONING 0
#define USE_AP_PROVISIONING 1
#elif PROVISIONING_METHOD == 3
#define USE_BLE_PROVISIONING 1
#define USE_AP_PROVISIONING 1
#endif

// Zigbee-провизионинг включается автоматически при выборе Zigbee-транспорта
#if TRANSPORT_TYPE == TRANSPORT_TYPE_ZIGBEE
#define USE_ZIGBEE_PROVISIONING 1
#else
#define USE_ZIGBEE_PROVISIONING 0
#endif
---

## 9. Требования к доработке

| # | Проблема | Статус | Решение |
|---|----------|--------|---------|
| 1 | **AP-страница `/savewifi` в Web-слое** | ✅ **ИСПРАВЛЕНО** | Перенесено в `ProvisioningManager` |
| 2 | **Функции точки доступа и HTTP-сервера** | ⬜ **ЗАПЛАНИРОВАНО** | Вынести в отдельный класс `ApProvisioningServer` по образу `BleProvisioningServer` |
| 3 | **Функции Zigbee** | ⬜ **ЗАПЛАНИРОВАНО** | Реализовать в отдельном классе `ZigbeeProvisioningServer` |
| 4 | **BLE-сервер использует глобальный доступ к менеджеру** | ⬜ **ЗАПЛАНИРОВАНО** | Передавать указатель на менеджер через `start(manager)` |
| 5 | **Глобальный колбэк `SysProvEvent` вне класса** | ⬜ **ЗАПЛАНИРОВАНО** | Сделать статическим методом класса `BleProvisioningServer` |
| 6 | **Именование методов BLE-сервера** | ⬜ **ЗАПЛАНИРОВАНО** | `begin()` → `start()`, добавить `isActive()` |
| 7 | **HTTP-сервер в `ProvisioningManager`** | ⬜ **ЗАПЛАНИРОВАНО** | Удалить из менеджера, передать управление `ApProvisioningServer` |
| 8 | **Отсутствие `stop()` у AP-сервера** | ⬜ **ЗАПЛАНИРОВАНО** | Реализовать остановку без перезагрузки |
| 9 | **Приведение всех серверов к единому интерфейсу** | ⬜ **ЗАПЛАНИРОВАНО** | Все серверы реализуют `start()`, `stop()`, `update()`, `isActive()` |

---

## 10. Структура серверов

Все серверы (`BleProvisioningServer`, `ApProvisioningServer`, `ZigbeeProvisioningServer`) приводятся к единому интерфейсу и архитектуре.

### 10.1. Единый интерфейс

| Метод | Описание |
|-------|----------|
| `start(manager)` | Запустить сервер, передать указатель на менеджер |
| `stop()` | Остановить сервер, очистить ресурсы |
| `update()` | Периодическая обработка (для AP — `handleClient()`, для Zigbee — обработка стека) |
| `isActive()` | Проверить, активен ли сервер |

---

### 10.2. Детали рефакторинга BLE-сервера

**Было:**
```cpp
// ble_server.cpp — глобальный колбэк
static void SysProvEvent(arduino_event_t* sys_event) {
    auto& prov = ProvisioningManager::getInstance();  // ← глобальный доступ
    prov.onDataReceived(data);
}

bool BleProvisioningServer::begin() {
    WiFi.onEvent(SysProvEvent);
    WiFiProv.beginProvision(...);
    _active = true;
}
```

**Стало:**
```cpp
// ble_server.cpp — статический метод класса
void BleProvisioningServer::eventHandler(arduino_event_t* sys_event) {
    if (_instance && _instance->_manager) {
        _instance->_manager->onDataReceived(data);
    }
}

bool BleProvisioningServer::start(ProvisioningManager* manager) {
    _manager = manager;
    _instance = this;
    WiFi.onEvent(eventHandler);
    WiFiProv.beginProvision(...);
    _active = true;
}

void BleProvisioningServer::stop() {
    WiFi.removeEvent(eventHandler);
    _active = false;
    _manager = nullptr;
}

bool BleProvisioningServer::isActive() const {
    return _active;
}
```

**Что меняется:**
- Убирается глобальный вызов `ProvisioningManager::getInstance()`
- Менеджер передаётся через `start(manager)`
- Колбэк становится статическим методом класса
- Добавляется `isActive()` для единообразия

---

### 10.3. Детали рефакторинга AP-сервера

**Было:** логика AP размазана по `ProvisioningManager`.

**Стало:** выделенный класс `ApProvisioningServer`:

```cpp
class ApProvisioningServer {
public:
    explicit ApProvisioningServer(const char* deviceId);
    ~ApProvisioningServer();

    bool start(ProvisioningManager* manager);
    void stop();
    void update();
    bool isActive() const;

private:
    void startAccessPoint();
    void stopAccessPoint();
    void startHttpServer();
    void stopHttpServer();

    WebServerClass* _server = nullptr;
    char _ssid[32];
    bool _active = false;
    ProvisioningManager* _manager = nullptr;
};
```

---

### 10.4. Детали рефакторинга Zigbee-сервера

**Было:** не реализован.

**Стало:** выделенный класс `ZigbeeProvisioningServer`:

```cpp
class ZigbeeProvisioningServer {
public:
    ZigbeeProvisioningServer();
    ~ZigbeeProvisioningServer();

    bool start(ProvisioningManager* manager);
    void stop();
    void update();
    bool isActive() const;

private:
    bool _active = false;
    ProvisioningManager* _manager = nullptr;
};
```

---

*Конец документа*