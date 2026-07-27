/**
 * @file web.cpp
 * @brief Реализация веб-интерфейса
 */

#include "web.h"
#include "config_manager.h"
#include "logger.h"
#include "provisioning.h"
#include "settings.h"
#include "system_state.h"
#include "web_templates.h"
#include "wifi_manager.h"

#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI

// ============================================================================
// ГЛОБАЛЬНЫЕ ФЛАГИ ДЛЯ КОММУНИКАЦИИ С MAIN
// ============================================================================

volatile bool g_webConfigPending = false;
volatile bool g_webRestartPending = false;
volatile bool g_webCommandPending = false;
ConfigData g_webPendingConfig;
WebCommand g_webCommand;

// ============================================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// ============================================================================

WebServerClass server(80);
static bool g_setupMode = false;
static IWebStatusProvider* g_statusProvider = nullptr;

// ============================================================================
// ПУБЛИЧНЫЕ ФУНКЦИИ
// ============================================================================

void web_registerStatusProvider(IWebStatusProvider* provider) {
  g_statusProvider = provider;
}

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================================

static String formatRemainingTime(unsigned long remainingMs) {
  if (remainingMs <= 0)
    return "0 sec";
  unsigned long remainingSec = remainingMs / 1000;
  if (remainingSec < 60)
    return String(remainingSec) + " sec";
  if (remainingSec < 3600) {
    return String(remainingSec / 60) + " min " + String(remainingSec % 60) +
           " sec";
  }
  return String(remainingSec / 3600) + " ч " +
         String((remainingSec % 3600) / 60) + " min";
}

static String getCurrentModeText() {
  if (!g_statusProvider)
    return "N/A";

#if DEVICE_TYPE == 1
  if (g_statusProvider->isSensorControlMode()) {
    return F("<span style='color:#4CAF50;'>SENSOR</span>");
  }
#endif

  if (g_statusProvider->isDelayActive()) {
    unsigned long remaining = g_statusProvider->getDelayTimer() - millis();
    if (remaining > 0) {
      char buf[64];
      snprintf_P(buf, sizeof(buf),
                 PSTR("<span style='color:#FFC107;'>TIMER: %s</span>"),
                 formatRemainingTime(remaining).c_str());
      return String(buf);
    }
    return F("<span style='color:#FFC107;'>TIMER</span>");
  }

  return F("<span style='color:#f44336;'>MANUAL</span>");
}

static String getMaxOnTimeRemaining() {
  uint32_t maxOnTime = g_statusProvider ? g_statusProvider->getMaxOnTime() : 0;
  if (maxOnTime == 0)
    return "disabled";

  if (!g_statusProvider || !g_statusProvider->isDeviceOn()) {
    return String(maxOnTime) + " sec (not active)";
  }

  unsigned long elapsed = millis() - g_statusProvider->getStartTime();
  if (elapsed >= maxOnTime * 1000UL)
    return "0 sec (triggered)";

  unsigned long remaining = (maxOnTime * 1000UL) - elapsed;
  return formatRemainingTime(remaining);
}

static String getDelayTimerRemaining() {
  if (!g_statusProvider)
    return "N/A";

  if (!g_statusProvider->isDelayActive()) {
    int delaySec = g_statusProvider->getDelaySeconds();
    if (delaySec > 0) {
      return String(delaySec) + " sec (inactive)";
    }
    return "disabled (0 sec)";
  }

  unsigned long remaining = g_statusProvider->getDelayTimer() - millis();
  if (remaining <= 0)
    return "0 sec (will turn on)";
  return formatRemainingTime(remaining);
}

// ============================================================================
// ОБРАБОТЧИК /set (оперативные команды)
// ============================================================================

void handleSetCommand() {
  if (!g_statusProvider) {
    server.send(500, "text/plain", "Status provider not registered");
    return;
  }

  String param = server.arg("param");
  String value = server.arg("value");

  XLOG_INFO(CAT_WEB, "SET command: %s = %s", param.c_str(), value.c_str());

  // --- PARAM_STATE ---
  if (param == "state") {
    bool on = (value == "on" || value == "1");
    g_webCommandPending = true;
    g_webCommand.type = CMD_STATE;
    g_webCommand.value.boolVal = on;
    server.send(
        200, "text/html",
        web_buildResultHtml("State set to " + String(on ? "ON" : "OFF"), true));
    return;
  }

  // --- PARAM_SPEED (только TYPE 1) ---
#if DEVICE_TYPE == 1
  if (param == "speed") {
    int speed = value.toInt();
    if (speed < 0 || speed > 100) {
      server.send(400, "text/html",
                  web_buildResultHtml("Speed must be 0-100%", false));
      return;
    }
    g_webCommandPending = true;
    g_webCommand.type = CMD_SPEED;
    g_webCommand.value.intVal = speed;
    server.send(
        200, "text/html",
        web_buildResultHtml("Speed set to " + String(speed) + "%", true));
    return;
  }
#endif

  // --- PARAM_MANUAL_MODE (только TYPE 1) ---
#if DEVICE_TYPE == 1
  if (param == "manualMode") {
    bool enabled = (value == "1" || value == "on");
    g_webCommandPending = true;
    g_webCommand.type = CMD_MANUAL_MODE;
    g_webCommand.value.boolVal = enabled;
    server.send(
        200, "text/html",
        web_buildResultHtml(
            "Manual mode " + String(enabled ? "enabled" : "disabled"), true));
    return;
  }
#endif

  // --- Неизвестная команда ---
  server.send(400, "text/html",
              web_buildResultHtml("Unknown command: " + param, false));
}

// ============================================================================
// ВСПОМОГАТЕЛЬНАЯ ФУНКЦИЯ ОТПРАВКИ HTML
// ============================================================================

static void webSendContent(const String& chunk, void* context) {
  WebServerClass* srv = (WebServerClass*)context;
  srv->sendContent(chunk);
}

// ============================================================================
// ПОСТРОЕНИЕ HTML СТАТУСА
// ============================================================================

String web_buildStatusHtml() {
  if (!g_statusProvider) {
    return F("<div class='error'>Status provider not registered</div>");
  }

  String html;
  bool sensorOk = g_statusProvider->isSensorOk();
  bool state = g_statusProvider->isDeviceOn();

  // ===== ДАТЧИК =====
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  if (sensorOk) {
    float temp = g_statusProvider->getTemperature();
    float hum = g_statusProvider->getHumidity();

    String tempColor = "#2196F3";
    String humColor = "#2196F3";

#if DEVICE_TYPE == 1
    if (temp >= g_statusProvider->getHighTemp())
      tempColor = "#f44336";
    else if (temp <= g_statusProvider->getLowTemp())
      tempColor = "#4CAF50";

    if (hum >= g_statusProvider->getHighHum())
      humColor = "#f44336";
    else if (hum <= g_statusProvider->getLowHum())
      humColor = "#4CAF50";
#endif

    html += F("<div class='flex-container'>");

    // Температура
    html += F("<div class='sensor-card' style='background:");
    html += tempColor;
    html += F("20; border:2px solid ");
    html += tempColor;
    html += F(";'>");
    html += F("<div class='sensor-value' style='color:");
    html += tempColor;
    html += F(";'>");
    html += String(temp, 1);
    html += F(" °C</div>");
    html += F("<div class='sensor-label'>Temperature");
#if DEVICE_TYPE == 1
    html += F(" (off: ");
    html += String(g_statusProvider->getLowTemp(), 1);
    html += F(" on: ");
    html += String(g_statusProvider->getHighTemp(), 1);
    html += F(")");
#endif
    html += F("</div></div>");

    // Влажность
    html += F("<div class='sensor-card' style='background:");
    html += humColor;
    html += F("20; border:2px solid ");
    html += humColor;
    html += F(";'>");
    html += F("<div class='sensor-value' style='color:");
    html += humColor;
    html += F(";'>");
    html += String(hum, 1);
    html += F(" %</div>");
    html += F("<div class='sensor-label'>Humidity");
#if DEVICE_TYPE == 1
    html += F(" (off: ");
    html += String(g_statusProvider->getLowHum(), 1);
    html += F(" on: ");
    html += String(g_statusProvider->getHighHum(), 1);
    html += F(")");
#endif
    html += F("</div></div>");

    html += F("</div>");
  } else {
    html += F("<div class='sensor-error'><strong>Sensor error</strong><br>");
    html += g_statusProvider->getSensorError();
    html += F("</div>");
  }
#endif

  // ===== EMERGENCY =====
  if (g_statusProvider->isEmergencyStop()) {
    html += F("<div class='status-card error'>");
    html += F("<div style='font-size:1.2em;'>EMERGENCY STOPPED</div>");
    html +=
        F("<div class='note'>Device was automatically turned off after "
          "exceeding the maximum allowed runtime.</div>");
    html += F("</div>");
  }

  // ===== РЕЖИМ =====
  html +=
      F("<div class='status-card' style='background:#f5f5f5; border:2px solid "
        "#ddd;'>");
  html += F("<div style='font-size:1.5em;font-weight:bold;'>Mode: ");
  html += getCurrentModeText();
  html += F("</div></div>");

  // ===== СОСТОЯНИЕ =====
  String stateColor = state ? "#f44336" : "#2196F3";
  String stateText = state ? "ON" : "OFF";

  html += F("<div class='status-card' style='background:");
  html += stateColor;
  html += F("20; border:2px solid ");
  html += stateColor;
  html += F(";'>");
  html += F("<div style='font-size:2em;font-weight:bold;color:");
  html += stateColor;
  html += F(";'>Device: ");
  html += stateText;
  html += F("</div></div>");

  // ===== СКОРОСТЬ (TYPE 1) =====
#if DEVICE_TYPE == 1
  if (state) {
    int speed = g_statusProvider->getSpeedPercent();
    html +=
        F("<div class='status-card' style='background:#2196F320; border:2px "
          "solid #2196F3;'>");
    html += F("<div style='font-size:1.2em;font-weight:bold;'>Speed: ");
    html += String(speed);
    html += F("%</div>");
    html += F("<div class='duty-bar'><div class='duty-fill' style='width:");
    html += String(speed);
    html += F("%;'></div></div>");
    if (speed < 100) {
      html += F("<div style='font-size:0.9em;color:#555;'>Quiet mode active");
      if (g_statusProvider->isAdaptiveModeActive())
        html += F(" + adaptive");
      html += F("</div>");
    }
    html += F("</div>");
  }
#endif

  // ===== ИНФОРМАЦИОННАЯ ПАНЕЛЬ =====
  html += F("<hr><div class='info'>");

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  html += F("Sensor polling ");
  html += String(g_statusProvider->getSensorInterval());
  html += F(" sec<br>");
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  html += F("Emergency timeout: ");
  html += getMaxOnTimeRemaining();
  html += F("<br>");
  html += F("Delay timer: ");
  html += getDelayTimerRemaining();
  html += F("<br>");
#endif

  html += F("MQTT: ");
  html +=
      g_statusProvider->isMqttConnected() ? F("connected") : F("disconnected");
  html += F("<br>");

#if WEB_SHOW_RSSI == 1
  html += F("RSSI: ");
  html += String(g_statusProvider->getWifiRssi());
  html += F(" dBm<br>");
#endif

  html += F("</div>");

  // ===== КНОПКИ УПРАВЛЕНИЯ =====
  html += F("<div class='button-group'>");

  // ON / OFF
  String onOffLabel = state ? "Turn OFF" : "Turn ON";
  html += F("<a href='/set?param=state&value=");
  html += state ? "off" : "on";
  html += F("'><button class='link-btn'>");
  html += onOffLabel;
  html += F("</button></a>");

  // Скорость (TYPE 1, если включён)
#if DEVICE_TYPE == 1
  if (state) {
    html += F("<a href='/set?param=speed&value=25'><button>25%</button></a>");
    html += F("<a href='/set?param=speed&value=50'><button>50%</button></a>");
    html += F("<a href='/set?param=speed&value=75'><button>75%</button></a>");
    html += F("<a href='/set?param=speed&value=100'><button>100%</button></a>");
  }
#endif

  // Manual / Auto (TYPE 1)
#if DEVICE_TYPE == 1
  if (g_statusProvider->isSensorControlMode()) {
    html +=
        F("<a href='/set?param=manualMode&value=1'><button>Manual "
          "mode</button></a>");
  } else {
    html +=
        F("<a href='/set?param=manualMode&value=0'><button>Auto "
          "mode</button></a>");
  }
#endif

  html += F("</div>");

  // ===== КНОПКА ВКЛЮЧЕНИЯ СЕНСОРНОГО РЕЖИМА (устаревшая, но пока оставлена)
  // =====
#if DEVICE_TYPE == 1
  if (!g_statusProvider->isSensorControlMode() && sensorOk) {
    html += F("<div class='button-group' style='margin-top:10px;'>");
    html +=
        F("<a href='/set?param=manualMode&value=0'><button>Enable Sensor "
          "Control</button></a>");
    html += F("</div>");
  }
#endif

  return html;
}

// ============================================================================
// ОБРАБОТЧИК AP-ПРОВИЗИОНИНГА (временный, будет перенесён в provisioning)
// ============================================================================

void web_handleApProvisioning() {
  String ssid = server.arg("wifiSsid");
  String password = server.arg("wifiPassword");

  if (ssid.length() > 0) {
    XLOG_INFO(CAT_WEB, "WiFi credentials received via AP: %s", ssid.c_str());

    ProvisioningData data;
    memset(&data, 0, sizeof(data));
    data.type = 0;

#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI
    strncpy(data.wifiSsid, ssid.c_str(), sizeof(data.wifiSsid) - 1);
    strncpy(data.wifiPassword, password.c_str(), sizeof(data.wifiPassword) - 1);
#endif

    ProvisioningManager::getInstance().onDataReceived(data);
    server.send(200, "text/html",
                web_buildResultHtml("WiFi credentials saved", true));
  } else {
    server.send(400, "text/html",
                web_buildResultHtml("WiFi credentials not saved", false));
  }
}

// ============================================================================
// ОТПРАВКА СТРАНИЦЫ СТАТУСА
// ============================================================================

void web_sendStatusPage(int refreshInterval) {
  if (!g_statusProvider) {
    server.send(500, "text/html", "Status provider not registered");
    return;
  }

  const char* deviceId = g_configManager.getDeviceId();
  String statusHtml = web_buildStatusHtml();

  if (statusHtml.length() == 0) {
    statusHtml = F("<div class='warning'>Device status is loading...</div>");
    XLOG_WARN(CAT_WEB, "web_buildStatusHtml() returned empty, using fallback");
  }

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

#if defined(ESP32)
  String buffer;
  buffer.reserve(1024);

  auto flush = [&]() {
    if (buffer.length() > 0) {
      server.sendContent(buffer);
      buffer = "";
    }
  };

  auto send = [&](const String& chunk) {
    if (buffer.length() + chunk.length() > 1024)
      flush();
    buffer += chunk;
  };

  send(FPSTR(HTML_PAGE_START));

  if (refreshInterval > 0) {
    char refresh[64];
    snprintf_P(refresh, sizeof(refresh),
               PSTR("<meta http-equiv='refresh' content='%d'>"),
               refreshInterval);
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
  send(
      F("<div class='button-group'><a "
        "href='/config'><button>Settings</button></a></div>"));
  send(FPSTR(HTML_PAGE_END));

  flush();

#elif defined(ESP8266)
  webSendContent(FPSTR(HTML_PAGE_START), &server);

  if (refreshInterval > 0) {
    char refresh[64];
    snprintf_P(refresh, sizeof(refresh),
               PSTR("<meta http-equiv='refresh' content='%d'>"),
               refreshInterval);
    webSendContent(refresh, &server);
  } else {
    webSendContent(F("{META_REFRESH}"), &server);
  }

  webSendContent(F("<title>"), &server);
  webSendContent(deviceId, &server);
  webSendContent(F("</title>"), &server);
  webSendContent(FPSTR(HTML_STYLE), &server);
  webSendContent(F("</head><body><div class='container'>"), &server);
  webSendContent(F("<h1>"), &server);
  webSendContent(deviceId, &server);
  webSendContent(F(" VERSION "), &server);
  webSendContent(VERSION, &server);
  webSendContent(F("</h1>"), &server);
  webSendContent(statusHtml, &server);
  webSendContent(F("<div class='button-group'><a "
                   "href='/config'><button>Settings</button></a></div>"),
                 &server);
  webSendContent(FPSTR(HTML_PAGE_END), &server);

#else
  String fullHtml = FPSTR(HTML_PAGE_START);
  if (refreshInterval > 0) {
    char refresh[64];
    snprintf_P(refresh, sizeof(refresh),
               PSTR("<meta http-equiv='refresh' content='%d'>"),
               refreshInterval);
    fullHtml += refresh;
  }
  fullHtml += F("<title>");
  fullHtml += deviceId;
  fullHtml += F("</title>");
  fullHtml += FPSTR(HTML_STYLE);
  fullHtml += F("</head><body><div class='container'>");
  fullHtml += F("<h1>");
  fullHtml += deviceId;
  fullHtml += F(" VERSION ");
  fullHtml += VERSION;
  fullHtml += F("</h1>");
  fullHtml += statusHtml;
  fullHtml +=
      F("<div class='button-group'><a "
        "href='/config'><button>Settings</button></a></div>");
  fullHtml += FPSTR(HTML_PAGE_END);
  server.sendContent(fullHtml);
#endif
}

// ============================================================================
// СТРАНИЦА AP-ПРОВИЗИОНИНГА
// ============================================================================

const char HTML_AP_CONTENT[] PROGMEM = R"rawliteral(
<div class='container'>
    <h1>WiFi Setup</h1>
    <form method='POST' action='/savewifi'>
        <label>WiFi SSID</label>
        <input type='text' name='wifiSsid' required placeholder='Enter WiFi name'>
        <label>WiFi Password</label>
        <input type='password' name='wifiPassword' placeholder='Leave empty for open network'>
        <input type='submit' value='Save and Reboot'>
    </form>
    <div class='note'>Device will reboot and connect to your WiFi network.</div>
</div>
)rawliteral";

// ============================================================================
// ОТПРАВКА СТРАНИЦЫ КОНФИГУРАЦИИ
// ============================================================================

#if defined(ESP32)
static String g_configBuffer;
static bool g_configFlushNeeded = false;

static void configFlush() {
  if (g_configBuffer.length() > 0) {
    server.sendContent(g_configBuffer);
    g_configBuffer = "";
    g_configFlushNeeded = false;
  }
}

static void configSend(const String& chunk) {
  if (g_configBuffer.length() + chunk.length() > 1024)
    configFlush();
  g_configBuffer += chunk;
  g_configFlushNeeded = true;
}

static void configSendWrapper(const String& chunk, void* context) {
  (void)context;
  configSend(chunk);
}
#endif

void web_sendConfigPage(const String& errorMsg, const String& successMsg) {
  const ConfigData* cfg = g_configManager.get();
  bool isApMode = system_state_has_bit(STATE_PROVISIONING);
  const char* deviceId = g_configManager.getDeviceId();

  String currentMode = isApMode ? F("Access Point") : F("Client WiFi");
  String currentSsid = isApMode ? String(deviceId) : String(cfg->wifiSsid);
  String currentIp = wifi_get_local_ip();
  int refreshSeconds = (successMsg.length() > 0) ? DEFAULT_WEB_REFRESH : 0;

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

#if defined(ESP32)
  g_configBuffer = "";
  g_configBuffer.reserve(1024);
  g_configFlushNeeded = false;

  sendConfigPage(configSendWrapper, nullptr, errorMsg, successMsg, *cfg,
                 currentMode, currentSsid, currentIp, refreshSeconds,
                 g_setupMode);

  if (g_configFlushNeeded)
    configFlush();

#elif defined(ESP8266)
  sendConfigPage(webSendContent, &server, errorMsg, successMsg, *cfg,
                 currentMode, currentSsid, currentIp, refreshSeconds,
                 g_setupMode);

#else
  String fullHtml;
  auto send = [&](const String& chunk, void* context) {
    (void)context;
    fullHtml += chunk;
  };
  sendConfigPage(send, nullptr, errorMsg, successMsg, *cfg, currentMode,
                 currentSsid, currentIp, refreshSeconds, g_setupMode);
  server.sendContent(fullHtml);
#endif
}

// ============================================================================
// СОХРАНЕНИЕ КОНФИГУРАЦИИ (ПОСТ-ФОРМА)
// ============================================================================

void web_saveConfig() {
  XLOG_INFO(CAT_WEB, "Processing config form...");

  // Копируем текущую конфигурацию как базу
  memcpy(&g_webPendingConfig, g_configManager.get(), sizeof(ConfigData));

  // --- WiFi ---
  if (server.hasArg("wifiSsid")) {
    String ssid = server.arg("wifiSsid");
    if (ssid.length() > 0 &&
        ssid.length() < sizeof(g_webPendingConfig.wifiSsid)) {
      strncpy(g_webPendingConfig.wifiSsid, ssid.c_str(),
              sizeof(g_webPendingConfig.wifiSsid) - 1);
      g_webPendingConfig.wifiSsid[sizeof(g_webPendingConfig.wifiSsid) - 1] =
          '\0';
    } else {
      web_sendConfigPage("WiFi SSID is empty or too long", "");
      return;
    }
  }

  if (server.hasArg("wifiPassword")) {
    String pwd = server.arg("wifiPassword");
    if (pwd.length() > 0) {
      if (pwd.length() < sizeof(g_webPendingConfig.wifiPassword)) {
        strncpy(g_webPendingConfig.wifiPassword, pwd.c_str(),
                sizeof(g_webPendingConfig.wifiPassword) - 1);
        g_webPendingConfig
            .wifiPassword[sizeof(g_webPendingConfig.wifiPassword) - 1] = '\0';
      } else {
        web_sendConfigPage("WiFi password too long", "");
        return;
      }
    }
  }

  // --- MQTT ---
#if FEATURE_MQTT_ENABLED == 1
  if (server.hasArg("mqttBroker")) {
    String broker = server.arg("mqttBroker");
    if (broker.length() > 0 &&
        broker.length() < sizeof(g_webPendingConfig.mqttBroker)) {
      strncpy(g_webPendingConfig.mqttBroker, broker.c_str(),
              sizeof(g_webPendingConfig.mqttBroker) - 1);
      g_webPendingConfig.mqttBroker[sizeof(g_webPendingConfig.mqttBroker) - 1] =
          '\0';
    } else {
      web_sendConfigPage("MQTT Broker is empty or too long", "");
      return;
    }
  }

  if (server.hasArg("mqttPort")) {
    int port = server.arg("mqttPort").toInt();
    if (port >= 1 && port <= 65535) {
      g_webPendingConfig.mqttPort = (uint16_t)port;
    } else {
      web_sendConfigPage("MQTT Port must be 1-65535", "");
      return;
    }
  }

  if (server.hasArg("mqttUser")) {
    String user = server.arg("mqttUser");
    if (user.length() < sizeof(g_webPendingConfig.mqttUser)) {
      strncpy(g_webPendingConfig.mqttUser, user.c_str(),
              sizeof(g_webPendingConfig.mqttUser) - 1);
      g_webPendingConfig.mqttUser[sizeof(g_webPendingConfig.mqttUser) - 1] =
          '\0';
    }
  }

  if (server.hasArg("mqttPassword")) {
    String pwd = server.arg("mqttPassword");
    if (pwd.length() > 0 &&
        pwd.length() < sizeof(g_webPendingConfig.mqttPassword)) {
      strncpy(g_webPendingConfig.mqttPassword, pwd.c_str(),
              sizeof(g_webPendingConfig.mqttPassword) - 1);
      g_webPendingConfig
          .mqttPassword[sizeof(g_webPendingConfig.mqttPassword) - 1] = '\0';
    }
  }

  if (server.hasArg("mqttClientId")) {
    String cid = server.arg("mqttClientId");
    if (cid.length() > 0 &&
        cid.length() < sizeof(g_webPendingConfig.mqttClientId)) {
      strncpy(g_webPendingConfig.mqttClientId, cid.c_str(),
              sizeof(g_webPendingConfig.mqttClientId) - 1);
      g_webPendingConfig
          .mqttClientId[sizeof(g_webPendingConfig.mqttClientId) - 1] = '\0';
    } else {
      web_sendConfigPage("MQTT Client ID is empty or too long", "");
      return;
    }
  }
#endif

  // --- Sensor Interval ---
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  if (server.hasArg("sensorInterval")) {
    int interval = server.arg("sensorInterval").toInt();
    if (interval >= SENSOR_INTERVAL_MIN && interval <= SENSOR_INTERVAL_MAX) {
      g_webPendingConfig.sensorInterval = (uint16_t)interval;
    } else {
      web_sendConfigPage("Sensor interval out of range", "");
      return;
    }
  }
#endif

  // --- TYPE 1 ---
#if DEVICE_TYPE == 1
  if (server.hasArg("lowTemp")) {
    float val = server.arg("lowTemp").toFloat();
    if (val >= TEMP_MIN && val <= TEMP_MAX)
      g_webPendingConfig.lowTemp = val;
    else {
      web_sendConfigPage("Low Temp out of range", "");
      return;
    }
  }

  if (server.hasArg("highTemp")) {
    float val = server.arg("highTemp").toFloat();
    if (val >= TEMP_MIN && val <= TEMP_MAX)
      g_webPendingConfig.highTemp = val;
    else {
      web_sendConfigPage("High Temp out of range", "");
      return;
    }
  }

  if (server.hasArg("lowHum")) {
    float val = server.arg("lowHum").toFloat();
    if (val >= HUM_MIN && val <= HUM_MAX)
      g_webPendingConfig.lowHum = val;
    else {
      web_sendConfigPage("Low Hum out of range", "");
      return;
    }
  }

  if (server.hasArg("highHum")) {
    float val = server.arg("highHum").toFloat();
    if (val >= HUM_MIN && val <= HUM_MAX)
      g_webPendingConfig.highHum = val;
    else {
      web_sendConfigPage("High Hum out of range", "");
      return;
    }
  }

  if (server.hasArg("maxOnTime")) {
    uint32_t val = server.arg("maxOnTime").toInt();
    if (val <= MAX_ON_TIME_MAX)
      g_webPendingConfig.maxOnTime = val;
    else {
      web_sendConfigPage("MaxOnTime out of range", "");
      return;
    }
  }

  if (server.hasArg("delaySeconds")) {
    int val = server.arg("delaySeconds").toInt();
    if (val >= DELAY_SECONDS_MIN && val <= DELAY_SECONDS_MAX)
      g_webPendingConfig.delaySeconds = val;
    else {
      web_sendConfigPage("Delay seconds out of range", "");
      return;
    }
  }

  if (server.hasArg("speedPercent")) {
    int val = server.arg("speedPercent").toInt();
    if (val >= 0 && val <= 100)
      g_webPendingConfig.speedPercent = (uint16_t)val;
    else {
      web_sendConfigPage("Speed must be 0-100%", "");
      return;
    }
  }

  g_webPendingConfig.adaptiveMode = server.hasArg("adaptiveMode");
  g_webPendingConfig.bootState = server.hasArg("bootState");
  g_webPendingConfig.sensorControlMode = server.hasArg("sensorControlMode");
#endif

  // --- TYPE 3 ---
#if DEVICE_TYPE == 3
  if (server.hasArg("maxOnTime")) {
    uint32_t val = server.arg("maxOnTime").toInt();
    if (val <= MAX_ON_TIME_MAX)
      g_webPendingConfig.maxOnTime = val;
    else {
      web_sendConfigPage("MaxOnTime out of range", "");
      return;
    }
  }

  if (server.hasArg("delaySeconds")) {
    int val = server.arg("delaySeconds").toInt();
    if (val >= DELAY_SECONDS_MIN && val <= DELAY_SECONDS_MAX)
      g_webPendingConfig.delaySeconds = val;
    else {
      web_sendConfigPage("Delay seconds out of range", "");
      return;
    }
  }

  g_webPendingConfig.bootState = server.hasArg("bootState");
#endif

  // Устанавливаем флаг для main
  g_webConfigPending = true;
  XLOG_INFO(CAT_WEB, "Config parsed, pending for main to apply");

  server.send(200, "text/html",
              web_buildResultHtml("Configuration saved", true));
}

// ============================================================================
// СТРАНИЦА РЕЗУЛЬТАТА
// ============================================================================

String web_buildResultHtml(const String& action, bool success) {
  String html = R"rawliteral(<!DOCTYPE html>
<html>
<head>
    <meta charset='UTF-8'>
    <meta http-equiv='refresh' content='2;url=/'>
    <title>)rawliteral";
  html += success ? "Success" : "Error";
  html += R"rawliteral(</title>
    <style>
        body{font-family:Arial;text-align:center;margin-top:50px;background:#f0f0f0;}
        .result{)rawliteral";
  html += success ? "color:#2e7d32;background:#e8f5e9;"
                  : "color:#c62828;background:#ffebee;";
  html += R"rawliteral(padding:20px;border-radius:10px;display:inline-block;}
    </style>
</head>
<body>
    <div class='result'>
        <h2>)rawliteral";
  html += action;
  html += success ? F(" successful</h2><p>Redirecting...</p>")
                  : F(" failed</h2><p>Please try again.</p>");
  html += R"rawliteral(</div>
</body>
</html>)rawliteral";
  return html;
}

// ============================================================================
// ИНИЦИАЛИЗАЦИЯ
// ============================================================================

void web_init(bool setupMode) {
  g_setupMode = setupMode;

  if (setupMode) {
    // AP-режим: только страница ввода WiFi
    server.on("/", []() {
      String html = FPSTR(HTML_PAGE_START);
      html += F("<title>WiFi Setup</title>");
      html += FPSTR(HTML_STYLE);
      html += F("</head><body>");
      html += FPSTR(HTML_AP_CONTENT);
      html += FPSTR(HTML_PAGE_END);
      server.send(200, "text/html", html);
    });

    server.on("/savewifi", web_handleApProvisioning);
    server.on("/favicon.ico", []() { server.send(404); });

  } else {
    // Нормальный режим
    int refreshInterval = DEFAULT_WEB_REFRESH;
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    refreshInterval = g_configManager.getSensorInterval();
#endif

    // Главная страница (статус)
#if FEATURE_WEB_STATUS_ENABLED == 1
    server.on("/", [refreshInterval]() {
      XLOG_DEBUG(CAT_WEB, "GET / - serving status page");
#ifdef ESP32
      server.client().setNoDelay(true);
#endif
      web_sendStatusPage(refreshInterval);
    });
#else
    server.on("/", []() {
      XLOG_DEBUG(CAT_WEB, "GET / - redirect to config");
      server.sendHeader("Location", "/config", true);
      server.send(302, "text/plain", "");
    });
#endif

    // Конфигурация
    server.on("/config", []() {
      XLOG_DEBUG(CAT_WEB, "GET /config - serving config page");
      web_sendConfigPage("", "");
    });

    server.on("/save", web_saveConfig);
    server.on("/favicon.ico", []() { server.send(404); });

    // Команды /set (оперативное управление)
    server.on("/set", handleSetCommand);

    // Сброс настроек (через флаг для оркестратора)
#if WEB_RESET_ENABLED == 1
    server.on("/resetall", []() {
      g_webRestartPending = true;
      server.send(
          200, "text/html",
          web_buildResultHtml("Configuration was reset, rebooting...", true));
    });
#endif
  }

  server.begin();
  XLOG_DEBUG(CAT_WEB, "Web server started. (Mode: %s)",
             setupMode ? "SETUP" : "NORMAL");
}

void web_loop() {
  server.handleClient();
}

#endif  // TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI