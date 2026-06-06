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
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <DNSServer.h>
  DNSServer dnsServer;
  const byte DNS_PORT = 53;
#endif

#if OTA_ENABLED == 1
  #include <ElegantOTA.h>

    static bool otaAvailable = false;


    void web_setOtaAvailable(bool available) {
        otaAvailable = available;
    }

    bool web_isOtaAvailable() {
      return otaAvailable;
  }
#endif

// ========== ФОРМИРОВАНИЕ СТАТУСА ДЛЯ СТРАНИЦЫ СОСТОЯНИЯ ==========
String web_buildStatusHtml() {
    String html;
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    html += F("<div class='flex-container'>");
    
    #if DEVICE_TYPE == 1
    String tempColor = (currentTemp >= config_get()->highTemp) ? "#f44336" : 
                       ((currentTemp <= config_get()->lowTemp) ? "#4CAF50" : "#2196F3");
    String humColor = (currentHum >= config_get()->highHum) ? "#f44336" : 
                      ((currentHum <= config_get()->lowHum) ? "#4CAF50" : "#2196F3");
    #else
    String tempColor = "#2196F3";
    String humColor = "#2196F3";
    #endif
    
    // ========== ТЕМПЕРАТУРА ==========
    html += F("<div class='sensor-card' style='background:");
    html += tempColor;
    html += F("20; border:2px solid ");
    html += tempColor;
    html += F(";'>");
    
    html += F("<div class='sensor-value' style='color:");
    html += tempColor;
    html += F(";'>");
    html += String(currentTemp, 1);
    html += F(" °C");
    html += F("</div>");
    
    html += F("<div class='sensor-label'>Температура");
    #if DEVICE_TYPE == 1
    html += F(" (выкл: ");
    html += String(config_get()->lowTemp, 1);
    html += F(" вкл: ");
    html += String(config_get()->highTemp, 1);
    html += F(")");
    #endif
    html += F("</div>");
    html += F("</div>");
    
    // ========== ВЛАЖНОСТЬ ==========
    html += F("<div class='sensor-card' style='background:");
    html += humColor;
    html += F("20; border:2px solid ");
    html += humColor;
    html += F(";'>");
    
    html += F("<div class='sensor-value' style='color:");
    html += humColor;
    html += F(";'>");
    html += String(currentHum, 1);
    html += F(" %");
    html += F("</div>");
    
    html += F("<div class='sensor-label'>Влажность");
    #if DEVICE_TYPE == 1
    html += F(" (выкл: ");
    html += String(config_get()->lowHum, 1);
    html += F(" вкл: ");
    html += String(config_get()->highHum, 1);
    html += F(")");
    #endif
    html += F("</div>");
    html += F("</div>");
    
    html += F("</div>");
    
    if (!sensorOk && strlen(sensorError) > 0) {
        html += F("<div class='sensor-error'><strong>Ошибка датчика</strong><br>");
        html += sensorError;
        html += F("</div>");
    }
    #endif
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    #if DEVICE_TYPE == 1
    bool state = fan_getState();
    const char* label = "Вентилятор";
    const char* toggleUrl = "/fan/toggle";
    #else
    bool state = switch_getState();
    const char* label = "Выключатель";
    const char* toggleUrl = "/switch/toggle";
    #endif
    
    String stateColor = state ? "#f44336" : "#2196F3";
    String stateText = state ? "ВКЛ" : "ВЫКЛ";
    
    html += F("<a href='");
    html += toggleUrl;
    html += F("'>");
    html += F("<div class='status-card' style='background:");
    html += stateColor;
    html += F("20; border:2px solid ");
    html += stateColor;
    html += F(";'>");
    html += F("<div style='font-size:2em;font-weight:bold;color:");
    html += stateColor;
    html += F(";'>");
    html += label;
    html += F(": ");
    html += stateText;
    html += F("</div>");
    html += F("</div>");
    html += F("</a>");
    
    #if DEVICE_TYPE == 1
    if (state) {
        int currentSpeed = config_get()->speedPercent;
        html += F("<div class='status-card' style='background:#2196F320; border:2px solid #2196F3;'>");
        html += F("<div style='font-size:1.2em;font-weight:bold;'>Скорость: ");
        html += String(currentSpeed);
        html += F("%</div>");
        html += F("<div class='duty-bar'><div class='duty-fill' style='width:");
        html += String(currentSpeed);
        html += F("%;'></div></div>");
        if (config_get()->speedPercent < 100) {
            html += F("<div style='font-size:0.9em;color:#555;'>Тихий режим активен");
            if (config_get()->adaptiveMode) html += F(" + адаптация");
            html += F("</div>");
        }
        html += F("</div>");
    }
    
    String modeText = config_get()->sensorControlMode ? "УПРАВЛЕНИЕ СЕНСОРОМ" : "РУЧНОЙ";
    String modeColor = config_get()->sensorControlMode ? "#4CAF50" : "#f44336";
    
    html += F("<div class='status-card' style='background:");
    html += modeColor;
    html += F("20; border:2px solid ");
    html += modeColor;
    html += F(";'>");
    html += F("<div style='font-size:1.5em;font-weight:bold;color:");
    html += modeColor;
    html += F(";'>Режим: ");
    html += modeText;
    html += F("</div>");
    html += F("</div>");
    #endif
    #endif
    
    html += F("<hr><div class='info'>");
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    html += F("Опрос датчика ");
    html += String(config_get()->sensorInterval);
    html += F(" сек<br>");
    #endif
    
    #if MQTT_ENABLED == 1
    html += F("MQTT: ");
    html += mqttManager.isConnected() ? F("подключен") : F("отключен");
    html += F("<br>");
    #endif
    
    #if WEB_SHOW_RSSI == 1
    html += F("RSSI: ");
    html += String(WiFi.RSSI());
    html += F(" dBm<br>");
    #endif
    
    html += F("</div>");
    
    #if DEVICE_TYPE == 1
    if (!config_get()->sensorControlMode && sensorOk) {
        html += F("<div class='button-group' style='margin-top:10px;'>");
        html += F("<a href='/fan/auto'><button>Режим управления сенсором</button></a>");
        html += F("</div>");
    }
    #endif
    
    return html;
}

// ========== ОБРАБОТЧИКИ ==========

#if DEVICE_TYPE == 1
void handleToggle() {
    config_setSensorControlMode(false);
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

// ========== УНИФИЦИРОВАННАЯ РЕАЛИЗАЦИЯ (ДЛЯ ESP32 И ESP8266) ==========

void web_sendStatusPage(int refreshInterval) {
    String statusHtml = web_buildStatusHtml();
    
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");
    
    auto send = [&](const String& chunk) {
        server.sendContent(chunk);
    };
    
    send(FPSTR(HTML_PAGE_START));
    
    if (refreshInterval > 0) {
        char refresh[64];
        snprintf_P(refresh, sizeof(refresh), PSTR("<meta http-equiv='refresh' content='%d'>"), refreshInterval);
        send(refresh);
    } else {
        send(F("{META_REFRESH}"));
    }
    
    send(F("<title>"));
    send(deviceId);
    send(F("</title>"));
    send(FPSTR(HTML_STYLE));
    send(F("</head><body><div class='container'>"));
    send(F("<h1>"));
    send(deviceId);
    send(F(" VERSION "));
    send(VERSION);
    send(F("</h1>"));
    
    send(statusHtml);
    
    send(F("<div class='button-group'><a href='/config'><button>Настройки</button></a></div>"));
    
    send(FPSTR(HTML_PAGE_END));
}

void web_sendConfigPage(const String& errorMsg, const String& successMsg) {
    Config savedConfig = config_getSaved();
    String currentMode = apMode ? F("Точка доступа (AP)") : F("Клиент WiFi");
    String currentSsid = apMode ? String(deviceId) : String(savedConfig.wifiSsid);
    String currentIp = apMode ? String(AP_IP_ADDRESS) : WiFi.localIP().toString();
    
    int refreshSeconds = (successMsg.length() > 0) ? 5 : 0;
    
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");
    
    auto send = [&](const String& chunk) {
        server.sendContent(chunk);
    };
    
    sendConfigPage(send, errorMsg, successMsg, savedConfig, currentMode, currentSsid, currentIp, refreshSeconds);
}

void web_saveConfig() {
    if (server.hasArg("wifiSsid")) {
        config_setWifiSsid(server.arg("wifiSsid").c_str());
    }

    if (server.hasArg("wifiPassword")) {
        String pwd = server.arg("wifiPassword");
        if (pwd.length() > 0) {
            config_setWifiPassword(pwd.c_str());
        }
        // Если пусто — пароль не меняем (сеттер сам обработает)
    }

    #if MQTT_ENABLED == 1
    if (server.hasArg("mqttBroker")) {
        config_setMqttBroker(server.arg("mqttBroker").c_str());
    }
    if (server.hasArg("mqttPort")) {
        config_setMqttPort(server.arg("mqttPort").toInt());
    }
    if (server.hasArg("mqttUser")) {
        config_setMqttUser(server.arg("mqttUser").c_str());
    }
    if (server.hasArg("mqttPassword")) {
        String pwd = server.arg("mqttPassword");
        if (pwd.length() > 0) {
            config_setMqttPassword(pwd.c_str());
        }
    }
    if (server.hasArg("mqttClientId")) {
        String cid = server.arg("mqttClientId");
        if (cid.length() > 0 && cid.length() < sizeof(config_get()->mqttClientId)) {
            config_setMqttClientId(cid.c_str());
        }
    }
    #endif

    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    if (server.hasArg("sensorInterval")) {
        config_setSensorInterval(server.arg("sensorInterval").toInt());
    }
    #endif
    
    #if DEVICE_TYPE == 1
    if (server.hasArg("lowTemp")) {
        config_setLowTemp(server.arg("lowTemp").toFloat());
    }
    if (server.hasArg("highTemp")) {
        config_setHighTemp(server.arg("highTemp").toFloat());
    }
    if (server.hasArg("lowHum")) {
        config_setLowHum(server.arg("lowHum").toFloat());
    }
    if (server.hasArg("highHum")) {
        config_setHighHum(server.arg("highHum").toFloat());
    }
    if (server.hasArg("maxOnTime")) {
        config_setMaxOnTime(server.arg("maxOnTime").toInt());
    }
    if (server.hasArg("delaySeconds")) {
        config_setDelaySeconds(server.arg("delaySeconds").toInt());
    }
    if (server.hasArg("speedPercent")) {
        config_setSpeedPercent(server.arg("speedPercent").toInt());
    }
    
    config_setAdaptiveMode(server.hasArg("adaptiveMode"));
    config_setBootState(server.hasArg("bootState"));
    config_setSensorControlMode(server.hasArg("sensorControlMode"));
    #endif
    
    #if DEVICE_TYPE == 3
    if (server.hasArg("maxOnTime")) {
        config_setMaxOnTime(server.arg("maxOnTime").toInt());
    }
    if (server.hasArg("delaySeconds")) {
        config_setDelaySeconds(server.arg("delaySeconds").toInt());
    }
    config_setBootState(server.hasArg("bootState"));
    #endif
    
    if (!config_validate()) {
        web_sendConfigPage(String(config_getLastError()), "");
        return;
    }
    
    if (!config_write()) {
        web_sendConfigPage("Ошибка записи во Flash. Пожалуйста, попробуйте ещё раз.", "");
        return;
    }
    
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset='UTF-8'>
    <meta http-equiv='refresh' content='2;url=/'>
    <title>Сохранение</title>
    <style>
        body{font-family:Arial;text-align:center;margin-top:50px;background:#f0f0f0;}
        .success{color:#2e7d32;background:#e8f5e9;padding:20px;border-radius:10px;display:inline-block;}
    </style>
</head>
<body>
    <div class='success'>
        <h2>✓ Настройки сохранены</h2>
        <p>Перезагрузка устройства...</p>
    </div>
</body>
</html>
)rawliteral";
    
    server.send(200, "text/html", html);
    delay(2000);
    ESP.restart();
}

void web_init() {
    int refreshInterval = 5;
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    refreshInterval = config_get()->sensorInterval;
    #endif
    
    #if WEB_STATUS_ENABLED == 1
    server.on("/", [refreshInterval](){ web_sendStatusPage(refreshInterval); });
    #else
    server.on("/", [](){ server.sendHeader("Location", "/config", true); server.send(302, "text/plain", ""); });
    #endif
    
    server.on("/config", [](){ web_sendConfigPage("", ""); });
    server.on("/save", web_saveConfig);
    server.on("/favicon.ico", [](){ server.send(404); });
    
    #if WEB_RESET_ENABLED == 1
    server.on("/resetall", [](){
        config_clear();
        server.send(200, "text/html", F("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'></head><body><h2>Настройки сброшены, перезагрузка...</h2></body></html>"));
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
    
    #if OTA_ENABLED == 1
    if (web_isOtaAvailable()) {
        ElegantOTA.begin(&server);
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
      
      server.on("/", [](){ web_sendConfigPage("", ""); });
      server.on("/save", web_saveConfig);
      server.on("/favicon.ico", [](){ server.send(404); });
      
    #elif defined(ESP32)    
      WiFi.softAP(deviceId);
      
      server.on("/", [](){ web_sendConfigPage("", ""); });
      server.on("/save", web_saveConfig);
      server.on("/favicon.ico", [](){ server.send(404); });
    #endif

    #if OTA_ENABLED == 1
    if (web_isOtaAvailable()) {
        ElegantOTA.begin(&server);
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
    #elif defined(ESP32)
      server.handleClient();
    #endif
}