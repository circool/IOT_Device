#include "wifi_manager.h"
#include "config_manager.h"
#include "logger.h"
#include "web.h"

#if WIFI_ENABLED == 1
bool apMode = false;

bool wifi_is_ap_mode() {
  return apMode;
}

static unsigned long wifi_connect_start_time = 0;
bool wifi_is_connecting = false;
static unsigned long wifi_lost_time = 0;

void wifi_begin() {
  if (strlen(g_configManager.getWifiSsid()) == 0) {
    LOG_WARN(CAT_WIFI, "No SSID configured");
    return;
  }

  if (WiFi.status() == WL_CONNECTED)
    return;
  if (wifi_is_connecting)
    return;

  LOG_INFO(CAT_WIFI, "Connecting to " ANSI_BOLD "%s" ANSI_RESET,
           g_configManager.getWifiSsid());

  WiFi.mode(WIFI_STA);
  WiFi.begin(g_configManager.getWifiSsid(), g_configManager.getWifiPassword());
  wifi_is_connecting = true;
  wifi_connect_start_time = millis();
}

void wifi_check() {
  if (!wifi_is_connecting)
    return;

  wl_status_t status = WiFi.status();
  if (status == WL_CONNECTED) {
    wifi_is_connecting = false;
    wifi_lost_time = 0;
    LOG_INFO(CAT_WIFI, "Connected! IP: " ANSI_BOLD "%s" ANSI_RESET,
             WiFi.localIP().toString().c_str());

// Выход из AP режима при успешном подключении
#if AP_ENABLED == 1
    if (apMode) {
      WiFi.softAPdisconnect(true);
      apMode = false;
      LOG_INFO(CAT_WIFI, "Exited AP mode, back to client mode");
      WiFi.mode(WIFI_STA);
    }
#endif

  } else if (millis() - wifi_connect_start_time > WIFI_CONNECT_TIMEOUT_MS) {
    LOG_INFO(CAT_WIFI, "Connection timeout! (%d)", WIFI_CONNECT_TIMEOUT_MS);
    wifi_is_connecting = false;
    WiFi.disconnect();
  }
}

void wifi_monitor() {
  if (apMode) {
    if (strlen(g_configManager.getWifiSsid()) == 0)
      return;

    if (!wifi_is_connecting) {
      LOG_INFO(CAT_WIFI,
               "AP mode active, attempting to connect to WiFi in background");
      wifi_begin();
    }
    wifi_check();
    return;
  }

  if (!wifi_is_connected()) {
    if (!wifi_is_connecting) {
      LOG_INFO(CAT_WIFI, "WiFi lost, attempting to reconnect");
      wifi_begin();
    }
  }

  wifi_check();

  // Fallback в AP при длительной потере (только если не в AP режиме)
  if (!apMode && !wifi_is_connected() && !wifi_is_connecting) {
    if (wifi_lost_time == 0) {
      wifi_lost_time = millis();
      LOG_INFO(CAT_WIFI, "WiFi lost, starting fallback timer");
    } else if (millis() - wifi_lost_time > AP_FALLBACK_TIMEOUT_MS) {
      LOG_INFO(CAT_WIFI, "WiFi lost for %d ms, switching to AP mode",
               AP_FALLBACK_TIMEOUT_MS);
      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      delay(100);  // Allow WiFi hardware to fully deinitialize before AP start
                   // (critical for ESP8266)
      web_initAP();
      wifi_lost_time = 0;
    }
  } else {
    // сбрасываем таймер только при реальном подключении
    if (wifi_is_connected()) {
      wifi_lost_time = 0;
    }
  }
}

void wifi_start_ap(const char* ssid) {
  LOG_DEBUG(CAT_WIFI, "========================================");
  LOG_DEBUG(CAT_WIFI, "Starting AP with SSID: %s", ssid);
  LOG_DEBUG(CAT_WIFI, "========================================");

  // Пробуем режим WIFI_AP (только точка доступа)
  WiFi.mode(WIFI_AP);

#ifdef ESP8266
  IPAddress apIP;
  apIP.fromString(AP_IP_ADDRESS);
  if (!WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0))) {
    LOG_ERROR(CAT_WIFI, "softAPConfig FAILED!");
  }
#endif

  // Пытаемся запустить AP
  

  if (WiFi.softAP(ssid)) {
    LOG_INFO(CAT_WIFI, "AP started SUCCESSFULLY!");
    LOG_DEBUG(CAT_WIFI, "  SSID: %s", ssid);
    LOG_DEBUG(CAT_WIFI, "  IP: %s", WiFi.softAPIP().toString().c_str());
    LOG_DEBUG(CAT_WIFI, "  MAC: %s", WiFi.softAPmacAddress().c_str());
    LOG_DEBUG(CAT_WIFI, "  Channel: %d", WiFi.channel());
    LOG_DEBUG(CAT_WIFI, "  Mode: %d", WiFi.getMode());
  } else {
    LOG_ERROR(CAT_WIFI, "AP start FAILED!");
    LOG_DEBUG(CAT_WIFI, "  WiFi mode: %d", WiFi.getMode());
    LOG_DEBUG(CAT_WIFI, "  Status: %d", WiFi.status());
  }

  LOG_INFO(CAT_WIFI, "========================================");
}

String wifi_get_local_ip() {
  return WiFi.localIP().toString();
}

int wifi_get_rssi() {
  return WiFi.RSSI();
}

bool wifi_is_connected() {
  return WiFi.status() == WL_CONNECTED;
}

void wifi_start_ap_mode() {
  if (apMode)
    return;
  apMode = true;
  wifi_start_ap(g_configManager.getDeviceId());
  web_initAP();
}

#endif