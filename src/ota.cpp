#include "ota.h"
#include "config.h"
#include "logger.h"
#include "wifi_manager.h"

#if OTA_ENABLED == 1

// Системные заголовки
#ifdef ESP32
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <ElegantOTA.h>

// ========== СТАТИЧЕСКИЕ ПЕРЕМЕННЫЕ ==========
static bool ota_available = false;
static WebServerClass* ota_server = nullptr;

// ========== РЕАЛИЗАЦИЯ ==========
void ota_init(WebServerClass* server) {  // ← уже правильно в вашем файле
  if (!ota_available || !server)
    return;
  ota_server = server;
  ElegantOTA.begin(server);
}

bool ota_is_available() {
  return ota_available;
}

void ota_set_available(bool available) {
  ota_available = available;
  LOG_DEBUG(CAT_OTA, "Available: %s", available ? "YES" : "NO");
}

#endif  // OTA_ENABLED == 1