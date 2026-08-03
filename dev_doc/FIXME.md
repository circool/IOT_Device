```cpp
/**
 * @file FIXME.md
 * @brief Архитектурный долг - чеклист
 * @note Статус: В процессе
 */
```
# FIXME.md 

## О структуре файла

**Раздел 1. Архитектурный рефакторинг** — содержит задачи, вытекающие из архитектурных решений, принятых в ARCH_REFACTORING_ROADMAP.md. Они требуют изменения структуры кода, но уже осмыслены и разбиты на конкретные шаги.

**Раздел 2. Исправление найденных ошибок** — содержит изолированные проблемы, не связанные с архитектурой: баги, несовместимости, недочёты интерфейса, которые можно исправить точечно, без перестройки слоёв.

**Раздел 3. Косметические улучшения** — задачи, не влияющие на функциональность: стили, предупреждения компиляции, улучшение читаемости кода.

## 1. Архитектурный рефакторинг

| # | Задача | Подзадачи | Статус | Приоритет |
|---|--------|-----------|--------|-----------|
| **1.1** | **Рефакторинг слоя провайдера данных** | ... | ⬜ | 🔴 Критично |
| **1.2** | **Перенести AP-провизионинг в `provisioning`** | ... | ✅ | 🔴 Критично |
| **1.3** | **Вынести управление железом в `DeviceController`** | ... | ⬜ | 🔴 Критично |
| **1.4** | **Абстракция актуаторов** | ... | ⬜ | 🟡 Средний |
| **1.5** | **Транспортная абстракция** | ... | ⬜ | 🔴 Критично |
| **1.6** | **Инкапсуляция ProvisioningManager** | ... | В процессе | 🟡 Средний |
| **1.7** | **Остановка провизионинга без перезагрузки** | ... | В процессе | 🟡 Средний |
| **1.7.1** | **Завязано на wifi_manager** Обеспечить возможность переподключения с новыми данными  | ... | В процессе | 🔴 Критично |
| **1.8** | **Привести имена слоёв к единому стандарту** | ... | ⬜ | 🟡 Средний |
| **1.9** | **Создать недостающую документацию для слоёв** | ... | ✅ | 🟡 Средний |
| **1.10** | **Рефакторинг HTML-шаблонов** | ... | ✅ | 🟡 Средний |
| **1.11** | **Привести ProvisioningManager к единому интерфейсу слоёв** | ... | ⬜ | 🟡 Средний |
| **1.12** | **Привести имена функций Web-слоя к `snake_case`** | ... | ✅ | 🟡 Средний |
| **1.13** | **Рефакторинг IWebStatusProvider (подготовка к StateProvider)** | | ⬜ | 🟡 Средний |
| 1.13.1 | `web_status_provider.h` | Добавить `DeviceController*` в конструкторы провайдеров | ⬜ | |
| 1.13.2 | `web_status_provider.cpp` | Переделать `isDeviceOn()` — читать из `_controller->get_state()->is_on` | ⬜ | |
| 1.13.3 | `web_status_provider.cpp` | Переделать `getSpeedPercent()` — читать из `_controller->get_state()->speed` | ⬜ | |
| 1.13.4 | `web_status_provider.cpp` | Переделать `isSensorControlMode()` — читать из `!_controller->get_state()->manual_mode` | ⬜ | |
| 1.13.5 | `web_status_provider.cpp` | Убрать прямые вызовы `g_configManager` из провайдера | ⬜ | |
| 1.13.6 | `main.cpp` | Обновить создание провайдера — передавать `&deviceController` | ⬜ | |
| 1.13.7 | `web_manager.cpp` | Обновить `web_build_status_html()` — использовать `isSensorControlMode()` | ⬜ | |
| 1.13.8 | `web_manager.cpp` | Обновить `getCurrentModeText()` — использовать `isSensorControlMode()` | ⬜ | |
| | | | | |
| **1.14** | **Внедрение StateProvider (единый источник правды)** | | В процессе | 🟡 Средний |
| 1.14.1 | `state_provider.h/cpp` | Создать класс StateProvider (синглтон) с `DeviceState` структурой | ✅ | |
| 1.14.2 | `state_provider.h/cpp` | Реализовать методы обновления состояния от всех слоёв | ✅ | |
| 1.14.3 | `device_controller.h/cpp` | Убрать `operational_state_t _state` из DeviceController | ⬜ | |
| 1.14.4 | `device_controller.h/cpp` | DeviceController получает `StateProvider*` и работает с ним | ⬜ | |
| 1.14.5 | `system_state.h/cpp` | Интегрировать битовую маску в StateProvider | ⬜ | |
| 1.14.6 | `web_status_provider.h/cpp` | Удалить IWebStatusProvider (заменён на StateProvider) | ⬜ | |
| 1.14.7 | `web_manager.cpp` | Переделать на чтение из StateProvider вместо IWebStatusProvider | ⬜ | |
| 1.14.8 | `main.cpp` | Передать StateProvider во все слои | ⬜ | |
| 1.14.9 | `wifi_manager.cpp` | Обновлять StateProvider при изменении WiFi | ✅ | |
| 1.14.9.1 | `wifi_manager.cpp` | Заменить `#include "system_state.h"` на `#include "state_provider.h"` | ✅ | |
| 1.14.9.2 | `wifi_manager.cpp` | В `wifi_manager_update()` заменить `system_state_set_bit(STATE_WIFI_OK)` на `StateProvider::getInstance().update_connection(true, false, WiFi.RSSI())` | ✅ | |
| 1.14.9.3 | `wifi_manager.cpp` | В `wifi_manager_update()` заменить `system_state_clear_bit(STATE_WIFI_OK)` на `StateProvider::getInstance().update_connection(false, false,0)` | ✅ | |
| 1.14.9.4 | `wifi_manager.cpp` | Заменить проверку `system_state_has_bit(STATE_WIFI_OK)` (на StateProvider::getInstance().get_state()->wifi_connected ) | ✅ | |
| 1.14.9.4 | `main.cpp` | Заменить обращения к `STATE_WIFI_OK` на `state->wifi_connected` | ✅ | |
| 1.14.10 | `mqtt_manager.cpp` | Обновлять StateProvider при изменении MQTT | ✅ | |
| 1.14.10.1 | `mqtt_manager.cpp` | Заменить `#include "system_state.h"` на `#include "state_provider.h"` | ✅ | |
| 1.14.10.2 | `mqtt_manager.cpp` | В `reconnect()` заменить `system_state_set_bit(STATE_MQTT_OK)` на `StateProvider::getInstance().update_connection(state->wifi_connected, true, state->wifi_rssi)` при успешном подключении | ✅ | |
| 1.14.10.3 | `mqtt_manager.cpp` | В `disconnect()` или при потере соединения заменить `system_state_clear_bit(STATE_MQTT_OK)` на `StateProvider::getInstance().update_connection(state->wifi_connected, false, state->wifi_rssi)` | ✅ | |
| 1.14.11 | `provisioning_manager.cpp` | Обновлять StateProvider при изменении режима | ✅ | |
| 1.14.12 | `reset_button_manager.cpp` | Обновлять StateProvider при нажатии кнопки | ✅ | |
| 1.14.13 | `main.cpp` | Читать StateProvider для определения режима | ✅ | |
| 1.14.14 | `sensor.cpp` | Обновлять StateProvider после чтения датчика | ⬜ | |
| 1.14.15 | `restart_manager.cpp` | Обновлять StateProvider при запросе перезагрузки | ✅ | |
| 1.14.16 | `transport_*.cpp` | Читать StateProvider для публикации состояния | ⬜ | |
| 1.14.17 | `system_state.h/cpp` | Удалить (интегрирован в StateProvider) | ⬜ | |
| 1.14.18 | `web_status_provider.h/cpp` | Удалить (заменён на StateProvider) | ⬜ | |
| 1.14.19 | | Обновить архитектурную документацию | ⬜ | |
| **1.15** | **Актуализация документации под StateProvider** | | ⬜ | 🟡 Средний |
| 1.15.1 | `ARCHITECTURE.md` | Обновить архитектурную документацию | ⬜ | |
| 1.15.2 | `WEB_MANAGER.md` | Обновить описание Web-слоя | ⬜ | |
| 1.15.3 | `DEVICE_CONTROLLER.md` | Обновить описание контроллера | ⬜ | |
| 1.15.4 | `ORCHESTRATOR.md` | Обновить описание оркестратора | ⬜ | |
| 1.15.5 | `TRANSPORT_ABSTRACTION.md` | Обновить описание транспорта | ⬜ | |
| 1.15.6 | `PROVISIONING_MANAGER.md` | Обновить описание провизионинга | ⬜ | |
| 1.15.7 | `LED_MANAGER.md` | Обновить описание LED | ⬜ | |
| 1.15.8 | `WIFI_MANAGER.md` | Обновить описание WiFi | ⬜ | |
| 1.15.9 | `RESTART_MANAGER.md` | Обновить описание перезагрузок | ⬜ | |
| 1.15.10| `RESET_BUTTON_MANAGER.md` | Обновить описание кнопки | ⬜ | |
| 1.15.11| `WDT_MANAGER.md` | Обновить описание WDT | ⬜ | |
| 1.15.12| `LOGGER.md` | Обновить описание логирования | ⬜ | |
| 1.15.13| `PROJECT.md` | Обновить описание проекта | ⬜ | |
| 1.15.14| `ACTUATOR_SENSOR.md` | Обновить описание актуатора/датчика | ⬜ | |
| 1.15.15| `TRANSPORT_MQTT.md` | Обновить описание MQTT | ⬜ | |
| 1.15.16| `TRANSPORT_ZIGBEE.md` | Обновить описание Zigbee | ⬜ | |
| 1.15.17| `WEB_OTA_MANAGER.md` | Обновить описание OTA | ⬜ | |

## 2. Исправление найденных ошибок

| # | Задача | Подзадачи | Статус | Приоритет |
|---|--------|-----------|--------|-----------|
| **2.1** | **LEDC API несовместим с ESP32-C6** || ✅ | 🔴 Критично |
| **2.2** | **Сохранение конфигурации не происходит** |(зависит от 1.14)| ⬜ | 🔴 Критично |
| **2.3** | **Интерфейс web-страницы** | | ⬜ | 🔴 Критично |
| 2.3.1 | | Исправить отображение состояния (зависит от 1.13) | ⬜ | |
| 2.3.2 | | Исправить выравнивание иконок | ✅ | |
| 2.3.3 | | Добавить отказ от сохранения при снятой галке Confirm saving | ✅ | |
| **2.4** | **Дублирование Zigbee** || ✅ | 🟡 Средний |
| **2.5** | **Web не должен вызывать `sensor_*()` напрямую** |(зависит от 1.14) | ⬜ | 🟡 Средний |
| **2.6** | **Инкапсуляция слоев** || Частично | 🟡 Средний |
| **2.7** | **Транспорт (MQTT)** |Данные не публикуются (только WDT)| ⬜ | 🔴 Критично |
| **2.8** | **Актуализация ARCHITECTURE.md** |(зависит от 1.14) | ⬜ | 🟡 Средний |
| **2.9** | **Транспорт создаётся в режиме провизионинга** || ⬜ | 🟡 Средний |


## 3. Косметические улучшения

| # | Задача | Подзадачи | Статус | Приоритет |
|---|--------|-----------|--------|-----------|
| **3.1** | **HTML_STYLE** | Разделить на `HTML_STYLE_BASE` и `HTML_STYLE_EXTENDED` | ⬜ | 🟢 Низкий |
