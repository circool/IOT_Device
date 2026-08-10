/**
 * @file main.cpp
 * @brief Оркестратор — пример использования транспортной абстракции
 */

#include <Arduino.h>
#include "common_types.h"
#include "settings.h"

#include "config_manager.h"
#include "state_provider.h"

#include "button_manager.h"
#include "debug_tools.h"

#include "logger.h"
#include "restart_manager.h"
#include "led_manager.h"

ConfigManager g_configManager;
TransportConfig g_transportConfig;
DeviceConfig g_deviceConfig;
StateProvider g_stateProvider;
ButtonManager g_buttonManager;

void setup() {
  delay(3000);
  Logger::getInstance().init((LogLevel)XLOG_LEVEL, XLOG_CATEGORIES,
                             XLOG_USE_COLOR);

  XLOG_INFO(CAT_MAIN, "========================================");
  XLOG_INFO(CAT_MAIN, "PROTOTYPE STARTING...");
  XLOG_INFO(CAT_MAIN, "Version: %s", VERSION);
  XLOG_INFO(CAT_MAIN, "Device: %s (TYPE %d)", DEVICE_PREFIX, DEVICE_TYPE);
  XLOG_INFO(CAT_MAIN, "========================================");

  print_system_info();

  XLOG_DEBUG(CAT_MAIN, "Init config manager");
  g_configManager.init();
  if (!g_configManager.get(g_deviceConfig)) {
    g_configManager.reset(g_deviceConfig);
  };
  if (!g_configManager.get(g_transportConfig)) {
    g_configManager.reset(g_transportConfig);
  };
  printConfig(g_transportConfig, g_deviceConfig);

  if (!g_stateProvider.init()) {
    XLOG_ERROR(CAT_MAIN, "StateProvider init failed");
  }
  g_buttonManager.init();
  led_init();
  XLOG_INFO(CAT_MAIN, "Setup complete.");
}

void loop() {
  delay(10);
  led_update();
  g_buttonManager.update();
  
  ButtonStage stage = g_buttonManager.getStage();
  if (stage == BUTTON_IDLE) {
    // Кнопка не нажата — логика по StateProvider
    if (g_stateProvider.link_ok && g_stateProvider.gateway_ok) {
      led_set_mode(LED_ON);
    } else if (!g_stateProvider.link_ok) {
      led_set_mode(LED_MORZE_E);
    } else if (!g_stateProvider.setup_mode) {
      led_set_mode(LED_MORZE_I);
    } else {
      led_set_mode(LED_OFF);
    }
  } else {
    // Кнопка нажата — логика по стадии
    switch (stage) {
      case BUTTON_MID:
        led_set_mode(LED_MORZE_I);
        break;
      case BUTTON_LONG:
      case BUTTON_WARN:
        led_set_mode(LED_MORZE_S);
        break;
      case BUTTON_HOLD:
        // Сброс
        break;
      default:
        break;
    }
  }
  // Обработка события короткого нажатия
  if (stage == BUTTON_SHORT) {
    XLOG_INFO(CAT_MAIN, "Short press — toggling actuator");
    // device_controller_toggle(); // или другая команда
    // Сброс стадии, чтобы не обрабатывать повторно
    g_buttonManager.clearEvent();
  }

  restartUpdate();
}