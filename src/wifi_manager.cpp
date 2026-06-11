#include "wifi_manager.h"
#include "config.h"
#include "led.h"
#include "logger.h"
#include "web.h"

#if WIFI_ENABLED == 1



static unsigned long wifi_connect_start_time = 0;

bool wifi_is_connecting = false;
unsigned long wifi_lost_time = 0;

void wifi_begin() {
    #if STATUS_LED_PIN > 0
        led_setMode(LED_MODE_SLOW_BLINK);
    #endif
    
    if (strlen(config_get()->wifiSsid) == 0) {
        LOG_INFO(CAT_WIFI, "No SSID configured");

        return;
    }
    
    if (WiFi.status() == WL_CONNECTED) return;
    if (wifi_is_connecting) return;
    
    LOG_INFO(CAT_WIFI, "Starting async connection to %s", config_get()->wifiSsid);

    
    WiFi.mode(WIFI_STA);
    WiFi.begin(config_get()->wifiSsid, config_get()->wifiPassword);
    wifi_is_connecting = true;
    wifi_connect_start_time = millis();
    wifi_lost_time = 0;
}

void wifi_check() {
    if (!wifi_is_connecting) return;
    
    wl_status_t status = WiFi.status();
    
    if (status == WL_CONNECTED) {
        wifi_is_connecting = false;
        wifi_lost_time = 0;
        
        LOG_INFO(CAT_WIFI, "Connected! IP: %s", WiFi.localIP().toString().c_str());

        
        #if STATUS_LED_PIN > 0
            #if MQTT_ENABLED == 1
                led_setMode(LED_MODE_FAST_BLINK);
            #else
                led_setMode(LED_MODE_ON);
            #endif
        #endif

        #if AP_ENABLED == 1
            if (apMode) {
                WiFi.softAPdisconnect(true);
                apMode = false;
            }
        #endif
        
    } else if (millis() - wifi_connect_start_time > WIFI_CONNECT_TIMEOUT_MS) {
        LOG_INFO(CAT_WIFI, "Connection timeout!");
        wifi_is_connecting = false;
        WiFi.disconnect();

        #if STATUS_LED_PIN > 0
            led_setMode(LED_MODE_SLOW_BLINK);
        #endif
    }
}

void wifi_fallback_to_ap() {
    if (strlen(config_get()->wifiSsid) == 0) return;
    if (apMode) return;
    
    IPAddress ip = WiFi.localIP();
    bool hasValidIp = (ip != IPAddress(0,0,0,0));
    bool isConnected = (WiFi.status() == WL_CONNECTED && hasValidIp);
    
    if (isConnected) {
        wifi_lost_time = 0;
        return;
    }
    
    if (!isConnected && !wifi_is_connecting) {
        if (wifi_lost_time == 0) {
            wifi_lost_time = millis();
            LOG_INFO(CAT_WIFI, "WiFi lost, starting fallback timer");

            
            #if STATUS_LED_PIN > 0
                led_setMode(LED_MODE_SLOW_BLINK);
            #endif
            
        } else if (millis() - wifi_lost_time > AP_FALLBACK_TIMEOUT_MS) {
            LOG_INFO(CAT_WIFI, "WiFi lost for %d ms, switching to AP mode", AP_FALLBACK_TIMEOUT_MS);

            
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
            delay(100);
            
            web_initAP();
            wifi_lost_time = 0;
        }
    } else if (!isConnected && wifi_is_connecting) {
        wifi_lost_time = millis();
    }
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

void wifi_start_ap(const char* ssid) {
    WiFi.mode(WIFI_AP);
    #ifdef ESP8266
        IPAddress apIP;
        apIP.fromString(AP_IP_ADDRESS);
        WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    #endif
    WiFi.softAP(ssid);
}

#endif