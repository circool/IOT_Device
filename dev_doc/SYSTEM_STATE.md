# SYSTEM_STATE.md

## Состояние системы (битовая маска)

## Оглавление
- [1. Назначение](#1-назначение)
- [2. Битовая маска](#2-битовая-маска)
- [3. API](#3-api)
- [4. Кто устанавливает и снимает биты](#4-кто-устанавливает-и-снимает-биты)

## 1. Назначение

`SystemState` — это глобальная битовая маска, отражающая **состояние устройства как целого**. Она используется всеми слоями для:

- Проверки статуса подключений (WiFi, MQTT)
- Определения режима работы (настройка / нормальный)
- Индикации аварийных состояний (emergency)
- Координации действий между слоями

**Важно:** `SystemState` не является частью `Config` или `OperationalState`. Это **отдельный слой**, доступный для чтения и записи из любого места кода.

---

## 2. Битовая маска

```cpp
enum SystemStateBit : uint16_t {
    STATE_NONE           = 0,
    STATE_BUTTON_PRESSED = 1 << 0,  // 0x0001 — кнопка нажата
    STATE_WIFI_OK        = 1 << 1,  // 0x0002 — WiFi подключён
    STATE_MQTT_OK        = 1 << 2,  // 0x0004 — MQTT подключён
    STATE_PROVISIONING   = 1 << 3,  // 0x0008 — режим настройки
    STATE_EMERGENCY      = 1 << 4,  // 0x0010 — аварийное отключение
    STATE_RESTART        = 1 << 5,  // 0x0020 — ожидание перезагрузки
};
```

## 3. API

```cpp
void system_state_init();                     // Инициализация (все биты сброшены)
void system_state_set_bit(uint16_t bit);      // Установить флаг
void system_state_clear_bit(uint16_t bit);    // Снять флаг
bool system_state_has_bit(uint16_t bit);      // Проверить флаг
uint16_t system_state_get_bits();             // Получить все флаги
```

## 4. Кто устанавливает и снимает биты

Биты состояния доступны для изменения только прямо связанным слоям

**TODO:** Продумать и окончательно утвердить права установки/сброса битов для слоев

| Бит | Кто устанавливает | Кто снимает | Когда |
|-----|-------------------|-------------|-------|
| `STATE_WIFI_OK` | `WiFiManager` | `WiFiManager` | При подключении / потере WiFi |
| `STATE_MQTT_OK` | `MQTTManager` | `MQTTManager` | При подключении / потере MQTT |
| `STATE_PROVISIONING` | `ProvisioningManager` | `ProvisioningManager` | При входе / выходе из режима настройки |
| `STATE_EMERGENCY` | `Actuator` | `DeviceController` (при ручном включении) | При превышении `maxOnTime` |
| `STATE_RESTART` | `RestartManager` | `RestartManager` (при перезагрузке) | При запросе перезагрузки |
| `STATE_BUTTON_PRESSED` | `ResetButton` | `ResetButton` (при отпускании) | При нажатии / отпускании кнопки |

## 5. Кто читает биты

Информация о состоянии битов доступна только оркестратору и связанному слою. 
В исключительных случаях допускается предоставлять информицию посторонним слоям когда подобное целесообразно с практической точки   

| Слой | Читает биты | Для чего |
|------|-------------|----------|
| **Оркестратор (`main.cpp`)** | Все | Определяет режим LED, управляет перезагрузкой |
| **WiFiManager** | `STATE_PROVISIONING` | Не запускает STA, если идёт настройка |
| **MQTTManager** | `STATE_WIFI_OK` | Не пытается подключиться без WiFi |
| **Actuator** | `STATE_EMERGENCY` | Блокирует включение при аварии |
| **DeviceController** | `STATE_EMERGENCY` | Учитывает при принятии решений |
| **Web** | `STATE_PROVISIONING` | Показывает страницу настройки вместо состояния |
| **LED** (через оркестратор) | Все | Определяет режим индикации |

---

## 6. Приоритеты битов

Биты **не являются взаимоисключающими** — могут быть установлены одновременно. Однако есть **логические приоритеты** для принятия решений:

| Приоритет | Бит | Что это значит |
|-----------|-----|----------------|
| **1 (высший)** | `STATE_RESTART` | Устройство перезагружается — все остальные биты игнорируются |
| **2** | `STATE_EMERGENCY` | Авария — блокирует работу актуатора |
| **3** | `STATE_PROVISIONING` | Режим настройки — транспорт не работает |
| **4** | `STATE_WIFI_OK` | WiFi есть — можно подключать MQTT |
| **5** | `STATE_MQTT_OK` | MQTT есть — полная связь |
| **6 (низший)** | `STATE_BUTTON_PRESSED` | Индикация нажатия кнопки (не влияет на логику) |

---

## 7. Примеры использования

### 7.1. WiFiManager — установка STATE_WIFI_OK

```cpp
void wifi_manager_loop() {
    if (WiFi.status() == WL_CONNECTED) {
        if (!system_state_has_bit(STATE_WIFI_OK)) {
            system_state_set_bit(STATE_WIFI_OK);
            XLOG_INFO(CAT_WIFI, "WiFi connected");
        }
    } else {
        if (system_state_has_bit(STATE_WIFI_OK)) {
            system_state_clear_bit(STATE_WIFI_OK);
            XLOG_WARN(CAT_WIFI, "WiFi lost");
        }
    }
}
```

### 7.2. Оркестратор — определение режима LED

```cpp
void update_led_mode() {
    if (system_state_has_bit(STATE_RESTART)) {
        led_set_mode(LED_OFF);
    } else if (system_state_has_bit(STATE_EMERGENCY)) {
        led_set_mode(LED_SLOW_BLINK);
    } else if (system_state_has_bit(STATE_PROVISIONING)) {
        led_set_mode(LED_MORZE_S);
    } else if (!system_state_has_bit(STATE_WIFI_OK)) {
        led_set_mode(LED_MORZE_E);
    } else if (!system_state_has_bit(STATE_MQTT_OK)) {
        led_set_mode(LED_MORZE_I);
    } else {
        led_set_mode(LED_ON);
    }
}
```

### 7.3. Оркестратор - проверка WiFi перед подключением MQTT

```cpp
void loop() {
  if (if system_state_has_bit(STATE_WIFI_OK) && !isConnected()) {     
    reconnect();
  }
}
```


## 8. Особенности реализации

1. **Битовая маска** — экономит память (всего 2 байта)
2. **Глобальный доступ** — из любого места кода (через `#include "system_state.h"`)
3. **Атомарность не требуется** — операции над битами выполняются за один такт на ESP
4. **Нет блокировок** — все вызовы из loop() или прерываний (кроме ISR)



## 9. Связь с другими слоями

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           SystemState (битовая маска)                       │
│                                                                             │
│  • STATE_WIFI_OK      ← WiFiManager                                         │
│  • STATE_MQTT_OK      ← MQTTManager                                         │
│  • STATE_PROVISIONING ← ProvisioningManager                                 │
│  • STATE_EMERGENCY    ← Actuator                                            │
│  • STATE_RESTART      ← RestartManager                                      │
│  • STATE_BUTTON_PRESSED ← ResetButton                                       │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
          ┌─────────────────────────┼─────────────────────────┐
          │                         │                         │
          ▼                         ▼                         ▼
┌───────────────────┐    ┌────────────────────┐    ┌─────────────────────────┐
│   Оркестратор     │    │   DeviceController │    │   LED                   │
│   (main.cpp)      │    │   (бизнес-логика)  │    │   (индикация)           │
│                   │    │                    │    │                         │
│ • Определяет LED  │    │ • Читает           │    │ • Читает через          │
│ • Управляет       │    │   STATE_EMERGENCY  │    │   оркестратор           │
│   перезагрузкой   │    │ • Блокирует        │    │                         │
│ • Проверяет       │    │   включение        │    │                         │
│   STATE_PROVISION │    │                    │    │                         │
└───────────────────┘    └────────────────────┘    └─────────────────────────┘
```



## 10. Итоговая таблица

| Бит | Значение | Устанавливает | Снимает | Читают |
|-----|----------|---------------|---------|--------|
| `STATE_BUTTON_PRESSED` | 0x0001 | ResetButton | ResetButton | Оркестратор |
| `STATE_WIFI_OK` | 0x0002 | WiFiManager | WiFiManager | Оркестратор, Web |
| `STATE_MQTT_OK` | 0x0004 | MQTTManager | MQTTManager | Оркестратор, Web |
| `STATE_PROVISIONING` | 0x0008 | ProvisioningManager | ProvisioningManager | WiFiManager, Оркестратор, Web |
| `STATE_EMERGENCY` | 0x0010 | Actuator | DeviceController | DeviceController, Оркестратор, LED |
| `STATE_RESTART` | 0x0020 | RestartManager | RestartManager | Оркестратор, LED |

---