#include <Arduino.h>
#include "config_manager.h"
#include "led.h"
#include "logger.h"
#include "provisioning.h"
#include "reset_btn.h"
#include "restart_manager.h"
#include "system_state.h"
#include "wdt_manager.h"
#include "web.h"
#include "debug_tools.h"
#include "wifi_manager.h"

#include "fan_actuator.h"
static FanActuator* fan = nullptr;

#include "switch_actuator.h"
static SwitchActuator* switchActuator = nullptr;

#include "sensor.h"
#include "mqtt.h"



#if FEATURE_WIFI_ENABLED == 1
static unsigned long wifi_fail_start = 0;   // для отслеживания времени без WiFi
#endif

#if FEATURE_WEB_ENABLED
static bool web_started = false;            // для отслеживания состояния cервера web


static IWebStatusProvider* statusProvider = nullptr;
#endif

void setup() {
  delay(2000);
  Logger::getInstance().begin((LogLevel)XLOG_LEVEL, XLOG_CATEGORIES,
                              XLOG_USE_COLOR);
  print_system_info();
  XLOG_INFO(CAT_MAIN, "========================================");
  XLOG_INFO(CAT_MAIN, "SYSTEM STARTING...");
  XLOG_INFO(CAT_MAIN, "Version: %s", VERSION);
  XLOG_INFO(CAT_MAIN, "Device: %s (TYPE %d)", DEVICE_PREFIX, DEVICE_TYPE);
  XLOG_INFO(CAT_MAIN, "========================================");

  system_state_init();
  wdt_init();
  g_configManager.init();
  g_configManager.print();
  resetBtn_init();
  led_init();
#if SCANNING_WIFI_ENABLED == 1
  wifi_scan_and_log(g_configManager.getWifiSsid());
#endif
  // Режим первоначальной настройки (провизионинг) или обычная работа
  if (strlen(g_configManager.getWifiSsid()) < 1) {
    XLOG_INFO(CAT_MAIN, "Set provisioning mode due invalid WiFi configuration");
    system_state_set_bit(STATE_PROVISIONING);
    XLOG_DEBUG(CAT_MAIN, "Calling startProvisioning");
    startProvisioning();
  } else {
#if FEATURE_WIFI_ENABLED == 1
    wifi_manager_init();
    wifi_manager_begin();
#endif
  }

#if FEATURE_WEB_ENABLED == 1

#if DEVICE_TYPE == 1
#if FEATURE_MQTT_ENABLED == 1
  statusProvider = new FanWebStatusProvider(fan, nullptr, &mqttManager);
#else
  statusProvider = new FanWebStatusProvider(fan, nullptr);
#endif
  // web_registerStatusProvider(statusProvider);

#elif DEVICE_TYPE == 2

#if FEATURE_MQTT_ENABLED == 1
  statusProvider = new SensorWebStatusProvider(&mqttManager);
#else
  statusProvider = new SensorWebStatusProvider();
#endif  // FEATURE_MQTT_ENABLED



#elif DEVICE_TYPE == 3

#if FEATURE_MQTT_ENABLED == 1
  statusProvider = new SwitchWebStatusProvider(switchActuator, &mqttManager);
#else
  statusProvider = new SwitchWebStatusProvider(switchActuator);
#endif  // FEATURE_MQTT_ENABLED
  


#endif  // DEVICE_TYPE == 3
  
  web_registerStatusProvider(statusProvider);

#endif // FEATURE_WEB_ENABLED

  XLOG_INFO(CAT_MAIN, "Setup complete");
}

void loop() {
  wdt_feed();
  resetBtn_update();
  wifi_manager_loop();

  uint16_t bits = system_state_get_bits();
  
  // WiFi
  if (!(bits & STATE_WIFI_OK) && !(bits & STATE_PROVISIONING)) {
    if (wifi_fail_start == 0) {
      wifi_fail_start = millis();
    } else if (millis() - wifi_fail_start > WIFI_FALLBACK_TIMEOUT_MS) {
      system_state_set_bit(STATE_PROVISIONING);
      XLOG_DEBUG(
          CAT_MAIN,
          "Calling startProvisioning due WIFI_FALLBACK_TIMEOUT_MS expired");
      startProvisioning();
      
    }
  } else {
    wifi_fail_start = 0;
#if FEATURE_WEB_ENABLED == 1
    if (!web_started && (bits & STATE_WIFI_OK)) {
      web_init(false);
      web_started = true;
      XLOG_INFO(CAT_MAIN, "Web server started (normal mode)");
    }
#endif
  }

  // Провизионинг
  if (system_state_has_bit(STATE_PROVISIONING)) {
    ProvisioningManager::getInstance().update();

    if (isProvisioningComplete()) {
      auto method = getProvisioningMethod();

      if (method == ProvisioningMethod::FAILED) {
        XLOG_ERROR(CAT_MAIN, "Provisioning FAILED!");
      } else {
        const auto* data = ProvisioningManager::getInstance().getData();
        
        #if FEATURE_WIFI_ENABLED == 1
        if (data && strlen(data->wifiSsid) > 0) {
          XLOG_INFO(CAT_MAIN, "Provisioning complete! SSID: %s",
                    data->wifiSsid);

          // 1. Применяем дефолты
          g_configManager.setDefaults();

          // 2. Перезаписываем WiFi
          g_configManager.setWifiSsid(data->wifiSsid);
          g_configManager.setWifiPassword(data->wifiPassword);

          // 3. Сохраняем
          if (g_configManager.save()) {
            system_state_clear_bit(STATE_PROVISIONING);
            restart_request(500);
          } else {
            XLOG_ERROR(CAT_MAIN, "Failed to save config!");
          }
        }
        #endif
      }
    }
  }


  // Состояние WiFi

  // Кнопка сброса
  ResetButtonStage stage = resetBtn_get_stage();
  if ((bits & STATE_BUTTON_PRESSED) && !(bits & STATE_RESTART)) {   
    if (stage == STAGE_3S) {
      XLOG_WARN(CAT_MAIN, "Reset button triggered.");
      wdt_stop();
      if (g_configManager.reset()) {
        system_state_set_bit(STATE_RESTART);
        restart_request(500);
      }
    }
  }

  // Индикатор LED
  if (bits & STATE_RESTART) {
    led_set_mode(LED_OFF);
  } else if (bits & STATE_EMERGENCY) {
   led_set_mode(LED_SLOW_BLINK);
  } else if (bits & STATE_PROVISIONING) {
    led_set_mode(LED_MORZE_S);
  } else if (bits & STATE_BUTTON_PRESSED) {
    if (stage == STAGE_3S || stage == STAGE_2S) {
      led_set_mode(LED_MORZE_S);
    } else if (stage == STAGE_1S) {
      led_set_mode(LED_MORZE_I);
    } else {
      led_set_mode(LED_MORZE_E);
    }
  } else if (!(bits & STATE_WIFI_OK)) {
    led_set_mode(LED_MORZE_E);
  } else if (!(bits & STATE_MQTT_OK)) {
    led_set_mode(LED_MORZE_I);
  } else {
    led_set_mode(LED_ON);
  }
  led_loop();
  
  // Функциональные слои
  



  web_update();
  restart_loop();
}