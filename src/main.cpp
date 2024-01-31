// Платформа
#ifdef ESP8266
#elif defined(ESP32)
#else
#error "Неизвестная платформа. Приложение расчитано на устройства ESP8266 или ESP32."
#endif

/* Здесь задается тип устройства
* 1     - Выключатель управляемый по протоколу MQTT, состоянием датчика температуры/влажности или по таймеру
* 2     - Датчик температуры/влажности отправляющий показания MQTT брокеру
* 3     - Выключатель управляемый по протоколу MQTT
*/
#define DEVICE_TYPE 2

/* Здесь задаются дополнительные параметры 
* DEBUG_ENABLE    - Включить режим отладки (вывод сообщений в консоль)
* OTA_ENABLE      - Возможность обновления прошивки по воздуху
* SENSOR_TYPE     - Тип используемого датчика (если применимо)
* SWITCH_PIN      - Управляющий выход для выключателя или реле (если применимо)
*/
#define DEBUG_ENABLE
#define SWITCH_PIN    4
#define OTA_ENABLE
/* OTA WARNING: 
Для исключения ошибок при компиляции в platformIo.ini:
```
lib_deps = 	
  me-no-dev/ESP Async WebServer @ ^1.2.3 на https://github.com/me-no-dev/ESPAsyncWebServer.git

build_flags=-DELEGANTOTA_USE_ASYNC_WEBSERVER=1
```
*/  

/* Здесь задается тип датчика
* 1     - AHT10 
* 2     - DHT11 + SENSOR_PIN
* 3     - TODO
*/
#define SENSOR_TYPE   1
#define SENSOR_PIN    0

////////////////////////////////////////////////////////////////
// Зависимости
////////////////////////////////////////////////////////////////
#if DEVICE_TYPE == 1
#define FAN_CONTROL_FEATURE_ENABLE
#endif

#if DEVICE_TYPE == 2
#define SENSOR_TEMP_HUM_ENABLE
#ifndef SENSOR_TYPE
#error "Не указан тип датчика"
#endif
#endif

#if DEVICE_TYPE == 3
#define SWITCH_FEATURE_ENABLE
#endif

// Обновление прошивки по воздуху
#ifdef OTA_ENABLE
#ifndef WIFI_FEATURE_ENABLE
#define WIFI_FEATURE_ENABLE
#endif
#endif

// Выключатель
#ifdef SWITCH_FEATURE_ENABLE

#ifndef WIFI_FEATURE_ENABLE
#define WIFI_FEATURE_ENABLE
#endif

#ifndef MQTT_FEATURE_ENABLE
#define MQTT_FEATURE_ENABLE
#endif

#endif

// Вентилятор
#ifdef FAN_CONTROL_FEATURE_ENABLE

#ifndef WIFI_FEATURE_ENABLE
#define WIFI_FEATURE_ENABLE
#endif

#ifndef MQTT_FEATURE_ENABLE
#define MQTT_FEATURE_ENABLE
#endif

#ifndef SENSOR_TEMP_HUM_ENABLE
#define SENSOR_TEMP_HUM_ENABLE
#endif

#ifndef SWITCH_FEATURE_ENABLE
#define SWITCH_FEATURE_ENABLE
#endif

#endif

// Датчик температуры/влажности
#ifdef SENSOR_TEMP_HUM_ENABLE

#ifndef WIFI_FEATURE_ENABLE
#define WIFI_FEATURE_ENABLE
#endif

#ifndef MQTT_FEATURE_ENABLE
#define MQTT_FEATURE_ENABLE
#endif

#endif

////////////////////////////////////////////////////////////////
// Объявление зависимостей, глобальных переменных и классов
////////////////////////////////////////////////////////////////
// #include <Arduino.h>
#include "credentials.h"

// Соединение WIFI
#ifdef WIFI_FEATURE_ENABLE

#ifdef ESP8266
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

byte mac[6];
const char* ssid = SSID_NAME;
const char* wifiPassword = WIFI_PASSWORD;

unsigned long lastWiFiCheckTime = 0;
const unsigned long wifiCheckInterval = 10000;          // Проверка WiFi каждые 10 секунд 
bool needShowConnectionInfo = true;                     // Показывать сообщение о подключении только один раз

/**
 * @brief Проверка соединения и повторное подключение в случае потери соединения
 */
void checkWiFiConnection() {
  unsigned long currentTime = millis();
  if (currentTime - lastWiFiCheckTime >= wifiCheckInterval) {
    lastWiFiCheckTime = currentTime;

    if (WiFi.status() != WL_CONNECTED) {
      needShowConnectionInfo = true;

      #ifdef DEBUG_ENABLE
      printf("Выполняю попытку соединиться с сетью WiFi...\n");
      #endif

      WiFi.disconnect();
      WiFi.reconnect();
    } else {
      if(needShowConnectionInfo){
        Serial.print("WiFi соединение установлено, IP адрес: ");
        Serial.println(WiFi.localIP()); needShowConnectionInfo = false;
      }
    }
  }
}

#endif

// Обновление OTA
#ifdef OTA_ENABLE

#if defined(ESP8266)
  #include <ESPAsyncTCP.h>
#elif defined(ESP32)
  #include <AsyncTCP.h>
#endif

#include <ESPAsyncWebServer.h>
#include <ElegantOTA.h>

AsyncWebServer server(80);
unsigned long ota_progress_millis = 0;



void onOTAStart() {
#ifdef DEBUG_ENABLE
  Serial.println("Запущено обновление по воздуху");
#endif
}

void onOTAProgress(size_t current, size_t final) {
  
  if (millis() - ota_progress_millis > 1000) {
    ota_progress_millis = millis();
    Serial.printf("Обновление: %u байт, из: %u \n", current, final);
  }
}

void onOTAEnd(bool success) {
  // Log when OTA has finished
  if (success) {
    Serial.println("Обновление окончено");
  } else {
    Serial.println("Ошибка обновления!");
  }
  
}


#endif

// Канал для брокера
#ifdef MQTT_FEATURE_ENABLE

WiFiClient espClient;
#include <PubSubClient.h>
PubSubClient client(espClient);
const char mqttServer[] = MQTT_ADDRESS;
const char mqttUser[] = MQTT_USER;
const char mqttPassword[] = MQTT_PASSWORD;
char mqttId[18];
char lastWillTopic[25];

// Разбор сообщений
String byteToString(byte* payload, unsigned int length) {
  String result;
  for (unsigned int i = 0; i < length; i++) {
    result += (char)payload[i];
  };
  return result;
}

// Проверка подключения к брокеру
void checkMqttConnection() {
  const char* lastWillMessage = "offline";
  
  if (!client.connected()) {
    #ifdef DEBUG_ENABLE
    printf("Попытка подключиться к mqtt брокеру по адресу %s с идентификатором %s.\n",mqttServer, mqttId);
    #endif

    if (client.connect(mqttId, mqttUser, mqttPassword, lastWillTopic, 1, true, lastWillMessage)) {
      #ifdef DEBUG_ENABLE
      printf("Подключение к mqtt брокеру установлено.\n");
      #endif

      // Обновить статус канала
      client.publish(lastWillTopic, "online");

    } else {
      #ifdef DEBUG_ENABLE
      printf("Подключение к mqtt брокеру не установлено ( rc=%d).\n", client.state());
      printf("Повторная попытка через 5 секунд\n");
      #endif
      
      delay(5000);
    };
  } else {
    client.loop();
  }
}

#endif

// Обработка команды конфигурации для управляемого вентилятора
#ifdef FAN_CONTROL_FEATURE_ENABLE

#define HUMIDITY_LOW_RANGE      60
#define HUMIDITY_HIGH_RANGE     70
#define TEMPERATURE_LOW_RANGE   27
#define TEMPERATURE_HIGH_RANGE  29
#define FAN_TIME_DELAY          30

#endif

// Выключатель
#ifdef SWITCH_FEATURE_ENABLE

// Топики для выключателя
char switchStateTopic[31];
char switchControlTopic[27];

#ifndef SWITCH_ON_VALUE
#define SWITCH_ON_VALUE "ON"
#endif

#ifndef SWITCH_OFF_VALUE
#define SWITCH_OFF_VALUE "OFF"
#endif

void publishSwitchState() {
  boolean switchState = digitalRead(SWITCH_PIN);
  if (switchState) {
    client.publish(switchStateTopic, SWITCH_ON_VALUE);
  } else {
    client.publish(switchStateTopic, SWITCH_OFF_VALUE);
  }
  client.subscribe(switchControlTopic);  // Подписаться на получение команд
}

void callbackSwitchCommand(char* topic, byte* payload, unsigned int length){
  String recievedPayload;
  recievedPayload = byteToString(payload, length);
  if (recievedPayload == "1" || recievedPayload == SWITCH_ON_VALUE) {
    digitalWrite(SWITCH_PIN, LOW);
    //client.publish(switchStateTopic, SWITCH_ON_VALUE);
  } else if (recievedPayload == "0" || recievedPayload == SWITCH_OFF_VALUE) {
    digitalWrite(SWITCH_PIN, HIGH);
    //client.publish(switchStateTopic, SWITCH_OFF_VALUE);
  };
  publishSwitchState();
}

#endif

// Хранение настроек вентилятора в EEPROM
#ifdef EEPROM_FEATURE_ENABLE

#include <EEPROM.h>

// Структура данных для хранения параметров управляемого вентилятора
struct Params { 
  double lowHum; 
  double highHum; 
  double lowTemp; 
  double highTemp;
  int delaySeconds;

  Params(double lowHum, double highHum, double lowTemp, double highTemp, int delaySeconds) : lowHum(lowHum), highHum(highHum), lowTemp(lowTemp), highTemp(highTemp), delaySeconds(delaySeconds) {}

  Params() : lowHum(0.0), highHum(0.0), lowTemp(0.0), highTemp(0.0), delaySeconds(0) {} 

};

// Обьект с конфигурацией
class Config {

private:
  Params params;

public:
  
  Params get(){
    return params;
  }

  void set(Params value) {     
    params.lowHum = value.lowHum;
    params.highHum = value.highHum;
    params.lowTemp = value.lowTemp;
    params.highTemp = value.highTemp; 
    params.delaySeconds = value.delaySeconds;
  }

  void read(){
    EEPROM.begin(sizeof(params));
    EEPROM.get(0, params);
  }

  void write(){
    EEPROM.begin(sizeof(params));
    EEPROM.put(0, params);
    EEPROM.commit();
  }
  
  bool isValid() {
    if ( isnan(params.lowHum) || isnan(params.highHum) || isnan(params.lowTemp) || isnan(params.highTemp) || isnan(params.delaySeconds) ) {
      return false;
    }
    // Убеждаемся, что значения были инициализированы ненулевыми значениями
    if ( params.lowHum == 0  &&  params.highHum == 0  && params.lowTemp ==0 && params.highTemp == 0 ) {
      return false;
    }

    // Проверяем, что значения находятся в допустимом диапазоне
    if (params.lowHum < 0 || params.lowHum > 100 ) {
      return false;
    }
    
    if (params.highHum < 0 || params.highHum > 100 ) {
      return false;
    }
    
    if (params.lowTemp < -50 || params.lowTemp > 50 ) {
      return false;
    }
    
    if (params.highTemp < -50 || params.highTemp > 50 ) {
      return false;
    }

    if (params.delaySeconds < 0  ) {
      return false;
    }
    
    return true;
  }

  Config(Params newdata) { params = newdata;}

  Config() {}

};

Config deviceConfig;

#endif

// Датчик температуры/влажности
#ifdef SENSOR_TEMP_HUM_ENABLE
bool sensor_temperature_found = false;
double temperature, humidity;

// Топики для температуры/влажности
char temperatureStateTopic[37];
char humidityStateTopic[34];

void publishSensorState() {
  client.publish(temperatureStateTopic, String(temperature).c_str());
  client.publish(humidityStateTopic, String(humidity).c_str());
}

// Реализация датчика для AHT10
#if SENSOR_TYPE == 1

#ifndef SENSOR_DURATION
#define SENSOR_DURATION 10
#endif

#include <Adafruit_AHTX0.h>

class Sensor {
 private:
  Adafruit_AHTX0 tempHumSensor;
  sensors_event_t humidity, temperature;
  int lastRead;
  double lastTemp, lastHum;
  
  void readData() {
    if (millis() - lastRead >= SENSOR_DURATION * 1000 || lastRead == 0) {
      tempHumSensor.getEvent(&humidity, &temperature);
      lastRead = millis();
      lastHum = humidity.relative_humidity;
      lastTemp = temperature.temperature;

      #ifdef DEBUG_ENABLE
      Serial.println("Чтение показаний датчика");Serial.println();
      #endif
    }
  }

 public:
  bool initialization() {
    lastRead = 0;
    return tempHumSensor.begin();
  }

  double getTemperature() {
    readData();
    return lastTemp;
  }

  double getHumidity() {
    readData();
    return lastHum;
  }
};

#endif

Sensor tempHumSensor;

#endif


////////////////////////////////////////////////////////////////
// методы 
////////////////////////////////////////////////////////////////

void setup() {


#ifdef DEBUG_ENABLE
  Serial.begin(115200); 
  Serial.println();
  delay(5000);
  Serial.println("Выполняется инициализация устройства ... ");
#endif

#ifdef EEPROM_FEATURE_ENABLE

  deviceConfig.read();
  if(!deviceConfig.isValid()){
    Params newParams( HUMIDITY_LOW_RANGE, HUMIDITY_HIGH_RANGE, TEMPERATURE_LOW_RANGE, TEMPERATURE_HIGH_RANGE, FAN_TIME_DELAY);
    deviceConfig.set(newParams ) ;
  };

#endif

#ifdef WIFI_FEATURE_ENABLE
  
  #ifdef DEBUG_ENABLE
    printf("Инициализия соединения WiFi.\n");
  #endif
  
  // WiFi.mode(WIFI_STA);  
  WiFi.begin(ssid, wifiPassword);
    
  #ifdef DEBUG_ENABLE
    WiFi.macAddress(mac);
    Serial.print("MAC адрес: ");
    Serial.print(mac[0],HEX);
    Serial.print(":");
    Serial.print(mac[1],HEX);
    Serial.print(":");
    Serial.print(mac[2],HEX);
    Serial.print(":");
    Serial.print(mac[3],HEX);
    Serial.print(":");
    Serial.print(mac[4],HEX);
    Serial.print(":");
    Serial.println(mac[5],HEX);     
  #endif
  
#endif

#ifdef OTA_ENABLE  
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "<html><body><a href='/update'>Страница обновления</a></body></html>");
  });

  ElegantOTA.begin(&server);    
  ElegantOTA.onStart(onOTAStart);
  ElegantOTA.onProgress(onOTAProgress);
  ElegantOTA.onEnd(onOTAEnd);
  server.begin();
  #ifdef DEBUG_ENABLE
  Serial.println("HTTP сервер доступен по адресу ");
  #endif
#endif

#ifdef MQTT_FEATURE_ENABLE

  client.setServer(mqttServer, 1883);
  snprintf(mqttId, sizeof(mqttId), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  snprintf(lastWillTopic, sizeof(lastWillTopic), "%s/status", mqttId);

#endif

#ifdef SWITCH_FEATURE_ENABLE
  digitalWrite(SWITCH_PIN, HIGH);
  snprintf(switchStateTopic, sizeof(switchStateTopic), "%s/switch/state", mqttId);
  snprintf(switchControlTopic, sizeof(switchControlTopic), "%s/switch/c", mqttId);
  client.setCallback(callbackSwitchCommand);
#endif

#ifdef SENSOR_TEMP_HUM_ENABLE
  snprintf(temperatureStateTopic, sizeof(temperatureStateTopic), "%s/sensor/temperature", mqttId);
  snprintf(humidityStateTopic, sizeof(humidityStateTopic), "%s/sensor/humidity", mqttId);
  
  #ifdef DEBUG_ENABLE
  Serial.print("Инициализация датчика температуры и влажности: ");
  #endif

  if(tempHumSensor.initialization()){
    sensor_temperature_found = true;
    
    #ifdef DEBUG_ENABLE
    Serial.println("успешно");        
    Serial.printf("Текущие показания датчика: температура %2.2f ºC, влажность %2.2f%%\n", tempHumSensor.getTemperature(),tempHumSensor.getHumidity());
    #endif

  } else {
    sensor_temperature_found = false;
    
    #ifdef DEBUG_ENABLE
    Serial.println("датчик не найден");
    #endif  
  
  };
#endif

#ifdef DEBUG_ENABLE
printf("Инициализация устройства окончена.\n");
#endif
  
}

void loop() {
    #ifdef WIFI_FEATURE_ENABLE   
    checkWiFiConnection();   
    #endif

    #ifdef OTA_ENABLE
    ElegantOTA.loop();
    #endif

    #ifdef MQTT_FEATURE_ENABLE  
    if (WiFi.status() == WL_CONNECTED) {
      checkMqttConnection(); 
    }
    #endif

    #ifdef SENSOR_TEMP_HUM_ENABLE
    if(sensor_temperature_found){
      double curTemperature = tempHumSensor.getTemperature();
      double curHumidity = tempHumSensor.getHumidity();
      // Опубликовать показания датчика при изменении
      if(!(curTemperature == temperature || curHumidity == humidity)) {
        
        #ifdef DEBUG_ENABLE
        Serial.printf("Temperature: %.2fC, Humidity: %.2f%% \n", curTemperature, curHumidity);
        #endif

        temperature = curTemperature;
        humidity = curHumidity;
        publishSensorState();
        
        #ifdef DEBUG_ENABLE
        Serial.println("Показания датчика опубликованы");
        #endif
      };
    }
    #endif 
}
