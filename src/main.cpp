/**
 * @file main.cpp
 * @brief Оркестратор — связывает все слои
 */

#include <Arduino.h>
#include "debug_tools.h"
#include "device_controller.h"
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

static DeviceController deviceController;

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
  sensor_init();

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
  if (fan == nullptr) {
    XLOG_ERROR(CAT_MAIN, "Failed to allocate FanActuator!");
    // Не блокируем, просто падаем дальше — но с логом
  } else {
    XLOG_INFO(CAT_MAIN, "FanActuator allocated successfully!");
    fan->init(
        SWITCH_PIN, RELAY_ON_LEVEL, g_configManager.getBootState(),
        g_configManager.getSpeedPercent(), g_configManager.getAdaptiveMode(),
        g_configManager.getDelaySeconds(), g_configManager.getMaxOnTime());
#if FEATURE_MQTT_ENABLED == 1
    statusProvider = new FanWebStatusProvider(fan, &mqttManager);
#else
    statusProvider = new FanWebStatusProvider(fan);
#endif
  }

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

  // ===== ТРАНСПОРТ =====
  g_transport = createTransport();
  if (g_transport) {
    XLOG_INFO(CAT_MAIN, "Transport created: %s", g_transport->getName());
    g_transport->begin(&wifiClient, g_configManager.get());
  } else {
    XLOG_ERROR(CAT_MAIN, "No transport available!");
  }

  // ===== DEVICE CONTROLLER =====
#if DEVICE_TYPE == 1
  deviceController.init(g_configManager.get(), fan);
#elif DEVICE_TYPE == 3
  deviceController.init(g_configManager.get(), switchActuator);
#elif DEVICE_TYPE == 2
  deviceController.init(g_configManager.get());
#endif

  deviceController.set_state_callback(
      [](const operational_state_t* state, bool need_save) {
        if (need_save) {
          g_configManager.save();
        }
        // Публикация через Transport
        if (g_transport && g_transport->isConnected()) {
          g_transport->publishState(state->is_on);
#if DEVICE_TYPE == 1
          g_transport->publishSpeed(state->speed);
          g_transport->publishSensorControlMode(!state->manual_mode);
          g_transport->publishAdaptiveMode(state->adaptive_mode_active);
#endif
        }
      });

  // ===== MQTT КОЛБЭКИ (после инициализации DeviceController) =====
#if FEATURE_MQTT_ENABLED == 1
  mqttManager.onState(
      [](bool value, void* context) {
        deviceController.handle_command(CMD_SET_ACTUATOR, value ? 1.0f : 0.0f);
      },
      nullptr);
#if DEVICE_TYPE == 1
  mqttManager.onSpeed(
      [](int value, void* context) {
        deviceController.handle_command(CMD_SET_SPEED, (float)value);
      },
      nullptr);
#endif

  mqttManager.onDelaySec(
      [](int value, void* context) {
        deviceController.handle_command(CMD_SET_DELAY_SEC, (float)value);
      },
      nullptr);

  mqttManager.onMaxOnTime(
      [](uint32_t value, void* context) {
        deviceController.handle_command(CMD_SET_MAX_ON_TIME, (float)value);
      },
      nullptr);

#if DEVICE_TYPE == 1
  mqttManager.onAdaptiveMode(
      [](bool value, void* context) {
        deviceController.handle_command(CMD_SET_ADAPTIVE_MODE,
                                        value ? 1.0f : 0.0f);
      },
      nullptr);

  mqttManager.onLowTemp(
      [](float value, void* context) {
        deviceController.handle_command(CMD_SET_LOW_TEMP, value);
      },
      nullptr);

  mqttManager.onHighTemp(
      [](float value, void* context) {
        deviceController.handle_command(CMD_SET_HIGH_TEMP, value);
      },
      nullptr);

  mqttManager.onLowHum(
      [](float value, void* context) {
        deviceController.handle_command(CMD_SET_LOW_HUM, value);
      },
      nullptr);

  mqttManager.onHighHum(
      [](float value, void* context) {
        deviceController.handle_command(CMD_SET_HIGH_HUM, value);
      },
      nullptr);

  mqttManager.onSensorControlMode(
      [](bool value, void* context) {
        deviceController.handle_command(CMD_SET_SENSOR_CONTROL_MODE,
                                        value ? 1.0f : 0.0f);
      },
      nullptr);
#endif
#endif

  XLOG_INFO(CAT_MAIN, "Setup complete");
}

void loop() {
  wdt_feed();
  resetBtn_update();
  wifi_manager_update();

  uint16_t bits = system_state_get_bits();

  // Переход в режим провизионинга при потере соединения WiFi
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
#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI
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

          g_configManager.setDefaults();
          g_configManager.setWifiSsid(data->wifiSsid);
          g_configManager.setWifiPassword(data->wifiPassword);

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
#endif
  } else {
    led_set_mode(LED_ON);
  }
  led_update();

  // ===== ЖЕЛЕЗО =====
#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  sensor_update();
#endif

#if DEVICE_TYPE == 1
  fan->update();
#elif DEVICE_TYPE == 3
  switchActuator->update(g_configManager.getDelaySeconds(),
                         g_configManager.getMaxOnTime());
#endif

  // ===== БИЗНЕС-ЛОГИКА =====
  deviceController.update();

  // ===== ФУНКЦИОНАЛЬНЫЕ СЛОИ =====
  web_update();

  // ===== КОМАНДЫ ОТ WEB (/set) =====
  if (g_webCommandPending) {
    switch (g_webCommand.type) {
      case CMD_STATE:
        deviceController.set_state(g_webCommand.value.boolVal);
        break;
      case CMD_SPEED:
        deviceController.set_speed(g_webCommand.value.intVal);
        break;
      case CMD_MANUAL_MODE:
        deviceController.set_manual_mode(g_webCommand.value.boolVal);
        break;
    }
    g_webCommandPending = false;
  }

  // ===== СБРОС НАСТРОЕК ОТ WEB (/resetall) =====
  if (g_webRestartPending) {
    g_configManager.reset();
    g_webRestartPending = false;
    restart_request(500);
  }

  restart_update();

  if (g_transport && (bits & STATE_WIFI_OK)) {
    g_transport->update();
  }
}