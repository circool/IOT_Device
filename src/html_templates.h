/**
 * @file html_templates.h
 * @brief HTML-шаблоны для веб-интерфейса
 * @details Содержит HTML-шаблоны в PROGMEM и функцию отправки страницы
 * конфигурации. Использует унифицированный рендеринг через web_common.
 * @date 2026-07-28
 */

#ifndef HTML_TEMPLATES_H
#define HTML_TEMPLATES_H

#include <Arduino.h>
#include "config_manager.h"
#include "web_ota_manager.h" //@deprecated see FIXME 1.10
#include "web_common.h"

#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI

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
body{font-family:Arial;margin:20px;background: #f0f0f0;}
.container{max-width:700px;margin:auto;background:white;padding:20px;border-radius:10px;}
h1{color: #2c3e50;text-align:center;}
h3{color: #2c3e50;border-bottom:1px solid #ccc;padding-bottom:5px;}
label{display:block;margin-top:10px;font-weight:bold;}
input[type=text],input[type=password],input[type=number]{width:100%;padding:8px;margin:5px 0;border:1px solid #ccc;border-radius:4px;font-size:1.2em;box-sizing:border-box;}
input[type=checkbox]{width:20px;height:20px;margin-right:10px;vertical-align:middle;cursor:pointer;transform:scale(1.5);}
input[type=submit],button,.link-btn{color:white;background: #050505;padding:10px 20px;margin-top:20px;border:none;border-radius:4px;cursor:pointer;width:100%;font-size:1em;text-align:center;text-decoration:none;display:block;box-sizing:border-box;}
input[type=submit]:hover,button:hover,.link-btn:hover{background: #030303;}


.block {
    padding: 15px;
    border-radius: 5px;
    margin: 10px 0;
}

.info {
  background: #e7f3ff;
}

.warning {
  background: #fff3cd; 
  color: #856404;
}

.error {
  background: #ffebee;
  color: #c62828;
  border:1px solid #ef9a9a;
}

.success {
  background: #e8f5e9;
  color: #2e7d32;
  border:1px solid #a5d6a7;
}

.center {
    text-align: center;
}

.text_small {
  font-size:0.9em
}

.text_header {
  font-size:1.2em;
  font-weight:bold
}
.row{display:flex;gap:10px;}.row>div{flex:1;}
.password-hint{color: #7f8c8d;margin-top:-2px;margin-bottom:8px;}
.note{
  margin-top:10px;
  font-size:0.9em;
  color: #050505;
  display:flex;
  gap:8px;
  align-items:flex-start;
}
.note::before{content:"ℹ️";font-weight:bold;flex-shrink:0;display:inline-block;}
.flex-container{display:flex;flex-wrap:wrap;justify-content:center;}

.sensor-card{display:inline-block;width:45%;margin:10px;padding:15px;border-radius:10px;text-align:center;}
.sensor-value{font-size:2em;font-weight:bold;}
.sensor-label{margin-top:5px;}

.status-card{padding:15px;border-radius:10px;text-align:center;margin:10px;}
.sensor-error{background: #ffebee;padding:15px;border-radius:8px;margin:15px 10px;color: #c62828;text-align:center;border:2px solid #ef9a9a;}
.duty-bar{background: #e0e0e0;border-radius:10px;margin:10px 0;height:20px;overflow:hidden;}
.duty-fill{background: #2c3e50;height:100%;border-radius:10px;transition:width 0.3s;}
.button-group{display:flex;justify-content:center;gap:10px;margin-top:20px;flex-wrap:wrap;}
a{text-decoration:none;}
</style>
)rawliteral";

/**
 * @brief Конец HTML-страницы
 */
const char HTML_PAGE_END[] PROGMEM = R"rawliteral(
</div></body></html>
)rawliteral";

/**
 * @brief Отправить HTML-страницу настроек
 * @param send Колбэк для отправки контента
 * @param context Контекст для колбэка
 * @param cfg Указатель на структуру ConfigData
 * @param currentMode Текущий режим (AP/STA)
 * @param currentSsid Текущий SSID
 * @param currentIp Текущий IP-адрес
 * @param refreshSeconds Интервал автообновления (0 = отключено)
 * @param isApMode true — режим точки доступа
 * @param errorMsg Текст ошибки (NULL если нет)
 * @param successMsg Текст успеха (NULL если нет)
 * @deprecated see FIXME 1.10
 */
inline void sendConfigPage(WebSendCallback send,
                           void* context,
                           const ConfigData* cfg,
                           const char* currentMode,
                           const char* currentSsid,
                           const char* currentIp,
                           int refreshSeconds,
                           bool isApMode,
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
  confirmField.note = NULL;
  confirmField.min = NULL;
  confirmField.max = NULL;
  confirmField.step = NULL;
  confirmField.link = NULL;
  confirmField.buttonText = NULL;
  confirmField.checked = false;
  confirmField.required = true;
  web_renderField(buf, sizeof(buf), &confirmField);
  send(buf, context);

  send((const char*)"<input type='submit' value='Save and reboot'>", context);
  send((const char*)"</form>", context);

  if (ota_is_available()) {
    send((const char*)"<a href='/update' class='link-btn'>Upgrade firmware (OTA)</a>", context);
  }

  if (!isApMode) {
    send((const char*)"<a href='/' class='link-btn'>Home</a>", context);
  }

  send((const char*)FPSTR(HTML_PAGE_END), context);
}

#endif  // TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI

#endif  // HTML_TEMPLATES_H