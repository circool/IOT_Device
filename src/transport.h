/**
 * @file transport.h
 * @version 0.12
 * @brief Транспортная абстракция — единый интерфейс для всех каналов связи
 */

#ifndef TRANSPORT_H
#define TRANSPORT_H

#include <Arduino.h>
#include "common_types.h"
#include "transport_types.h"

// ============================================================================
// СТРУКТУРА TRANSPORT
// ============================================================================

/**
 * @brief Единый интерфейс для всех транспортов
 * @details Оркестратор работает только с этим интерфейсом.
 *          Конкретные реализации скрыты за фабрикой.
 */
typedef struct Transport {
  // ========================================================================
  // УПРАВЛЕНИЕ
  // ========================================================================

  /**
   * @brief Инициализация транспорта
   * @param client Указатель на WiFiClient (для MQTT)
   * @param config Указатель на конфигурацию транспорта (TransportConfig)
   * @param outState Ссылка на указатель, куда будет записан адрес внутреннего
   * TransportState
   * @return true — успех, false — ошибка
   */
  bool (*begin)(Client* client,
                const TransportConfig* config,
                const TransportState*& outState);

  /**
   * @brief Установить указатель на конфигурацию устройства
   * @param config Указатель на DeviceConfig
   * @note Вызывается до begin() для передачи данных в WebManager
   */
  void (*setDeviceConfig)(const DeviceConfig* config);

  /**
   * @brief Установить указатель на состояние устройства
   * @param state Указатель на DeviceState
   * @note Вызывается до begin() для передачи данных в WebManager
   */
  void (*setDeviceState)(const DeviceState* state);

  /**
   * @brief Периодическая обработка (вызывается в loop())
   */
  void (*update)();

  /**
   * @brief Проверить, готов ли транспорт к работе
   * @return true — среда + протокол готовы
   */
  bool (*isConnected)();

  /**
   * @brief Принудительное отключение
   */
  void (*disconnect)();

  /**
   * @brief Получить имя транспорта (для логов)
   */
  const char* (*getName)();

  // ========================================================================
  // ПУБЛИКАЦИЯ (устройство → сеть)
  // ========================================================================

  /**
   * @brief Опубликовать статус Online
   */
  void (*publishOnline)();

  /**
   * @brief Опубликовать состояние актуатора
   * @param on true — включён, false — выключен
   */
  void (*publishState)(bool on);

  /**
   * @brief Опубликовать скорость
   * @param percent Скорость 0-100%
   */
  void (*publishSpeed)(int percent);

  /**
   * @brief Опубликовать задержку включения
   * @param seconds Задержка в секундах
   */
  void (*publishDelaySec)(int seconds);

  /**
   * @brief Опубликовать время аварийного отключения
   * @param seconds Время в секундах
   */
  void (*publishMaxOnTime)(uint32_t seconds);

  /**
   * @brief Опубликовать режим управления по датчику
   * @param enabled true — AUTO, false — MANUAL
   */
  void (*publishSensorControlMode)(bool enabled);

  /**
   * @brief Опубликовать показания датчика
   * @param temp Температура в °C
   * @param hum Влажность в %
   */
  void (*publishSensor)(float temp, float hum);

  /**
   * @brief Опубликовать состояние адаптивного режима
   * @param enabled true — включён, false — выключен
   */
  void (*publishAdaptiveMode)(bool enabled);

  /**
   * @brief Опубликовать пороговые значения
   * @param lowTemp Нижний порог температуры
   * @param highTemp Верхний порог температуры
   * @param lowHum Нижний порог влажности
   * @param highHum Верхний порог влажности
   */
  void (*publishThresholds)(float lowTemp,
                            float highTemp,
                            float lowHum,
                            float highHum);

  /**
   * @brief Опубликовать уровень сигнала
   * @param rssi RSSI в dBm
   */
  void (*publishRSSI)(int rssi);

  /**
   * @brief Опубликовать версию прошивки
   * @param version Строка с версией
   */
  void (*publishVersion)(const char* version);

  /**
   * @brief Опубликовать причину последней перезагрузки
   * @param reason Строка с причиной
   */
  void (*publishResetReason)(const char* reason);

  /**
   * @brief Опубликовать оперативное состояние устройства
   * @param state Указатель на DeviceState
   */
  void (*publishFullState)(const DeviceState* state);

  /**
   * @brief Опубликовать настройки устройства
   * @param config Указатель на DeviceConfig
   */
  void (*publishConfig)(const DeviceConfig* config);

  // ========================================================================
  // КОЛБЭКИ (сеть → устройство)
  // ========================================================================

  /**
   * @brief Регистрация колбэка для событий транспорта
   * @param callback Функция обратного вызова (TransportEventCallback)
   * @param context Контекст (передаётся в колбэк)
   * @note Заменяет ранее существовавшие onCommand и onConfigUpdate
   */
  void (*onEvent)(TransportEventCallback callback, void* context);

} Transport;

// ============================================================================
// ГЛОБАЛЬНЫЙ УКАЗАТЕЛЬ
// ============================================================================

/**
 * @brief Глобальный указатель на текущий транспорт
 * @details Устанавливается Оркестратором через createTransport()
 */
extern Transport* g_transport;

#endif  // TRANSPORT_H