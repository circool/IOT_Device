#include "ble.h"

// Глобальные переменные
static BleWifiConfig receivedConfig = {0};
static bool configReceived = false;
static bool isActive = false;
static ble_config_callback_t configCallback = nullptr;
static wifi_event_id_t eventId = 0;

// Обработчик Arduino-событий
static void onWiFiEvent(arduino_event_t* event) {
  if (event == nullptr)
    return;

  switch (event->event_id) {
    case ARDUINO_EVENT_PROV_START:
      Serial.println("BLE: Provisioning started");
      isActive = true;
      break;

    case ARDUINO_EVENT_PROV_CRED_RECV: {
      Serial.println("BLE: Credentials received!");

      // Данные в event->event_info.prov_cred_recv
      wifi_sta_config_t* cred = &event->event_info.prov_cred_recv;

      if (cred != NULL) {
        // Копируем SSID
        memset(receivedConfig.wifiSsid, 0, sizeof(receivedConfig.wifiSsid));
        strncpy(receivedConfig.wifiSsid, (char*)cred->ssid,
                sizeof(receivedConfig.wifiSsid) - 1);
        receivedConfig.wifiSsid[sizeof(receivedConfig.wifiSsid) - 1] = '\0';

        // Копируем пароль
        memset(receivedConfig.wifiPassword, 0,
               sizeof(receivedConfig.wifiPassword));
        strncpy(receivedConfig.wifiPassword, (char*)cred->password,
                sizeof(receivedConfig.wifiPassword) - 1);
        receivedConfig.wifiPassword[sizeof(receivedConfig.wifiPassword) - 1] =
            '\0';

        Serial.printf("BLE: SSID: %s\n", receivedConfig.wifiSsid);
        configReceived = true;

        if (configCallback != nullptr) {
          configCallback(&receivedConfig);
        }
      }
      break;
    }

    case ARDUINO_EVENT_PROV_CRED_FAIL: {
      wifi_prov_sta_fail_reason_t* reason = &event->event_info.prov_fail_reason;
      Serial.printf("BLE: Provisioning failed! Reason: %d\n", *reason);
      configReceived = false;
      break;
    }

    case ARDUINO_EVENT_PROV_CRED_SUCCESS:
      Serial.println("BLE: WiFi connected successfully!");
      break;

    case ARDUINO_EVENT_PROV_END:
      Serial.println("BLE: Provisioning ended");
      isActive = false;
      break;

    case ARDUINO_EVENT_PROV_DEINIT:
      Serial.println("BLE: Deinitialized");
      isActive = false;
      break;

    default:
      break;
  }
}

bool ble_start(ble_config_callback_t callback,
               const char* serviceName,
               const char* pop) {
  if (isActive) {
    ble_stop();
  }

  if (callback == nullptr) {
    Serial.println("BLE: Error - callback is null");
    return false;
  }

  configCallback = callback;
  configReceived = false;
  memset(&receivedConfig, 0, sizeof(receivedConfig));

  WiFi.mode(WIFI_STA);

  Serial.printf("BLE: Starting provisioning with service: %s\n", serviceName);
  Serial.printf("BLE: POP (PIN): %s\n", pop);

  // Регистрируем обработчик Arduino-событий
  eventId = WiFi.onEvent(onWiFiEvent);

  WiFiProv.beginProvision(
      WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BTDM,
      WIFI_PROV_SECURITY_1, pop, serviceName, NULL, NULL, false);

  isActive = true;
  Serial.println("BLE: Provisioning started successfully");

  return true;
}

void ble_stop() {
  if (!isActive) {
    return;
  }

  Serial.println("BLE: Stopping provisioning...");

  // Удаляем обработчик по ID
  if (eventId != 0) {
    WiFi.removeEvent(eventId);
    eventId = 0;
  }

  isActive = false;
  configReceived = false;
  configCallback = nullptr;

  Serial.println("BLE: Stopped");
}

bool ble_is_active() {
  return isActive;
}

bool ble_is_config_received() {
  return configReceived;
}

bool ble_get_config(BleWifiConfig* outConfig) {
  if (!configReceived || outConfig == nullptr) {
    return false;
  }

  memcpy(outConfig, &receivedConfig, sizeof(BleWifiConfig));
  return true;
}