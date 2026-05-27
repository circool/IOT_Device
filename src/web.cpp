#include "web.h"
#include "sensor.h"
#if DEVICE_TYPE == 1
  #include "fan.h"
#endif

#if DEVICE_TYPE == 3
  #include "switch.h"
#endif

#include "config.h"
#if MQTT_ENABLED == 1
  #include "mqtt.h"
#endif


#ifdef ESP32
  #include <WiFi.h>
  #include <AsyncTCP.h>
  #include <ESPAsyncWebServer.h>
  #include <esp_chip_info.h>
  #include <esp_flash.h>
  #if OTA_ENABLED == 1
    #include <ElegantOTA.h>
  #endif
  extern AsyncWebServer server;
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  #include <DNSServer.h>
  #if OTA_ENABLED == 1
    #include <ElegantOTA.h>
  #endif
  #include <Esp.h>
  extern ESP8266WebServer server;
  
  DNSServer dnsServer;
  const byte DNS_PORT = 53;
#endif







#if OTA_ENABLED == 1
  static bool otaInitialized = false;
#endif

#if DEVICE_TYPE == 1

  void handleToggle() {

      config.sensorControlMode = false;
    fan_set(!fan_getState());
  }

  void handleSensorControlMode() {
    fan_setOverrideMode(true);
  }
#endif

#if DEVICE_TYPE == 3
  void handleToggle() {
    switch_set(!switch_getState());
  }
#endif


String web_getConfigPage(String errorMsg) {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>";
  #if DEVICE_TYPE == 1
    html += "Fan";
  #elif DEVICE_TYPE == 2
    html += "Sensor";
  #elif DEVICE_TYPE == 3
    html += "Switch";
  #else
    html += "Device";
  #endif
  html += " Configuration</title>";
  html += "<style>";
  html += "body{font-family:Arial;margin:20px;background:#f0f0f0;}";
  html += ".container{max-width:700px;margin:auto;background:white;padding:20px;border-radius:10px;}";
  html += "h1{color:#2c3e50;}h3{color:#2c3e50;border-bottom:1px solid #ccc;padding-bottom:5px;}";
  html += "label{display:block;margin-top:10px;font-weight:bold;}";
  html += "input[type=text],input[type=password],input[type=number]{width:100%;padding:8px;margin:5px 0;border:1px solid #ccc;border-radius:4px;font-size:1.2em;box-sizing:border-box;}";
  html += "input[type=checkbox]{width:20px;height:20px;margin-right:10px;vertical-align:middle;cursor:pointer;transform:scale(1.5);}";
  html += "input[type=submit]{background:#2c3e50;color:white;padding:10px 20px;margin-top:20px;border:none;border-radius:4px;cursor:pointer;width:100%;font-size:1em;box-sizing:border-box;}";
  html += "input[type=submit]:hover{background:#1a252f;}";
  html += ".info{background:#e7f3ff;padding:10px;border-radius:5px;margin:10px 0;}";
  html += ".warning{background:#fff3cd;padding:10px;border-radius:5px;margin:10px 0;color:#856404;}";
  html += ".error{background:#ffebee;padding:10px;border-radius:5px;margin:10px 0;color:#c62828;}";
  html += ".row{display:flex;gap:10px;}.row>div{flex:1;}";
  html += ".password-hint{color:#7f8c8d;margin-top:-2px;margin-bottom:8px;}";
  html += ".note{background:#f9f9f9;padding:8px;margin-top:10px;border-left:3px solid #2c3e50;font-size:0.9em;color:#555;}";
  html += ".link-btn{background:#555;color:white;padding:10px 20px;margin-top:10px;border:none;border-radius:4px;cursor:pointer;font-size:1em;text-align:center;text-decoration:none;display:block;box-sizing:border-box;}";
  html += ".link-btn:hover{background:#333;}"; 
  html += "</style></head><body><div class='container'>";
  html += "<h1>Настройка устройства " + String (deviceId)  + " v. " + String(VERSION) + "</h1>";
  html += "<h3>Текущее состояние</h3>";
  html += "<div class='info'>";
  if (apMode) {  
    html += "Platform: <strong>";
    #ifdef ESP32
      html += "ESP32";
    #elif defined(ESP8266)
      html += "ESP8266";
    #else
      html += "Unknown";
    #endif

    html += "</strong><br>";
    html += "Режим: <strong>Точка доступа (AP)</strong><br>";
    html += "SSID: <strong>" + String(deviceId) + "</strong><br>";
    html += "IP адрес: <strong>" + String(AP_IP_ADDRESS) + "</strong><br>";   
  } else {
    html += "Режим: <strong>Клиент WiFi</strong><br>";
    html += "SSID: <strong>" + String(staticConfig.wifiSsid) + "</strong><br>";
    html += "IP адрес: <strong>" + WiFi.localIP().toString() + "</strong><br>";   
  }
  html += "</div><form method='POST' action='/save'>";
  
  if (errorMsg.length() > 0) {
    html += "<div class='error'><strong>Ошибка:</strong> " + errorMsg + "</div>";
  }
  
  html += "<h3>Настройки сети</h3>";
  html += "<label>WiFi SSID:</label><input type='text' name='wifiSsid' required value='" + String(staticConfig.wifiSsid) + "'>";
  html += "<label>WiFi Password:</label><input type='password' name='wifiPassword' placeholder='(не показан)'>";
  html += "<div class='password-hint'>Оставьте пустым, чтобы сохранить текущий пароль WiFi</div>";
  #if MQTT_ENABLED == 1
  html += "<h3>MQTT настройки</h3>";
  html += "<div class='row'><div><label>MQTT Broker:</label><input type='text' name='mqttBroker' required value='" + String(staticConfig.mqttBroker) + "'></div>";
  html += "<div><label>MQTT Port:</label><input type='number' name='mqttPort' required value='" + String(staticConfig.mqttPort) + "'></div></div>";
  html += "<div class='row'><div><label>MQTT User:</label><input type='text' name='mqttUser' value='" + String(staticConfig.mqttUser) + "'></div>";
  html += "<div><label>MQTT Password:</label><input type='password' name='mqttPassword' placeholder='(не показан)'></div></div>";
  html += "<div class='password-hint'>Оставьте пустым, чтобы сохранить текущий пароль MQTT</div>";
  html += "<label>MQTT Client ID:</label><input type='text' name='mqttClientId' required value='" + String(staticConfig.mqttClientId) + "'>";
  #endif

  #if DEVICE_TYPE == 1
  html += "<h3>Настройки датчиков</h3>";
  html += "<div class='row'><div><label>Low Temp (°C):</label><input type='number' step='0.1' name='lowTemp' required value='" + String(staticConfig.lowTemp) + "'></div>";
  html += "<div><label>High Temp (°C):</label><input type='number' step='0.1' name='highTemp' required value='" + String(staticConfig.highTemp) + "'></div></div>";
  html += "<div class='row'><div><label>Low Hum (%):</label><input type='number' step='0.1' name='lowHum' required value='" + String(staticConfig.lowHum) + "'></div>";
  html += "<div><label>High Hum (%):</label><input type='number' step='0.1' name='highHum' required value='" + String(staticConfig.highHum) + "'></div></div>";
  html += "<div class='row'><div><label>Интервал опроса датчика (сек)</label><input type='number' name='sensorInterval' required value='" + String(staticConfig.sensorInterval) + "'></div>";
  html += "<div><label>Аварийное отключение через </label><input type='number' name='maxOnTime' min='0' required value='" + String(staticConfig.maxOnTime) + "'></div></div>";
  html += "<h3>Управление</h3>";
  html += "<label>Принудительно включить через </label><input type='number' name='delaySeconds' required value='" + String(staticConfig.delaySeconds) + "'>";
  
  html += "<h3>Тихий режим (ШИМ)</h3>";
  html += "<label>Скорость (0-100%):</label>";  
  html += "<input type='number' name='speedPercent' min='0' max='100' required value='" + String(staticConfig.speedPercent) + "'>";  
  html += "<div class='note'>0% - выключено, 100% - полная мощность (тихий режим выключен).<br>";
  html += "При значении ниже 100% вентилятор работает тише.</div>";
  
  html += "<h3>Адаптивный тихий режим</h3>";
  html += "<label><input type='checkbox' name='adaptiveMode' value='1' " + String(staticConfig.adaptiveMode ? "checked" : "") + "> Включить адаптацию</label>";
  html += "<div class='note'>Адаптивный режим автоматически регулирует скорость для поддержания температуры и влажности на уровне, зафиксированном при включении вентилятора.</div>";
  
  html += "<h3>Поведение при старте</h3>";
  html += "<label><input type='checkbox' name='bootState' value='1' " + String(staticConfig.bootState ? "checked" : "") + "> Включать при старте</label>";
  html += "<div class='note'>При включенной опции вентилятор будет включен сразу после подачи питания.</div>";
  
  html += "<h3>Режимы работы</h3>";
  html += "<label><input type='checkbox' name='sensorControlMode' value='1' " + String(staticConfig.sensorControlMode ? "checked" : "") + "> Режим управления сенсором</label>";
  html += "<div class='note'>При включённом режиме вентилятор управляется по показаниям датчиков температуры и влажности. При выключении — только вручную.</div>";
  #endif
  
  #if DEVICE_TYPE == 2
  html += "<h3>Настройки датчиков</h3>";
  html += "<div><label>Интервал опроса датчика (сек)</label><input type='number' name='sensorInterval' required value='" + String(staticConfig.sensorInterval) + "'></div>";
  #endif
  
  #if DEVICE_TYPE == 3
  html += "<h3>Настройки управления</h3>";
  html += "<label>Принудительно включить через </label><input type='number' name='delaySeconds' required value='" + String(staticConfig.delaySeconds) + "'>";
  html += "<label>Аварийное отключение через </label><input type='number' name='maxOnTime' min='0' required value='" + String(staticConfig.maxOnTime) + "'>";
  //@deprecated - ШИМ только у вентилятора!
  // html += "<h3>Тихий режим (ШИМ)</h3>";
  // html += "<label>Скорость (0-100%):</label>";  
  // html += "<input type='number' name='speedPercent' min='0' max='100' required value='" + String(staticConfig.speedPercent) + "'>";  
  // html += "<div class='note'>0% - выключено, 100% - полная мощность (тихий режим выключен).<br>";
  // html += "При значении ниже 100% выключатель работает в режиме ШИМ.</div>";
  
  html += "<h3>Поведение при старте</h3>";
  html += "<label><input type='checkbox' name='bootState' value='1' " + String(staticConfig.bootState ? "checked" : "") + "> Включать при старте</label>";
  html += "<div class='note'>При включенной опции выключатель будет включен сразу после подачи питания.</div>";
  
  // html += "<h3>Режимы работы</h3>";
  // html += "<label><input type='checkbox' name='sensorControlMode' value='1' " + String(staticConfig.sensorControlMode ? "checked" : "") + "> Режим управления сенсором</label>";
  // html += "<div class='note'>Для TYPE 3 (управляемый выключатель) этот режим не использует датчики, только ручное управление.</div>";
  #endif
  
  html += "<label><input type='checkbox' name='confirmSave' required> Подтвердить сохранение</label>";
  html += "<input type='submit' value='Сохранить и перезагрузить'>";
  html += "</form>";
  
  #if OTA_ENABLED == 1
  html += "<a href='/update' class='link-btn'>Обновить прошивку (OTA)</a>";
  #endif
  
  html += "<a href='/' class='link-btn'>Домой</a>";

  html += "</div></body></html>"; 
  return html;
}

#if WEB_STATUS_ENABLED==1
  String web_getStatusPage(int refreshInterval) {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
  html += "<meta http-equiv='refresh' content='" + String(refreshInterval) + "'>";
  html += "<title>" + String(DEVICE_PREFIX) + "</title>";
  html += "<style>";
  html += "body{font-family:Arial;margin:20px;background:#f0f0f0;}";
  html += ".container{max-width:800px;margin:auto;background:white;padding:20px;border-radius:10px;}";
  html += "h1{color:#2c3e50;text-align:center;word-break:break-all;}";
  html += ".sensor-card{display:inline-block;width:45%;margin:10px;padding:15px;border-radius:10px;text-align:center;}";
  html += ".sensor-value{font-size:2em;font-weight:bold;}.sensor-label{margin-top:5px;}";
  html += ".status-card{padding:15px;border-radius:10px;text-align:center;margin:10px;}";
  html += ".info{color:#7f8c8d;margin-top:20px;text-align:center;}";
  
  html += "button{background:#2c3e50;color:white;padding:10px;border:none;border-radius:4px;cursor:pointer;margin:5px;font-size:1em}";
  html += ".flex-container{display:flex;flex-wrap:wrap;justify-content:center;}";
  html += ".button-group{display:flex;justify-content:center;gap:10px;margin-top:20px;flex-wrap:wrap;}";
  html += "a{text-decoration:none;}";
  html += ".sensor-error{background:#ffebee;padding:15px;border-radius:8px;margin:15px 10px;color:#c62828;text-align:center;border:2px solid #ef9a9a;}";
  html += ".duty-bar{background:#e0e0e0;border-radius:10px;margin:10px 0;height:20px;overflow:hidden;}";
  html += ".duty-fill{background:#2c3e50;height:100%;border-radius:10px;transition:width 0.3s;}";
  html += "</style></head><body><div class='container'>";
  
  html += "<h1>" + String(DEVICE_PREFIX) + " VERSION " + String(VERSION) + "</h1>";
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  html += "<div class='flex-container'>";
  
  #if DEVICE_TYPE == 1
  String tempColor = (currentTemp >= config.highTemp) ? "#f44336" : (currentTemp <= config.lowTemp) ? "#4CAF50" : "#2196F3";
  String humColor = (currentHum >= config.highHum) ? "#f44336" : (currentHum <= config.lowHum) ? "#4CAF50" : "#2196F3";
  #else
  String tempColor = "#2196F3";
  String humColor = "#2196F3";
  #endif
  
  html += "<div class='sensor-card' style='background:" + tempColor + "20; border:2px solid " + tempColor + ";'>";
  html += "<div class='sensor-value' style='color:" + tempColor + ";'>" + String(currentTemp, 1) + " °C</div>";
  html += "<div class='sensor-label'>Температура";
  
  #if DEVICE_TYPE == 1
  html += " (выкл: " + String(config.lowTemp, 1) + " вкл: " + String(config.highTemp, 1) + ")";
  #endif
  
  html += "</div></div>";
  
  html += "<div class='sensor-card' style='background:" + humColor + "20; border:2px solid " + humColor + ";'>";
  html += "<div class='sensor-value' style='color:" + humColor + ";'>" + String(currentHum, 1) + " %</div>";
  html += "<div class='sensor-label'>Влажность";
  
  #if DEVICE_TYPE == 1
  html += " (выкл: " + String(config.lowHum, 1) + " вкл: " + String(config.highHum, 1) + ")";
  #endif
  
  html += "</div></div>"; 
  html += "</div>";
  
  if (!sensorOk && sensorError.length() > 0) {
    html += "<div class='sensor-error'>";
    html += "<strong>Ошибка датчика</strong><br>";
    html += sensorError;
    html += "</div>";
  }
  #endif 

  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  
  
  
  #if DEVICE_TYPE == 1
    bool state = fan_getState();
    String label = "Вентилятор";
    String toggleUrl = "/fan/toggle";
    String autoUrl = "/fan/auto";
  #else
    bool state = switch_getState();
    String label = "Выключатель";
    String toggleUrl = "/switch/toggle";
  #endif
  
  String stateColor = state ? "#f44336" : "#2196F3";
  String stateText = state ? "ВКЛ" : "ВЫКЛ";

  html += "<a href='" + toggleUrl + "'>";
  html += "<div class='status-card' style='background:" + stateColor + "20; border:2px solid " + stateColor + ";'>";
  html += "<div style='font-size:2em;font-weight:bold;color:" + stateColor + ";'>" + label + ": " + stateText + "</div></div>";
  html += "</a>";
  
  #if DEVICE_TYPE == 1
    if (state) {
      int currentSpeed;
      if (startingPulseActive) {
        currentSpeed = 100;
      } else {
        currentSpeed = config.speedPercent;  
      }
      
      html += "<div class='status-card' style='background:#2196F320; border:2px solid #2196F3;'>";
      html += "<div style='font-size:1.2em;font-weight:bold;'>Скорость: " + String(currentSpeed) + "%</div>";
      html += "<div class='duty-bar'><div class='duty-fill' style='width:" + String(currentSpeed) + "%;'></div></div>";
      if (config.speedPercent < 100) {  
        html += "<div style='font-size:0.9em;color:#555;'>Тихий режим активен";
        if (config.adaptiveMode && DEVICE_TYPE == 1) {
          html += " + адаптация";
        }
        html += "</div>";
      } else if (config.speedPercent == 100) {  
        html += "<div style='font-size:0.9em;color:#555;'>Максимальная скорость</div>";
      }
      html += "</div>";
    }
    
    String modeText = config.sensorControlMode ? "УПРАВЛЕНИЕ СЕНСОРОМ" : "РУЧНОЙ";
    String modeColor = config.sensorControlMode ? "#4CAF50" : "#f44336";
  
  

    html += "<div class='status-card' style='background:" + modeColor + "20; border:2px solid " + modeColor + ";'>";
    html += "<div style='font-size:1.5em;font-weight:bold;color:" + modeColor + ";'>Режим: " + modeText + "</div>";
    

    if (config.sensorControlMode  ) {
      if (config.adaptiveMode && adaptiveActive) {
        html += "<div style='font-size:0.9em;color:#4CAF50;'>Адаптивный режим активен</div>";
      } else if (config.adaptiveMode && !adaptiveActive) {
        html += "<div style='font-size:0.9em;color:#FF9800;'>Адаптивный режим ожидает включения вентилятора</div>";
      } else if (!config.adaptiveMode && config.speedPercent != 100) {  
        html += "<div style='font-size:0.9em;color:#f44336;'>Адаптивный режим отключён (ручное управление скоростью)</div>";
      } else if (!config.adaptiveMode && config.speedPercent == 100) {  
        html += "<div style='font-size:0.9em;color:#f44336;'>Адаптивный режим отключён</div>";
      }
    }
    
    
    if (state) {
      int currentSpeed;  
      if (startingPulseActive) {
        currentSpeed = 100;
      } else {
        currentSpeed = config.speedPercent;  
      }
      
      if (config.speedPercent == 100) {  
        html += "<div style='font-size:0.9em;color:#555;'>Режим: максимальная скорость</div>";
      } else if (config.adaptiveMode && adaptiveActive) {
        html += "<div style='font-size:0.9em;color:#4CAF50;'>Режим: адаптивный тихий (цель: " + String(currentSpeed) + "%)</div>";
      } else if (!config.adaptiveMode && config.speedPercent < 100) {  
        html += "<div style='font-size:0.9em;color:#FF9800;'>Режим: ручной тихий (" + String(currentSpeed) + "%)</div>";
      } else {
        html += "<div style='font-size:0.9em;color:#555;'>Режим: максимальная скорость</div>";
      }
    }
  #endif
  
  html += "</div>";
#endif
  
  html += "<hr><div class='info'>Обновление: " + String(refreshInterval) + " сек<br>";
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  html += "Опрос датчика " + String(config.sensorInterval) + " сек<br>";
  #endif
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
  if (delayActive && delayTimer > 0) {
    unsigned long now = millis();
    if (now < delayTimer) {
      unsigned long remaining = (delayTimer - now + 999) / 1000;
      html += "Принудительное включение через <strong>" + String(remaining) + "</strong> сек<br>";
    } else {
      html += "Принудительное включение: <strong>выполняется...</strong><br>";
    }
  } else if (config.delaySeconds > 0) {
    html += "Автоматический старт: настроено на " + String(config.delaySeconds) + " сек<br>";
  } else {
    html += "Автоматический старт: <strong>отключено</strong><br>";
  }
  
  #if DEVICE_TYPE == 1
  if (fanOn && config.maxOnTime > 0 && fanStartTime > 0) {
    unsigned long elapsed = (millis() - fanStartTime) / 1000;
  #endif

  #if DEVICE_TYPE == 3
  if (switchOn && config.maxOnTime > 0 && switchStartTime > 0) {
    unsigned long elapsed = (millis() - switchStartTime) / 1000;
  #endif

    if (elapsed < config.maxOnTime) {
      unsigned long remaining = config.maxOnTime - elapsed;
      html += "Аварийное отключение через <strong>" + String(remaining) + "</strong> сек<br>";
    } else {
      html += "Аварийное отключение: <strong>сейчас</strong><br>";
    }
  } else if (config.maxOnTime > 0) {
    html += "Аварийное отключение: неактивно (лимит " + String(config.maxOnTime) + " сек)<br>";
  } else {
    html += "Аварийное отключение: <strong>отключено</strong><br>";
  }
  #endif

  #if MQTT_ENABLED == 1
  html += "MQTT: " + String(mqttManager.isConnected() ? "подключен" : "отключен") + "<br>";
  #endif

  #if WEB_SHOW_RSSI == 1
    int rssi = WiFi.RSSI();
    html += "RSSI: " + String(rssi) + "<br>";
  #endif

  html += "</div>";
  
  html += "<div class='button-group'>";
  #if DEVICE_TYPE == 1
  if (!config.sensorControlMode) {
    html += "<a href='" + autoUrl + "'><button>Режим управления сенсором</button></a>";
  }
  #endif

  html += "<a href='/config'><button>Настройки</button></a>";
  html += "</div></div></body></html>";
  
  return html;
}
#endif 

#ifdef ESP32
void web_saveConfig(AsyncWebServerRequest *request) {
  if (request->hasParam("wifiSsid", true)) {
    const AsyncWebParameter* p = request->getParam("wifiSsid", true);
    if (p) p->value().toCharArray(config.wifiSsid, sizeof(config.wifiSsid));
  }
  if (request->hasParam("wifiPassword", true)) {
    const AsyncWebParameter* p = request->getParam("wifiPassword", true);
    if (p) {
      String pwd = p->value();
      if (pwd.length() > 0) pwd.toCharArray(config.wifiPassword, sizeof(config.wifiPassword));
    }
  }

  #if MQTT_ENABLED == 1
  if (request->hasParam("mqttBroker", true)) {
    const AsyncWebParameter* p = request->getParam("mqttBroker", true);
    if (p) p->value().toCharArray(config.mqttBroker, sizeof(config.mqttBroker));
  }
  if (request->hasParam("mqttPort", true)) {
    const AsyncWebParameter* p = request->getParam("mqttPort", true);
    if (p) config.mqttPort = p->value().toInt();
  }
  if (request->hasParam("mqttUser", true)) {
    const AsyncWebParameter* p = request->getParam("mqttUser", true);
    if (p) p->value().toCharArray(config.mqttUser, sizeof(config.mqttUser));
  }
  if (request->hasParam("mqttPassword", true)) {
    const AsyncWebParameter* p = request->getParam("mqttPassword", true);
    if (p) {
      String pwd = p->value();
      if (pwd.length() > 0) pwd.toCharArray(config.mqttPassword, sizeof(config.mqttPassword));
    }
  }
  if (request->hasParam("mqttClientId", true)) {
    const AsyncWebParameter* p = request->getParam("mqttClientId", true);
    if (p) {
      String cid = p->value();
      if (cid.length() > 0 && cid.length() < sizeof(config.mqttClientId)) {
        cid.toCharArray(config.mqttClientId, sizeof(config.mqttClientId));
      } else if (cid.length() == 0) {
        config.mqttClientId[0] = '\0';
      }
    }
  }
  #endif


  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  if (request->hasParam("sensorInterval", true)) {
    const AsyncWebParameter* p = request->getParam("sensorInterval", true);
    if (p) config.sensorInterval = p->value().toInt();
  }
  #endif
  
  #if DEVICE_TYPE == 1
  if (request->hasParam("lowTemp", true)) {
    const AsyncWebParameter* p = request->getParam("lowTemp", true);
    if (p) config.lowTemp = p->value().toFloat();
  }
  if (request->hasParam("highTemp", true)) {
    const AsyncWebParameter* p = request->getParam("highTemp", true);
    if (p) config.highTemp = p->value().toFloat();
  }
  if (request->hasParam("lowHum", true)) {
    const AsyncWebParameter* p = request->getParam("lowHum", true);
    if (p) config.lowHum = p->value().toFloat();
  }
  if (request->hasParam("highHum", true)) {
    const AsyncWebParameter* p = request->getParam("highHum", true);
    if (p) config.highHum = p->value().toFloat();
  }
  if (request->hasParam("maxOnTime", true)) {
    const AsyncWebParameter* p = request->getParam("maxOnTime", true);
    if (p) config.maxOnTime = p->value().toInt();
  }
  if (request->hasParam("delaySeconds", true)) {
    const AsyncWebParameter* p = request->getParam("delaySeconds", true);
    if (p) config.delaySeconds = p->value().toInt();
  }
  
  if (request->hasParam("speedPercent", true)) {  
    const 
    AsyncWebParameter* p = request->getParam("speedPercent", true);
    if (p) config.speedPercent = p->value().toInt();
  }
  config.adaptiveMode = request->hasParam("adaptiveMode", true);
  config.bootState = request->hasParam("bootState", true);
  config.sensorControlMode = request->hasParam("sensorControlMode", true);
  #endif
  
  #if DEVICE_TYPE == 3
  if (request->hasParam("maxOnTime", true)) {
    const AsyncWebParameter* p = request->getParam("maxOnTime", true);
    if (p) config.maxOnTime = p->value().toInt();
  }
  if (request->hasParam("delaySeconds", true)) {
    const AsyncWebParameter* p = request->getParam("delaySeconds", true);
    if (p) config.delaySeconds = p->value().toInt();
  }
  // if (request->hasParam("speedPercent", true)) {  
  //   AsyncWebParameter* p = request->getParam("speedPercent", true);
  //   if (p) config.speedPercent = p->value().toInt();
  // }
  config.bootState = request->hasParam("bootState", true);
  // config.sensorControlMode = request->hasParam("sensorControlMode", true);
  #endif
  
  if (!config_validate()) {
    request->send(200, "text/html", web_getConfigPage(configLastError));
    return;
  }
  
  config_write();
  request->send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'></head><body><div style='text-align:center;margin-top:50px;'><h2>Настройки сохранены!</h2><p>Перезагрузка...</p></div></body></html>");
  delay(1000);
  ESP.restart();
}
#elif defined(ESP8266)
void web_saveConfig() {
  if (server.hasArg("wifiSsid"))
    server.arg("wifiSsid").toCharArray(config.wifiSsid, sizeof(config.wifiSsid));
  if (server.hasArg("wifiPassword")) {
    String pwd = server.arg("wifiPassword");
    if (pwd.length() > 0) pwd.toCharArray(config.wifiPassword, sizeof(config.wifiPassword));
  }

  #if MQTT_ENABLED == 1
  if (server.hasArg("mqttBroker"))
    server.arg("mqttBroker").toCharArray(config.mqttBroker, sizeof(config.mqttBroker));
  if (server.hasArg("mqttPort"))
    config.mqttPort = server.arg("mqttPort").toInt();
  if (server.hasArg("mqttUser"))
    server.arg("mqttUser").toCharArray(config.mqttUser, sizeof(config.mqttUser));
  if (server.hasArg("mqttPassword")) {
    String pwd = server.arg("mqttPassword");
    if (pwd.length() > 0) pwd.toCharArray(config.mqttPassword, sizeof(config.mqttPassword));
  }
  if (server.hasArg("mqttClientId")) {
    String cid = server.arg("mqttClientId");
    if (cid.length() > 0 && cid.length() < sizeof(config.mqttClientId)) {
      cid.toCharArray(config.mqttClientId, sizeof(config.mqttClientId));
    } else if (cid.length() == 0) {
      config.mqttClientId[0] = '\0';
    }
  }
  #endif


  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  if (server.hasArg("sensorInterval"))
    config.sensorInterval = server.arg("sensorInterval").toInt();
  #endif
  
  #if DEVICE_TYPE == 1
  if (server.hasArg("lowTemp")) {
    config.lowTemp = server.arg("lowTemp").toFloat();
  }
  
  if (server.hasArg("highTemp")) {
    config.highTemp = server.arg("highTemp").toFloat();
  }
  
  if (server.hasArg("lowHum")) {
    config.lowHum = server.arg("lowHum").toFloat();
  }
  
  if (server.hasArg("highHum")) {
    config.highHum = server.arg("highHum").toFloat();
  }
  
  if (server.hasArg("maxOnTime")) {
    config.maxOnTime = server.arg("maxOnTime").toInt();
  }
  
  if (server.hasArg("delaySeconds")) {
    config.delaySeconds = server.arg("delaySeconds").toInt();
  }
  
  if (server.hasArg("speedPercent")) {
    config.speedPercent = server.arg("speedPercent").toInt();
  }    
  
  config.adaptiveMode = server.hasArg("adaptiveMode");
  config.bootState = server.hasArg("bootState");
  config.sensorControlMode = server.hasArg("sensorControlMode");

  #endif
  
  #if DEVICE_TYPE == 3
  if (server.hasArg("maxOnTime")) {
    config.maxOnTime = server.arg("maxOnTime").toInt();
  }
  if (server.hasArg("delaySeconds")) {
    config.delaySeconds = server.arg("delaySeconds").toInt();
  }
  //@deprecated
  // if (server.hasArg("speedPercent")) {
  //   config.speedPercent = server.arg("speedPercent").toInt();
  // }  
  
  config.bootState = server.hasArg("bootState");
  
  #endif
  
  if (!config_validate()) {
    server.send(200, "text/html", web_getConfigPage(configLastError));
    return;
  }
  
  config_write();
  server.send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'></head><body><div style='text-align:center;margin-top:50px;'><h2>Настройки сохранены!</h2><p>Перезагрузка...</p></div></body></html>");
  delay(1000);
  #if DEBUG_ENABLED == 1
    Serial.println("[DEBUG] Restarting...");
  #endif
  ESP.restart();
}
#endif

void web_init() {
  int refreshInterval = 5;

  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  // unsigned long elapsed = (millis() - lastSensorRead) / 1000;
  // int timeUntilNextRead = config.sensorInterval - elapsed;
  // refreshInterval = (timeUntilNextRead > 2) ? timeUntilNextRead : 2;
  // if (refreshInterval > 10) refreshInterval = 10;
  
  refreshInterval = config.sensorInterval;

  #endif
  
  #if WEB_STATUS_ENABLED==1
    #ifdef ESP32
      server.on("/", HTTP_GET, [refreshInterval](AsyncWebServerRequest *request){ 
        request->send(200, "text/html", web_getStatusPage(refreshInterval)); 
      });
    #elif defined(ESP8266)
      server.on("/", [refreshInterval](){ 
        server.send(200, "text/html", web_getStatusPage(refreshInterval)); 
      });
    #endif
  #else
    #ifdef ESP32
      server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ 
        request->redirect("/config"); 
      });
    #elif defined(ESP8266)
      server.on("/", [](){ 
        server.sendHeader("Location", "/config", true); 
        server.send(302, "text/plain", ""); 
      });
    #endif
  #endif
  
  #ifdef ESP32
    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){ 
      request->send(200, "text/html", web_getConfigPage("")); 
    });
    
    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){ 
      web_saveConfig(request); 
    });
    
    #if WEB_RESET_ENABLED==1
    server.on("/resetall", HTTP_GET, [](AsyncWebServerRequest *request){
      config_clear();
      request->send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'></head><body><h2>Настройки сброшены, перезагрузка...</h2></body></html>");
      delay(1000);
      #if DEBUG_ENABLED == 1
        Serial.println("[DEBUG] Restarting...");
      #endif
      ESP.restart();
    });
    #endif
    
  #elif defined(ESP8266)
    server.on("/config", [](){ 
      server.send(200, "text/html", web_getConfigPage("")); 
    });
    
    server.on("/save", web_saveConfig);
    
    #if WEB_RESET_ENABLED==1
    server.on("/resetall", [](){
      config_clear();
      server.send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta http-equiv='refresh' content='5;url=/'></head><body><h2>Настройки сброшены, перезагрузка...</h2></body></html>");
      delay(1000);
      #if DEBUG_ENABLED == 1
      Serial.println("[DEBUG] Restarting...");
    #endif
      ESP.restart();
    });
    #endif
  #endif
  
  #if DEVICE_TYPE == 1 || DEVICE_TYPE == 3
    #ifdef ESP32
      #if DEVICE_TYPE == 1
      server.on("/fan/toggle", HTTP_GET, [](AsyncWebServerRequest *request){ 
        handleToggle(); 
        request->redirect("/"); 
      });
      server.on("/fan/auto", HTTP_GET, [](AsyncWebServerRequest *request){ 
        handleSensorControlMode(); 
        request->redirect("/"); 
      });
      #elif DEVICE_TYPE == 3
      server.on("/switch/toggle", HTTP_GET, [](AsyncWebServerRequest *request){ 
        handleToggle(); 
        request->redirect("/"); 
      });
      // server.on("/switch/auto", HTTP_GET, [](AsyncWebServerRequest *request){ 
      //   handleSensorControlMode(); 
      //   request->redirect("/"); 
      // });
      #endif
    #elif defined(ESP8266)
      #if DEVICE_TYPE == 1
      server.on("/fan/toggle", [](){ 
        handleToggle(); 
        server.sendHeader("Location", "/", true); 
        server.send(302, "text/plain", ""); 
      });
      server.on("/fan/auto", [](){ 
        handleSensorControlMode(); 
        server.sendHeader("Location", "/", true); 
        server.send(302, "text/plain", ""); 
      });
      #elif DEVICE_TYPE == 3
      server.on("/switch/toggle", [](){ 
        handleToggle(); 
        server.sendHeader("Location", "/", true); 
        server.send(302, "text/plain", ""); 
      });
      server.on("/switch/auto", [](){ 
        handleSensorControlMode(); 
        server.sendHeader("Location", "/", true); 
        server.send(302, "text/plain", ""); 
      });
      #endif
    #endif
  #endif
  

  #if OTA_ENABLED == 1
    #if defined(ESP32)
      ElegantOTA.begin(&server);
      
      #if LOG_OTA == 1
        Serial.println("[OTA] ElegantOTA initialized for ESP32");
      #endif
    
    #elif defined(ESP8266)
      if (!otaInitialized) {
        
        #if LOG_OTA == 1
          Serial.printf("[OTA] Free heap before ElegantOTA: %d\n", ESP.getFreeHeap());
        #endif
        
        ElegantOTA.begin(&server);
        otaInitialized = true;
        
        #if LOG_OTA == 1
          Serial.println("[OTA] ElegantOTA initialized for ESP8266");
        #endif  
      }
    #endif
  #endif
  
  server.begin();
  
  #if LOG_WEB == 1
    Serial.println("[WEB] Web server started on port 80 (client mode)");
      
  #endif
}

void web_initAP() {
  if (apMode) return;
  
  apMode = true;


  uint8_t mac[6];
  WiFi.macAddress(mac);
  
  
  
  WiFi.mode(WIFI_AP);
  
  #ifdef ESP8266
    // WiFi.softAPConfig(IPAddress(192,168,4,1), IPAddress(192,168,4,1), IPAddress(255,255,255,0));
    IPAddress apIP;
    apIP.fromString(AP_IP_ADDRESS);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));

    
    WiFi.softAP(deviceId); 
    dnsServer.start(DNS_PORT, "*", IPAddress(192,168,4,1));
    
    #if LOG_AP == 1
      Serial.printf("[AP] AP started: %s, IP: %s\n", deviceId, AP_IP_ADDRESS);
    #endif
  
  #elif defined(ESP32)    
    WiFi.softAP(deviceId);
    
    #if LOG_AP == 1
      Serial.printf("[AP] AP started: %s, IP: %s\n", deviceId, WiFi.softAPIP().toString().c_str());
    #endif
  
  #endif
  

  #if OTA_ENABLED == 1
    #if defined(ESP32)
      ElegantOTA.begin(&server);
      #if LOG_OTA == 1
        Serial.println("[OTA] ElegantOTA initialized for ESP32 (AP mode)");
      #endif
    #elif defined(ESP8266)
      #if LOG_OTA == 1
          Serial.printf("[OTA] Free heap before ElegantOTA: %d\n", ESP.getFreeHeap());
        #endif
      ElegantOTA.begin(&server);
      #if LOG_OTA == 1
        Serial.println("[OTA] ElegantOTA initialized for ESP8266 (AP mode)");
      #endif
    #endif
  #endif

  #ifdef ESP8266
    server.on("/", [](){ server.send(200, "text/html", web_getConfigPage("")); });
    server.on("/save", web_saveConfig);
  #elif defined(ESP32)
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ request->send(200, "text/html", web_getConfigPage("")); });
    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){ web_saveConfig(request); });
  #endif
  
  server.begin();
}

void web_update() {
  #ifdef ESP8266
    server.handleClient();   
  #endif
  
  #ifdef ESP8266
    if (apMode) {
      dnsServer.processNextRequest();
    }
  #endif
}