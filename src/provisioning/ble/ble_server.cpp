// ===== ФАЙЛ: src/provisioning/ble/ble_server.cpp =====

#include "ble_server.h"
#include "config_manager.h"
#include "logger.h"
#include "settings.h"

#if defined(ESP32) && !defined(ESP8266)

#include <WiFi.h>
#include <WiFiProv.h>

// Глобальный указатель для колбэков WiFi
static BleProvisioningServer* g_provServer = nullptr;

// Функция обратного вызова для WiFi событий
// КЛЮЧЕВОЕ ИЗМЕНЕНИЕ: проверка isActive() для игнорирования событий после
// stop()
static void sysProvEvent(arduino_event_t* sys_event) {
  if (g_provServer && g_provServer->isActive()) {
    g_provServer->onWiFiEvent(sys_event);
  }
  // Если сервер не активен — событие игнорируется
}

// Конструктор
BleProvisioningServer::BleProvisioningServer(const char* deviceName) {
  if (deviceName && strlen(deviceName) < sizeof(_deviceName)) {
    strncpy(_deviceName, deviceName, sizeof(_deviceName) - 1);
    _deviceName[sizeof(_deviceName) - 1] = '\0';
  } else {
    strncpy(_deviceName, "PROV_123", sizeof(_deviceName) - 1);
  }
  g_provServer = this;
}

// Деструктор
BleProvisioningServer::~BleProvisioningServer() {
  deinit();
  g_provServer = nullptr;
}

bool BleProvisioningServer::begin(ProvConfigCallback configCallback,
                                  ProvStatusCallback statusCallback,
                                  uint32_t timeoutMs) {
  if (_active)
    return true;

  _configCallback = configCallback;
  _statusCallback = statusCallback;
  _timeoutMs = timeoutMs;
  _startTimeMs = millis();
  _wifiAttempts = 0;
  _state = BleProvisioningState::WAITING_CREDENTIALS;
  _credentialsReceived = false;
  _configSaved = false;

  LOG_INFO(CAT_PROVISIONING, "========================================");
  LOG_INFO(CAT_PROVISIONING, "Starting BLE Provisioning");
  LOG_INFO(CAT_PROVISIONING, "Device name: %s", _deviceName);
  LOG_INFO(CAT_PROVISIONING, "Timeout: %u ms", _timeoutMs);
  LOG_INFO(CAT_PROVISIONING, "Max attempts: %d", MAX_WIFI_ATTEMPTS);
  LOG_INFO(CAT_PROVISIONING, "========================================");

  // Регистрируем обработчик событий
  WiFi.onEvent(sysProvEvent);

  if (!startWiFiProvisioning()) {
    LOG_ERROR(CAT_PROVISIONING, "Failed to start WiFiProv!");
    _state = BleProvisioningState::STOPPED;
    deinit();
    return false;
  }

  _active = true;
  return true;
}

bool BleProvisioningServer::startWiFiProvisioning() {
  const char* pop = BLE_PROVISIONING_PIN;
  const uint8_t PROV_UUID[16] = {0xb4, 0xdf, 0x5a, 0x1c, 0x3f, 0x6b,
                                 0xf4, 0xbf, 0xea, 0x4a, 0x82, 0x03,
                                 0x04, 0x90, 0x1a, 0x02};

  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true);
  delay(100);

  WiFiProv.beginProvision(
      WIFI_PROV_SCHEME_BLE, WIFI_PROV_SCHEME_HANDLER_FREE_BLE,
      WIFI_PROV_SECURITY_1, pop, _deviceName, NULL, (uint8_t*)PROV_UUID, true);

  _provisioningStarted = true;
  LOG_INFO(CAT_PROVISIONING, "WiFiProv.beginProvision called successfully");
  return true;
}

void BleProvisioningServer::onWiFiEvent(arduino_event_t* sys_event) {
  // Дополнительная проверка на случай, если sysProvEvent пропустила
  if (!_active)
    return;

  switch (sys_event->event_id) {
    case ARDUINO_EVENT_PROV_START:
      LOG_INFO(CAT_PROVISIONING, "Provisioning started");
      _state = BleProvisioningState::WAITING_CREDENTIALS;
      _credentialsReceived = false;
      _configSaved = false;
      _wifiAttempts = 0;
      if (_statusCallback)
        _statusCallback(0x01);
      break;

    case ARDUINO_EVENT_PROV_CRED_RECV:
      LOG_INFO(CAT_PROVISIONING, "Credentials received from BLE");
      _credentialsReceived = true;
      _state = BleProvisioningState::CONNECTING_WIFI;
      break;

    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      LOG_INFO(CAT_PROVISIONING, "WiFi connected successfully! IP: %s",
               IPAddress(sys_event->event_info.got_ip.ip_info.ip.addr)
                   .toString()
                   .c_str());
      _state = BleProvisioningState::WIFI_CONNECTED;

      if (_credentialsReceived && !_configSaved) {
        String ssid = WiFi.SSID();
        String password = WiFi.psk();

        memset(&_config, 0, sizeof(_config));
        strncpy(_config.wifiSsid, ssid.c_str(), sizeof(_config.wifiSsid) - 1);
        strncpy(_config.wifiPassword, password.c_str(),
                sizeof(_config.wifiPassword) - 1);

        if (_configCallback) {
          _configCallback(&_config);
          _configSaved = true;
          if (_statusCallback)
            _statusCallback(0x02);
        }
      }
      break;

    case ARDUINO_EVENT_PROV_CRED_FAIL:
      _wifiAttempts++;
      LOG_ERROR(CAT_PROVISIONING, "Provisioning failed (attempt %d/%d)",
                _wifiAttempts, MAX_WIFI_ATTEMPTS);

      if (_wifiAttempts >= MAX_WIFI_ATTEMPTS) {
        _state = BleProvisioningState::MAX_ATTEMPTS_REACHED;
        LOG_ERROR(CAT_PROVISIONING, "Max attempts reached!");
        if (_statusCallback)
          _statusCallback(0x05);
        // НЕМЕДЛЕННО ОСТАНАВЛИВАЕМ ПРОВИЗИОНИНГ
        stop();
      } else {
        _state = BleProvisioningState::WAITING_CREDENTIALS;
        _credentialsReceived = false;
        if (_statusCallback)
          _statusCallback(0x04);
      }
      break;

    case ARDUINO_EVENT_PROV_END:
      LOG_INFO(CAT_PROVISIONING, "Provisioning ended");
      _provisioningStarted = false;
      if (_state != BleProvisioningState::WIFI_CONNECTED) {
        if (_state == BleProvisioningState::CONNECTING_WIFI) {
          _state = BleProvisioningState::TIMEOUT;
        }
        if (_statusCallback)
          _statusCallback(0x03);
      }
      break;

    default:
      break;
  }
}

void BleProvisioningServer::process() {
  if (!_active)
    return;

  // Проверка общего таймаута
  if (_timeoutMs > 0 && (millis() - _startTimeMs) > _timeoutMs) {
    LOG_WARN(CAT_PROVISIONING, "Provisioning timeout!");
    _state = BleProvisioningState::TIMEOUT;
    stop();
    return;
  }

  // Если ошибка аутентификации - ждем новые креденшелы
  if (_state == BleProvisioningState::WIFI_AUTH_ERROR) {
    LOG_INFO(CAT_PROVISIONING,
             "WiFi auth error, waiting for new credentials...");
    _state = BleProvisioningState::WAITING_CREDENTIALS;
    _credentialsReceived = false;
  }
}

void BleProvisioningServer::stop() {
  if (!_active)
    return;

  // ВАЖНО: СНАЧАЛА УСТАНАВЛИВАЕМ _active = false
  // Это гарантирует, что sysProvEvent не будет обрабатывать новые события
  _active = false;
  _provisioningStarted = false;
  _state = BleProvisioningState::STOPPED;

  // Отключаем WiFi
  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_OFF);

  LOG_INFO(CAT_PROVISIONING, "BLE provisioning stopped");
}

void BleProvisioningServer::deinit() {
  stop();
  // Дополнительная очистка при необходимости
}

bool BleProvisioningServer::isActive() const {
  return _active;
}

BleProvisioningState BleProvisioningServer::getState() const {
  return _state;
}

#else  // !ESP32 || ESP8266

// ===== ЗАГЛУШКИ ДЛЯ ESP8266 =====
BleProvisioningServer::BleProvisioningServer(const char* deviceName) {
  (void)deviceName;
  _state = BleProvisioningState::STOPPED;
}

BleProvisioningServer::~BleProvisioningServer() {}

bool BleProvisioningServer::begin(ProvConfigCallback,
                                  ProvStatusCallback,
                                  uint32_t) {
  return false;
}

void BleProvisioningServer::stop() {}
void BleProvisioningServer::deinit() {}
bool BleProvisioningServer::isActive() const {
  return false;
}
BleProvisioningState BleProvisioningServer::getState() const {
  return BleProvisioningState::STOPPED;
}
void BleProvisioningServer::process() {}
void BleProvisioningServer::onWiFiEvent(arduino_event_t*) {}

#endif  // ESP32