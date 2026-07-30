#include "wifi_manager.h"
#include "config_manager.h"
#include "system_state.h"
#include "logger.h"


#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI

void wifi_manager_init() {
  XLOG_INFO(CAT_WIFI, "Initializing WiFi manager...");
  WiFi.mode(WIFI_STA);

// Для компенсации неустойчивого соединения на ESP 32C снижаем мощность до 8.5 dBm
#if PLATFORM_ESP32C3==1
  WiFi.setTxPower(WIFI_POWER_8_5dBm); 
#endif
  WiFi.setAutoReconnect(true);

}

void wifi_manager_connect(const char* ssid, const char* password) {
  if (strlen(ssid) == 0) {
    XLOG_WARN(CAT_WIFI, "No SSID configured");
    return;
  }
  XLOG_INFO(CAT_WIFI, "Connecting to " ANSI_BOLD "%s" ANSI_RESET "...", ssid);
  WiFi.begin(ssid, password);
}

void wifi_manager_update() {
  if (WiFi.status() == WL_CONNECTED) {
    if (!system_state_has_bit(STATE_WIFI_OK)) {
      XLOG_INFO(CAT_WIFI, "Connected! IP: " ANSI_BOLD "%s" ANSI_BOLD_RESET ", RSSI: " ANSI_BOLD "%d" ANSI_BOLD_RESET " dBm.", WiFi.localIP().toString().c_str(), WiFi.RSSI());
      system_state_set_bit(STATE_WIFI_OK);
    }
     
  } else {
    system_state_clear_bit(STATE_WIFI_OK);
  }
}

int wifi_scan_and_log(const char* targetSsid) {
  #if SCANNING_WIFI_ENABLED == 0
  return -1;
  #endif
  
  static bool is_scanning = false;

  if (is_scanning) {
    XLOG_WARN(CAT_WIFI, "Scan already in progress, skipping");
    return -1;
  }

  if (WiFi.status() == WL_IDLE_STATUS || WiFi.status() == WL_SCAN_COMPLETED) {
    XLOG_WARN(CAT_WIFI, "Cannot scan while connecting to WiFi");
    return -1;
  }

  is_scanning = true;
  XLOG_DEBUG(CAT_WIFI, "Scanning WiFi networks...");

  int networksFound = WiFi.scanNetworks();

  if (networksFound == WIFI_SCAN_FAILED) {
    XLOG_ERROR(CAT_WIFI, "WiFi scan failed");
    WiFi.scanDelete();
    is_scanning = false;
    return -1;
  }

  bool markTarget = (targetSsid != nullptr && strlen(targetSsid) > 0);
  XLOG_DEBUG(CAT_WIFI, "%-32s %-8s %-8s %-12s %s", "SSID", "RSSI", "CH", "AUTH","TARGET");
  XLOG_DEBUG(CAT_WIFI,"------------------------------------------------------------");

  for (int i = 0; i < networksFound; i++) {
    String ssid = WiFi.SSID(i);
    int32_t rssi = WiFi.RSSI(i);
    int channel = WiFi.channel(i);

    String authType;

#ifdef ESP8266
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
    authType = "?";
#endif

    String targetMark = (markTarget && ssid == targetSsid) ? "<<<" : "";
    XLOG_DEBUG(CAT_WIFI, "%-32s %-4d dBm  %-3d  %-12s  %s", ssid.c_str(), rssi,
               channel, authType.c_str(), targetMark.c_str());
  }

  XLOG_DEBUG(CAT_WIFI, "Scan complete: %d network(s) found", networksFound);
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

#if PROVISIONING_METHOD == 2 || PROVISIONING_METHOD == 3
void wifi_start_ap(const char* ssid) {
  WiFi.setAutoReconnect(false);
  WiFi.disconnect(true, true);
  
  delay(100);
  XLOG_INFO(CAT_WIFI, "Starting AP mode: %s", ssid);

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
  WiFi.setTxPower(WIFI_POWER_11dBm);  // или WIFI_POWER_11dBm
  delay(50);

  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
  delay(50);
#endif

  WiFi.softAP(ssid);

  XLOG_DEBUG(CAT_WIFI,
             "AP started. SSID: " ANSI_BOLD
             "%s" ANSI_BOLD_RESET ", IP: " ANSI_BOLD "%s",
             ssid, AP_IP_ADDRESS);
}

void wifi_stop_ap() {
  WiFi.softAPdisconnect(true);
}

#endif  // PROVISIONING_METHOD
#endif  // TRANSPORT_TYPE
