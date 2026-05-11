#include "web.h"
#include "sensor.h"
#include "fan.h"
#include "config.h"
#include "mqtt.h"

#ifdef ESP32
  #include <WiFi.h>
  #include <ESPAsyncWebServer.h>
  #include <ElegantOTA.h>
  #include <DNSServer.h>
  DNSServer dnsServer;
  const byte DNS_PORT = 53;
  
  AsyncWebServer server(80);
  
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <ElegantOTA.h>
  #include <DNSServer.h>
  DNSServer dnsServer;
  const byte DNS_PORT = 53;
  ESP8266WebServer server(80);
#endif

String apSSID;
static bool otaInitialized = false;

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
void handleToggle() {
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  config.automaticMode = false;  // переходим в ручной режим
  #endif
  fan_set(!fan_getState());
}

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
void handleAutoMode() {
  fan_setOverrideMode(true);  // включаем автоматический режим
}
#endif
#endif

String web_getConfigPage(String errorMsg) {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
#if DEVICE_TYPE == 1
  html += "<title>Fan Configuration</title>";
#elif DEVICE_TYPE == 2
  html += "<title>Sensor Configuration</title>";
#elif DEVICE_TYPE == 3
  html += "<title>Switch Configuration</title>";
#else
  html += "<title>Device Configuration</title>";
#endif
  html += "<style>";
  html += "body{font-family:Arial;margin:20px;background:#f0f0f0;}";
  html += ".container{max-width:700px;margin:auto;background:white;padding:20px;border-radius:10px;}";
  html += "h1{color:#2c3e50;}h3{color:#2c3e50;border-bottom:1px solid #ccc;padding-bottom:5px;}";
  html += "label{display:block;margin-top:10px;font-weight:bold;}";
  html += "input[type=text],input[type=password],input[type=number]{width:100%;padding:8px;margin:5px 0;border:1px solid #ccc;border-radius:4px;font-size:1.2em;box-sizing:border-box;}";
  html += "input[type=checkbox]{width:20px;height:20px;margin-right:10px;vertical-align:middle;cursor:pointer;transform:scale(1.5);}";
  html += "input[type=submit]{background:#2c3e50;color:white;padding:10px 20px;margin-top:20px;border:none;border-radius:4px;cursor:pointer;width:100%;font-size:1em;box-sizing:border-box;}";
  html += "input[type=submit]:hover{background:#1a252f;}";
  html += ".info{background:#e7f3ff;padding:10px;border-radius:5px;margin:10px 0;}";
  html += ".warning{background:#fff3cd;padding:10px;border-radius:5px;margin:10px 0;color:#856404;}";
  html += ".error{background:#ffebee;padding:10px;border-radius:5px;margin:10px 0;color:#c62828;}";
  html += ".row{display:flex;gap:10px;}.row>div{flex:1;}";
  html += ".password-hint{color:#7f8c8d;margin-top:-2px;margin-bottom:8px;}";
  html += ".ota-btn{background:#555;color:white;padding:10px 20px;margin-top:10px;border:none;border-radius:4px;cursor:pointer;font-size:1em;text-align:center;text-decoration:none;display:block;box-sizing:border-box;}";
  html += ".ota-btn:hover{background:#333;}";
  html += "</style></head><body><div class='container'>";
  html += "<h1>Настройка устройства</h1>";
  html += "<div class='info'><strong>Текущее состояние</strong><br>";
  if (apMode) {  
    html += "Точка доступа: <strong>" + apSSID + "</strong><br>";
    html += "IP адрес: <strong>" + String(AP_IP_ADDRESS) + "</strong><br>";
    html += "Режим: <strong>Точка доступа (AP)</strong>";
  } else {
    html += "WiFi сеть: <strong>" + String(staticConfig.wifiSsid) + "</strong><br>";
    html += "IP адрес: <strong>" + WiFi.localIP().toString() + "</strong><br>";
    html += "Режим: <strong>Клиент WiFi</strong>";
  }
  html += "</div><form method='POST' action='/save'>";
  
  if (errorMsg.length() > 0) {
    html += "<div class='error'><strong>Ошибка:</strong> " + errorMsg + "</div>";
  }
  
  html += "<h3>Настройки сети</h3>";
  html += "<label>WiFi SSID:</label><input type='text' name='wifiSsid' required value='" + String(staticConfig.wifiSsid) + "'>";
  html += "<label>WiFi Password:</label><input type='password' name='wifiPassword' placeholder='(не показан)'>";
  html += "<div class='password-hint'>Оставьте пустым, чтобы сохранить текущий пароль WiFi</div>";
  
  html += "<h3>MQTT настройки</h3>";
  html += "<div class='row'><div><label>MQTT Broker:</label><input type='text' name='mqttBroker' required value='" + String(staticConfig.mqttBroker) + "'></div>";
  html += "<div><label>MQTT Port:</label><input type='number' name='mqttPort' required value='" + String(staticConfig.mqttPort) + "'></div></div>";
  html += "<div class='row'><div><label>MQTT User:</label><input type='text' name='mqttUser' value='" + String(staticConfig.mqttUser) + "'></div>";
  html += "<div><label>MQTT Password:</label><input type='password' name='mqttPassword' placeholder='(не показан)'></div></div>";
  html += "<div class='password-hint'>Оставьте пустым, чтобы сохранить текущий пароль MQTT</div>";
  html += "<label>MQTT Client ID:</label><input type='text' name='mqttClientId' required value='" + String(staticConfig.mqttClientId) + "'>";
  
  #if DEVICE_TYPE == 1
  html += "<h3>Настройки датчиков</h3>";
  html += "<div class='row'><div><label>Low Temp (°C):</label><input type='number' step='0.1' name='lowTemp' required value='" + String(staticConfig.lowTemp) + "'></div>";
  html += "<div><label>High Temp (°C):</label><input type='number' step='0.1' name='highTemp' required value='" + String(staticConfig.highTemp) + "'></div></div>";
  html += "<div class='row'><div><label>Low Hum (%):</label><input type='number' step='0.1' name='lowHum' required value='" + String(staticConfig.lowHum) + "'></div>";
  html += "<div><label>High Hum (%):</label><input type='number' step='0.1' name='highHum' required value='" + String(staticConfig.highHum) + "'></div></div>";
  html += "<div class='row'><div><label>Интервал опроса датчика (сек)</label><input type='number' name='sensorInterval' required value='" + String(staticConfig.sensorInterval) + "'></div>";
  html += "<div><label>Аварийное отключение через </label><input type='number' name='maxOnTime' min='0' required value='" + String(staticConfig.maxOnTime) + "'></div></div>";
  html += "<h3>Управление</h3>";
  html += "<label>Принудительно включить через </label><input type='number' name='delaySeconds' required value='" + String(staticConfig.delaySeconds) + "'>";
  html += "<h3>Тихий режим</h3>";
  html += "<label><input type='checkbox' name='slowModeEnabled' value='1' " + String(staticConfig.slowModeEnabled ? "checked" : "") + "> Включить</label>";
  html += "<label>Скважность (0-255):</label><input type='number' name='slowModeDuty' required value='" + String(staticConfig.slowModeDuty) + "'>";
  html += "<h3>Поведение при старте</h3>";
  html += "<label><input type='checkbox' name='forceOffOnBoot' value='1' " + String(staticConfig.forceOffOnBoot ? "checked" : "") + "> Принудительно выключать при старте</label>";
  html += "<h3>Режимы работы</h3>";
  html += "<label><input type='checkbox' name='automaticMode' value='1' " + String(staticConfig.automaticMode ? "checked" : "") + "> Автоматический режим</label>";
  #endif
  
  #if DEVICE_TYPE == 2
  html += "<h3>Настройки датчиков</h3>";
  html += "<div><label>Интервал опроса датчика (сек)</label><input type='number' name='sensorInterval' required value='" + String(staticConfig.sensorInterval) + "'></div>";
  #endif
  
  #if DEVICE_TYPE == 3
  html += "<h3>Настройки управления</h3>";
  html += "<label>Принудительно включить через </label><input type='number' name='delaySeconds' required value='" + String(staticConfig.delaySeconds) + "'>";
  html += "<label>Аварийное отключение через </label><input type='number' name='maxOnTime' min='0' required value='" + String(staticConfig.maxOnTime) + "'>";
  html += "<h3>Тихий режим</h3>";
  html += "<label><input type='checkbox' name='slowModeEnabled' value='1' " + String(staticConfig.slowModeEnabled ? "checked" : "") + "> Включить</label>";
  html += "<label>Скважность (0-255):</label><input type='number' name='slowModeDuty' required value='" + String(staticConfig.slowModeDuty) + "'>";
  html += "<h3>Поведение при старте</h3>";
  html += "<label><input type='checkbox' name='forceOffOnBoot' value='1' " + String(staticConfig.forceOffOnBoot ? "checked" : "") + "> Принудительно выключать при старте</label>";
  html += "<h3>Режимы работы</h3>";
  html += "<label><input type='checkbox' name='automaticMode' value='1' " + String(staticConfig.automaticMode ? "checked" : "") + "> Автоматический режим</label>";
  #endif
  
  html += "<input type='submit' value='Сохранить и перезагрузить'>";
  html += "</form>";
  html += "<a href='/update' class='ota-btn'>Обновить прошивку (OTA)</a>";
  html += "</div></body></html>";
  
  return html;
}

#if WEB_STATUS_ENABLED
String web_getStatusPage(int refreshInterval) {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta http-equiv='refresh' content='" + String(refreshInterval) + "'>";
  html += "<title>" + String(config.mqttClientId) + "</title>";
  html += "<style>";
  html += "body{font-family:Arial;margin:20px;background:#f0f0f0;}";
  html += ".container{max-width:800px;margin:auto;background:white;padding:20px;border-radius:10px;}";
  html += "h1{color:#2c3e50;text-align:center;word-break:break-all;}";
  html += ".sensor-card{display:inline-block;width:45%;margin:10px;padding:15px;border-radius:10px;text-align:center;}";
  html += ".sensor-value{font-size:2em;font-weight:bold;}.sensor-label{margin-top:5px;}";
  html += ".status-card{padding:15px;border-radius:10px;text-align:center;margin:10px;}";
  html += ".info{color:#7f8c8d;margin-top:20px;text-align:center;}";
  html += "button{background:#2c3e50;color:white;padding:10px;border:none;border-radius:4px;cursor:pointer;margin:5px;font-size:1em}";
  html += ".flex-container{display:flex;flex-wrap:wrap;justify-content:center;}";
  html += ".button-group{display:flex;justify-content:center;gap:10px;margin-top:20px;flex-wrap:wrap;}";
  html += "a{text-decoration:none;}";
  html += "</style></head><body><div class='container'>";
  
  html += "<h1>" + String(config.mqttClientId) + "</h1>";
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  html += "<div class='flex-container'>";
  
  #if DEVICE_TYPE == 1
  String tempColor = (currentTemp >= config.highTemp) ? "#f44336" : (currentTemp <= config.lowTemp) ? "#4CAF50" : "#2196F3";
  String humColor = (currentHum >= config.highHum) ? "#f44336" : (currentHum <= config.lowHum) ? "#4CAF50" : "#2196F3";
  #else
  String tempColor = "#2196F3";
  String humColor = "#2196F3";
  #endif
  
  html += "<div class='sensor-card' style='background:" + tempColor + "20; border:2px solid " + tempColor + ";'>";
  html += "<div class='sensor-value' style='color:" + tempColor + ";'>" + String(currentTemp) + " °C</div>";
  html += "<div class='sensor-label'>Температура";
  #if DEVICE_TYPE == 1
  html += " (выкл: " + String(config.lowTemp) + " вкл: " + String(config.highTemp) + ")";
  #endif
  html += " °C</div></div>";
  
  html += "<div class='sensor-card' style='background:" + humColor + "20; border:2px solid " + humColor + ";'>";
  html += "<div class='sensor-value' style='color:" + humColor + ";'>" + String(currentHum) + " %</div>";
  html += "<div class='sensor-label'>Влажность";
  #if DEVICE_TYPE == 1
  html += " (выкл: " + String(config.lowHum) + " вкл: " + String(config.highHum) + ")";
  #endif
  html += " %</div></div>";
  
  html += "</div>";
  #endif
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  bool state = fan_getRealState();
  String stateColor = state ? "#f44336" : "#2196F3";
  String stateText = state ? "ВКЛ" : "ВЫКЛ";
  #if DEVICE_TYPE == 1
  String label = "Вентилятор";
  String toggleUrl = "/fan/toggle";
  String autoUrl = "/fan/auto";
  #else
  String label = "Выключатель";
  String toggleUrl = "/switch/toggle";
  String autoUrl = "/switch/auto";
  #endif
  
  html += "<a href='" + toggleUrl + "'>";
  html += "<div class='status-card' style='background:" + stateColor + "20; border:2px solid " + stateColor + ";'>";
  html += "<div style='font-size:2em;font-weight:bold;color:" + stateColor + ";'>" + label + ": " + stateText + "</div></div>";
  html += "</a>";
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  String modeText = config.automaticMode ? "АВТО" : "РУЧНОЙ";
  String modeColor = config.automaticMode ? "#4CAF50" : "#f44336";
  html += "<div class='status-card' style='background:" + modeColor + "20; border:2px solid " + modeColor + ";'>";
  html += "<div style='font-size:1.5em;font-weight:bold;color:" + modeColor + ";'>Режим: " + modeText + "</div></div>";
  #endif
  #endif
  
  html += "<hr><div class='info'>Обновление: " + String(refreshInterval) + " сек<br>";
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  html += "Опрос датчика " + String(config.sensorInterval) + " сек<br>";
  #endif
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  if (delayActive && delayTimer > 0) {
    unsigned long now = millis();
    if (now < delayTimer) {
      unsigned long remaining = (delayTimer - now + 999) / 1000;
      html += "Принудительное включение через <strong>" + String(remaining) + "</strong> сек<br>";
    } else {
      html += "Принудительное включение: <strong>выполняется...</strong><br>";
    }
  } else if (config.delaySeconds > 0) {
    html += "Принудительное включение: настроено на " + String(config.delaySeconds) + " сек<br>";
  } else {
    html += "Принудительное включение: <strong>отключено</strong><br>";
  }
  if (fanOn && config.maxOnTime > 0 && fanStartTime > 0) {
    unsigned long elapsed = (millis() - fanStartTime) / 1000;
    if (elapsed < config.maxOnTime) {
      unsigned long remaining = config.maxOnTime - elapsed;
      html += "Аварийное отключение через <strong>" + String(remaining) + "</strong> сек</div>";
    } else {
      html += "Аварийное отключение: <strong>сейчас</strong></div>";
    }
  } else if (config.maxOnTime > 0) {
    html += "Аварийное отключение: неактивно (лимит " + String(config.maxOnTime) + " сек)</div>";
  } else {
    html += "Аварийное отключение: <strong>отключено</strong></div>";
  }
  #endif
  
  html += "<div class='button-group'>";
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  if (!config.automaticMode) {
    html += "<a href='" + autoUrl + "'><button>Автоматический режим</button></a>";
  }
  #endif

  html += "<a href='/config'><button>Настройки</button></a>";
  html += "</div></div></body></html>";
  
  return html;
}
#endif // WEB_STATUS_ENABLED

#ifdef ESP32
void web_saveConfig(AsyncWebServerRequest *request) {
  if (request->hasParam("wifiSsid", true)) {
    AsyncWebParameter* p = request->getParam("wifiSsid", true);
    if (p) p->value().toCharArray(config.wifiSsid, sizeof(config.wifiSsid));
  }
  if (request->hasParam("wifiPassword", true)) {
    AsyncWebParameter* p = request->getParam("wifiPassword", true);
    if (p) {
      String pwd = p->value();
      if (pwd.length() > 0) pwd.toCharArray(config.wifiPassword, sizeof(config.wifiPassword));
    }
  }
  if (request->hasParam("mqttBroker", true)) {
    AsyncWebParameter* p = request->getParam("mqttBroker", true);
    if (p) p->value().toCharArray(config.mqttBroker, sizeof(config.mqttBroker));
  }
  if (request->hasParam("mqttPort", true)) {
    AsyncWebParameter* p = request->getParam("mqttPort", true);
    if (p) config.mqttPort = p->value().toInt();
  }
  if (request->hasParam("mqttUser", true)) {
    AsyncWebParameter* p = request->getParam("mqttUser", true);
    if (p) p->value().toCharArray(config.mqttUser, sizeof(config.mqttUser));
  }
  if (request->hasParam("mqttPassword", true)) {
    AsyncWebParameter* p = request->getParam("mqttPassword", true);
    if (p) {
      String pwd = p->value();
      if (pwd.length() > 0) pwd.toCharArray(config.mqttPassword, sizeof(config.mqttPassword));
    }
  }
  if (request->hasParam("mqttClientId", true)) {
    AsyncWebParameter* p = request->getParam("mqttClientId", true);
    if (p) {
      String cid = p->value();
      if (cid.length() > 0 && cid.length() < sizeof(config.mqttClientId)) {
        cid.toCharArray(config.mqttClientId, sizeof(config.mqttClientId));
      } else if (cid.length() == 0) {
        config.mqttClientId[0] = '\0';
      }
    }
  }
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  if (request->hasParam("sensorInterval", true)) {
    AsyncWebParameter* p = request->getParam("sensorInterval", true);
    if (p) config.sensorInterval = p->value().toInt();
  }
  #endif
  
  #if DEVICE_TYPE == 1
  if (request->hasParam("lowTemp", true)) {
    AsyncWebParameter* p = request->getParam("lowTemp", true);
    if (p) config.lowTemp = p->value().toFloat();
  }
  if (request->hasParam("highTemp", true)) {
    AsyncWebParameter* p = request->getParam("highTemp", true);
    if (p) config.highTemp = p->value().toFloat();
  }
  if (request->hasParam("lowHum", true)) {
    AsyncWebParameter* p = request->getParam("lowHum", true);
    if (p) config.lowHum = p->value().toFloat();
  }
  if (request->hasParam("highHum", true)) {
    AsyncWebParameter* p = request->getParam("highHum", true);
    if (p) config.highHum = p->value().toFloat();
  }
  if (request->hasParam("maxOnTime", true)) {
    AsyncWebParameter* p = request->getParam("maxOnTime", true);
    if (p) config.maxOnTime = p->value().toInt();
  }
  if (request->hasParam("delaySeconds", true)) {
    AsyncWebParameter* p = request->getParam("delaySeconds", true);
    if (p) config.delaySeconds = p->value().toInt();
  }
  config.slowModeEnabled = request->hasParam("slowModeEnabled", true);
  if (request->hasParam("slowModeDuty", true)) {
    AsyncWebParameter* p = request->getParam("slowModeDuty", true);
    if (p) config.slowModeDuty = p->value().toInt();
  }
  config.forceOffOnBoot = request->hasParam("forceOffOnBoot", true);
  config.automaticMode = request->hasParam("automaticMode", true);
  #endif
  
  #if DEVICE_TYPE == 3
  if (request->hasParam("maxOnTime", true)) {
    AsyncWebParameter* p = request->getParam("maxOnTime", true);
    if (p) config.maxOnTime = p->value().toInt();
  }
  if (request->hasParam("delaySeconds", true)) {
    AsyncWebParameter* p = request->getParam("delaySeconds", true);
    if (p) config.delaySeconds = p->value().toInt();
  }
  config.slowModeEnabled = request->hasParam("slowModeEnabled", true);
  if (request->hasParam("slowModeDuty", true)) {
    AsyncWebParameter* p = request->getParam("slowModeDuty", true);
    if (p) config.slowModeDuty = p->value().toInt();
  }
  config.forceOffOnBoot = request->hasParam("forceOffOnBoot", true);
  config.automaticMode = request->hasParam("automaticMode", true);
  #endif
  
  if (!config_validate()) {
    request->send(200, "text/html", web_getConfigPage(configLastError));
    return;
  }
  
  config_write();
  request->send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'></head><body><div style='text-align:center;margin-top:50px;'><h2>Настройки сохранены!</h2><p>Перезагрузка...</p></div></body></html>");
  delay(1000);
  ESP.restart();
}
#elif defined(ESP8266)
void web_saveConfig() {
  if (server.hasArg("wifiSsid"))
    server.arg("wifiSsid").toCharArray(config.wifiSsid, sizeof(config.wifiSsid));
  if (server.hasArg("wifiPassword")) {
    String pwd = server.arg("wifiPassword");
    if (pwd.length() > 0) pwd.toCharArray(config.wifiPassword, sizeof(config.wifiPassword));
  }
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
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  if (server.hasArg("sensorInterval"))
    config.sensorInterval = server.arg("sensorInterval").toInt();
  #endif
  
  #if DEVICE_TYPE == 1
  if (server.hasArg("lowTemp")) config.lowTemp = server.arg("lowTemp").toFloat();
  if (server.hasArg("highTemp")) config.highTemp = server.arg("highTemp").toFloat();
  if (server.hasArg("lowHum")) config.lowHum = server.arg("lowHum").toFloat();
  if (server.hasArg("highHum")) config.highHum = server.arg("highHum").toFloat();
  if (server.hasArg("maxOnTime")) config.maxOnTime = server.arg("maxOnTime").toInt();
  if (server.hasArg("delaySeconds")) config.delaySeconds = server.arg("delaySeconds").toInt();
  config.slowModeEnabled = server.hasArg("slowModeEnabled");
  if (server.hasArg("slowModeDuty")) config.slowModeDuty = server.arg("slowModeDuty").toInt();
  config.forceOffOnBoot = server.hasArg("forceOffOnBoot");
  config.automaticMode = server.hasArg("automaticMode");
  #endif
  
  #if DEVICE_TYPE == 3
  if (server.hasArg("maxOnTime")) config.maxOnTime = server.arg("maxOnTime").toInt();
  if (server.hasArg("delaySeconds")) config.delaySeconds = server.arg("delaySeconds").toInt();
  config.slowModeEnabled = server.hasArg("slowModeEnabled");
  if (server.hasArg("slowModeDuty")) config.slowModeDuty = server.arg("slowModeDuty").toInt();
  config.forceOffOnBoot = server.hasArg("forceOffOnBoot");
  config.automaticMode = server.hasArg("automaticMode");
  #endif
  
  if (!config_validate()) {
    server.send(200, "text/html", web_getConfigPage(configLastError));
    return;
  }
  
  config_write();
  server.send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'></head><body><div style='text-align:center;margin-top:50px;'><h2>Настройки сохранены!</h2><p>Перезагрузка...</p></div></body></html>");
  delay(1000);
  ESP.restart();
}
#endif

void web_init() {
  int refreshInterval = 2;
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  unsigned long elapsed = (millis() - lastSensorRead) / 1000;
  int timeUntilNextRead = config.sensorInterval - elapsed;
  refreshInterval = (timeUntilNextRead > 2) ? timeUntilNextRead : 2;
  if (refreshInterval > 10) refreshInterval = 10;
  #endif
  
  // Обработчик корневого пути
  #if WEB_STATUS_ENABLED
    #ifdef ESP32
      server.on("/", HTTP_GET, [refreshInterval](AsyncWebServerRequest *request){ 
        request->send(200, "text/html", web_getStatusPage(refreshInterval)); 
      });
    #elif defined(ESP8266)
      server.on("/", [refreshInterval](){ 
        server.send(200, "text/html", web_getStatusPage(refreshInterval)); 
      });
    #endif
  #else
    // Страница статуса отключена – перенаправляем на конфигурацию
    #ifdef ESP32
      server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ 
        request->redirect("/config"); 
      });
    #elif defined(ESP8266)
      server.on("/", [](){ 
        server.sendHeader("Location", "/config", true); 
        server.send(302, "text/plain", ""); 
      });
    #endif
  #endif
  
  // Страница конфигурации
  #ifdef ESP32
    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){ 
      request->send(200, "text/html", web_getConfigPage("")); 
    });
    
    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){ 
      web_saveConfig(request); 
    });
    
    #if WEB_RESET_ENABLED
    server.on("/resetall", HTTP_GET, [](AsyncWebServerRequest *request){
      config_clear();
      request->send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'></head><body><h2>Настройки сброшены, перезагрузка...</h2></body></html>");
      delay(1000);
      ESP.restart();
    });
    #endif
    
  #elif defined(ESP8266)
    server.on("/config", [](){ 
      server.send(200, "text/html", web_getConfigPage("")); 
    });
    
    server.on("/save", web_saveConfig);
    
    #if WEB_RESET_ENABLED
    server.on("/resetall", [](){
      config_clear();
      server.send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'></head><body><h2>Настройки сброшены, перезагрузка...</h2></body></html>");
      delay(1000);
      ESP.restart();
    });
    #endif
  #endif
  
  // Специфичные обработчики только для нужных типов устройств
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    #ifdef ESP32
      #if DEVICE_TYPE == 1
      server.on("/fan/toggle", HTTP_GET, [](AsyncWebServerRequest *request){ 
        handleToggle(); 
        request->redirect("/"); 
      });
      server.on("/fan/auto", HTTP_GET, [](AsyncWebServerRequest *request){ 
        handleAutoMode(); 
        request->redirect("/"); 
      });
      #elif DEVICE_TYPE == 3
      server.on("/switch/toggle", HTTP_GET, [](AsyncWebServerRequest *request){ 
        handleToggle(); 
        request->redirect("/"); 
      });
      server.on("/switch/auto", HTTP_GET, [](AsyncWebServerRequest *request){ 
        handleAutoMode(); 
        request->redirect("/"); 
      });
      #endif
    #elif defined(ESP8266)
      #if DEVICE_TYPE == 1
      server.on("/fan/toggle", [](){ 
        handleToggle(); 
        server.sendHeader("Location", "/", true); 
        server.send(302, "text/plain", ""); 
      });
      server.on("/fan/auto", [](){ 
        handleAutoMode(); 
        server.sendHeader("Location", "/", true); 
        server.send(302, "text/plain", ""); 
      });
      #elif DEVICE_TYPE == 3
      server.on("/switch/toggle", [](){ 
        handleToggle(); 
        server.sendHeader("Location", "/", true); 
        server.send(302, "text/plain", ""); 
      });
      server.on("/switch/auto", [](){ 
        handleAutoMode(); 
        server.sendHeader("Location", "/", true); 
        server.send(302, "text/plain", ""); 
      });
      #endif
    #endif
  #endif
  
  if (!otaInitialized) {
    ElegantOTA.begin(&server);
    otaInitialized = true;
    Serial.println("[WEB] ElegantOTA initialized");
  }
  
  server.begin();
  Serial.println("[WEB] Web server started on port 80 (client mode)");
}

void web_initAP() {
  if (apMode) return;
  
  apMode = true;
  
  uint8_t mac[6];
  WiFi.macAddress(mac);
  #if DEVICE_TYPE == 1
    apSSID = "Fan_";
  #elif DEVICE_TYPE == 2
    apSSID = "Sensor_";
  #elif DEVICE_TYPE == 3
    apSSID = "Switch_";
  #else
    apSSID = "Device_";
  #endif
  apSSID += String(mac[4], HEX) + String(mac[5], HEX);
  
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  WiFi.softAP(apSSID.c_str());
  dnsServer.start(DNS_PORT, "*", IPAddress(192, 168, 4, 1));
  Serial.printf("[WEB] AP started: %s, IP: %s\n", apSSID.c_str(), AP_IP_ADDRESS);
  
  #ifdef ESP8266
    ElegantOTA.begin(&server);
    Serial.println("[WEB] ElegantOTA initialized");
    
    server.on("/", [](){ server.send(200, "text/html", web_getConfigPage("")); });
    server.on("/save", web_saveConfig);
  #elif defined(ESP32)
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(200, "text/html", web_getConfigPage("")); });
    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){ web_saveConfig(request); });
    
    if (!otaInitialized) {
      ElegantOTA.begin(&server);
      otaInitialized = true;
      Serial.println("[WEB] ElegantOTA initialized");
    }
  #endif
  
  server.begin();
}

void web_update() {
  #ifdef ESP8266
    server.handleClient();
  #endif
  
  if (apMode) {
    dnsServer.processNextRequest();
  }
}