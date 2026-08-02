/**
 * @file web_ota_manager.cpp
 * @brief Реализация обновления прошивки по воздухуъ
 */
#include "web_ota_manager.h"

// #include "config_manager.h"
#include "logger.h"
// #include "wifi_manager.h"

#if FEATURE_OTA_ENABLED == 1

// Системные заголовки
#ifdef ESP32
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

#include <ElegantOTA.h>

// ========== СТАТИЧЕСКИЕ ПЕРЕМЕННЫЕ ==========
static bool ota_available = false;
// static bool ota_initialized = false;
static WebServerClass* ota_server = nullptr;

// ========== РЕАЛИЗАЦИЯ ==========

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
  ota_available = result;
  XLOG_DEBUG(CAT_OTA, "Check availablity: %s", result ? "YES" : "NO");
  return result;
}

void web_ota_manager_init(WebServerClass* server) {
  if (!ota_available) {
    XLOG_WARN(CAT_OTA, "OTA not available - insufficient flash memory");
    return;
  }
  if (!server) {
    XLOG_ERROR(CAT_OTA, "WebServer is null!");
    return;
  }

  ota_server = server;

  // ElegantOTA 2.2.x — достаточно вызвать begin()
  // Все остальное обрабатывается через server.handleClient()
  ElegantOTA.begin(server);
  // ota_initialized = true;

  XLOG_DEBUG(CAT_OTA, "OTA initialized at /update");
}

// ========== web_ota_manager_update() НЕ НУЖЕН для ElegantOTA 2.2.x ==========
// ElegantOTA работает через WebServer::handleClient()
// Эта функция оставлена для совместимости, но ничего не делает

void web_ota_manager_update() {
  // ElegantOTA 2.2.x не требует отдельного loop()
  // Всё обрабатывается через server.handleClient()
  // Функция оставлена для совместимости с main.cpp
}

String ota_getButtonHtml() {
  if (ota_is_available()) {
    return F("<a href='/update' class='link-btn'>Upgrade firmware (OTA)</a>");
  } else {
    return F(
        "<div class='warning'>OTA unavailable: insufficient Flash memory (2MB "
        "required)</div>");
  }
}

#endif  // OTA_ENABLED == 1