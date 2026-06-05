#ifndef WEB_H
#define WEB_H

#include <Arduino.h>
#include "config.h"

#if defined(ESP8266)
  #include <ESP8266WebServer.h>
  #define WebServerClass ESP8266WebServer
#elif defined(ESP32)
  #include <WebServer.h>
  #define WebServerClass WebServer
#endif

extern WebServerClass server;

// ========== ОБЩИЕ ФУНКЦИИ ==========
String web_buildStatusHtml();

#if OTA_ENABLED == 1
void web_setOtaAvailable(bool available);
bool web_isOtaAvailable();
#endif

void web_sendConfigPage(const String& errorMsg = "", const String& successMsg = "");
#if WEB_STATUS_ENABLED == 1
void web_sendStatusPage(int refreshInterval);
#endif
void web_saveConfig();

void web_init();
void web_initAP();
void web_update();

#endif // WEB_H