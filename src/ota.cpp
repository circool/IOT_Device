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
  bool result = false;
#ifdef ESP8266
  uint32_t flashSize = ESP.getFlashChipRealSize();
  uint32_t freeSketchSpace = ESP.getFreeSketchSpace();
  uint32_t currentSketchSize = ESP.getSketchSize();
  result = (flashSize >= (2 * 1024 * 1024)) &&
         (freeSketchSpace >= currentSketchSize);
#elif defined(ESP32)
      result = (ESP.getFlashChipSize() >= (2 * 1024 * 1024));
#endif
  LOG_INFO(CAT_OTA, "Check availablity: %s", result ? "YES" : "NO");
  return result;
}

#endif  // OTA_ENABLED == 1