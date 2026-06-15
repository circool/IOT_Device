#include "wifi_manager.h"
#include "config.h"
#include "logger.h"

#if WIFI_ENABLED == 1

// ============================================================================
// СТАТИЧЕСКИЕ ПЕРЕМЕННЫЕ (скрытые внутри модуля)
// ============================================================================

static bool _isConnecting = false;
static unsigned long _connectStartTime = 0;
static bool _isAPActive = false;
static char _lastSsid[32] = "";
static char _lastPassword[64] = "";

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================================

/**
 * @brief Сбросить состояние подключения
 */
static void resetConnectionState() {
  _isConnecting = false;
  _connectStartTime = 0;
}

/**
 * @brief Получить строковое представление статуса WiFi
 */
static const char* statusToString(wl_status_t status) {
  switch (status) {
    case WL_IDLE_STATUS:
      return "IDLE";
    case WL_NO_SSID_AVAIL:
      return "NO_SSID_AVAIL";
    case WL_SCAN_COMPLETED:
      return "SCAN_COMPLETED";
    case WL_CONNECTED:
      return "CONNECTED";
    case WL_CONNECT_FAILED:
      return "CONNECT_FAILED";
    case WL_CONNECTION_LOST:
      return "CONNECTION_LOST";
    case WL_DISCONNECTED:
      return "DISCONNECTED";
    default:
      return "UNKNOWN";
  }
}

// ============================================================================
// ПУБЛИЧНЫЕ ФУНКЦИИ
// ============================================================================

void wifi_init() {
  LOG_INFO(CAT_WIFI, "Initializing WiFi...");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);
  delay(100);

  LOG_DEBUG(CAT_WIFI, "WiFi initialized in STA mode");
}

bool wifi_connect(const char* ssid, const char* password) {
  if (!ssid || strlen(ssid) == 0) {
    LOG_WARN(CAT_WIFI, "Cannot connect: empty SSID");
    return false;
  }

  if (wifi_isConnected()) {
    LOG_DEBUG(CAT_WIFI, "Already connected to %s", ssid);
    return true;
  }

  if (_isConnecting) {
    LOG_DEBUG(CAT_WIFI, "Already connecting to %s", _lastSsid);
    return true;
  }

  LOG_INFO(CAT_WIFI, "Connecting to SSID: '%s'", ssid);

  // Сохраняем для переподключения
  strncpy(_lastSsid, ssid, sizeof(_lastSsid) - 1);
  _lastSsid[sizeof(_lastSsid) - 1] = '\0';
  if (password) {
    strncpy(_lastPassword, password, sizeof(_lastPassword) - 1);
    _lastPassword[sizeof(_lastPassword) - 1] = '\0';
  } else {
    _lastPassword[0] = '\0';
  }

  // Запускаем асинхронное подключение
  WiFi.begin(ssid, password);
  _isConnecting = true;
  _connectStartTime = millis();

  return true;
}

bool wifi_isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

String wifi_getLocalIP() {
  return WiFi.localIP().toString();
}

int wifi_getRSSI() {
  return WiFi.RSSI();
}

wl_status_t wifi_getStatus() {
  return WiFi.status();
}

void wifi_process() {
  // Если не в процессе подключения — выходим
  if (!_isConnecting) {
    return;
  }

  wl_status_t status = WiFi.status();

  if (status == WL_CONNECTED) {
    // Подключились успешно
    LOG_INFO(CAT_WIFI, "Connected! IP: %s, RSSI: %d dBm",
             wifi_getLocalIP().c_str(), wifi_getRSSI());
    resetConnectionState();
  } else if (millis() - _connectStartTime > WIFI_CONNECT_TIMEOUT_MS) {
    // Таймаут подключения
    LOG_WARN(CAT_WIFI, "Connection timeout! Status: %s",
             statusToString(status));
    WiFi.disconnect();
    resetConnectionState();
  }
}

bool wifi_startAP(const char* ssid, const char* password) {
  if (!ssid || strlen(ssid) == 0) {
    LOG_WARN(CAT_WIFI, "Cannot start AP: empty SSID");
    return false;
  }

  if (_isAPActive) {
    LOG_DEBUG(CAT_WIFI, "AP already active");
    return true;
  }

  LOG_INFO(CAT_WIFI, "Starting AP mode: SSID='%s'", ssid);

  // Отключаем клиентский режим
  WiFi.disconnect(true);
  WiFi.mode(WIFI_AP);

  // Настраиваем IP (для ESP8266)
#ifdef ESP8266
  IPAddress apIP;
  apIP.fromString(AP_IP_ADDRESS);
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
#endif

  // Запускаем точку доступа
  if (password && strlen(password) >= 8) {
    WiFi.softAP(ssid, password);
    LOG_INFO(CAT_WIFI, "AP started with password (secured)");
  } else {
    WiFi.softAP(ssid);
    LOG_INFO(CAT_WIFI, "AP started without password (open)");
  }

  _isAPActive = true;
  LOG_INFO(CAT_WIFI, "AP IP: %s", AP_IP_ADDRESS);

  return true;
}

void wifi_stopAP() {
  if (!_isAPActive) {
    return;
  }

  LOG_INFO(CAT_WIFI, "Stopping AP mode...");
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  _isAPActive = false;
  LOG_INFO(CAT_WIFI, "AP stopped, back to STA mode");
}

bool wifi_isAPActive() {
  return _isAPActive;
}

int wifi_scan(const char* targetSsid) {
  static bool isScanning = false;

  if (isScanning) {
    LOG_WARN(CAT_WIFI, "Scan already in progress, skipping");
    return -1;
  }

  isScanning = true;
  LOG_INFO(CAT_WIFI, "Scanning WiFi networks...");

  int networksFound = WiFi.scanNetworks();

  if (networksFound == WIFI_SCAN_FAILED) {
    LOG_ERROR(CAT_WIFI, "WiFi scan failed");
    WiFi.scanDelete();
    isScanning = false;
    return -1;
  }

  bool markTarget = (targetSsid != nullptr && strlen(targetSsid) > 0);

  for (int i = 0; i < networksFound; i++) {
    String ssid = WiFi.SSID(i);
    int32_t rssi = WiFi.RSSI(i);

    if (markTarget && ssid == targetSsid) {
      LOG_DEBUG(CAT_WIFI, "%s (RSSI: %d) <<< TARGET", ssid.c_str(), rssi);
    } else {
      LOG_DEBUG(CAT_WIFI, "%s (RSSI: %d)", ssid.c_str(), rssi);
    }
  }

  LOG_INFO(CAT_WIFI, "Scan complete: %d network(s) found", networksFound);

  WiFi.scanDelete();
  isScanning = false;
  return networksFound;
}

#endif  // WIFI_ENABLED == 1