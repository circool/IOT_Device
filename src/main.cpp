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
  // 1. СИСТЕМНОЕ СОСТОЯНИЕ — INIT
  // ================================================================
  system_state_set(SystemState::INIT);

  // ================================================================
  // 2. ИНИЦИАЛИЗАЦИЯ СЛОЁВ
  // ================================================================
  wdt_init();
  g_configManager.init();
  g_configManager.print();
  resetBtn_init();
  led_init();

  XLOG_INFO(CAT_MAIN, "Setup complete");
}

void loop() {
  wdt_feed();

  // ================================================================
  // 1. СЛОЙ: RESET BUTTON
  // ================================================================
  ResetButtonStage btn = resetBtn_getState();

  if (system_state_get() != SystemState::RESTART_PENDING && btn == STAGE_3S) {
    XLOG_WARN(CAT_MAIN, "!!! RESET TRIGGERED !!!");
    wdt_stop();
    if (g_configManager.reset()) {
      system_state_set(SystemState::RESTART_PENDING);
      restart_request(500);
    }
  }

  // ================================================================
  // 2. СЛОЙ: КНОПКА — обновляет состояние
  // ================================================================
  if (system_state_get() != SystemState::RESTART_PENDING) {
    if (btn == PRESSED) {
      system_state_set(SystemState::MODE_1);
    } else if (btn == STAGE_1S) {
      system_state_set(SystemState::MODE_2);
    } else if (btn == STAGE_2S) {
      system_state_set(SystemState::MODE_3);
    }
  }

  // ================================================================
  // 3-6. ОСТАЛЬНЫЕ СЛОИ (заглушки)
  // ================================================================
  // TODO: добавить при реализации

  // ================================================================
  // 7. СЛОЙ: LED — читает состояние и обновляет физику
  // ================================================================
  led_update();

  // ================================================================
  // 8. СЛОЙ: RESTART MANAGER
  // ================================================================
  restart_loop();
}