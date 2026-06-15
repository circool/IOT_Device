// ============================================================================
// @file web.cpp
// @brief Реализация веб-сервера
// ============================================================================

#include "web.h"
#include "logger.h"
#include "sensor.h"
#include "web_templates.h"
#include "wifi_manager.h"

WebServerClass server(80);

// ========== СТАТИЧЕСКИЕ ПЕРЕМЕННЫЕ (КОНТЕКСТ) ==========
static const Config* _cfg = nullptr;
static TransportType _transport = TRANSPORT_NONE;
static uint8_t _deviceType = 0;
static bool _statusPageEnabled = false;
static bool _isApMode = false;

// Данные для отображения статуса
static float _lastTemp = 0;
static float _lastHum = 0;
static bool _actuatorState = false;
static int _actuatorSpeed = -1;

// ========== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ==========
static String formatRemainingTime(unsigned long remainingMs) {
  if (remainingMs <= 0)
    return "0 sec";
  unsigned long remainingSec = remainingMs / 1000;
  if (remainingSec < 60)
    return String(remainingSec) + " sec";
  if (remainingSec < 3600)
    return String(remainingSec / 60) + " min " + String(remainingSec % 60) +
           " sec";
  return String(remainingSec / 3600) + " h " +
         String((remainingSec % 3600) / 60) + " min";
}

// ========== ПУБЛИЧНЫЕ ФУНКЦИИ УСТАНОВКИ КОНТЕКСТА ==========

void web_setTransport(TransportType type) {
  _transport = type;
  LOG_DEBUG(CAT_WEB, "Transport set to: %d", (int)type);
}

void web_setDeviceType(uint8_t type) {
  _deviceType = type;
  LOG_DEBUG(CAT_WEB, "Device type set to: %d", type);
}

void web_setConfig(const Config* cfg) {
  _cfg = cfg;
}

void web_setApMode(bool isApMode) {
  _isApMode = isApMode;
}

void web_enableStatusPage(bool enabled) {
  _statusPageEnabled = enabled;
  LOG_DEBUG(CAT_WEB, "Status page enabled: %s", enabled ? "true" : "false");
}

void web_setSensorData(float temp, float hum) {
  _lastTemp = temp;
  _lastHum = hum;
}

void web_setActuatorState(bool on, int speed) {
  _actuatorState = on;
  if (speed >= 0)
    _actuatorSpeed = speed;
}

// ========== ФОРМИРОВАНИЕ СТРАНИЦЫ СТАТУСА ==========

static String buildStatusHtml() {
  if (!_cfg)
    return "<div class='error'>Config not available</div>";

  String html;

  // Показания датчика (если устройство имеет датчик)
  if (_deviceType == 1 || _deviceType == 2) {
    html += F("<div class='flex-container'>");

    String tempColor = "#2196F3";
    String humColor = "#2196F3";

    if (_deviceType == 1) {  // Fan с датчиком
      if (_lastTemp >= _cfg->highTemp)
        tempColor = "#f44336";
      else if (_lastTemp <= _cfg->lowTemp)
        tempColor = "#4CAF50";
      if (_lastHum >= _cfg->highHum)
        humColor = "#f44336";
      else if (_lastHum <= _cfg->lowHum)
        humColor = "#4CAF50";
    }

    html += "<div class='sensor-card' style='background:" + tempColor +
            "20; border:2px solid " + tempColor + ";'>";
    html += "<div class='sensor-value' style='color:" + tempColor + ";'>" +
            String(_lastTemp, 1) + " °C</div>";
    html += "<div class='sensor-label'>Temperature</div></div>";

    html += "<div class='sensor-card' style='background:" + humColor +
            "20; border:2px solid " + humColor + ";'>";
    html += "<div class='sensor-value' style='color:" + humColor + ";'>" +
            String(_lastHum, 1) + " %</div>";
    html += "<div class='sensor-label'>Humidity</div></div>";

    html += F("</div>");
  }

  // Состояние актуатора
  if (_deviceType == 1 || _deviceType == 3) {
    String stateColor = _actuatorState ? "#f44336" : "#2196F3";
    String stateText = _actuatorState ? "ON" : "OFF";
    const char* label = (_deviceType == 1) ? "Fan" : "Switch";

    html += "<div class='status-card' style='background:" + stateColor +
            "20; border:2px solid " + stateColor + ";'>";
    html += "<div style='font-size:2em;font-weight:bold;color:" + stateColor +
            ";'>" + String(label) + ": " + stateText + "</div>";

    if (_deviceType == 1 && _actuatorState && _actuatorSpeed >= 0) {
      html += "<div style='font-size:1.2em;'>Speed: " + String(_actuatorSpeed) +
              "%</div>";
      html += "<div class='duty-bar'><div class='duty-fill' style='width:" +
              String(_actuatorSpeed) + "%;'></div></div>";
    }
    html += "</div>";
  }

  // Кнопка перехода к настройкам
  html +=
      "<div class='button-group'><a "
      "href='/config'><button>Settings</button></a></div>";

  return html;
}

// ========== ФОРМИРОВАНИЕ СТРАНИЦЫ КОНФИГУРАЦИИ ==========

static void sendConfigPage(const String& errorMsg, const String& successMsg) {
  if (!_cfg)
    return;

  String currentMode = _isApMode ? "Access Point" : "Client WiFi";
  String currentSsid =
      _isApMode ? String(_cfg->deviceId) : String(_cfg->wifiSsid);
  String currentIp = _isApMode ? String(AP_IP_ADDRESS) : wifi_getLocalIP();

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  auto send = [&](const String& chunk) { server.sendContent(chunk); };

  send(FPSTR(HTML_PAGE_START));
  send(F("<title>"));
  send(_cfg->deviceId);
  send(F(" Configuration</title>"));
  send(FPSTR(HTML_STYLE));
  send(F("</head><body><div class='container'>"));

  send(F("<h1>Settings "));
  send(_cfg->deviceId);
  send(F(" v. "));
  send(VERSION);
  send(F("</h1>"));

  send(F("<h3>State</h3><div class='info'>"));
  send(F("Mode: <strong>"));
  send(currentMode);
  send(F("</strong><br>"));
  send(F("SSID: <strong>"));
  send(currentSsid);
  send(F("</strong><br>"));
  send(F("IP: <strong>"));
  send(currentIp);
  send(F("</strong><br>"));
  send(F("</div>"));

  if (errorMsg.length() > 0) {
    send(F("<div class='error'><strong>Error:</strong> "));
    send(errorMsg);
    send(F("</div>"));
  }
  if (successMsg.length() > 0) {
    send(F("<div class='success'><strong>Success:</strong> "));
    send(successMsg);
    send(F("</div>"));
  }

  send(F("<form method='POST' action='/save'>"));

  // ========== WiFi НАСТРОЙКИ (всегда) ==========
  send(F("<h3>WiFi setup</h3>"));
  send(F("<label>WiFi SSID:</label>"));
  send(F("<input type='text' name='wifiSsid' required value='"));
  send(_cfg->wifiSsid);
  send(F("'>"));
  send(F("<label>WiFi Password:</label>"));
  send(F("<input type='password' name='wifiPassword' placeholder='(hidden)'>"));
  send(F(
      "<div class='password-hint'>Leave empty to keep current password</div>"));

  // ========== ТРАНСПОРТНЫЕ НАСТРОЙКИ (в зависимости от _transport) ==========

  send(F("<h3>Transport setup</h3>"));

  switch (_transport) {
    case TRANSPORT_MQTT:
      send(F("<div class='row'><div><label>MQTT Broker:</label>"));
      send(F("<input type='text' name='mqttBroker' value='"));
      send(_cfg->mqttBroker);
      send(F("'></div>"));
      send(F("<div><label>MQTT Port:</label>"));
      send(F("<input type='number' name='mqttPort' value='"));
      send(String(_cfg->mqttPort));
      send(F("'></div></div>"));
      send(F("<div class='row'><div><label>MQTT User:</label>"));
      send(F("<input type='text' name='mqttUser' value='"));
      send(_cfg->mqttUser);
      send(F("'></div>"));
      send(F("<div><label>MQTT Password:</label>"));
      send(
          F("<input type='password' name='mqttPassword' "
            "placeholder='(hidden)'></div></div>"));
      break;

    case TRANSPORT_ZIGBEE:
      send(F("<div class='row'><div><label>Zigbee PAN ID:</label>"));
      send(F("<input type='text' name='zigbeePanId' value='0x1234'></div>"));
      send(F("<div><label>Zigbee Channel:</label>"));
      send(
          F("<input type='number' name='zigbeeChannel' min='11' max='26' "
            "value='15'></div></div>"));
      send(F("<label>Zigbee Device ID:</label>"));
      send(F("<input type='text' name='zigbeeDeviceId' value=''></div>"));
      break;

    case TRANSPORT_MATTER:
      send(F("<label>Matter Setup Code:</label>"));
      send(
          F("<input type='text' name='matterSetupCode' "
            "placeholder='34970112332'>"));
      send(F("<label>Matter Vendor Name:</label>"));
      send(F("<input type='text' name='matterVendorName' value=''>"));
      send(F("<label>Matter Product Name:</label>"));
      send(F("<input type='text' name='matterProductName' value=''>"));
      break;

    case TRANSPORT_NONE:
    default:
      send(F("<div class='warning'>No transport configured</div>"));
      break;
  }

  // ========== НАСТРОЙКИ УСТРОЙСТВА (в зависимости от _deviceType) ==========

  switch (_deviceType) {
    case 1:  // Fan
      send(F("<h3>Sensor</h3>"));
      send(F("<div class='row'><div><label>Low Temp (°C):</label>"));
      send(
          F("<input type='number' step='0.1' min='-40' max='85' name='lowTemp' "
            "value='"));
      send(String(_cfg->lowTemp));
      send(F("'></div>"));
      send(F("<div><label>High Temp (°C):</label>"));
      send(
          F("<input type='number' step='0.1' min='-40' max='85' "
            "name='highTemp' value='"));
      send(String(_cfg->highTemp));
      send(F("'></div></div>"));
      send(F("<div class='row'><div><label>Low Hum (%):</label>"));
      send(
          F("<input type='number' step='0.1' min='0' max='100' name='lowHum' "
            "value='"));
      send(String(_cfg->lowHum));
      send(F("'></div>"));
      send(F("<div><label>High Hum (%):</label>"));
      send(
          F("<input type='number' step='0.1' min='0' max='100' name='highHum' "
            "value='"));
      send(String(_cfg->highHum));
      send(F("'></div></div>"));
      send(F("<label>Sensor interval (sec):</label>"));
      send(F("<input type='number' name='sensorInterval' value='"));
      send(String(_cfg->sensorInterval));
      send(F("'>"));
      send(F("<label>Emergency timeout (sec):</label>"));
      send(F("<input type='number' name='maxOnTime' value='"));
      send(String(_cfg->maxOnTime));
      send(F("'>"));
      send(F("<h3>Control</h3>"));
      send(F("<label>Delay turn on (sec):</label>"));
      send(F("<input type='number' name='delaySeconds' value='"));
      send(String(_cfg->delaySeconds));
      send(F("'>"));
      send(F("<label>Speed (0-100%):</label>"));
      send(
          F("<input type='number' name='speedPercent' min='0' max='100' "
            "value='"));
      send(String(_cfg->speedPercent));
      send(F("'>"));
      send(F("<label><input type='checkbox' name='adaptiveMode' value='1'"));
      if (_cfg->adaptiveMode)
        send(F(" checked"));
      send(F("> Adaptive quiet mode</label><br>"));
      send(F("<label><input type='checkbox' name='bootState' value='1'"));
      if (_cfg->bootState)
        send(F(" checked"));
      send(F("> Turn on at startup</label><br>"));
      send(F(
          "<label><input type='checkbox' name='sensorControlMode' value='1'"));
      if (_cfg->sensorControlMode)
        send(F(" checked"));
      send(F("> Sensor control mode</label><br>"));
      break;

    case 2:  // Sensor
      send(F("<h3>Sensor</h3>"));
      send(F("<label>Reading interval (sec):</label>"));
      send(F("<input type='number' name='sensorInterval' value='"));
      send(String(_cfg->sensorInterval));
      send(F("'>"));
      break;

    case 3:  // Switch
      send(F("<h3>Control</h3>"));
      send(F("<label>Delay turn on (sec):</label>"));
      send(F("<input type='number' name='delaySeconds' value='"));
      send(String(_cfg->delaySeconds));
      send(F("'>"));
      send(F("<label>Emergency timeout (sec):</label>"));
      send(F("<input type='number' name='maxOnTime' value='"));
      send(String(_cfg->maxOnTime));
      send(F("'>"));
      send(F("<label><input type='checkbox' name='bootState' value='1'"));
      if (_cfg->bootState)
        send(F(" checked"));
      send(F("> Turn on at startup</label><br>"));
      break;
  }

  send(
      F("<label><input type='checkbox' name='confirmSave' required> Confirm "
        "saving</label>"));
  send(F("<input type='submit' value='Save and reboot'>"));
  send(F("</form>"));

  if (!_isApMode) {
    send(F("<a href='/' class='link-btn'>Back to Status</a>"));
  }

  send(FPSTR(HTML_PAGE_END));
}

// ========== ОБРАБОТЧИКИ ==========

static void handleRoot() {
  if (_statusPageEnabled && !_isApMode) {
    int refresh = (_cfg && (_deviceType == 1 || _deviceType == 2))
                      ? _cfg->sensorInterval
                      : DEFAULT_WEB_REFRESH;
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");

    auto send = [&](const String& chunk) { server.sendContent(chunk); };

    send(FPSTR(HTML_PAGE_START));
    char refreshMeta[64];
    snprintf_P(refreshMeta, sizeof(refreshMeta),
               PSTR("<meta http-equiv='refresh' content='%d'>"), refresh);
    send(refreshMeta);
    send(F("<title>"));
    send(_cfg ? _cfg->deviceId : "Device");
    send(F("</title>"));
    send(FPSTR(HTML_STYLE));
    send(F("</head><body><div class='container'>"));
    send(F("<h1>"));
    send(_cfg ? _cfg->deviceId : "Device");
    send(F(" VERSION "));
    send(VERSION);
    send(F("</h1>"));
    send(buildStatusHtml());
    send(FPSTR(HTML_PAGE_END));
  } else {
    sendConfigPage("", "");
  }
}

static void handleConfig() {
  sendConfigPage("", "");
}

static void handleSave() {
  if (!_cfg) {
    server.send(500, "text/plain", "Config not available");
    return;
  }

  Config newConfig = *_cfg;

  // WiFi настройки
  if (server.hasArg("wifiSsid")) {
    strncpy(newConfig.wifiSsid, server.arg("wifiSsid").c_str(),
            sizeof(newConfig.wifiSsid) - 1);
    newConfig.wifiSsid[sizeof(newConfig.wifiSsid) - 1] = '\0';
  }
  if (server.hasArg("wifiPassword") &&
      server.arg("wifiPassword").length() > 0) {
    strncpy(newConfig.wifiPassword, server.arg("wifiPassword").c_str(),
            sizeof(newConfig.wifiPassword) - 1);
    newConfig.wifiPassword[sizeof(newConfig.wifiPassword) - 1] = '\0';
  }

  // Транспортные настройки (в зависимости от _transport)
  switch (_transport) {
    case TRANSPORT_MQTT:
      if (server.hasArg("mqttBroker")) {
        strncpy(newConfig.mqttBroker, server.arg("mqttBroker").c_str(),
                sizeof(newConfig.mqttBroker) - 1);
        newConfig.mqttBroker[sizeof(newConfig.mqttBroker) - 1] = '\0';
      }
      if (server.hasArg("mqttPort")) {
        newConfig.mqttPort = server.arg("mqttPort").toInt();
      }
      if (server.hasArg("mqttUser")) {
        strncpy(newConfig.mqttUser, server.arg("mqttUser").c_str(),
                sizeof(newConfig.mqttUser) - 1);
        newConfig.mqttUser[sizeof(newConfig.mqttUser) - 1] = '\0';
      }
      if (server.hasArg("mqttPassword") &&
          server.arg("mqttPassword").length() > 0) {
        strncpy(newConfig.mqttPassword, server.arg("mqttPassword").c_str(),
                sizeof(newConfig.mqttPassword) - 1);
        newConfig.mqttPassword[sizeof(newConfig.mqttPassword) - 1] = '\0';
      }
      break;

    case TRANSPORT_ZIGBEE:
      // TODO: парсинг Zigbee полей
      break;

    case TRANSPORT_MATTER:
      // TODO: парсинг Matter полей
      break;

    default:
      break;
  }

  // Настройки устройства
  switch (_deviceType) {
    case 1:  // Fan
      if (server.hasArg("sensorInterval"))
        newConfig.sensorInterval = server.arg("sensorInterval").toInt();
      if (server.hasArg("lowTemp"))
        newConfig.lowTemp = server.arg("lowTemp").toFloat();
      if (server.hasArg("highTemp"))
        newConfig.highTemp = server.arg("highTemp").toFloat();
      if (server.hasArg("lowHum"))
        newConfig.lowHum = server.arg("lowHum").toFloat();
      if (server.hasArg("highHum"))
        newConfig.highHum = server.arg("highHum").toFloat();
      if (server.hasArg("speedPercent"))
        newConfig.speedPercent = server.arg("speedPercent").toInt();
      if (server.hasArg("delaySeconds"))
        newConfig.delaySeconds = server.arg("delaySeconds").toInt();
      if (server.hasArg("maxOnTime"))
        newConfig.maxOnTime = server.arg("maxOnTime").toInt();
      newConfig.sensorControlMode = server.hasArg("sensorControlMode");
      newConfig.adaptiveMode = server.hasArg("adaptiveMode");
      newConfig.bootState = server.hasArg("bootState");
      break;

    case 2:  // Sensor
      if (server.hasArg("sensorInterval"))
        newConfig.sensorInterval = server.arg("sensorInterval").toInt();
      break;

    case 3:  // Switch
      if (server.hasArg("delaySeconds"))
        newConfig.delaySeconds = server.arg("delaySeconds").toInt();
      if (server.hasArg("maxOnTime"))
        newConfig.maxOnTime = server.arg("maxOnTime").toInt();
      newConfig.bootState = server.hasArg("bootState");
      break;
  }

  extern bool config_updateFromWeb(const Config&);
  if (config_updateFromWeb(newConfig)) {
    sendConfigPage("", "Configuration saved, rebooting...");
  } else {
    sendConfigPage("Failed to save configuration", "");
  }
}

void web_init() {
  LOG_INFO(CAT_WEB, "Initializing web server...");

  server.on("/", handleRoot);
  server.on("/config", handleConfig);
  server.on("/save", handleSave);
  server.on("/favicon.ico", []() { server.send(404); });

  server.begin();
  LOG_INFO(CAT_WEB, "Web server started");
}

void web_update() {
  server.handleClient();
}