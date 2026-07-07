#ifndef WEB_H
#define WEB_H

#include <Arduino.h>
#include "config_manager.h"
#include "web_status_provider.h"

// ============================================================================
// НАСТРОЙКИ WEB ИНТЕРФЕЙСА
// ============================================================================

#if WEB_ENABLED == 1
/** @brief Показывать страницу состояния (иначе сразу /config) */
#ifndef WEB_STATUS_ENABLED
#define WEB_STATUS_ENABLED 1
#endif

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

#if WEB_ENABLED == 1

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
 * @brief Отправить страницу настроек (HTTP)
 * @param errorMsg Сообщение об ошибке (если есть)
 * @param successMsg Сообщение об успехе (если есть)
 */
void web_sendConfigPage(const String& errorMsg = "",
                        const String& successMsg = "");

#if WEB_STATUS_ENABLED == 1
/**
 * @brief Отправить страницу состояния (HTTP)
 * @param refreshInterval Интервал автообновления страницы (сек)
 */
void web_sendStatusPage(int refreshInterval);
#endif

/**
 * @brief Обработчик POST-запроса на сохранение конфигурации
 */
void web_saveConfig();

/**
 * @brief Инициализация веб-сервера
 * @param setupMode true - режим настройки (только страница конфигурации),
 *                  false - нормальный режим (полный функционал)
 */
void web_init(bool setupMode = false);

/**
 * @brief Инициализация веб-сервера в режиме точки доступа (AP)
 * @note Создаёт WiFi сеть для первоначальной настройки
 */
// void web_initAP();

/**
 * @brief Периодическая обработка HTTP-запросов
 * @note Вызывается в loop()
 */
void web_update();

// ========== ОБРАБОТЧИКИ ДЕЙСТВИЙ ==========
// Они больше не нужны для Web, но оставлены для совместимости

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
void handleToggle();
#endif

#if DEVICE_TYPE == 1
void handleSensorControlMode();
#endif

#else  // WEB_ENABLED == 0

// Заглушки
inline void web_registerStatusProvider(IWebStatusProvider* provider) {
  (void)provider;
}

// inline void web_initAP() {}
inline String web_buildStatusHtml() {
  return String();
}
inline void web_sendConfigPage(const String&, const String&) {}

#if WEB_STATUS_ENABLED == 1
inline void web_sendStatusPage(int) {}
#endif

inline void web_saveConfig() {}
inline void web_init() {}
inline void web_update() {}

#endif  // WEB_ENABLED == 1

#endif  // WEB_H