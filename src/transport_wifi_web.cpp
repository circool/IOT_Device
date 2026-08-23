/**
 * @file transport_wifi_web.cpp
 * @version 0.12
 * @brief Web-менеджер — реализация HTTP-сервера
 */

#include "transport_wifi_web.h"
#include "html_templates.h"
#include "settings.h"
#include "web_common.h"

#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI

// ============================================================================
// КОНСТРУКТОР
// ============================================================================

WebManager::WebManager()
    : _server(nullptr),
      _transportConfig(nullptr),
      _deviceConfig(nullptr),
      _deviceState(nullptr),
      _transportState(nullptr),
      _rssi(-99),
      _running(false),
      _connected(false),
      _provisioningMode(false),
      _serverStarted(false),
      _wifiReady(false),
      _eventCallback(nullptr),
      _eventContext(nullptr),
      configPending(false),
      restartPending(false),
      commandPending(false) {
  memset(&pendingConfig, 0, sizeof(pendingConfig));
  memset(&pendingDeviceConfig, 0, sizeof(pendingDeviceConfig));
}

// ============================================================================
// УПРАВЛЕНИЕ
// ============================================================================

bool WebManager::begin(const TransportConfig* transportConfig,
                       const DeviceConfig* deviceConfig,
                       const DeviceState* deviceState,
                       const TransportState* transportState) {
  if (!transportConfig || !deviceConfig || !deviceState || !transportState) {
    XLOG_ERROR(CAT_WEB, "One or more config pointers are NULL");
    return false;
  }

  _transportConfig = transportConfig;
  _deviceConfig = deviceConfig;
  _deviceState = deviceState;
  _transportState = transportState;

  _running = true;
  _connected = true;
  _provisioningMode = false;
  _serverStarted = false;
  _wifiReady = false;

  XLOG_DEBUG(CAT_WEB, "creating WebServer instance");

  _server = new WebServerClass(80);
  if (!_server) {
    XLOG_ERROR(CAT_WEB, "Failed to create WebServer");
    _running = false;
    _connected = false;
    return false;
  }

  setupRoutes();
  web_ota_manager_init(_server);

  XLOG_DEBUG(CAT_WEB, "Web server instance created (waiting for WiFi)");
  return true;
}

void WebManager::startServer() {
  if (_serverStarted)
    return;
  if (!_server || !_running)
    return;

  if (!_wifiReady) {
    return;
  }

  XLOG_INFO(CAT_WEB, "WiFi ready, starting Web server...");
  _server->begin();
  _serverStarted = true;

  XLOG_INFO(CAT_WEB, "Web server started on port 80");
  XLOG_DEBUG(CAT_WEB, "Pages: / (status), /config (settings), /set (commands)");
  XLOG_DEBUG(CAT_WEB, "Device ID: %s", getDeviceId());
#if FEATURE_MQTT_ENABLED == 1
  XLOG_DEBUG(
      CAT_WEB, "MQTT Broker: %s:%d",
      _transportConfig->mqttBroker ? _transportConfig->mqttBroker : "NONE",
      _transportConfig->mqttPort);
#endif
}

void WebManager::update() {
  if (!_running || !_server)
    return;

  if (!_serverStarted) {
    startServer();
    return;
  }

  _server->handleClient();

  static unsigned long lastLog = 0;
  if (millis() - lastLog > 10000) {
    lastLog = millis();
    XLOG_DEBUG(CAT_WEB, "State: %s, Speed: %d%%, Temp: %.1f°C, Hum: %.1f%%",
               _deviceState->isOn ? "ON" : "OFF", _deviceState->speed,
               _deviceState->temperature, _deviceState->humidity);
  }
}

bool WebManager::isConnected() const {
  return _connected && _running;
}

void WebManager::disconnect() {
  XLOG_DEBUG(CAT_WEB, "Try disconnect...");
  if (_server) {
    _server->stop();
    delete _server;
    _server = nullptr;
  }
  _running = false;
  _connected = false;
}

const char* WebManager::getName() const {
  return "Web";
}

// ============================================================================
// РЕЖИМ AP
// ============================================================================

void WebManager::enableProvisioningMode() {
  _provisioningMode = true;
  XLOG_DEBUG(CAT_WEB, "Provisioning mode enabled");
}

void WebManager::disableProvisioningMode() {
  _provisioningMode = false;
  XLOG_DEBUG(CAT_WEB, "Provisioning mode disabled");
}

bool WebManager::isProvisioningMode() const {
  return _provisioningMode;
}

// ============================================================================
// RSSI (обновляется из WiFiTransport)
// ============================================================================

void WebManager::setRSSI(int rssi) {
  _rssi = rssi;
  XLOG_DEBUG(CAT_WEB, "RSSI updated: %d dBm", rssi);
}

// ============================================================================
// КОЛБЭКИ
// ============================================================================

void WebManager::onEvent(TransportEventCallback callback, void* context) {
  _eventCallback = callback;
  _eventContext = context;
  XLOG_DEBUG(CAT_WEB, "onEvent() registered");
}

// ============================================================================
// НАСТРОЙКА МАРШРУТОВ
// ============================================================================

void WebManager::setupRoutes() {
  _server->on("/", std::bind(&WebManager::handleRoot, this));
  _server->on("/config", std::bind(&WebManager::handleConfig, this));
  _server->on("/save", HTTP_POST, std::bind(&WebManager::handleSave, this));
  _server->on("/savewifi", HTTP_POST,
              std::bind(&WebManager::handleSaveWifi, this));
  _server->on("/set", HTTP_GET, std::bind(&WebManager::handleSet, this));
  _server->onNotFound(std::bind(&WebManager::handleNotFound, this));
}

// ============================================================================
// ОБРАБОТЧИКИ
// ============================================================================

void WebManager::handleRoot() {
  if (!_server)
    return;

  if (_provisioningMode) {
    XLOG_DEBUG(CAT_WEB, "GET / (provisioning mode)");
    sendApProvisioningPage();
    return;
  }

  XLOG_DEBUG(CAT_WEB, "GET / (normal mode)");

  web_sendPageStart(
      [](const char* chunk, void* context) {
        WebManager* mgr = static_cast<WebManager*>(context);
        if (mgr && mgr->_server)
          mgr->_server->sendContent(chunk);
      },
      this, "Device Status", PAGE_MODE_NORMAL);

  char block[512];
  char temp[512];

  const DeviceState& state = *_deviceState;
  const DeviceConfig& cfg = *_deviceConfig;
  const TransportState& tState = *_transportState;

  snprintf(temp, sizeof(temp), "<h1>%s %s</h1>", getDeviceId(), VERSION);
  _server->sendContent(temp);

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  char val[16];
  snprintf(val, sizeof(val), "%.1f°C", state.temperature);
  TextBlockParams t{"Temperature", val, "", "info"};
  render(block, sizeof(block), t);
  _server->sendContent(block);

  snprintf(val, sizeof(val), "%.1f%%", state.humidity);
  TextBlockParams h{"Humidity", val, "", "info"};
  render(block, sizeof(block), h);
  _server->sendContent(block);
#endif

  StatusBlockParams s{state.isOn, state.isOn ? "ON" : "OFF"};
  render(block, sizeof(block), s);
  _server->sendContent(block);

  if (state.speed > 0) {
    char sp[16];
    snprintf(sp, sizeof(sp), "%d%%", state.speed);
    TextBlockParams spd{"Speed", sp, "", "info"};
    render(block, sizeof(block), spd);
    _server->sendContent(block);
  }

  TextBlockParams mode{"Mode", cfg.sensorMode ? "AUTO" : "MANUAL", "", "info"};
  render(block, sizeof(block), mode);
  _server->sendContent(block);

  if (cfg.adaptiveMode) {
    TextBlockParams ad{"Adaptive", "ON", "", "success"};
    render(block, sizeof(block), ad);
    _server->sendContent(block);
  }

  snprintf(temp, sizeof(temp),
           "<div class='info-block'>"
           "<h3>System Info</h3>"
           "<table>"
           "<tr><td>Device ID</td><td>%s</td></tr>"
           "<tr><td>Version</td><td>%s</td></tr>"
           "<tr><td>WiFi</td><td>%s</td></tr>"
           "<tr><td>RSSI</td><td>%d dBm</td></tr>"
           "<tr><td>Reset Reason</td><td>N/A</td></tr>"
           "</table>"
           "</div>",
           getDeviceId(), VERSION,
           tState.link_ok ? "Connected" : "Disconnected", _rssi);
  _server->sendContent(temp);

  _server->sendContent("<div style='margin-top:20px;'>");
  ButtonParams settings{"Settings", "/config", "link-btn"};
  render(block, sizeof(block), settings);
  _server->sendContent(block);
  ButtonParams toggle{state.isOn ? "Turn OFF" : "Turn ON",
                      state.isOn ? "/set?state=off" : "/set?state=on",
                      "link-btn"};
  render(block, sizeof(block), toggle);
  _server->sendContent(block);
  _server->sendContent("</div>");

  web_sendPageEnd(
      [](const char* chunk, void* context) {
        WebManager* mgr = static_cast<WebManager*>(context);
        if (mgr && mgr->_server)
          mgr->_server->sendContent(chunk);
      },
      this);
}

void WebManager::handleConfig() {
  if (!_server)
    return;
  if (_provisioningMode) {
    _server->send(404, "text/plain", "Not available in provisioning mode");
    return;
  }
  XLOG_DEBUG(CAT_WEB, "GET /config");
  sendConfigPage();
}

// ============================================================================
// handleSave() — сохранение настроек устройства с /config
// ============================================================================

void WebManager::handleSave() {
  if (!_server)
    return;
  if (_provisioningMode) {
    _server->send(403, "text/plain", "Forbidden in provisioning mode");
    return;
  }

  XLOG_DEBUG(CAT_WEB, "POST /save");

  if (!_server->hasArg("confirmSave") || _server->arg("confirmSave") != "1") {
    sendConfigPage("Please confirm settings by checking the confirmation box.");
    return;
  }

  const DeviceConfig& cfg = *_deviceConfig;
  DeviceConfig newDeviceConfig = cfg;
  bool valid = true;
  char errorMsg[128] = {0};

#if DEVICE_TYPE == 1
  // Temperature thresholds
  if (_server->hasArg("lowTemp")) {
    float val = _server->arg("lowTemp").toFloat();
    if (val >= TEMP_MIN && val <= TEMP_MAX) {
      newDeviceConfig.lowTemp = val;
    } else if (_server->arg("lowTemp").length() > 0) {
      valid = false;
      snprintf(errorMsg, sizeof(errorMsg), "Low temperature must be %.1f..%.1f",
               TEMP_MIN, TEMP_MAX);
    }
  }

  if (_server->hasArg("highTemp")) {
    float val = _server->arg("highTemp").toFloat();
    if (val >= TEMP_MIN && val <= TEMP_MAX) {
      newDeviceConfig.highTemp = val;
    } else if (_server->arg("highTemp").length() > 0) {
      valid = false;
      snprintf(errorMsg, sizeof(errorMsg),
               "High temperature must be %.1f..%.1f", TEMP_MIN, TEMP_MAX);
    }
  }

  if (_server->hasArg("lowHum")) {
    float val = _server->arg("lowHum").toFloat();
    if (val >= HUM_MIN && val <= HUM_MAX) {
      newDeviceConfig.lowHum = val;
    } else if (_server->arg("lowHum").length() > 0) {
      valid = false;
      snprintf(errorMsg, sizeof(errorMsg), "Low humidity must be %.1f..%.1f",
               HUM_MIN, HUM_MAX);
    }
  }

  if (_server->hasArg("highHum")) {
    float val = _server->arg("highHum").toFloat();
    if (val >= HUM_MIN && val <= HUM_MAX) {
      newDeviceConfig.highHum = val;
    } else if (_server->arg("highHum").length() > 0) {
      valid = false;
      snprintf(errorMsg, sizeof(errorMsg), "High humidity must be %.1f..%.1f",
               HUM_MIN, HUM_MAX);
    }
  }

  if (_server->hasArg("speedPercent")) {
    int val = _server->arg("speedPercent").toInt();
    if (val >= SPEED_PERCENT_MIN && val <= SPEED_PERCENT_MAX) {
      newDeviceConfig.speedPercent = (uint8_t)val;
    } else if (_server->arg("speedPercent").length() > 0) {
      valid = false;
      snprintf(errorMsg, sizeof(errorMsg), "Speed must be %d..%d",
               SPEED_PERCENT_MIN, SPEED_PERCENT_MAX);
    }
  }

  if (_server->hasArg("sensorMode")) {
    newDeviceConfig.sensorMode = (_server->arg("sensorMode") == "1");
  }
  if (_server->hasArg("adaptiveMode")) {
    newDeviceConfig.adaptiveMode = (_server->arg("adaptiveMode") == "1");
  }
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  if (_server->hasArg("delaySeconds")) {
    int val = _server->arg("delaySeconds").toInt();
    if (val >= DELAY_SECONDS_MIN && val <= DELAY_SECONDS_MAX) {
      newDeviceConfig.delaySeconds = (uint32_t)val;
    } else if (_server->arg("delaySeconds").length() > 0) {
      valid = false;
      snprintf(errorMsg, sizeof(errorMsg), "Delay must be %d..%d",
               DELAY_SECONDS_MIN, DELAY_SECONDS_MAX);
    }
  }

  if (_server->hasArg("maxOnTime")) {
    int val = _server->arg("maxOnTime").toInt();
    if (val >= MAX_ON_TIME_MIN && val <= MAX_ON_TIME_MAX) {
      newDeviceConfig.maxOnTime = (uint32_t)val;
    } else if (_server->arg("maxOnTime").length() > 0) {
      valid = false;
      snprintf(errorMsg, sizeof(errorMsg), "Max On Time must be %d..%d",
               MAX_ON_TIME_MIN, MAX_ON_TIME_MAX);
    }
  }

  if (_server->hasArg("bootState")) {
    newDeviceConfig.bootState = (_server->arg("bootState") == "1") ? 1 : 0;
  }
#endif

  if (!valid) {
    XLOG_WARN(CAT_WEB, "Validation error: %s", errorMsg);
    sendConfigPage(errorMsg);
    return;
  }

  if (_eventCallback) {
    TransportEventData event;
    event.event = DEVICE_CONFIG;
    event.device = &newDeviceConfig;
    event.state = nullptr;
    event.transport = nullptr;
    event.deviceState = nullptr;
    _eventCallback(&event, _eventContext);
    XLOG_DEBUG(CAT_WEB, "DEVICE_CONFIG event sent");
  } else {
    XLOG_WARN(CAT_WEB, "eventCallback is null, using configPending flag");
    configPending = true;
    memcpy(&pendingDeviceConfig, &newDeviceConfig, sizeof(DeviceConfig));
  }

  XLOG_INFO(CAT_WEB, "Configuration saved successfully");
  sendConfigPage(nullptr, "Configuration saved! Device will reboot.");
}

// ============================================================================
// handleSaveWifi() — сохранение настроек транспорта с /savewifi (провизионинг)
// ============================================================================

void WebManager::handleSaveWifi() {
  if (!_server)
    return;

  XLOG_INFO(CAT_WEB, "POST /savewifi (provisioning)");

  String ssid = _server->arg("wifiSsid");
  String password = _server->arg("wifiPassword");

  if (ssid.length() == 0) {
    web_send_result_page(
        [](const char* chunk, void* context) {
          WebServerClass* srv = static_cast<WebServerClass*>(context);
          if (srv && chunk)
            srv->sendContent(chunk);
        },
        _server, "WiFi SSID cannot be empty", false);
    return;
  }

  if (ssid.length() >= 32) {
    web_send_result_page(
        [](const char* chunk, void* context) {
          WebServerClass* srv = static_cast<WebServerClass*>(context);
          if (srv && chunk)
            srv->sendContent(chunk);
        },
        _server, "SSID too long (max 31 chars)", false);
    return;
  }

  if (password.length() >= 64) {
    web_send_result_page(
        [](const char* chunk, void* context) {
          WebServerClass* srv = static_cast<WebServerClass*>(context);
          if (srv && chunk)
            srv->sendContent(chunk);
        },
        _server, "Password too long (max 63 chars)", false);
    return;
  }

  TransportConfig newConfig;
  memcpy(&newConfig, _transportConfig, sizeof(TransportConfig));

  strncpy(newConfig.wifiSsid, ssid.c_str(), sizeof(newConfig.wifiSsid) - 1);
  newConfig.wifiSsid[sizeof(newConfig.wifiSsid) - 1] = '\0';
  strncpy(newConfig.wifiPassword, password.c_str(),
          sizeof(newConfig.wifiPassword) - 1);
  newConfig.wifiPassword[sizeof(newConfig.wifiPassword) - 1] = '\0';

#if FEATURE_MQTT_ENABLED == 1
  if (_server->hasArg("mqttBroker")) {
    String broker = _server->arg("mqttBroker");
    if (broker.length() > 0 && broker.length() < sizeof(newConfig.mqttBroker)) {
      strncpy(newConfig.mqttBroker, broker.c_str(),
              sizeof(newConfig.mqttBroker) - 1);
      newConfig.mqttBroker[sizeof(newConfig.mqttBroker) - 1] = '\0';
    }
  }
  if (_server->hasArg("mqttPort")) {
    int port = _server->arg("mqttPort").toInt();
    if (port >= 1 && port <= 65535) {
      newConfig.mqttPort = port;
    }
  }
  if (_server->hasArg("mqttUser")) {
    String user = _server->arg("mqttUser");
    if (user.length() < sizeof(newConfig.mqttUser)) {
      strncpy(newConfig.mqttUser, user.c_str(), sizeof(newConfig.mqttUser) - 1);
      newConfig.mqttUser[sizeof(newConfig.mqttUser) - 1] = '\0';
    }
  }
  if (_server->hasArg("mqttPassword")) {
    String pass = _server->arg("mqttPassword");
    if (pass.length() < sizeof(newConfig.mqttPassword)) {
      strncpy(newConfig.mqttPassword, pass.c_str(),
              sizeof(newConfig.mqttPassword) - 1);
      newConfig.mqttPassword[sizeof(newConfig.mqttPassword) - 1] = '\0';
    }
  }
#endif

  if (_eventCallback) {
    TransportEventData event;
    event.event = TRANSPORT_CONFIG;
    event.transport = &newConfig;
    event.state = nullptr;
    event.deviceState = nullptr;
    event.device = nullptr;
    _eventCallback(&event, _eventContext);
    XLOG_DEBUG(CAT_WEB, "TRANSPORT_CONFIG event sent");
  } else {
    XLOG_WARN(CAT_WEB, "eventCallback is null, using configPending flag");
    configPending = true;
    memcpy(&pendingConfig, &newConfig, sizeof(TransportConfig));
  }

  web_send_result_page(
      [](const char* chunk, void* context) {
        WebServerClass* srv = static_cast<WebServerClass*>(context);
        if (srv && chunk)
          srv->sendContent(chunk);
      },
      _server, "WiFi credentials saved. Device will try to connect.", true);
}

// ============================================================================
// handleSet()
// ============================================================================

void WebManager::handleSet() {
  if (!_server)
    return;
  if (_provisioningMode) {
    _server->send(403, "text/plain", "Forbidden in provisioning mode");
    return;
  }

  XLOG_DEBUG(CAT_WEB, "GET /set");

  if (!_eventCallback) {
    _server->send(503, "text/plain", "No event callback registered");
    return;
  }

  const DeviceState& state = *_deviceState;
  const DeviceConfig& cfg = *_deviceConfig;

  DeviceState newState = {};
  newState.speed = state.speed;
  newState.isOn = state.isOn;
  newState.sensorMode = cfg.sensorMode;
  newState.adaptiveMode = cfg.adaptiveMode;

  bool hasChanges = false;
  char resultMsg[64] = "Command executed";

  if (_server->hasArg("state")) {
    String val = _server->arg("state");
    bool isOn = (val == "on" || val == "1" || val == "true");
    newState.isOn = isOn;
    hasChanges = true;
    snprintf(resultMsg, sizeof(resultMsg), "State: %s", isOn ? "ON" : "OFF");
    XLOG_DEBUG(CAT_WEB, "  → state: %s", isOn ? "ON" : "OFF");
  }

  if (_server->hasArg("speed")) {
    int val = _server->arg("speed").toInt();
    if (val >= 0 && val <= 100) {
      newState.speed = (uint8_t)val;
      hasChanges = true;
      snprintf(resultMsg, sizeof(resultMsg), "Speed: %d%%", val);
      XLOG_DEBUG(CAT_WEB, "  → speed: %d%%", val);
    } else {
      XLOG_WARN(CAT_WEB, "Invalid speed value: %d (ignored)", val);
    }
  }

  if (_server->hasArg("sensorMode")) {
    String val = _server->arg("sensorMode");
    bool mode = (val == "1" || val == "true" || val == "AUTO");
    newState.sensorMode = mode;
    hasChanges = true;
    snprintf(resultMsg, sizeof(resultMsg), "Mode: %s",
             mode ? "AUTO" : "MANUAL");
    XLOG_DEBUG(CAT_WEB, "  → sensorMode: %s", mode ? "AUTO" : "MANUAL");
  }

  if (_server->hasArg("adaptiveMode")) {
    String val = _server->arg("adaptiveMode");
    bool mode = (val == "1" || val == "true" || val == "ON");
    newState.adaptiveMode = mode;
    hasChanges = true;
    snprintf(resultMsg, sizeof(resultMsg), "Adaptive: %s", mode ? "ON" : "OFF");
    XLOG_DEBUG(CAT_WEB, "  → adaptiveMode: %s", mode ? "ON" : "OFF");
  }

  if (!hasChanges) {
    _server->send(400, "text/plain", "No valid parameters");
    XLOG_WARN(CAT_WEB, "No valid parameters in /set request");
    return;
  }

  TransportEventData event;
  event.event = DEVICE_COMMAND;
  event.deviceState = &newState;
  event.state = nullptr;
  event.transport = nullptr;
  event.device = nullptr;
  _eventCallback(&event, _eventContext);

  XLOG_DEBUG(CAT_WEB, "Command sent to device controller");

  web_send_result_page(
      [](const char* chunk, void* context) {
        WebManager* mgr = static_cast<WebManager*>(context);
        if (mgr && mgr->_server) {
          mgr->_server->sendContent(chunk);
        }
      },
      this, resultMsg, true);
}

void WebManager::handleNotFound() {
  if (!_server)
    return;
  _server->send(404, "text/plain", "Not found");
}

// ============================================================================
// ОТПРАВКА СТРАНИЦ
// ============================================================================

void WebManager::sendApProvisioningPage() {
  if (!_server)
    return;

  char html[2048];
  html[0] = '\0';

  web_sendPageStart(
      [](const char* chunk, void* context) {
        WebManager* mgr = static_cast<WebManager*>(context);
        if (mgr && mgr->_server) {
          mgr->_server->sendContent(chunk);
        }
      },
      this, "Device Setup", PAGE_MODE_AP);

  char temp[1024];
  snprintf(temp, sizeof(temp),
           "<h1>Device Setup — %s</h1>"
           "<form method='POST' action='/savewifi'>",
           getDeviceId());
  _server->sendContent(temp);

  TransportConfig displayConfig;
  memcpy(&displayConfig, _transportConfig, sizeof(TransportConfig));

  strncpy(
      displayConfig.wifiSsid,
      strlen(displayConfig.wifiSsid) == 0 ? WIFI_SSID : displayConfig.wifiSsid,
      sizeof(displayConfig.wifiSsid) - 1);
  displayConfig.wifiSsid[sizeof(displayConfig.wifiSsid) - 1] = '\0';

  strncpy(displayConfig.wifiPassword,
          strlen(displayConfig.wifiPassword) == 0 ? WIFI_PASSWORD
                                                  : displayConfig.wifiPassword,
          sizeof(displayConfig.wifiPassword) - 1);
  displayConfig.wifiPassword[sizeof(displayConfig.wifiPassword) - 1] = '\0';

  strncpy(displayConfig.mqttBroker,
          strlen(displayConfig.mqttBroker) == 0 ? MQTT_BROKER
                                                : displayConfig.mqttBroker,
          sizeof(displayConfig.mqttBroker) - 1);
  displayConfig.mqttBroker[sizeof(displayConfig.mqttBroker) - 1] = '\0';

  displayConfig.mqttPort =
      (displayConfig.mqttPort == 0) ? MQTT_PORT : displayConfig.mqttPort;

  strncpy(
      displayConfig.mqttUser,
      strlen(displayConfig.mqttUser) == 0 ? MQTT_USER : displayConfig.mqttUser,
      sizeof(displayConfig.mqttUser) - 1);
  displayConfig.mqttUser[sizeof(displayConfig.mqttUser) - 1] = '\0';

  strncpy(displayConfig.mqttPassword,
          strlen(displayConfig.mqttPassword) == 0 ? MQTT_PASSWORD
                                                  : displayConfig.mqttPassword,
          sizeof(displayConfig.mqttPassword) - 1);
  displayConfig.mqttPassword[sizeof(displayConfig.mqttPassword) - 1] = '\0';

  renderWifiBlock(html, sizeof(html), &displayConfig, PAGE_MODE_AP);
  _server->sendContent(html);

  renderMQTTBlock(html, sizeof(html), &displayConfig);
  _server->sendContent(html);

  _server->sendContent(
      "<input type='submit' value='Save and Reboot'>"
      "</form>"
      "<div class='note'>Device will reboot and connect to your WiFi "
      "network.</div>");

  web_sendPageEnd(
      [](const char* chunk, void* context) {
        WebManager* mgr = static_cast<WebManager*>(context);
        if (mgr && mgr->_server) {
          mgr->_server->sendContent(chunk);
        }
      },
      this);
}

// ============================================================================
// sendConfigPage()
// ============================================================================

void WebManager::sendConfigPage(const char* errorMsg, const char* successMsg) {
  if (!_server)
    return;

  char html[2048];
  html[0] = '\0';

  web_sendPageStart(
      [](const char* chunk, void* context) {
        WebManager* mgr = static_cast<WebManager*>(context);
        if (mgr && mgr->_server) {
          mgr->_server->sendContent(chunk);
        }
      },
      this, "Configuration", PAGE_MODE_NORMAL);

  char temp[256];
  snprintf(temp, sizeof(temp), "<h1>Configuration — %s</h1>", getDeviceId());
  _server->sendContent(temp);

  if (errorMsg) {
    snprintf(temp, sizeof(temp), "<div class='error'>%s</div>", errorMsg);
    _server->sendContent(temp);
  }

  if (successMsg) {
    snprintf(temp, sizeof(temp),
             "<div class='success'>%s</div>"
             "<meta http-equiv='refresh' content='3;url=/'>",
             successMsg);
    _server->sendContent(temp);
  }

  char block[3072];

  // ===== ФОРМА 1: ТРАНСПОРТ =====
  _server->sendContent("<div class='block'>");
  _server->sendContent("<h2>Transport</h2>");
  _server->sendContent("<form method='POST' action='/savewifi'>");

  renderWifiBlock(block, sizeof(block), _transportConfig, PAGE_MODE_NORMAL);
  _server->sendContent(block);

  renderMQTTBlock(block, sizeof(block), _transportConfig);
  _server->sendContent(block);

  renderConfirmBlock(block, sizeof(block));
  _server->sendContent(block);

  _server->sendContent("<input type='submit' value='Save Transport'>");
  _server->sendContent("</form>");
  _server->sendContent("</div>");

  // ===== ФОРМА 2: УСТРОЙСТВО =====
  
  _server->sendContent("<div class='block'>");
  _server->sendContent("<h2>Device</h2>");
  _server->sendContent("<form method='POST' action='/save'>");

  renderDeviceSettingsBlock(block, sizeof(block));
  _server->sendContent(block);

  renderConfirmBlock(block, sizeof(block));
  _server->sendContent(block);

  _server->sendContent("<input type='submit' value='Save Device'>");
  _server->sendContent("</form>");
  _server->sendContent("</div>");

  // ===== КНОПКА HOME (ВНЕ ФОРМ) =====
  _server->sendContent("<div style='margin-top:20px;'>");
  ButtonParams homeBtn{"Home", "/", "link-btn"};
  render(block, sizeof(block), homeBtn);
  _server->sendContent(block);
  _server->sendContent("</div>");

  if (ota_is_available()) {
    _server->sendContent(
        "<div style='margin-top:20px'><a href='/update' class='link-btn'>"
        "Upgrade firmware (OTA)</a></div>");
  } else {
    _server->sendContent(
        "<div class='warning'>OTA unavailable: insufficient Flash "
        "memory</div>");
  }

  web_sendPageEnd(
      [](const char* chunk, void* context) {
        WebManager* mgr = static_cast<WebManager*>(context);
        if (mgr && mgr->_server) {
          mgr->_server->sendContent(chunk);
        }
      },
      this);
}

// ============================================================================
// РЕНДЕРИНГ СПЕЦИФИЧНЫХ БЛОКОВ
// ============================================================================

void WebManager::renderDeviceSettingsBlock(char* buf, size_t size) {
  if (!buf || size == 0 || !_deviceConfig) {
    if (buf && size > 0)
      buf[0] = '\0';
    return;
  }

  // ===== ССЫЛКА НА КОНФИГУРАЦИЮ =====
  const DeviceConfig& cfg = *_deviceConfig;

  buf[0] = '\0';
  char temp[1024];
  char numStr[32];

  snprintf(temp, sizeof(temp), "<h3>Device Settings</h3>");
  SAFE_STRCAT(buf, temp, size);

#if DEVICE_TYPE == 1
  // Temperature
  NumberField lowTempField;
  lowTempField.label = "Low Temperature (°C)";
  lowTempField.name = "lowTemp";
  snprintf(temp, sizeof(temp), "%.1f..%.1f", TEMP_MIN, TEMP_MAX);
  lowTempField.note = temp;
  snprintf(numStr, sizeof(numStr), "%.1f", cfg.lowTemp);
  lowTempField.value = numStr;
  lowTempField.placeholder = "27.0";
  snprintf(temp, sizeof(temp), "%.1f", TEMP_MIN);
  lowTempField.min = temp;
  snprintf(temp, sizeof(temp), "%.1f", TEMP_MAX);
  lowTempField.max = temp;
  lowTempField.step = "0.1";
  lowTempField.required = true;
  render(temp, sizeof(temp), lowTempField);
  SAFE_STRCAT(buf, temp, size);

  NumberField highTempField;
  highTempField.label = "High Temperature (°C)";
  highTempField.name = "highTemp";
  snprintf(temp, sizeof(temp), "%.1f..%.1f", TEMP_MIN, TEMP_MAX);
  highTempField.note = temp;
  snprintf(numStr, sizeof(numStr), "%.1f", cfg.highTemp);
  highTempField.value = numStr;
  highTempField.placeholder = "29.0";
  snprintf(temp, sizeof(temp), "%.1f", TEMP_MIN);
  highTempField.min = temp;
  snprintf(temp, sizeof(temp), "%.1f", TEMP_MAX);
  highTempField.max = temp;
  highTempField.step = "0.1";
  highTempField.required = true;
  render(temp, sizeof(temp), highTempField);
  SAFE_STRCAT(buf, temp, size);

  // Humidity
  NumberField lowHumField;
  lowHumField.label = "Low Humidity (%)";
  lowHumField.name = "lowHum";
  snprintf(temp, sizeof(temp), "%.1f..%.1f", HUM_MIN, HUM_MAX);
  lowHumField.note = temp;
  snprintf(numStr, sizeof(numStr), "%.1f", cfg.lowHum);
  lowHumField.value = numStr;
  lowHumField.placeholder = "55.0";
  snprintf(temp, sizeof(temp), "%.1f", HUM_MIN);
  lowHumField.min = temp;
  snprintf(temp, sizeof(temp), "%.1f", HUM_MAX);
  lowHumField.max = temp;
  lowHumField.step = "0.1";
  lowHumField.required = true;
  render(temp, sizeof(temp), lowHumField);
  SAFE_STRCAT(buf, temp, size);

  NumberField highHumField;
  highHumField.label = "High Humidity (%)";
  highHumField.name = "highHum";
  snprintf(temp, sizeof(temp), "%.1f..%.1f", HUM_MIN, HUM_MAX);
  highHumField.note = temp;
  snprintf(numStr, sizeof(numStr), "%.1f", cfg.highHum);
  highHumField.value = numStr;
  highHumField.placeholder = "60.0";
  snprintf(temp, sizeof(temp), "%.1f", HUM_MIN);
  highHumField.min = temp;
  snprintf(temp, sizeof(temp), "%.1f", HUM_MAX);
  highHumField.max = temp;
  highHumField.step = "0.1";
  highHumField.required = true;
  render(temp, sizeof(temp), highHumField);
  SAFE_STRCAT(buf, temp, size);

  // Speed
  NumberField speedField;
  speedField.label = "Speed (%)";
  speedField.name = "speedPercent";
  snprintf(temp, sizeof(temp), "%d..%d", SPEED_PERCENT_MIN, SPEED_PERCENT_MAX);
  speedField.note = temp;
  snprintf(numStr, sizeof(numStr), "%d", cfg.speedPercent);
  speedField.value = numStr;
  speedField.placeholder = "50";
  snprintf(temp, sizeof(temp), "%d", SPEED_PERCENT_MIN);
  speedField.min = temp;
  snprintf(temp, sizeof(temp), "%d", SPEED_PERCENT_MAX);
  speedField.max = temp;
  speedField.step = "1";
  speedField.required = true;
  render(temp, sizeof(temp), speedField);
  SAFE_STRCAT(buf, temp, size);

  // Checkboxes
  CheckboxField sensorModeField;
  sensorModeField.label = "Sensor Control Mode (AUTO)";
  sensorModeField.name = "sensorMode";
  sensorModeField.note = "Enable to use sensor for automatic control";
  sensorModeField.checked = cfg.sensorMode;
  sensorModeField.required = false;
  render(temp, sizeof(temp), sensorModeField);
  SAFE_STRCAT(buf, temp, size);

  CheckboxField adaptiveField;
  adaptiveField.label = "Adaptive Mode";
  adaptiveField.name = "adaptiveMode";
  adaptiveField.note = "Automatically adjust speed based on readings";
  adaptiveField.checked = cfg.adaptiveMode;
  adaptiveField.required = false;
  render(temp, sizeof(temp), adaptiveField);
  SAFE_STRCAT(buf, temp, size);
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  // Delay
  NumberField delayField;
  delayField.label = "Delay Seconds";
  delayField.name = "delaySeconds";
  snprintf(temp, sizeof(temp), "%d..%d", DELAY_SECONDS_MIN, DELAY_SECONDS_MAX);
  delayField.note = temp;
  snprintf(numStr, sizeof(numStr), "%d", cfg.delaySeconds);
  delayField.value = numStr;
  delayField.placeholder = "60";
  snprintf(temp, sizeof(temp), "%d", DELAY_SECONDS_MIN);
  delayField.min = temp;
  snprintf(temp, sizeof(temp), "%d", DELAY_SECONDS_MAX);
  delayField.max = temp;
  delayField.step = "1";
  delayField.required = true;
  render(temp, sizeof(temp), delayField);
  SAFE_STRCAT(buf, temp, size);

  // Max On Time
  NumberField maxOnField;
  maxOnField.label = "Max On Time (seconds)";
  maxOnField.name = "maxOnTime";
  snprintf(temp, sizeof(temp), "%d..%d", MAX_ON_TIME_MIN, MAX_ON_TIME_MAX);
  maxOnField.note = temp;
  snprintf(numStr, sizeof(numStr), "%lu", cfg.maxOnTime);
  maxOnField.value = numStr;
  maxOnField.placeholder = "3600";
  snprintf(temp, sizeof(temp), "%d", MAX_ON_TIME_MIN);
  maxOnField.min = temp;
  snprintf(temp, sizeof(temp), "%d", MAX_ON_TIME_MAX);
  maxOnField.max = temp;
  maxOnField.step = "1";
  maxOnField.required = true;
  render(temp, sizeof(temp), maxOnField);
  SAFE_STRCAT(buf, temp, size);

  // Boot State
  CheckboxField bootField;
  bootField.label = "Boot State (ON after reboot)";
  bootField.name = "bootState";
  bootField.note = "If enabled, device starts with ON state";
  bootField.checked = (cfg.bootState == 1);
  bootField.required = false;
  render(temp, sizeof(temp), bootField);
  SAFE_STRCAT(buf, temp, size);
#endif
}

void WebManager::renderConfirmBlock(char* buf, size_t size) {
  if (!buf || size == 0)
    return;
  buf[0] = '\0';

  char temp[256];
  snprintf(temp, sizeof(temp), "<hr>");
  SAFE_STRCAT(buf, temp, size);

  CheckboxField confirmField;
  confirmField.label = "Confirm settings";
  confirmField.name = "confirmSave";
  confirmField.note = "Check this to confirm";
  confirmField.checked = false;
  confirmField.required = true;
  render(temp, sizeof(temp), confirmField);
  SAFE_STRCAT(buf, temp, size);
}

// ============================================================================
// buildStatusPage()
// ============================================================================

void WebManager::buildStatusPage(char* buf, size_t size) {
  if (!buf || size == 0 || !_deviceState || !_deviceConfig) {
    if (buf && size > 0)
      buf[0] = '\0';
    return;
  }

  const DeviceState& state = *_deviceState;
  const DeviceConfig& cfg = *_deviceConfig;
  const TransportState& tState = *_transportState;

  char temp[512];

  snprintf(temp, sizeof(temp), "<h1>%s %s</h1>", getDeviceId(), VERSION);
  SAFE_STRCAT(buf, temp, size);

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  char val[16];
  snprintf(val, sizeof(val), "%.1f°C", state.temperature);
  TextBlockParams t{"Temperature", val, "", "info"};
  render(temp, sizeof(temp), t);
  SAFE_STRCAT(buf, temp, size);

  snprintf(val, sizeof(val), "%.1f%%", state.humidity);
  TextBlockParams h{"Humidity", val, "", "info"};
  render(temp, sizeof(temp), h);
  SAFE_STRCAT(buf, temp, size);
#endif

  StatusBlockParams s{state.isOn, state.isOn ? "ON" : "OFF"};
  render(temp, sizeof(temp), s);
  SAFE_STRCAT(buf, temp, size);

  if (state.speed > 0) {
    char sp[16];
    snprintf(sp, sizeof(sp), "%d%%", state.speed);
    TextBlockParams spd{"Speed", sp, "", "info"};
    render(temp, sizeof(temp), spd);
    SAFE_STRCAT(buf, temp, size);
  }

  TextBlockParams mode{"Mode", cfg.sensorMode ? "AUTO" : "MANUAL", "", "info"};
  render(temp, sizeof(temp), mode);
  SAFE_STRCAT(buf, temp, size);

  if (cfg.adaptiveMode) {
    TextBlockParams ad{"Adaptive", "ON", "", "success"};
    render(temp, sizeof(temp), ad);
    SAFE_STRCAT(buf, temp, size);
  }

  snprintf(temp, sizeof(temp),
           "<div class='info-block'>"
           "<h3>System Info</h3>"
           "<table>"
           "<tr><td>Device ID</td><td>%s</td></tr>"
           "<tr><td>Version</td><td>%s</td></tr>"
           "<tr><td>WiFi</td><td>%s</td></tr>"
           "<tr><td>RSSI</td><td>%d dBm</td></tr>"
           "<tr><td>Reset Reason</td><td>N/A</td></tr>"
           "</table>"
           "</div>",
           getDeviceId(), VERSION,
           tState.link_ok ? "Connected" : "Disconnected", _rssi);
  SAFE_STRCAT(buf, temp, size);

  snprintf(temp, sizeof(temp),
           "<div style='margin-top:20px;'>"
           "<a href='/config' class='link-btn'>Settings</a>");
  SAFE_STRCAT(buf, temp, size);

  ButtonParams toggleBtn{state.isOn ? "Turn OFF" : "Turn ON",
                         state.isOn ? "/set?state=off" : "/set?state=on",
                         "link-btn"};
  render(temp, sizeof(temp), toggleBtn);
  SAFE_STRCAT(buf, temp, size);

  SAFE_STRCAT(buf, "</div>", size);
}

// ============================================================================
// getDeviceId()
// ============================================================================

const char* WebManager::getDeviceId() const {
  if (_transportConfig && strlen(_transportConfig->deviceId) > 0) {
    return _transportConfig->deviceId;
  }
  return "unknown";
}

#endif  // TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI