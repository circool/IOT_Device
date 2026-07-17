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

  if (strlen(g_configManager.getWifiSsid()) < 1) {
    XLOG_INFO(CAT_MAIN, "Set provisioning mode due invalid WiFi configuration");
    system_state_set_bit(STATE_PROVISIONING);
    startProvisioning();
  }
  XLOG_INFO(CAT_MAIN, "Setup complete");
}

void loop() {
  wdt_feed();
  resetBtn_update();

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
  uint16_t bits = system_state_get_bits();

  // Кнопка сброса
  if ((bits & STATE_BUTTON_PRESSED) && !(bits & STATE_RESTART)) {
    ResetButtonStage stage = resetBtn_get_stage();
    if (stage == STAGE_3S) {
      XLOG_WARN(CAT_MAIN, "!!! RESET TRIGGERED !!!");
      wdt_stop();
      if (g_configManager.reset()) {
        system_state_set_bit(STATE_RESTART);
        restart_request(500);
      }
    }
  }

  // Индикатор LED
  LedMode mode = LED_OFF;
  if (bits & STATE_RESTART) {
    mode = LED_OFF;
  } else if (bits & STATE_EMERGENCY) {
    mode = LED_SLOW_BLINK;
  } else if (bits & STATE_PROVISIONING) {
    mode = LED_MORZE_S;
  } else if (bits & STATE_BUTTON_PRESSED) {
    ResetButtonStage stage = resetBtn_get_stage();
    if (stage == STAGE_3S || stage == STAGE_2S) {
      mode = LED_MORZE_S;
    } else if (stage == STAGE_1S) {
      mode = LED_MORZE_I;
    } else {
      mode = LED_MORZE_E;
    }
  } else if (!(bits & STATE_WIFI_OK)) {
    mode = LED_MORZE_E;
  } else if (!(bits & STATE_MQTT_OK)) {
    mode = LED_MORZE_I;
  } else {
    mode = LED_ON;
  }

  
  // Функциональные слои - периодические 
  web_update();

  led_set_mode(mode);
  led_loop();
  restart_loop();
}