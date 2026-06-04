#ifndef WEB_TEMPLATES_H
#define WEB_TEMPLATES_H

#include <Arduino.h>
#include "config.h"

#if OTA_ENABLED == 1
extern bool web_isOtaAvailable();
#endif

// Для currentTemp, currentHum, sensorOk, sensorError
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
#include "sensor.h"
#endif

// Для fan_getState, startingPulseActive
#if DEVICE_TYPE == 1
#include "fan.h"
#endif

// Для switch_getState
#if DEVICE_TYPE == 3
#include "switch.h"
#endif

// Для mqttManager
#if MQTT_ENABLED == 1
#include "mqtt.h"
#endif

// Для web_isOtaAvailable
#if OTA_ENABLED == 1
#include "web.h"
#endif

// ========== ОБЩИЙ ШАБЛОН СТРАНИЦЫ ==========
const char HTML_PAGE_START[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head><meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1'>
{META_REFRESH}
<title>{TITLE}</title>
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
</head>
<body><div class='container'>
)rawliteral";

const char HTML_PAGE_END[] PROGMEM = R"rawliteral(
</div></body></html>
)rawliteral";

// ========== ФУНКЦИИ ГЕНЕРАЦИИ СТРАНИЦ ==========

inline String renderStatusPage(int refreshInterval) {
    String html = FPSTR(HTML_PAGE_START);
    
    // Мета-тег refresh
    if (refreshInterval > 0) {
        char refresh[64];
        snprintf(refresh, sizeof(refresh), "<meta http-equiv='refresh' content='%d'>", refreshInterval);
        html.replace("{META_REFRESH}", refresh);
    } else {
        html.replace("{META_REFRESH}", "");
    }
    
    // Заголовок
    char title[64];
    snprintf(title, sizeof(title), "<title>%s</title>", DEVICE_PREFIX);
    html.replace("{TITLE}", title);
    
    // Заголовок страницы
    char header[128];
    snprintf(header, sizeof(header), "<h1>%s VERSION %s</h1>", DEVICE_PREFIX, VERSION);
    html += header;
    
    // Блок датчиков (TYPE 1 или 2)
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    html += "<div class='flex-container'>";
    
    #if DEVICE_TYPE == 1
    const char* tempColor = (currentTemp >= config.highTemp) ? "#f44336" : 
                           ((currentTemp <= config.lowTemp) ? "#4CAF50" : "#2196F3");
    const char* humColor = (currentHum >= config.highHum) ? "#f44336" : 
                          ((currentHum <= config.lowHum) ? "#4CAF50" : "#2196F3");
    #else
    const char* tempColor = "#2196F3";
    const char* humColor = "#2196F3";
    #endif
    
    char buffer[256];
    
    // Температура
    snprintf(buffer, sizeof(buffer), 
        "<div class='sensor-card' style='background:%s20; border:2px solid %s;'>"
        "<div class='sensor-value' style='color:%s;'>%.1f °C</div>"
        "<div class='sensor-label'>Температура",
        tempColor, tempColor, tempColor, currentTemp);
    html += buffer;
    
    #if DEVICE_TYPE == 1
    snprintf(buffer, sizeof(buffer), " (выкл: %.1f вкл: %.1f)", config.lowTemp, config.highTemp);
    html += buffer;
    #endif
    
    html += "</div></div>";
    
    // Влажность
    snprintf(buffer, sizeof(buffer),
        "<div class='sensor-card' style='background:%s20; border:2px solid %s;'>"
        "<div class='sensor-value' style='color:%s;'>%.1f %%</div>"
        "<div class='sensor-label'>Влажность",
        humColor, humColor, humColor, currentHum);
    html += buffer;
    
    #if DEVICE_TYPE == 1
    snprintf(buffer, sizeof(buffer), " (выкл: %.1f вкл: %.1f)", config.lowHum, config.highHum);
    html += buffer;
    #endif
    
    html += "</div></div></div>";
    
    if (!sensorOk && strlen(sensorError) > 0) {
        snprintf(buffer, sizeof(buffer), 
            "<div class='sensor-error'><strong>Ошибка датчика</strong><br>%s</div>", sensorError);
        html += buffer;
    }
    #endif // DEVICE_TYPE == 1 || 2
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    #if DEVICE_TYPE == 1
    bool state = fan_getState();
    const char* label = "Вентилятор";
    const char* toggleUrl = "/fan/toggle";
    #else
    bool state = switch_getState();
    const char* label = "Выключатель";
    const char* toggleUrl = "/switch/toggle";
    #endif
    
    const char* stateColor = state ? "#f44336" : "#2196F3";
    const char* stateText = state ? "ВКЛ" : "ВЫКЛ";
    
    snprintf(buffer, sizeof(buffer),
        "<a href='%s'><div class='status-card' style='background:%s20; border:2px solid %s;'>"
        "<div style='font-size:2em;font-weight:bold;color:%s;'>%s: %s</div></div></a>",
        toggleUrl, stateColor, stateColor, stateColor, label, stateText);
    html += buffer;
    
    #if DEVICE_TYPE == 1
    if (state) {
        int currentSpeed = startingPulseActive ? 100 : config.speedPercent;
        snprintf(buffer, sizeof(buffer),
            "<div class='status-card' style='background:#2196F320; border:2px solid #2196F3;'>"
            "<div style='font-size:1.2em;font-weight:bold;'>Скорость: %d%%</div>"
            "<div class='duty-bar'><div class='duty-fill' style='width:%d%%;'></div></div>",
            currentSpeed, currentSpeed);
        html += buffer;
        if (config.speedPercent < 100) {
            html += "<div style='font-size:0.9em;color:#555;'>Тихий режим активен";
            if (config.adaptiveMode) html += " + адаптация";
            html += "</div>";
        }
        html += "</div>";
    }
    
    const char* modeText = config.sensorControlMode ? "УПРАВЛЕНИЕ СЕНСОРОМ" : "РУЧНОЙ";
    const char* modeColor = config.sensorControlMode ? "#4CAF50" : "#f44336";
    
    snprintf(buffer, sizeof(buffer),
        "<div class='status-card' style='background:%s20; border:2px solid %s;'>"
        "<div style='font-size:1.5em;font-weight:bold;color:%s;'>Режим: %s</div></div>",
        modeColor, modeColor, modeColor, modeText);
    html += buffer;
    #endif // DEVICE_TYPE == 1
    #endif // DEVICE_TYPE == 1 || 3
    
    // Информационная панель
    html += "<hr><div class='info'>";
    snprintf(buffer, sizeof(buffer), "Обновление: %d сек<br>", refreshInterval);
    html += buffer;
    
    #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
    snprintf(buffer, sizeof(buffer), "Опрос датчика %d сек<br>", config.sensorInterval);
    html += buffer;
    #endif
    
    #if MQTT_ENABLED == 1
    html += "MQTT: ";
    html += mqttManager.isConnected() ? "подключен" : "отключен";
    html += "<br>";
    #endif
    
    #if WEB_SHOW_RSSI == 1
    snprintf(buffer, sizeof(buffer), "RSSI: %d dBm<br>", WiFi.RSSI());
    html += buffer;
    #endif
    
    html += "</div>";
    
    // Кнопки
    html += "<div class='button-group'>";
    #if DEVICE_TYPE == 1
    if (!config.sensorControlMode && sensorOk) {
        html += "<a href='/fan/auto'><button>Режим управления сенсором</button></a>";
    }
    #endif
    html += "<a href='/config'><button>Настройки</button></a>";
    html += "</div>";
    
    html += FPSTR(HTML_PAGE_END);
    return html;
}

inline String renderConfigPage(const String& errorMsg = "") {
    String html = FPSTR(HTML_PAGE_START);
    html.replace("{META_REFRESH}", "");
    
    char title[64];
    snprintf(title, sizeof(title), "<title>%s Configuration</title>", DEVICE_PREFIX);
    html.replace("{TITLE}", title);
    
    // Заголовок
    char header[256];
    snprintf(header, sizeof(header), "<h1>Настройка устройства %s v. %s</h1>", deviceId, VERSION);
    html += header;
    
    // Блок состояния
    html += "<h3>Текущее состояние</h3><div class='info'>";
    #if AP_ENABLED == 1
    if (apMode) {
        html += "Режим: <strong>Точка доступа (AP)</strong><br>";
        snprintf(header, sizeof(header), "SSID: <strong>%s</strong><br>", deviceId);
        html += header;
        snprintf(header, sizeof(header), "IP адрес: <strong>%s</strong><br>", AP_IP_ADDRESS);
        html += header;
    } else 
    #endif
    {
        html += "Режим: <strong>Клиент WiFi</strong><br>";
        snprintf(header, sizeof(header), "SSID: <strong>%s</strong><br>", staticConfig.wifiSsid);
        html += header;
        snprintf(header, sizeof(header), "IP адрес: <strong>%s</strong><br>", WiFi.localIP().toString().c_str());
        html += header;
    }
    html += "</div>";
    
    if (errorMsg.length() > 0) {
        snprintf(header, sizeof(header), "<div class='error'><strong>Ошибка:</strong> %s</div>", errorMsg.c_str());
        html += header;
    }
    
    // Форма
    html += "<form method='POST' action='/save'>";
    html += "<h3>Настройки сети</h3>";
    html += "<label>WiFi SSID:</label>";
    snprintf(header, sizeof(header), "<input type='text' name='wifiSsid' required value='%s'>", staticConfig.wifiSsid);
    html += header;
    html += "<label>WiFi Password:</label>";
    html += "<input type='password' name='wifiPassword' placeholder='(не показан)'>";
    html += "<div class='password-hint'>Оставьте пустым, чтобы сохранить текущий пароль</div>";
    
    #if MQTT_ENABLED == 1
    html += "<h3>MQTT настройки</h3>";
    html += "<div class='row'><div><label>MQTT Broker:</label>";
    snprintf(header, sizeof(header), "<input type='text' name='mqttBroker' required value='%s'></div>", staticConfig.mqttBroker);
    html += header;
    html += "<div><label>MQTT Port:</label>";
    snprintf(header, sizeof(header), "<input type='number' name='mqttPort' required value='%d'></div></div>", staticConfig.mqttPort);
    html += header;
    html += "<div class='row'><div><label>MQTT User:</label>";
    snprintf(header, sizeof(header), "<input type='text' name='mqttUser' value='%s'></div>", staticConfig.mqttUser);
    html += header;
    html += "<div><label>MQTT Password:</label>";
    html += "<input type='password' name='mqttPassword' placeholder='(не показан)'></div></div>";
    html += "<div class='password-hint'>Оставьте пустым, чтобы сохранить текущий пароль</div>";
    html += "<label>MQTT Client ID:</label>";
    snprintf(header, sizeof(header), "<input type='text' name='mqttClientId' required value='%s'>", staticConfig.mqttClientId);
    html += header;
    #endif
    
    #if DEVICE_TYPE == 1
    html += "<h3>Настройки датчиков</h3>";
    html += "<div class='row'><div><label>Low Temp (°C):</label>";
    snprintf(header, sizeof(header), "<input type='number' step='0.1' name='lowTemp' required value='%.1f'></div>", staticConfig.lowTemp);
    html += header;
    html += "<div><label>High Temp (°C):</label>";
    snprintf(header, sizeof(header), "<input type='number' step='0.1' name='highTemp' required value='%.1f'></div></div>", staticConfig.highTemp);
    html += header;
    html += "<div class='row'><div><label>Low Hum (%):</label>";
    snprintf(header, sizeof(header), "<input type='number' step='0.1' name='lowHum' required value='%.1f'></div>", staticConfig.lowHum);
    html += header;
    html += "<div><label>High Hum (%):</label>";
    snprintf(header, sizeof(header), "<input type='number' step='0.1' name='highHum' required value='%.1f'></div></div>", staticConfig.highHum);
    html += header;
    html += "<div class='row'><div><label>Интервал опроса датчика (сек)</label>";
    snprintf(header, sizeof(header), "<input type='number' name='sensorInterval' required value='%d'></div>", staticConfig.sensorInterval);
    html += header;
    html += "<div><label>Аварийное отключение через </label>";
    snprintf(header, sizeof(header), "<input type='number' name='maxOnTime' min='0' required value='%u'></div></div>", staticConfig.maxOnTime);
    html += header;
    
    html += "<h3>Управление</h3>";
    html += "<label>Принудительно включить через </label>";
    snprintf(header, sizeof(header), "<input type='number' name='delaySeconds' required value='%d'> сек", staticConfig.delaySeconds);
    html += header;
    
    html += "<h3>Тихий режим (ШИМ)</h3>";
    html += "<label>Скорость (0-100%):</label>";
    snprintf(header, sizeof(header), "<input type='number' name='speedPercent' min='0' max='100' required value='%d'>", staticConfig.speedPercent);
    html += header;
    html += "<div class='note'>0% - выключено, 100% - полная мощность (тихий режим выключен).<br>При значении ниже 100% вентилятор работает тише.</div>";
    
    html += "<h3>Адаптивный тихий режим</h3>";
    html += "<label><input type='checkbox' name='adaptiveMode' value='1'";
    if (staticConfig.adaptiveMode) html += " checked";
    html += "> Включить адаптацию</label>";
    html += "<div class='note'>Адаптивный режим автоматически регулирует скорость для поддержания температуры и влажности на уровне, зафиксированном при включении вентилятора.</div>";
    
    html += "<h3>Поведение при старте</h3>";
    html += "<label><input type='checkbox' name='bootState' value='1'";
    if (staticConfig.bootState) html += " checked";
    html += "> Включать при старте</label>";
    html += "<div class='note'>При включенной опции вентилятор будет включен сразу после подачи питания.</div>";
    
    html += "<h3>Режимы работы</h3>";
    html += "<label><input type='checkbox' name='sensorControlMode' value='1'";
    if (staticConfig.sensorControlMode) html += " checked";
    html += "> Режим управления сенсором</label>";
    html += "<div class='note'>При включённом режиме вентилятор управляется по показаниям датчиков температуры и влажности. При выключении — только вручную.</div>";
    
    #elif DEVICE_TYPE == 2
    html += "<h3>Настройки датчиков</h3>";
    html += "<label>Интервал опроса датчика (сек)</label>";
    snprintf(header, sizeof(header), "<input type='number' name='sensorInterval' required value='%d'>", staticConfig.sensorInterval);
    html += header;
    
    #elif DEVICE_TYPE == 3
    html += "<h3>Управление</h3>";
    html += "<label>Принудительно включить через </label>";
    snprintf(header, sizeof(header), "<input type='number' name='delaySeconds' required value='%d'> сек<br>", staticConfig.delaySeconds);
    html += header;
    html += "<label>Аварийное отключение через </label>";
    snprintf(header, sizeof(header), "<input type='number' name='maxOnTime' min='0' required value='%u'> сек", staticConfig.maxOnTime);
    html += header;
    
    html += "<h3>Поведение при старте</h3>";
    html += "<label><input type='checkbox' name='bootState' value='1'";
    if (staticConfig.bootState) html += " checked";
    html += "> Включать при старте</label>";
    html += "<div class='note'>При включенной опции выключатель будет включен сразу после подачи питания.</div>";
    #endif
    
    html += "<label><input type='checkbox' name='confirmSave' required> Подтвердить сохранение</label>";
    html += "<input type='submit' value='Сохранить и перезагрузить'>";
    html += "</form>";
    
    #if OTA_ENABLED == 1
    if (web_isOtaAvailable()) {
        html += "<a href='/update' class='link-btn'>Обновить прошивку (OTA)</a>";
    } else {
        html += "<div class='warning' style='text-align:center;background:#fff3cd;padding:10px;border-radius:5px;'>OTA недоступно: недостаточно Flash памяти (требуется 2MB)</div>";
    }
    #endif
    
    html += "<a href='/' class='link-btn'>Домой</a>";
    html += FPSTR(HTML_PAGE_END);
    
    return html;
}

#endif // WEB_TEMPLATES_H