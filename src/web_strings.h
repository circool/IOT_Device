#ifndef WEB_STRINGS_H
#define WEB_STRINGS_H

#include <Arduino.h>

// HTML заголовок
const char HTML_DOCTYPE[] PROGMEM = "<!DOCTYPE html><html><head><meta charset='UTF-8'>";
const char HTML_VIEWPORT[] PROGMEM = "<meta name='viewport' content='width=device-width, initial-scale=1'>";
const char HTML_STYLE[] PROGMEM = 
  "<style>"
  "body{font-family:Arial;margin:20px;background:#f0f0f0;}"
  ".container{max-width:700px;margin:auto;background:white;padding:20px;border-radius:10px;}"
  "h1{color:#2c3e50;}h3{color:#2c3e50;border-bottom:1px solid #ccc;padding-bottom:5px;}"
  "label{display:block;margin-top:10px;font-weight:bold;}"
  "input[type=text],input[type=password],input[type=number]{width:100%;padding:8px;margin:5px 0;border:1px solid #ccc;border-radius:4px;font-size:1.2em;box-sizing:border-box;}"
  "input[type=checkbox]{width:20px;height:20px;margin-right:10px;vertical-align:middle;cursor:pointer;transform:scale(1.5);}"
  "input[type=submit]{background:#2c3e50;color:white;padding:10px 20px;margin-top:20px;border:none;border-radius:4px;cursor:pointer;width:100%;font-size:1em;box-sizing:border-box;}"
  "input[type=submit]:hover{background:#1a252f;}"
  ".info{background:#e7f3ff;padding:10px;border-radius:5px;margin:10px 0;}"
  ".warning{background:#fff3cd;padding:10px;border-radius:5px;margin:10px 0;color:#856404;}"
  ".error{background:#ffebee;padding:10px;border-radius:5px;margin:10px 0;color:#c62828;}"
  ".row{display:flex;gap:10px;}.row>div{flex:1;}"
  ".password-hint{color:#7f8c8d;margin-top:-2px;margin-bottom:8px;}"
  ".note{background:#f9f9f9;padding:8px;margin-top:10px;border-left:3px solid #2c3e50;font-size:0.9em;color:#555;}"
  ".link-btn{background:#555;color:white;padding:10px 20px;margin-top:10px;border:none;border-radius:4px;cursor:pointer;font-size:1em;text-align:center;text-decoration:none;display:block;box-sizing:border-box;}"
  ".link-btn:hover{background:#333;}"
  ".sensor-card{display:inline-block;width:45%;margin:10px;padding:15px;border-radius:10px;text-align:center;}"
  ".sensor-value{font-size:2em;font-weight:bold;}"
  ".sensor-label{margin-top:5px;}"
  ".status-card{padding:15px;border-radius:10px;text-align:center;margin:10px;}"
  ".flex-container{display:flex;flex-wrap:wrap;justify-content:center;}"
  ".button-group{display:flex;justify-content:center;gap:10px;margin-top:20px;flex-wrap:wrap;}"
  "a{text-decoration:none;}"
  ".sensor-error{background:#ffebee;padding:15px;border-radius:8px;margin:15px 10px;color:#c62828;text-align:center;border:2px solid #ef9a9a;}"
  ".duty-bar{background:#e0e0e0;border-radius:10px;margin:10px 0;height:20px;overflow:hidden;}"
  ".duty-fill{background:#2c3e50;height:100%;border-radius:10px;transition:width 0.3s;}"
  "</style>";

const char HTML_CONTAINER_OPEN[] PROGMEM = "</head><body><div class='container'>";
const char HTML_CONTAINER_CLOSE[] PROGMEM = "</div></body></html>";

const char HTML_FORM_OPEN[] PROGMEM = "<form method='POST' action='/save'>";
const char HTML_FORM_CLOSE[] PROGMEM = "</form>";

const char HTML_CHECKBOX_CONFIRM[] PROGMEM = "<label><input type='checkbox' name='confirmSave' required> Подтвердить сохранение</label>";
const char HTML_SUBMIT_BUTTON[] PROGMEM = "<input type='submit' value='Сохранить и перезагрузить'>";

// Секции
const char HTML_SECTION_NETWORK[] PROGMEM = "<h3>Настройки сети</h3>";
const char HTML_SECTION_MQTT[] PROGMEM = "<h3>MQTT настройки</h3>";
const char HTML_SECTION_SENSOR[] PROGMEM = "<h3>Настройки датчиков</h3>";
const char HTML_SECTION_CONTROL[] PROGMEM = "<h3>Управление</h3>";
const char HTML_SECTION_SILENT[] PROGMEM = "<h3>Тихий режим (ШИМ)</h3>";
const char HTML_SECTION_ADAPTIVE[] PROGMEM = "<h3>Адаптивный тихий режим</h3>";
const char HTML_SECTION_BOOT[] PROGMEM = "<h3>Поведение при старте</h3>";
const char HTML_SECTION_MODES[] PROGMEM = "<h3>Режимы работы</h3>";

// Лейблы
const char LABEL_WIFI_SSID[] PROGMEM = "<label>WiFi SSID:</label>";
const char LABEL_WIFI_PASSWORD[] PROGMEM = "<label>WiFi Password:</label>";
const char LABEL_MQTT_BROKER[] PROGMEM = "<label>MQTT Broker:</label>";
const char LABEL_MQTT_PORT[] PROGMEM = "<label>MQTT Port:</label>";
const char LABEL_MQTT_USER[] PROGMEM = "<label>MQTT User:</label>";
const char LABEL_MQTT_PASSWORD[] PROGMEM = "<label>MQTT Password:</label>";
const char LABEL_MQTT_CLIENT_ID[] PROGMEM = "<label>MQTT Client ID:</label>";
const char LABEL_LOW_TEMP[] PROGMEM = "<label>Low Temp (°C):</label>";
const char LABEL_HIGH_TEMP[] PROGMEM = "<label>High Temp (°C):</label>";
const char LABEL_LOW_HUM[] PROGMEM = "<label>Low Hum (%):</label>";
const char LABEL_HIGH_HUM[] PROGMEM = "<label>High Hum (%):</label>";
const char LABEL_SENSOR_INTERVAL[] PROGMEM = "<label>Интервал опроса датчика (сек)</label>";
const char LABEL_MAX_ON_TIME[] PROGMEM = "<label>Аварийное отключение через </label>";
const char LABEL_DELAY_SECONDS[] PROGMEM = "<label>Принудительно включить через </label>";
const char LABEL_SPEED_PERCENT[] PROGMEM = "<label>Скорость (0-100%):</label>";
const char LABEL_ADAPTIVE_MODE[] PROGMEM = "<label><input type='checkbox' name='adaptiveMode' value='1'> Включить адаптацию</label>";
const char LABEL_BOOT_STATE[] PROGMEM = "<label><input type='checkbox' name='bootState' value='1'> Включать при старте</label>";
const char LABEL_SENSOR_CONTROL_MODE[] PROGMEM = "<label><input type='checkbox' name='sensorControlMode' value='1'> Режим управления сенсором</label>";

// Подсказки
const char HINT_PASSWORD[] PROGMEM = "<div class='password-hint'>Оставьте пустым, чтобы сохранить текущий пароль</div>";
const char NOTE_SILENT_MODE[] PROGMEM = "<div class='note'>0% - выключено, 100% - полная мощность (тихий режим выключен).<br>При значении ниже 100% вентилятор работает тише.</div>";
const char NOTE_ADAPTIVE[] PROGMEM = "<div class='note'>Адаптивный режим автоматически регулирует скорость для поддержания температуры и влажности на уровне, зафиксированном при включении вентилятора.</div>";
const char NOTE_BOOT[] PROGMEM = "<div class='note'>При включенной опции вентилятор будет включен сразу после подачи питания.</div>";
const char NOTE_SENSOR_CONTROL[] PROGMEM = "<div class='note'>При включённом режиме вентилятор управляется по показаниям датчиков температуры и влажности. При выключении — только вручную.</div>";

// Строки состояния
const char STATUS_INFO_OPEN[] PROGMEM = "<div class='info'>";
const char STATUS_INFO_CLOSE[] PROGMEM = "</div>";
const char STATUS_MODE_CLIENT[] PROGMEM = "Режим: <strong>Клиент WiFi</strong><br>";
const char STATUS_MODE_AP[] PROGMEM = "Режим: <strong>Точка доступа (AP)</strong><br>";

#endif