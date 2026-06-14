#ifndef WEB_H
#define WEB_H

#include <Arduino.h>
#include "config.h"

#if WEB_ENABLED == 1

#if defined(ESP8266)
#include <ESP8266WebServer.h>
typedef ESP8266WebServer WebServerClass;
#elif defined(ESP32)
#include <WebServer.h>
typedef WebServer WebServerClass;
#endif

#ifndef DEFAULT_WEB_REFRESH
#define DEFAULT_WEB_REFRESH 5
#endif

class FanActuator;
class SwitchActuator;

/**
 * @brief Глобальный экземпляр веб-сервера
 */
extern WebServerClass server;

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
 * @brief Регистратор актуаторов в веб-модуле
 * @param fanPtr Указатель на FanActuator (для TYPE 1)
 * @param switchPtr Указатель на SwitchActuator (для TYPE 3)
 */
void web_registerActuators(FanActuator* fanPtr = nullptr,
                           SwitchActuator* switchPtr = nullptr);

/**
 * @brief Инициализация веб-сервера в режиме клиента WiFi
 */
void web_init();

/**
 * @brief Инициализация веб-сервера в режиме точки доступа (AP)
 * @note Создаёт WiFi сеть для первоначальной настройки
 */
void web_initAP();

/**
 * @brief Периодическая обработка HTTP-запросов
 * @note Вызывается в loop()
 */
void web_update();

#else  // WEB_ENABLED == 0

/**
 * @brief Заглушка: генерация HTML страницы состояния
 * @return Пустая строка
 */
inline String web_buildStatusHtml() {
  return String();
}

/**
 * @brief Заглушка: отправка страницы настроек
 * @param errorMsg Не используется
 * @param successMsg Не используется
 */
inline void web_sendConfigPage(const String&, const String&) {}

#if WEB_STATUS_ENABLED == 1
/**
 * @brief Заглушка: отправка страницы состояния
 * @param refreshInterval Не используется
 */
inline void web_sendStatusPage(int) {}
#endif

/**
 * @brief Заглушка: сохранение конфигурации
 */
inline void web_saveConfig() {}

/**
 * @brief Заглушка: регистрация актуаторов
 * @param fanPtr Не используется
 * @param switchPtr Не используется
 */
inline void web_registerActuators(void*, void*) {}

/**
 * @brief Заглушка: инициализация веб-сервера
 */
inline void web_init() {}

/**
 * @brief Заглушка: инициализация точки доступа
 */
inline void web_initAP() {}

/**
 * @brief Заглушка: обработка HTTP запросов
 */
inline void web_update() {}

#endif  // WEB_ENABLED == 1

#endif  // WEB_H