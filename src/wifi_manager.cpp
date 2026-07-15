#include "wifi_manager.h"
#include "config_manager.h"
#include "logger.h"
#include "web.h"

#if FEATURE_WIFI_ENABLED == 1
bool apMode = false;

bool wifi_is_ap_mode() {
  // Если флаг установлен - точно AP режим
  if (apMode)
    return true;

// Проверяем реальное состояние WiFi
#if defined(ESP32)
  wifi_mode_t mode = WiFi.getMode();
  return (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA);
#elif defined(ESP8266)
  uint8_t mode = WiFi.getMode();
  return (mode == WIFI_AP || mode == WIFI_AP_STA);
#else
  return false;
#endif
}

static unsigned long wifi_connect_start_time = 0;
bool wifi_is_connecting = false;
static unsigned long wifi_lost_time = 0;

void wifi_manager_init() {
  XLOG_INFO(CAT_WIFI, "Initializing WiFi manager...");

  // ================================================================
  // ТОЛЬКО МИНИМАЛЬНАЯ ИНИЦИАЛИЗАЦИЯ — НИКАКОГО WiFi.begin()!
  // ================================================================

  // Отключаем persistent режим (не сохраняем credentials в NVS)
  WiFi.persistent(false);

  // Устанавливаем режим STA
  WiFi.mode(WIFI_STA);

  // Отключаем энергосбережение для стабильности
  WiFi.setSleep(false);

  // Отключаем авто-переподключение — управляем вручную
  WiFi.setAutoReconnect(false);

  // Сбрасываем флаги
  wifi_is_connecting = false;
  wifi_lost_time = 0;
  wifi_connect_start_time = 0;

  XLOG_INFO(CAT_WIFI, "WiFi manager initialized (ready to connect)");
}

void wifi_begin() {
  if (strlen(g_configManager.getWifiSsid()) == 0) {
    XLOG_WARN(CAT_WIFI, "No SSID configured");
    return;
  }

  if (WiFi.status() == WL_CONNECTED) {
    XLOG_DEBUG(CAT_WIFI, "Already connected");
    return;
  }

  if (wifi_is_connecting) {
    XLOG_DEBUG(CAT_WIFI, "Already connecting");
    return;
  }

  XLOG_INFO(CAT_WIFI, "Connecting to " ANSI_BOLD "%s" ANSI_RESET,
            g_configManager.getWifiSsid());

  // ================================================================
  // ЗАПУСКАЕМ ПОДКЛЮЧЕНИЕ — НЕБЛОКИРУЮЩЕЕ!
  // ================================================================
  WiFi.begin(g_configManager.getWifiSsid(), g_configManager.getWifiPassword());

  wifi_is_connecting = true;
  wifi_connect_start_time = millis();
  wifi_lost_time = 0;
}

void wifi_check() {
  if (!wifi_is_connecting)
    return;

  wl_status_t status = WiFi.status();

  switch (status) {
    case WL_CONNECTED:
      wifi_is_connecting = false;
      wifi_lost_time = 0;
      XLOG_INFO(CAT_WIFI, "Connected! IP: " ANSI_BOLD "%s" ANSI_RESET,
                WiFi.localIP().toString().c_str());

      // Выход из AP режима при успешном подключении
      if (apMode) {
        WiFi.softAPdisconnect(true);
        apMode = false;
        XLOG_INFO(CAT_WIFI, "Exited AP mode, back to client mode");
        WiFi.mode(WIFI_STA);
      }
      break;

    case WL_IDLE_STATUS:
    case WL_SCAN_COMPLETED:
    case WL_NO_SSID_AVAIL:
    case WL_DISCONNECTED:
      // Промежуточные состояния — просто ждём
      break;

    case WL_CONNECT_FAILED:
      XLOG_WARN(CAT_WIFI, "Connection failed, will retry");
      wifi_is_connecting = false;
      // Не отключаемся — wifi_monitor() вызовет wifi_begin() снова
      break;

    default:
      break;
  }
  if (millis() - wifi_connect_start_time > WIFI_CONNECT_TIMEOUT_MS) {
    XLOG_INFO(CAT_WIFI, "Connection timeout! (%d)", WIFI_CONNECT_TIMEOUT_MS);
    wifi_is_connecting = false;
    WiFi.disconnect(true, true);
  }
}

void wifi_monitor() {
  if (apMode) {
    if (strlen(g_configManager.getWifiSsid()) == 0)
      return;

    if (!wifi_is_connecting) {
      XLOG_INFO(CAT_WIFI,
                "AP mode active, attempting to connect to WiFi in background");
      wifi_begin();
    }
    wifi_check();
    return;
  }

  if (!wifi_is_connected()) {
    if (!wifi_is_connecting) {
      XLOG_INFO(CAT_WIFI, "WiFi lost, attempting to reconnect");
      wifi_begin();
    }
  }

  wifi_check();

  // Fallback в AP при длительной потере (только если не в AP режиме)
  if (!apMode && !wifi_is_connected() && !wifi_is_connecting) {
    if (wifi_lost_time == 0) {
      wifi_lost_time = millis();
      XLOG_INFO(CAT_WIFI, "WiFi lost, starting fallback timer");
    } else if (millis() - wifi_lost_time > AP_FALLBACK_TIMEOUT_MS) {
      XLOG_INFO(CAT_WIFI, "WiFi lost for %d ms, switching to AP mode",
                AP_FALLBACK_TIMEOUT_MS);

      WiFi.disconnect(true);
      WiFi.mode(WIFI_OFF);
      delay(100);

      // Запускаем AP через WiFiManager
      const char* deviceId = g_configManager.getDeviceId();
      wifi_start_ap(deviceId);

      // Web в режиме настройки
      web_init(true);

      wifi_lost_time = 0;
    }
  }
}

void wifi_start_ap(const char* ssid) {
  // WiFi.mode(WIFI_AP);

#ifdef ESP8266
  IPAddress apIP;
  apIP.fromString(AP_IP_ADDRESS);
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
#elif defined(ESP32)
  // ================================================================
  // ESP32-C3 SuperMini: снижаем TX мощность из-за аппаратных проблем
  // ================================================================

  IPAddress apIP;
  apIP.fromString(AP_IP_ADDRESS);
  WiFi.mode(WIFI_AP);

  // Снижаем мощность передачи (решает проблему с AP на некоторых C3)
  WiFi.setTxPower(WIFI_POWER_8_5dBm);  // или WIFI_POWER_11dBm
  delay(50);

  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  delay(50);
#endif

  WiFi.softAP(ssid);
  apMode = true;
  XLOG_DEBUG(CAT_WIFI,
             "AP started. Find WiFi " ANSI_BOLD "%s" ANSI_BOLD_RESET
             ", connect and visit " ANSI_BOLD "%s",
             ssid, AP_IP_ADDRESS);
}

void wifi_stop_ap() {
  WiFi.softAPdisconnect(true);
  apMode = false;
}

int wifi_scan_and_log(const char* targetSsid) {
  static bool is_scanning = false;

  // Защита от реентерабельности
  if (is_scanning) {
    XLOG_WARN(CAT_WIFI, "Scan already in progress, skipping");
    return -1;
  }

  // Не сканируем, если в процессе подключения к другой сети
  if (wifi_is_connecting) {
    XLOG_WARN(CAT_WIFI, "Cannot scan while connecting to WiFi");
    return -1;
  }

  is_scanning = true;

  XLOG_INFO(CAT_WIFI, "Scanning WiFi networks...");

  int networksFound = WiFi.scanNetworks();

  if (networksFound == WIFI_SCAN_FAILED) {
    XLOG_ERROR(CAT_WIFI, "WiFi scan failed");
    WiFi.scanDelete();
    is_scanning = false;
    return -1;
  }

  bool markTarget = (targetSsid != nullptr && strlen(targetSsid) > 0);

  // Заголовок таблицы
  XLOG_DEBUG(CAT_WIFI, "%-32s %-8s %-8s %-12s %s", "SSID", "RSSI", "CH", "AUTH",
             "TARGET");
  XLOG_DEBUG(CAT_WIFI,
             "------------------------------------------------------------");

  for (int i = 0; i < networksFound; i++) {
    String ssid = WiFi.SSID(i);
    int32_t rssi = WiFi.RSSI(i);
    int channel = WiFi.channel(i);

    // === ОПРЕДЕЛЕНИЕ ТИПА ШИФРОВАНИЯ ===
    String authType;

#ifdef ESP8266
    // ESP8266: используем ENC_TYPE_* константы
    uint8_t encryption = WiFi.encryptionType(i);
    switch (encryption) {
      case ENC_TYPE_NONE:
        authType = "Open";
        break;
      case ENC_TYPE_TKIP:
        authType = "WPA";
        break;
      case ENC_TYPE_CCMP:
        authType = "WPA2";
        break;
      case ENC_TYPE_AUTO:
        authType = "Auto";
        break;
      default:
        authType = "Unknown";
        break;
    }
#elif defined(ESP32)
    // ESP32: используем wifi_auth_mode_t
    wifi_auth_mode_t encryption = WiFi.encryptionType(i);
    switch (encryption) {
      case WIFI_AUTH_OPEN:
        authType = "Open";
        break;
      case WIFI_AUTH_WEP:
        authType = "WEP";
        break;
      case WIFI_AUTH_WPA_PSK:
        authType = "WPA";
        break;
      case WIFI_AUTH_WPA2_PSK:
        authType = "WPA2";
        break;
      case WIFI_AUTH_WPA_WPA2_PSK:
        authType = "WPA/WPA2";
        break;
      case WIFI_AUTH_WPA2_ENTERPRISE:
        authType = "WPA2-Ent";
        break;
      case WIFI_AUTH_WPA3_PSK:
        authType = "WPA3";
        break;
      case WIFI_AUTH_WPA2_WPA3_PSK:
        authType = "WPA2/WPA3";
        break;
      case WIFI_AUTH_WAPI_PSK:
        authType = "WAPI";
        break;
      default:
        authType = "Unknown";
        break;
    }
#else
    // Fallback
    authType = "?";
#endif

    // Формируем строку с пометкой TARGET
    String targetMark = (markTarget && ssid == targetSsid) ? "<<<" : "";

    // Вывод с информацией
    XLOG_DEBUG(CAT_WIFI, "%-32s %-4d dBm  %-3d  %-12s  %s", ssid.c_str(), rssi,
               channel, authType.c_str(), targetMark.c_str());
  }

  XLOG_INFO(CAT_WIFI, "Scan complete: %d network(s) found", networksFound);

  WiFi.scanDelete();
  is_scanning = false;

  return networksFound;
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

#endif