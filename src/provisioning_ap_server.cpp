/**
 * @file provisioning_ap_server.cpp
 * @brief Реализация AP-сервера для провизионинга
 */

#include "provisioning_ap_server.h"
#include "logger.h"
#include "system_state.h"
#include "web_common.h"

#if USE_AP_PROVISIONING == 1

#include <cstring>

#if defined(ESP8266)
#include <ESP8266WebServer.h>
#include <ESP8266WiFi.h>
typedef ESP8266WebServer WebServerClass;
#elif defined(ESP32)
#include <WebServer.h>
#include <WiFi.h>
typedef WebServer WebServerClass;
#endif

// ============================================================================
// СТАТИЧЕСКИЕ ОБЪЕКТЫ
// ============================================================================

static WebServerClass _server(80);
static bool _running = false;
static char _received_ssid[32] = "";
static char _received_password[64] = "";

// ============================================================================
// ВНУТРЕННИЕ ФУНКЦИИ
// ============================================================================

static void send_result_page(const char* message, bool success) {
  web_send_result_page(
      [](const char* chunk, void* context) {
        WebServerClass* srv = static_cast<WebServerClass*>(context);
        if (srv && chunk)
          srv->sendContent(chunk);
      },
      &_server, message, success);
}

static void handle_root() {
  XLOG_DEBUG(CAT_PROVISIONING, "AP: GET /");
  web_sendApProvisioningPage(
      [](const char* chunk, void* context) {
        WebServerClass* srv = static_cast<WebServerClass*>(context);
        if (srv && chunk)
          srv->sendContent(chunk);
      },
      &_server);
}

static void handle_save_wifi() {
  XLOG_INFO(CAT_PROVISIONING, "AP: POST /savewifi");

  if (_server.method() != HTTP_POST) {
    _server.send(405, "text/plain", "Method Not Allowed");
    return;
  }

  String ssid = _server.arg("wifiSsid");
  String password = _server.arg("wifiPassword");

  XLOG_DEBUG(CAT_PROVISIONING, "AP: SSID='%s', Password=%s", ssid.c_str(),
             password.length() > 0 ? "***" : "(empty)");

  if (ssid.length() == 0) {
    XLOG_WARN(CAT_PROVISIONING, "AP: SSID is empty");
    send_result_page("WiFi SSID cannot be empty", false);
    return;
  }

  if (ssid.length() >= sizeof(_received_ssid)) {
    XLOG_WARN(CAT_PROVISIONING, "AP: SSID too long (%d chars)", ssid.length());
    send_result_page("SSID too long (max 31 chars)", false);
    return;
  }

  if (password.length() >= sizeof(_received_password)) {
    XLOG_WARN(CAT_PROVISIONING, "AP: password too long (%d chars)",
              password.length());
    send_result_page("Password too long (max 63 chars)", false);
    return;
  }

  strncpy(_received_ssid, ssid.c_str(), sizeof(_received_ssid) - 1);
  _received_ssid[sizeof(_received_ssid) - 1] = '\0';

  if (password.length() > 0) {
    strncpy(_received_password, password.c_str(),
            sizeof(_received_password) - 1);
    _received_password[sizeof(_received_password) - 1] = '\0';
  } else {
    _received_password[0] = '\0';
  }

  XLOG_INFO(CAT_PROVISIONING, "AP: WiFi credentials saved");
  send_result_page("WiFi credentials saved. Device will try to connect.", true);
}

static void handle_config() {
  _server.sendHeader("Location", "/");
  _server.send(302, "text/plain", "Redirecting...");
}

static void handle_favicon() {
  _server.send(404);
}

// ============================================================================
// ПУБЛИЧНЫЕ ФУНКЦИИ
// ============================================================================

bool ap_server_start(const char* deviceId) {
  if (_running) {
    XLOG_WARN(CAT_PROVISIONING, "AP server already running");
    return true;
  }

  if (!deviceId || strlen(deviceId) == 0) {
    XLOG_ERROR(CAT_PROVISIONING, "AP server: deviceId is empty");
    return false;
  }

  XLOG_INFO(CAT_PROVISIONING, "Starting AP server with SSID: %s", deviceId);

  _received_ssid[0] = '\0';
  _received_password[0] = '\0';

  // ===== ЗАПУСК WiFi В РЕЖИМЕ AP =====
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(true, true);
  delay(100);

#ifdef ESP8266
  IPAddress apIP;
  apIP.fromString(AP_IP_ADDRESS);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
#elif defined(ESP32)
  IPAddress apIP;
  apIP.fromString(AP_IP_ADDRESS);
  WiFi.mode(WIFI_AP);
  WiFi.setTxPower(WIFI_POWER_11dBm);
  delay(50);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  delay(50);
#endif

  WiFi.softAP(deviceId);

  XLOG_DEBUG(CAT_PROVISIONING,
             "AP started. SSID: " ANSI_BOLD "%s" ANSI_BOLD_RESET
             ", IP: " ANSI_BOLD "%s",
             deviceId, AP_IP_ADDRESS);
  // ===== КОНЕЦ ЗАПУСКА AP =====

  _server.on("/", handle_root);
  _server.on("/savewifi", HTTP_POST, handle_save_wifi);
  _server.on("/config", handle_config);
  _server.on("/favicon.ico", handle_favicon);

  _server.begin();
  _running = true;

  XLOG_INFO(CAT_PROVISIONING,
            "AP server started. Connect to SSID: \"%s\" and visit %s", deviceId,
            AP_IP_ADDRESS);

  return true;
}

void ap_server_update() {
  if (_running) {
    _server.handleClient();
  }
}

void ap_server_stop() {
  if (!_running)
    return;

  XLOG_DEBUG(CAT_PROVISIONING, "Stopping AP server...");

  _server.stop();
  WiFi.softAPdisconnect(true);
  _running = false;

  XLOG_DEBUG(CAT_PROVISIONING, "AP server stopped");
}

bool ap_server_is_running() {
  return _running;
}

const char* ap_server_get_ssid() {
  return (_received_ssid[0] != '\0') ? _received_ssid : nullptr;
}

const char* ap_server_get_password() {
  return (_received_password[0] != '\0') ? _received_password : nullptr;
}

bool ap_server_has_data() {
  return _received_ssid[0] != '\0';
}

void ap_server_clear_data() {
  _received_ssid[0] = '\0';
  _received_password[0] = '\0';
}

#endif  // USE_AP_PROVISIONING