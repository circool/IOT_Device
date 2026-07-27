#include <Arduino.h>
#include "debug_tools.h"
#include "fan_actuator.h"
#include "led.h"
#include "logger.h"
#include "mqtt.h"
#include "provisioning.h"
#include "reset_btn.h"
#include "restart_manager.h"
#include "settings.h"
#include "system_state.h"
#include "transport_factory.h"
#include "wdt_manager.h"
#include "web.h"
#include "wifi_manager.h"

WiFiClient wifiClient;


static FanActuator* fan = nullptr;

#include "switch_actuator.h"
static SwitchActuator* switchActuator = nullptr;

#include "mqtt.h"

#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI
static unsigned long wifi_fail_start = 0;
#endif

#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI
#include "web_status_provider.h"
static bool web_started = false;
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
  XLOG_INFO(CAT_MAIN, "Provisioning method: %d", PROVISIONING_METHOD);
  XLOG_INFO(CAT_MAIN, "========================================");

  system_state_init();
  wdt_init();
  g_configManager.init();
  g_configManager.print();
  resetBtn_init();
  led_init();
  wifi_scan_and_log(g_configManager.getWifiSsid());

  // Режим первоначальной настройки (провизионинг) или обычная работа
  if (strlen(g_configManager.getWifiSsid()) < 1) {
    XLOG_INFO(CAT_MAIN, "Set provisioning mode due invalid WiFi configuration");
    system_state_set_bit(STATE_PROVISIONING);
    startProvisioning();
  } else {
#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI
    wifi_manager_init();
    wifi_manager_begin();

#endif
  }

#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI
#if DEVICE_TYPE == 1
  fan = new FanActuator();
  fan->init(SWITCH_PIN, RELAY_ON_LEVEL, g_configManager.getBootState(),
            g_configManager.getSpeedPercent(),
            g_configManager.getAdaptiveMode(),
            g_configManager.getDelaySeconds(), g_configManager.getMaxOnTime());
#if FEATURE_MQTT_ENABLED == 1
  statusProvider = new FanWebStatusProvider(fan, &mqttManager);
#else
  statusProvider = new FanWebStatusProvider(fan);
#endif  // FEATURE_MQTT_ENABLED

#elif DEVICE_TYPE == 2

#if FEATURE_MQTT_ENABLED == 1
  statusProvider = new SensorWebStatusProvider(&mqttManager);
#else
  statusProvider = new SensorWebStatusProvider();
#endif  // FEATURE_MQTT_ENABLED

#elif DEVICE_TYPE == 3
  switchActuator = new SwitchActuator();
  switchActuator->init(
      SWITCH_PIN, RELAY_ON_LEVEL, g_configManager.getBootState(),
      g_configManager.getDelaySeconds(), g_configManager.getMaxOnTime());

#if FEATURE_MQTT_ENABLED == 1
  statusProvider = new SwitchWebStatusProvider(switchActuator, &mqttManager);
#else
  statusProvider = new SwitchWebStatusProvider(switchActuator);
#endif  // FEATURE_MQTT_ENABLED
#endif  // DEVICE_TYPE == 3
  web_registerStatusProvider(statusProvider);
#endif  // TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI

  g_transport = createTransport();
  if (g_transport) {
    XLOG_INFO(CAT_MAIN, "Transport created: %s", g_transport->getName());
    g_transport->begin(&wifiClient, g_configManager.get());
  } else {
    XLOG_ERROR(CAT_MAIN, "No transport available!");
  }

  XLOG_INFO(CAT_MAIN, "Setup complete");
}

void loop() {
  wdt_feed();
  resetBtn_update();
  wifi_manager_loop();

  uint16_t bits = system_state_get_bits();

  // Переход в режим провизионинга при потеле соединения WiFi
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
#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI  // Для провизионинга в режиме AP нужен сервер
    if (!web_started && (bits & STATE_WIFI_OK)) {
      web_init(false);
      web_started = true;
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

#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI
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
#if FEATURE_MQTT_ENABLED
  } else if (!(bits & STATE_MQTT_OK)) {
    led_set_mode(LED_MORZE_I);
#endif  // FEATURE_MQTT_ENABLED
  } else {
    led_set_mode(LED_ON);
  }
  led_loop();

  // Функциональные слои
  web_loop();
  // Обработка команд от Web (/set)
  if (g_webCommandPending) {
    // DeviceController должен быть создан и инициализирован
    // (в текущей реализации DeviceController ещё нет, поэтому временно:
    //   - CMD_STATE → fan->set(value)
    //   - CMD_SPEED → fan->setSpeed(value)
    //   - CMD_MANUAL_MODE → fan->setAdaptiveMode(!value) или логика в main)
    switch (g_webCommand.type) {
      case CMD_STATE:
        // deviceController.setOperationalParam(PARAM_STATE,
        // g_webCommand.value.boolVal);
        if (fan)
          fan->set(g_webCommand.value.boolVal, true);
        break;
      case CMD_SPEED:
        if (fan)
          fan->setSpeed(g_webCommand.value.intVal, true);
        break;
      case CMD_MANUAL_MODE:
        // deviceController.setOperationalParam(PARAM_MANUAL_MODE,
        // g_webCommand.value.boolVal);
        if (fan)
          fan->setAdaptiveMode(!g_webCommand.value.boolVal);
        break;
    }
    g_webCommandPending = false;
  }

  // Обработка сброса настроек от Web (/resetall)
  if (g_webRestartPending) {
    g_configManager.reset();
    g_webRestartPending = false;
    restart_request(500);
  }

  restart_loop();

  if (g_transport && (bits & STATE_WIFI_OK)) {
    g_transport->process();
  }
}