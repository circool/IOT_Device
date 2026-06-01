#include "web.h"
#include "web_strings.h"
#include "sensor.h"
#include "led.h"

#if DEVICE_TYPE == 1
  #include "fan.h"
#endif

#if DEVICE_TYPE == 3
  #include "switch.h"
#endif

#include "config.h"
#if MQTT_ENABLED == 1
  #include "mqtt.h"
#endif

#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
  #include <ESPAsyncWebServer.h>
  #include <esp_chip_info.h>
  #include <esp_flash.h>
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
  #include <Esp.h>
  extern ESP8266WebServer server;
  
  DNSServer dnsServer;
  const byte DNS_PORT = 53;
#endif

#if OTA_ENABLED == 1
  static bool otaInitialized = false;
#endif

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

// ========== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ДЛЯ ОТПРАВКИ HTML ==========

#ifdef ESP8266
static void web_sendChunk_P(PGM_P chunk) {
    server.sendContent(FPSTR(chunk));
}

static void web_sendFormatted(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    server.sendContent(buffer);
}
#endif



// ========== СТРАНИЦА КОНФИГУРАЦИИ ==========

#ifdef ESP8266
void web_sendConfigPage(const String& errorMsg) {
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");
    
    web_sendChunk_P(HTML_DOCTYPE);
    web_sendChunk_P(HTML_VIEWPORT);
    
    web_sendChunk_P("<title>");
    #if DEVICE_TYPE == 1
    web_sendChunk_P("Fan");
    #elif DEVICE_TYPE == 2
    web_sendChunk_P("Sensor");
    #elif DEVICE_TYPE == 3
    web_sendChunk_P("Switch");
    #else
    web_sendChunk_P("Device");
    #endif
    web_sendChunk_P(" Configuration</title>");
    
    web_sendChunk_P(HTML_STYLE);
    web_sendChunk_P(HTML_CONTAINER_OPEN);
    
    web_sendFormatted("<h1>Настройка устройства %s v. %s</h1>", deviceId, VERSION);
    
    web_sendChunk_P("<h3>Текущее состояние</h3>");
    web_sendChunk_P(STATUS_INFO_OPEN);
    
    #ifdef ESP32
    web_sendChunk_P("Platform: <strong>ESP32</strong><br>");
    #elif defined(ESP8266)
    web_sendChunk_P("Platform: <strong>ESP8266</strong><br>");
    #endif
    
    if (apMode) {
        web_sendChunk_P(STATUS_MODE_AP);
        web_sendFormatted("SSID: <strong>%s</strong><br>", deviceId);
        web_sendFormatted("IP адрес: <strong>%s</strong><br>", AP_IP_ADDRESS);
    } else {
        web_sendChunk_P(STATUS_MODE_CLIENT);
        web_sendFormatted("SSID: <strong>%s</strong><br>", staticConfig.wifiSsid);
        web_sendFormatted("IP адрес: <strong>%s</strong><br>", WiFi.localIP().toString().c_str());
    }
    
    web_sendChunk_P(STATUS_INFO_CLOSE);
    web_sendChunk_P(HTML_FORM_OPEN);
    
    if (errorMsg.length() > 0) {
        web_sendFormatted("<div class='error'><strong>Ошибка:</strong> %s</div>", errorMsg.c_str());
    }
    
    web_sendChunk_P(HTML_SECTION_NETWORK);
    web_sendChunk_P(LABEL_WIFI_SSID);
    web_sendFormatted("<input type='text' name='wifiSsid' required value='%s'>", staticConfig.wifiSsid);
    web_sendChunk_P(LABEL_WIFI_PASSWORD);
    web_sendChunk_P("<input type='password' name='wifiPassword' placeholder='(не показан)'>");
    web_sendChunk_P(HINT_PASSWORD);
    
    #if MQTT_ENABLED == 1
    web_sendChunk_P(HTML_SECTION_MQTT);
    web_sendChunk_P("<div class='row'><div>");
    web_sendChunk_P(LABEL_MQTT_BROKER);
    web_sendFormatted("<input type='text' name='mqttBroker' required value='%s'></div>", staticConfig.mqttBroker);
    web_sendChunk_P("<div>");
    web_sendChunk_P(LABEL_MQTT_PORT);
    web_sendFormatted("<input type='number' name='mqttPort' required value='%d'></div></div>", staticConfig.mqttPort);
    
    web_sendChunk_P("<div class='row'><div>");
    web_sendChunk_P(LABEL_MQTT_USER);
    web_sendFormatted("<input type='text' name='mqttUser' value='%s'></div>", staticConfig.mqttUser);
    web_sendChunk_P("<div>");
    web_sendChunk_P(LABEL_MQTT_PASSWORD);
    web_sendChunk_P("<input type='password' name='mqttPassword' placeholder='(не показан)'></div></div>");
    web_sendChunk_P(HINT_PASSWORD);
    
    web_sendChunk_P(LABEL_MQTT_CLIENT_ID);
    web_sendFormatted("<input type='text' name='mqttClientId' required value='%s'>", staticConfig.mqttClientId);
    #endif
    
    #if DEVICE_TYPE == 1
    web_sendChunk_P(HTML_SECTION_SENSOR);
    web_sendChunk_P("<div class='row'><div>");
    web_sendChunk_P(LABEL_LOW_TEMP);
    web_sendFormatted("<input type='number' step='0.1' name='lowTemp' required value='%.1f'></div>", staticConfig.lowTemp);
    web_sendChunk_P("<div>");
    web_sendChunk_P(LABEL_HIGH_TEMP);
    web_sendFormatted("<input type='number' step='0.1' name='highTemp' required value='%.1f'></div></div>", staticConfig.highTemp);
    
    web_sendChunk_P("<div class='row'><div>");
    web_sendChunk_P(LABEL_LOW_HUM);
    web_sendFormatted("<input type='number' step='0.1' name='lowHum' required value='%.1f'></div>", staticConfig.lowHum);
    web_sendChunk_P("<div>");
    web_sendChunk_P(LABEL_HIGH_HUM);
    web_sendFormatted("<input type='number' step='0.1' name='highHum' required value='%.1f'></div></div>", staticConfig.highHum);
    
    web_sendChunk_P("<div class='row'><div>");
    web_sendChunk_P(LABEL_SENSOR_INTERVAL);
    web_sendFormatted("<input type='number' name='sensorInterval' required value='%d'></div>", staticConfig.sensorInterval);
    web_sendChunk_P("<div>");
    web_sendChunk_P(LABEL_MAX_ON_TIME);
    web_sendFormatted("<input type='number' name='maxOnTime' min='0' required value='%u'></div></div>", staticConfig.maxOnTime);
    
    web_sendChunk_P(HTML_SECTION_CONTROL);
    web_sendChunk_P(LABEL_DELAY_SECONDS);
    web_sendFormatted("<input type='number' name='delaySeconds' required value='%d'>", staticConfig.delaySeconds);
    
    web_sendChunk_P(HTML_SECTION_SILENT);
    web_sendChunk_P(LABEL_SPEED_PERCENT);
    web_sendFormatted("<input type='number' name='speedPercent' min='0' max='100' required value='%d'>", staticConfig.speedPercent);
    web_sendChunk_P(NOTE_SILENT_MODE);
    
    web_sendChunk_P(HTML_SECTION_ADAPTIVE);
    web_sendChunk_P(LABEL_ADAPTIVE_MODE);
    if (staticConfig.adaptiveMode) {
        web_sendChunk_P("<script>document.querySelector('input[name=\"adaptiveMode\"]').checked=true;</script>");
    }
    web_sendChunk_P(NOTE_ADAPTIVE);
    
    web_sendChunk_P(HTML_SECTION_BOOT);
    web_sendChunk_P(LABEL_BOOT_STATE);
    if (staticConfig.bootState) {
        web_sendChunk_P("<script>document.querySelector('input[name=\"bootState\"]').checked=true;</script>");
    }
    web_sendChunk_P(NOTE_BOOT);
    
    web_sendChunk_P(HTML_SECTION_MODES);
    web_sendChunk_P(LABEL_SENSOR_CONTROL_MODE);
    if (staticConfig.sensorControlMode) {
        web_sendChunk_P("<script>document.querySelector('input[name=\"sensorControlMode\"]').checked=true;</script>");
    }
    web_sendChunk_P(NOTE_SENSOR_CONTROL);
    #endif
    
    #if DEVICE_TYPE == 2
    web_sendChunk_P(HTML_SECTION_SENSOR);
    web_sendChunk_P(LABEL_SENSOR_INTERVAL);
    web_sendFormatted("<input type='number' name='sensorInterval' required value='%d'>", staticConfig.sensorInterval);
    #endif
    
    #if DEVICE_TYPE == 3
    web_sendChunk_P(HTML_SECTION_CONTROL);
    web_sendChunk_P(LABEL_DELAY_SECONDS);
    web_sendFormatted("<input type='number' name='delaySeconds' required value='%d'>", staticConfig.delaySeconds);
    web_sendChunk_P(LABEL_MAX_ON_TIME);
    web_sendFormatted("<input type='number' name='maxOnTime' min='0' required value='%u'>", staticConfig.maxOnTime);
    
    web_sendChunk_P(HTML_SECTION_BOOT);
    web_sendChunk_P(LABEL_BOOT_STATE);
    if (staticConfig.bootState) {
        web_sendChunk_P("<script>document.querySelector('input[name=\"bootState\"]').checked=true;</script>");
    }
    web_sendChunk_P(NOTE_BOOT);
    #endif
    
    web_sendChunk_P(HTML_CHECKBOX_CONFIRM);
    web_sendChunk_P(HTML_SUBMIT_BUTTON);
    web_sendChunk_P(HTML_FORM_CLOSE);
    
    #if OTA_ENABLED == 1
    web_sendChunk_P("<a href='/update' class='link-btn'>Обновить прошивку (OTA)</a>");
    #endif
    
    web_sendChunk_P("<a href='/' class='link-btn'>Домой</a>");
    web_sendChunk_P(HTML_CONTAINER_CLOSE);
}

#else // ESP32

void web_sendConfigPage(AsyncWebServerRequest *request, const String& errorMsg) {
    String html;
    html.reserve(4096);
    
    html += FPSTR(HTML_DOCTYPE);
    html += FPSTR(HTML_VIEWPORT);
    
    html += "<title>";
    #if DEVICE_TYPE == 1
    html += "Fan";
    #elif DEVICE_TYPE == 2
    html += "Sensor";
    #elif DEVICE_TYPE == 3
    html += "Switch";
    #else
    html += "Device";
    #endif
    html += " Configuration</title>";
    
    html += FPSTR(HTML_STYLE);
    html += FPSTR(HTML_CONTAINER_OPEN);
    
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "<h1>Настройка устройства %s v. %s</h1>", deviceId, VERSION);
    html += buffer;
    
    html += "<h3>Текущее состояние</h3>";
    html += FPSTR(STATUS_INFO_OPEN);
    
    #ifdef ESP32
    html += "Platform: <strong>ESP32</strong><br>";
    #endif
    
    if (apMode) {
        html += FPSTR(STATUS_MODE_AP);
        snprintf(buffer, sizeof(buffer), "SSID: <strong>%s</strong><br>", deviceId);
        html += buffer;
        snprintf(buffer, sizeof(buffer), "IP адрес: <strong>%s</strong><br>", AP_IP_ADDRESS);
        html += buffer;
    } else {
        html += FPSTR(STATUS_MODE_CLIENT);
        snprintf(buffer, sizeof(buffer), "SSID: <strong>%s</strong><br>", staticConfig.wifiSsid);
        html += buffer;
        snprintf(buffer, sizeof(buffer), "IP адрес: <strong>%s</strong><br>", WiFi.localIP().toString().c_str());
        html += buffer;
    }
    
    html += FPSTR(STATUS_INFO_CLOSE);
    html += FPSTR(HTML_FORM_OPEN);
    
    if (errorMsg.length() > 0) {
        snprintf(buffer, sizeof(buffer), "<div class='error'><strong>Ошибка:</strong> %s</div>", errorMsg.c_str());
        html += buffer;
    }
    
    html += FPSTR(HTML_SECTION_NETWORK);
    html += FPSTR(LABEL_WIFI_SSID);
    snprintf(buffer, sizeof(buffer), "<input type='text' name='wifiSsid' required value='%s'>", staticConfig.wifiSsid);
    html += buffer;
    html += FPSTR(LABEL_WIFI_PASSWORD);
    html += "<input type='password' name='wifiPassword' placeholder='(не показан)'>";
    html += FPSTR(HINT_PASSWORD);
    
    #if MQTT_ENABLED == 1
    html += FPSTR(HTML_SECTION_MQTT);
    html += "<div class='row'><div>";
    html += FPSTR(LABEL_MQTT_BROKER);
    snprintf(buffer, sizeof(buffer), "<input type='text' name='mqttBroker' required value='%s'></div>", staticConfig.mqttBroker);
    html += buffer;
    html += "<div>";
    html += FPSTR(LABEL_MQTT_PORT);
    snprintf(buffer, sizeof(buffer), "<input type='number' name='mqttPort' required value='%d'></div></div>", staticConfig.mqttPort);
    html += buffer;
    
    html += "<div class='row'><div>";
    html += FPSTR(LABEL_MQTT_USER);
    snprintf(buffer, sizeof(buffer), "<input type='text' name='mqttUser' value='%s'></div>", staticConfig.mqttUser);
    html += buffer;
    html += "<div>";
    html += FPSTR(LABEL_MQTT_PASSWORD);
    html += "<input type='password' name='mqttPassword' placeholder='(не показан)'></div></div>";
    html += FPSTR(HINT_PASSWORD);
    
    html += FPSTR(LABEL_MQTT_CLIENT_ID);
    snprintf(buffer, sizeof(buffer), "<input type='text' name='mqttClientId' required value='%s'>", staticConfig.mqttClientId);
    html += buffer;
    #endif
    
    #if DEVICE_TYPE == 1
    html += FPSTR(HTML_SECTION_SENSOR);
    html += "<div class='row'><div>";
    html += FPSTR(LABEL_LOW_TEMP);
    snprintf(buffer, sizeof(buffer), "<input type='number' step='0.1' name='lowTemp' required value='%.1f'></div>", staticConfig.lowTemp);
    html += buffer;
    html += "<div>";
    html += FPSTR(LABEL_HIGH_TEMP);
    snprintf(buffer, sizeof(buffer), "<input type='number' step='0.1' name='highTemp' required value='%.1f'></div></div>", staticConfig.highTemp);
    html += buffer;
    
    html += "<div class='row'><div>";
    html += FPSTR(LABEL_LOW_HUM);
    snprintf(buffer, sizeof(buffer), "<input type='number' step='0.1' name='lowHum' required value='%.1f'></div>", staticConfig.lowHum);
    html += buffer;
    html += "<div>";
    html += FPSTR(LABEL_HIGH_HUM);
    snprintf(buffer, sizeof(buffer), "<input type='number' step='0.1' name='highHum' required value='%.1f'></div></div>", staticConfig.highHum);
    html += buffer;
    
    html += "<div class='row'><div>";
    html += FPSTR(LABEL_SENSOR_INTERVAL);
    snprintf(buffer, sizeof(buffer), "<input type='number' name='sensorInterval' required value='%d'></div>", staticConfig.sensorInterval);
    html += buffer;
    html += "<div>";
    html += FPSTR(LABEL_MAX_ON_TIME);
    snprintf(buffer, sizeof(buffer), "<input type='number' name='maxOnTime' min='0' required value='%u'></div></div>", staticConfig.maxOnTime);
    html += buffer;
    
    html += FPSTR(HTML_SECTION_CONTROL);
    html += FPSTR(LABEL_DELAY_SECONDS);
    snprintf(buffer, sizeof(buffer), "<input type='number' name='delaySeconds' required value='%d'>", staticConfig.delaySeconds);
    html += buffer;
    
    html += FPSTR(HTML_SECTION_SILENT);
    html += FPSTR(LABEL_SPEED_PERCENT);
    snprintf(buffer, sizeof(buffer), "<input type='number' name='speedPercent' min='0' max='100' required value='%d'>", staticConfig.speedPercent);
    html += buffer;
    html += FPSTR(NOTE_SILENT_MODE);
    
    html += FPSTR(HTML_SECTION_ADAPTIVE);
    html += FPSTR(LABEL_ADAPTIVE_MODE);
    if (staticConfig.adaptiveMode) {
        html += "<script>document.querySelector('input[name=\"adaptiveMode\"]').checked=true;</script>";
    }
    html += FPSTR(NOTE_ADAPTIVE);
    
    html += FPSTR(HTML_SECTION_BOOT);
    html += FPSTR(LABEL_BOOT_STATE);
    if (staticConfig.bootState) {
        html += "<script>document.querySelector('input[name=\"bootState\"]').checked=true;</script>";
    }
    html += FPSTR(NOTE_BOOT);
    
    html += FPSTR(HTML_SECTION_MODES);
    html += FPSTR(LABEL_SENSOR_CONTROL_MODE);
    if (staticConfig.sensorControlMode) {
        html += "<script>document.querySelector('input[name=\"sensorControlMode\"]').checked=true;</script>";
    }
    html += FPSTR(NOTE_SENSOR_CONTROL);
    #endif
    
    #if DEVICE_TYPE == 2
    html += FPSTR(HTML_SECTION_SENSOR);
    html += FPSTR(LABEL_SENSOR_INTERVAL);
    snprintf(buffer, sizeof(buffer), "<input type='number' name='sensorInterval' required value='%d'>", staticConfig.sensorInterval);
    html += buffer;
    #endif
    
    #if DEVICE_TYPE == 3
    html += FPSTR(HTML_SECTION_CONTROL);
    html += FPSTR(LABEL_DELAY_SECONDS);
    snprintf(buffer, sizeof(buffer), "<input type='number' name='delaySeconds' required value='%d'>", staticConfig.delaySeconds);
    html += buffer;
    html += FPSTR(LABEL_MAX_ON_TIME);
    snprintf(buffer, sizeof(buffer), "<input type='number' name='maxOnTime' min='0' required value='%u'>", staticConfig.maxOnTime);
    html += buffer;
    
    html += FPSTR(HTML_SECTION_BOOT);
    html += FPSTR(LABEL_BOOT_STATE);
    if (staticConfig.bootState) {
        html += "<script>document.querySelector('input[name=\"bootState\"]').checked=true;</script>";
    }
    html += FPSTR(NOTE_BOOT);
    #endif
    
    html += FPSTR(HTML_CHECKBOX_CONFIRM);
    html += FPSTR(HTML_SUBMIT_BUTTON);
    html += FPSTR(HTML_FORM_CLOSE);
    
    #if OTA_ENABLED == 1
    html += "<a href='/update' class='link-btn'>Обновить прошивку (OTA)</a>";
    #endif
    
    html += "<a href='/' class='link-btn'>Домой</a>";
    html += FPSTR(HTML_CONTAINER_CLOSE);
    
    request->send(200, "text/html", html);
}
#endif

// ========== СТРАНИЦА СТАТУСА ==========

#if WEB_STATUS_ENABLED == 1
#ifdef ESP8266
void web_sendStatusPage(int refreshInterval) {
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");
    
    web_sendChunk_P(HTML_DOCTYPE);
    web_sendFormatted("<meta http-equiv='refresh' content='%d'>", refreshInterval);
    web_sendFormatted("<title>%s</title>", DEVICE_PREFIX);
    web_sendChunk_P(HTML_STYLE);
    web_sendChunk_P(HTML_CONTAINER_OPEN);
    
    web_sendFormatted("<h1>%s VERSION %s</h1>", DEVICE_PREFIX, VERSION);
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    web_sendChunk_P("<div class='flex-container'>");
    
    #if DEVICE_TYPE == 1
    const char* tempColor = (currentTemp >= config.highTemp) ? "#f44336" : 
                           ((currentTemp <= config.lowTemp) ? "#4CAF50" : "#2196F3");
    const char* humColor = (currentHum >= config.highHum) ? "#f44336" : 
                          ((currentHum <= config.lowHum) ? "#4CAF50" : "#2196F3");
    #else
    const char* tempColor = "#2196F3";
    const char* humColor = "#2196F3";
    #endif
    
    web_sendFormatted("<div class='sensor-card' style='background:%s20; border:2px solid %s;'>", tempColor, tempColor);
    web_sendFormatted("<div class='sensor-value' style='color:%s;'>%.1f °C</div>", tempColor, currentTemp);
    web_sendChunk_P("<div class='sensor-label'>Температура");
    #if DEVICE_TYPE == 1
    web_sendFormatted(" (выкл: %.1f вкл: %.1f)", config.lowTemp, config.highTemp);
    #endif
    web_sendChunk_P("</div></div>");
    
    web_sendFormatted("<div class='sensor-card' style='background:%s20; border:2px solid %s;'>", humColor, humColor);
    web_sendFormatted("<div class='sensor-value' style='color:%s;'>%.1f %%</div>", humColor, currentHum);
    web_sendChunk_P("<div class='sensor-label'>Влажность");
    #if DEVICE_TYPE == 1
    web_sendFormatted(" (выкл: %.1f вкл: %.1f)", config.lowHum, config.highHum);
    #endif
    web_sendChunk_P("</div></div></div>");
    
    if (!sensorOk && strlen(sensorError) > 0) {
        web_sendFormatted("<div class='sensor-error'><strong>Ошибка датчика</strong><br>%s</div>", sensorError);
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
    
    const char* stateColor = state ? "#f44336" : "#2196F3";
    const char* stateText = state ? "ВКЛ" : "ВЫКЛ";
    
    web_sendFormatted("<a href='%s'>", toggleUrl);
    web_sendFormatted("<div class='status-card' style='background:%s20; border:2px solid %s;'>", stateColor, stateColor);
    web_sendFormatted("<div style='font-size:2em;font-weight:bold;color:%s;'>%s: %s</div></div></a>", stateColor, label, stateText);
    
    #if DEVICE_TYPE == 1
    if (state) {
        int currentSpeed = startingPulseActive ? 100 : config.speedPercent;
        web_sendChunk_P("<div class='status-card' style='background:#2196F320; border:2px solid #2196F3;'>");
        web_sendFormatted("<div style='font-size:1.2em;font-weight:bold;'>Скорость: %d%%</div>", currentSpeed);
        web_sendFormatted("<div class='duty-bar'><div class='duty-fill' style='width:%d%%;'></div></div>", currentSpeed);
        if (config.speedPercent < 100) {
            web_sendChunk_P("<div style='font-size:0.9em;color:#555;'>Тихий режим активен");
            if (config.adaptiveMode) {
                web_sendChunk_P(" + адаптация");
            }
            web_sendChunk_P("</div>");
        }
        web_sendChunk_P("</div>");
    }
    
    const char* modeText = config.sensorControlMode ? "УПРАВЛЕНИЕ СЕНСОРОМ" : "РУЧНОЙ";
    const char* modeColor = config.sensorControlMode ? "#4CAF50" : "#f44336";
    
    web_sendFormatted("<div class='status-card' style='background:%s20; border:2px solid %s;'>", modeColor, modeColor);
    web_sendFormatted("<div style='font-size:1.5em;font-weight:bold;color:%s;'>Режим: %s</div>", modeColor, modeText);
    web_sendChunk_P("</div>");
    #endif
    #endif
    
    web_sendChunk_P("<hr><div class='info'>");
    web_sendFormatted("Обновление: %d сек<br>", refreshInterval);
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    web_sendFormatted("Опрос датчика %d сек<br>", config.sensorInterval);
    #endif
    
    #if MQTT_ENABLED == 1
    web_sendFormatted("MQTT: %s<br>", mqttManager.isConnected() ? "подключен" : "отключен");
    #endif
    
    #if WEB_SHOW_RSSI == 1
    web_sendFormatted("RSSI: %d dBm<br>", WiFi.RSSI());
    #endif
    
    web_sendChunk_P("</div>");
    
    web_sendChunk_P("<div class='button-group'>");
    #if DEVICE_TYPE == 1
    if (!config.sensorControlMode) {
        web_sendChunk_P("<a href='/fan/auto'><button>Режим управления сенсором</button></a>");
    }
    #endif
    web_sendChunk_P("<a href='/config'><button>Настройки</button></a>");
    web_sendChunk_P("</div>");
    
    web_sendChunk_P(HTML_CONTAINER_CLOSE);
}

#else // ESP32

void web_sendStatusPage(AsyncWebServerRequest *request, int refreshInterval) {
    String html;
    html.reserve(4096);
    
    html += FPSTR(HTML_DOCTYPE);
    char buffer[256];
    snprintf(buffer, sizeof(buffer), "<meta http-equiv='refresh' content='%d'>", refreshInterval);
    html += buffer;
    snprintf(buffer, sizeof(buffer), "<title>%s</title>", DEVICE_PREFIX);
    html += buffer;
    html += FPSTR(HTML_STYLE);
    html += FPSTR(HTML_CONTAINER_OPEN);
    
    snprintf(buffer, sizeof(buffer), "<h1>%s VERSION %s</h1>", DEVICE_PREFIX, VERSION);
    html += buffer;
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    html += "<div class='flex-container'>";
    
    #if DEVICE_TYPE == 1
    const char* tempColor = (currentTemp >= config.highTemp) ? "#f44336" : 
                           ((currentTemp <= config.lowTemp) ? "#4CAF50" : "#2196F3");
    const char* humColor = (currentHum >= config.highHum) ? "#f44336" : 
                          ((currentHum <= config.lowHum) ? "#4CAF50" : "#2196F3");
    #else
    const char* tempColor = "#2196F3";
    const char* humColor = "#2196F3";
    #endif
    
    snprintf(buffer, sizeof(buffer), "<div class='sensor-card' style='background:%s20; border:2px solid %s;'>", tempColor, tempColor);
    html += buffer;
    snprintf(buffer, sizeof(buffer), "<div class='sensor-value' style='color:%s;'>%.1f °C</div>", tempColor, currentTemp);
    html += buffer;
    html += "<div class='sensor-label'>Температура";
    #if DEVICE_TYPE == 1
    snprintf(buffer, sizeof(buffer), " (выкл: %.1f вкл: %.1f)", config.lowTemp, config.highTemp);
    html += buffer;
    #endif
    html += "</div></div>";
    
    snprintf(buffer, sizeof(buffer), "<div class='sensor-card' style='background:%s20; border:2px solid %s;'>", humColor, humColor);
    html += buffer;
    snprintf(buffer, sizeof(buffer), "<div class='sensor-value' style='color:%s;'>%.1f %%</div>", humColor, currentHum);
    html += buffer;
    html += "<div class='sensor-label'>Влажность";
    #if DEVICE_TYPE == 1
    snprintf(buffer, sizeof(buffer), " (выкл: %.1f вкл: %.1f)", config.lowHum, config.highHum);
    html += buffer;
    #endif
    html += "</div></div></div>";
    
    if (!sensorOk && strlen(sensorError) > 0) {
        snprintf(buffer, sizeof(buffer), "<div class='sensor-error'><strong>Ошибка датчика</strong><br>%s</div>", sensorError);
        html += buffer;
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
    
    const char* stateColor = state ? "#f44336" : "#2196F3";
    const char* stateText = state ? "ВКЛ" : "ВЫКЛ";
    
    snprintf(buffer, sizeof(buffer), "<a href='%s'>", toggleUrl);
    html += buffer;
    snprintf(buffer, sizeof(buffer), "<div class='status-card' style='background:%s20; border:2px solid %s;'>", stateColor, stateColor);
    html += buffer;
    snprintf(buffer, sizeof(buffer), "<div style='font-size:2em;font-weight:bold;color:%s;'>%s: %s</div></div></a>", stateColor, label, stateText);
    html += buffer;
    
    #if DEVICE_TYPE == 1
    if (state) {
        int currentSpeed = startingPulseActive ? 100 : config.speedPercent;
        html += "<div class='status-card' style='background:#2196F320; border:2px solid #2196F3;'>";
        snprintf(buffer, sizeof(buffer), "<div style='font-size:1.2em;font-weight:bold;'>Скорость: %d%%</div>", currentSpeed);
        html += buffer;
        snprintf(buffer, sizeof(buffer), "<div class='duty-bar'><div class='duty-fill' style='width:%d%%;'></div></div>", currentSpeed);
        html += buffer;
        if (config.speedPercent < 100) {
            html += "<div style='font-size:0.9em;color:#555;'>Тихий режим активен";
            if (config.adaptiveMode) {
                html += " + адаптация";
            }
            html += "</div>";
        }
        html += "</div>";
    }
    
    const char* modeText = config.sensorControlMode ? "УПРАВЛЕНИЕ СЕНСОРОМ" : "РУЧНОЙ";
    const char* modeColor = config.sensorControlMode ? "#4CAF50" : "#f44336";
    
    snprintf(buffer, sizeof(buffer), "<div class='status-card' style='background:%s20; border:2px solid %s;'>", modeColor, modeColor);
    html += buffer;
    snprintf(buffer, sizeof(buffer), "<div style='font-size:1.5em;font-weight:bold;color:%s;'>Режим: %s</div>", modeColor, modeText);
    html += buffer;
    html += "</div>";
    #endif
    #endif
    
    html += "<hr><div class='info'>";
    snprintf(buffer, sizeof(buffer), "Обновление: %d сек<br>", refreshInterval);
    html += buffer;
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    snprintf(buffer, sizeof(buffer), "Опрос датчика %d сек<br>", config.sensorInterval);
    html += buffer;
    #endif
    
    #if MQTT_ENABLED == 1
    snprintf(buffer, sizeof(buffer), "MQTT: %s<br>", mqttManager.isConnected() ? "подключен" : "отключен");
    html += buffer;
    #endif
    
    #if WEB_SHOW_RSSI == 1
    snprintf(buffer, sizeof(buffer), "RSSI: %d dBm<br>", WiFi.RSSI());
    html += buffer;
    #endif
    
    html += "</div>";
    
    html += "<div class='button-group'>";
    #if DEVICE_TYPE == 1
    if (!config.sensorControlMode) {
        html += "<a href='/fan/auto'><button>Режим управления сенсором</button></a>";
    }
    #endif
    html += "<a href='/config'><button>Настройки</button></a>";
    html += "</div>";
    
    html += FPSTR(HTML_CONTAINER_CLOSE);
    
    request->send(200, "text/html", html);
}
#endif
#endif

// ========== СОХРАНЕНИЕ КОНФИГУРАЦИИ ==========

#ifdef ESP8266
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
    server.send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'></head><body><div style='text-align:center;margin-top:50px;'><h2>Настройки сохранены!</h2><p>Перезагрузка...</p></div></body></html>");
    delay(1000);
    #if DEBUG_ENABLED == 1
        Serial.println("[DEBUG] Restarting...");
    #endif
    ESP.restart();
}

#else // ESP32

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
    request->send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'></head><body><div style='text-align:center;margin-top:50px;'><h2>Настройки сохранены!</h2><p>Перезагрузка...</p></div></body></html>");
    delay(1000);
    ESP.restart();
}
#endif

// ========== ИНИЦИАЛИЗАЦИЯ WEB СЕРВЕРА ==========

void web_init() {
    int refreshInterval = 5;

    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    refreshInterval = config.sensorInterval;
    #endif
    
    #if WEB_STATUS_ENABLED == 1
    #ifdef ESP8266
    server.on("/", [refreshInterval](){ 
        web_sendStatusPage(refreshInterval);
    });
    #else
    server.on("/", HTTP_GET, [refreshInterval](AsyncWebServerRequest *request){ 
        web_sendStatusPage(request, refreshInterval);
    });
    #endif
    #else
    #ifdef ESP8266
    server.on("/", [](){ 
        server.sendHeader("Location", "/config", true); 
        server.send(302, "text/plain", ""); 
    });
    #else
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ 
        request->redirect("/config"); 
    });
    #endif
    #endif
    
    #ifdef ESP8266
    server.on("/config", [](){ 
        web_sendConfigPage(""); 
    });
    
    server.on("/save", web_saveConfig);
    
    #if WEB_RESET_ENABLED == 1
    server.on("/resetall", [](){
        config_clear();
        server.send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'></head><body><h2>Настройки сброшены, перезагрузка...</h2></body></html>");
        delay(1000);
        #if DEBUG_ENABLED == 1
        Serial.println("[DEBUG] Restarting...");
        #endif
        ESP.restart();
    });
    #endif
    
    #else // ESP32
    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){ 
        web_sendConfigPage(request, ""); 
    });
    
    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){ 
        web_saveConfig(request); 
    });
    
    #if WEB_RESET_ENABLED == 1
    server.on("/resetall", HTTP_GET, [](AsyncWebServerRequest *request){
        config_clear();
        request->send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'></head><body><h2>Настройки сброшены, перезагрузка...</h2></body></html>");
        delay(1000);
        #if DEBUG_ENABLED == 1
        Serial.println("[DEBUG] Restarting...");
        #endif
        ESP.restart();
    });
    #endif
    #endif
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    #ifdef ESP8266
    #if DEVICE_TYPE == 1
    server.on("/fan/toggle", [](){ 
        handleToggle(); 
        server.sendHeader("Location", "/", true); 
        server.send(302, "text/plain", ""); 
    });
    server.on("/fan/auto", [](){ 
        handleSensorControlMode(); 
        server.sendHeader("Location", "/", true); 
        server.send(302, "text/plain", ""); 
    });
    #elif DEVICE_TYPE == 3
    server.on("/switch/toggle", [](){ 
        handleToggle(); 
        server.sendHeader("Location", "/", true); 
        server.send(302, "text/plain", ""); 
    });
    #endif
    
    #else // ESP32
    #if DEVICE_TYPE == 1
    server.on("/fan/toggle", HTTP_GET, [](AsyncWebServerRequest *request){ 
        handleToggle(); 
        request->redirect("/"); 
    });
    server.on("/fan/auto", HTTP_GET, [](AsyncWebServerRequest *request){ 
        handleSensorControlMode(); 
        request->redirect("/"); 
    });
    #elif DEVICE_TYPE == 3
    server.on("/switch/toggle", HTTP_GET, [](AsyncWebServerRequest *request){ 
        handleToggle(); 
        request->redirect("/"); 
    });
    #endif
    #endif
    #endif

    #if OTA_ENABLED == 1
    #if defined(ESP32)
      ElegantOTA.begin(&server);
      #if LOG_OTA == 1
        Serial.println("[OTA] ElegantOTA initialized for ESP32");
      #endif
    #elif defined(ESP8266)
      if (!otaInitialized) {
        #if LOG_OTA == 1
          Serial.printf("[OTA] Free heap before ElegantOTA: %d\n", ESP.getFreeHeap());
        #endif
        ElegantOTA.begin(&server);
        otaInitialized = true;
        #if LOG_OTA == 1
          Serial.println("[OTA] ElegantOTA initialized for ESP8266");
        #endif  
      }
    #endif
    #endif
  
    server.begin();
  
    #if LOG_WEB == 1
    Serial.println("[WEB] Web server started on port 80 (client mode)");
    #endif
}

// ========== РЕЖИМ ТОЧКИ ДОСТУПА ==========

void web_initAP() {
    if (apMode) return;
  
    apMode = true;

    #if STATUS_LED_PIN > 0
      led_setMode(LED_MODE_SLOW_BLINK);
    #endif

    WiFi.mode(WIFI_AP);
  
    #ifdef ESP8266
      IPAddress apIP;
      apIP.fromString(AP_IP_ADDRESS);
      WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
      WiFi.softAP(deviceId);
      dnsServer.start(DNS_PORT, "*", IPAddress(192,168,4,1));
    
      #if LOG_AP == 1
        Serial.printf("[AP] AP started: %s, IP: %s\n", deviceId, AP_IP_ADDRESS);
      #endif
    #elif defined(ESP32)    
      WiFi.softAP(deviceId);
      #if LOG_AP == 1
        Serial.printf("[AP] AP started: %s, IP: %s\n", deviceId, WiFi.softAPIP().toString().c_str());
      #endif
    #endif

    #if OTA_ENABLED == 1
    #if defined(ESP32)
      ElegantOTA.begin(&server);
      #if LOG_OTA == 1
        Serial.println("[OTA] ElegantOTA initialized for ESP32 (AP mode)");
      #endif
    #elif defined(ESP8266)
      #if LOG_OTA == 1
        Serial.printf("[OTA] Free heap before ElegantOTA: %d\n", ESP.getFreeHeap());
      #endif
      ElegantOTA.begin(&server);
      #if LOG_OTA == 1
        Serial.println("[OTA] ElegantOTA initialized for ESP8266 (AP mode)");
      #endif
    #endif
    #endif

    #ifdef ESP8266
      server.on("/", [](){ web_sendConfigPage(""); });
      server.on("/save", web_saveConfig);
    #elif defined(ESP32)
      server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ 
          web_sendConfigPage(request, ""); 
      });
      server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){ 
          web_saveConfig(request); 
      });
    #endif
  
    server.begin();
}

// ========== ОБНОВЛЕНИЕ ==========

void web_update() {
    #ifdef ESP8266
      server.handleClient();   
    #endif
  
    #ifdef ESP8266
      if (apMode) {
        dnsServer.processNextRequest();
      }
    #endif
}