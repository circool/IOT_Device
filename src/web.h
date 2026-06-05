#ifndef WEB_H
#define WEB_H

#ifdef ESP32
  #include <ESPAsyncWebServer.h>
  extern AsyncWebServer server;
#elif defined(ESP8266)
  #include <ESP8266WebServer.h>
  extern ESP8266WebServer server;
#endif

// Функции для ESP8266
#ifdef ESP8266
  void web_sendConfigPage(const String& errorMsg);
  #if WEB_STATUS_ENABLED == 1
    void web_sendStatusPage(int refreshInterval);
  #endif
  void web_saveConfig();
#else
  // Для ESP32
  void web_sendConfigPage(AsyncWebServerRequest *request, const String& errorMsg);
  #if WEB_STATUS_ENABLED == 1
    void web_sendStatusPage(AsyncWebServerRequest *request, int refreshInterval);
  #endif
  void web_saveConfig(AsyncWebServerRequest *request);
#endif

// Формирует HTML-строку статуса для страницы состояния
String web_buildStatusHtml();

#if OTA_ENABLED == 1
    void web_setOtaAvailable(bool available);
    bool web_isOtaAvailable();
#endif

void web_init();
void web_initAP();
void web_update();

#endif // WEB_H