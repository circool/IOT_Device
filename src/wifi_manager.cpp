#include "wifi_manager.h"
#include "config.h"
#include "logger.h"
#include "web.h"

#if WIFI_ENABLED == 1

static unsigned long wifi_connect_start_time = 0;
bool wifi_is_connecting = false;
static unsigned long wifi_lost_time = 0;

// ============================================================================
// wifi_begin() - Начать подключение к WiFi
// ============================================================================
void wifi_begin() {
  if (strlen(config_get()->wifiSsid) == 0) {
    LOG_INFO(CAT_WIFI, "No SSID configured");
    return;
  }
    
  if (WiFi.status() == WL_CONNECTED) return;
  if (wifi_is_connecting) return;
  
  LOG_INFO(CAT_WIFI, "Connecting to %s", config_get()->wifiSsid);
    
  WiFi.mode(WIFI_STA);
  WiFi.begin(config_get()->wifiSsid, config_get()->wifiPassword);
  wifi_is_connecting = true;
  wifi_connect_start_time = millis();

}

// ============================================================================
// wifi_check() - Проверить статус активного подключения
// ============================================================================
void wifi_check() {
  if (!wifi_is_connecting) return;
  
  wl_status_t status = WiFi.status();  
  if (status == WL_CONNECTED) {
    wifi_is_connecting = false;
    wifi_lost_time = 0;
    LOG_INFO(CAT_WIFI, "Connected! IP: %s", WiFi.localIP().toString().c_str());   
    
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
    LOG_INFO(CAT_WIFI, "Connection timeout!");
    wifi_is_connecting = false;
    WiFi.disconnect();
    
  }
}

// ============================================================================
// wifi_monitor() - Постоянный мониторинг WiFi (вызывается в loop)
// ============================================================================
void wifi_monitor() {
  // --- Режим AP: проверяем, не восстановился ли WiFi ---
  if (apMode) {
      if (strlen(config_get()->wifiSsid) == 0) return;
      
      // Запускаем фоновое подключение, если ещё не пытаемся
      if (!wifi_is_connecting) {
          LOG_INFO(CAT_WIFI, "AP mode active, attempting to connect to WiFi in background");
          wifi_begin();
      }
      
      // Проверяем статус
      wifi_check();  // ← используем существующую логику проверки
      
      // Если подключились — wifi_check() сам выключит AP
      return;
  }
    
  // --- Нормальный режим: проверяем, не потеряли ли соединение ---
  if (!wifi_is_connected()) {
      if (!wifi_is_connecting) {
          LOG_INFO(CAT_WIFI, "WiFi lost, attempting to reconnect");
          wifi_begin();
      }
  }
  
  // Проверяем прогресс подключения
  wifi_check();
    
  // Fallback в AP при длительной потере (только если не в AP режиме)
  if (!apMode && !wifi_is_connected() && !wifi_is_connecting) {
      if (wifi_lost_time == 0) {
          wifi_lost_time = millis();
          LOG_INFO(CAT_WIFI, "WiFi lost, starting fallback timer");
      } else if (millis() - wifi_lost_time > AP_FALLBACK_TIMEOUT_MS) {
          LOG_INFO(CAT_WIFI, "WiFi lost for %d ms, switching to AP mode", AP_FALLBACK_TIMEOUT_MS);
          
          WiFi.disconnect(true);
          WiFi.mode(WIFI_OFF);
          delay(100);
          
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

// ============================================================================
// wifi_start_ap() - Запустить точку доступа (mixed mode)
// ============================================================================
void wifi_start_ap(const char* ssid) {
    WiFi.mode(WIFI_AP_STA);
    
    #ifdef ESP8266
    IPAddress apIP;
    apIP.fromString(AP_IP_ADDRESS);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    #endif
    
    WiFi.softAP(ssid);
    
    LOG_INFO(CAT_WIFI, "AP started: SSID=%s, IP=%s", ssid, AP_IP_ADDRESS);
}

// ============================================================================
// wifi_scan_and_log() - Сканирование WiFi сетей с выводом в лог
// ============================================================================
int wifi_scan_and_log(const char* targetSsid) {
    static bool is_scanning = false;
    
    // Защита от реентерабельности
    if (is_scanning) {
        LOG_WARN(CAT_WIFI, "Scan already in progress, skipping");
        return -1;
    }
    
    // Не сканируем, если в процессе подключения к другой сети
    if (wifi_is_connecting) {
        LOG_WARN(CAT_WIFI, "Cannot scan while connecting to WiFi");
        return -1;
    }
    
    is_scanning = true;
    
    LOG_INFO(CAT_WIFI, "Scanning WiFi networks...");
    
    int networksFound = WiFi.scanNetworks();
    
    if (networksFound == WIFI_SCAN_FAILED) {
        LOG_ERROR(CAT_WIFI, "WiFi scan failed");
        WiFi.scanDelete();
        is_scanning = false;
        return -1;
    }
    
    bool markTarget = (targetSsid != nullptr && strlen(targetSsid) > 0);
    
    for (int i = 0; i < networksFound; i++) {
        String ssid = WiFi.SSID(i);
        int32_t rssi = WiFi.RSSI(i);
        
        if (markTarget && ssid == targetSsid) {
            LOG_DEBUG(CAT_WIFI, "%s (RSSI: %d) " ANSI_BRIGHT_GREEN "<<< TARGET" ANSI_RESET, 
                      ssid.c_str(), rssi);
        } else {
            LOG_DEBUG(CAT_WIFI, "%s (RSSI: %d)", ssid.c_str(), rssi);
        }
    }
    
    LOG_INFO(CAT_WIFI, "Scan complete: %d network(s) found", networksFound);
    
    WiFi.scanDelete();
    is_scanning = false;
    
    return networksFound;
}


// ============================================================================
// Вспомогательные функции
// ============================================================================
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