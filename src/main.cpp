#include <Arduino.h>

// Проверка на корректность платформы 
#ifdef ESP8266
#elif defined(ESP32)
#else
  #error "Неизвестная платформа. Приложение расчитано на устройства ESP8266 или ESP32."
#endif

/* Здесь задаются параметры отладки (DEBUG_ENABLE, DEBUG_MQTT, DEBUG_EEPROM)
* DEBUG_ENABLE    - Включить режим отладки (вывод сообщений в консоль)
* DEBUG_MQTT      - Включить режим отладки (вывод сообщений в консоль)
* DEBUG_EEPROM    - Включить режим отладки (не записывать данные в память EEPROM)
*/
#define DEBUG_ENABLE
#define DEBUG_MQTT
#define DEBUG_EEPROM

/* Здесь задаются функциональные возможности (EEPROM_FEATURE_ENABLE, OTA_ENABLE)
* OTA_ENABLE      - Возможность обновления прошивки по воздуху
* EEPROM_FEATURE_ENABLE - Использование EEPROM для хранения настроек
*/
#define EEPROM_FEATURE_ENABLE
#define OTA_ENABLE

/* Здесь задается тип устройства
* 1     - Выключатель, управляемый датчиками температуры/влажности.
* 2     - Датчик температуры/влажности, транслирующий показания по протоколу MQTT.
* 3     - Выключатель, управляемый по протоколу MQTT
*/
#define DEVICE_TYPE 1

// Место установки: ванная комната (1) или туалет (2)
#define ROOM_TYPE 1


/** Здесь задаются константы устройства 
 * -----------------------------------------
 * */ 

// Для выключателя или вентилятора
/* Здесь задается пин к которому подключен выключатель или вентилятор */
#ifdef ESP8266
  #define SWITCH_PIN    14
#endif

#ifdef ESP32
  // Так вышло, что на макете ESP32 был выбран пин 4. В продакшене будем использовать единый пин (14).
  #define SWITCH_PIN    4
#endif

/* Здесь задается первоначальное состояние выключателя или вентилятора 
* Если эта константа определена, то при инициализации выключателя или вентилятора он будет выключен
* Если эта константа не определена, то при инициализации выключателя или вентилятора он будет в неопределенном состоянии, в зависимости от типа GPIO
*/
#define SET_SWITCH_OFF   // Если опрелелено, будем выключать реле при инициализации

// Для датчика температуры/влажности
/* Здесь задается тип датчика и пин к которому он подключен
* 1     - AHT10 (Управляемый по I2C)
* 2     - DHT11 + SENSOR_PIN (Управляемый по GPIO)
* 3     - TODO
*/
#define SENSOR_TYPE   1
//#define SENSOR_PIN    0

/** 
 * Здесь задаются функциональные блоки, необходимые для реализации функционала для выбранного типа устройства и проверяется наличие необходимых констант
 */ 

// Выключатель вентилятора управляемый таймером и/или показаниям датчиков
#if DEVICE_TYPE == 1
  
  #ifndef SWITCH_PIN
    #error "Не указан пин управления выключателем"
  #endif
  
  #define FAN_CONTROL_FEATURE_ENABLE
  
  #ifndef DEVICE_DESC
    #define DEVICE_DESC "Выключатель, управляемый таймером или датчиками температуры/влажности."
  #endif

#endif

// Сенсор температуры/влажности
#if DEVICE_TYPE == 2
  
  #ifndef SENSOR_TYPE
    #error "Не указан тип датчика"
  #endif

  #if SENSOR_TYPE == 2
    #ifndef SENSOR_PIN
      #error "Не указан пин датчика"
    #endif  
  #endif
 
  #define SENSOR_TEMP_HUM_ENABLE
  #ifndef DEVICE_DESC
    #define DEVICE_DESC "Датчик температуры/влажности, транслирующий показания по протоколу MQTT."
  #endif

#endif

// Управляемый выключатель
#if DEVICE_TYPE == 3
  
  #ifndef SWITCH_PIN
    #error "Не указан пин управления выключателем"
  #endif

  #define SWITCH_FEATURE_ENABLE

  #ifndef DEVICE_DESC
    #define DEVICE_DESC "Выключатель, управляемый по протоколу MQTT."
  #endif

#endif


/** Разные константы */

// Здесь задается описание чипа
#ifdef ESP8266
  #define CHIP_DESC     "ESP8266"
#endif

#ifdef ESP32
  #define CHIP_DESC     "ESP32"
#endif

/* OTA WARNING: 
Для исключения ошибок при компиляции в platformIo.ini:
```
lib_deps = 	
  me-no-dev/ESP Async WebServer @ ^1.2.3 на https://github.com/me-no-dev/ESPAsyncWebServer.git

build_flags=-DELEGANTOTA_USE_ASYNC_WEBSERVER=1
```
*/  


////////////////////////////////////////////////////////////////
// Определение зависимых функциональных блоков
////////////////////////////////////////////////////////////////

// Обновление прошивки по воздуху
#ifdef OTA_ENABLE
  #ifndef WIFI_FEATURE_ENABLE
    #define WIFI_FEATURE_ENABLE
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

// Выключатель
#ifdef SWITCH_FEATURE_ENABLE

  #ifndef SWITCH_ON_VALUE
    #define SWITCH_ON_VALUE "ON"
    #define SWITCH_ON_LEVEL HIGH
  #endif

  #ifndef SWITCH_OFF_VALUE
    #define SWITCH_OFF_VALUE "OFF"
    #define SWITCH_OFF_LEVEL LOW
  #endif

  #ifndef WIFI_FEATURE_ENABLE
    #define WIFI_FEATURE_ENABLE
  #endif

  #ifndef MQTT_FEATURE_ENABLE
    #define MQTT_FEATURE_ENABLE
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
// Объявление методов, глобальных переменных и классов
////////////////////////////////////////////////////////////////

// Глобальные переменные, классы и методы
#include "credentials.h"
#include "main.h"

// Датчик температуры/влажности
#ifdef SENSOR_TEMP_HUM_ENABLE
  
  // Наличие (исправность) датчика температуры/влажности
  bool sensor_found = false;

  // Реализация класса Sensor для датчика для AHT10
  #if SENSOR_TYPE == 1
    
    // Частота опроса датчика
    #ifndef SENSOR_DURATION
      #define SENSOR_DURATION 10
    #endif

    #include <Adafruit_AHTX0.h>

    class Sensor {
      private:
        Adafruit_AHTX0 tempHumSensor;
        sensors_event_t humidity, temperature;
        
        // Для определения частоты опроса
        int lastRead;
        
        // Для хранения показаний
        double currentTemperature, currentHumidity;
      // Обновляет показания датчика, но не чаще чем раз в SENSOR_DURATION секунд
      void readData() {   
        if (millis() - lastRead >= SENSOR_DURATION * 1000 || lastRead == 0) {
          tempHumSensor.getEvent(&humidity, &temperature);
          lastRead = millis();
          currentHumidity = humidity.relative_humidity;
          currentTemperature = temperature.temperature;

          #ifdef DEBUG_ENABLE
            Serial.println("Показания датчика обновлены в соответствии с заданной частой опроса.");
          #endif
          
          return;
        }   
      }

    public:
      bool initialization() {
        lastRead = 0;
        return tempHumSensor.begin();
      }

      double getTemperature() {
        readData();
        return currentTemperature;
      }

      double getHumidity() {
        readData();
        return currentHumidity;
      }
    };

    Sensor tempHumSensor;

  #endif

  // Реализация класса Sensor для датчика для DHT11
  #if SENSOR_TYPE == 2 
    // TODO: Реализовать датчик температуры/влажности для DHT11
  #endif

#endif

// Функционал для работы WIFI
#ifdef WIFI_FEATURE_ENABLE
  
	#ifdef ESP8266
    #include <ESP8266WiFi.h>
  #endif
  
	#ifdef ESP32
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
        WiFi.disconnect();
        WiFi.reconnect();
      
      } else {
        
        if(needShowConnectionInfo){
          
          Serial.print("WiFi соединение установлено, IP адрес: ");
          Serial.println(WiFi.localIP()); 
          needShowConnectionInfo = false;
        
        }
      }
    }
  
	}

#endif

// Функционал для работы с EEPROM
#ifdef EEPROM_FEATURE_ENABLE
  
  #include <EEPROM.h>
  
  // Значения параметров конфигурации по умолчанию
  #define HUMIDITY_LOW_RANGE      60
  #define HUMIDITY_HIGH_RANGE     70
  #define TEMPERATURE_LOW_RANGE   27
  #define TEMPERATURE_HIGH_RANGE  29
  
  // Для туалета (ROOM_TYPE == 2) задержка 2 минуты, для ванной (ROOM_TYPE == 1) - выключена
  #if ROOM_TYPE == 2  
    #define FAN_TIME_DELAY          120
  #endif
  #if ROOM_TYPE == 1  
    #define FAN_TIME_DELAY          0
  #endif
   


  /** 
   * Структура данных для хранения параметров управляемого вентилятора
   * double lowHum - нижняя граница диапазона влажности
   * double highHum - верхняя граница диапазона влажности 
   * double lowTemp - нижняя граница диапазона температуры
   * double highTemp - верхняя граница диапазона температуры
   * int delaySeconds - задержка включения вентилятора в секундах
   * 
  */ 
  struct Params { 
    double lowHum; 
    double highHum; 
    double lowTemp; 
    double highTemp;
    int delaySeconds;
    bool automaticModeEnabled;

    Params(double lowHum, double highHum, double lowTemp, double highTemp, int delaySeconds, bool automaticMode) : lowHum(lowHum), highHum(highHum), lowTemp(lowTemp), highTemp(highTemp), delaySeconds(delaySeconds), automaticModeEnabled(automaticMode) {}

    Params() : lowHum(0.0f), highHum(0.0f), lowTemp(0.0f), highTemp(0.0f), delaySeconds(0), automaticModeEnabled(false) {} 

  };

  /**
   *  Params params - структура для хранения параметров;
   * private: 
   *  bool delayMode - режим задержки включения вентилятора
   *  bool overrideMode - режим ручного управления вентилятором
   *  bool configPublished - флаг публикации параметров в MQTT
   *  bool delayPublished - флаг публикации задержки включения вентилятора
   * public:
   *  Params getConfig() - возвращает параметры конфигурации
   *  void setConfig(Params value) - задает параметры конфигурации
   *  void setLowHumidity(double value) - задает нижнюю границу  влажности
   *  void setHighHumidity(double value) - задает верхнюю границу  влажности
   *  void setLowTemperature(double value) - задает нижнюю границу температуры
   *  void setHighTemperature(double value) - задает верхнюю границу температуры
   *  void setDelaySeconds(int value) - задает время задержки в секундах
   *  void setConfigPublished(bool value) - устанавливает флаг публикации параметров в MQTT
   *  void setDelayPublished(bool value) - устанавливает флаг публикации задержки включения вентилятора
   *  bool isDelayMode() - возвращает режим задержки включения вентилятора
   *  bool isOverrideMode() - возвращает режим ручного управления вентилятором
   *  bool isConfigPublished() - возвращает флаг публикации параметров в MQTT
   *  bool isDelayPublished() - возвращает флаг публикации задержки включения вентилятора
   *  bool isConfigValid() - проверяет, что параметры были инициализированы ненулевыми и валидным
   *  
   * 
  */
  class Config {

  private:
    Params params;
    bool delayMode = false;
    bool overrideMode = false;
    bool configPublished = false;
    bool delayPublished = false;
    bool stateChanged = false;

  public:
    
    Params getConfig(){
      return params;
    }

    /** Задает параметры конфигурации */
    void setConfig(Params value) {     
      params.lowHum = value.lowHum;
      params.highHum = value.highHum;
      params.lowTemp = value.lowTemp;
      params.highTemp = value.highTemp; 
      params.delaySeconds = value.delaySeconds;
    }
    
    /** Задает нижнюю границу  влажности */
    void setLowHumidity(double value) {
      params.lowHum = value;
    }
    
    /** Задает верхнюю границу  влажности */
    void setHighHumidity(double value) {
      params.highHum = value;
    }
    
    /** Задает нижнюю границу температуры */
    void setLowTemperature(double value) {
      params.lowTemp = value;
    }
    
    /** Задает верхнюю границу температуры */
    void setHighTemperature(double value) {
      params.highTemp = value;
    }

    /** Задает время задержки в секундах */
    void setDelaySeconds(int value) {
      params.delaySeconds = value;
      if (params.delaySeconds > 0) {
        delayMode = true;
      }
    }

    /** Читает все параметры из EEPROM */
    void readConfigFromEEPROM(){
      
      EEPROM.get(0, params);
      #ifdef DEBUG_ENABLE
        printf("Прочитаны параметры (lowHum=%.2f, highHum=%.2f, lowTemp=%.2f, highTemp=%.2f, delaySeconds=%d, automaticModeEnabled=%d) в EEPROM\n",params.lowHum, params.highHum, params.lowTemp, params.highTemp, params.delaySeconds, params.automaticModeEnabled);
      #endif
    }

    /** Записывает параметры в EEPROM */
    void writeConfigToEEPROM(){
      
      #ifdef DEBUG_ENABLE
        printf("Записываю параметры (lowHum=%.2f, highHum=%.2f, lowTemp=%.2f, highTemp=%.2f, delaySeconds=%d) в EEPROM\n",params.lowHum, params.highHum, params.lowTemp, params.highTemp, params.delaySeconds);
      #endif

      EEPROM.begin(sizeof(params));
      EEPROM.put(0, params);
      
      #ifndef DEBUG_EEPROM
        EEPROM.commit();
      #endif

    }
    
    /** Проверяет, что параметры были инициализированы ненулевыми и валиднымизначениями */
    bool isConfigValid() {
            
      if ( isnan(params.lowHum) || isnan(params.highHum) || isnan(params.lowTemp) || isnan(params.highTemp) || isnan(params.delaySeconds) ) {
        #ifdef DEBUG_ENABLE
          Serial.println("Пустые значения параметров в EEPROM");
        #endif
        return false;
      }
      
      // Убеждаемся, что значения были инициализированы ненулевыми значениями
      const float EPSILON = 1e-6; // Допустимая погрешность
      
      if (fabs(params.lowHum) < EPSILON && fabs(params.highHum) < EPSILON &&
          fabs(params.lowTemp) < EPSILON && fabs(params.highTemp) < EPSILON) {
        #ifdef DEBUG_ENABLE
          Serial.println("В EEPROM хранятся нулевые значения пороговых значений температуры/влажности");
        #endif   
        return false;
      }
      
      // Проверяем, что значения находятся в допустимом диапазоне
      if (params.lowHum < 0 || params.lowHum > 100 ) {
        #ifdef DEBUG_ENABLE
          Serial.println("Недопустимые значения нижнего порога влажности в EEPROM (" + String(params.lowHum) + ")");
        #endif
        return false;
      }
      
      if (params.highHum < 0 || params.highHum > 100 ) {
        #ifdef DEBUG_ENABLE
          Serial.println("Недопустимые значения верхнего порога влажности в EEPROM (" + String(params.highHum) + ")");
        #endif  
        return false;
      }
      
      if (params.lowTemp < -50 || params.lowTemp > 50 ) {
        #ifdef DEBUG_ENABLE
          Serial.println("Недопустимые значения нижнего порога температуры в EEPROM (" + String(params.lowTemp) + ")");
        #endif
        return false;
      }
      
      if (params.highTemp < -50 || params.highTemp > 50 ) {
        #ifdef DEBUG_ENABLE
          Serial.println("Недопустимые значения верхнего порога температуры в EEPROM (" + String(params.highTemp) + ")");
        #endif
        return false;
      }

      if (params.delaySeconds < 0  || params.delaySeconds > 3600 ) {
        #ifdef DEBUG_ENABLE
          Serial.println("Недопустимые значения задержки включения вентилятора в EEPROM (" + String(params.delaySeconds) + ")");
        #endif
        return false;
      }

      if(params.automaticModeEnabled < 0 || params.automaticModeEnabled > 1) {
        #ifdef DEBUG_ENABLE
          Serial.println("Недопустимые значения флага Автоматический режим вентилятора в EEPROM (" + String(params.automaticModeEnabled) + ")");
        #endif
        return false;
      }
      
      return true;
    }

    /** Возвращает факт публикации параметров в MQTT */
    bool isConfigPublished() {
      return configPublished;  
    }

    /** Устанавливает флаг публикации параметров в MQTT */
    void setConfigPublished(bool value) {
      configPublished = value;
    }

    /** Возвращает флаг необходимости отсчета задержки включения вентилятора */
    bool isDelayMode() {
      return delayMode;
    }
    
    // Устанавливает режим включения вентилятора по таймеру
    void setDelayMode(bool value) {
      delayMode = value;

      #ifdef DEBUG_ENABLE
        printf("Задержка включения вентилятора  %s.\n", value ? ("активна") : "выключена");    
      #endif
    }
    
    //* Возвращает время задерки в секундах
    int getDelaySeconds() {
      return params.delaySeconds;
    }

    /** Возвращает флаг публикации задержки включения вентилятора */
    bool isDelayPublished() {
      return delayPublished;
    }

    // Устанавливает флаг публикации задержки включения вентилятора
    void setDelayPublished(bool value) {
      delayPublished = value;
    }

    // Возвращает флаг нахождения в режиме ручного управления
    bool isOverrideMode() {
      return overrideMode;
    }

    // Устанавливает режим ручного управления вентилятором
    void setSwitchOverrideMode(bool value) {
      overrideMode = value; 
    }
    
    // Возвращает флаг изменения состояния
    bool isStateChanged() { return stateChanged; }

    // Включает флаг изменения состояния 
    void setStateChanged() { stateChanged = true; }

    bool isFanMode() { return params.automaticModeEnabled; }
    
    void setFanMode(bool value) { params.automaticModeEnabled = value; }
    
    Config(Params newdata) { params = newdata;}

    Config() {}

  };

  Config deviceConfig;

#endif

// Функционал для работы с OTA
#ifdef OTA_ENABLE

  #if defined(ESP8266)
    #include <ESPAsyncTCP.h>
    #include <AsyncElegantOTA.h>
  #elif defined(ESP32)
    #include <AsyncTCP.h>
    #include <ElegantOTA.h>
  #endif

  #include <ESPAsyncWebServer.h>

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

// Функционал управления выключателем
#ifdef SWITCH_FEATURE_ENABLE
  

  /**
   * @brief Возвращает состояние выключателя
   * 
   * @return true - выключатель включен
   * @return false - выключатель выключен
   */
  bool isSwitchOn() {
    
    bool curSwitchState = digitalRead(SWITCH_PIN);  
    
    if(curSwitchState == SWITCH_ON_LEVEL) {
      return true; 
      
    } else {
      
      return false;
    }
    
  }
  
  /**
 * @brief Устанавливает выключатель в заданное состояние, если оно не совпадает с текущим
 * 
 * @param state - true - включить, false - выключить
 */
  void setSwitchState(bool state) {
    
    bool curSwitchState = isSwitchOn();
    
    
    if(!(state == curSwitchState)){
      
      digitalWrite(SWITCH_PIN, state ? SWITCH_ON_LEVEL : SWITCH_OFF_LEVEL); 
      
    }
  }
  
#endif


// Функционал управления вентилятором
#ifdef FAN_CONTROL_FEATURE_ENABLE
  
  // Переменные для рассчета оставшегося времени в таймере
  unsigned long currentTime, lastTime;
  
  int delayCounter;

  #ifdef DEBUG_ENABLE
    // Переменные для отображения оставшихся секунд в таймере
    int secondRemain, lastSecondRemain;
  #endif
  
  // Предпочтительное начальное состояние вентилятора
  bool prefferedFanState(const Params& params, double temperature, double humidity) {
      // Проверяем, превышает ли текущая температура порог высокой температуры
      // или текущая влажность порог высокой влажности
      if (temperature > params.highTemp || humidity > params.highHum) {
          
          #ifdef DEBUG_ENABLE
            Serial.println("Температура или влажность выше пороговых значений.");
          #endif
          
          return true; // Вентилятор должен быть включен
      }
      
      // Проверяем, ниже ли текущая температура порога низкой температуры
      // и ниже ли текущая влажность порога низкой влажности
      if (temperature < params.lowTemp && humidity < params.lowHum) {
          
          #ifdef DEBUG_ENABLE
            Serial.println("Температура или влажность ниже пороговых значений.");
          #endif           
          
          return false; // Вентилятор должен быть выключен
      }
      
      // Если температура и влажность находятся в пределах пороговых значений, возвращаем текущее состояние вентилятора
      
      bool curState = isSwitchOn();
      return curState;

  }
  
  

#endif

// Функционал работы с брокером MQTT
#ifdef MQTT_FEATURE_ENABLE

  // Определение переменных для соединения
  WiFiClient espClient;
  #include <PubSubClient.h>
  PubSubClient client(espClient);
  const char mqttServer[] = MQTT_ADDRESS;
  const char mqttUser[] = MQTT_USER;
  const char mqttPassword[] = MQTT_PASSWORD;
  char mqttId[18];
  char lastWillTopic[25];

  ////////////////////////////////////////////////////////////////
  // Функции для работы с брокером
  ////////////////////////////////////////////////////////////////

  // Переменные для работы с выключателем
  // Определение переменных для MQTT
  #ifdef SWITCH_FEATURE_ENABLE
    char switchStateTopic[50];
    char switchControlTopic[50];  
    // Хранит предыдущее состояние выключателя чтобы понимать, нужно ли публиковать его состояние
    bool lastSwitchState;
  #endif
  
  // Переменные для работы с датчиком температуры/влажности
  #ifdef SENSOR_TEMP_HUM_ENABLE
      
    // Состояние температуры/влажности
    char temperatureStateTopic[37];
    char humidityStateTopic[37];
  
  #endif

  // Переменные для работы с вентилятором
  #ifdef FAN_CONTROL_FEATURE_ENABLE
    // Топики для изменения задержки включения вентиляции
    char fanTimeDelayStateTopic[37];
    char fanTimeDelayChangeTopic[37];
  
  #endif
  // Переменные для работы с конфигурацией вентилятора
  #ifdef EEPROM_FEATURE_ENABLE
    
    // Определение переменных для отображения границ температуры/влажности
    char lowTemperatureStateTopic[37];
    char highTemperatureStateTopic[37];
    char lowHumidityStateTopic[37];
    char highHumidityStateTopic[37];     

    // Определение переменных для изменения границ температуры/влажности
    char lowTemperatureChangeTopic[37];
    char highTemperatureChangeTopic[37];
    char lowHumidityChangeTopic[37];
    char highHumidityChangeTopic[37];
  
  #endif  

  // Проверка подключения к брокеру
  void checkMqttConnection() {
    const char* lastWillMessage = "Offline";
    
    if (!client.connected()) {
      
      #ifdef DEBUG_ENABLE
        printf("Попытка подключиться к mqtt брокеру по адресу %s с идентификатором %s.\n",mqttServer, mqttId);
      #endif

      if (client.connect(mqttId, mqttUser, mqttPassword, lastWillTopic, 1, true, lastWillMessage)) {
        
        #ifdef DEBUG_ENABLE
          printf("Подключение к mqtt брокеру установлено.\n");
        #endif

        // Обновить статус канала
        client.publish(lastWillTopic, "Online");
        #ifdef DEBUG_MQTT
          printf("Опубликовано 'Online' в топике %s.\n",lastWillTopic);
        #endif

        // Функционал для работы с выключателем
        #ifdef SWITCH_FEATURE_ENABLE
          
          client.subscribe(switchControlTopic);

          #ifdef DEBUG_MQTT
            printf("Подписка на топик %s выполнена.\n",switchControlTopic);
          #endif
        
        #endif

        // Функционал для работы с датчиком
        #ifdef SENSOR_TEMP_HUM_ENABLE
          if(sensor_found) {
            client.subscribe(lowTemperatureChangeTopic);
            client.subscribe(highTemperatureChangeTopic);
            client.subscribe(lowHumidityChangeTopic);
            client.subscribe(highHumidityChangeTopic);
            
            #ifdef DEBUG_MQTT
              printf("Подписка на топик %s выполнена.\n",lowTemperatureChangeTopic);
              printf("Подписка на топик %s выполнена.\n",highTemperatureChangeTopic);
              printf("Подписка на топик %s выполнена.\n",lowHumidityChangeTopic);
              printf("Подписка на топик %s выполнена.\n",highHumidityChangeTopic);
            #endif
          }

        #endif

        // Функционал для работы с вентилятором
        #ifdef FAN_CONTROL_FEATURE_ENABLE
          
          client.subscribe(fanTimeDelayChangeTopic);
          
          #ifdef DEBUG_MQTT
            printf("Подписка на топик %s выполнена.\n",fanTimeDelayChangeTopic);
          #endif 
        
        #endif

        

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

  // Обработка полученных комманд
  void callback(char* topic, byte* payload, unsigned int length){

    #ifdef DEBUG_MQTT
      printf("Received mqtt command. \n");
    #endif

    String recievedPayload;
    for (unsigned int i = 0; i < length; i++) {
      recievedPayload += (char)payload[i];
    };
    
    // Если получена команда управления выключателем, обработать ее
    #ifdef SWITCH_FEATURE_ENABLE
      
      if (strcmp(topic, switchControlTopic) == 0){
      
      #ifdef DEBUG_MQTT
        printf("This is command for switch\n");
      #endif

      if (recievedPayload == "1" || recievedPayload == SWITCH_ON_VALUE) {
        setSwitchState(SWITCH_ON_LEVEL);
      } else if (recievedPayload == "0" || recievedPayload == SWITCH_OFF_VALUE) {
        setSwitchState(SWITCH_OFF_LEVEL);
      } else {
        #ifdef DEBUG_MQTT
          printf("Unknown switch command\n");
        #endif
      }
      
      // Команда управления выключателем переводит его в режим ручного управления
      deviceConfig.setSwitchOverrideMode(true);
      return;
    }
    #endif

    // Если получена команда управления границами температуры/влажности, обработать ее
    #ifdef SENSOR_TEMP_HUM_ENABLE
      if(sensor_found) {
        
        // Если получена команда настройки параметров, обработать ее
        // Если отладка не включена, сохранить новые параметры в EEPROM       
        #ifdef EEPROM_FEATURE_ENABLE
          
          if (strcmp(topic, lowTemperatureChangeTopic) == 0){  
            float newTemperature = recievedPayload.toFloat();    
            deviceConfig.setLowTemperature(newTemperature);
            
            
            #ifndef DEBUG_EEPROM
              deviceConfig.write();
            #endif  
            
            #ifdef DEBUG_MQTT
              printf("This is command for lowTemperatureChange\n");
            #endif
            
            publishConfigTopic(topic);
            return;
          
          } else if (strcmp(topic, highTemperatureChangeTopic) == 0){
            
            float newTemperature = recievedPayload.toFloat();
            deviceConfig.setHighTemperature(newTemperature);
            
            #ifndef DEBUG_EEPROM
              deviceConfig.write();
            #endif
        
            #ifdef DEBUG_MQTT
              printf("This is command for highTemperatureChange\n");
            #endif

            publishConfigTopic(topic);
            return;
          
          } else if (strcmp(topic, lowHumidityChangeTopic) == 0){
            float newHumidity = recievedPayload.toFloat();
            deviceConfig.setLowHumidity(newHumidity);
            
            #ifndef DEBUG_EEPROM
              deviceConfig.write();
            #endif

            #ifdef DEBUG_MQTT
              printf("This is command for lowHumidityChange\n");
            #endif

            publishConfigTopic(topic);
            return;

          } else if (strcmp(topic, highHumidityChangeTopic) == 0){
            float newHumidity = recievedPayload.toFloat();
            deviceConfig.setHighHumidity(newHumidity);
            
            #ifndef DEBUG_EEPROM
              deviceConfig.write();
            #endif

            #ifdef DEBUG_MQTT
              printf("This is command for highHumidityChange\n");
            #endif
            
            publishConfigTopic(topic);
            return;

          }
          
        #endif 
      }
    #endif
    
    // Если получена команда установки задержки включения вентилятора, обработать ее
    #ifdef FAN_CONTROL_FEATURE_ENABLE
     
      if (strcmp(topic, fanTimeDelayChangeTopic) == 0){
        int newDelay = recievedPayload.toInt();
        deviceConfig.setDelaySeconds(newDelay);
        
        #ifndef DEBUG_EEPROM
          deviceConfig.write();
        #endif
        
        #ifdef DEBUG_MQTT
          printf("This is command for settinngfanTimeDelay to %d\n", newDelay);
        #endif
        
        deviceConfig.setDelayPublished(false);
        return;
      }

    #endif

    // Иначе сообщить что команда не распознана
    #ifdef DEBUG_MQTT
      printf("This is unknown command\n");
    #endif  
  }

  // Методы работы с выключателем
  #ifdef SWITCH_FEATURE_ENABLE
  
    /** Публикация состояния выключателя */
    bool publishSwitchState() {
      bool result = false;
      bool currentSwitchState = isSwitchOn();
      
      if (client.connected() ) {
        const char *stateValue = (currentSwitchState ? SWITCH_ON_VALUE : SWITCH_OFF_VALUE);
        result = client.publish(switchStateTopic, (stateValue));
        
        #ifdef DEBUG_MQTT

          if(result){
            printf("В топике : %s опубликовано состояние выключателя: %s\n", switchStateTopic, stateValue );
          } else {
            Serial.println("Не удалось опубликовать состояние выключателя");
          }
          
        #endif
      
      }
      return result;
    }

  #endif

  // Методы работы с датчиком температуры/влажности
  #ifdef SENSOR_TEMP_HUM_ENABLE
    
    double temperature, humidity;

    /** Публикация состояния датчика */
    void publishSensorState(double temperature, double humidity) {
      // Состояние температуры и влажности
      client.publish(temperatureStateTopic, String(temperature).c_str());
      #ifdef DEBUG_MQTT
        printf("В топике : %s опубликованы показания датчика температуры: %.2f\n", temperatureStateTopic,temperature);
      #endif
      
      client.publish(humidityStateTopic, String(humidity).c_str());
      #ifdef DEBUG_MQTT
        printf("В топике : %s опубликованы показания датчика влажности: %.2f\n", humidityStateTopic,humidity);
      #endif

    }
  
  #endif

  // Методы работы с вентилятором
  #ifdef FAN_CONTROL_FEATURE_ENABLE
    
    // Публикация задержки включения вентилятора
    bool piblishFanDelayState() {
    
      bool result = false;
      
      if (client.connected() ) {
        int delaySec = deviceConfig.getDelaySeconds();
        result = client.publish(fanTimeDelayStateTopic, String(delaySec).c_str() );
        
        #ifdef DEBUG_MQTT
          if(result){
            printf("В топике : %s опубликована задержка включения вентилятора\n", switchStateTopic );
          } else {
            Serial.println("Не удалось опубликовать задержку включения вентилятора");
          }
          
        #endif
      
      }
    return result;
    }

  #endif
  
  

  // Методы работы с конфигурацией
  #ifdef EEPROM_FEATURE_ENABLE
  
    /**
     * @brief Публикует границы значени в MQTT
     * @return Истина в случае успеха, ложь в случае ошибки
     */
    bool publishConfig() {
      
      Params curr_params = deviceConfig.getConfig();

      if(client.connected() ) {
        bool succes = true;

        succes = (succes && client.publish(highTemperatureStateTopic, String(curr_params.highTemp).c_str()));
        #ifdef DEBUG_MQTT
          printf("В топике %s %s опубликована верхняя граница температуры: %.2f\n", (succes?" успешно":" неуспешно"), highTemperatureStateTopic,curr_params.highTemp);  
        #endif
        succes = (succes && client.publish(lowTemperatureStateTopic, String(curr_params.lowTemp).c_str()));
        #ifdef DEBUG_MQTT
          printf("В топике %s %s опубликована нижняя граница температуры: %.2f\n", (succes?" успешно":" неуспешно"),lowTemperatureStateTopic,curr_params.lowTemp);
        #endif
        succes = (succes && client.publish(highHumidityStateTopic, String(curr_params.highHum).c_str()));
        #ifdef DEBUG_MQTT
          printf("В топике %s %s опубликована верхняя граница влажности: %.2f\n", (succes?" успешно":" неуспешно"), highHumidityStateTopic,curr_params.highHum);  
        #endif
        succes = (succes && client.publish(lowHumidityStateTopic, String(curr_params.lowHum).c_str()));
        #ifdef DEBUG_MQTT
          printf("В топике %s %s опубликована нижняя граница влажности: %.2f\n", (succes?" успешно":" неуспешно"), lowHumidityStateTopic,curr_params.lowHum);
        #endif
        succes = (succes && client.publish(fanTimeDelayStateTopic, String(curr_params.delaySeconds).c_str()));
        #ifdef DEBUG_MQTT
          printf("В топике %s %s опубликована задержка вентиляции: %d\n", (succes?" успешно":" неуспешно"), fanTimeDelayStateTopic,curr_params.delaySeconds);
        #endif
        return succes;
      } else {
        
        #ifdef DEBUG_ENABLE
          printf("Нет подключения к MQTT брокеру.\n");
        #endif
        return false;
      }
    }
       
    /**
     * @brief Публикует границы значения в MQTT
     * 
     */
    void publishConfigTopic(char * topic) {
      
      #ifdef DEBUG_MQTT
        printf("Вызов функции publishConfigTopic(%s)\n", topic);  
      #endif

      Params curr_params = deviceConfig.getConfig();

      if(client.connected() ) {
        
        if(strcmp(topic, highTemperatureChangeTopic) == 0) {
          client.publish(highTemperatureStateTopic, String(curr_params.highTemp).c_str());
          #ifdef DEBUG_MQTT
            printf("В топике %s опубликована верхняя граница температуры: %.2f\n", highTemperatureStateTopic,curr_params.highTemp);  
          #endif
          return;
        } else  if(strcmp(topic, lowTemperatureChangeTopic) == 0) {
          client.publish(lowTemperatureStateTopic, String(curr_params.lowTemp).c_str());
          #ifdef DEBUG_MQTT
            printf("В топике %s опубликована нижняя граница температуры: %.2f\n", lowTemperatureStateTopic,curr_params.lowTemp);
          #endif
          return;
        } else if(strcmp(topic, highHumidityChangeTopic) == 0) {
          client.publish(highHumidityStateTopic, String(curr_params.highHum).c_str());
          #ifdef DEBUG_MQTT
            printf("В топике %s опубликована верхняя граница влажности: %.2f\n", highHumidityStateTopic,curr_params.highHum);  
          #endif
          return;
        } else if(strcmp(topic, lowHumidityChangeTopic) == 0) {
          client.publish(lowHumidityStateTopic, String(curr_params.lowHum).c_str());
          #ifdef DEBUG_MQTT
            printf("В топике %s опубликована нижняя граница влажности: %.2f\n", lowHumidityStateTopic,curr_params.lowHum);
          #endif
          return;
        } else if(strcmp(topic, fanTimeDelayChangeTopic) == 0) {
          client.publish(fanTimeDelayStateTopic, String(curr_params.delaySeconds).c_str());
          #ifdef DEBUG_MQTT
            printf("В топике %s опубликована задержка вентиляции: %d\n", fanTimeDelayStateTopic,curr_params.delaySeconds);
          #endif
          return;
        } else {
          #ifdef DEBUG_ENABLE
            printf("Топик %s не распознан\n", topic);
          #endif
          return;
        }

      } else {
        #ifdef DEBUG_ENABLE
          printf("Нет подключения к MQTT брокеру.\n");
        #endif
        return;
      }
    }
  
  #endif

#endif


void setup() {

  Serial.begin(115200); 
  Serial.println();
  delay(1000);
 
  #ifdef DEBUG_ENABLE

    delay(2000);
    Serial.println("Выполняется инициализация устройства ... ");   
    Serial.println("Устройство cконфигурировано для отладки (DEBUG_ENABLE)");
    
    // Platform info   
    printf("Устройство cконфигурировано для чипа %s\n",CHIP_DESC);
    
    // Mode info
    #ifdef FAN_CONTROL_FEATURE_ENABLE
      Serial.println("Устройство cконфигурировано для управление вентилятором на основании датчика температуры/влажности (FAN_CONTROL_FEATURE_ENABLE)");
    #endif

    #ifdef SENSOR_TEMP_HUM_ENABLE
      Serial.println("Устройство cконфигурировано для работы с датчиком температуры/влажности (SENSOR_TEMP_HUM_ENABLE)");
    #endif

    #ifdef SWITCH_FEATURE_ENABLE
      Serial.println("Устройство cконфигурировано для управления выключателем (SWITCH_FEATURE_ENABLE)");
    #endif

    // Features info
    #ifdef WIFI_FEATURE_ENABLE
      Serial.println("Устройство cконфигурировано для управления WiFi (WIFI_FEATURE_ENABLE)");
    #endif
    
    #ifdef MQTT_FEATURE_ENABLE
      Serial.println("Устройство cконфигурировано для управления по MQTT (MQTT_FEATURE_ENABLE)");
    #endif

    #ifdef EEPROM_FEATURE_ENABLE
      Serial.println("Устройство cконфигурировано для чтения параметров из EEPROM (EEPROM_FEATURE_ENABLE)");
    #endif

    #ifdef OTA_ENABLE
      Serial.println("Устройство cконфигурировано для обновления по воздуху (OTA_ENABLE)");
    #endif

    #ifdef ROOM_TYPE
      printf("Устройство cконфигурировано для %s.\n", ROOM_TYPE == 1 ? "ванной комнаты" : "туалета");
    #endif

    // End of info
    #ifdef DEBUG_ENABLE
      Serial.println("Инициализация устройства завершена.\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
    #endif

  #endif

  // Если используется EEPROM, прочитаем сохраненную конфигурацию или инициализируем значения по умолчанию
  #ifdef EEPROM_FEATURE_ENABLE
    
    deviceConfig.readConfigFromEEPROM();

    if(!deviceConfig.isConfigValid()){
      Params newParams((double) HUMIDITY_LOW_RANGE,(double) HUMIDITY_HIGH_RANGE,(double) TEMPERATURE_LOW_RANGE,(double) TEMPERATURE_HIGH_RANGE, (int) FAN_TIME_DELAY, (bool) false);
      deviceConfig.setConfig(newParams ) ;
      
      #ifdef DEBUG_ENABLE       
        printf("Установлены параметры по умолчанию (lowHum=%.2f, highHum=%.2f, lowTemp=%.2f, highTemp=%.2f, delaySeconds=%d, automaticModeEnabled=%d) в EEPROM\n",newParams.lowHum, newParams.highHum, newParams.lowTemp, newParams.highTemp, newParams.delaySeconds, newParams.automaticModeEnabled);
      #endif

    } else {

      #ifdef DEBUG_ENABLE
        Serial.println("Параметры загружены из EEPROM");
      #endif
    };

    
    // Если задержка включения вентилятора больше нуля, установим режим задержки включения вентилятора
    if(deviceConfig.getDelaySeconds() >0 ){
      deviceConfig.setDelayMode(true);
    }

    // Если включен автоматический режим, флаг автоматического режима установим в true

    deviceConfig.setFanMode(deviceConfig.isFanMode());
  
  #endif
  
  // Если используется Wi-Fi, подключаемся к сети
  #ifdef WIFI_FEATURE_ENABLE
    
    #ifdef DEBUG_ENABLE
      printf("Подключение к сети WiFi.");
    #endif
    
    WiFi.mode(WIFI_STA);  
    WiFi.begin(ssid, wifiPassword);
    
    
    while(!WiFi.isConnected()){
      #ifdef DEBUG_ENABLE
        printf(".");
      #endif
      delay(500);
    }
    #ifdef DEBUG_ENABLE
      printf("OK.\n");
    #endif
    
  #endif
  
  // Если используется обновление по воздуху, создадим HTTP сервер и его обработку
  #ifdef OTA_ENABLE 
    
    const char* deviceDesc = DEVICE_DESC;
    String deviceModel = CHIP_DESC;
    
    #ifdef ESP32
      deviceModel = deviceModel + " " + ESP.getChipModel();
    #endif
    
    #ifdef ESP8266
      deviceModel = deviceModel + " model " + String(ESP.getChipId(), HEX);
    #endif

    const String pageContent = "<html><body style='text-align: center; font-family: Arial, sans-serif; font-size: 16px;'><h1>Device Info</h1><p>Device model: <b>" + deviceModel +
                         "</b></p><p>Device description: <b>" + deviceDesc +
                         "</b></p><h3><a href='/update'>Страница обновления</a></h3></body></html>";
    
    server.on("/", HTTP_GET, [pageContent](AsyncWebServerRequest *request) {
      request->send(200, "text/html", pageContent);
    });
    
    #if defined(ESP32)
      ElegantOTA.begin(&server);    
      ElegantOTA.onStart(onOTAStart);
      ElegantOTA.onProgress(onOTAProgress);
      ElegantOTA.onEnd(onOTAEnd);
      server.begin();
    #elif defined(ESP8266)
      AsyncElegantOTA.begin(&server);    
      server.begin();
    #endif
    
    #ifdef DEBUG_ENABLE
      Serial.println("Создан HTTP сервер"); 
      Serial.println("Cтраница обновления на http://"+WiFi.localIP().toString()+"/update");
    #endif
  #endif
  
  // Если используется MQTT, создадим клиент MQTT 
  #ifdef MQTT_FEATURE_ENABLE
    // Сохраним мак-адрес для последующего использования 
    WiFi.macAddress(mac);
    
    // Создадим строки для публикации состояния подключения к брокеру  
    snprintf(mqttId, sizeof(mqttId), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    snprintf(lastWillTopic, sizeof(lastWillTopic), "%s/status", mqttId);
    
    client.setServer(mqttServer, 1883);
    
    // Для управляемых устройств создаем функцию обработки
    client.setCallback(callback);
    
    #ifdef SWITCH_FEATURE_ENABLE
    
      // Настроим топики для отображения состояния выключателя
      snprintf(switchStateTopic, sizeof(switchStateTopic), "%s/switch/state", mqttId);
      snprintf(switchControlTopic, sizeof(switchControlTopic), "%s/switch/c/state", mqttId);
      
      #ifdef DEBUG_MQTT
        Serial.println("Топик состояния выключателя =" + String(switchStateTopic));
        Serial.println("Топик команд выключателя =" + String(switchControlTopic));
      #endif
    #endif
    
  #endif
  

  // Настроим датчик температуры и влажности если необходимо
  // @note: Из-за неверно выбранного pin (GPIO4) для управления реле ( связан с I2C Wire), 
  // при инициализации датчика AHT происходит перевод режима в Wire/I2C, поэтому GPIO для выключателя приходилось инициировать уже после датчика
  // Изменение SWITCH_PIN на 2 должно исправить проблему 
  #ifdef SENSOR_TEMP_HUM_ENABLE
    
    #ifdef DEBUG_ENABLE
      Serial.print("Инициализация датчика температуры и влажности: ");
    #endif
    
    if(tempHumSensor.initialization()){
      sensor_found = true;
      
      #ifdef DEBUG_ENABLE
        Serial.println("успешно");        
        Serial.printf("Текущие показания датчика: температура %2.2f ºC, влажность %2.2f%%\n", tempHumSensor.getTemperature(),tempHumSensor.getHumidity()); 
      #endif

    } else {
      sensor_found = false;
      
      #ifdef DEBUG_ENABLE
        Serial.println("датчик не найден");
      #endif  
    
    };
    
    // Если датчик найден, то настроим топики для отображения показаний датчика
    #ifdef MQTT_FEATURE_ENABLE

      if(sensor_found){
        
        // Настроим топики для отображения показаний датчика температуры/влажности
        snprintf(temperatureStateTopic, sizeof(temperatureStateTopic), "%s/sensor/temperature", mqttId);
        snprintf(humidityStateTopic, sizeof(humidityStateTopic), "%s/sensor/humidity", mqttId);
        
        #ifdef DEBUG_MQTT 
          Serial.println("Топик состояния температуры =" + String(temperatureStateTopic));
          Serial.println("Топик состояния влажности =" + String(humidityStateTopic));
        #endif
      }
    
    #endif
  
  #endif
  
  // Инициируем выключатель
  #ifdef SWITCH_FEATURE_ENABLE

    pinMode(SWITCH_PIN, OUTPUT);

    // Если при включении состояние выключателя должно быть установлено в определенное значение, сделаем это сразу
    #ifdef SET_SWITCH_OFF
      
      #ifdef DEBUG_ENABLE
        Serial.println("Установка первоначального состояния выключателя в состояние OFF");
      #endif
      
      setSwitchState(SWITCH_OFF_LEVEL); 

    #endif
    
    // Сбросить флаг ручного управления выключателем
    deviceConfig.setSwitchOverrideMode(false);

    #ifdef MQTT_FEATURE_ENABLE
      // Чтобы состояние выключателя публиковалось при первом прохождении loop
      lastSwitchState = !isSwitchOn();
    #endif

    
  
  #endif

  
  // Настроим вентилятор. 
  #ifdef FAN_CONTROL_FEATURE_ENABLE   
    
    #ifdef MQTT_FEATURE_ENABLE
      // Настроим топики для jnj,hf;tybz и управления задеержкой включения вентилятора
      sprintf(fanTimeDelayStateTopic, "%s/switch/fanDelay", mqttId);
      sprintf(fanTimeDelayChangeTopic, "%s/switch/c/fanDelay", mqttId);
      
      #ifdef DEBUG_MQTT
        Serial.println("Топик состояния задержки вентилятора =" + String(fanTimeDelayStateTopic));
        Serial.println("Топик команд для задержки включения вентиляции =" + String(fanTimeDelayChangeTopic));
      #endif
      
      // Если есть датчик, настроим элементы управления вентилятором   
      if (sensor_found) {      
        sprintf(lowTemperatureStateTopic, "%s/sensor/lowTemp", mqttId);
        sprintf(highTemperatureStateTopic, "%s/sensor/highTemp", mqttId);
        sprintf(lowHumidityStateTopic, "%s/sensor/lowHum", mqttId);
        sprintf(highHumidityStateTopic, "%s/sensor/highHum", mqttId);

        #ifdef DEBUG_MQTT
          Serial.println("Топик состояния нижнего порога температуры =" + String(lowTemperatureStateTopic));
          Serial.println("Топик состояния верхнего порога температуры =" + String(highTemperatureStateTopic));
          Serial.println("Топик состояния нижнего порога влажности =" + String(lowHumidityStateTopic));
          Serial.println("Топик состояния верхнего порога влажности =" + String(highHumidityStateTopic));
        #endif
          
        // Настроим топики для установки параметров вентилятора управляемого показаниями датчика температуры/влажности
        snprintf(lowTemperatureChangeTopic, sizeof(lowTemperatureChangeTopic), "%s/sensor/c/lowTemp", mqttId);
        snprintf(highTemperatureChangeTopic, sizeof(highTemperatureChangeTopic), "%s/sensor/c/highTemp", mqttId);
        snprintf(lowHumidityChangeTopic, sizeof(lowHumidityChangeTopic), "%s/sensor/c/lowHum", mqttId);
        snprintf(highHumidityChangeTopic, sizeof(highHumidityChangeTopic), "%s/sensor/c/highHum", mqttId);

        #ifdef DEBUG_MQTT
          Serial.println("Топик команд для изменения нижнего порога температуры =" + String(lowTemperatureChangeTopic));
          Serial.println("Топик команд для изменения верхнего порога температуры =" + String(highTemperatureChangeTopic));
          Serial.println("Топик команд для изменения нижнего порога влажности =" + String(lowHumidityChangeTopic));
          Serial.println("Топик команд для изменения верхнего порога влажности =" + String(highHumidityChangeTopic)); 
        #endif

        // Сбросить флаг публикации конфигурации
        deviceConfig.setConfigPublished(false);

      }

    #endif
    
    // Если используется задержка включения по таймеру, то настроим таймер
    
    #ifdef DEBUG_ENABLE
      printf("Режим включения по таймеру %s\n", deviceConfig.isDelayMode() ? "активирован" : "отключен");
    #endif

    if(deviceConfig.isDelayMode())  {

      delayCounter = deviceConfig.getDelaySeconds();
      #ifdef DEBUG_ENABLE
        printf("Задержка включения по таймеру = %d сек.\n", delayCounter);
      #endif
    
    }
  
  #endif

  #ifdef DEBUG_ENABLE
    printf("Инициализация устройства окончена.\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n\n");
  #endif
   
}

void loop() {
  
  // Проверка подключения к WiFi
  #ifdef WIFI_FEATURE_ENABLE   
    checkWiFiConnection();   
  #endif

  // Обработка прошивки по воздуху нужна только для ESP32
  #ifdef OTA_ENABLE    
    #if defined(ESP32)
      if (WiFi.status() == WL_CONNECTED) {
        ElegantOTA.loop(); 
      }
      
    #endif
  #endif

  // Проверка подключения к MQTT
  #ifdef MQTT_FEATURE_ENABLE  
    if (WiFi.status() == WL_CONNECTED) {
      checkMqttConnection(); 
    }
  #endif

  // Признак изменения состояния сенсора
  bool sensorDataChanged = false;

  // Обработка логики сенсора, если он подключен
  #ifdef SENSOR_TEMP_HUM_ENABLE
    if(sensor_found){
      
      double curTemperature = tempHumSensor.getTemperature();
      double curHumidity = tempHumSensor.getHumidity();
      
      // Убедиться что состояние сенсора поменялось
      sensorDataChanged = (!(curTemperature == temperature || curHumidity == humidity)) ;
           
      if(sensorDataChanged) {
      
        #ifdef DEBUG_ENABLE
          Serial.printf("Изменились текущие показания датчика: Температура: %.2fC, Влажность: %.2f%% \n", curTemperature, curHumidity);
        #endif
        
        // Опубликовать новые показания датчика
        #ifdef MQTT_FEATURE_ENABLE
          
          publishSensorState(curTemperature, curHumidity);
        
          #ifdef DEBUG_MQTT
            Serial.println("Показания датчика опубликованы");
          #endif     

        #endif  
        
        // Запомнить новые показания датчика
        temperature = curTemperature;
        humidity = curHumidity;

      }
    }
  #endif 

  // Обработка конфигурации
  #ifdef EEPROM_FEATURE_ENABLE
    // Если есть датчик и конфигурация еще не опубликована, опубликовать ее
    if(sensor_found){
      if(!deviceConfig.isConfigPublished()){
        #ifdef DEBUG_MQTT
          Serial.println("Публикую конфигурацию в MQTT");
        #endif
        deviceConfig.setConfigPublished(publishConfig());
      }
    }
  #endif

  // Обработка логики выключателя
  #ifdef SWITCH_FEATURE_ENABLE
    
    // Опубликовать его состояние если оно не опубликовано или изменилось
    #ifdef MQTT_FEATURE_ENABLE
      bool currentSwitchState = isSwitchOn();
      if( !(lastSwitchState == currentSwitchState) ) {
        
        #ifdef DEBUG_ENABLE
          printf("Необходимо обновить состояние выключателя в MQTT (currentSwitchState=%s, lastSwitchState=%s)\n", (currentSwitchState ? "ON" : "OFF"), (lastSwitchState ? "ON" : "OFF"));
        #endif

        //if(client.connected()) {
        if(publishSwitchState()){
          
          #ifdef DEBUG_ENABLE
            printf("Выравниваю значения (currentSwitchState=%s -> lastSwitchState=%s)\n", (currentSwitchState ? "ON" : "OFF"), (lastSwitchState ? "ON" : "OFF"));
          #endif
          lastSwitchState = currentSwitchState;
        } else {       
          #ifdef DEBUG_ENABLE
            Serial.println("Нет подключения к MQTT.");
          #endif
        }
        //}
      }
    #endif

  #endif 

  // Обработка логики вентилятора
  #ifdef FAN_CONTROL_FEATURE_ENABLE
    
    // Обработать логику вентилятора если он не в ручном режиме       
    if (! deviceConfig.isOverrideMode()) {
      // Состояние выключателя зависит от показаний датчика, если он есть
      if (sensor_found) {
        if (sensorDataChanged) {
          
          Params curParams = deviceConfig.getConfig();
          bool curFanState = isSwitchOn();
          bool prefferedState  = prefferedFanState(curParams, temperature, humidity);
          if (curFanState != prefferedState) {
            setSwitchState(prefferedState); 
            #ifdef DEBUG_ENABLE
              printf("Состояние вентилятора изменилось c %s на %s\n", (curFanState ? "ON" : "OFF"), (prefferedState ? "ON" : "OFF") );
            #endif
          }
        
        }

        // double curTemperature = tempHumSensor.getTemperature();
        // double curHumidity = tempHumSensor.getHumidity();

        // Params curParams = deviceConfig.getConfig();
        // bool curFanState = isSwitchOn();
        // bool prefferedState  = prefferedFanState(curParams, curTemperature, curHumidity);
        
        // if (curFanState != prefferedState) {
        //   setSwitchState(prefferedState); 
        //   #ifdef DEBUG_ENABLE
        //     printf("Состояние вентилятора изменилось c %s на %s\n", (curFanState ? "ON" : "OFF"), (prefferedState ? "ON" : "OFF") );
        //   #endif
        // }

      }          
    }  
           
    // Обработать логику включения по задержке
    if(deviceConfig.isDelayMode()){
      // Пока таймер отсчета не закончился, убавлять его значение каждую секунду
      if(delayCounter > 0){
        
        unsigned long currentTime = millis();
        if (currentTime - lastTime >= 1000) {
          lastTime = currentTime;
          delayCounter--;
          
          //  Вывести обратный отсчет до включения вентилятора
          #ifdef DEBUG_ENABLE
            printf("Осталось %d секунд до включения вентилятора\n", delayCounter); 
          #endif
        
        }   
      } else {
        // Включить вентилятор
        setSwitchState(SWITCH_ON_LEVEL);

        #ifdef DEBUG_ENABLE
          printf("Выключатель включен по таймеру\n");
        #endif

        // Установить режим ручного управления
        deviceConfig.setSwitchOverrideMode(true);

        // Отключить режим управления по таймеру
        deviceConfig.setDelayMode(false);
      }
    
      // Опубликовать настройку задержки включения по таймеру если она не опубликована или изменилась
      if(!deviceConfig.isDelayPublished()){
        #ifdef DEBUG_MQTT
            Serial.println("Публикую настройку задержки включения вентилятора");
          #endif
        deviceConfig.setDelayPublished(piblishFanDelayState());  
      }
    
    }
  
    

  #endif
}
