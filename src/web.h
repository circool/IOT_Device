#ifndef WEB_H
#define WEB_H

#include <Arduino.h>
#include "config.h"


#if WEB_ENABLED == 1

#include <functional>

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

extern WebServerClass server;

String web_buildStatusHtml();
void web_sendConfigPage(const String& errorMsg = "",
                        const String& successMsg = "");
#if WEB_STATUS_ENABLED == 1
void web_sendStatusPage(int refreshInterval);
#endif
void web_saveConfig();
void web_registerActuators(FanActuator* fanPtr = nullptr,
                           SwitchActuator* switchPtr = nullptr);
void web_init();
void web_initAP();
void web_update();

#else  // WEB_ENABLED == 0

// Простые заглушки без WebServerClass
inline String web_buildStatusHtml() {
  return String();
}
inline void web_sendConfigPage(const String&, const String&) {}
#if WEB_STATUS_ENABLED == 1
inline void web_sendStatusPage(int) {}
#endif
inline void web_saveConfig() {}
inline void web_registerActuators(void*, void*) {}
inline void web_init() {}
inline void web_initAP() {}
inline void web_update() {}

#endif  // WEB_ENABLED == 1

#endif  // WEB_H