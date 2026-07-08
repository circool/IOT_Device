/**
 * @file web.cpp
 * @brief Реализация веб-интерфейса
 */

#include "web.h"
#include "config_manager.h"
#include "logger.h"
#include "settings.h"
#include "web_templates.h"
#include "wifi_manager.h"

#if WEB_ENABLED == 1

WebServerClass server(80);

static bool g_setupMode = false;

// ========== ЕДИНСТВЕННАЯ ЗАВИСИМОСТЬ ==========
static IWebStatusProvider* g_statusProvider = nullptr;

void web_registerStatusProvider(IWebStatusProvider* provider) {
  g_statusProvider = provider;
}

// ========== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ==========

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

  if (g_configManager.getSensorControlMode()) {
    return F("<span style='color:#4CAF50;'>SENSOR</span>");
  }

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
  uint32_t maxOnTime = g_configManager.getMaxOnTime();
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
    uint16_t delaySec = g_configManager.getDelaySeconds();
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

// ========== ПОСТРОЕНИЕ HTML СТАТУСА ==========

String web_buildStatusHtml() {
  if (!g_statusProvider) {
    return F("<div class='error'>Status provider not registered</div>");
  }

  String html;
  bool sensorOk = g_statusProvider->isSensorOk();

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  if (sensorOk) {
    float temp = g_statusProvider->getTemperature();
    float hum = g_statusProvider->getHumidity();

    String tempColor = "#2196F3";
    String humColor = "#2196F3";

#if DEVICE_TYPE == 1
    if (temp >= g_configManager.getHighTemp())
      tempColor = "#f44336";
    else if (temp <= g_configManager.getLowTemp())
      tempColor = "#4CAF50";

    if (hum >= g_configManager.getHighHum())
      humColor = "#f44336";
    else if (hum <= g_configManager.getLowHum())
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
    html += String(g_configManager.getLowTemp(), 1);
    html += F(" on: ");
    html += String(g_configManager.getHighTemp(), 1);
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
    html += String(g_configManager.getLowHum(), 1);
    html += F(" on: ");
    html += String(g_configManager.getHighHum(), 1);
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

  // ========== EMERGENCY STOP ==========
  if (g_statusProvider->isEmergencyStop()) {
    html += F("<div class='status-card error'>");
    html += F("<div style='font-size:1.2em;'>EMERGENCY STOPPED</div>");
    html += F("<div class='note'>");
    html +=
        F("Device was automatically turned off after exceeding the maximum "
          "allowed runtime.");
    html += F("</div></div>");
  }

  // ========== РЕЖИМ ==========
  html +=
      F("<div class='status-card' style='background:#f5f5f5; border:2px solid "
        "#ddd;'>");
  html += F("<div style='font-size:1.5em;font-weight:bold;'>Mode: ");
  html += getCurrentModeText();
  html += F("</div></div>");

  // ========== СОСТОЯНИЕ УСТРОЙСТВА ==========
  bool state = g_statusProvider->isDeviceOn();
  String stateColor = state ? "#f44336" : "#2196F3";
  String stateText = state ? "ON" : "OFF";

  html += F("<div class='status-card' style='background:");
  html += stateColor;
  html += F("20; border:2px solid ");
  html += stateColor;
  html += F(";'>");
  html += F("<div style='font-size:2em;font-weight:bold;color:");
  html += stateColor;
  html += F(";'>");
  html += F("Device: ");
  html += stateText;
  html += F("</div></div>");

  // ========== СКОРОСТЬ (только TYPE 1) ==========
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

  // ========== ИНФОРМАЦИЯ ==========
  html += F("<hr><div class='info'>");

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  html += F("Sensor polling ");
  html += String(g_configManager.getSensorInterval());
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

  // ========== КНОПКА ВКЛЮЧЕНИЯ SENSOR MODE ==========
#if DEVICE_TYPE == 1
  if (!g_configManager.getSensorControlMode() && sensorOk) {
    html += F("<div class='button-group' style='margin-top:10px;'>");
    html += F("<a href='/fan/auto'><button>Enable Sensor Control</button></a>");
    html += F("</div>");
  }
#endif

  return html;
}

// ========== ОБРАБОТЧИКИ ==========

#if DEVICE_TYPE == 1
void handleToggle() {
  LOG_INFO(CAT_WEB, "Toggle button pressed");
  // TODO: Реализовать через IWebControlProvider
}

void handleSensorControlMode() {
  g_configManager.setSensorControlMode(true);
  LOG_INFO(CAT_WEB, "Sensor control mode enabled");
  // TODO: Обновить состояние актуатора через интерфейс
}
#endif

#if DEVICE_TYPE == 3
void handleToggle() {
  LOG_INFO(CAT_WEB, "Toggle button pressed");
  // TODO: Реализовать через IWebControlProvider
}
#endif

// ========== ФУНКЦИЯ ОТПРАВКИ HTML ДЛЯ WEB_TEMPLATES ==========

/**
 * @brief Отправка HTML-контента через WebServer
 * @param chunk Строка для отправки
 * @param context Указатель на WebServerClass
 */
static void webSendContent(const String& chunk, void* context) {
  WebServerClass* srv = (WebServerClass*)context;
  srv->sendContent(chunk);
}

// ========== ОТПРАВКА СТРАНИЦЫ СТАТУСА ==========

void web_sendStatusPage(int refreshInterval) {
  if (!g_statusProvider) {
    server.send(500, "text/html", "Status provider not registered");
    return;
  }

  const char* deviceId = g_configManager.getDeviceId();
  String statusHtml = web_buildStatusHtml();

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

#if defined(ESP32)
  // ========== БУФЕРИЗИРОВАННАЯ ВЕРСИЯ ДЛЯ ESP32 ==========
  String buffer;
  buffer.reserve(1024);

  auto flush = [&]() {
    if (buffer.length() > 0) {
      server.sendContent(buffer);
      buffer = "";
    }
  };

  auto send = [&](const String& chunk) {
    if (buffer.length() + chunk.length() > 1024) {
      flush();
    }
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
  // ========== ВЕРСИЯ БЕЗ БУФЕРИЗАЦИИ ДЛЯ ESP8266 ==========
  // Используем webSendContent напрямую
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
  // ========== FALLBACK ==========
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

// ========== ОТПРАВКА СТРАНИЦЫ КОНФИГУРАЦИИ ==========

// Глобальные переменные для буферизации при отправке страницы конфигурации
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
  if (g_configBuffer.length() + chunk.length() > 1024) {
    configFlush();
  }
  g_configBuffer += chunk;
  g_configFlushNeeded = true;
}

// Функция-обёртка для WebSendCallback (без захвата!)
static void configSendWrapper(const String& chunk, void* context) {
  (void)context;
  configSend(chunk);
}
#endif

void web_sendConfigPage(const String& errorMsg, const String& successMsg) {
  const ConfigData* cfg = g_configManager.get();
  bool isApMode = wifi_is_ap_mode();
  const char* deviceId = g_configManager.getDeviceId();

  String currentMode = isApMode ? F("Access Point") : F("Client WiFi");
  String currentSsid = isApMode ? String(deviceId) : String(cfg->wifiSsid);
  String currentIp = isApMode ? String(AP_IP_ADDRESS) : wifi_get_local_ip();
  int refreshSeconds = (successMsg.length() > 0) ? DEFAULT_WEB_REFRESH : 0;

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

#if defined(ESP32)
  // ========== БУФЕРИЗИРОВАННАЯ ВЕРСИЯ ДЛЯ ESP32 ==========
  g_configBuffer = "";
  g_configBuffer.reserve(1024);
  g_configFlushNeeded = false;

  sendConfigPage(configSendWrapper, nullptr, errorMsg, successMsg, *cfg,
                 currentMode, currentSsid, currentIp, refreshSeconds,
                 g_setupMode);

  if (g_configFlushNeeded) {
    configFlush();
  }

#elif defined(ESP8266)
  // ========== ВЕРСИЯ БЕЗ БУФЕРИЗАЦИИ ДЛЯ ESP8266 ==========
  // Используем webSendContent как WebSendCallback
  sendConfigPage(webSendContent, &server, errorMsg, successMsg, *cfg,
                 currentMode, currentSsid, currentIp, refreshSeconds,
                 g_setupMode);

#else
  // ========== FALLBACK ==========
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

// ========== СОХРАНЕНИЕ КОНФИГУРАЦИИ ==========

void web_saveConfig() {
  if (server.hasArg("wifiSsid")) {
    if (!g_configManager.setWifiSsid(server.arg("wifiSsid").c_str())) {
      web_sendConfigPage(g_configManager.getLastError(), "");
      return;
    }
  }

  if (server.hasArg("wifiPassword")) {
    String pwd = server.arg("wifiPassword");
    if (pwd.length() > 0) {
      if (!g_configManager.setWifiPassword(pwd.c_str())) {
        web_sendConfigPage(g_configManager.getLastError(), "");
        return;
      }
    }
  }

#if MQTT_ENABLED == 1
  if (server.hasArg("mqttBroker")) {
    if (!g_configManager.setMqttBroker(server.arg("mqttBroker").c_str())) {
      web_sendConfigPage(g_configManager.getLastError(), "");
      return;
    }
  }
  if (server.hasArg("mqttPort")) {
    if (!g_configManager.setMqttPort(server.arg("mqttPort").toInt())) {
      web_sendConfigPage(g_configManager.getLastError(), "");
      return;
    }
  }
  if (server.hasArg("mqttUser")) {
    if (!g_configManager.setMqttUser(server.arg("mqttUser").c_str())) {
      web_sendConfigPage(g_configManager.getLastError(), "");
      return;
    }
  }
  if (server.hasArg("mqttPassword")) {
    String pwd = server.arg("mqttPassword");
    if (pwd.length() > 0) {
      if (!g_configManager.setMqttPassword(pwd.c_str())) {
        web_sendConfigPage(g_configManager.getLastError(), "");
        return;
      }
    }
  }
  if (server.hasArg("mqttClientId")) {
    String cid = server.arg("mqttClientId");
    if (cid.length() > 0 && cid.length() < sizeof(ConfigData::mqttClientId)) {
      if (!g_configManager.setMqttClientId(cid.c_str())) {
        web_sendConfigPage(g_configManager.getLastError(), "");
        return;
      }
    }
  }
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  if (server.hasArg("sensorInterval")) {
    if (!g_configManager.setSensorInterval(
            server.arg("sensorInterval").toInt())) {
      web_sendConfigPage(g_configManager.getLastError(), "");
      return;
    }
  }
#endif

#if DEVICE_TYPE == 1
  if (server.hasArg("lowTemp")) {
    if (!g_configManager.setLowTemp(server.arg("lowTemp").toFloat())) {
      web_sendConfigPage(g_configManager.getLastError(), "");
      return;
    }
  }
  if (server.hasArg("highTemp")) {
    if (!g_configManager.setHighTemp(server.arg("highTemp").toFloat())) {
      web_sendConfigPage(g_configManager.getLastError(), "");
      return;
    }
  }
  if (server.hasArg("lowHum")) {
    if (!g_configManager.setLowHum(server.arg("lowHum").toFloat())) {
      web_sendConfigPage(g_configManager.getLastError(), "");
      return;
    }
  }
  if (server.hasArg("highHum")) {
    if (!g_configManager.setHighHum(server.arg("highHum").toFloat())) {
      web_sendConfigPage(g_configManager.getLastError(), "");
      return;
    }
  }
  if (server.hasArg("maxOnTime")) {
    if (!g_configManager.setMaxOnTime(server.arg("maxOnTime").toInt())) {
      web_sendConfigPage(g_configManager.getLastError(), "");
      return;
    }
  }
  if (server.hasArg("delaySeconds")) {
    if (!g_configManager.setDelaySeconds(server.arg("delaySeconds").toInt())) {
      web_sendConfigPage(g_configManager.getLastError(), "");
      return;
    }
  }
  if (server.hasArg("speedPercent")) {
    if (!g_configManager.setSpeedPercent(server.arg("speedPercent").toInt())) {
      web_sendConfigPage(g_configManager.getLastError(), "");
      return;
    }
  }

  g_configManager.setAdaptiveMode(server.hasArg("adaptiveMode"));
  g_configManager.setBootState(server.hasArg("bootState"));
  g_configManager.setSensorControlMode(server.hasArg("sensorControlMode"));
#endif

#if DEVICE_TYPE == 3
  if (server.hasArg("maxOnTime")) {
    if (!g_configManager.setMaxOnTime(server.arg("maxOnTime").toInt())) {
      web_sendConfigPage(g_configManager.getLastError(), "");
      return;
    }
  }
  if (server.hasArg("delaySeconds")) {
    if (!g_configManager.setDelaySeconds(server.arg("delaySeconds").toInt())) {
      web_sendConfigPage(g_configManager.getLastError(), "");
      return;
    }
  }
  g_configManager.setBootState(server.hasArg("bootState"));
#endif

  if (!g_configManager.save()) {
    web_sendConfigPage("Error writing to Flash. Please try again.", "");
    return;
  }

  LOG_INFO(CAT_WEB, "Configuration saved successfully, restarting...");
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset='UTF-8'>
    <meta http-equiv='refresh' content='2;url=/'>
    <title>Save</title>
    <style>
        body{font-family:Arial;text-align:center;margin-top:50px;background:#f0f0f0;}
        .success{color:#2e7d32;background:#e8f5e9;padding:20px;border-radius:10px;display:inline-block;}
    </style>
</head>
<body>
    <div class='success'>
        <h2>Configuration saved</h2>
        <p>Rebooting...</p>
    </div>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
#ifdef ESP32
  server.client().flush();
#elif defined(ESP32C3)
  server.client().clear();
#endif
  delay(100);
  ESP.restart();
}

// ========== ИНИЦИАЛИЗАЦИЯ ==========

void web_init(bool setupMode) {
  LOG_DEBUG(CAT_WEB, "web_init() called, setupMode=%d", setupMode);
  g_setupMode = setupMode;

  if (setupMode) {
    // ========== РЕЖИМ НАСТРОЙКИ (AP) ==========
    LOG_DEBUG(CAT_WEB, "Initializing in SETUP mode");

    server.on("/", []() {
      LOG_DEBUG(CAT_WEB, "GET / - Config page (setup mode)");
      web_sendConfigPage("", "");
    });

    server.on("/save", web_saveConfig);
    server.on("/favicon.ico", []() { server.send(404); });

  } else {
    // ========== НОРМАЛЬНЫЙ РЕЖИМ (STA) ==========
    LOG_DEBUG(CAT_WEB, "Initializing in NORMAL mode");

    int refreshInterval = DEFAULT_WEB_REFRESH;
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    refreshInterval = g_configManager.getSensorInterval();
#endif

#if WEB_STATUS_ENABLED == 1
    server.on("/", [refreshInterval]() {
#ifdef ESP32
      server.client().setNoDelay(true);
#endif
      LOG_DEBUG(CAT_WEB, "GET / - serving status page");
      web_sendStatusPage(refreshInterval);
    });
#else
    server.on("/", []() {
      LOG_DEBUG(CAT_WEB, "GET / - redirect to config");
      server.sendHeader("Location", "/config", true);
      server.send(302, "text/plain", "");
    });
#endif

    server.on("/config", []() {
      LOG_DEBUG(CAT_WEB, "GET /config - serving config page");
      web_sendConfigPage("", "");
    });

    server.on("/save", web_saveConfig);
    server.on("/favicon.ico", []() { server.send(404); });

#if WEB_RESET_ENABLED == 1
    server.on("/resetall", []() {
      g_configManager.reset();
      server.send(200, "text/html",
                  F("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta "
                    "http-equiv='refresh' "
                    "content='5;url=/'></head><body><h2>Configuration "
                    "was reset, rebooting...</h2></body></html>"));
      delay(1000);
      ESP.restart();
    });
#endif

#if DEVICE_TYPE == 1
    server.on("/fan/toggle", []() {
      handleToggle();
      server.sendHeader("Location", "/", true);
      server.send(302, "text/plain", "");
    });
    server.on("/fan/auto", []() {
      handleSensorControlMode();
      server.sendHeader("Location", "/", true);
      server.send(302, "text/plain", "");
    });
#elif DEVICE_TYPE == 3
    server.on("/switch/toggle", []() {
      handleToggle();
      server.sendHeader("Location", "/", true);
      server.send(302, "text/plain", "");
    });
#endif
  }

  server.begin();
  LOG_DEBUG(CAT_WEB, "Web server started (mode: %s)",
           setupMode ? "SETUP" : "NORMAL");
}

void web_update() {
  server.handleClient();
}

#endif  // WEB_ENABLED == 1