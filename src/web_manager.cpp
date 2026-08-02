/**
 * @file web_manager.cpp
 * @brief Реализация веб-интерфейса
 * @date 2026-07-28
 */

#include "web_manager.h"
#include "html_templates.h"
#include "web_common.h"
#include "web_ota_manager.h"

#include "settings.h"

#include <cstring>

#include "config_manager.h"
#include "logger.h"
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

void web_handle_set(void) {
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
      web_send_result_page(webSendContent, &server, "Speed must be 0-100%",
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

  web_send_result_page(webSendContent, &server, "Unknown command", false);
}

String web_build_status_html(void) {
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

void web_send_status_page(int refreshInterval) {
  if (!g_statusProvider) {
    server.send(500, "text/html", "Status provider not registered");
    return;
  }

  const char* deviceId = g_configManager.getDeviceId();
  String statusHtml = web_build_status_html();

  if (statusHtml.length() == 0) {
    statusHtml = F("<div class='warning'>Device status is loading...</div>");
    XLOG_WARN(CAT_WEB, "web_build_status_html() returned empty, using fallback");
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

void web_send_config_page(WebSendCallback send,
                          void* context,
                          const ConfigData* cfg,
                          const char* currentMode,
                          const char* currentSsid,
                          const char* currentIp,
                          int refreshSeconds,
                          const char* errorMsg,
                          const char* successMsg) {
  if (!send || !cfg)
    return;

  char buf[384];
  const char* deviceId = g_configManager.getDeviceId();

  send((const char*)FPSTR(HTML_PAGE_START), context);

  if (refreshSeconds > 0) {
    char refresh[64];
    snprintf_P(refresh, sizeof(refresh),
               PSTR("<meta http-equiv='refresh' content='%d'>"),
               refreshSeconds);
    send(refresh, context);
  }

  send((const char*)"<title>", context);
  send(deviceId, context);
  send((const char*)" Configuration</title>", context);
  send((const char*)FPSTR(HTML_STYLE), context);
  send((const char*)"</head><body><div class='container'>", context);

  snprintf(buf, sizeof(buf), "<h1>Settings %s v. %s</h1>", deviceId, VERSION);
  send(buf, context);

  snprintf(buf, sizeof(buf),
           "<h3>State</h3><div class='info'>"
           "Mode: <strong>%s</strong><br>"
           "SSID: <strong>%s</strong><br>"
           "IP: <strong>%s</strong></div>",
           currentMode ? currentMode : "N/A", currentSsid ? currentSsid : "N/A",
           currentIp ? currentIp : "N/A");
  send(buf, context);

  if (errorMsg && errorMsg[0] != '\0') {
    snprintf(buf, sizeof(buf),
             "<div class='error'><strong>Error:</strong> %s</div>", errorMsg);
    send(buf, context);
  }
  if (successMsg && successMsg[0] != '\0') {
    snprintf(buf, sizeof(buf),
             "<div class='success'><strong>Config saved!</strong> %s</div>",
             successMsg);
    send(buf, context);
  }

  send((const char*)"<form method='POST' action='/save'>", context);

  // ===== WiFi =====
  send((const char*)"<h3>WiFi setup</h3>", context);

  FieldDef ssidField;
  ssidField.type = FIELD_TYPE_TEXT;
  ssidField.label = "WiFi SSID";
  ssidField.name = "wifiSsid";
  ssidField.value = cfg->wifiSsid;
  ssidField.placeholder = NULL;
  ssidField.note = NULL;
  ssidField.min = NULL;
  ssidField.max = NULL;
  ssidField.step = NULL;
  ssidField.link = NULL;
  ssidField.buttonText = NULL;
  ssidField.checked = false;
  ssidField.required = true;
  web_renderField(buf, sizeof(buf), &ssidField);
  send(buf, context);

  FieldDef pwdField;
  pwdField.type = FIELD_TYPE_PASSWORD;
  pwdField.label = "WiFi Password";
  pwdField.name = "wifiPassword";
  pwdField.value = NULL;
  pwdField.placeholder = "(hidden)";
  pwdField.note = "Leave empty to keep current password";
  pwdField.min = NULL;
  pwdField.max = NULL;
  pwdField.step = NULL;
  pwdField.link = NULL;
  pwdField.buttonText = NULL;
  pwdField.checked = false;
  pwdField.required = false;
  web_renderField(buf, sizeof(buf), &pwdField);
  send(buf, context);

  // ===== MQTT =====
#if FEATURE_MQTT_ENABLED == 1
  send((const char*)"<h3>MQTT setup</h3>", context);

  FieldDef brokerField;
  brokerField.type = FIELD_TYPE_TEXT;
  brokerField.label = "MQTT Broker";
  brokerField.name = "mqttBroker";
  brokerField.value = cfg->mqttBroker;
  brokerField.placeholder = NULL;
  brokerField.note = NULL;
  brokerField.min = NULL;
  brokerField.max = NULL;
  brokerField.step = NULL;
  brokerField.link = NULL;
  brokerField.buttonText = NULL;
  brokerField.checked = false;
  brokerField.required = true;
  web_renderField(buf, sizeof(buf), &brokerField);
  send(buf, context);

  char portStr[8];
  snprintf(portStr, sizeof(portStr), "%d", cfg->mqttPort);
  FieldDef portField;
  portField.type = FIELD_TYPE_NUMBER;
  portField.label = "MQTT Port";
  portField.name = "mqttPort";
  portField.value = portStr;
  portField.placeholder = NULL;
  portField.note = NULL;
  portField.min = "1";
  portField.max = "65535";
  portField.step = "1";
  portField.link = NULL;
  portField.buttonText = NULL;
  portField.checked = false;
  portField.required = true;
  web_renderField(buf, sizeof(buf), &portField);
  send(buf, context);

  FieldDef userField;
  userField.type = FIELD_TYPE_TEXT;
  userField.label = "MQTT User";
  userField.name = "mqttUser";
  userField.value = cfg->mqttUser;
  userField.placeholder = NULL;
  userField.note = NULL;
  userField.min = NULL;
  userField.max = NULL;
  userField.step = NULL;
  userField.link = NULL;
  userField.buttonText = NULL;
  userField.checked = false;
  userField.required = false;
  web_renderField(buf, sizeof(buf), &userField);
  send(buf, context);

  FieldDef mqttPwdField;
  mqttPwdField.type = FIELD_TYPE_PASSWORD;
  mqttPwdField.label = "MQTT Password";
  mqttPwdField.name = "mqttPassword";
  mqttPwdField.value = NULL;
  mqttPwdField.placeholder = "(hidden)";
  mqttPwdField.note = "Leave empty to keep current password";
  mqttPwdField.min = NULL;
  mqttPwdField.max = NULL;
  mqttPwdField.step = NULL;
  mqttPwdField.link = NULL;
  mqttPwdField.buttonText = NULL;
  mqttPwdField.checked = false;
  mqttPwdField.required = false;
  web_renderField(buf, sizeof(buf), &mqttPwdField);
  send(buf, context);

  FieldDef clientField;
  clientField.type = FIELD_TYPE_TEXT;
  clientField.label = "MQTT Client ID";
  clientField.name = "mqttClientId";
  clientField.value = cfg->mqttClientId;
  clientField.placeholder = NULL;
  clientField.note = NULL;
  clientField.min = NULL;
  clientField.max = NULL;
  clientField.step = NULL;
  clientField.link = NULL;
  clientField.buttonText = NULL;
  clientField.checked = false;
  clientField.required = true;
  web_renderField(buf, sizeof(buf), &clientField);
  send(buf, context);
#endif

  // ===== TYPE 1 =====
#if DEVICE_TYPE == 1
  send((const char*)"<h3>Sensor</h3>", context);

  char tempBuf[16];
  snprintf(tempBuf, sizeof(tempBuf), "%.1f", cfg->lowTemp);
  FieldDef lowTempField;
  lowTempField.type = FIELD_TYPE_NUMBER;
  lowTempField.label = "Low Temp (\u00B0C)";
  lowTempField.name = "lowTemp";
  lowTempField.value = tempBuf;
  lowTempField.placeholder = NULL;
  lowTempField.note = NULL;
  lowTempField.min = "-40";
  lowTempField.max = "85";
  lowTempField.step = "0.1";
  lowTempField.link = NULL;
  lowTempField.buttonText = NULL;
  lowTempField.checked = false;
  lowTempField.required = true;
  web_renderField(buf, sizeof(buf), &lowTempField);
  send(buf, context);

  snprintf(tempBuf, sizeof(tempBuf), "%.1f", cfg->highTemp);
  FieldDef highTempField;
  highTempField.type = FIELD_TYPE_NUMBER;
  highTempField.label = "High Temp (\u00B0C)";
  highTempField.name = "highTemp";
  highTempField.value = tempBuf;
  highTempField.placeholder = NULL;
  highTempField.note = NULL;
  highTempField.min = "-40";
  highTempField.max = "85";
  highTempField.step = "0.1";
  highTempField.link = NULL;
  highTempField.buttonText = NULL;
  highTempField.checked = false;
  highTempField.required = true;
  web_renderField(buf, sizeof(buf), &highTempField);
  send(buf, context);

  char humBuf[16];
  snprintf(humBuf, sizeof(humBuf), "%.1f", cfg->lowHum);
  FieldDef lowHumField;
  lowHumField.type = FIELD_TYPE_NUMBER;
  lowHumField.label = "Low Hum (%)";
  lowHumField.name = "lowHum";
  lowHumField.value = humBuf;
  lowHumField.placeholder = NULL;
  lowHumField.note = NULL;
  lowHumField.min = "0";
  lowHumField.max = "100";
  lowHumField.step = "0.1";
  lowHumField.link = NULL;
  lowHumField.buttonText = NULL;
  lowHumField.checked = false;
  lowHumField.required = true;
  web_renderField(buf, sizeof(buf), &lowHumField);
  send(buf, context);

  snprintf(humBuf, sizeof(humBuf), "%.1f", cfg->highHum);
  FieldDef highHumField;
  highHumField.type = FIELD_TYPE_NUMBER;
  highHumField.label = "High Hum (%)";
  highHumField.name = "highHum";
  highHumField.value = humBuf;
  highHumField.placeholder = NULL;
  highHumField.note = NULL;
  highHumField.min = "0";
  highHumField.max = "100";
  highHumField.step = "0.1";
  highHumField.link = NULL;
  highHumField.buttonText = NULL;
  highHumField.checked = false;
  highHumField.required = true;
  web_renderField(buf, sizeof(buf), &highHumField);
  send(buf, context);

  char intervalBuf[8];
  snprintf(intervalBuf, sizeof(intervalBuf), "%d", cfg->sensorInterval);
  FieldDef intervalField;
  intervalField.type = FIELD_TYPE_NUMBER;
  intervalField.label = "Sensor polling interval (sec)";
  intervalField.name = "sensorInterval";
  intervalField.value = intervalBuf;
  intervalField.placeholder = NULL;
  intervalField.note = NULL;
  intervalField.min = "1";
  intervalField.max = "50";
  intervalField.step = "1";
  intervalField.link = NULL;
  intervalField.buttonText = NULL;
  intervalField.checked = false;
  intervalField.required = true;
  web_renderField(buf, sizeof(buf), &intervalField);
  send(buf, context);

  char maxOnBuf[16];
  snprintf(maxOnBuf, sizeof(maxOnBuf), "%lu", cfg->maxOnTime);
  FieldDef maxOnField;
  maxOnField.type = FIELD_TYPE_NUMBER;
  maxOnField.label = "Emergency timeout (sec)";
  maxOnField.name = "maxOnTime";
  maxOnField.value = maxOnBuf;
  maxOnField.placeholder = NULL;
  maxOnField.note = "0 = disabled";
  maxOnField.min = "0";
  maxOnField.max = "86400";
  maxOnField.step = "1";
  maxOnField.link = NULL;
  maxOnField.buttonText = NULL;
  maxOnField.checked = false;
  maxOnField.required = true;
  web_renderField(buf, sizeof(buf), &maxOnField);
  send(buf, context);

  char delayBuf[16];
  snprintf(delayBuf, sizeof(delayBuf), "%d", cfg->delaySeconds);
  FieldDef delayField;
  delayField.type = FIELD_TYPE_NUMBER;
  delayField.label = "Turn on after (sec)";
  delayField.name = "delaySeconds";
  delayField.value = delayBuf;
  delayField.placeholder = NULL;
  delayField.note = "0 = disabled";
  delayField.min = "0";
  delayField.max = "86400";
  delayField.step = "1";
  delayField.link = NULL;
  delayField.buttonText = NULL;
  delayField.checked = false;
  delayField.required = true;
  web_renderField(buf, sizeof(buf), &delayField);
  send(buf, context);

  char speedBuf[8];
  snprintf(speedBuf, sizeof(speedBuf), "%d", cfg->speedPercent);
  FieldDef speedField;
  speedField.type = FIELD_TYPE_NUMBER;
  speedField.label = "Speed (0-100%)";
  speedField.name = "speedPercent";
  speedField.value = speedBuf;
  speedField.placeholder = NULL;
  speedField.note = "0% - off, 100% - maximal speed";
  speedField.min = "0";
  speedField.max = "100";
  speedField.step = "1";
  speedField.link = NULL;
  speedField.buttonText = NULL;
  speedField.checked = false;
  speedField.required = true;
  web_renderField(buf, sizeof(buf), &speedField);
  send(buf, context);

  FieldDef adaptiveField;
  adaptiveField.type = FIELD_TYPE_CHECKBOX;
  adaptiveField.label = "Enable adaptive mode";
  adaptiveField.name = "adaptiveMode";
  adaptiveField.value = NULL;
  adaptiveField.placeholder = NULL;
  adaptiveField.note =
      "Automatically adjusts speed to maintain temperature and humidity";
  adaptiveField.min = NULL;
  adaptiveField.max = NULL;
  adaptiveField.step = NULL;
  adaptiveField.link = NULL;
  adaptiveField.buttonText = NULL;
  adaptiveField.checked = cfg->adaptiveMode;
  adaptiveField.required = false;
  web_renderField(buf, sizeof(buf), &adaptiveField);
  send(buf, context);

  FieldDef bootField;
  bootField.type = FIELD_TYPE_CHECKBOX;
  bootField.label = "Turn on at startup";
  bootField.name = "bootState";
  bootField.value = NULL;
  bootField.placeholder = NULL;
  bootField.note = "Fan turns on immediately after power is applied";
  bootField.min = NULL;
  bootField.max = NULL;
  bootField.step = NULL;
  bootField.link = NULL;
  bootField.buttonText = NULL;
  bootField.checked = cfg->bootState;
  bootField.required = false;
  web_renderField(buf, sizeof(buf), &bootField);
  send(buf, context);

  FieldDef sensorModeField;
  sensorModeField.type = FIELD_TYPE_CHECKBOX;
  sensorModeField.label = "Sensor control";
  sensorModeField.name = "sensorControlMode";
  sensorModeField.value = NULL;
  sensorModeField.placeholder = NULL;
  sensorModeField.note = "When enabled, fan is controlled by sensors";
  sensorModeField.min = NULL;
  sensorModeField.max = NULL;
  sensorModeField.step = NULL;
  sensorModeField.link = NULL;
  sensorModeField.buttonText = NULL;
  sensorModeField.checked = cfg->sensorControlMode;
  sensorModeField.required = false;
  web_renderField(buf, sizeof(buf), &sensorModeField);
  send(buf, context);

#elif DEVICE_TYPE == 2
  char intervalBuf[8];
  snprintf(intervalBuf, sizeof(intervalBuf), "%d", cfg->sensorInterval);
  FieldDef intervalField;
  intervalField.type = FIELD_TYPE_NUMBER;
  intervalField.label = "Reading interval (sec)";
  intervalField.name = "sensorInterval";
  intervalField.value = intervalBuf;
  intervalField.placeholder = NULL;
  intervalField.note = NULL;
  intervalField.min = "1";
  intervalField.max = "50";
  intervalField.step = "1";
  intervalField.link = NULL;
  intervalField.buttonText = NULL;
  intervalField.checked = false;
  intervalField.required = true;
  web_renderField(buf, sizeof(buf), &intervalField);
  send(buf, context);

#elif DEVICE_TYPE == 3
  char maxOnBuf[16];
  snprintf(maxOnBuf, sizeof(maxOnBuf), "%lu", cfg->maxOnTime);
  FieldDef maxOnField;
  maxOnField.type = FIELD_TYPE_NUMBER;
  maxOnField.label = "Emergency timeout (sec)";
  maxOnField.name = "maxOnTime";
  maxOnField.value = maxOnBuf;
  maxOnField.placeholder = NULL;
  maxOnField.note = "0 = disabled";
  maxOnField.min = "0";
  maxOnField.max = "86400";
  maxOnField.step = "1";
  maxOnField.link = NULL;
  maxOnField.buttonText = NULL;
  maxOnField.checked = false;
  maxOnField.required = true;
  web_renderField(buf, sizeof(buf), &maxOnField);
  send(buf, context);

  char delayBuf[16];
  snprintf(delayBuf, sizeof(delayBuf), "%d", cfg->delaySeconds);
  FieldDef delayField;
  delayField.type = FIELD_TYPE_NUMBER;
  delayField.label = "Turn on after (sec)";
  delayField.name = "delaySeconds";
  delayField.value = delayBuf;
  delayField.placeholder = NULL;
  delayField.note = "0 = disabled";
  delayField.min = "0";
  delayField.max = "86400";
  delayField.step = "1";
  delayField.link = NULL;
  delayField.buttonText = NULL;
  delayField.checked = false;
  delayField.required = true;
  web_renderField(buf, sizeof(buf), &delayField);
  send(buf, context);

  FieldDef bootField;
  bootField.type = FIELD_TYPE_CHECKBOX;
  bootField.label = "Turn on at startup";
  bootField.name = "bootState";
  bootField.value = NULL;
  bootField.placeholder = NULL;
  bootField.note = "Switch turns on immediately after power is applied";
  bootField.min = NULL;
  bootField.max = NULL;
  bootField.step = NULL;
  bootField.link = NULL;
  bootField.buttonText = NULL;
  bootField.checked = cfg->bootState;
  bootField.required = false;
  web_renderField(buf, sizeof(buf), &bootField);
  send(buf, context);
#endif

  FieldDef confirmField;
  confirmField.type = FIELD_TYPE_CHECKBOX;
  confirmField.label = "Confirm saving";
  confirmField.name = "confirmSave";
  confirmField.value = NULL;
  confirmField.placeholder = NULL;
  confirmField.note = "Required — check to confirm changes";
  confirmField.min = NULL;
  confirmField.max = NULL;
  confirmField.step = NULL;
  confirmField.link = NULL;
  confirmField.buttonText = NULL;
  confirmField.checked = false;
  confirmField.required = true;
  web_renderField(buf, sizeof(buf), &confirmField);
  send(buf, context);

  // ===== SUBMIT =====
  send((const char*)"<input type='submit' value='Save'>", context);
  send((const char*)"</form>", context);

  if (ota_is_available()) {
    send((const char*)"<a href='/update' class='link-btn'>Upgrade firmware (OTA)</a>", context);
  }

  send((const char*)"<a href='/' class='link-btn'>Home</a>", context);
  send((const char*)FPSTR(HTML_PAGE_END), context);
}

void web_handle_save(void) {
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
      // Ошибка — отправить страницу с ошибкой
      const ConfigData* cfg = g_configManager.get();
      String currentIp = wifi_get_local_ip();
      server.setContentLength(CONTENT_LENGTH_UNKNOWN);
      server.send(200, "text/html", "");
      web_send_config_page(webSendContent, &server, cfg, "Client WiFi", cfg->wifiSsid,
                     currentIp.c_str(), 0, "WiFi SSID is empty or too long",
                     "");
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
        const ConfigData* cfg = g_configManager.get();
        String currentIp = wifi_get_local_ip();
        server.setContentLength(CONTENT_LENGTH_UNKNOWN);
        server.send(200, "text/html", "");
        web_send_config_page(webSendContent, &server, cfg, "Client WiFi",
                       cfg->wifiSsid, currentIp.c_str(), 0,
                       "WiFi password too long", "");
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
      const ConfigData* cfg = g_configManager.get();
      String currentIp = wifi_get_local_ip();
      server.setContentLength(CONTENT_LENGTH_UNKNOWN);
      server.send(200, "text/html", "");
      web_send_config_page(webSendContent, &server, cfg, "Client WiFi", cfg->wifiSsid,
                     currentIp.c_str(), 0, "MQTT Broker is empty or too long",
                     "");
      return;
    }
  }

  if (server.hasArg("mqttPort")) {
    int port = server.arg("mqttPort").toInt();
    if (port >= 1 && port <= 65535) {
      g_webPendingConfig.mqttPort = (uint16_t)port;
    } else {
      const ConfigData* cfg = g_configManager.get();
      String currentIp = wifi_get_local_ip();
      server.setContentLength(CONTENT_LENGTH_UNKNOWN);
      server.send(200, "text/html", "");
      web_send_config_page(webSendContent, &server, cfg, "Client WiFi", cfg->wifiSsid,
                     currentIp.c_str(), 0, "MQTT Port must be 1-65535", "");
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
      const ConfigData* cfg = g_configManager.get();
      String currentIp = wifi_get_local_ip();
      server.setContentLength(CONTENT_LENGTH_UNKNOWN);
      server.send(200, "text/html", "");
      web_send_config_page(webSendContent, &server, cfg, "Client WiFi", cfg->wifiSsid,
                     currentIp.c_str(), 0,
                     "MQTT Client ID is empty or too long", "");
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
      const ConfigData* cfg = g_configManager.get();
      String currentIp = wifi_get_local_ip();
      server.setContentLength(CONTENT_LENGTH_UNKNOWN);
      server.send(200, "text/html", "");
      web_send_config_page(webSendContent, &server, cfg, "Client WiFi", cfg->wifiSsid,
                     currentIp.c_str(), 0, "Sensor interval out of range", "");
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
      const ConfigData* cfg = g_configManager.get();
      String currentIp = wifi_get_local_ip();
      server.setContentLength(CONTENT_LENGTH_UNKNOWN);
      server.send(200, "text/html", "");
      web_send_config_page(webSendContent, &server, cfg, "Client WiFi", cfg->wifiSsid,
                     currentIp.c_str(), 0, "Low Temp out of range", "");
      return;
    }
  }

  if (server.hasArg("highTemp")) {
    float val = server.arg("highTemp").toFloat();
    if (val >= TEMP_MIN && val <= TEMP_MAX) {
      g_webPendingConfig.highTemp = val;
    } else {
      const ConfigData* cfg = g_configManager.get();
      String currentIp = wifi_get_local_ip();
      server.setContentLength(CONTENT_LENGTH_UNKNOWN);
      server.send(200, "text/html", "");
      web_send_config_page(webSendContent, &server, cfg, "Client WiFi", cfg->wifiSsid,
                     currentIp.c_str(), 0, "High Temp out of range", "");
      return;
    }
  }

  if (server.hasArg("lowHum")) {
    float val = server.arg("lowHum").toFloat();
    if (val >= HUM_MIN && val <= HUM_MAX) {
      g_webPendingConfig.lowHum = val;
    } else {
      const ConfigData* cfg = g_configManager.get();
      String currentIp = wifi_get_local_ip();
      server.setContentLength(CONTENT_LENGTH_UNKNOWN);
      server.send(200, "text/html", "");
      web_send_config_page(webSendContent, &server, cfg, "Client WiFi", cfg->wifiSsid,
                     currentIp.c_str(), 0, "Low Hum out of range", "");
      return;
    }
  }

  if (server.hasArg("highHum")) {
    float val = server.arg("highHum").toFloat();
    if (val >= HUM_MIN && val <= HUM_MAX) {
      g_webPendingConfig.highHum = val;
    } else {
      const ConfigData* cfg = g_configManager.get();
      String currentIp = wifi_get_local_ip();
      server.setContentLength(CONTENT_LENGTH_UNKNOWN);
      server.send(200, "text/html", "");
      web_send_config_page(webSendContent, &server, cfg, "Client WiFi", cfg->wifiSsid,
                     currentIp.c_str(), 0, "High Hum out of range", "");
      return;
    }
  }

  if (server.hasArg("maxOnTime")) {
    uint32_t val = server.arg("maxOnTime").toInt();
    if (val <= MAX_ON_TIME_MAX) {
      g_webPendingConfig.maxOnTime = val;
    } else {
      const ConfigData* cfg = g_configManager.get();
      String currentIp = wifi_get_local_ip();
      server.setContentLength(CONTENT_LENGTH_UNKNOWN);
      server.send(200, "text/html", "");
      web_send_config_page(webSendContent, &server, cfg, "Client WiFi", cfg->wifiSsid,
                     currentIp.c_str(), 0, "MaxOnTime out of range", "");
      return;
    }
  }

  if (server.hasArg("delaySeconds")) {
    int val = server.arg("delaySeconds").toInt();
    if (val >= DELAY_SECONDS_MIN && val <= DELAY_SECONDS_MAX) {
      g_webPendingConfig.delaySeconds = val;
    } else {
      const ConfigData* cfg = g_configManager.get();
      String currentIp = wifi_get_local_ip();
      server.setContentLength(CONTENT_LENGTH_UNKNOWN);
      server.send(200, "text/html", "");
      web_send_config_page(webSendContent, &server, cfg, "Client WiFi", cfg->wifiSsid,
                     currentIp.c_str(), 0, "Delay seconds out of range", "");
      return;
    }
  }

  if (server.hasArg("speedPercent")) {
    int val = server.arg("speedPercent").toInt();
    if (val >= 0 && val <= 100) {
      g_webPendingConfig.speedPercent = (uint16_t)val;
    } else {
      const ConfigData* cfg = g_configManager.get();
      String currentIp = wifi_get_local_ip();
      server.setContentLength(CONTENT_LENGTH_UNKNOWN);
      server.send(200, "text/html", "");
      web_send_config_page(webSendContent, &server, cfg, "Client WiFi", cfg->wifiSsid,
                     currentIp.c_str(), 0, "Speed must be 0-100%", "");
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
      const ConfigData* cfg = g_configManager.get();
      String currentIp = wifi_get_local_ip();
      server.setContentLength(CONTENT_LENGTH_UNKNOWN);
      server.send(200, "text/html", "");
      web_send_config_page(webSendContent, &server, cfg, "Client WiFi", cfg->wifiSsid,
                     currentIp.c_str(), 0, "MaxOnTime out of range", "");
      return;
    }
  }

  if (server.hasArg("delaySeconds")) {
    int val = server.arg("delaySeconds").toInt();
    if (val >= DELAY_SECONDS_MIN && val <= DELAY_SECONDS_MAX) {
      g_webPendingConfig.delaySeconds = val;
    } else {
      const ConfigData* cfg = g_configManager.get();
      String currentIp = wifi_get_local_ip();
      server.setContentLength(CONTENT_LENGTH_UNKNOWN);
      server.send(200, "text/html", "");
      web_send_config_page(webSendContent, &server, cfg, "Client WiFi", cfg->wifiSsid,
                     currentIp.c_str(), 0, "Delay seconds out of range", "");
      return;
    }
  }

  g_webPendingConfig.bootState = server.hasArg("bootState");
#endif

  g_webConfigPending = true;
  XLOG_INFO(CAT_WEB, "Config parsed, pending for main to apply");

  // Отправить страницу успеха
  const ConfigData* cfg = g_configManager.get();
  String currentIp = wifi_get_local_ip();
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");
  web_send_result_page(webSendContent, &server,
                       "Configuration saved. Page will reloaded.", true);
}

// ============================================================================
// ИНИЦИАЛИЗАЦИЯ WEB
// ============================================================================

void web_init(void) {
  int refreshInterval = DEFAULT_WEB_REFRESH;
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  refreshInterval = g_configManager.getSensorInterval();
#endif

  server.on("/", [refreshInterval]() {
    XLOG_DEBUG(CAT_WEB, "GET / - serving status page");
#ifdef ESP32
    server.client().setNoDelay(true);
#endif
    web_send_status_page(refreshInterval);
  });

  server.on("/config", []() {
    XLOG_DEBUG(CAT_WEB, "GET /config - serving config page");
    const ConfigData* cfg = g_configManager.get();
    String currentIp = wifi_get_local_ip();
    server.setContentLength(CONTENT_LENGTH_UNKNOWN);
    server.send(200, "text/html", "");
    web_send_config_page(webSendContent, &server, cfg, "Client WiFi", cfg->wifiSsid,
                   currentIp.c_str(), 0, "", "");
  });

  server.on("/save", web_handle_save);
  server.on("/favicon.ico", []() { server.send(404); });
  server.on("/set", web_handle_set);

#if WEB_RESET_ENABLED == 1
  server.on("/resetall", []() {
    g_webRestartPending = true;
    web_send_result_page(webSendContent, &server,
                       "Configuration was reset, rebooting...", true);
  });
#endif

  if (ota_is_available()) {
    web_ota_manager_init(&server);
  }

  server.begin();
  XLOG_DEBUG(CAT_WEB, "Web server started (NORMAL mode)");
}

void web_update(void) {
  server.handleClient();
}

void web_register_status_provider(IWebStatusProvider* provider) {
  g_statusProvider = provider;
}

#endif  // TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI && FEATURE_WEB_STATUS_ENABLED
        // == 1