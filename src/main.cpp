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
* FAN_CONTROL_FEATURE_ENABLED     - Управление выключателем на основании показаний датчика температуры/влажности
*/

// #define SWITCH_FEATURE_ENABLED
// #define SENSOR_TEMP_HUM_ENABLED

//#define MQTT_FEATURE_ENABLED
#define DEBUG_FEATURE_ENABLED
//#define OTA_FEATURE_ENABLED

//#define SENSOR_PRESENCE_ENABLED
//#define SENSOR_LIGHTING_ENABLED
#define EEPROM_FEATURE_ENABLED

#include <Arduino.h>

// Описание зависимостей
// #ifdef OTA_FEATURE_ENABLED
// #define WIFI_FEATURE_ENABLED
// #endif

// #ifdef SWITCH_FEATURE_ENABLED
// #define WIFI_FEATURE_ENABLED
// #define MQTT_FEATURE_ENABLED
// #endif

// #ifdef FAN_CONTROL_ENABLED
// #define WIFI_FEATURE_ENABLED
// #define MQTT_FEATURE_ENABLED
// #define SENSOR_TEMP_HUM_ENABLED
// #define SWITCH_FEATURE_ENABLED
// #endif


//Описание класса для датчика температуры/влажности
// #ifdef SENSOR_TEMP_HUM_ENABLED
// #define AHT10
// #include "Sensor.c"
// #endif

// Работа с выключателем
// #ifdef SWITCH_FEATURE_ENABLED
//   #define SWITCH_PIN     4  
// #endif

// Работа с реле управляемом температурой/влажностью или временем
// #ifdef FAN_CONTROL_FEATURE_ENABLED    
// #include "FanControl.c"
// #endif 



#include <EEPROM.h>

// Структура данных для хранения параметров управляемого вентилятора
struct Params { 
  double lowHum; 
  double highHum; 
  double lowTemp; 
  double highTemp;

  Params(double lowHum, double highHum, double lowTemp, double highTemp) : lowHum(lowHum), highHum(highHum), lowTemp(lowTemp), highTemp(highTemp) {}

  Params() : lowHum(0.0), highHum(0.0), lowTemp(0.0), highTemp(0.0) {} 

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
    if ( isnan(params.lowHum) || isnan(params.highHum) || isnan(params.lowTemp) || isnan(params.highTemp) ) {
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
    
    return true;
  }

  Config(Params newdata) { params = newdata;}

  Config() {}

};










  



// --------------------------------

void setup() {


#ifdef DEBUG_FEATURE_ENABLED
  Serial.begin(115200); 
  Serial.println();
  delay(5000);
  Serial.println("Выполняется инициализация устройства ... ");
#endif

Serial.println("Создается конфигурация ... ");
Config data;


Serial.printf("Конфигурация без ошибок? %s\n", data.isValid() ? "Да":"Нет");
printf("Текущие значения: lowTemp=%f, highTemp=%f, lowHum=%f, highHum=%f\n", data.get().lowTemp, data.get().highTemp, data.get().lowHum, data.get().highHum);


Serial.println("Читаю параметры из памяти");
data.read();
// printf("Текущие значения: lowTemp=%f, highTemp=%f, lowHum=%f, highHum=%f\n", data.params.lowTemp, data.params.highTemp, data.params.lowHum, data.params.highHum);
Serial.printf("Конфигурация без ошибок? %s\n", data.isValid() ? "Да":"Нет");

Serial.println("New config");
Params param2 (1,1,1,1);
Config data2(param2);


printf("Текущие значения: lowTemp=%f, highTemp=%f, lowHum=%f, highHum=%f\n", data2.get().lowTemp, data2.get().highTemp, data2.get().lowHum, data2.get().highHum);

// printf("Текущие значения: lowTemp=%f, highTemp=%f, lowHum=%f, highHum=%f\n", data.params.lowTemp, data.params.highTemp, data.params.lowHum, data.params.highHum);

// Serial.println("Читаю параметры из памяти");
// data.read();
// printf("Текущие значения: lowTemp=%f, highTemp=%f, lowHum=%f, highHum=%f\n", data.params.lowTemp, data.params.highTemp, data.params.lowHum, data.params.highHum);

Serial.printf("Конфигурация без ошибок? %s\n", data2.isValid() ? "Да":"Нет");


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
      
        #ifdef SENSOR_TEMP_HUM_ENABLED
          if(sensor_temperature_found){
            double curTemperature = tempHumSensor.getTemperature();
            double curHumidity = tempHumSensor.getHumidity();
            if(!(curTemperature == temperature) || (curHumidity == humidity)) {
              
              #ifdef DEBUG_FEATURE_ENABLED
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
