#include <Arduino.h>
#include "config_manager.h"
#include "led.h"
#include "logger.h"
#include "reset_btn.h"
#include "restart_manager.h"
#include "system_state.h"
#include "wdt_manager.h"

void setup() {
  delay(2000);
  Logger::getInstance().begin((LogLevel)XLOG_LEVEL, XLOG_CATEGORIES,
                              XLOG_USE_COLOR);

  XLOG_INFO(CAT_MAIN, "========================================");
  XLOG_INFO(CAT_MAIN, "SYSTEM STARTING...");
  XLOG_INFO(CAT_MAIN, "Version: %s", VERSION);
  XLOG_INFO(CAT_MAIN, "Device: %s (TYPE %d)", DEVICE_PREFIX, DEVICE_TYPE);
  XLOG_INFO(CAT_MAIN, "========================================");

  // ================================================================
  // 1. ИНИЦИАЛИЗАЦИЯ ВСЕХ СЛОЁВ
  // ================================================================
  system_state_init();
  wdt_init();
  g_configManager.init();
  // g_configManager.print();
  resetBtn_init();
  led_init();

  // Обработка наличия настройки wifi
  if (strlen(g_configManager.getWifiSsid()) < 1) {
    XLOG_INFO(CAT_MAIN, "Set provisioning mode due invalid WiFi configuration");
    system_state_set_bit(STATE_PROVISIONING);
    g_configManager.print();
    
  }
   XLOG_INFO(CAT_MAIN, "Setup complete");
}

void loop() {
  wdt_feed();

  // ================================================================
  // 1. ОБНОВЛЕНИЕ СОСТОЯНИЙ СЛОЁВ
  // ================================================================
  resetBtn_update();  // обновляет STATE_BUTTON_PRESSED

  // ... WiFi_update() — обновляет STATE_WIFI_OK
  // ... MQTT_update() — обновляет STATE_MQTT_OK
  // ... Provisioning_update() — обновляет STATE_PROVISIONING
  // ... Fan_update() — обновляет STATE_EMERGENCY

  // ================================================================
  // 2. ПОЛУЧАЕМ ТЕКУЩЕЕ СОСТОЯНИЕ УСТРОЙСТВА
  // ================================================================
  uint16_t bits = system_state_get_bits();

  // ================================================================
  // 3. RESET BUTTON (бизнес-логика) — только если кнопка нажата
  // ================================================================
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

  // ================================================================
  // 4. ОПРЕДЕЛЕНИЕ РЕЖИМА LED (ВСЯ ЛОГИКА ЗДЕСЬ)
  // ================================================================
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
    mode = LED_MORZE_E;  // ← WiFi ПЕРВЫЙ!
  } else if (!(bits & STATE_MQTT_OK)) {
    mode = LED_MORZE_I;  // ← MQTT ВТОРОЙ!
  } else {
    mode = LED_ON;
  }

  led_set_mode(mode);
  led_update();

  // ================================================================
  // 5. RESTART MANAGER
  // ================================================================
  restart_loop();
}