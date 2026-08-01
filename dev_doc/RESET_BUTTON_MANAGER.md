# RESET_BUTTON_MANAGER.md

## Кнопка сброса

Обработка нажатия кнопки с определением стадий удержания для различных действий.

---

## Оглавление

- [1. Назначение](#1-назначение)
- [2. Принцип работы](#2-принцип-работы)
- [3. API](#3-api)
  - [3.1. resetBtn_init()](#31-resetbtn_init)
  - [3.2. resetBtn_update()](#32-resetbtn_update)
  - [3.3. resetBtn_get_stage()](#33-resetbtn_get_stage)
  - [3.4. resetBtn_is_pressed()](#34-resetbtn_is_pressed-опционально)
- [4. Стадии нажатия](#4-стадии-нажатия)
- [5. Интеграция с оркестратором](#5-интеграция-с-оркестратором)
- [6. Интеграция с SystemState](#6-интеграция-с-systemstate)
- [7. Интеграция с LED](#7-интеграция-с-led)
- [8. Примеры использования](#8-примеры-использования)
- [9. Настройка через build_flags](#9-настройка-через-build_flags)
- [10. Особенности реализации](#10-особенности-реализации)

---

## 1. Назначение

Слой `reset_btn` определяет длительность нажатия кнопки и предоставляет оркестратору информацию о стадии удержания. На основе этой информации оркестратор принимает решения:

| Стадия | Действие |
|--------|----------|
| **STAGE_1S** (1-2 секунды) | LED индикация (MORZE_I) |
| **STAGE_2S** (2-3 секунды) | LED индикация (MORZE_S) |
| **STAGE_3S** (>3 секунды) | Сброс настроек (factory reset) + перезагрузка |

---

## 2. Принцип работы

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          АППАРАТНЫЙ УРОВЕНЬ                                 │
│                                                                             │
│  Кнопка подключена к GPIO (обычно с подтяжкой к GND)                       │
│  LOW = нажата, HIGH = отпущена (или инвертировано)                         │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         resetBtn_update()                                   │
│                                                                             │
│  1. Читает состояние пина (с учётом инверсии)                              │
│  2. Антидребезг (проверка стабильности уровня)                             │
│  3. Подсчёт времени удержания (millis)                                     │
│  4. Определяет стадию (RELEASED/PRESSED/STAGE_1S/STAGE_2S/STAGE_3S)        │
│  5. Устанавливает/снимает STATE_BUTTON_PRESSED в SystemState               │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                           ОРКЕСТРАТОР (main.cpp)                            │
│                                                                             │
│  switch (resetBtn_get_stage()) {                                            │
│      case STAGE_3S:                                                         │
│          g_configManager.reset();                                           │
│          restart_request(500);                                              │
│          break;                                                             │
│  }                                                                          │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## 3. API

### 3.1. resetBtn_init()

```cpp
void resetBtn_init();
```

**Назначение:** инициализация пина кнопки.

**Параметры:** отсутствуют.

**Возвращаемое значение:** отсутствует (`void`).

**Поведение:**
- Настраивает GPIO пина как вход с подтяжкой (`INPUT_PULLUP` или аналогично)
- Сбрасывает внутреннее состояние (таймеры, флаги)

**Пример:**
```cpp
void setup() {
    // ...
    resetBtn_init();
    // ...
}
```

---

### 3.2. resetBtn_update()

```cpp
void resetBtn_update();
```

**Назначение:** обновление состояния кнопки. Вызывается в `loop()`.

**Параметры:** отсутствуют.

**Возвращаемое значение:** отсутствует (`void`).

**Поведение:**
- Читает состояние пина
- Выполняет антидребезг (фильтрация ложных срабатываний)
- Подсчитывает время удержания
- Определяет текущую стадию
- Управляет `STATE_BUTTON_PRESSED` в SystemState

**Пример:**
```cpp
void loop() {
    wdt_feed();
    resetBtn_update();  // ← ОБНОВЛЕНИЕ СОСТОЯНИЯ КНОПКИ
    // ...
}
```

---

### 3.3. resetBtn_get_stage()

```cpp
ResetButtonStage resetBtn_get_stage();
```

**Назначение:** получить текущую стадию нажатия кнопки.

**Параметры:** отсутствуют.

**Возвращаемое значение:**
- `RELEASED` — кнопка отпущена
- `PRESSED` — кнопка нажата (0-1 секунда)
- `STAGE_1S` — нажата 1-2 секунды
- `STAGE_2S` — нажата 2-3 секунды
- `STAGE_3S` — нажата >3 секунды

**Пример:**
```cpp
ResetButtonStage stage = resetBtn_get_stage();
if (stage == STAGE_3S) {
    // Выполнить сброс настроек
}
```

---

### 3.4. resetBtn_is_pressed() (опционально)

```cpp
bool resetBtn_is_pressed();
```

**Назначение:** проверить, нажата ли кнопка (без учёта стадии).

**Параметры:** отсутствуют.

**Возвращаемое значение:**
- `true` — кнопка нажата
- `false` — кнопка отпущена

**Пример:**
```cpp
if (resetBtn_is_pressed()) {
    // Кнопка нажата, но стадия неизвестна
}
```

---

## 4. Стадии нажатия

| Стадия | Интервал | Символ | Описание |
|--------|----------|--------|----------|
| `RELEASED` | — | — | Кнопка отпущена |
| `PRESSED` | 0-1 секунда | `P` | Краткое нажатие |
| `STAGE_1S` | 1-2 секунды | `1` | 1 вспышка LED в секунду |
| `STAGE_2S` | 2-3 секунды | `2` | 2 вспышки LED в секунду |
| `STAGE_3S` | >3 секунды | `3` | Сброс настроек |

---

## 5. Интеграция с оркестратором

### В `loop()`:

```cpp
void loop() {
    wdt_feed();
    
    // 1. Обновить состояние кнопки
    resetBtn_update();
    
    // 2. Проверить стадию
    ResetButtonStage stage = resetBtn_get_stage();
    
    // 3. Проверить, нажата ли кнопка и нет ли уже запланированной перезагрузки
    if (system_state_has_bit(STATE_BUTTON_PRESSED) && !(bits & STATE_RESTART)) {
        if (stage == STAGE_3S) {
            XLOG_WARN(CAT_MAIN, "Reset button triggered. Factory reset.");
            wdt_stop();
            if (g_configManager.reset()) {
                system_state_set_bit(STATE_RESTART);
                restart_request(500);
            }
        }
    }
    
    // ... остальной код
}
```

---

## 6. Интеграция с SystemState

Слой `reset_btn` управляет битом `STATE_BUTTON_PRESSED`:

```cpp
// reset_btn.cpp — внутри resetBtn_update()
if (isPressed) {
    if (!_wasPressed) {
        _pressStartTime = millis();
        system_state_set_bit(STATE_BUTTON_PRESSED);
        _wasPressed = true;
    }
    // ... обновление стадии
} else {
    if (_wasPressed) {
        system_state_clear_bit(STATE_BUTTON_PRESSED);
        _wasPressed = false;
        _currentStage = RELEASED;
    }
}
```

---

## 7. Интеграция с LED

В `main.cpp` LED-индикация зависит от стадии кнопки:

```cpp
if (bits & STATE_BUTTON_PRESSED) {
    if (stage == STAGE_3S || stage == STAGE_2S) {
        led_set_mode(LED_MORZE_S);  // 3 вспышки/сек
    } else if (stage == STAGE_1S) {
        led_set_mode(LED_MORZE_I);  // 2 вспышки/сек
    } else {
        led_set_mode(LED_MORZE_E);  // 1 вспышка/сек
    }
}
```

---

## 8. Примеры использования

### 8.1. Получение стадии с проверкой нажатия

```cpp
if (system_state_has_bit(STATE_BUTTON_PRESSED)) {
    ResetButtonStage stage = resetBtn_get_stage();
    switch (stage) {
        case STAGE_1S:
            XLOG_DEBUG(CAT_RESET_BTN, "Button held 1s");
            break;
        case STAGE_2S:
            XLOG_DEBUG(CAT_RESET_BTN, "Button held 2s");
            break;
        case STAGE_3S:
            XLOG_WARN(CAT_RESET_BTN, "Factory reset requested!");
            break;
        default:
            break;
    }
}
```

### 8.2. Реакция на отпускание кнопки

```cpp
static bool wasPressed = false;

void loop() {
    resetBtn_update();
    
    bool isPressed = system_state_has_bit(STATE_BUTTON_PRESSED);
    if (wasPressed && !isPressed) {
        // Кнопка была нажата и отпущена
        ResetButtonStage lastStage = resetBtn_get_stage();
        if (lastStage < STAGE_3S) {
            // Короткое нажатие — игнорируем или выполняем другое действие
        }
    }
    wasPressed = isPressed;
}
```

---

## 9. Настройка через build_flags

В `platformio.ini` можно переопределить настройки:

```ini
build_flags =
    -D RESET_PIN=0                   # GPIO для кнопки (по умолчанию 0)
    -D FEATURE_RESET_BUTTON_ENABLED=1 # 1 = включена, 0 = выключена
    -D DEBOUNCE_MS=50                # Антидребезг (мс)
```

Если `FEATURE_RESET_BUTTON_ENABLED=0`, все функции становятся заглушками:

```cpp
// Заглушка — кнопка сброса отключена
inline void resetBtn_init() {}
inline void resetBtn_update() {}
inline ResetButtonStage resetBtn_get_stage() { return RELEASED; }
```

---

## 10. Особенности реализации

### 10.1. Антидребезг

```cpp
static unsigned long _lastDebounceTime = 0;
static bool _lastStableState = false;

void resetBtn_update() {
    bool currentReading = digitalRead(RESET_PIN);
    
    if (currentReading != _lastStableState) {
        _lastDebounceTime = millis();
    }
    
    if (millis() - _lastDebounceTime > DEBOUNCE_MS) {
        // Уровень стабилен, обновляем состояние
        _isPressed = currentReading == PRESSED_LEVEL;
    }
}
```

### 10.2. Инверсия логики

Кнопка может быть подключена двумя способами:
- **LOW = нажата** (с подтяжкой к VCC) — `PRESSED_LEVEL = LOW`
- **HIGH = нажата** (с подтяжкой к GND) — `PRESSED_LEVEL = HIGH`

В `settings.h` или `reset_btn.h`:

```cpp
#ifndef RESET_BTN_INVERTED
#define RESET_BTN_INVERTED 1  // 1 = LOW активен, 0 = HIGH активен
#endif

#if RESET_BTN_INVERTED == 1
#define PRESSED_LEVEL LOW
#else
#define PRESSED_LEVEL HIGH
#endif
```

### 10.3. Внутреннее состояние (reset_btn.cpp)

```cpp
static bool _isPressed = false;
static bool _wasPressed = false;
static unsigned long _pressStartTime = 0;
static ResetButtonStage _currentStage = RELEASED;
```

---

## 11. Тип ResetButtonStage

```cpp
enum ResetButtonStage : uint8_t {
    RELEASED = 0,  // Кнопка отпущена
    PRESSED = 1,   // Нажата 0-1с
    STAGE_1S = 2,  // Нажата 1-2с
    STAGE_2S = 3,  // Нажата 2-3с
    STAGE_3S = 4   // Нажата >3с
};
```

-