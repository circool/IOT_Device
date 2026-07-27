#ifndef WEB_H
#define WEB_H

#include <Arduino.h>
#include "config_manager.h"
#include "web_status_provider.h"

// ============================================================================
// НАСТРОЙКИ WEB ИНТЕРФЕЙСА
// ============================================================================

#if TRANSPORT_TYPE == 1
/** 
 * @brief Показывать страницу состояния (иначе сразу /config) 
 * @deprecated Неочевидная зависимость
 * @FIXME Убедиться что реализовано в коде  
 */
// #ifndef FEATURE_WEB_STATUS_ENABLED
// #define FEATURE_WEB_STATUS_ENABLED 1
// #endif

/** @brief Показывать RSSI на странице состояния */
#ifndef WEB_SHOW_RSSI
#define WEB_SHOW_RSSI 1
#endif

#ifndef DEFAULT_WEB_REFRESH
#define DEFAULT_WEB_REFRESH 5
#endif

/** @brief Включить сброс настроек через веб */
#ifndef WEB_RESET_ENABLED
#define WEB_RESET_ENABLED 0
#endif

#endif

#if defined(ESP8266)
#include <ESP8266WebServer.h>
typedef ESP8266WebServer WebServerClass;
#elif defined(ESP32)
#include <WebServer.h>
typedef WebServer WebServerClass;
#endif

#if TRANSPORT_TYPE == 1

/**
 * @brief Глобальный экземпляр веб-сервера
 */
extern WebServerClass server;

/**
 * @brief Зарегистрировать провайдер статуса для Web
 * @param provider Указатель на реализацию IWebStatusProvider
 * @note Web слой использует этот интерфейс вместо прямых вызовов
 */
void web_registerStatusProvider(IWebStatusProvider* provider);

/**
 * @brief Сгенерировать HTML-код страницы состояния
 * @return Строка с HTML
 */
String web_buildStatusHtml();

/**
 * @brief Сгенерировать HTML-код страницы сообщения о результате выполнения
 * действия
 * @param action Текст действия (например: "Save config", "WiFi save")
 * @param success true - успешное выполнение, false - ошибка
 * @return Строка с HTML-кодом страницы
 * @details Генерирует простую HTML-страницу с сообщением о результате.
 *          При успехе выполняет автоматический редирект на главную через 2
 * секунды.
 * @note Стиль страницы полностью соответствует web_saveConfig()
 * @see web_saveConfig() - пример использования аналогичного шаблона
 */
String web_buildResultHtml(const String& action, bool success);

    

/**
 * @brief Отправить фрагмент HTML-контента через веб-сервер
 * @param chunk Строка с HTML-контентом для отправки
 * @param context Указатель на экземпляр WebServerClass (используется как void*)
 * @details Вспомогательная функция-обёртка для отправки контента через
 *          server.sendContent(). Разработана специально для использования
 *          в качестве колбэка WebSendCallback на платформе ESP8266.
 *
 *          Особенности:
 *          - Принимает контекст как void* для совместимости с сигнатурой
 *          - Приводит context к WebServerClass* и вызывает sendContent()
 *          - Используется в sendConfigPage() для потоковой передачи HTML
 *
 * @note В отличие от ESP32, где используется буферизированная отправка
 *       через configSend(), на ESP8266 контент отправляется напрямую.
 * @see WebSendCallback - тип колбэка, которому соответствует сигнатура
 * @see sendConfigPage() - функция, использующая этот колбэк
 * @see web_sendStatusPage() - использует эту функцию для отправки статуса
 */
static void webSendContent(const String& chunk, void* context);

#if FEATURE_WEB_STATUS_ENABLED == 1
    /**
     * @brief Отправить страницу состояния (HTTP)
     * @param refreshInterval Интервал автообновления страницы (сек)
     */
    void web_sendStatusPage(int refreshInterval);
#endif



/**
 * @brief Инициализация веб-сервера
 * @param setupMode true - режим настройки (только страница конфигурации),
 *                  false - нормальный режим (полный функционал)
 */
void web_init(bool setupMode = false);


/**
 * @brief Периодическая обработка HTTP-запросов
 * @note Вызывается в loop()
 */
void web_loop();

// ========== ОБРАБОТЧИКИ ДЕЙСТВИЙ ==========
// Они больше не нужны для Web, но оставлены для совместимости

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3

/**
 * @brief Обработчик нажатия кнопки переключения состояния устройства
 * @details Вызывается при переходе по маршруту /switch/toggle на веб-странице.
 *          В текущей реализации является заглушкой — только логирует факт
 * нажатия через XLOG_INFO без выполнения фактического переключения.
 *
 *          Предназначена для:
 *          - TYPE 1 (вентилятор с датчиком) — ручное переключение вентилятора
 *          - TYPE 3 (управляемый выключатель) — ручное переключение выключателя
 *
 * @note После обработки выполняется редирект на главную страницу (/)
 *       в соответствии с настройкой маршрута в web_init().
 * @see web_init() - регистрация маршрута /switch/toggle
 * @deprecated Функция является заглушкой для обратной совместимости.
 *             В будущем будет заменена на полноценную реализацию
 *             с управлением через DeviceController или удалена.
 */
void handleToggle();
#endif

#if DEVICE_TYPE == 1
/**
 * @brief Обработчик включения режима управления по датчику
 * @details Активирует режим управления вентилятором на основе показаний
 *          датчика температуры и влажности. Вызывается при переходе
 *          по ссылке /fan/auto на веб-странице состояния.
 *
 *          При активации:
 *          - Устанавливает sensorControlMode = true в ConfigManager
 *          - Логирует событие через XLOG_INFO
 *          - После обработки выполняется редирект на главную страницу (/)
 *
 * @note Функция является обработчиком HTTP-маршрута /fan/auto.
 *       Режим управления по датчику позволяет автоматически включать
 *       вентилятор при превышении порогов температуры или влажности.
 * @see web_init() - регистрация маршрута
 * @see g_configManager.setSensorControlMode()
 * @deprecated Является заглушкой для обратной совместимости.
 *             В будущем будет заменена на универсальный обработчик команд или удалена.
 */
void handleSensorControlMode();
#endif

// ============================================================================
// ГЛОБАЛЬНЫЕ ФЛАГИ ДЛЯ КОММУНИКАЦИИ С MAIN
// ============================================================================
/**
 * @brief Флаг: есть новые настройки от Web
 * @note Устанавливается в web.cpp, обрабатывается в main.cpp
 */
extern volatile bool g_webConfigPending;

/**
 * @brief Флаг: команда перезагрузки от Web
 * @note Устанавливается в web.cpp, обрабатывается в main.cpp
 */
extern volatile bool g_webRestartPending;

/**
 * @brief Временный буфер с новыми настройками
 * @note Заполняется в web.cpp, применяется в main.cpp
 */
extern ConfigData g_webPendingConfig;

/**
 * @brief Обработчик страницы провизионинга через точку доступа
 * @details Принимает POST-запрос с WiFi SSID и паролем,
 *          сохраняет их и инициирует перезагрузку.
 *          Используется при PROVISIONING_METHOD == 2 (AP mode).
 * @deprecated Перенести в слой provisioning
 */
void web_handleApProvisioning();

/**
 * @brief Сбросить буфер конфигурации и отправить накопленный контент
 * @details Используется только для ESP32 для оптимизации отправки больших
 * HTML-страниц. Отправляет накопленный в g_configBuffer контент через
 * server.sendContent() и очищает буфер.
 * @note Вызывается автоматически при заполнении буфера или в конце отправки
 * @see configSend()
 */
static void configFlush();

/**
 * @brief Добавить фрагмент HTML в буфер конфигурации (ESP32)
 * @param chunk Фрагмент HTML-контента для отправки
 * @details Буферизирует данные и автоматически сбрасывает буфер при достижении
 *          порогового размера (1024 байта) для эффективной отправки через HTTP.
 *          Используется только на ESP32.
 * @note Это внутренняя функция, вызываемая через configSendWrapper из
 * sendConfigPage
 * @see configFlush()
 * @see configSendWrapper()
 */
static void configSend(const String& chunk);

    /**
     * @brief Обёртка для configSend, совместимая с сигнатурой WebSendCallback
     * @param chunk Фрагмент HTML-контента
     * @param context Неиспользуемый контекст (требуется для совместимости с
     * колбэком)
     * @details Используется в sendConfigPage как колбэк для отправки контента
     * на ESP32. Преобразует вызов в configSend(chunk).
     * @note Сигнатура соответствует WebSendCallback: void (*)(const String&,
     * void*)
     * @see WebSendCallback
     * @see configSend()
     */
    static void configSendWrapper(const String& chunk, void* context);

    /**
     * @brief Отправить HTML-страницу конфигурации клиенту
     * @param errorMsg Сообщение об ошибке для отображения (если не пусто)
     * @param successMsg Сообщение об успехе для отображения (если не пусто)
     * @details Генерирует полную HTML-страницу настроек устройства с
     * использованием sendConfigPage() из web_templates.h. Содержит:
     *          - Информацию о текущем режиме (AP/STA), SSID и IP-адресе
     *          - Форму с полями для всех параметров конфигурации
     *          - Сообщения об ошибках или успехе
     *          - Для TYPE 1: пороги температуры/влажности, таймеры, режимы
     * работы
     *          - Для TYPE 3: таймеры и параметры включения
     *
     *          Использует потоковую передачу (chunked transfer) для экономии
     * RAM. На ESP32 используется буферизированная отправка через
     * g_configBuffer.
     * @note Устанавливает Content-Length как UNKNOWN для поддержки chunked
     * encoding
     * @see sendConfigPage()
     */
    void web_sendConfigPage(const String& errorMsg, const String& successMsg);

    /**
     * @brief Обработать POST-запрос сохранения конфигурации
     * @details Основной обработчик веб-формы настроек. Выполняет:
     *          1. Копирует текущую конфигурацию в g_webPendingConfig как базу
     *          2. Парсит все параметры из HTTP-запроса (WiFi, MQTT, сенсор,
     * таймеры)
     *          3. Валидирует каждый параметр (диапазоны, длины строк)
     *          4. При ошибке валидации показывает страницу с сообщением об
     * ошибке
     *          5. При успехе устанавливает g_webConfigPending = true
     *          6. Отправляет страницу подтверждения с автоматическим редиректом
     *
     *          Параметры формы:
     *          - wifiSsid, wifiPassword
     *          - mqttBroker, mqttPort, mqttUser, mqttPassword, mqttClientId
     *          - sensorInterval (TYPE 1,2)
     *          - lowTemp, highTemp, lowHum, highHum (TYPE 1)
     *          - maxOnTime, delaySeconds (TYPE 1,3)
     *          - speedPercent, adaptiveMode, bootState, sensorControlMode (TYPE
     * 1)
     *
     * @note Сохранение в энергонезависимую память выполняется в main.cpp
     *       при обработке флага g_webConfigPending
     * @see g_webPendingConfig
     * @see g_webConfigPending
     */
    void web_saveConfig();

    

#else  // TRANSPORT_TYPE == 0

// Заглушки
inline void web_registerStatusProvider(IWebStatusProvider* provider) {
  (void)provider;
}

// inline void web_initAP() {}
inline String web_buildStatusHtml() {
  return String();
}
inline void web_sendConfigPage(const String&, const String&) {}
inline void web_sendStatusPage(int) {}
inline void web_saveConfig() {}
inline void web_init(bool setupMode) {}
inline void web_loop() {}

#endif  // TRANSPORT_TYPE == 1

#endif  // WEB_H