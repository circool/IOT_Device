// ===== ФАЙЛ: web.cpp (ПОЛНОСТЬЮ, РАБОЧАЯ ВЕРСИЯ) =====

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

// ========== ФОРМИРОВАНИЕ СТАТУСА ДЛЯ СТРАНИЦЫ СОСТОЯНИЯ ==========
String web_buildStatusHtml() {
    String html;
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    html += F("<div class='flex-container'>");
    
    #if DEVICE_TYPE == 1
    String tempColor = (currentTemp >= config.highTemp) ? "#f44336" : 
                       ((currentTemp <= config.lowTemp) ? "#4CAF50" : "#2196F3");
    String humColor = (currentHum >= config.highHum) ? "#f44336" : 
                      ((currentHum <= config.lowHum) ? "#4CAF50" : "#2196F3");
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
    html += String(config.lowTemp, 1);
    html += F(" вкл: ");
    html += String(config.highTemp, 1);
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
    html += String(config.lowHum, 1);
    html += F(" вкл: ");
    html += String(config.highHum, 1);
    html += F(")");
    #endif
    html += F("</div>");
    html += F("</div>");
    
    html += F("</div>"); // закрываем flex-container
    
    if (!sensorOk && strlen(sensorError) > 0) {
        html += F("<div class='sensor-error'><strong>Ошибка датчика</strong><br>");
        html += sensorError;
        html += F("</div>");
    }
    #endif // DEVICE_TYPE == 1 || 2
    
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
        int currentSpeed = config.speedPercent;
        html += F("<div class='status-card' style='background:#2196F320; border:2px solid #2196F3;'>");
        html += F("<div style='font-size:1.2em;font-weight:bold;'>Скорость: ");
        html += String(currentSpeed);
        html += F("%</div>");
        html += F("<div class='duty-bar'><div class='duty-fill' style='width:");
        html += String(currentSpeed);
        html += F("%;'></div></div>");
        if (config.speedPercent < 100) {
            html += F("<div style='font-size:0.9em;color:#555;'>Тихий режим активен");
            if (config.adaptiveMode) html += F(" + адаптация");
            html += F("</div>");
        }
        html += F("</div>");
    }
    
    String modeText = config.sensorControlMode ? "УПРАВЛЕНИЕ СЕНСОРОМ" : "РУЧНОЙ";
    String modeColor = config.sensorControlMode ? "#4CAF50" : "#f44336";
    
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
    #endif // DEVICE_TYPE == 1
    #endif // DEVICE_TYPE == 1 || 3
    
    html += F("<hr><div class='info'>");
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    html += F("Опрос датчика ");
    html += String(config.sensorInterval);
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
    if (!config.sensorControlMode && sensorOk) {
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

// ========== ESP8266 РЕАЛИЗАЦИЯ (потоковая отправка) ==========
#ifdef ESP8266

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
    
    send(F("<h1>"));
    send(deviceId);
    send(F(" VERSION "));
    send(VERSION);
    send(F("</h1>"));
    
    send(statusHtml);
    
    send(F("<div class='button-group'><a href='/config'><button>Настройки</button></a></div>"));
    
    send(FPSTR(HTML_PAGE_END));
}

void web_sendConfigPage(const String& errorMsg) {
    Config savedConfig = config_getSaved();
    String currentMode = apMode ? F("Точка доступа (AP)") : F("Клиент WiFi");
    String currentSsid = apMode ? String(deviceId) : String(savedConfig.wifiSsid);
    String currentIp = apMode ? String(AP_IP_ADDRESS) : WiFi.localIP().toString();
    
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");
    
    auto send = [&](const String& chunk) {
        server.sendContent(chunk);
    };
    
    send(FPSTR(HTML_PAGE_START));
    send(F("{META_REFRESH}"));
    
    send(F("<title>"));
    send(deviceId);
    send(F(" Configuration</title>"));
    
    send(F("<h1>Настройка устройства "));
    send(deviceId);
    send(F(" v. "));
    send(VERSION);
    send(F("</h1>"));
    
    send(F("<h3>Текущее состояние</h3><div class='info'>"));
    send(F("Режим: <strong>")); send(currentMode); send(F("</strong><br>"));
    send(F("SSID: <strong>")); send(currentSsid); send(F("</strong><br>"));
    send(F("IP адрес: <strong>")); send(currentIp); send(F("</strong><br>"));
    send(F("</div>"));
    
    if (errorMsg.length() > 0) {
        send(F("<div class='error'><strong>Ошибка:</strong> "));
        send(errorMsg);
        send(F("</div>"));
    }
    
    send(F("<form method='POST' action='/save'>"));
    send(F("<h3>Настройки сети</h3>"));
    send(F("<label>WiFi SSID:</label>"));
    send(F("<input type='text' name='wifiSsid' required value='"));
    send(savedConfig.wifiSsid);
    send(F("'>"));
    
    send(F("<label>WiFi Password:</label>"));
    send(F("<input type='password' name='wifiPassword' placeholder='(не показан)'>"));
    send(F("<div class='password-hint'>Оставьте пустым, чтобы сохранить текущий пароль</div>"));
    
    #if MQTT_ENABLED == 1
    send(F("<h3>MQTT настройки</h3>"));
    send(F("<div class='row'><div><label>MQTT Broker:</label>"));
    send(F("<input type='text' name='mqttBroker' required value='"));
    send(savedConfig.mqttBroker);
    send(F("'></div>"));
    
    send(F("<div><label>MQTT Port:</label>"));
    send(F("<input type='number' name='mqttPort' required value='"));
    send(String(savedConfig.mqttPort));
    send(F("'></div></div>"));
    
    send(F("<div class='row'><div><label>MQTT User:</label>"));
    send(F("<input type='text' name='mqttUser' value='"));
    send(savedConfig.mqttUser);
    send(F("'></div>"));
    
    send(F("<div><label>MQTT Password:</label>"));
    send(F("<input type='password' name='mqttPassword' placeholder='(не показан)'></div></div>"));
    send(F("<div class='password-hint'>Оставьте пустым, чтобы сохранить текущий пароль</div>"));
    
    send(F("<label>MQTT Client ID:</label>"));
    send(F("<input type='text' name='mqttClientId' required value='"));
    send(savedConfig.mqttClientId);
    send(F("'>"));
    #endif
    
    #if DEVICE_TYPE == 1
    send(F("<h3>Настройки датчиков</h3>"));
    
    send(F("<div class='row'><div><label>Low Temp (°C):</label>"));
    send(F("<input type='number' step='0.1' name='lowTemp' required value='"));
    send(String(savedConfig.lowTemp));
    send(F("'></div>"));
    
    send(F("<div><label>High Temp (°C):</label>"));
    send(F("<input type='number' step='0.1' name='highTemp' required value='"));
    send(String(savedConfig.highTemp));
    send(F("'></div></div>"));
    
    send(F("<div class='row'><div><label>Low Hum (%):</label>"));
    send(F("<input type='number' step='0.1' name='lowHum' required value='"));
    send(String(savedConfig.lowHum));
    send(F("'></div>"));
    
    send(F("<div><label>High Hum (%):</label>"));
    send(F("<input type='number' step='0.1' name='highHum' required value='"));
    send(String(savedConfig.highHum));
    send(F("'></div></div>"));
    
    send(F("<div class='row'><div><label>Интервал опроса датчика (сек)</label>"));
    send(F("<input type='number' name='sensorInterval' required value='"));
    send(String(savedConfig.sensorInterval));
    send(F("'></div>"));
    
    send(F("<div><label>Аварийное отключение через </label>"));
    send(F("<input type='number' name='maxOnTime' min='0' required value='"));
    send(String(savedConfig.maxOnTime));
    send(F("'></div></div>"));
    
    send(F("<h3>Управление</h3>"));
    send(F("<label>Принудительно включить через </label>"));
    send(F("<input type='number' name='delaySeconds' required value='"));
    send(String(savedConfig.delaySeconds));
    send(F("'> сек"));
    
    send(F("<h3>Тихий режим (ШИМ)</h3>"));
    send(F("<label>Скорость (0-100%):</label>"));
    send(F("<input type='number' name='speedPercent' min='0' max='100' required value='"));
    send(String(savedConfig.speedPercent));
    send(F("'>"));
    send(F("<div class='note'>0% - выключено, 100% - полная мощность (тихий режим выключен).<br>При значении ниже 100% вентилятор работает тише.</div>"));
    
    send(F("<h3>Адаптивный тихий режим</h3>"));
    send(F("<label><input type='checkbox' name='adaptiveMode' value='1'"));
    if (savedConfig.adaptiveMode) send(F(" checked"));
    send(F("> Включить адаптацию</label>"));
    send(F("<div class='note'>Адаптивный режим автоматически регулирует скорость для поддержания температуры и влажности на уровне, зафиксированном при включении вентилятора.</div>"));
    
    send(F("<h3>Поведение при старте</h3>"));
    send(F("<label><input type='checkbox' name='bootState' value='1'"));
    if (savedConfig.bootState) send(F(" checked"));
    send(F("> Включать при старте</label>"));
    send(F("<div class='note'>При включенной опции вентилятор будет включен сразу после подачи питания.</div>"));
    
    send(F("<h3>Режимы работы</h3>"));
    send(F("<label><input type='checkbox' name='sensorControlMode' value='1'"));
    if (savedConfig.sensorControlMode) send(F(" checked"));
    send(F("> Режим управления сенсором</label>"));
    send(F("<div class='note'>При включённом режиме вентилятор управляется по показаниям датчиков температуры и влажности. При выключении — только вручную.</div>"));
    
    #elif DEVICE_TYPE == 2
    send(F("<h3>Настройки датчиков</h3>"));
    send(F("<label>Интервал опроса датчика (сек)</label>"));
    send(F("<input type='number' name='sensorInterval' required value='"));
    send(String(savedConfig.sensorInterval));
    send(F("'>"));
    
    #elif DEVICE_TYPE == 3
    send(F("<h3>Управление</h3>"));
    send(F("<label>Принудительно включить через </label>"));
    send(F("<input type='number' name='delaySeconds' required value='"));
    send(String(savedConfig.delaySeconds));
    send(F("'> сек<br>"));
    
    send(F("<label>Аварийное отключение через </label>"));
    send(F("<input type='number' name='maxOnTime' min='0' required value='"));
    send(String(savedConfig.maxOnTime));
    send(F("'> сек"));
    
    send(F("<h3>Поведение при старте</h3>"));
    send(F("<label><input type='checkbox' name='bootState' value='1'"));
    if (savedConfig.bootState) send(F(" checked"));
    send(F("> Включать при старте</label>"));
    send(F("<div class='note'>При включенной опции выключатель будет включен сразу после подачи питания.</div>"));
    #endif
    
    send(F("<label><input type='checkbox' name='confirmSave' required> Подтвердить сохранение</label>"));
    send(F("<input type='submit' value='Сохранить и перезагрузить'>"));
    send(F("</form>"));
    
    #if OTA_ENABLED == 1
    if (web_isOtaAvailable()) {
        send(F("<a href='/update' class='link-btn'>Обновить прошивку (OTA)</a>"));
    } else {
        send(F("<div class='warning'>OTA недоступно: недостаточно Flash памяти (требуется 2MB)</div>"));
    }
    #endif
    
    send(F("<a href='/' class='link-btn'>Домой</a>"));
    send(FPSTR(HTML_PAGE_END));
}

void web_saveConfig() {
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
    
    if (!config_validate()) {
        web_sendConfigPage(String(configLastError));
        return;
    }
    
    config_write();
    
    server.send(200, "text/html", F("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'></head><body><div style='text-align:center;margin-top:50px;'><h2>Настройки сохранены!</h2><p>Перезагрузка...</p></div></body></html>"));
    
    delay(1000);
    ESP.restart();
}

#endif // ESP8266

#ifdef ESP32

void web_sendStatusPage(AsyncWebServerRequest *request, int refreshInterval) {
    String statusHtml = web_buildStatusHtml();
    String html = renderStatusPage(refreshInterval, statusHtml);  // используем функцию из web_templates.h
    request->send(200, "text/html", html);
}

void web_sendConfigPage(AsyncWebServerRequest *request, const String& errorMsg) {
    Config savedConfig = config_getSaved();
    String currentMode = apMode ? F("Точка доступа (AP)") : F("Клиент WiFi");
    String currentSsid = apMode ? String(deviceId) : String(savedConfig.wifiSsid);
    String currentIp = apMode ? String(AP_IP_ADDRESS) : WiFi.localIP().toString();
    
    String html = renderConfigPage(errorMsg, savedConfig, currentMode, currentSsid, currentIp);  // используем функцию из web_templates.h
    request->send(200, "text/html", html);
}
void web_saveConfig(AsyncWebServerRequest *request) {
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
    
    if (!config_validate()) {
        web_sendConfigPage(request, String(configLastError));
        return;
    }
    
    config_write();
    
    request->send(200, "text/html", F("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'></head><body><div style='text-align:center;margin-top:50px;'><h2>Настройки сохранены!</h2><p>Перезагрузка...</p></div></body></html>"));
    
    delay(1000);
    ESP.restart();
}

#endif // ESP32

// ========== ИНИЦИАЛИЗАЦИЯ ==========

void web_init() {
    int refreshInterval = 5;
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    refreshInterval = config.sensorInterval;
    #endif
    
    #ifdef ESP8266
    #if WEB_STATUS_ENABLED == 1
    server.on("/", [refreshInterval](){ web_sendStatusPage(refreshInterval); });
    #else
    server.on("/", [](){ server.sendHeader("Location", "/config", true); server.send(302, "text/plain", ""); });
    #endif
    
    server.on("/config", [](){ web_sendConfigPage(""); });
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
    
    #elif defined(ESP32)
    #if WEB_STATUS_ENABLED == 1
    server.on("/", HTTP_GET, [refreshInterval](AsyncWebServerRequest *request){ 
        web_sendStatusPage(request, refreshInterval); 
    });
    #else
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ request->redirect("/config"); });
    #endif
    
    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){ web_sendConfigPage(request, ""); });
    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){ web_saveConfig(request); });
    server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(404);
    });
    
    #if WEB_RESET_ENABLED == 1
    server.on("/resetall", HTTP_GET, [](AsyncWebServerRequest *request){
        config_clear();
        request->send(200, "text/html", F("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'></head><body><h2>Настройки сброшены, перезагрузка...</h2></body></html>"));
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