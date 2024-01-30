/* Здесь задается тип устройства
* SENSOR_TEMP_HUM     - Датчик температуры/влажности отправляющий показания MQTT брокеру
* SWITCH              - Выключатель управляемый по протоколу MQTT
* FAN                 - Выключатель управляемый по протоколу MQTT или состоянием датчика температуры/влажности или по таймеру
*/
#define DEVICE_TYPE FAN

/* Здесь задаются параметры реализации
* DEBUG_ENABLE        - Включить режим отладки (вывод сообщений в консоль)
* OTA_ENABLED         - Возможность обновления прошивки по воздуху
* SENSOR_TYPE         - Тип используемого датчика (если применимо)
* SWITCH_PIN          - Управляющий выход для выключателя или реле (если применимо)
*/
#define DEBUG_ENABLE
#define OTA_ENABLE
#define SENSOR_TYPE AHT10
#define SWITCH_PIN  4

////////////////////////////////////////////////////////////////
// Функциональный состав устройства 
////////////////////////////////////////////////////////////////

#if DEVICE_TYPE == FAN

#define FAN_CONTROL_FEATURE_ENABLED

#ifndef EEPROM_FEATURE_ENABLED
#define EEPROM_FEATURE_ENABLED
#endif

#ifndef WIFI_FEATURE_ENABLED
#define WIFI_FEATURE_ENABLED
#endif

#endif

#if DEVICE_TYPE == SENSOR_TEMP_HUM
#define SENSOR_TEMP_HUM_ENABLED
#endif

#if DEVICE_TYPE == SWITCH
#define SWITCH_FEATURE_ENABLED
#endif

////////////////////////////////////////////////////////////////
// Зависимости
////////////////////////////////////////////////////////////////

// Устройство поддерживающее обновление прошивки по воздуху
#ifdef OTA_ENABLED
#ifndef WIFI_FEATURE_ENABLED
#define WIFI_FEATURE_ENABLED
#endif
#endif

// Выключатель управляемый по MQTT
#ifdef SWITCH_FEATURE_ENABLED

#ifndef WIFI_FEATURE_ENABLED
#define WIFI_FEATURE_ENABLED
#endif

#ifndef MQTT_FEATURE_ENABLED
#define MQTT_FEATURE_ENABLED
#endif

#endif

// Вентилятор
#ifdef FAN_CONTROL_FEATURE_ENABLED

#ifndef WIFI_FEATURE_ENABLED
#define WIFI_FEATURE_ENABLED
#endif

#ifndef MQTT_FEATURE_ENABLED
#define MQTT_FEATURE_ENABLED
#endif

#ifndef SENSOR_TEMP_HUM_ENABLED
#define SENSOR_TEMP_HUM_ENABLED
#endif

#ifndef SWITCH_FEATURE_ENABLED
#define SWITCH_FEATURE_ENABLED
#endif

#endif



#include <Arduino.h>

////////////////////////////////////////////////////////////////
// Объявление 
////////////////////////////////////////////////////////////////

// Соединение WIFI
#ifdef WIFI_FEATURE_ENABLED
#include "credentials.h"

#ifdef ESP8266
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#else
#error "Unsupported platform. Define ESP8266 or ESP32."
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

#include <AsyncElegantOTA.h>
#include <ESPAsyncWebServer.h>
AsyncWebServer webServer(80);

#ifdef ESP8266
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif

#ifdef ESP32
#include <AsyncTCP.h>
#include <WiFi.h>
#endif

#endif


// Канал для брокера
#ifdef MQTT_FEATURE_ENABLED
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
  // #ifdef DEBUG_FEATURE_ENABLED
  //     printf(
  //         "Попытка подключиться к mqtt брокеру по адресу %s с идентификатором "
  //         "%s.\n",
  //         mqttServer, mqttId);
  // #endif

    if (client.connect(mqttId, mqttUser, mqttPassword, lastWillTopic, 1, true, lastWillMessage)) {
    // #ifdef DEBUG_FEATURE_ENABLED
    //       printf("Подключение к mqtt брокеру установлено.\n");
    // #endif

      // Обновить статус канала
      client.publish(lastWillTopic, "online");

    } else {
      // #ifdef DEBUG_FEATURE_ENABLED
      //       printf("Подключение к mqtt брокеру не установлено ( rc=%d).\n",
      //             client.state());
      //       printf("Повторная попытка через 5 секунд\n");
      // #endif
      delay(5000);
    }
  } else {
    client.loop();
  }
}

#endif


// Обработка команды конфигурации для управляемого вентилятора
#ifdef FAN_CONTROL_FEATURE_ENABLED

#endif


// Выключатель
#ifdef SWITCH_FEATURE_ENABLED

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
#ifdef EEPROM_FEATURE_ENABLED

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

#endif


// Датчик температуры/влажности
#ifdef SENSOR_TEMP_HUM_ENABLED
bool sensor_temperature_found = false;
double temperature, humidity;

// Топики для температуры/влажности
char temperatureStateTopic[37];
char humidityStateTopic[34];

void publishSensorState() {
  client.publish(temperatureStateTopic, String(temperature).c_str());
  client.publish(humidityStateTopic, String(humidity).c_str());
}


#if SENSOR_TYPE == AHT10

#ifndef SENSOR_DURATION
#define SENSOR_DURATION 2
#endif

#include <Adafruit_AHTX0.h>

// Реализация датчика для AHT10
class Sensor {
 private:
  Adafruit_AHTX0 tempHumSensor;
  sensors_event_t humidity, temperature;
  int lastRead;

  void readData() {
    if (millis() - lastRead >= SENSOR_DURATION * 1000 || lastRead == 0) {
      tempHumSensor.getEvent(&humidity, &temperature);
      lastRead = millis();
    }
  }

 public:
  bool initialization() {
    lastRead = 0;
    return tempHumSensor.begin();
  }

  double getTemperature() {
    readData();
    return temperature.temperature;
  }

  double getHumidity() {
    readData();
    return humidity.relative_humidity;
  }
};

#endif

Sensor tempHumSensor;

#endif








  



////////////////////////////////////////////////////////////////
// Основной модуль 
////////////////////////////////////////////////////////////////

void setup() {


#ifdef DEBUG_ENABLE
  Serial.begin(115200); 
  Serial.println();
  delay(5000);
  Serial.println("Выполняется инициализация устройства ... ");
#endif

#ifdef EEPROM_FEATURE_ENABLED

  Config data;

#ifdef DEBUG_ENABLE
  Serial.printf("Конфигурация без ошибок? %s\n", data.isValid() ? "Да":"Нет");
  printf("Текущие значения: lowTemp=%f, highTemp=%f, lowHum=%f, highHum=%f\n", data.get().lowTemp, data.get().highTemp, data.get().lowHum, data.get().highHum);
  Serial.println("Читаю параметры из памяти");
  data.read();
  // printf("Текущие значения: lowTemp=%f, highTemp=%f, lowHum=%f, highHum=%f\n", data.params.lowTemp, data.params.highTemp, data.params.lowHum, data.params.highHum);
  Serial.printf("Конфигурация без ошибок? %s\n", data.isValid() ? "Да":"Нет");
  Serial.println("New config");
  // Params param2 (1,1,1,1);
  // Config data2(param2);
  // printf("Текущие значения: lowTemp=%f, highTemp=%f, lowHum=%f, highHum=%f\n", data2.get().lowTemp, data2.get().highTemp, data2.get().lowHum, data2.get().highHum);
  // printf("Текущие значения: lowTemp=%f, highTemp=%f, lowHum=%f, highHum=%f\n", data.params.lowTemp, data.params.highTemp, data.params.lowHum, data.params.highHum);

  // Serial.println("Читаю параметры из памяти");
  // data.read();
  // printf("Текущие значения: lowTemp=%f, highTemp=%f, lowHum=%f, highHum=%f\n", data.params.lowTemp, data.params.highTemp, data.params.lowHum, data.params.highHum);

  // Serial.printf("Конфигурация без ошибок? %s\n", data2.isValid() ? "Да":"Нет");



#endif

#endif












#ifdef DEBUG_ENABLE
  printf("Инициализация устройства окончена.\n");
#endif
  
}

void loop() {
    #ifdef WIFI_FEATURE_ENABLED   
      checkWiFiConnection();   

      #ifdef MQTT_FEATURE_ENABLED
        if (WiFi.status() == WL_CONNECTED) {
          checkMqttConnection(); 
        }
      
        #ifdef SENSOR_TEMP_HUM_ENABLED
          if(sensor_temperature_found){
            double curTemperature = tempHumSensor.getTemperature();
            double curHumidity = tempHumSensor.getHumidity();
            if(!(curTemperature == temperature) || (curHumidity == humidity)) {
              
              #ifdef DEBUG_ENABLE
                Serial.printf("Temperature: %2.2fC, Humidity: %2.2f%% \r", curTemperature, curHumidity);
              #endif

              temperature = curTemperature;
              humidity = curHumidity;
              publishSensorState();
            };
          }
        #endif 

      #endif

    #endif

    
    
    
       

    
}
