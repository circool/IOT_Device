/**
 * @file main.cpp
 * @brief Оркестратор — связывает все слои системы
 * @version 0.13
 * @date 11.08.2026
 */

#include <Arduino.h>
#include "common_types.h"
#include "debug_tools.h"
#include "logger.h"
#include "settings.h"

#include "config_manager.h"
#include "device_controller.h"
#include "transport_factory.h"

#include "button_manager.h"
#include "led_manager.h"
#include "restart_manager.h"
#include "wdt_manager.h"

// ============================================================
// ГЛОБАЛЬНЫЕ ЭКЗЕМПЛЯРЫ
// ============================================================

DeviceController g_deviceController;
ConfigManager g_configManager;
DeviceConfig g_deviceConfig;
TransportConfig g_transportConfig;
ButtonManager g_buttonManager;
LedManager g_ledManager;
RestartManager g_restartManager;
Transport* g_transport = nullptr;

// ============================================================
// УКАЗАТЕЛИ НА СОСТОЯНИЕ
// ============================================================

const DeviceState* g_deviceStatePtr = nullptr;
const TransportState* g_transportStatePtr = nullptr;

// ============================================================
// LED
// ============================================================

void updateLED() {
  if (!g_transportStatePtr) {
    g_ledManager.setMode(LED_OFF);
    return;
  }

  if (g_transportStatePtr->setup_mode) {
    g_ledManager.setMode(LED_MORZE_S);
  } else if (g_transportStatePtr->gateway_ok) {
    g_ledManager.setMode(LED_ON);
  } else if (g_transportStatePtr->link_ok) {
    g_ledManager.setMode(LED_SLOW_BLINK);
  } else {
    g_ledManager.setMode(LED_OFF);
  }
}

// ============================================================
// КОЛБЭК ОТ DEVICE CONTROLLER
// ============================================================

void onDeviceStateChanged(uint32_t changes) {
  if (!g_transport)
    return;  // ← ЗАЩИТА!

  if (changes & STATE_CHANGED_IS_ON) {
    g_transport->publishState(g_deviceStatePtr->isOn);
    XLOG_INFO(CAT_MAIN, "Device state changed: isOn: %d",
              g_deviceStatePtr->isOn);
  }

  if (changes & STATE_CHANGED_SPEED) {
    g_transport->publishSpeed(g_deviceStatePtr->speed);
    XLOG_INFO(CAT_MAIN, "Device state changed: speed: %d%%",
              g_deviceStatePtr->speed);
  }

  if (changes & STATE_CHANGED_MANUAL_MODE) {
    XLOG_INFO(CAT_MAIN, "Device state changed: manualMode: %d",
              g_deviceStatePtr->manualMode);
  }

  if (changes & STATE_CHANGED_SENSOR_MODE) {
    g_transport->publishSensorControlMode(g_deviceStatePtr->sensorMode);
    XLOG_INFO(CAT_MAIN, "Device state changed: sensorMode: %d",
              g_deviceStatePtr->sensorMode);
  }

  if (changes & STATE_CHANGED_ADAPTIVE_MODE) {
    g_transport->publishAdaptiveMode(g_deviceStatePtr->adaptiveMode);
    XLOG_INFO(CAT_MAIN, "Device state changed: adaptiveMode: %d",
              g_deviceStatePtr->adaptiveMode);
  }

  if (changes & STATE_CHANGED_TEMPERATURE) {
    g_transport->publishSensor(g_deviceStatePtr->temperature,
                               g_deviceStatePtr->humidity);
    XLOG_INFO(CAT_MAIN, "Device state changed: temperature: %.1f°C",
              g_deviceStatePtr->temperature);
  }

  if (changes & STATE_CHANGED_HUMIDITY) {
    XLOG_INFO(CAT_MAIN, "Device state changed: humidity: %.1f%%",
              g_deviceStatePtr->humidity);
  }

  if (changes & STATE_CHANGED_SENSOR_VALID) {
    XLOG_INFO(CAT_MAIN, "Device state changed: sensorValid: %d",
              g_deviceStatePtr->sensorValid);
  }

  if (changes & STATE_CHANGED_DELAY_REMAIN) {
    g_transport->publishDelaySec(g_deviceStatePtr->delayRemain);
    XLOG_INFO(CAT_MAIN, "Device state changed: delayRemain: %lu",
              g_deviceStatePtr->delayRemain);
  }

  if (changes & STATE_CHANGED_MAX_ON_REMAIN) {
    g_transport->publishMaxOnTime(g_deviceStatePtr->maxOnRemain);
    XLOG_INFO(CAT_MAIN, "Device state changed: maxOnRemain: %lu",
              g_deviceStatePtr->maxOnRemain);
  }
}

// ============================================================
// КОЛБЭК ОТ ТРАНСПОРТА
// ============================================================

void onTransportEvent(const TransportEventData* event, void* context) {
  (void)context;

  if (!event) {
    XLOG_ERROR(CAT_MAIN, "Transport event: NULL event");
    return;
  }

  switch (event->event) {
    case STATE_CHANGED:
      XLOG_DEBUG(
          CAT_MAIN,
          "Transport state changed: link_ok=%d, gateway_ok=%d, setup_mode=%d",
          event->state->link_ok, event->state->gateway_ok,
          event->state->setup_mode);

      if (event->state && event->state->gateway_ok) {
        g_transport->publishFullState(g_deviceStatePtr);
        g_transport->publishConfig(&g_deviceConfig);
      }

      updateLED();
      break;

    case DEVICE_COMMAND:
      XLOG_DEBUG(CAT_MAIN, "Transport device command received");
      if (event->deviceState) {
        if (event->deviceState->isOn != g_deviceStatePtr->isOn) {
          g_deviceController.setOn(event->deviceState->isOn);
        }
        if (event->deviceState->speed != g_deviceStatePtr->speed) {
          g_deviceController.setSpeed(event->deviceState->speed);
        }
        if (event->deviceState->sensorMode != g_deviceStatePtr->sensorMode) {
          g_deviceController.setSensorMode(event->deviceState->sensorMode);
        }
        if (event->deviceState->adaptiveMode !=
            g_deviceStatePtr->adaptiveMode) {
          g_deviceController.setAdaptiveMode(event->deviceState->adaptiveMode);
        }
      }
      break;
    // @todo Как быть с одним параметром пришедшим например из  mqtt?
    case DEVICE_CONFIG:
      XLOG_DEBUG(CAT_MAIN, "Transport device config received");
      if (event->device) {
        memcpy(&g_deviceConfig, event->device, sizeof(DeviceConfig));
        g_configManager.set(g_deviceConfig);
        g_deviceController.init(&g_deviceConfig, g_deviceStatePtr);
        XLOG_INFO(CAT_MAIN, "Device config updated and applied");
      }
      break;

    case TRANSPORT_CONFIG:
      XLOG_DEBUG(CAT_MAIN, "Transport config received");
      if (event->transport) {
        memcpy(&g_transportConfig, event->transport, sizeof(TransportConfig));
        g_configManager.set(g_transportConfig);
        XLOG_INFO(CAT_MAIN, "Transport config saved, restarting transport...");
        g_restartManager.request(1000);
      }
      break;

    default:
      XLOG_WARN(CAT_MAIN, "Unknown transport event: %d", event->event);
      break;
  }
}

// ============================================================
// SETUP
// ============================================================

void setup() {
  delay(3000);
  log_init((LogLevel)XLOG_LEVEL, XLOG_CATEGORIES, XLOG_USE_COLOR);

  XLOG_INFO(CAT_MAIN, "========================================");
  XLOG_INFO(CAT_MAIN, "Device starting...");
  XLOG_INFO(CAT_MAIN, "Version: %s", VERSION);
  XLOG_INFO(CAT_MAIN, "Device: %s (TYPE %d)", DEVICE_PREFIX, DEVICE_TYPE);
  XLOG_INFO(CAT_MAIN, "========================================");

  wdtInit();
  XLOG_INFO(CAT_MAIN, "WDT initialized");

  XLOG_DEBUG(CAT_MAIN, "Init ConfigManager");
  g_configManager.init();

  if (!g_configManager.get(g_deviceConfig)) {
    XLOG_WARN(CAT_MAIN, "DeviceConfig invalid, using defaults");
    g_configManager.reset(g_deviceConfig);
  }

  if (!g_configManager.get(g_transportConfig)) {
    XLOG_WARN(CAT_MAIN, "TransportConfig invalid, using defaults");
    g_configManager.reset(g_transportConfig);
  }

  XLOG_DEBUG(CAT_MAIN, "Init DeviceController");
  g_deviceController.onStateChanged(onDeviceStateChanged);
  g_deviceController.init(&g_deviceConfig, g_deviceStatePtr);

  g_buttonManager.init();
  XLOG_INFO(CAT_MAIN, "Button initialized");

  g_ledManager.init();
  XLOG_INFO(CAT_MAIN, "LED initialized");

  XLOG_DEBUG(CAT_MAIN, "Creating transport...");
  g_transport = createTransport();

  if (g_transport) {
    XLOG_DEBUG(CAT_MAIN, "Transport created: %s", g_transport->getName());

    g_transport->setDeviceConfig(&g_deviceConfig);
    g_transport->setDeviceState(g_deviceStatePtr);
    g_transport->onEvent(onTransportEvent, nullptr);

    WiFiClient wifiClient;
    if (!g_transport->begin(&wifiClient, &g_transportConfig,
                            g_transportStatePtr)) {
      XLOG_WARN(CAT_MAIN, "Transport begin() returned false");
    } else {
      XLOG_INFO(CAT_MAIN, "Transport initialized successfully");
    }
  } else {
    XLOG_ERROR(CAT_MAIN, "Failed to create transport!");
  }

  updateLED();
  XLOG_INFO(CAT_MAIN, "=== Setup complete ===");
}

// ============================================================
// LOOP
// ============================================================

void loop() {
  wdtFeed();

  g_deviceController.update();
  g_transport->update();

  g_buttonManager.update();
  ButtonStage stage = g_buttonManager.getStage();

  if (stage == BUTTON_SHORT) {
    bool newState = !g_deviceStatePtr->isOn;
    XLOG_INFO(CAT_MAIN, "Button short press: toggle -> %s",
              newState ? "ON" : "OFF");
    g_deviceController.setOn(newState);
    if (newState) {
      g_deviceController.setSpeed(15);
    }
    g_buttonManager.clearEvent();
  }

  if (stage == BUTTON_HOLD) {
    XLOG_WARN(CAT_MAIN, "Button hold: factory reset");
    g_configManager.reset(g_deviceConfig);
    g_configManager.reset(g_transportConfig);
    g_buttonManager.clearEvent();
    g_restartManager.request(500);
  }

  updateLED();
  g_ledManager.update();
  g_restartManager.update();
}