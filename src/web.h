#ifndef WEB_H
#define WEB_H

#ifdef ESP32
  #include <ESPAsyncWebServer.h>
  extern AsyncWebServer server;
#elif defined(ESP8266)
  #include <ESP8266WebServer.h>
  extern ESP8266WebServer server;
#endif

// Функции для ESP8266 (отправка HTML частями)
#ifdef ESP8266
  void web_sendConfigPage(const String& errorMsg);
  #if WEB_STATUS_ENABLED == 1
    void web_sendStatusPage(int refreshInterval);
  #endif
  void web_saveConfig();
#else
  // Для ESP32 (стандартный подход с String)
  String web_getConfigPage(String errorMsg);
  #if WEB_STATUS_ENABLED == 1
    String web_getStatusPage(int refreshInterval);
  #endif
  void web_saveConfig(AsyncWebServerRequest *request);
#endif

void web_init();
void web_initAP();
void web_update();

#endif // WEB_H