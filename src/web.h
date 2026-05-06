#ifndef WEB_H
#define WEB_H

#ifdef ESP32
  #include <ESPAsyncWebServer.h>
  extern AsyncWebServer server;
#elif defined(ESP8266)
  #include <ESP8266WebServer.h>
  extern ESP8266WebServer server;
#endif

void web_init();
void web_initAP();
void web_update();

#endif