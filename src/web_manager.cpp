/**
 * @file web_manager.cpp
 * @brief Реализация веб-интерфейса
 * @date 2026-07-28
 */

#include "web_manager.h"
#include "web_ota_manager.h"
#include "web_common.h"
#include "html_templates.h"

#include "settings.h"

#include <cstring>

#include "config_manager.h"
#include "logger.h"
#include "provisioning_manager.h" //@deprecated see FIXME 1.10
#include "sensor.h"
#include "system_state.h"


#include "wifi_manager.h"

#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI && FEATURE_WEB_STATUS_ENABLED == 1

volatile bool g_webConfigPending = false;
volatile bool g_webRestartPending = false;
volatile bool g_webCommandPending = false;
ConfigData g_webPendingConfig;
WebCommand g_webCommand;

WebServerClass server(80);
static bool g_setupMode = false;
static IWebStatusProvider* g_statusProvider = nullptr;

/**
 * @brief Форматировать оставшееся время
 * @param remainingMs Время в миллисекундах
 * @return Строка с временем (например, "1 min 30 sec")
 */
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

/**
 * @brief Получить HTML-строку текущего режима
 * @return HTML с цветовой индикацией
 */
static String getCurrentModeText(void) {
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

/**
 * @brief Получить оставшееся время аварийного отключения
 * @return Строка с временем
 */
static String getMaxOnTimeRemaining(void) {
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

/**
 * @brief Получить оставшееся время таймера отложенного включения
 * @return Строка с временем
 */
static String getDelayTimerRemaining(void) {
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

void handleSetCommand(void) {
  if (!g_statusProvider) {
    server.send(500, "text/plain", "Status provider not registered");
    return;
  }

  String param = server.arg("param");
  String value = server.arg("value");

  XLOG_INFO(CAT_WEB, "SET command: %s = %s", param.c_str(), value.c_str());

  if (param == "state") {
    bool on = (value == "on" || value == "1");
    g_webCommandPending = true;
    g_webCommand.type = CMD_STATE;
    g_webCommand.value.boolVal = on;
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
    return;
  }

#if DEVICE_TYPE == 1
  if (param == "speed") {
    int speed = value.toInt();
    if (speed < 0 || speed > 100) {
      web_sendResultPage(webSendContent, &server, "Speed must be 0-100%",
                         false);
      return;
    }
    g_webCommandPending = true;
    g_webCommand.type = CMD_SPEED;
    g_webCommand.value.intVal = speed;
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
    return;
  }
#endif

#if DEVICE_TYPE == 1
  if (param == "manualMode") {
    bool enabled = (value == "1" || value == "on");
    g_webCommandPending = true;
    g_webCommand.type = CMD_MANUAL_MODE;
    g_webCommand.value.boolVal = enabled;
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
    return;
  }
#endif

  web_sendResultPage(webSendContent, &server, "Unknown command", false);
}

String web_buildStatusHtml(void) {
  if (!g_statusProvider) {
    return F(
        "<div class='block error center'>Status provider not registered</div>");
  }

  String html;
  char buf[512];
  bool sensorOk = g_statusProvider->isSensorOk();
  bool state = g_statusProvider->isDeviceOn();

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  if (sensorOk) {
    float temp = g_statusProvider->getTemperature();
    float hum = g_statusProvider->getHumidity();

    const char* tempColorClass = "info";
    const char* humColorClass = "info";

#if DEVICE_TYPE == 1
    if (temp >= g_statusProvider->getHighTemp())
      tempColorClass = "error";
    else if (temp <= g_statusProvider->getLowTemp())
      tempColorClass = "success";

    if (hum >= g_statusProvider->getHighHum())
      humColorClass = "error";
    else if (hum <= g_statusProvider->getLowHum())
      humColorClass = "success";
#endif

    char tempNote[64] = "";
    char humNote[64] = "";
#if DEVICE_TYPE == 1
    snprintf(tempNote, sizeof(tempNote), "off: %.1f on: %.1f",
             g_statusProvider->getLowTemp(), g_statusProvider->getHighTemp());
    snprintf(humNote, sizeof(humNote), "off: %.1f on: %.1f",
             g_statusProvider->getLowHum(), g_statusProvider->getHighHum());
#endif

    html += F("<div class='flex-container'>");
    web_renderSensorCard(buf, sizeof(buf), temp, "Temperature", "°C",
                         tempColorClass, tempNote);
    html += buf;
    web_renderSensorCard(buf, sizeof(buf), hum, "Humidity", "%", humColorClass,
                         humNote);
    html += buf;
    html += F("</div>");
  } else {
    html +=
        F("<div class='block error center'><strong>Sensor error</strong><br>");
    html += g_statusProvider->getSensorError();
    html += F("</div>");
  }
#endif

  if (g_statusProvider->isEmergencyStop()) {
    FieldDef emergency;
    emergency.type = FIELD_TYPE_CARD;
    emergency.label = "EMERGENCY STOPPED";
    emergency.value = NULL;
    emergency.note =
        "Device was automatically turned off after exceeding the maximum "
        "allowed runtime.";
    emergency.name = NULL;
    emergency.placeholder = NULL;
    emergency.min = NULL;
    emergency.max = NULL;
    emergency.step = NULL;
    emergency.link = NULL;
    emergency.buttonText = NULL;
    emergency.checked = false;
    emergency.required = false;
    web_renderField(buf, sizeof(buf), &emergency);
    html += buf;
  }

  html +=
      F("<div class='block info center'>"
        "<div class='text_header'>Mode: ");
  html += getCurrentModeText();
  html += F("</div></div>");

  const char* stateText = state ? "ON" : "OFF";
  web_renderStatusCard(buf, sizeof(buf), stateText, state);
  html += buf;

#if DEVICE_TYPE == 1
  if (state) {
    int speed = g_statusProvider->getSpeedPercent();
    html +=
        F("<div class='block info center'>"
          "<div class='text_header'>Speed: ");
    html += String(speed);
    html += F("%</div>");
    web_renderSpeedBar(buf, sizeof(buf), speed);
    html += buf;
    if (speed < 100) {
      html += F("<div class='text_small'>Quiet mode active");
      if (g_statusProvider->isAdaptiveModeActive())
        html += F(" + adaptive");
      html += F("</div>");
    }
    html += F("</div>");
  }
#endif

  html += F("<hr><div class='block info'>");

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

  html += F("<div class='button-group'>");

  const char* onOffLabel = state ? "Turn OFF" : "Turn ON";
  const char* onOffValue = state ? "off" : "on";
  char url[64];
  snprintf(url, sizeof(url), "/set?param=state&value=%s", onOffValue);
  web_renderButton(buf, sizeof(buf), onOffLabel, url, "link-btn");
  html += buf;

#if DEVICE_TYPE == 1
  if (state) {
    web_renderButton(buf, sizeof(buf), "25%", "/set?param=speed&value=25",
                     NULL);
    html += buf;
    web_renderButton(buf, sizeof(buf), "50%", "/set?param=speed&value=50",
                     NULL);
    html += buf;
    web_renderButton(buf, sizeof(buf), "75%", "/set?param=speed&value=75",
                     NULL);
    html += buf;
    web_renderButton(buf, sizeof(buf), "100%", "/set?param=speed&value=100",
                     NULL);
    html += buf;
  }
#endif

#if DEVICE_TYPE == 1
  if (g_statusProvider->isSensorControlMode()) {
    web_renderButton(buf, sizeof(buf), "Manual mode",
                     "/set?param=manualMode&value=1", NULL);
  } else {
    web_renderButton(buf, sizeof(buf), "Auto mode",
                     "/set?param=manualMode&value=0", NULL);
  }
  html += buf;
#endif

  html += F("</div>");

  return html;
}

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

  char title[32];
  snprintf(title, sizeof(title), "%s Status", deviceId);
  web_sendPageStart(webSendContent, &server, title, PAGE_MODE_NORMAL);
  web_sendRefreshMeta(webSendContent, &server, refreshInterval, NULL);

  char header[64];
  snprintf(header, sizeof(header), "<h1>%s VERSION %s</h1>", deviceId, VERSION);
  webSendContent(header, &server);

  webSendContent(statusHtml.c_str(), &server);

  webSendContent(
      "<div class='button-group'><a href='/config'><button "
      "class='link-btn'>Settings</button></a></div>",
      &server);

  web_sendPageEnd(webSendContent, &server);
}

void web_sendConfigPage(const char* errorMsg, const char* successMsg) {
  const ConfigData* cfg = g_configManager.get();
  bool isApMode = system_state_has_bit(STATE_PROVISIONING);
  const char* deviceId = g_configManager.getDeviceId();

  const char* currentMode = isApMode ? "Access Point" : "Client WiFi";
  const char* currentSsid = isApMode ? deviceId : cfg->wifiSsid;

  String currentIp = wifi_get_local_ip();

  int refreshSeconds =
      (successMsg && strlen(successMsg) > 0) ? DEFAULT_WEB_REFRESH : 0;

  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

  sendConfigPage(webSendContent, &server, cfg, currentMode, currentSsid,
                 currentIp.c_str(), refreshSeconds, g_setupMode, errorMsg,
                 successMsg);
}

void web_saveConfig(void) {
  XLOG_INFO(CAT_WEB, "Processing config form...");

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
    if (val >= TEMP_MIN && val <= TEMP_MAX) {
      g_webPendingConfig.lowTemp = val;
    } else {
      web_sendConfigPage("Low Temp out of range", "");
      return;
    }
  }

  if (server.hasArg("highTemp")) {
    float val = server.arg("highTemp").toFloat();
    if (val >= TEMP_MIN && val <= TEMP_MAX) {
      g_webPendingConfig.highTemp = val;
    } else {
      web_sendConfigPage("High Temp out of range", "");
      return;
    }
  }

  if (server.hasArg("lowHum")) {
    float val = server.arg("lowHum").toFloat();
    if (val >= HUM_MIN && val <= HUM_MAX) {
      g_webPendingConfig.lowHum = val;
    } else {
      web_sendConfigPage("Low Hum out of range", "");
      return;
    }
  }

  if (server.hasArg("highHum")) {
    float val = server.arg("highHum").toFloat();
    if (val >= HUM_MIN && val <= HUM_MAX) {
      g_webPendingConfig.highHum = val;
    } else {
      web_sendConfigPage("High Hum out of range", "");
      return;
    }
  }

  if (server.hasArg("maxOnTime")) {
    uint32_t val = server.arg("maxOnTime").toInt();
    if (val <= MAX_ON_TIME_MAX) {
      g_webPendingConfig.maxOnTime = val;
    } else {
      web_sendConfigPage("MaxOnTime out of range", "");
      return;
    }
  }

  if (server.hasArg("delaySeconds")) {
    int val = server.arg("delaySeconds").toInt();
    if (val >= DELAY_SECONDS_MIN && val <= DELAY_SECONDS_MAX) {
      g_webPendingConfig.delaySeconds = val;
    } else {
      web_sendConfigPage("Delay seconds out of range", "");
      return;
    }
  }

  if (server.hasArg("speedPercent")) {
    int val = server.arg("speedPercent").toInt();
    if (val >= 0 && val <= 100) {
      g_webPendingConfig.speedPercent = (uint16_t)val;
    } else {
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
    if (val <= MAX_ON_TIME_MAX) {
      g_webPendingConfig.maxOnTime = val;
    } else {
      web_sendConfigPage("MaxOnTime out of range", "");
      return;
    }
  }

  if (server.hasArg("delaySeconds")) {
    int val = server.arg("delaySeconds").toInt();
    if (val >= DELAY_SECONDS_MIN && val <= DELAY_SECONDS_MAX) {
      g_webPendingConfig.delaySeconds = val;
    } else {
      web_sendConfigPage("Delay seconds out of range", "");
      return;
    }
  }

  g_webPendingConfig.bootState = server.hasArg("bootState");
#endif

  g_webConfigPending = true;
  XLOG_INFO(CAT_WEB, "Config parsed, pending for main to apply");

  web_sendResultPage(webSendContent, &server, "Configuration saved", true);
}

// ============================================================================
// ИНИЦИАЛИЗАЦИЯ WEB
// ============================================================================

void web_init(bool setupMode) {
  g_setupMode = setupMode;

  if (setupMode) {
    // ===== AP-режим: вся логика в provisioning =====
    server.on("/", []() { provisioning_send_ap_page(&server); });

    server.on("/savewifi", HTTP_POST,
              []() { provisioning_handle_ap_save(&server); });

    // Перенаправляем /config на / для единообразия
    server.on("/config", []() {
      server.sendHeader("Location", "/");
      server.send(302, "text/plain", "Redirecting...");
    });

    server.on("/favicon.ico", []() { server.send(404); });

  } else {
    // ===== Обычный режим =====
    int refreshInterval = DEFAULT_WEB_REFRESH;
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    refreshInterval = g_configManager.getSensorInterval();
#endif

    server.on("/", [refreshInterval]() {
      XLOG_DEBUG(CAT_WEB, "GET / - serving status page");
#ifdef ESP32
      server.client().setNoDelay(true);
#endif
      web_sendStatusPage(refreshInterval);
    });

    server.on("/config", []() {
      XLOG_DEBUG(CAT_WEB, "GET /config - serving config page");
      web_sendConfigPage("", "");
    });

    server.on("/save", web_saveConfig);
    server.on("/favicon.ico", []() { server.send(404); });
    server.on("/set", handleSetCommand);

#if WEB_RESET_ENABLED == 1
    server.on("/resetall", []() {
      g_webRestartPending = true;
      web_sendResultPage(webSendContent, &server,
                         "Configuration was reset, rebooting...", true);
    });
#endif

    if (ota_is_available()) {
      web_ota_manager_init(&server);
    }
  }

  server.begin();
  XLOG_DEBUG(CAT_WEB, "Web server started. (Mode: %s)",
             setupMode ? "SETUP" : "NORMAL");
}

void web_update(void) {
  server.handleClient();
}

void web_registerStatusProvider(IWebStatusProvider* provider) {
  g_statusProvider = provider;
}

#endif  // TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI && FEATURE_WEB_STATUS_ENABLED
        // == 1