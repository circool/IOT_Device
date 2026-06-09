// ===== ФАЙЛ: wifi_manager.h (исправленный) =====
#ifndef WIFI_H
#define WIFI_H

#include <Arduino.h>

#ifdef ESP32
  #include <WiFi.h>
  #include <WiFiClient.h>
  #include <WiFiServer.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
#endif

#if WIFI_ENABLED == 1



// ========== ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ (extern) ==========
extern bool wifi_is_connecting;
extern unsigned long wifi_lost_time;

void wifi_begin();
void wifi_check();
void wifi_fallback_to_ap();

String wifi_get_local_ip();
int wifi_get_rssi();
bool wifi_is_connected();

void wifi_start_ap(const char* ssid);
void wifi_stop_ap();

typedef void (*wifi_ap_mode_callback_t)(void);
void wifi_register_ap_callback(wifi_ap_mode_callback_t callback);

#else

inline void wifi_begin() {}
inline void wifi_check() {}
inline void wifi_fallback_to_ap() {}
inline String wifi_get_local_ip() { return "0.0.0.0"; }
inline int wifi_get_rssi() { return 0; }
inline bool wifi_is_connected() { return false; }
inline void wifi_start_ap(const char* ssid) { (void)ssid; }
inline void wifi_stop_ap() {}
inline void wifi_register_ap_callback(wifi_ap_mode_callback_t callback) { (void)callback; }

static bool wifi_is_connecting = false;
static unsigned long wifi_lost_time = 0;

#endif

#endif