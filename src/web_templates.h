#ifndef WEB_TEMPLATES_H
#define WEB_TEMPLATES_H

#include <Arduino.h>
#include "config.h"


#if WEB_ENABLED == 1

#include <functional>

/**
 * @brief Константы HTML-шаблонов
 * 
 * Хранятся в PROGMEM (Flash) для экономии RAM
 */


#if OTA_ENABLED == 1
extern bool web_isOtaAvailable();
#endif

// ========== ОБЩИЙ ШАБЛОН СТРАНИЦЫ (НАЧАЛО, БЕЗ МАРКЕРОВ) ==========
const char HTML_PAGE_START[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head><meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1'>
)rawliteral";

const char HTML_STYLE[] PROGMEM = R"rawliteral(
<style>
body{font-family:Arial;margin:20px;background:#f0f0f0;}
.container{max-width:700px;margin:auto;background:white;padding:20px;border-radius:10px;}
h1{color:#2c3e50;text-align:center;}
h3{color:#2c3e50;border-bottom:1px solid #ccc;padding-bottom:5px;}
label{display:block;margin-top:10px;font-weight:bold;}
input[type=text],input[type=password],input[type=number]{width:100%;padding:8px;margin:5px 0;border:1px solid #ccc;border-radius:4px;font-size:1.2em;box-sizing:border-box;}
input[type=checkbox]{width:20px;height:20px;margin-right:10px;vertical-align:middle;cursor:pointer;transform:scale(1.5);}
input[type=submit],button,.link-btn{color:white;background:#555;padding:10px 20px;margin-top:20px;border:none;border-radius:4px;cursor:pointer;width:100%;font-size:1em;text-align:center;text-decoration:none;display:block;box-sizing:border-box;}
input[type=submit]:hover,button:hover,.link-btn:hover{background:#333;}
.info{background:#e7f3ff;padding:10px;border-radius:5px;margin:10px 0;}
.warning{background:#fff3cd;padding:10px;border-radius:5px;margin:10px 0;color:#856404;}
.error{background:#ffebee;padding:10px;border-radius:5px;margin:10px 0;color:#c62828;border:1px solid #ef9a9a;}
.success{background:#e8f5e9;padding:10px;border-radius:5px;margin:10px 0;color:#2e7d32;border:1px solid #a5d6a7;}
.row{display:flex;gap:10px;}.row>div{flex:1;}
.password-hint{color:#7f8c8d;margin-top:-2px;margin-bottom:8px;}
.note{background:#f9f9f9;padding:8px;margin-top:10px;border-left:3px solid #2c3e50;font-size:0.9em;color:#555;}
.flex-container{display:flex;flex-wrap:wrap;justify-content:center;}
.sensor-card{display:inline-block;width:45%;margin:10px;padding:15px;border-radius:10px;text-align:center;}
.sensor-value{font-size:2em;font-weight:bold;}
.sensor-label{margin-top:5px;}
.status-card{padding:15px;border-radius:10px;text-align:center;margin:10px;}
.sensor-error{background:#ffebee;padding:15px;border-radius:8px;margin:15px 10px;color:#c62828;text-align:center;border:2px solid #ef9a9a;}
.duty-bar{background:#e0e0e0;border-radius:10px;margin:10px 0;height:20px;overflow:hidden;}
.duty-fill{background:#2c3e50;height:100%;border-radius:10px;transition:width 0.3s;}
.button-group{display:flex;justify-content:center;gap:10px;margin-top:20px;flex-wrap:wrap;}
a{text-decoration:none;}
</style>
)rawliteral";

const char HTML_PAGE_END[] PROGMEM = R"rawliteral(
</div></body></html>
)rawliteral";

using WebSendCallback = std::function<void(const String&)>;

inline void sendConfigPage(WebSendCallback send, 
                            const String& errorMsg,
                            const String& successMsg,
                            const Config& savedConfig,
                            const String& currentMode,
                            const String& currentSsid,
                            const String& currentIp,
                            int refreshSeconds = 0,
                            bool isApMode = false) {
    send(FPSTR(HTML_PAGE_START));
    
    if (refreshSeconds > 0) {
        char refresh[64];
        snprintf_P(refresh, sizeof(refresh), PSTR("<meta http-equiv='refresh' content='%d'>"), refreshSeconds);
        send(refresh);
    }
    
    #if DEBUG_ENABLED == 1
    send(F("<meta http-equiv='Cache-Control' content='no-cache, no-store, must-revalidate'>"));
    send(F("<meta http-equiv='Pragma' content='no-cache'>"));
    send(F("<meta http-equiv='Expires' content='0'>"));
    #endif
    
    send(F("<title>"));
    send(deviceId);
    send(F(" Configuration</title>"));
    
    send(FPSTR(HTML_STYLE));
    send(F("</head><body><div class='container'>"));
    
    send(F("<h1>Setting "));
    send(deviceId);
    send(F(" v. "));
    send(VERSION);
    send(F("</h1>"));
    
    send(F("<h3>State</h3><div class='info'>"));
    send(F("Mode: <strong>")); send(currentMode); send(F("</strong><br>"));
    send(F("SSID: <strong>")); send(currentSsid); send(F("</strong><br>"));
    send(F("IP: <strong>")); send(currentIp); send(F("</strong><br>"));
    send(F("</div>"));
    
    if (errorMsg.length() > 0) {
        send(F("<div class='error'><strong>Error:</strong> "));
        send(errorMsg);
        send(F("</div>"));
    }
    
    if (successMsg.length() > 0) {
        send(F("<div class='success'><strong>Config saved!</strong> "));
        send(successMsg);
        send(F("</div>"));
    }
    
    send(F("<form method='POST' action='/save'>"));
    send(F("<h3>WiFi setup</h3>"));
    send(F("<label>WiFi SSID:</label>"));
    send(F("<input type='text' name='wifiSsid' required value='"));
    send(savedConfig.wifiSsid);
    send(F("'>"));
    
    send(F("<label>WiFi Password:</label>"));
    send(F("<input type='password' name='wifiPassword' placeholder='(hidden)'>"));
    send(F("<div class='password-hint'>Leave empty to keep current password</div>"));
    
    #if MQTT_ENABLED == 1
    send(F("<h3>MQTT setup</h3>"));
    send(F("<div class='row'><div><label>MQTT Broker:</label>"));
    send(F("<input type='text' name='mqttBroker' required value='"));
    send(savedConfig.mqttBroker);
    send(F("'></div>"));
    
    send(F("<div><label>MQTT Port:</label>"));
    send(F("<input type='number' name='mqttPort' required value='"));
    send(String(savedConfig.mqttPort));
    send(F("'></div></div>"));
    
    send(F("<div class='row'><div><label>MQTT User:</label>"));
    send(F("<input type='text' name='mqttUser' value='"));
    send(savedConfig.mqttUser);
    send(F("'></div>"));
    
    send(F("<div><label>MQTT Password:</label>"));
    send(F("<input type='password' name='mqttPassword' placeholder='(hidden)'></div></div>"));
    send(F("<div class='password-hint'>Leave empty to keep current password</div>"));
    
    send(F("<label>MQTT Client ID:</label>"));
    send(F("<input type='text' name='mqttClientId' required value='"));
    send(savedConfig.mqttClientId);
    send(F("'>"));
    #endif
    
    #if DEVICE_TYPE == 1
    send(F("<h3>Sensor</h3>"));
    
    send(F("<div class='row'><div><label>Low Temp (°C):</label>"));
    send(F("<input type='number' step='0.1' min='"));
    send(String(TEMP_MIN));
    send(F("' max='"));
    send(String(TEMP_MAX));
    send(F("' name='lowTemp' required value='"));
    send(String(savedConfig.lowTemp));
    send(F("'></div>"));
    
    send(F("<div><label>High Temp (°C):</label>"));
    send(F("<input type='number' step='0.1' min='"));
    send(String(TEMP_MIN));
    send(F("' max='"));
    send(String(TEMP_MAX));
    send(F("' name='highTemp' required value='"));
    send(String(savedConfig.highTemp));
    send(F("'></div></div>"));
    
    send(F("<div class='row'><div><label>Low Hum (%):</label>"));
    send(F("<input type='number' step='0.1' min='"));
    send(String(HUM_MIN));
    send(F("' max='"));
    send(String(HUM_MAX));
    send(F("' name='lowHum' required value='"));
    send(String(savedConfig.lowHum));
    send(F("'></div>"));
    
    send(F("<div><label>High Hum (%):</label>"));
    send(F("<input type='number' step='0.1' min='"));
    send(String(HUM_MIN));
    send(F("' max='"));
    send(String(HUM_MAX));
    send(F("' name='highHum' required value='"));
    send(String(savedConfig.highHum));
    send(F("'></div></div>"));
    
    send(F("<div class='row'><div><label>Sensor polling interval (sec)</label>"));
    send(F("<input type='number' min='"));
    send(String(SENSOR_INTERVAL_MIN));
    send(F("' max='"));
    send(String(SENSOR_INTERVAL_MAX));
    send(F("' name='sensorInterval' required value='"));
    send(String(savedConfig.sensorInterval));
    send(F("'></div>"));
    
    send(F("<div><label>Emergency off after </label>"));
    send(F("<input type='number' min='"));
    send(String(MAX_ON_TIME_MIN));
    send(F("' max='"));
    send(String(MAX_ON_TIME_MAX));
    send(F("' name='maxOnTime' required value='"));
    send(String(savedConfig.maxOnTime));
    send(F("'></div></div>"));
    
    send(F("<h3>Control</h3>"));
    send(F("<label>Tun on after </label>"));
    send(F("<input type='number' min='"));
    send(String(DELAY_SECONDS_MIN));
    send(F("' max='"));
    send(String(DELAY_SECONDS_MAX));
    send(F("' name='delaySeconds' required value='"));
    send(String(savedConfig.delaySeconds));
    send(F("'> sec"));
    
    send(F("<h3>Slow mode</h3>"));
    send(F("<label>Speed (0-100%):</label>"));
    send(F("<input type='number' min='0' max='100' name='speedPercent' required value='"));
    send(String(savedConfig.speedPercent));
    send(F("'>"));
    send(F("<div class='note'>0% - off, 100% - maximal speed (slow mode off).<br>Values below 100% reduce fan noise.</div>"));
    
    send(F("<h3>Smart slow mode</h3>"));
    send(F("<label><input type='checkbox' name='adaptiveMode' value='1'"));
    if (savedConfig.adaptiveMode) send(F(" checked"));
    send(F("> Turn smart on</label>"));
    send(F("<div class='note'>Adaptive mode automatically adjusts speed to maintain temperature and humidity levels measured at fan startup.</div>"));
    
    send(F("<h3>Startup behavior</h3>"));
    send(F("<label><input type='checkbox' name='bootState' value='1'"));
    if (savedConfig.bootState) send(F(" checked"));
    send(F("> Turn on at startup</label>"));
    send(F("<div class='note'>When enabled, the switch will turn on immediately after power is applied.</div>"));
    
    send(F("<h3>Mode</h3>"));
    send(F("<label><input type='checkbox' name='sensorControlMode' value='1'"));
    if (savedConfig.sensorControlMode) send(F(" checked"));
    send(F("> Sensor control</label>"));
    send(F("<div class='note'>When enabled, the fan is controlled by temperature and humidity sensors. When disabled, only manual control works.</div>"));
    
    #elif DEVICE_TYPE == 2
    send(F("<h3>Sensor</h3>"));
    send(F("<label>Reading interval (sec)</label>"));
    send(F("<input type='number' min='"));
    send(String(SENSOR_INTERVAL_MIN));
    send(F("' max='"));
    send(String(SENSOR_INTERVAL_MAX));
    send(F("' name='sensorInterval' required value='"));
    send(String(savedConfig.sensorInterval));
    send(F("'>"));
    
    #elif DEVICE_TYPE == 3
    send(F("<h3>Control</h3>"));
    send(F("<label>Turn on after </label>"));
    send(F("<input type='number' min='"));
    send(String(DELAY_SECONDS_MIN));
    send(F("' max='"));
    send(String(DELAY_SECONDS_MAX));
    send(F("' name='delaySeconds' required value='"));
    send(String(savedConfig.delaySeconds));
    send(F("'> sec<br>"));
    
    send(F("<label>Emergency timer </label>"));
    send(F("<input type='number' min='"));
    send(String(MAX_ON_TIME_MIN));
    send(F("' max='"));
    send(String(MAX_ON_TIME_MAX));
    send(F("' name='maxOnTime' required value='"));
    send(String(savedConfig.maxOnTime));
    send(F("'> sec"));
    
    send(F("<h3>Turn on at startup</h3>"));
    send(F("<label><input type='checkbox' name='bootState' value='1'"));
    if (savedConfig.bootState) send(F(" checked"));
    send(F("> Turn on at startup</label>"));
    send(F("<div class='note'>When enabled, the switch will turn on immediately after power is applied.</div>"));
    #endif
    
    send(F("<label><input type='checkbox' name='confirmSave' required> Confirm saving</label>"));
    send(F("<input type='submit' value='Save and reboot'>"));
    send(F("</form>"));
    
    #if OTA_ENABLED == 1
    if (web_isOtaAvailable()) {
        send(F("<a href='/update' class='link-btn'>Upgrade firmware (OTA)</a>"));
    } else {
        send(F("<div class='warning'>OTA unavailable: insufficient Flash memory (2MB required)</div>"));
    }
    #endif
    
    if (!isApMode) {
        send(F("<a href='/' class='link-btn'>Home</a>"));
    }
    send(FPSTR(HTML_PAGE_END));
}
#endif // WEB_ENABLED
#endif // WEB_TEMPLATES_H