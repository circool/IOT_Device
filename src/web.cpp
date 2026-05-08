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
  
  class CaptiveRequestHandler : public RequestHandler {
  public:
    CaptiveRequestHandler() {}
    bool canHandle(HTTPMethod method, String uri) { return true; }
    bool handle(ESP8266WebServer& server, HTTPMethod requestMethod, String requestUri) {
      server.sendHeader("Location", "http://" + String(AP_IP_ADDRESS) + "/", true);
      server.send(302, "text/plain", "");
      return true;
    }
  };
#endif

String apSSID;

// Обработчик переключения (общий для TYPE 1 и 3)
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
void handleToggle() {
  fan_setOverrideMode(true);
  fan_set(!fan_getState());
  mqtt_publishState();
}
#endif

String web_getConfigPage() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Smart Fan Configuration</title>";
  html += "<style>";
  html += "body{font-family:Arial;margin:20px;background:#f0f0f0;}";
  html += ".container{max-width:700px;margin:auto;background:white;padding:20px;border-radius:10px;}";
  html += "h1{color:#2c3e50;}h3{color:#2c3e50;border-bottom:1px solid #ccc;padding-bottom:5px;}";
  html += "label{display:block;margin-top:10px;font-weight:bold;}";
  html += "input[type=text],input[type=password],input[type=number]{width:100%;padding:8px;margin:5px 0;border:1px solid #ccc;border-radius:4px;font-size:1.2em}";
  html += "input[type=checkbox]{width:20px;height:20px;margin-right:10px;vertical-align:middle;cursor:pointer;transform:scale(1.5);}";
  html += "input[type=submit]{background:#2c3e50;color:white;padding:10px 20px;margin-top:20px;border:none;border-radius:4px;cursor:pointer;width:100%;font-size:1em}";
  html += "input[type=submit]:hover{background:#1a252f;}";
  html += ".info{background:#e7f3ff;padding:10px;border-radius:5px;margin:10px 0;}";
  html += ".warning{background:#fff3cd;padding:10px;border-radius:5px;margin:10px 0;color:#856404;}";
  html += ".row{display:flex;gap:10px;}.row>div{flex:1;}";
  html += ".password-hint{color:#7f8c8d;margin-top:-2px;margin-bottom:8px;}";
  html += "</style></head><body><div class='container'>";
  html += "<h1>Настройка устройства</h1>";
  html += "<div class='info'><strong>Текущее состояние</strong><br>";
  if (apMode) {  
    html += "Точка доступа: <strong>" + apSSID + "</strong><br>";
    html += "IP адрес: <strong>" + String(AP_IP_ADDRESS) + "</strong><br>";
    html += "Режим: <strong>Точка доступа (AP)</strong>";
  } else {
    html += "WiFi сеть: <strong>" + String(config.wifiSsid) + "</strong><br>";
    html += "IP адрес: <strong>" + WiFi.localIP().toString() + "</strong><br>";
    html += "Режим: <strong>Клиент WiFi</strong>";
  }
  html += "</div><form method='POST' action='/save'>";
  
  // Общие настройки сети и MQTT
  html += "<h3>Настройки сети</h3>";
  html += "<label>WiFi SSID:</label><input type='text' name='wifiSsid' required value='" + String(config.wifiSsid) + "'>";
  html += "<label>WiFi Password:</label><input type='password' name='wifiPassword' placeholder='(не показан)'>";
  html += "<div class='password-hint'>Оставьте пустым, чтобы сохранить текущий пароль WiFi</div>";
  
  html += "<h3>MQTT настройки</h3>";
  html += "<div class='row'><div><label>MQTT Broker:</label><input type='text' name='mqttBroker' value='" + String(config.mqttBroker) + "'></div>";
  html += "<div><label>MQTT Port:</label><input type='number' name='mqttPort' value='" + String(config.mqttPort) + "'></div></div>";
  html += "<div class='row'><div><label>MQTT User:</label><input type='text' name='mqttUser' value='" + String(config.mqttUser) + "'></div>";
  html += "<div><label>MQTT Password:</label><input type='password' name='mqttPassword' placeholder='(не показан)'></div></div>";
  html += "<div class='password-hint'>Оставьте пустым, чтобы сохранить текущий пароль MQTT</div>";
  html += "<label>MQTT Client ID:</label><input type='text' name='mqttClientId' value='" + String(config.mqttClientId) + "'>";
  
  // DEVICE_TYPE 1: вентилятор с датчиками
  #if DEVICE_TYPE == 1
  html += "<h3>Настройки датчиков</h3>";
  html += "<div class='row'><div><label>Low Temp (°C):</label><input type='number' step='0.1' name='lowTemp' value='" + String(config.lowTemp) + "'></div>";
  html += "<div><label>High Temp (°C):</label><input type='number' step='0.1' name='highTemp' value='" + String(config.highTemp) + "'></div></div>";
  html += "<div class='row'><div><label>Low Hum (%):</label><input type='number' step='0.1' name='lowHum' value='" + String(config.lowHum) + "'></div>";
  html += "<div><label>High Hum (%):</label><input type='number' step='0.1' name='highHum' value='" + String(config.highHum) + "'></div></div>";
  html += "<div class='row'><div><label>Интервал датчика (сек):</label><input type='number' name='sensorInterval' min='2' max='300' value='" + String(config.sensorInterval) + "'></div>";
  html += "<div><label>maxOnTime (сек):</label><input type='number' name='maxOnTime' min='60' max='86400' value='" + String(config.maxOnTime) + "'></div></div>";
  html += "<h3>Управление</h3>";
  html += "<label>Задержка ВКЛЮЧЕНИЯ (сек):</label><input type='number' name='delaySeconds' min='0' max='3600' value='" + String(config.delaySeconds) + "'>";
  html += "<h3>Slow Mode</h3>";
  html += "<label><input type='checkbox' name='slowModeEnabled' value='1' " + String(config.slowModeEnabled ? "checked" : "") + "> Включить</label>";
  html += "<label>Скважность (0-255):</label><input type='number' name='slowModeDuty' min='0' max='255' value='" + String(config.slowModeDuty) + "'>";
  html += "<h3>Поведение при старте</h3>";
  html += "<label><input type='checkbox' name='forceOffOnBoot' value='1' " + String(config.forceOffOnBoot ? "checked" : "") + "> Принудительно выключать при старте</label>";
  html += "<h3>Режимы работы</h3>";
  html += "<label><input type='checkbox' name='automaticMode' value='1' " + String(config.automaticMode ? "checked" : "") + "> Автоматический режим</label>";
  #endif
  
  // DEVICE_TYPE 2: только датчик
  #if DEVICE_TYPE == 2
  html += "<h3>Настройки датчиков</h3>";
  html += "<div><label>Интервал датчика (сек):</label><input type='number' name='sensorInterval' min='2' max='300' value='" + String(config.sensorInterval) + "'></div>";
  #endif
  
  // DEVICE_TYPE 3: управляемый выключатель
  #if DEVICE_TYPE == 3
  html += "<h3>Настройки управления</h3>";
  html += "<label>Задержка ВКЛЮЧЕНИЯ (сек):</label><input type='number' name='delaySeconds' min='0' max='3600' value='" + String(config.delaySeconds) + "'>";
  html += "<label>maxOnTime (сек):</label><input type='number' name='maxOnTime' min='60' max='86400' value='" + String(config.maxOnTime) + "'>";
  html += "<h3>Slow Mode</h3>";
  html += "<label><input type='checkbox' name='slowModeEnabled' value='1' " + String(config.slowModeEnabled ? "checked" : "") + "> Включить</label>";
  html += "<label>Скважность (0-255):</label><input type='number' name='slowModeDuty' min='0' max='255' value='" + String(config.slowModeDuty) + "'>";
  html += "<h3>Поведение при старте</h3>";
  html += "<label><input type='checkbox' name='forceOffOnBoot' value='1' " + String(config.forceOffOnBoot ? "checked" : "") + "> Принудительно выключать при старте</label>";
  #endif
  
  html += "<input type='submit' value='Сохранить и перезагрузить'>";
  html += "</form>";
  html += "<div class='warning'>После сохранения устройство перезагрузится.</div>";
  html += "</div></body></html>";
  
  return html;
}

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
  html += ".button-group{display:flex;justify-content:center;gap:10px;margin-top:20px;}";
  html += "a{text-decoration:none;}";
  html += "</style></head><body><div class='container'>";
  
  html += "<h1>" + String(config.mqttClientId) + "</h1>";
  
  // Датчики (TYPE 1 и 2)
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
  
  // Вентилятор (TYPE 1) или Выключатель (TYPE 3)
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  bool state = fan_getRealState();
  String stateColor = state ? "#f44336" : "#2196F3";
  String stateText = state ? "ВКЛ" : "ВЫКЛ";
  #if DEVICE_TYPE == 1
  String label = "Вентилятор";
  String toggleUrl = "/fan/toggle";
  #else
  String label = "Выключатель";
  String toggleUrl = "/switch/toggle";
  #endif
  
  html += "<a href='" + toggleUrl + "'>";
  html += "<div class='status-card' style='background:" + stateColor + "20; border:2px solid " + stateColor + ";'>";
  html += "<div style='font-size:2em;font-weight:bold;color:" + stateColor + ";'>" + label + ": " + stateText + "</div></div>";
  html += "</a>";
  #endif
  
  // Информация
  html += "<hr><div class='info'>Обновление: " + String(refreshInterval) + " сек<br>";
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  html += "Опрос датчика: " + String(config.sensorInterval) + " сек<br>";
  #endif
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  html += "Задержка вкл: " + String(config.delaySeconds) + " сек<br>";
  html += "maxOnTime: " + String(config.maxOnTime) + " сек</div>";
  #endif
  
  html += "<div class='button-group'>";
  html += "<a href='/update'><button>Обновить прошивку (OTA)</button></a>";
  html += "<a href='/config'><button>Настройки</button></a>";
  html += "</div></div></body></html>";
  
  return html;
}

#ifdef ESP32
void web_saveConfig(AsyncWebServerRequest *request) {
  if (request->hasParam("wifiSsid", true))
    request->getParam("wifiSsid", true)->value().toCharArray(config.wifiSsid, sizeof(config.wifiSsid));
  if (request->hasParam("wifiPassword", true)) {
    String pwd = request->getParam("wifiPassword", true)->value();
    if (pwd.length() > 0) pwd.toCharArray(config.wifiPassword, sizeof(config.wifiPassword));
  }
  if (request->hasParam("mqttBroker", true))
    request->getParam("mqttBroker", true)->value().toCharArray(config.mqttBroker, sizeof(config.mqttBroker));
  if (request->hasParam("mqttPort", true))
    config.mqttPort = request->getParam("mqttPort", true)->value().toInt();
  if (request->hasParam("mqttUser", true))
    request->getParam("mqttUser", true)->value().toCharArray(config.mqttUser, sizeof(config.mqttUser));
  if (request->hasParam("mqttPassword", true)) {
    String pwd = request->getParam("mqttPassword", true)->value();
    if (pwd.length() > 0) pwd.toCharArray(config.mqttPassword, sizeof(config.mqttPassword));
  }
  if (request->hasParam("mqttClientId", true)) {
    String cid = request->getParam("mqttClientId", true)->value();
    if (cid.length() > 0) cid.toCharArray(config.mqttClientId, sizeof(config.mqttClientId));
    else config.mqttClientId[0] = '\0';
  }
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  if (request->hasParam("sensorInterval", true)) {
    config.sensorInterval = request->getParam("sensorInterval", true)->value().toInt();
    if (config.sensorInterval < 2) config.sensorInterval = 2;
    if (config.sensorInterval > 300) config.sensorInterval = 300;
  }
  #endif
  
  #if DEVICE_TYPE == 1
  if (request->hasParam("lowTemp", true)) config.lowTemp = request->getParam("lowTemp", true)->value().toFloat();
  if (request->hasParam("highTemp", true)) config.highTemp = request->getParam("highTemp", true)->value().toFloat();
  if (request->hasParam("lowHum", true)) config.lowHum = request->getParam("lowHum", true)->value().toFloat();
  if (request->hasParam("highHum", true)) config.highHum = request->getParam("highHum", true)->value().toFloat();
  if (request->hasParam("maxOnTime", true)) {
    config.maxOnTime = request->getParam("maxOnTime", true)->value().toInt();
    if (config.maxOnTime < 60) config.maxOnTime = 60;
    if (config.maxOnTime > 86400) config.maxOnTime = 86400;
  }
  if (request->hasParam("delaySeconds", true)) {
    config.delaySeconds = request->getParam("delaySeconds", true)->value().toInt();
    if (config.delaySeconds < 0) config.delaySeconds = 0;
    if (config.delaySeconds > 3600) config.delaySeconds = 3600;
  }
  config.slowModeEnabled = request->hasParam("slowModeEnabled", true);
  if (request->hasParam("slowModeDuty", true)) {
    config.slowModeDuty = request->getParam("slowModeDuty", true)->value().toInt();
    if (config.slowModeDuty > 255) config.slowModeDuty = 255;
  }
  config.forceOffOnBoot = request->hasParam("forceOffOnBoot", true);
  config.automaticMode = request->hasParam("automaticMode", true);
  #endif
  
  #if DEVICE_TYPE == 3
  if (request->hasParam("maxOnTime", true)) {
    config.maxOnTime = request->getParam("maxOnTime", true)->value().toInt();
    if (config.maxOnTime < 60) config.maxOnTime = 60;
    if (config.maxOnTime > 86400) config.maxOnTime = 86400;
  }
  if (request->hasParam("delaySeconds", true)) {
    config.delaySeconds = request->getParam("delaySeconds", true)->value().toInt();
    if (config.delaySeconds < 0) config.delaySeconds = 0;
    if (config.delaySeconds > 3600) config.delaySeconds = 3600;
  }
  config.slowModeEnabled = request->hasParam("slowModeEnabled", true);
  if (request->hasParam("slowModeDuty", true)) {
    config.slowModeDuty = request->getParam("slowModeDuty", true)->value().toInt();
    if (config.slowModeDuty > 255) config.slowModeDuty = 255;
  }
  config.forceOffOnBoot = request->hasParam("forceOffOnBoot", true);
  #endif
  
  if (strlen(config.wifiSsid) == 0 || strlen(config.mqttBroker) == 0) {
    request->send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='3;url=/'></head><body><div style='text-align:center;margin-top:50px;'><h2 style='color:#f44336;'>Ошибка!</h2><p>SSID и MQTT Broker обязательны!</p></div></body></html>");
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
    if (cid.length() > 0) cid.toCharArray(config.mqttClientId, sizeof(config.mqttClientId));
    else config.mqttClientId[0] = '\0';
  }
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  if (server.hasArg("sensorInterval")) {
    config.sensorInterval = server.arg("sensorInterval").toInt();
    if (config.sensorInterval < 2) config.sensorInterval = 2;
    if (config.sensorInterval > 300) config.sensorInterval = 300;
  }
  #endif
  
  #if DEVICE_TYPE == 1
  if (server.hasArg("lowTemp")) config.lowTemp = server.arg("lowTemp").toFloat();
  if (server.hasArg("highTemp")) config.highTemp = server.arg("highTemp").toFloat();
  if (server.hasArg("lowHum")) config.lowHum = server.arg("lowHum").toFloat();
  if (server.hasArg("highHum")) config.highHum = server.arg("highHum").toFloat();
  if (server.hasArg("maxOnTime")) {
    config.maxOnTime = server.arg("maxOnTime").toInt();
    if (config.maxOnTime < 60) config.maxOnTime = 60;
    if (config.maxOnTime > 86400) config.maxOnTime = 86400;
  }
  if (server.hasArg("delaySeconds")) {
    config.delaySeconds = server.arg("delaySeconds").toInt();
    if (config.delaySeconds < 0) config.delaySeconds = 0;
    if (config.delaySeconds > 3600) config.delaySeconds = 3600;
  }
  config.slowModeEnabled = server.hasArg("slowModeEnabled");
  if (server.hasArg("slowModeDuty")) {
    config.slowModeDuty = server.arg("slowModeDuty").toInt();
    if (config.slowModeDuty > 255) config.slowModeDuty = 255;
  }
  config.forceOffOnBoot = server.hasArg("forceOffOnBoot");
  config.automaticMode = server.hasArg("automaticMode");
  #endif
  
  #if DEVICE_TYPE == 3
  if (server.hasArg("maxOnTime")) {
    config.maxOnTime = server.arg("maxOnTime").toInt();
    if (config.maxOnTime < 60) config.maxOnTime = 60;
    if (config.maxOnTime > 86400) config.maxOnTime = 86400;
  }
  if (server.hasArg("delaySeconds")) {
    config.delaySeconds = server.arg("delaySeconds").toInt();
    if (config.delaySeconds < 0) config.delaySeconds = 0;
    if (config.delaySeconds > 3600) config.delaySeconds = 3600;
  }
  config.slowModeEnabled = server.hasArg("slowModeEnabled");
  if (server.hasArg("slowModeDuty")) {
    config.slowModeDuty = server.arg("slowModeDuty").toInt();
    if (config.slowModeDuty > 255) config.slowModeDuty = 255;
  }
  config.forceOffOnBoot = server.hasArg("forceOffOnBoot");
  #endif
  
  if (strlen(config.wifiSsid) == 0 || strlen(config.mqttBroker) == 0) {
    server.send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='3;url=/'></head><body><div style='text-align:center;margin-top:50px;'><h2 style='color:#f44336;'>Ошибка!</h2><p>SSID и MQTT Broker обязательны!</p></div></body></html>");
    return;
  }
  
  config_write();
  server.send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'></head><body><div style='text-align:center;margin-top:50px;'><h2>Настройки сохранены!</h2><p>Перезагрузка...</p></div></body></html>");
  delay(1000);
  ESP.restart();
}
#endif

void web_init() {
  // Клиентский режим - обычный веб-сервер
  int refreshInterval = 2;
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  refreshInterval = config.sensorInterval / 2;
  if (refreshInterval < 2) refreshInterval = 2;
  if (refreshInterval > 10) refreshInterval = 10;
  #endif
  
  #ifdef ESP8266
    server.on("/", [refreshInterval](){ server.send(200, "text/html", web_getStatusPage(refreshInterval)); });
    server.on("/config", [](){ server.send(200, "text/html", web_getConfigPage()); });
    server.on("/save", web_saveConfig);
    server.on("/resetall", [](){
      config_clear();
      server.send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'></head><body><h2>Настройки сброшены, перезагрузка...</h2></body></html>");
      delay(1000);
      ESP.restart();
    });
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    server.on("/fan/toggle", [](){ handleToggle(); server.sendHeader("Location", "/", true); server.send(302, "text/plain", ""); });
    server.on("/switch/toggle", [](){ handleToggle(); server.sendHeader("Location", "/", true); server.send(302, "text/plain", ""); });
    #endif
  #elif defined(ESP32)
    server.on("/", HTTP_GET, [refreshInterval](AsyncWebServerRequest *request){ request->send(200, "text/html", web_getStatusPage(refreshInterval)); });
    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(200, "text/html", web_getConfigPage()); });
    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){ web_saveConfig(request); });
    server.on("/resetall", HTTP_GET, [](AsyncWebServerRequest *request){
      config_clear();
      request->send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'></head><body><h2>Настройки сброшены, перезагрузка...</h2></body></html>");
      delay(1000);
      ESP.restart();
    });
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    server.on("/fan/toggle", HTTP_GET, [](AsyncWebServerRequest *request){ handleToggle(); request->redirect("/"); });
    server.on("/switch/toggle", HTTP_GET, [](AsyncWebServerRequest *request){ handleToggle(); request->redirect("/"); });
    #endif
  #endif
  
  ElegantOTA.begin(&server);
  Serial.println("[WEB] ElegantOTA initialized");
  
  server.begin();
  Serial.println("[WEB] Web server started on port 80 (client mode)");
}

void web_initAP() {
  if (apMode) return;
  
  apMode = true;
  
  uint8_t mac[6];
  WiFi.macAddress(mac);
  apSSID = "SmartFan_" + String(mac[4], HEX) + String(mac[5], HEX);
  
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSSID.c_str());
  WiFi.softAPConfig(IPAddress(192, 168, 4, 1), IPAddress(192, 168, 4, 1), IPAddress(255, 255, 255, 0));
  
  dnsServer.start(DNS_PORT, "*", IPAddress(192, 168, 4, 1));
  
  Serial.printf("[WEB] AP started: %s, IP: %s\n", apSSID.c_str(), AP_IP_ADDRESS);
  
  #ifdef ESP8266
    server.on("/", [](){ server.send(200, "text/html", web_getConfigPage()); });
    server.on("/save", web_saveConfig);
    server.on("/resetall", [](){
      config_clear();
      server.send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=http://" + String(AP_IP_ADDRESS) + "/'></head><body><h2>Настройки сброшены, перезагрузка...</h2></body></html>");
      delay(1000);
      ESP.restart();
    });
    server.addHandler(new CaptiveRequestHandler());
  #elif defined(ESP32)
    
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(200, "text/html", web_getConfigPage()); });
    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){ web_saveConfig(request); });
    server.on("/resetall", HTTP_GET, [](AsyncWebServerRequest *request){
      config_clear();
      request->send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=http://" + String(AP_IP_ADDRESS) + "/'></head><body><h2>Настройки сброшены, перезагрузка...</h2></body></html>");
      delay(1000);
      ESP.restart();
    });
    server.onNotFound([](AsyncWebServerRequest *request){
      request->redirect("http://" + String(AP_IP_ADDRESS) + "/");
    });
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