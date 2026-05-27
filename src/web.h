// @filename: web.h
#ifndef WEB_H
#define WEB_H

#ifdef ESP32
  #include <ESPAsyncWebServer.h>
  extern AsyncWebServer server;
#elif defined(ESP8266)
  #include <ESP8266WebServer.h>
  extern ESP8266WebServer server;
#endif



// Прототипы функций, которые используются всегда
String web_getConfigPage(String errorMsg);

// Прототипы функций, которые зависят от WEB_STATUS_ENABLED
// Объявляем их условно, чтобы экономить память на ESP8266, если статус не нужен.
#if WEB_STATUS_ENABLED == 1
  String web_getStatusPage(int refreshInterval);
#endif

// Прототипы для сохранения конфигурации
#ifdef ESP32
  void web_saveConfig(AsyncWebServerRequest *request);
#elif defined(ESP8266)
  void web_saveConfig();
#endif

void web_init();
void web_initAP();
void web_update();

#endif // WEB_H