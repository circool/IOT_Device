#include "web.h"
#include "sensor.h"
#include "web_templates.h"
// #include "led.h"
#include "config.h"
#include "logger.h"
#include "ota.h"
#include "wifi_manager.h"

// extern WebServer server;
#if WEB_ENABLED == 1
WebServerClass server(80);
#endif

#if DEVICE_TYPE == 1
#include "fan_actuator.h"
#endif

#if DEVICE_TYPE == 3
#include "switch_actuator.h"
#endif

#if MQTT_ENABLED == 1
#include "mqtt.h"
#endif

// ========== ГЛОБАЛЬНЫЕ УКАЗАТЕЛИ НА АКТУАТОРЫ ==========
static FanActuator* g_fanActuator = nullptr;
static SwitchActuator* g_switchActuator = nullptr;

void web_registerActuators(FanActuator* fanPtr, SwitchActuator* switchPtr) {
  g_fanActuator = fanPtr;
  g_switchActuator = switchPtr;
}

// ========== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ДЛЯ РАСЧЁТА ТАЙМЕРОВ ==========
static String formatRemainingTime(unsigned long remainingMs) {
  if (remainingMs <= 0)
    return "0 sec";
  unsigned long remainingSec = remainingMs / 1000;
  if (remainingSec < 60)
    return String(remainingSec) + " sec";
  if (remainingSec < 3600)
    return String(remainingSec / 60) + " min " + String(remainingSec % 60) +
           " sec";
  return String(remainingSec / 3600) + " ч " +
         String((remainingSec % 3600) / 60) + " min";
}

static bool isEmergencyStopActive() {
#if DEVICE_TYPE == 1
  if (g_fanActuator != nullptr) {
    return g_fanActuator->isEmergencyStop();
  }
#elif DEVICE_TYPE == 3
  if (g_switchActuator != nullptr) {
    return g_switchActuator->isEmergencyStop();
  }
#endif
  return false;
}

static String getCurrentModeText() {
#if DEVICE_TYPE == 1
  if (g_fanActuator == nullptr)
    return "NA";

  if (config_get()->sensorControlMode) {
    return F("<span style='color:#4CAF50;'>SENSOR</span>");
  }

  if (g_fanActuator->isDelayActive()) {
    unsigned long remaining = g_fanActuator->getDelayTimer() - millis();
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

#elif DEVICE_TYPE == 3
  if (g_switchActuator == nullptr)
    return "Н/Д";

  if (g_switchActuator->isDelayActive()) {
    unsigned long remaining = g_switchActuator->getDelayTimer() - millis();
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

#else
  return "";
#endif
}

static String getMaxOnTimeRemaining() {
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  uint32_t maxOnTime = config_get()->maxOnTime;
  if (maxOnTime == 0)
    return "отключён";

  bool isOn = false;
  unsigned long startTime = 0;

#if DEVICE_TYPE == 1
  if (g_fanActuator != nullptr && g_fanActuator->getState()) {
    isOn = true;
    startTime = g_fanActuator->getStartTime();
  }
#elif DEVICE_TYPE == 3
  if (g_switchActuator != nullptr && g_switchActuator->getState()) {
    isOn = true;
    startTime = g_switchActuator->getStartTime();
  }
#endif

  if (!isOn || startTime == 0) {
    return String(maxOnTime) + " sec (not active)";
  }

  unsigned long elapsed = millis() - startTime;
  if (elapsed >= maxOnTime * 1000UL)
    return "0 sec (сработает)";

  unsigned long remaining = (maxOnTime * 1000UL) - elapsed;
  return formatRemainingTime(remaining);
#else
  return "NA";
#endif
}

static String getDelayTimerRemaining() {
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  bool delayActive = false;
  unsigned long delayTimer = 0;

#if DEVICE_TYPE == 1
  if (g_fanActuator != nullptr) {
    delayActive = g_fanActuator->isDelayActive();
    delayTimer = g_fanActuator->getDelayTimer();
  }
#elif DEVICE_TYPE == 3
  if (g_switchActuator != nullptr) {
    delayActive = g_switchActuator->isDelayActive();
    delayTimer = g_switchActuator->getDelayTimer();
  }
#endif

  if (!delayActive) {
    uint16_t delaySec = config_get()->delaySeconds;
    if (delaySec > 0) {
      return String(delaySec) + " sec (inactive)";
    } else {
      return "disabled (0 sec)";
    }
  }

  unsigned long remaining = delayTimer - millis();
  if (remaining <= 0)
    return "0 sec (will turn on)";
  return formatRemainingTime(remaining);
#else
  return "NA";
#endif
}

// ========== ФОРМИРОВАНИЕ СТАТУСА ДЛЯ СТРАНИЦЫ СОСТОЯНИЯ ==========
String web_buildStatusHtml() {
  String html;

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  html += F("<div class='flex-container'>");

  float currentTemp = sensor_getTemperature();
  float currentHum = sensor_getHumidity();

#if DEVICE_TYPE == 1
  String tempColor =
      (currentTemp >= config_get()->highTemp)
          ? "#f44336"
          : ((currentTemp <= config_get()->lowTemp) ? "#4CAF50" : "#2196F3");
  String humColor =
      (currentHum >= config_get()->highHum)
          ? "#f44336"
          : ((currentHum <= config_get()->lowHum) ? "#4CAF50" : "#2196F3");
#else
  String tempColor = "#2196F3";
  String humColor = "#2196F3";
#endif

  html += F("<div class='sensor-card' style='background:");
  html += tempColor;
  html += F("20; border:2px solid ");
  html += tempColor;
  html += F(";'>");
  html += F("<div class='sensor-value' style='color:");
  html += tempColor;
  html += F(";'>");
  html += String(currentTemp, 1);
  html += F(" °C</div>");
  html += F("<div class='sensor-label'>Temperature");
#if DEVICE_TYPE == 1
  html += F(" (off: ");
  html += String(config_get()->lowTemp, 1);
  html += F(" on: ");
  html += String(config_get()->highTemp, 1);
  html += F(")");
#endif
  html += F("</div></div>");

  html += F("<div class='sensor-card' style='background:");
  html += humColor;
  html += F("20; border:2px solid ");
  html += humColor;
  html += F(";'>");
  html += F("<div class='sensor-value' style='color:");
  html += humColor;
  html += F(";'>");
  html += String(currentHum, 1);
  html += F(" %</div>");
  html += F("<div class='sensor-label'>Humidity");
#if DEVICE_TYPE == 1
  html += F(" (off: ");
  html += String(config_get()->lowHum, 1);
  html += F(" on: ");
  html += String(config_get()->highHum, 1);
  html += F(")");
#endif
  html += F("</div></div>");

  html += F("</div>");

  if (!sensor_isOk() && strlen(sensor_getError()) > 0) {
    html += F("<div class='sensor-error'><strong>Sensor error</strong><br>");
    html += sensor_getError();
    html += F("</div>");
  }
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  if (isEmergencyStopActive()) {
    html += F("<div class='status-card error'>");
    html += F("<div style='font-size:1.2em;'>EMERGENCY STOPPED</div>");
    html += F("<div class='note'>");
    html +=
        F("Device was automatically turned off after exceeding the maximum "
          "allowed runtime.");
    html += F("</div></div>");
  }
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  html +=
      F("<div class='status-card' style='background:#f5f5f5; border:2px solid "
        "#ddd;'>");
  html += F("<div style='font-size:1.5em;font-weight:bold;'>Mode: ");
  html += getCurrentModeText();
  html += F("</div></div>");
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
#if DEVICE_TYPE == 1
  bool state = (g_fanActuator != nullptr) ? g_fanActuator->getState() : false;
  const char* label = "Fan";
  const char* toggleUrl = "/fan/toggle";
#else
  bool state =
      (g_switchActuator != nullptr) ? g_switchActuator->getState() : false;
  const char* label = "Switch";
  const char* toggleUrl = "/switch/toggle";
#endif

  String stateColor = state ? "#f44336" : "#2196F3";
  String stateText = state ? "ON" : "OFF";

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
  html += F("</div></div></a>");

#if DEVICE_TYPE == 1
  if (state && g_fanActuator != nullptr) {
    int currentSpeed = g_fanActuator->getSpeed();
    html +=
        F("<div class='status-card' style='background:#2196F320; border:2px "
          "solid #2196F3;'>");
    html += F("<div style='font-size:1.2em;font-weight:bold;'>Speed: ");
    html += String(currentSpeed);
    html += F("%</div>");
    html += F("<div class='duty-bar'><div class='duty-fill' style='width:");
    html += String(currentSpeed);
    html += F("%;'></div></div>");
    if (currentSpeed < 100) {
      html += F("<div style='font-size:0.9em;color:#555;'>Quiet mode active");
      if (config_get()->adaptiveMode)
        html += F(" + adaptive");
      html += F("</div>");
    }
    html += F("</div>");
  }
#endif
#endif

  html += F("<hr><div class='info'>");

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  html += F("Sensor polling ");
  html += String(config_get()->sensorInterval);
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

#if MQTT_ENABLED == 1
  html += F("MQTT: ");
  html += mqttManager.isConnected() ? F("connected") : F("disconnected");
  html += F("<br>");
#endif

#if WEB_SHOW_RSSI == 1
  html += F("RSSI: ");
  html += String(wifi_get_rssi());
  html += F(" dBm<br>");
#endif

  html += F("</div>");

#if DEVICE_TYPE == 1
  if (!config_get()->sensorControlMode && sensor_isOk() &&
      g_fanActuator != nullptr) {
    html += F("<div class='button-group' style='margin-top:10px;'>");
    html += F("<a href='/fan/auto'><button>Sensor control mode</button></a>");
    html += F("</div>");
  }
#endif

  return html;
}

// ========== ОБРАБОТЧИКИ ==========

#if DEVICE_TYPE == 1
void handleToggle() {
  LOG_INFO(CAT_WEB, "Toggle button pressed - toggling fan");
  if (g_fanActuator != nullptr) {
    g_fanActuator->set(!g_fanActuator->getState(), true);
  }
}

void handleSensorControlMode() {
  config_setSensorControlMode(true);
  LOG_INFO(CAT_WEB, "Sensor control mode button pressed - enabling AUTO mode");
  if (g_fanActuator != nullptr) {
    g_fanActuator->setAdaptiveMode(false);
  }
}
#endif

#if DEVICE_TYPE == 3
void handleToggle() {
  LOG_INFO(CAT_WEB, "Toggle button pressed - toggling switch");

  if (g_switchActuator != nullptr) {
    g_switchActuator->set(!g_switchActuator->getState(), true);
  }
}
#endif

void web_sendStatusPage(int refreshInterval) {
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
        "href='/config'><button>Настройки</button></a></div>"));
  send(FPSTR(HTML_PAGE_END));

  flush();

#else
  // ========== ВЕРСИЯ без буферизации для 8288==========
  auto send = [&](const String& chunk) { server.sendContent(chunk); };

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
        "href='/config'><button>Настройки</button></a></div>"));
  send(FPSTR(HTML_PAGE_END));
#endif
}

void web_sendConfigPage(const String& errorMsg, const String& successMsg) {
  const Config* cfg = config_get();
  String currentMode = apMode ? F("Access Point") : F("Client WiFi");
  String currentSsid = apMode ? String(deviceId) : String(cfg->wifiSsid);
  String currentIp = apMode ? String(AP_IP_ADDRESS) : wifi_get_local_ip();
  int refreshSeconds = (successMsg.length() > 0) ? 5 : 0;
  server.setContentLength(CONTENT_LENGTH_UNKNOWN);
  server.send(200, "text/html", "");

#if defined(ESP32)
  // ========== БУФЕРИЗИРОВАННАЯ ВЕРСИЯ для ESP32==========
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

  sendConfigPage(send, errorMsg, successMsg, *cfg, currentMode, currentSsid,
                 currentIp, refreshSeconds, apMode);
  flush();

#else
  // ========== ВЕРСИЯ без буферизации для 8266==========
  auto send = [&](const String& chunk) { server.sendContent(chunk); };

  sendConfigPage(send, errorMsg, successMsg, *cfg, currentMode, currentSsid,
                 currentIp, refreshSeconds, apMode);
#endif
}

void web_saveConfig() {
  // #if STATUS_LED_PIN > 0
  //     led_setMode(LED_MODE_OFF);
  //     delay(50);
  // #endif

  if (server.hasArg("wifiSsid")) {
    if (!config_setWifiSsid(server.arg("wifiSsid").c_str())) {
      // #if STATUS_LED_PIN > 0
      //     led_setMode(LED_MODE_MORZE_S);
      // #endif
      web_sendConfigPage(config_getLastError(), "");
      return;
    }
  }

  if (server.hasArg("wifiPassword")) {
    String pwd = server.arg("wifiPassword");
    if (pwd.length() > 0) {
      if (!config_setWifiPassword(pwd.c_str())) {
        // #if STATUS_LED_PIN > 0
        //     led_setMode(LED_MODE_MORZE_S);
        // #endif
        web_sendConfigPage(config_getLastError(), "");
        return;
      }
    }
  }

#if MQTT_ENABLED == 1
  if (server.hasArg("mqttBroker")) {
    if (!config_setMqttBroker(server.arg("mqttBroker").c_str())) {
      // #if STATUS_LED_PIN > 0
      //     led_setMode(LED_MODE_MORZE_S);
      // #endif
      web_sendConfigPage(config_getLastError(), "");
      return;
    }
  }
  if (server.hasArg("mqttPort")) {
    if (!config_setMqttPort(server.arg("mqttPort").toInt())) {
      // #if STATUS_LED_PIN > 0
      //     led_setMode(LED_MODE_MORZE_S);
      // #endif
      web_sendConfigPage(config_getLastError(), "");
      return;
    }
  }
  if (server.hasArg("mqttUser")) {
    if (!config_setMqttUser(server.arg("mqttUser").c_str())) {
      // #if STATUS_LED_PIN > 0
      //     led_setMode(LED_MODE_MORZE_S);
      // #endif
      web_sendConfigPage(config_getLastError(), "");
      return;
    }
  }
  if (server.hasArg("mqttPassword")) {
    String pwd = server.arg("mqttPassword");
    if (pwd.length() > 0) {
      if (!config_setMqttPassword(pwd.c_str())) {
        // #if STATUS_LED_PIN > 0
        //     led_setMode(LED_MODE_MORZE_S);
        // #endif
        web_sendConfigPage(config_getLastError(), "");
        return;
      }
    }
  }
  if (server.hasArg("mqttClientId")) {
    String cid = server.arg("mqttClientId");
    if (cid.length() > 0 && cid.length() < sizeof(config_get()->mqttClientId)) {
      if (!config_setMqttClientId(cid.c_str())) {
        // #if STATUS_LED_PIN > 0
        //     led_setMode(LED_MODE_MORZE_S);
        // #endif
        web_sendConfigPage(config_getLastError(), "");
        return;
      }
    }
  }
#endif

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  if (server.hasArg("sensorInterval")) {
    if (!config_setSensorInterval(server.arg("sensorInterval").toInt())) {
      // #if STATUS_LED_PIN > 0
      //     led_setMode(LED_MODE_MORZE_S);
      // #endif
      web_sendConfigPage(config_getLastError(), "");
      return;
    }
  }
#endif

#if DEVICE_TYPE == 1
  if (server.hasArg("lowTemp")) {
    if (!config_setLowTemp(server.arg("lowTemp").toFloat())) {
      // #if STATUS_LED_PIN > 0
      //     led_setMode(LED_MODE_MORZE_S);
      // #endif
      web_sendConfigPage(config_getLastError(), "");
      return;
    }
  }
  if (server.hasArg("highTemp")) {
    if (!config_setHighTemp(server.arg("highTemp").toFloat())) {
      // #if STATUS_LED_PIN > 0
      //     led_setMode(LED_MODE_MORZE_S);
      // #endif
      web_sendConfigPage(config_getLastError(), "");
      return;
    }
  }
  if (server.hasArg("lowHum")) {
    if (!config_setLowHum(server.arg("lowHum").toFloat())) {
      // #if STATUS_LED_PIN > 0
      //     led_setMode(LED_MODE_MORZE_S);
      // #endif
      web_sendConfigPage(config_getLastError(), "");
      return;
    }
  }
  if (server.hasArg("highHum")) {
    if (!config_setHighHum(server.arg("highHum").toFloat())) {
      // #if STATUS_LED_PIN > 0
      // led_setMode(LED_MODE_MORZE_S);
      // #endif
      web_sendConfigPage(config_getLastError(), "");
      return;
    }
  }
  if (server.hasArg("maxOnTime")) {
    if (!config_setMaxOnTime(server.arg("maxOnTime").toInt())) {
      // #if STATUS_LED_PIN > 0
      // led_setMode(LED_MODE_MORZE_S);
      // #endif
      web_sendConfigPage(config_getLastError(), "");
      return;
    }
  }
  if (server.hasArg("delaySeconds")) {
    if (!config_setDelaySeconds(server.arg("delaySeconds").toInt())) {
      // #if STATUS_LED_PIN > 0
      // led_setMode(LED_MODE_MORZE_S);
      // #endif
      web_sendConfigPage(config_getLastError(), "");
      return;
    }
  }
  if (server.hasArg("speedPercent")) {
    if (!config_setSpeedPercent(server.arg("speedPercent").toInt())) {
      // #if STATUS_LED_PIN > 0
      //     led_setMode(LED_MODE_MORZE_S);
      // #endif
      web_sendConfigPage(config_getLastError(), "");
      return;
    }
  }

  config_setAdaptiveMode(server.hasArg("adaptiveMode"));
  config_setBootState(server.hasArg("bootState"));
  config_setSensorControlMode(server.hasArg("sensorControlMode"));
#endif

#if DEVICE_TYPE == 3
  if (server.hasArg("maxOnTime")) {
    if (!config_setMaxOnTime(server.arg("maxOnTime").toInt())) {
      // #if STATUS_LED_PIN > 0
      //     led_setMode(LED_MODE_MORZE_S);
      // #endif
      web_sendConfigPage(config_getLastError(), "");
      return;
    }
  }
  if (server.hasArg("delaySeconds")) {
    if (!config_setDelaySeconds(server.arg("delaySeconds").toInt())) {
      // #if STATUS_LED_PIN > 0
      //     led_setMode(LED_MODE_MORZE_S);
      // #endif
      web_sendConfigPage(config_getLastError(), "");
      return;
    }
  }
  config_setBootState(server.hasArg("bootState"));
#endif

  if (!config_write()) {
    // #if STATUS_LED_PIN > 0
    // led_setMode(LED_MODE_MORZE_S);
    // #endif
    web_sendConfigPage(
        "Ошибка записи во Flash. Пожалуйста, попробуйте ещё раз.", "");
    return;
  }
  LOG_INFO(CAT_WEB, "Configuration saved successfully, restarting...");
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset='UTF-8'>
    <meta http-equiv='refresh' content='2;url=/'>
    <title>Сохранение</title>
    <style>
        body{font-family:Arial;text-align:center;margin-top:50px;background:#f0f0f0;}
        .success{color:#2e7d32;background:#e8f5e9;padding:20px;border-radius:10px;display:inline-block;}
    </style>
</head>
<body>
    <div class='success'>
        <h2>✓ Настройки сохранены</h2>
        <p>Перезагрузка устройства...</p>
    </div>
</body>
</html>
)rawliteral";

  server.send(200, "text/html", html);
  delay(2000);
  ESP.restart();
}

void web_init() {
  int refreshInterval = 5;
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  refreshInterval = config_get()->sensorInterval;
#endif

#if WEB_STATUS_ENABLED == 1
  server.on("/", [refreshInterval]() {
#ifdef ESP32
    server.client().setNoDelay(true);
#endif
    if (apMode) {
      web_sendConfigPage("", "");
      LOG_DEBUG(CAT_WEB, "GET / - show config page");
    } else {
      LOG_DEBUG(CAT_WEB, "GET / - serving status page");
      web_sendStatusPage(refreshInterval);
    }
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
  server.on("/save", []() {
    LOG_INFO(CAT_WEB, "POST /save - saving configuration");
    web_saveConfig();
  });

  server.on("/favicon.ico", []() { server.send(404); });

#if WEB_RESET_ENABLED == 1
  server.on("/resetall", []() {
    config_clear();
    server.send(
        200, "text/html",
        F("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta "
          "http-equiv='refresh' content='5;url=/'></head><body><h2>Настройки "
          "сброшены, перезагрузка...</h2></body></html>"));
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

#if OTA_ENABLED == 1
  ota_init(&server);
#endif
  server.begin();
  LOG_INFO(CAT_WEB, "Web server started");
}

void web_initAP() {
  if (apMode)
    return;

  apMode = true;
  wifi_start_ap(deviceId);

  server.on("/", []() { web_sendConfigPage("", ""); });
  server.on("/save", web_saveConfig);
  server.on("/favicon.ico", []() { server.send(404); });

#if OTA_ENABLED == 1
  if (ota_is_available()) {
    ota_init(&server);
  }
#endif

  LOG_INFO(CAT_WEB, "Web server started in AP mode: SSID %s, IP %s", deviceId,
           AP_IP_ADDRESS);

  server.begin();
}

void web_update() {
  server.handleClient();
}