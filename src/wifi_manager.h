#ifndef WIFI_H
#define WIFI_H

#include <Arduino.h>

#ifdef ESP32
  #include <WiFi.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif

#if WIFI_ENABLED == 1

extern bool wifi_is_connecting;

void wifi_begin();
void wifi_check();
void wifi_monitor();           
String wifi_get_local_ip();
int wifi_get_rssi();
bool wifi_is_connected();
void wifi_start_ap(const char* ssid);

#else

inline void wifi_begin() {}
inline void wifi_check() {}
inline void wifi_monitor() {}
inline String wifi_get_local_ip() { return "0.0.0.0"; }
inline int wifi_get_rssi() { return 0; }
inline bool wifi_is_connected() { return false; }
inline void wifi_start_ap(const char* ssid) { (void)ssid; }
static bool wifi_is_connecting = false;

#endif

#endif