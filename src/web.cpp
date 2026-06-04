#include "web.h"
#include "web_templates.h"
#include "sensor.h"
#include "led.h"
#include "ansi.h"
#include "config.h"

#if DEVICE_TYPE == 1
  #include "fan.h"
#endif

#if DEVICE_TYPE == 3
  #include "switch.h"
#endif

#if MQTT_ENABLED == 1
  #include "mqtt.h"
#endif

#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
  #include <ESPAsyncWebServer.h>
  #if OTA_ENABLED == 1
    #include <ElegantOTA.h>
  #endif
  extern AsyncWebServer server;
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <DNSServer.h>
  #if OTA_ENABLED == 1
    #include <ElegantOTA.h>
  #endif
  extern ESP8266WebServer server;
  DNSServer dnsServer;
  const byte DNS_PORT = 53;
#endif

#if OTA_ENABLED == 1
  static bool otaAvailable = false;
  static bool otaInitialized = false;

  void web_setOtaAvailable(bool available) {
      otaAvailable = available;
  }

  bool web_isOtaAvailable() {
      return otaAvailable;
  }
#endif

// ========== ОБРАБОТЧИКИ ==========

#if DEVICE_TYPE == 1
void handleToggle() {
    config.sensorControlMode = false;
    fan_set(!fan_getState());
}

void handleSensorControlMode() {
    fan_setOverrideMode(true);
}
#endif

#if DEVICE_TYPE == 3
void handleToggle() {
    switch_set(!switch_getState());
}
#endif

// ========== ESP8266 ВЕРСИИ (с синхронным сервером) ==========
#ifdef ESP8266

void web_sendStatusPage(int refreshInterval) {
    String html = renderStatusPage(refreshInterval);
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");
    server.sendContent(html);
}

void web_sendConfigPage(const String& errorMsg) {
    String html = renderConfigPage(errorMsg);
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");
    server.sendContent(html);
}

void web_saveConfig() {
    // Получение параметров из запроса
    if (server.hasArg("wifiSsid"))
        server.arg("wifiSsid").toCharArray(config.wifiSsid, sizeof(config.wifiSsid));
    if (server.hasArg("wifiPassword")) {
        String pwd = server.arg("wifiPassword");
        if (pwd.length() > 0) pwd.toCharArray(config.wifiPassword, sizeof(config.wifiPassword));
    }

    #if MQTT_ENABLED == 1
    if (server.hasArg("mqttBroker"))
        server.arg("mqttBroker").toCharArray(config.mqttBroker, sizeof(config.mqttBroker));
    if (server.hasArg("mqttPort"))
        config.mqttPort = server.arg("mqttPort").toInt();
    if (server.hasArg("mqttUser"))
        server.arg("mqttUser").toCharArray(config.mqttUser, sizeof(config.mqttUser));
    if (server.hasArg("mqttPassword")) {
        String pwd = server.arg("mqttPassword");
        if (pwd.length() > 0) pwd.toCharArray(config.mqttPassword, sizeof(config.mqttPassword));
    }
    if (server.hasArg("mqttClientId")) {
        String cid = server.arg("mqttClientId");
        if (cid.length() > 0 && cid.length() < sizeof(config.mqttClientId)) {
            cid.toCharArray(config.mqttClientId, sizeof(config.mqttClientId));
        } else if (cid.length() == 0) {
            config.mqttClientId[0] = '\0';
        }
    }
    #endif

    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    if (server.hasArg("sensorInterval"))
        config.sensorInterval = server.arg("sensorInterval").toInt();
    #endif
    
    #if DEVICE_TYPE == 1
    if (server.hasArg("lowTemp"))
        config.lowTemp = server.arg("lowTemp").toFloat();
    if (server.hasArg("highTemp"))
        config.highTemp = server.arg("highTemp").toFloat();
    if (server.hasArg("lowHum"))
        config.lowHum = server.arg("lowHum").toFloat();
    if (server.hasArg("highHum"))
        config.highHum = server.arg("highHum").toFloat();
    if (server.hasArg("maxOnTime"))
        config.maxOnTime = server.arg("maxOnTime").toInt();
    if (server.hasArg("delaySeconds"))
        config.delaySeconds = server.arg("delaySeconds").toInt();
    if (server.hasArg("speedPercent"))
        config.speedPercent = server.arg("speedPercent").toInt();
    
    config.adaptiveMode = server.hasArg("adaptiveMode");
    config.bootState = server.hasArg("bootState");
    config.sensorControlMode = server.hasArg("sensorControlMode");
    #endif
    
    #if DEVICE_TYPE == 3
    if (server.hasArg("maxOnTime"))
        config.maxOnTime = server.arg("maxOnTime").toInt();
    if (server.hasArg("delaySeconds"))
        config.delaySeconds = server.arg("delaySeconds").toInt();
    config.bootState = server.hasArg("bootState");
    #endif
    
    // Валидация
    if (!config_validate()) {
        web_sendConfigPage(String(configLastError));
        return;
    }
    
    // Сохранение
    config_write();
    
    // Отправка ответа
    server.send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'></head><body><div style='text-align:center;margin-top:50px;'><h2>Настройки сохранены!</h2><p>Перезагрузка...</p></div></body></html>");
    
    delay(1000);
    ESP.restart();
}

#endif // ESP8266

// ========== ESP32 ВЕРСИИ (с асинхронным сервером) ==========
#ifdef ESP32

void web_sendStatusPage(AsyncWebServerRequest *request, int refreshInterval) {
    String html = renderStatusPage(refreshInterval);
    request->send(200, "text/html", html);
}

void web_sendConfigPage(AsyncWebServerRequest *request, const String& errorMsg) {
    String html = renderConfigPage(errorMsg);
    request->send(200, "text/html", html);
}

void web_saveConfig(AsyncWebServerRequest *request) {
    // Получение параметров из запроса
    if (request->hasParam("wifiSsid", true)) {
        const AsyncWebParameter* p = request->getParam("wifiSsid", true);
        if (p) p->value().toCharArray(config.wifiSsid, sizeof(config.wifiSsid));
    }
    if (request->hasParam("wifiPassword", true)) {
        const AsyncWebParameter* p = request->getParam("wifiPassword", true);
        if (p) {
            String pwd = p->value();
            if (pwd.length() > 0) pwd.toCharArray(config.wifiPassword, sizeof(config.wifiPassword));
        }
    }

    #if MQTT_ENABLED == 1
    if (request->hasParam("mqttBroker", true)) {
        const AsyncWebParameter* p = request->getParam("mqttBroker", true);
        if (p) p->value().toCharArray(config.mqttBroker, sizeof(config.mqttBroker));
    }
    if (request->hasParam("mqttPort", true)) {
        const AsyncWebParameter* p = request->getParam("mqttPort", true);
        if (p) config.mqttPort = p->value().toInt();
    }
    if (request->hasParam("mqttUser", true)) {
        const AsyncWebParameter* p = request->getParam("mqttUser", true);
        if (p) p->value().toCharArray(config.mqttUser, sizeof(config.mqttUser));
    }
    if (request->hasParam("mqttPassword", true)) {
        const AsyncWebParameter* p = request->getParam("mqttPassword", true);
        if (p) {
            String pwd = p->value();
            if (pwd.length() > 0) pwd.toCharArray(config.mqttPassword, sizeof(config.mqttPassword));
        }
    }
    if (request->hasParam("mqttClientId", true)) {
        const AsyncWebParameter* p = request->getParam("mqttClientId", true);
        if (p) {
            String cid = p->value();
            if (cid.length() > 0 && cid.length() < sizeof(config.mqttClientId)) {
                cid.toCharArray(config.mqttClientId, sizeof(config.mqttClientId));
            } else if (cid.length() == 0) {
                config.mqttClientId[0] = '\0';
            }
        }
    }
    #endif

    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    if (request->hasParam("sensorInterval", true)) {
        const AsyncWebParameter* p = request->getParam("sensorInterval", true);
        if (p) config.sensorInterval = p->value().toInt();
    }
    #endif
    
    #if DEVICE_TYPE == 1
    if (request->hasParam("lowTemp", true)) {
        const AsyncWebParameter* p = request->getParam("lowTemp", true);
        if (p) config.lowTemp = p->value().toFloat();
    }
    if (request->hasParam("highTemp", true)) {
        const AsyncWebParameter* p = request->getParam("highTemp", true);
        if (p) config.highTemp = p->value().toFloat();
    }
    if (request->hasParam("lowHum", true)) {
        const AsyncWebParameter* p = request->getParam("lowHum", true);
        if (p) config.lowHum = p->value().toFloat();
    }
    if (request->hasParam("highHum", true)) {
        const AsyncWebParameter* p = request->getParam("highHum", true);
        if (p) config.highHum = p->value().toFloat();
    }
    if (request->hasParam("maxOnTime", true)) {
        const AsyncWebParameter* p = request->getParam("maxOnTime", true);
        if (p) config.maxOnTime = p->value().toInt();
    }
    if (request->hasParam("delaySeconds", true)) {
        const AsyncWebParameter* p = request->getParam("delaySeconds", true);
        if (p) config.delaySeconds = p->value().toInt();
    }
    if (request->hasParam("speedPercent", true)) {
        const AsyncWebParameter* p = request->getParam("speedPercent", true);
        if (p) config.speedPercent = p->value().toInt();
    }
    
    config.adaptiveMode = request->hasParam("adaptiveMode", true);
    config.bootState = request->hasParam("bootState", true);
    config.sensorControlMode = request->hasParam("sensorControlMode", true);
    #endif
    
    #if DEVICE_TYPE == 3
    if (request->hasParam("maxOnTime", true)) {
        const AsyncWebParameter* p = request->getParam("maxOnTime", true);
        if (p) config.maxOnTime = p->value().toInt();
    }
    if (request->hasParam("delaySeconds", true)) {
        const AsyncWebParameter* p = request->getParam("delaySeconds", true);
        if (p) config.delaySeconds = p->value().toInt();
    }
    config.bootState = request->hasParam("bootState", true);
    #endif
    
    // Валидация
    if (!config_validate()) {
        web_sendConfigPage(request, String(configLastError));
        return;
    }
    
    // Сохранение
    config_write();
    
    // Отправка ответа
    request->send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'></head><body><div style='text-align:center;margin-top:50px;'><h2>Настройки сохранены!</h2><p>Перезагрузка...</p></div></body></html>");
    
    delay(1000);
    ESP.restart();
}

#endif // ESP32

// ========== ИНИЦИАЛИЗАЦИЯ (общая логика, но с платформо-зависимыми вызовами) ==========

void web_init() {
    int refreshInterval = 5;
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    refreshInterval = config.sensorInterval;
    #endif
    
    #ifdef ESP8266
    // ===== ESP8266 =====
    #if WEB_STATUS_ENABLED == 1
    server.on("/", [refreshInterval](){ web_sendStatusPage(refreshInterval); });
    #else
    server.on("/", [](){ server.sendHeader("Location", "/config", true); server.send(302, "text/plain", ""); });
    #endif
    
    server.on("/config", [](){ web_sendConfigPage(""); });
    server.on("/save", web_saveConfig);
    
    // Обработчик favicon
    server.on("/favicon.ico", [](){ server.send(404); });
    
    #if WEB_RESET_ENABLED == 1
    server.on("/resetall", [](){
        config_clear();
        server.send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'></head><body><h2>Настройки сброшены, перезагрузка...</h2></body></html>");
        delay(1000);
        ESP.restart();
    });
    #endif
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    #if DEVICE_TYPE == 1
    server.on("/fan/toggle", [](){ handleToggle(); server.sendHeader("Location", "/", true); server.send(302, "text/plain", ""); });
    server.on("/fan/auto", [](){ handleSensorControlMode(); server.sendHeader("Location", "/", true); server.send(302, "text/plain", ""); });
    #elif DEVICE_TYPE == 3
    server.on("/switch/toggle", [](){ handleToggle(); server.sendHeader("Location", "/", true); server.send(302, "text/plain", ""); });
    #endif
    #endif
    
    #elif defined(ESP32)
    // ===== ESP32 =====
    #if WEB_STATUS_ENABLED == 1
    server.on("/", HTTP_GET, [refreshInterval](AsyncWebServerRequest *request){ 
        web_sendStatusPage(request, refreshInterval); 
    });
    #else
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ request->redirect("/config"); });
    #endif
    
    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){ web_sendConfigPage(request, ""); });
    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){ web_saveConfig(request); });
    
    // Обработчик favicon
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(404);
    });
    
    #if WEB_RESET_ENABLED == 1
    server.on("/resetall", HTTP_GET, [](AsyncWebServerRequest *request){
        config_clear();
        request->send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'></head><body><h2>Настройки сброшены, перезагрузка...</h2></body></html>");
        delay(1000);
        ESP.restart();
    });
    #endif
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    #if DEVICE_TYPE == 1
    server.on("/fan/toggle", HTTP_GET, [](AsyncWebServerRequest *request){ handleToggle(); request->redirect("/"); });
    server.on("/fan/auto", HTTP_GET, [](AsyncWebServerRequest *request){ handleSensorControlMode(); request->redirect("/"); });
    #elif DEVICE_TYPE == 3
    server.on("/switch/toggle", HTTP_GET, [](AsyncWebServerRequest *request){ handleToggle(); request->redirect("/"); });
    #endif
    #endif
    
    #endif
    
    #if OTA_ENABLED == 1
    if (web_isOtaAvailable()) {
        #if defined(ESP32)
            ElegantOTA.begin(&server);
        #elif defined(ESP8266)
            if (!otaInitialized) {
                ElegantOTA.begin(&server);
                otaInitialized = true;
            }
        #endif
    }
    #endif
  
    server.begin();
  
    #if LOG_WEB == 1
    Serial.println("[WEB] Web server started");
    #endif
}

void web_initAP() {
    if (apMode) return;
  
    apMode = true;

    #if STATUS_LED_PIN > 0
      led_setMode(LED_MODE_AP_BLINK);  
    #endif

    WiFi.mode(WIFI_AP);
  
    #ifdef ESP8266
      IPAddress apIP;
      apIP.fromString(AP_IP_ADDRESS);
      WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
      WiFi.softAP(deviceId);
      dnsServer.start(DNS_PORT, "*", IPAddress(192,168,4,1));
      
      server.on("/", [](){ web_sendConfigPage(""); });
      server.on("/save", web_saveConfig);
      server.on("/favicon.ico", [](){ server.send(404); });
      
    #elif defined(ESP32)    
      WiFi.softAP(deviceId);
      
      server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ web_sendConfigPage(request, ""); });
      server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){ web_saveConfig(request); });
      server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
          request->send(404);
      });
    #endif

    #if OTA_ENABLED == 1
    if (web_isOtaAvailable()) {
        #if defined(ESP32)
            ElegantOTA.begin(&server);
        #elif defined(ESP8266)
            if (!otaInitialized) {
                ElegantOTA.begin(&server);
                otaInitialized = true;
            }
        #endif
    }
    #endif
  
    server.begin();
}

void web_update() {
    #ifdef ESP8266
      server.handleClient();   
      if (apMode) {
        dnsServer.processNextRequest();
      }
    #endif
}