#ifndef WEB_H
#define WEB_H

#include <Arduino.h>
#include "config.h"

#if WEB_ENABLED == 1

// Абстракция для поддержки ESP8266WebServer и WebServer
#if defined(ESP8266)
  #include <ESP8266WebServer.h>
  typedef ESP8266WebServer WebServerClass;
#elif defined(ESP32)
  #include <WebServer.h>
  typedef WebServer WebServerClass;
#endif

extern WebServerClass server;

class FanActuator;
class SwitchActuator;

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
void web_sendConfigPage(const String& errorMsg = "", const String& successMsg = "");

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
 */
void web_registerActuators(FanActuator* fanPtr = nullptr, SwitchActuator* switchPtr = nullptr);

/**
 * @brief Инициализация веб-сервера в режиме клиента WiFi
 */
void web_init();

/**
 * @brief Инициализация веб-сервера в режиме точки доступа (AP)
 * Создаёт WiFi сеть для первоначальной настройки
 */
void web_initAP();

/**
 * @brief Периодическая обработка HTTP-запросов
 * Вызывается в loop()
 */
void web_update();

// Глобальный сервер (объявлен в main.cpp)
extern WebServerClass server;

#else 
  inline void web_init() {}
  inline void web_update() {}
  inline void web_sendStatusPage(int) {}
#endif // WEB_ENABLED

#endif // WEB_H