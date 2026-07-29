#ifndef ZIGBEE_H
#define ZIGBEE_H

#include <Arduino.h>
#include "settings.h"
#include "config_manager.h"


/**
 * @brief Включить поддержку ZigBee
 * @note Определяется в settings.h на основе TRANSPORT_TYPE == TRANSPORT_TYPE_ZIGBEE
 */
#if TRANSPORT_TYPE == TRANSPORT_TYPE_ZIGBEE

#include <functional>

/**
 * @brief Класс-заглушка для ZigBee транспорта
 *
 * На данный момент реализует только интерфейс, совместимый с MQTTManager.
 * В будущем будет заменён на реальную реализацию ZigBee (ESP-Zigbee-SDK).
 *
 * TODO: Реализовать реальное ZigBee-взаимодействие:
 *   - Инициализация ZigBee-стека
 *   - Подключение к координатору
 *   - Публикация состояний
 *   - Обработка входящих команд
 */
class ZigBeeManager {
 public:
  ZigBeeManager() = default;
  ~ZigBeeManager() = default;

  /**
   * @brief Инициализация ZigBee-менеджера
   * @param clientId Уникальный идентификатор устройства (используется как
   * endpoint)
   * @return true — успешно, false — ошибка
   */
  bool begin(const char* clientId) {
    (void)clientId;
    _initialized = true;
    return true;
  }

  /**
   * @brief Периодический вызов в loop()
   */
  void update() {
    if (!_initialized)
      return;
    // TODO: Реализовать обработку входящих ZigBee-сообщений
  }

  /**
   * @brief Проверить соединение с ZigBee-сетью
   * @return true — подключён, false — нет
   */
  bool isConnected() const { return _initialized; }

  /**
   * @brief Принудительно отключиться от ZigBee-сети
   */
  void disconnect() { _initialized = false; }

  // --- Публикации ---
  void publishOnline() {
    // TODO: Публикация статуса Online в ZigBee
  }

  void publishState(bool on) {
    (void)on;
    // TODO: Публикация состояния вентилятора/выключателя
  }

  void publishSpeed(int percent) {
    (void)percent;
    // TODO: Публикация скорости
  }

  void publishDelaySec(int seconds) {
    (void)seconds;
    // TODO: Публикация таймера отложенного включения
  }

  void publishMaxOnTime(uint32_t seconds) {
    (void)seconds;
    // TODO: Публикация таймера аварийного отключения
  }

  void publishSensorControlMode(bool enabled) {
    (void)enabled;
    // TODO: Публикация режима AUTO/MANUAL
  }

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  void publishSensor(float temp, float hum) {
    (void)temp;
    (void)hum;
    // TODO: Публикация показаний датчика
  }
#endif

#if DEVICE_TYPE == 1
  void publishAdaptiveMode(bool enabled) {
    (void)enabled;
    // TODO: Публикация адаптивного режима
  }

  void publishThresholds(float lowTemp,
                         float highTemp,
                         float lowHum,
                         float highHum) {
    (void)lowTemp;
    (void)highTemp;
    (void)lowHum;
    (void)highHum;
    // TODO: Публикация порогов
  }
#endif

#if MQTT_PUBLISH_RSSI == 1
  void publishRSSI(int rssi) {
    (void)rssi;
    // TODO: Публикация RSSI
  }
#endif

#if MQTT_PUBLISH_VERSION == 1
  void publishVersion(const char* version) {
    (void)version;
    // TODO: Публикация версии
  }
#endif

#if MQTT_PUBLISH_RESET_REASON == 1
  void publishResetReason(const char* reason) {
    (void)reason;
    // TODO: Публикация причины перезагрузки
  }
#endif

  // --- Колбэки на входящие команды ---
  void onStateCommand(std::function<void(bool)> callback) {
    _stateCallback = callback;
    // TODO: Подписка на команды состояния
  }

  void onSpeedCommand(std::function<void(int)> callback) {
    _speedCallback = callback;
    // TODO: Подписка на команды скорости
  }

  void onDelaySecCommand(std::function<void(int)> callback) {
    _delaySecCallback = callback;
    // TODO: Подписка на команды таймера
  }

  void onMaxOnTimeCommand(std::function<void(uint32_t)> callback) {
    _maxOnTimeCallback = callback;
    // TODO: Подписка на команды аварийного отключения
  }

  void onSensorControlModeCommand(std::function<void(bool)> callback) {
    _sensorControlModeCallback = callback;
    // TODO: Подписка на команды режима AUTO/MANUAL
  }

#if DEVICE_TYPE == 1
  void onAdaptiveModeCommand(std::function<void(bool)> callback) {
    _adaptiveModeCallback = callback;
    // TODO: Подписка на команды адаптивного режима
  }

  void onLowTempCommand(std::function<void(float)> callback) {
    _lowTempCallback = callback;
  }

  void onHighTempCommand(std::function<void(float)> callback) {
    _highTempCallback = callback;
  }

  void onLowHumCommand(std::function<void(float)> callback) {
    _lowHumCallback = callback;
  }

  void onHighHumCommand(std::function<void(float)> callback) {
    _highHumCallback = callback;
  }
#endif

#if MQTT_RESET_ENABLED == 1
  void onResetCommand(std::function<void()> callback) {
    _resetCallback = callback;
  }
#endif

 private:
  bool _initialized = false;

  // Колбэки (сохраняем для совместимости с MQTTManager)
  std::function<void(bool)> _stateCallback;
  std::function<void(int)> _speedCallback;
  std::function<void(int)> _delaySecCallback;
  std::function<void(uint32_t)> _maxOnTimeCallback;
  std::function<void(bool)> _sensorControlModeCallback;

#if DEVICE_TYPE == 1
  std::function<void(bool)> _adaptiveModeCallback;
  std::function<void(float)> _lowTempCallback;
  std::function<void(float)> _highTempCallback;
  std::function<void(float)> _lowHumCallback;
  std::function<void(float)> _highHumCallback;
#endif

#if MQTT_RESET_ENABLED == 1
  std::function<void()> _resetCallback;
#endif
};

extern ZigBeeManager zigbeeManager;

#else  // TRANSPORT_TYPE == TRANSPORT_TYPE_ZIGBEE == 0

// ============================================================================
// ЗАГЛУШКА ДЛЯ РЕЖИМА БЕЗ ZIGBEE
// ============================================================================

class ZigBeeManager {
 public:
  ZigBeeManager() = default;
  ~ZigBeeManager() = default;

  bool begin(const char*) { return false; }
  void update() {}
  bool isConnected() const { return false; }
  void disconnect() {}

  void publishOnline() {}
  void publishState(bool) {}
  void publishSpeed(int) {}
  void publishDelaySec(int) {}
  void publishMaxOnTime(uint32_t) {}
  void publishSensorControlMode(bool) {}

#if DEVICE_TYPE == 1 || DEVICE_TYPE == 2
  void publishSensor(float, float) {}
#endif

#if DEVICE_TYPE == 1
  void publishAdaptiveMode(bool) {}
  void publishThresholds(float, float, float, float) {}
#endif

#if MQTT_PUBLISH_RSSI == 1
  void publishRSSI(int) {}
#endif

#if MQTT_PUBLISH_VERSION == 1
  void publishVersion(const char*) {}
#endif

#if MQTT_PUBLISH_RESET_REASON == 1
  void publishResetReason(const char*) {}
#endif

  // Колбэки — просто сохраняем, но никогда не вызываем
  void onStateCommand(std::function<void(bool)>) {}
  void onSpeedCommand(std::function<void(int)>) {}
  void onDelaySecCommand(std::function<void(int)>) {}
  void onMaxOnTimeCommand(std::function<void(uint32_t)>) {}
  void onSensorControlModeCommand(std::function<void(bool)>) {}

#if DEVICE_TYPE == 1
  void onAdaptiveModeCommand(std::function<void(bool)>) {}
  void onLowTempCommand(std::function<void(float)>) {}
  void onHighTempCommand(std::function<void(float)>) {}
  void onLowHumCommand(std::function<void(float)>) {}
  void onHighHumCommand(std::function<void(float)>) {}
#endif

#if MQTT_RESET_ENABLED == 1
  void onResetCommand(std::function<void()>) {}
#endif
};

extern ZigBeeManager zigbeeManager;

#endif  // TRANSPORT_TYPE == TRANSPORT_TYPE_ZIGBEE

#endif  // ZIGBEE_H