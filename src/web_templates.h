/**
 * @file web_templates.h
 * @brief HTML-шаблоны для веб-интерфейса
 * @details Хранит HTML-страницы в PROGMEM для экономии RAM.
 *          Использует callback-функцию для отправки контента.
 * @note Для embedded: шаблоны хранятся во Flash (PROGMEM)
 * @note Колбэк — указатель на функцию вместо std::function (экономия RAM)
 */

#ifndef WEB_TEMPLATES_H
#define WEB_TEMPLATES_H

#include <Arduino.h>
#include "config_manager.h"
#include "ota.h"
#include "sensor.h"

#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI

// ============================================================================
// HTML-ШАБЛОНЫ (хранятся в PROGMEM)
// ============================================================================

/**
 * @brief Начало HTML-страницы
 */
const char HTML_PAGE_START[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head><meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1'>
)rawliteral";

/**
 * @brief CSS-стили для веб-интерфейса
 */
const char HTML_STYLE[] PROGMEM = R"rawliteral(
<style>
body {
  font-family:Arial;
  margin:20px;
  background:#f0f0f0;
}
.container {	
  max-width:700px;
  margin:auto;
  background:white;
  padding:20px;
  border-radius:10px;
}
h1 {
	color:#2c3e50;
  text-align:center;
}
h3 {
	color:#2c3e50;
  border-bottom:1px solid #ccc;
  padding-bottom:5px;
}
label {
	display:block;
  margin-top:10px;
  font-weight:bold;
}
input[type=text],input[type=password],input[type=number] {
	width:100%;
  padding:8px;
  margin:5px 0;
  border:1px solid #ccc;
  border-radius:4px;
  font-size:1.2em;
  box-sizing:border-box;
}
input[type=checkbox] {
	width:20px;
  height:20px;
  margin-right:10px;
  vertical-align:middle;
  cursor:pointer;
  transform:scale(1.5);
}
input[type=submit],button,.link-btn {
	color:white;
  background:#555;
  padding:10px 20px;
  margin-top:20px;
  border:none;
  border-radius:4px;
  cursor:pointer;
  width:100%;
  font-size:1em;
  text-align:center;
  text-decoration:none;
  display:block;
  box-sizing:border-box;
}
input[type=submit]:hover,button:hover,.link-btn:hover {
	background:#333;
}
.info {
	background:#e7f3ff;
  padding:10px;
  border-radius:5px;
  margin:10px 0;
}
.warning {
	background:#fff3cd;
  padding:10px;
  border-radius:5px;
  margin:10px 0;
  color:#856404;
}
.error {	
  background:#ffebee;
  padding:10px;
  border-radius:5px;
  margin:10px 0;
  color:#c62828;
  border:1px solid #ef9a9a;
}
.success {	
  background:#e8f5e9;
  padding:10px;
  border-radius:5px;
  margin:10px 0;
  color:#2e7d32;
  border:1px solid #a5d6a7;
}
.row {
  display:flex;
  gap:10px;
}
.row>div {	
  flex:1;
}
.password-hint {
  color:#7f8c8d;
  margin-top:-2px;
  margin-bottom:8px;
}
.note {
  margin-top: 10px;
  font-size: 0.9em;
  color: #555;
  display: flex;
  gap: 8px;
  align-items: flex-start;
}
.note::before {	
  content: "ℹ️";
  font-weight: bold;
  flex-shrink: 0;
  display: inline-block;
}
.flex-container {	
  display:flex; 
  flex-wrap:wrap; justify-content:center;
}
.sensor-card {
	display:inline-block;
  width:45%;
  margin:10px;
  padding:15px;
  border-radius:10px;
  text-align:center;
}
.sensor-value {
	font-size:2em;
  font-weight:bold;
}
.sensor-label {
	margin-top:5px;
}
.status-card {
	padding:15px;
  border-radius:10px;
  text-align:center;
  margin:10px;
}
.sensor-error {
	background:#ffebee;
  padding:15px;
  border-radius:8px;
  margin:15px 10px;
  color:#c62828;
  text-align:center;
  border:2px solid #ef9a9a;
}
.duty-bar {
	background:#e0e0e0;
  border-radius:10px;
  margin:10px 0;
  height:20px;
  overflow:hidden;
}
.duty-fill {
	background:#2c3e50;
  height:100%;
  border-radius:10px;
  transition:width 0.3s;
}
.button-group {
	display:flex;
  justify-content:center;
  gap:10px;
  margin-top:20px;
  flex-wrap:wrap;
}
a {
	text-decoration:none;
}
</style>
)rawliteral";

/**
 * @brief Конец HTML-страницы
 */
const char HTML_PAGE_END[] PROGMEM = R"rawliteral(
</div></body></html>
)rawliteral";

// ============================================================================
// ТИП КОЛБЭКА ДЛЯ ОТПРАВКИ HTML
// ============================================================================

/**
 * @brief Колбэк для отправки HTML-контента
 * @param chunk Строка для отправки
 * @param context Пользовательский контекст (обычно указатель на WebServer)
 */
typedef void (*WebSendCallback)(const String& chunk, void* context);

// ============================================================================
// ФУНКЦИЯ ГЕНЕРАЦИИ СТРАНИЦЫ НАСТРОЕК
// ============================================================================

/**
 * @brief Отправить HTML-страницу настроек
 * @param send Колбэк для отправки контента
 * @param context Контекст для колбэка
 * @param errorMsg Сообщение об ошибке (если есть)
 * @param successMsg Сообщение об успехе (если есть)
 * @param savedConfig Текущая конфигурация
 * @param currentMode Текущий режим (AP/STA)
 * @param currentSsid Текущий SSID
 * @param currentIp Текущий IP-адрес
 * @param refreshSeconds Интервал автообновления (0 = отключено)
 * @param isApMode true — режим точки доступа
 */
inline void sendConfigPage(WebSendCallback send,
                           void* context,
                           const String& errorMsg,
                           const String& successMsg,
                           const ConfigData& savedConfig,
                           const String& currentMode,
                           const String& currentSsid,
                           const String& currentIp,
                           int refreshSeconds = 0,
                           bool isApMode = false) {
  // Отправляем начало страницы
  send(FPSTR(HTML_PAGE_START), context);

  // Мета-тег для автообновления
  if (refreshSeconds > 0) {
    char refresh[64];
    snprintf_P(refresh, sizeof(refresh),
               PSTR("<meta http-equiv='refresh' content='%d'>"),
               refreshSeconds);
    send(refresh, context);
  }

#if XLOG_LEVEL > 3
  send(F("<meta http-equiv='Cache-Control' content='no-cache, no-store, "
         "must-revalidate'>"),
       context);
  send(F("<meta http-equiv='Pragma' content='no-cache'>"), context);
  send(F("<meta http-equiv='Expires' content='0'>"), context);
#endif

  // Заголовок
  send(F("<title>"), context);
  send(g_configManager.getDeviceId(), context);
  send(F(" Configuration</title>"), context);

  send(FPSTR(HTML_STYLE), context);
  send(F("</head><body><div class='container'>"), context);

  send(F("<h1>Settings "), context);
  send(g_configManager.getDeviceId(), context);
  send(F(" v. "), context);
  send(VERSION, context);
  send(F("</h1>"), context);

  // Информация о состоянии
  send(F("<h3>State</h3><div class='info'>"), context);
  send(F("Mode: <strong>"), context);
  send(currentMode, context);
  send(F("</strong><br>"), context);
  send(F("SSID: <strong>"), context);
  send(currentSsid, context);
  send(F("</strong><br>"), context);
  send(F("IP: <strong>"), context);
  send(currentIp, context);
  send(F("</strong><br>"), context);
  send(F("</div>"), context);

  // Сообщения об ошибках/успехе
  if (errorMsg.length() > 0) {
    send(F("<div class='error'><strong>Error:</strong> "), context);
    send(errorMsg, context);
    send(F("</div>"), context);
  }

  if (successMsg.length() > 0) {
    send(F("<div class='success'><strong>Config saved!</strong> "), context);
    send(successMsg, context);
    send(F("</div>"), context);
  }

  // Форма настроек
  send(F("<form method='POST' action='/save'>"), context);
  send(F("<h3>WiFi setup</h3>"), context);
  send(F("<label>WiFi SSID:</label>"), context);
  send(F("<input type='text' name='wifiSsid' required value='"), context);
  send(savedConfig.wifiSsid, context);
  send(F("'>"), context);

  send(F("<label>WiFi Password:</label>"), context);
  send(F("<input type='password' name='wifiPassword' placeholder='(hidden)'>"),
       context);
  send(F("<div class='password-hint'>Leave empty to keep current "
         "password</div>"),
       context);

#if FEATURE_MQTT_ENABLED == 1
  send(F("<h3>MQTT setup</h3>"), context);
  send(F("<div class='row'><div><label>MQTT Broker:</label>"), context);
  send(F("<input type='text' name='mqttBroker' required value='"), context);
  send(savedConfig.mqttBroker, context);
  send(F("'></div>"), context);

  send(F("<div><label>MQTT Port:</label>"), context);
  send(F("<input type='number' name='mqttPort' required value='"), context);
  send(String(savedConfig.mqttPort), context);
  send(F("'></div></div>"), context);

  send(F("<div class='row'><div><label>MQTT User:</label>"), context);
  send(F("<input type='text' name='mqttUser' value='"), context);
  send(savedConfig.mqttUser, context);
  send(F("'></div>"), context);

  send(F("<div><label>MQTT Password:</label>"), context);
  send(F("<input type='password' name='mqttPassword' "
         "placeholder='(hidden)'></div></div>"),
       context);
  send(F("<div class='password-hint'>Leave empty to keep current "
         "password</div>"),
       context);

  send(F("<label>MQTT Client ID:</label>"), context);
  send(F("<input type='text' name='mqttClientId' required value='"), context);
  send(savedConfig.mqttClientId, context);
  send(F("'>"), context);
#endif

#if DEVICE_TYPE == 1
  send(F("<h3>Sensor</h3>"), context);

  send(F("<div class='row'><div><label>Low Temp (°C):</label>"), context);
  send(F("<input type='number' step='0.1' min='"), context);
  send(String(TEMP_MIN), context);
  send(F("' max='"), context);
  send(String(TEMP_MAX), context);
  send(F("' name='lowTemp' required value='"), context);
  send(String(savedConfig.lowTemp), context);
  send(F("'></div>"), context);

  send(F("<div><label>High Temp (°C):</label>"), context);
  send(F("<input type='number' step='0.1' min='"), context);
  send(String(TEMP_MIN), context);
  send(F("' max='"), context);
  send(String(TEMP_MAX), context);
  send(F("' name='highTemp' required value='"), context);
  send(String(savedConfig.highTemp), context);
  send(F("'></div></div>"), context);

  send(F("<div class='row'><div><label>Low Hum (%):</label>"), context);
  send(F("<input type='number' step='0.1' min='"), context);
  send(String(HUM_MIN), context);
  send(F("' max='"), context);
  send(String(HUM_MAX), context);
  send(F("' name='lowHum' required value='"), context);
  send(String(savedConfig.lowHum), context);
  send(F("'></div>"), context);

  send(F("<div><label>High Hum (%):</label>"), context);
  send(F("<input type='number' step='0.1' min='"), context);
  send(String(HUM_MIN), context);
  send(F("' max='"), context);
  send(String(HUM_MAX), context);
  send(F("' name='highHum' required value='"), context);
  send(String(savedConfig.highHum), context);
  send(F("'></div></div>"), context);

  send(F("<div class='row'><div><label>Sensor polling interval (sec)</label>"),
       context);
  send(F("<input type='number' min='"), context);
  send(String(SENSOR_INTERVAL_MIN), context);
  send(F("' max='"), context);
  send(String(SENSOR_INTERVAL_MAX), context);
  send(F("' name='sensorInterval' required value='"), context);
  send(String(savedConfig.sensorInterval), context);
  send(F("'></div>"), context);

  send(F("<div><label>Emergency timeout after </label>"), context);
  send(F("<input type='number' min='"), context);
  send(String(MAX_ON_TIME_MIN), context);
  send(F("' max='"), context);
  send(String(MAX_ON_TIME_MAX), context);
  send(F("' name='maxOnTime' required value='"), context);
  send(String(savedConfig.maxOnTime), context);
  send(F("'></div></div>"), context);

  send(F("<h3>Control</h3>"), context);
  send(F("<label>Turn on after </label>"), context);
  send(F("<input type='number' min='"), context);
  send(String(DELAY_SECONDS_MIN), context);
  send(F("' max='"), context);
  send(String(DELAY_SECONDS_MAX), context);
  send(F("' name='delaySeconds' required value='"), context);
  send(String(savedConfig.delaySeconds), context);
  send(F("'> sec"), context);

  send(F("<h3>Slow mode</h3>"), context);
  send(F("<label>Speed (0-100%):</label>"), context);
  send(F("<input type='number' min='0' max='100' name='speedPercent' required "
         "value='"),
       context);
  send(String(savedConfig.speedPercent), context);
  send(F("'>"), context);
  send(F("<div class='note'>0% - off, 100% - maximal speed (slow mode "
         "off).<br>Values below 100% reduce fan noise.</div>"),
       context);

  send(F("<h3>Adaptive quiet mode</h3>"), context);
  send(F("<label><input type='checkbox' name='adaptiveMode' value='1'"),
       context);
  if (savedConfig.adaptiveMode)
    send(F(" checked"), context);
  send(F("> Enable adaptive mode</label>"), context);
  send(F("<div class='note'>Adaptive mode automatically adjusts speed to "
         "maintain temperature and humidity levels measured at fan "
         "startup.</div>"),
       context);

  send(F("<h3>Startup behavior</h3>"), context);
  send(F("<label><input type='checkbox' name='bootState' value='1'"), context);
  if (savedConfig.bootState)
    send(F(" checked"), context);
  send(F("> Turn on at startup</label>"), context);
  send(F("<div class='note'>When enabled, the fan will turn on immediately "
         "after power is applied.</div>"),
       context);

  send(F("<h3>Mode</h3>"), context);
  send(F("<label><input type='checkbox' name='sensorControlMode' value='1'"),
       context);
  if (savedConfig.sensorControlMode)
    send(F(" checked"), context);
  send(F("> Sensor control</label>"), context);
  send(F("<div class='note'>When enabled, the fan is controlled by temperature "
         "and humidity sensors. When disabled, only manual control "
         "works.</div>"),
       context);

#elif DEVICE_TYPE == 2
  send(F("<h3>Sensor</h3>"), context);
  send(F("<label>Reading interval (sec)</label>"), context);
  send(F("<input type='number' min='"), context);
  send(String(SENSOR_INTERVAL_MIN), context);
  send(F("' max='"), context);
  send(String(SENSOR_INTERVAL_MAX), context);
  send(F("' name='sensorInterval' required value='"), context);
  send(String(savedConfig.sensorInterval), context);
  send(F("'>"), context);

#elif DEVICE_TYPE == 3
  send(F("<h3>Control</h3>"), context);
  send(F("<label>Turn on after </label>"), context);
  send(F("<input type='number' min='"), context);
  send(String(DELAY_SECONDS_MIN), context);
  send(F("' max='"), context);
  send(String(DELAY_SECONDS_MAX), context);
  send(F("' name='delaySeconds' required value='"), context);
  send(String(savedConfig.delaySeconds), context);
  send(F("'> sec<br>"), context);

  send(F("<label>Emergency timer </label>"), context);
  send(F("<input type='number' min='"), context);
  send(String(MAX_ON_TIME_MIN), context);
  send(F("' max='"), context);
  send(String(MAX_ON_TIME_MAX), context);
  send(F("' name='maxOnTime' required value='"), context);
  send(String(savedConfig.maxOnTime), context);
  send(F("'> sec"), context);

  send(F("<h3>Turn on at startup</h3>"), context);
  send(F("<label><input type='checkbox' name='bootState' value='1'"), context);
  if (savedConfig.bootState)
    send(F(" checked"), context);
  send(F("> Turn on at startup</label>"), context);
  send(F("<div class='note'>When enabled, the switch will turn on immediately "
         "after power is applied.</div>"),
       context);
#endif

  send(F("<label><input type='checkbox' name='confirmSave' required> Confirm "
         "saving</label>"),
       context);
  send(F("<input type='submit' value='Save and reboot'>"), context);
  send(F("</form>"), context);

#if FEATURE_OTA_ENABLED == 1
  send(ota_getButtonHtml(), context);
#endif

  if (!isApMode) {
    send(F("<a href='/' class='link-btn'>Home</a>"), context);
  }

  send(FPSTR(HTML_PAGE_END), context);
}

#endif  // TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI

#endif  // WEB_TEMPLATES_H