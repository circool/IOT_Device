/* Здесь задаются особенности реализации устройства
* MQTT_FEATURE_ENABLED    - Возможность контроля по протоколу mqtt (зависит от WIFI_FEATURE_ENABLED)
* WIFI_FEATURE_ENABLED    - Возможность подключения к сети wifi
* DEBUG_FEATURE_ENABLED   - Вывод сообщений в консоль
* OTA_FEATURE_ENABLED     - Возможность обновления прошивки по воздуху (зависит от WIFI_FEATURE_ENABLED)
* SWITCH_FEATURE_ENABLED  - Возможность управления состоянием выключателя
* SENSOR_TEMP_HUM_ENABLED - Возможность измерения температуры и влажности
* SENSOR_PRESENCE_ENABLED - Возможность использования датчика присутствия
* SENSOR_LIGHTING_ENABLED - Возможность использования датчика освещения
* EEPROM_FEATURE_ENABLED  - Возможность сохранения настроек в EEPROM 
* и так далее
*/
//#define SWITCH_FEATURE_ENABLED
//#define MQTT_FEATURE_ENABLED
#define DEBUG_FEATURE_ENABLED
//#define WIFI_FEATURE_ENABLED
//#define OTA_FEATURE_ENABLED
#define SENSOR_TEMP_HUM_ENABLED
//#define SENSOR_PRESENCE_ENABLED
//#define SENSOR_LIGHTING_ENABLED
//#define EEPROM_FEATURE_ENABLED

#include <Arduino.h>


#ifdef OTA_FEATURE_ENABLED
  // Для работы OTA необходим wifi
  #define WIFI_FEATURE_ENABLED
#endif

#ifdef MQTT_FEATURE_ENABLED
  // Для работы mqtt необходим wifi
  #define WIFI_FEATURE_ENABLED
#endif

////////////////////////////////////////////////////////////////
#ifdef SENSOR_TEMP_HUM_ENABLED
bool sensor_temperature_found = false;
  #define AHT10
  // Переменные для работы с датчиком температуры и влажности
  #define TEMPERATURE_LOW_RANGE   25            	// Значение температуры, при снижении до которого вентилятор следует выключить
  #define TEMPERATURE_HIGH_RANGE  27            	// Значение температуры, при превышении которого следует включить вентилятор
  #define HUMIDITY_LOW_RANGE      35            	// Значение влажности, при снижении до которого вентилятор следует выключить
  #define HUMIDITY_HIGH_RANGE     40            	// Значение влажности, при превышении которого следует включить вентилятор  
  #define SENSOR_DURATION         10              // Рекомендуемая частота опроса датчика

  #ifdef DHT11
    #define SENSOR_TYPE           DHT11
    #define SENSOR_PIN            5
    #include <DHT.h>  
  #endif

  #ifdef AHT10
    #define SENSOR_TYPE   AHT10
    // #include <Wire.h>
    #include <Adafruit_AHTX0.h>
    Adafruit_AHTX0 tempHumSensor;
    sensors_event_t humidity, temperature;
  #endif
  
  



  #ifdef SWITCH_FEATURE_ENABLED
    #ifdef SENSOR_TEMP_HUM_ENABLED
      #ifdef SWITCH_FEATURE_ENABLED
        
        // double tempLowRange =  TEMPERATURE_LOW_RANGE;
        // double tempHighRange = TEMPERATURE_HIGH_RANGE;
        // double humLowRange = HUMIDITY_LOW_RANGE;
        // double humHighRange = HUMIDITY_HIGH_RANGE;
        
        struct RangeSettings {
            double lowHumRange; //
            double highHumRange; //
            double lowTempRange; //
            double highTempRange; // 
          RangeSettings(double lowHum=HUMIDITY_LOW_RANGE, double highHum=HUMIDITY_HIGH_RANGE, double lowTemp=TEMPERATURE_LOW_RANGE, double highTemp=TEMPERATURE_HIGH_RANGE)
            : lowHumRange(lowHum), highHumRange(highHum), lowTempRange(lowTemp), highTempRange(highTemp)
              {
                // Additional initialization code, if needed
              }    
        };
        
        RangeSettings configuration;
      #endif 
    #endif  
      
  #endif


#endif
#ifdef SWITCH_FEATURE_ENABLED
  // Переменные для работы с выключателем
  // Для работы реле его нужно указать, заодно укажем какая команда для него будет управляющей
  // Подключение реле
  #ifdef ESP8266
    #define SWITCH_PIN     0
  #elif defined(EPS32)
    #define SWITCH_PIN     1
  #else
    #error "Unsupported platform. Define ESP8266 or ESP32."
  #endif
  
  #ifdef MQTT_FEATURE_ENABLED
    // Управление выключателем по протоколу MQTT
    char switchStateTopic[31]; 
    char switchControlTopic[27];
    
    #ifndef SWITCH_ON_VALUE 
      #define SWITCH_ON_VALUE "ON"
    #endif

    #ifndef SWITCH_OFF_VALUE 
      #define SWITCH_OFF_VALUE "OFF"
    #endif

  #endif

#endif



#ifdef EEPROM_FEATURE_ENABLED
  // TODO: Определение списка параметров сохраняемых в EEPROM
     
#endif

#ifdef WIFI_FEATURE_ENABLED
  #include "credentials.h"
  // Переменные и процедуры для работы с соединением wifi
  byte mac[6]; 
  #ifdef ESP8266 
    #include <ESP8266WiFi.h>
  #elif defined(ESP32)
    #include <WiFi.h>
  #else
    #error "Unsupported platform. Define ESP8266 or ESP32."
  #endif
  
  const char* ssid = SSID_NAME;
  const char* wifiPassword = WIFI_PASSWORD;
  
  unsigned long lastWiFiCheckTime = 0;
  const unsigned long wifiCheckInterval = 10000;  // Проверка WiFi каждые 10 секунд
  boolean needShowConnectionInfo = true;          // Показывать сообщение о подключении только один раз

  void checkWiFiConnection() {
  
  unsigned long currentTime = millis();
  if (currentTime - lastWiFiCheckTime >= wifiCheckInterval) {
    lastWiFiCheckTime = currentTime;
    
    if (WiFi.status() != WL_CONNECTED) {
      needShowConnectionInfo = true;

      #ifdef DEBUG_FEATURE_ENABLED
        printf("Выполняю попытку соединиться с сетью WiFi...\n");
      #endif
      WiFi.disconnect();
      WiFi.reconnect();
    } else {
      if(needShowConnectionInfo){
        Serial.print("WiFi соединение установлено, IP адрес: "); Serial.println(WiFi.localIP());
        needShowConnectionInfo = false;
      }
      
    }
  } 
}


#endif

#ifdef MQTT_FEATURE_ENABLED
// Переменные для работы с соединением mqtt
    WiFiClient espClient;
    #include <PubSubClient.h>
    PubSubClient client(espClient);
    const char mqttServer[]   = MQTT_ADDRESS;
    const char mqttUser[]     = MQTT_USER;
    const char mqttPassword[] = MQTT_PASSWORD;
    char mqttId[18] ;
    char lastWillTopic[25]; 

  /**
   * @brief Вызывается при получении даных из подписанного канала MQTT
   * @param topic 
   * @param payload 
   * @param length 
   * 
   * Декодирует команду и выполняет соответствующее действие, после чего публикует результат выполнения
   */

  // Function to process switch commands
  void processSwitchCommand(const String& receivedPayload) {
    #ifdef SWITCH_FEATURE_ENABLED
      if (receivedPayload == "1" || receivedPayload == SWITCH_ON_VALUE) {
        digitalWrite(SWITCH_PIN, LOW); 
        client.publish(switchControlTopic, SWITCH_ON_VALUE);
      } else if (receivedPayload == "0" || receivedPayload == SWITCH_OFF_VALUE) {     
        digitalWrite(SWITCH_PIN, HIGH); 
        client.publish(switchControlTopic, SWITCH_OFF_VALUE);
      } else {
        #ifdef DEBUG_FEATURE_ENABLED
          Serial.println("SWITCH: Unknown command (" + receivedPayload + ")");
        #endif
      }
    #endif
  }

  // Callback function to handle incoming MQTT messages
  void callback (char* topic, byte* payload, unsigned int length) {
    
    
    String recievedPayload;
    for (unsigned int i = 0; i < length; i++) {
      recievedPayload += (char)payload[i];
    }
    #ifdef DEBUG_FEATURE_ENABLED
      printf("Получена команда '%s' из топика '%s'.\n", recievedPayload.c_str(), topic );
    #endif


    // Обработка команд для SWITCH
    #ifdef SWITCH_FEATURE_ENABLED
    
      if(strcmp(topic, switchControlTopic) == 0){
        if (recievedPayload == "1" || recievedPayload == SWITCH_ON_VALUE) {
          digitalWrite(SWITCH_PIN, LOW); 
          //switch = true;
          client.publish(switchStateTopic, SWITCH_ON_VALUE);
        } else if (recievedPayload == "0" || recievedPayload == SWITCH_OFF_VALUE) {     
          digitalWrite(SWITCH_PIN, HIGH); 
          //switch = false;
          client.publish(switchStateTopic, SWITCH_OFF_VALUE);
        } else {
          #ifdef DEBUG_FEATURE_ENABLED
            Serial.println("SWITCH: Unknown command (" + recievedPayload + ")");
          #endif
        }
      }
    #endif


  }

    /**
   * @brief Эта процедура вызывается при потере соединения с брокером MQTT
   * В случае успешного подключения:
   *  - формируется и публикуется LastWill сообщение "online"
   *  - публикуется текущий статус выключателя
   */
  void checkMqttConnection() {
      const char* lastWillMessage = "offline";
      if (!client.connected()) {     
        
        #ifdef DEBUG_FEATURE_ENABLED
          printf("Попытка подключиться к mqtt брокеру по адресу %s с идентификатором %s.\n", mqttServer, mqttId); 
          // Serial.print(""); 
          // Serial.print(); Serial.println(".");
        #endif

        if (client.connect(mqttId,mqttUser,mqttPassword,lastWillTopic,1, true,  lastWillMessage)) {
          #ifdef DEBUG_FEATURE_ENABLED
            printf("Подключение к mqtt брокеру установлено.\n");     
          #endif

          // Обновить статус канала
          client.publish(lastWillTopic, "online");
          
          #ifdef SWITCH_FEATURE_ENABLED
            // Опубликовать статус реле
            boolean switchState = digitalRead(SWITCH_PIN);
            if(switchState){
              client.publish(switchStateTopic, SWITCH_ON_VALUE);
            } else {
              client.publish(switchStateTopic, SWITCH_OFF_VALUE);
            }       
          #endif

          client.subscribe(switchControlTopic);              // Подписаться на получение команд          
        } 
        else 
        {
          #ifdef DEBUG_FEATURE_ENABLED
            printf("Подключение к mqtt брокеру не установлено ( rc=%d).\n",client.state());
            printf("Повторная попытка через 5 секунд\n");   
          #endif 
          delay(5000);
        }
      } else {
        client.loop();
      }
    }

#endif

#ifdef OTA_FEATURE_ENABLED
// Обновление
  #ifdef ESP8266
    #include <ESP8266WiFi.h>
    #include <ESPAsyncTCP.h>
  #elif defined(ESP32)
    #include <WiFi.h>
    #include <AsyncTCP.h>
  #endif

  #include <ESPAsyncWebServer.h>
  #include <AsyncElegantOTA.h>  
  AsyncWebServer webServer(80);

#endif

// --------------------------------

void setup() {
  
  

  #ifdef DEBUG_FEATURE_ENABLED
    Serial.begin(115200); 
    Serial.println();
    delay(5000);
    Serial.println("Выполняется инициализация устройства ... ");
  #endif

  #ifdef EEPROM_FEATURE_ENABLED
    // TODO: Прочитать настройки из EEPROM
  #endif

  #ifdef SENSOR_TEMP_HUM_ENABLED
    
    #ifdef DEBUG_FEATURE_ENABLED
      Serial.print("Инициализация датчика температуры и влажности: ");
    #endif

    if(tempHumSensor.begin()){
      sensor_temperature_found = true;
      
      #ifdef DEBUG_FEATURE_ENABLED
        Serial.println("успешно");
        
        tempHumSensor.getEvent(&humidity, &temperature);
        Serial.print("Текущие показания температуры: "); Serial.print(temperature.temperature); Serial.println(" ºC");
        Serial.print("Текущие показания влажности: "); Serial.print(humidity.relative_humidity); Serial.println(" %");
        
      
      
      #endif

    } else {
      sensor_temperature_found = false;
      
      #ifdef DEBUG_FEATURE_ENABLED
        Serial.println("датчик не найден");
      #endif  
    
    };
  #endif


  #ifdef WIFI_FEATURE_ENABLED
   
    #ifdef DEBUG_FEATURE_ENABLED
      printf("Инициализия соединения WiFi.\n");
    #endif
    
    WiFi.begin(ssid, wifiPassword);
    
    #ifdef DEBUG_FEATURE_ENABLED
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
  
  #ifdef SWITCH_FEATURE_ENABLED
    pinMode(SWITCH_PIN, OUTPUT); 
  #endif

  #ifdef MQTT_FEATURE_ENABLED
    client.setServer(mqttServer, 1883);
    client.setCallback(callback);
    snprintf(mqttId, sizeof(mqttId), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    snprintf(lastWillTopic, sizeof(lastWillTopic), "%s/status", mqttId);
    
    #ifdef SWITCH_FEATURE_ENABLED
      digitalWrite(SWITCH_PIN, HIGH);
      snprintf(switchStateTopic, sizeof(switchStateTopic), "%s/switch/state", mqttId);
      snprintf(switchControlTopic, sizeof(switchControlTopic), "%s/switch/c", mqttId);
    #endif
  
  #endif

  #ifdef OTA_FEATURE_ENABLED
    
    #ifdef DEBUG_FEATURE_ENABLED
      printf("Инициализация HTTP cервера для реализации функций OTA\n");
    #endif
    
    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
      request->send(200, "text/plain", "ESP device");
    });

    AsyncElegantOTA.begin(&webServer);
    webServer.begin();
    
    #ifdef DEBUG_FEATURE_ENABLED
      printf("Инициализация HTTP cервера окончена.\n");
    #endif
  
  #endif

  #ifdef DEBUG_FEATURE_ENABLED
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
      #endif

    #endif

    #ifdef SENSOR_TEMP_HUM_ENABLED
      // temperature = tempHumSensor.getTemperature();
      // humidity = tempHumSensor.getHumidity();
    #endif 
    
    #ifdef SWITCH_FEATURE_ENABLED
      calculateSwitchState(configuration);  
    #endif
       

    
}


