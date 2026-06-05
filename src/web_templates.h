// ===== ФАЙЛ: web_templates.h (ПОЛНОСТЬЮ, ИСПРАВЛЕННЫЙ) =====

#ifndef WEB_TEMPLATES_H
#define WEB_TEMPLATES_H

#include <Arduino.h>
#include "config.h"

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
.error{background:#ffebee;padding:10px;border-radius:5px;margin:10px 0;color:#c62828;}
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

// ========== ESP32: СТРОКОВЫЕ ФУНКЦИИ ==========
#ifdef ESP32

inline String renderStatusPage(int refreshInterval, const String& statusHtml) {
    String html;
    html += FPSTR(HTML_PAGE_START);
    
    // Meta refresh (если нужен)
    if (refreshInterval > 0) {
        char refresh[64];
        snprintf_P(refresh, sizeof(refresh), PSTR("<meta http-equiv='refresh' content='%d'>"), refreshInterval);
        html += refresh;
    }
    
    // Отключение кеша при отладке
    #if DEBUG_ENABLED == 1
    html += F("<meta http-equiv='Cache-Control' content='no-cache, no-store, must-revalidate'>");
    html += F("<meta http-equiv='Pragma' content='no-cache'>");
    html += F("<meta http-equiv='Expires' content='0'>");
    #endif
    
    html += F("<title>");
    html += deviceId;
    html += F("</title>");
    
    html += FPSTR(HTML_STYLE);
    html += F("</head><body><div class='container'>");
    
    html += F("<h1>");
    html += deviceId;
    html += F(" VERSION ");
    html += VERSION;
    html += F("</h1>");
    
    html += statusHtml;
    
    html += F("<div class='button-group'><a href='/config'><button>Настройки</button></a></div>");
    
    html += FPSTR(HTML_PAGE_END);
    return html;
}

inline String renderConfigPage(const String& errorMsg,
                                const Config& savedConfig,
                                const String& currentMode,
                                const String& currentSsid,
                                const String& currentIp) {
    String html;
    html += FPSTR(HTML_PAGE_START);
    
    // Отключение кеша при отладке
    #if DEBUG_ENABLED == 1
    html += F("<meta http-equiv='Cache-Control' content='no-cache, no-store, must-revalidate'>");
    html += F("<meta http-equiv='Pragma' content='no-cache'>");
    html += F("<meta http-equiv='Expires' content='0'>");
    #endif
    
    html += F("<title>");
    html += deviceId;
    html += F(" Configuration</title>");
    
    html += FPSTR(HTML_STYLE);
    html += F("</head><body><div class='container'>");
    
    html += F("<h1>Настройка устройства ");
    html += deviceId;
    html += F(" v. ");
    html += VERSION;
    html += F("</h1>");
    
    html += F("<h3>Текущее состояние</h3><div class='info'>");
    html += F("Режим: <strong>"); html += currentMode; html += F("</strong><br>");
    html += F("SSID: <strong>"); html += currentSsid; html += F("</strong><br>");
    html += F("IP адрес: <strong>"); html += currentIp; html += F("</strong><br>");
    html += F("</div>");
    
    if (errorMsg.length() > 0) {
        html += F("<div class='error'><strong>Ошибка:</strong> ");
        html += errorMsg;
        html += F("</div>");
    }
    
    html += F("<form method='POST' action='/save'>");
    html += F("<h3>Настройки сети</h3>");
    html += F("<label>WiFi SSID:</label>");
    html += F("<input type='text' name='wifiSsid' required value='");
    html += savedConfig.wifiSsid;
    html += F("'>");
    
    html += F("<label>WiFi Password:</label>");
    html += F("<input type='password' name='wifiPassword' placeholder='(не показан)'>");
    html += F("<div class='password-hint'>Оставьте пустым, чтобы сохранить текущий пароль</div>");
    
    #if MQTT_ENABLED == 1
    html += F("<h3>MQTT настройки</h3>");
    html += F("<div class='row'><div><label>MQTT Broker:</label>");
    html += F("<input type='text' name='mqttBroker' required value='");
    html += savedConfig.mqttBroker;
    html += F("'></div>");
    
    html += F("<div><label>MQTT Port:</label>");
    html += F("<input type='number' name='mqttPort' required value='");
    html += String(savedConfig.mqttPort);
    html += F("'></div></div>");
    
    html += F("<div class='row'><div><label>MQTT User:</label>");
    html += F("<input type='text' name='mqttUser' value='");
    html += savedConfig.mqttUser;
    html += F("'></div>");
    
    html += F("<div><label>MQTT Password:</label>");
    html += F("<input type='password' name='mqttPassword' placeholder='(не показан)'></div></div>");
    html += F("<div class='password-hint'>Оставьте пустым, чтобы сохранить текущий пароль</div>");
    
    html += F("<label>MQTT Client ID:</label>");
    html += F("<input type='text' name='mqttClientId' required value='");
    html += savedConfig.mqttClientId;
    html += F("'>");
    #endif
    
    #if DEVICE_TYPE == 1
    html += F("<h3>Настройки датчиков</h3>");
    
    html += F("<div class='row'><div><label>Low Temp (°C):</label>");
    html += F("<input type='number' step='0.1' name='lowTemp' required value='");
    html += String(savedConfig.lowTemp);
    html += F("'></div>");
    
    html += F("<div><label>High Temp (°C):</label>");
    html += F("<input type='number' step='0.1' name='highTemp' required value='");
    html += String(savedConfig.highTemp);
    html += F("'></div></div>");
    
    html += F("<div class='row'><div><label>Low Hum (%):</label>");
    html += F("<input type='number' step='0.1' name='lowHum' required value='");
    html += String(savedConfig.lowHum);
    html += F("'></div>");
    
    html += F("<div><label>High Hum (%):</label>");
    html += F("<input type='number' step='0.1' name='highHum' required value='");
    html += String(savedConfig.highHum);
    html += F("'></div></div>");
    
    html += F("<div class='row'><div><label>Интервал опроса датчика (сек)</label>");
    html += F("<input type='number' name='sensorInterval' required value='");
    html += String(savedConfig.sensorInterval);
    html += F("'></div>");
    
    html += F("<div><label>Аварийное отключение через </label>");
    html += F("<input type='number' name='maxOnTime' min='0' required value='");
    html += String(savedConfig.maxOnTime);
    html += F("'></div></div>");
    
    html += F("<h3>Управление</h3>");
    html += F("<label>Принудительно включить через </label>");
    html += F("<input type='number' name='delaySeconds' required value='");
    html += String(savedConfig.delaySeconds);
    html += F("'> сек");
    
    html += F("<h3>Тихий режим (ШИМ)</h3>");
    html += F("<label>Скорость (0-100%):</label>");
    html += F("<input type='number' name='speedPercent' min='0' max='100' required value='");
    html += String(savedConfig.speedPercent);
    html += F("'>");
    html += F("<div class='note'>0% - выключено, 100% - полная мощность (тихий режим выключен).<br>При значении ниже 100% вентилятор работает тише.</div>");
    
    html += F("<h3>Адаптивный тихий режим</h3>");
    html += F("<label><input type='checkbox' name='adaptiveMode' value='1'");
    if (savedConfig.adaptiveMode) html += F(" checked");
    html += F("> Включить адаптацию</label>");
    html += F("<div class='note'>Адаптивный режим автоматически регулирует скорость для поддержания температуры и влажности на уровне, зафиксированном при включении вентилятора.</div>");
    
    html += F("<h3>Поведение при старте</h3>");
    html += F("<label><input type='checkbox' name='bootState' value='1'");
    if (savedConfig.bootState) html += F(" checked");
    html += F("> Включать при старте</label>");
    html += F("<div class='note'>При включенной опции вентилятор будет включен сразу после подачи питания.</div>");
    
    html += F("<h3>Режимы работы</h3>");
    html += F("<label><input type='checkbox' name='sensorControlMode' value='1'");
    if (savedConfig.sensorControlMode) html += F(" checked");
    html += F("> Режим управления сенсором</label>");
    html += F("<div class='note'>При включённом режиме вентилятор управляется по показаниям датчиков температуры и влажности. При выключении — только вручную.</div>");
    
    #elif DEVICE_TYPE == 2
    html += F("<h3>Настройки датчиков</h3>");
    html += F("<label>Интервал опроса датчика (сек)</label>");
    html += F("<input type='number' name='sensorInterval' required value='");
    html += String(savedConfig.sensorInterval);
    html += F("'>");
    
    #elif DEVICE_TYPE == 3
    html += F("<h3>Управление</h3>");
    html += F("<label>Принудительно включить через </label>");
    html += F("<input type='number' name='delaySeconds' required value='");
    html += String(savedConfig.delaySeconds);
    html += F("'> сек<br>");
    
    html += F("<label>Аварийное отключение через </label>");
    html += F("<input type='number' name='maxOnTime' min='0' required value='");
    html += String(savedConfig.maxOnTime);
    html += F("'> сек");
    
    html += F("<h3>Поведение при старте</h3>");
    html += F("<label><input type='checkbox' name='bootState' value='1'");
    if (savedConfig.bootState) html += F(" checked");
    html += F("> Включать при старте</label>");
    html += F("<div class='note'>При включенной опции выключатель будет включен сразу после подачи питания.</div>");
    #endif
    
    html += F("<label><input type='checkbox' name='confirmSave' required> Подтвердить сохранение</label>");
    html += F("<input type='submit' value='Сохранить и перезагрузить'>");
    html += F("</form>");
    
    #if OTA_ENABLED == 1
    if (web_isOtaAvailable()) {
        html += F("<a href='/update' class='link-btn'>Обновить прошивку (OTA)</a>");
    } else {
        html += F("<div class='warning'>OTA недоступно: недостаточно Flash памяти (требуется 2MB)</div>");
    }
    #endif
    
    html += F("<a href='/' class='link-btn'>Домой</a>");
    html += FPSTR(HTML_PAGE_END);
    
    return html;
}

#endif // ESP32

// ========== ESP8266: ПОТОКОВЫЕ ФУНКЦИИ ==========
#ifdef ESP8266

using WebSendCallback = std::function<void(const String&)>;

inline void sendStatusPage(WebSendCallback send, int refreshInterval, const String& statusHtml) {
    send(FPSTR(HTML_PAGE_START));
    
    if (refreshInterval > 0) {
        char refresh[64];
        snprintf_P(refresh, sizeof(refresh), PSTR("<meta http-equiv='refresh' content='%d'>"), refreshInterval);
        send(refresh);
    }
    
    #if DEBUG_ENABLED == 1
    send(F("<meta http-equiv='Cache-Control' content='no-cache, no-store, must-revalidate'>"));
    send(F("<meta http-equiv='Pragma' content='no-cache'>"));
    send(F("<meta http-equiv='Expires' content='0'>"));
    #endif
    
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
    
    send(F("<div class='button-group'><a href='/config'><button>Настройки</button></a></div>"));
    
    send(FPSTR(HTML_PAGE_END));
}

inline void sendConfigPage(WebSendCallback send, const String& errorMsg,
                            const Config& savedConfig,
                            const String& currentMode,
                            const String& currentSsid,
                            const String& currentIp) {
    send(FPSTR(HTML_PAGE_START));
    
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
    
    send(F("<h1>Настройка устройства "));
    send(deviceId);
    send(F(" v. "));
    send(VERSION);
    send(F("</h1>"));
    
    send(F("<h3>Текущее состояние</h3><div class='info'>"));
    send(F("Режим: <strong>")); send(currentMode); send(F("</strong><br>"));
    send(F("SSID: <strong>")); send(currentSsid); send(F("</strong><br>"));
    send(F("IP адрес: <strong>")); send(currentIp); send(F("</strong><br>"));
    send(F("</div>"));
    
    if (errorMsg.length() > 0) {
        send(F("<div class='error'><strong>Ошибка:</strong> "));
        send(errorMsg);
        send(F("</div>"));
    }
    
    send(F("<form method='POST' action='/save'>"));
    send(F("<h3>Настройки сети</h3>"));
    send(F("<label>WiFi SSID:</label>"));
    send(F("<input type='text' name='wifiSsid' required value='"));
    send(savedConfig.wifiSsid);
    send(F("'>"));
    
    send(F("<label>WiFi Password:</label>"));
    send(F("<input type='password' name='wifiPassword' placeholder='(не показан)'>"));
    send(F("<div class='password-hint'>Оставьте пустым, чтобы сохранить текущий пароль</div>"));
    
    #if MQTT_ENABLED == 1
    send(F("<h3>MQTT настройки</h3>"));
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
    send(F("<input type='password' name='mqttPassword' placeholder='(не показан)'></div></div>"));
    send(F("<div class='password-hint'>Оставьте пустым, чтобы сохранить текущий пароль</div>"));
    
    send(F("<label>MQTT Client ID:</label>"));
    send(F("<input type='text' name='mqttClientId' required value='"));
    send(savedConfig.mqttClientId);
    send(F("'>"));
    #endif
    
    #if DEVICE_TYPE == 1
    send(F("<h3>Настройки датчиков</h3>"));
    
    send(F("<div class='row'><div><label>Low Temp (°C):</label>"));
    send(F("<input type='number' step='0.1' name='lowTemp' required value='"));
    send(String(savedConfig.lowTemp));
    send(F("'></div>"));
    
    send(F("<div><label>High Temp (°C):</label>"));
    send(F("<input type='number' step='0.1' name='highTemp' required value='"));
    send(String(savedConfig.highTemp));
    send(F("'></div></div>"));
    
    send(F("<div class='row'><div><label>Low Hum (%):</label>"));
    send(F("<input type='number' step='0.1' name='lowHum' required value='"));
    send(String(savedConfig.lowHum));
    send(F("'></div>"));
    
    send(F("<div><label>High Hum (%):</label>"));
    send(F("<input type='number' step='0.1' name='highHum' required value='"));
    send(String(savedConfig.highHum));
    send(F("'></div></div>"));
    
    send(F("<div class='row'><div><label>Интервал опроса датчика (сек)</label>"));
    send(F("<input type='number' name='sensorInterval' required value='"));
    send(String(savedConfig.sensorInterval));
    send(F("'></div>"));
    
    send(F("<div><label>Аварийное отключение через </label>"));
    send(F("<input type='number' name='maxOnTime' min='0' required value='"));
    send(String(savedConfig.maxOnTime));
    send(F("'></div></div>"));
    
    send(F("<h3>Управление</h3>"));
    send(F("<label>Принудительно включить через </label>"));
    send(F("<input type='number' name='delaySeconds' required value='"));
    send(String(savedConfig.delaySeconds));
    send(F("'> сек"));
    
    send(F("<h3>Тихий режим (ШИМ)</h3>"));
    send(F("<label>Скорость (0-100%):</label>"));
    send(F("<input type='number' name='speedPercent' min='0' max='100' required value='"));
    send(String(savedConfig.speedPercent));
    send(F("'>"));
    send(F("<div class='note'>0% - выключено, 100% - полная мощность (тихий режим выключен).<br>При значении ниже 100% вентилятор работает тише.</div>"));
    
    send(F("<h3>Адаптивный тихий режим</h3>"));
    send(F("<label><input type='checkbox' name='adaptiveMode' value='1'"));
    if (savedConfig.adaptiveMode) send(F(" checked"));
    send(F("> Включить адаптацию</label>"));
    send(F("<div class='note'>Адаптивный режим автоматически регулирует скорость для поддержания температуры и влажности на уровне, зафиксированном при включении вентилятора.</div>"));
    
    send(F("<h3>Поведение при старте</h3>"));
    send(F("<label><input type='checkbox' name='bootState' value='1'"));
    if (savedConfig.bootState) send(F(" checked"));
    send(F("> Включать при старте</label>"));
    send(F("<div class='note'>При включенной опции вентилятор будет включен сразу после подачи питания.</div>"));
    
    send(F("<h3>Режимы работы</h3>"));
    send(F("<label><input type='checkbox' name='sensorControlMode' value='1'"));
    if (savedConfig.sensorControlMode) send(F(" checked"));
    send(F("> Режим управления сенсором</label>"));
    send(F("<div class='note'>При включённом режиме вентилятор управляется по показаниям датчиков температуры и влажности. При выключении — только вручную.</div>"));
    
    #elif DEVICE_TYPE == 2
    send(F("<h3>Настройки датчиков</h3>"));
    send(F("<label>Интервал опроса датчика (сек)</label>"));
    send(F("<input type='number' name='sensorInterval' required value='"));
    send(String(savedConfig.sensorInterval));
    send(F("'>"));
    
    #elif DEVICE_TYPE == 3
    send(F("<h3>Управление</h3>"));
    send(F("<label>Принудительно включить через </label>"));
    send(F("<input type='number' name='delaySeconds' required value='"));
    send(String(savedConfig.delaySeconds));
    send(F("'> сек<br>"));
    
    send(F("<label>Аварийное отключение через </label>"));
    send(F("<input type='number' name='maxOnTime' min='0' required value='"));
    send(String(savedConfig.maxOnTime));
    send(F("'> сек"));
    
    send(F("<h3>Поведение при старте</h3>"));
    send(F("<label><input type='checkbox' name='bootState' value='1'"));
    if (savedConfig.bootState) send(F(" checked"));
    send(F("> Включать при старте</label>"));
    send(F("<div class='note'>При включенной опции выключатель будет включен сразу после подачи питания.</div>"));
    #endif
    
    send(F("<label><input type='checkbox' name='confirmSave' required> Подтвердить сохранение</label>"));
    send(F("<input type='submit' value='Сохранить и перезагрузить'>"));
    send(F("</form>"));
    
    #if OTA_ENABLED == 1
    if (web_isOtaAvailable()) {
        send(F("<a href='/update' class='link-btn'>Обновить прошивку (OTA)</a>"));
    } else {
        send(F("<div class='warning'>OTA недоступно: недостаточно Flash памяти (требуется 2MB)</div>"));
    }
    #endif
    
    send(F("<a href='/' class='link-btn'>Домой</a>"));
    send(FPSTR(HTML_PAGE_END));
}

#endif // ESP8266

#endif // WEB_TEMPLATES_H