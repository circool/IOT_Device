/**
 * @file web_common.cpp
 * @brief Реализация общих Web-функций
 */

#include "web_common.h"
#include "html_templates.h"
#include "logger.h"

#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI

// ============================================================================
// ВСПОМОГАТЕЛЬНЫЕ МАКРОСЫ
// ============================================================================

// ============================================================================
// РЕНДЕРИНГ ПОЛЕЙ
// ============================================================================

void render(char* buf, size_t size, const TextField& field) {
  if (!buf || size == 0)
    return;
  buf[0] = '\0';

  char temp[512];
  temp[0] = '\0';

  strncat(temp, "<div class='field-group'>", sizeof(temp) - strlen(temp) - 1);

  if (field.label) {
    char lbl[128];
    snprintf(lbl, sizeof(lbl), "<label>%s</label>", field.label);
    strncat(temp, lbl, sizeof(temp) - strlen(temp) - 1);
  }

  const char* inputType = field.hideInput ? "password" : "text";
  char input[256];
  snprintf(input, sizeof(input), "<input type='%s' name='%s'", inputType,
           field.name ? field.name : "");
  if (field.value) {
    char val[64];
    snprintf(val, sizeof(val), " value='%s'", field.value);
    strncat(input, val, sizeof(input) - strlen(input) - 1);
  }
  if (field.placeholder) {
    char ph[64];
    snprintf(ph, sizeof(ph), " placeholder='%s'", field.placeholder);
    strncat(input, ph, sizeof(input) - strlen(input) - 1);
  }
  if (field.required) {
    strncat(input, " required", sizeof(input) - strlen(input) - 1);
  }
  strncat(input, ">", sizeof(input) - strlen(input) - 1);
  strncat(temp, input, sizeof(temp) - strlen(temp) - 1);

  if (field.note && strlen(field.note) > 0) {
    char note[128];
    snprintf(note, sizeof(note), "<div class='note'>%s</div>", field.note);
    strncat(temp, note, sizeof(temp) - strlen(temp) - 1);
  }

  strncat(temp, "</div>", sizeof(temp) - strlen(temp) - 1);
  strncat(buf, temp, size - 1);
}

void render(char* buf, size_t size, const NumberField& field) {
  if (!buf || size == 0)
    return;
  buf[0] = '\0';

  char temp[512];
  temp[0] = '\0';

  strncat(temp, "<div class='field-group'>", sizeof(temp) - strlen(temp) - 1);

  if (field.label) {
    char lbl[128];
    snprintf(lbl, sizeof(lbl), "<label>%s</label>", field.label);
    strncat(temp, lbl, sizeof(temp) - strlen(temp) - 1);
  }

  char input[256];
  snprintf(input, sizeof(input), "<input type='number' name='%s'",
           field.name ? field.name : "");
  if (field.value) {
    char val[64];
    snprintf(val, sizeof(val), " value='%s'", field.value);
    strncat(input, val, sizeof(input) - strlen(input) - 1);
  }
  if (field.placeholder) {
    char ph[64];
    snprintf(ph, sizeof(ph), " placeholder='%s'", field.placeholder);
    strncat(input, ph, sizeof(input) - strlen(input) - 1);
  }
  if (field.min) {
    char mn[32];
    snprintf(mn, sizeof(mn), " min='%s'", field.min);
    strncat(input, mn, sizeof(input) - strlen(input) - 1);
  }
  if (field.max) {
    char mx[32];
    snprintf(mx, sizeof(mx), " max='%s'", field.max);
    strncat(input, mx, sizeof(input) - strlen(input) - 1);
  }
  if (field.step) {
    char st[32];
    snprintf(st, sizeof(st), " step='%s'", field.step);
    strncat(input, st, sizeof(input) - strlen(input) - 1);
  }
  if (field.required) {
    strncat(input, " required", sizeof(input) - strlen(input) - 1);
  }
  strncat(input, ">", sizeof(input) - strlen(input) - 1);
  strncat(temp, input, sizeof(temp) - strlen(temp) - 1);

  if (field.note && strlen(field.note) > 0) {
    char note[128];
    snprintf(note, sizeof(note), "<div class='note'>%s</div>", field.note);
    strncat(temp, note, sizeof(temp) - strlen(temp) - 1);
  }

  strncat(temp, "</div>", sizeof(temp) - strlen(temp) - 1);
  strncat(buf, temp, size - 1);
}

void render(char* buf, size_t size, const CheckboxField& field) {
  if (!buf || size == 0)
    return;
  buf[0] = '\0';

  char temp[256];
  temp[0] = '\0';

  strncat(temp, "<div class='field-group'>", sizeof(temp) - strlen(temp) - 1);

  char chk[128];
  snprintf(chk, sizeof(chk),
           "<label><input type='checkbox' name='%s' value='1'",
           field.name ? field.name : "");
  if (field.checked) {
    strncat(chk, " checked", sizeof(chk) - strlen(chk) - 1);
  }
  if (field.required) {
    strncat(chk, " required", sizeof(chk) - strlen(chk) - 1);
  }
  strncat(chk, ">", sizeof(chk) - strlen(chk) - 1);
  if (field.label) {
    strncat(chk, " ", sizeof(chk) - strlen(chk) - 1);
    strncat(chk, field.label, sizeof(chk) - strlen(chk) - 1);
  }
  strncat(chk, "</label>", sizeof(chk) - strlen(chk) - 1);
  strncat(temp, chk, sizeof(temp) - strlen(temp) - 1);

  if (field.note && strlen(field.note) > 0) {
    char note[128];
    snprintf(note, sizeof(note), "<div class='note'>%s</div>", field.note);
    strncat(temp, note, sizeof(temp) - strlen(temp) - 1);
  }

  strncat(temp, "</div>", sizeof(temp) - strlen(temp) - 1);
  strncat(buf, temp, size - 1);
}

// ============================================================================
// РЕНДЕРИНГ БЛОКОВ
// ============================================================================

void render(char* buf, size_t size, const TextBlockParams& params) {
  if (!buf || size == 0)
    return;
  buf[0] = '\0';

  char temp[128];
  snprintf(temp, sizeof(temp),
           "<div class='card %s'>"
           "<div class='value'>%s %s</div>"
           "<div class='label'>%s</div>"
           "</div>",
           params.colorClass ? params.colorClass : "",
           params.value ? params.value : "", params.unit ? params.unit : "",
           params.title ? params.title : "");
  strncat(buf, temp, size - 1);
}

void render(char* buf, size_t size, const StatusBlockParams& params) {
  if (!buf || size == 0)
    return;
  buf[0] = '\0';

  const char* colorClass = params.isOn ? "error" : "info";
  const char* label =
      params.label ? params.label : (params.isOn ? "ON" : "OFF");

  char temp[128];
  snprintf(temp, sizeof(temp),
           "<div class='block center %s'>"
           "<div class='large'>%s</div>"
           "</div>",
           colorClass, label);
  strncat(buf, temp, size - 1);
}

void render(char* buf, size_t size, const ButtonParams& params) {
  if (!buf || size == 0)
    return;
  buf[0] = '\0';

  const char* colorClass = params.colorClass ? params.colorClass : "link-btn";
  const char* url = params.url ? params.url : "#";
  const char* label = params.label ? params.label : "Button";

  char temp[128];
  snprintf(temp, sizeof(temp),
           "<a href='%s'><button class='%s'>%s</button></a>", url, colorClass,
           label);
  strncat(buf, temp, size - 1);
}

void render(char* buf, size_t size, const ProgressParams& params) {
  if (!buf || size == 0)
    return;
  buf[0] = '\0';

  int percent = params.percent;
  if (percent < 0)
    percent = 0;
  if (percent > 100)
    percent = 100;

  char temp[128];
  snprintf(
      temp, sizeof(temp),
      "<div class='bar'><div class='fill' style='width:%d%%;'></div></div>",
      percent);
  strncat(buf, temp, size - 1);
}

void render(char* buf, size_t size, const InfoBlockParams& params) {
  if (!buf || size == 0)
    return;
  buf[0] = '\0';

  char temp[256];
  const char* colorClass = params.colorClass ? params.colorClass : "info";
  snprintf(temp, sizeof(temp),
           "<div class='block %s'>"
           "<h3>%s</h3>"
           "%s"
           "</div>",
           colorClass, params.title ? params.title : "",
           params.text ? params.text : "");
  strncat(buf, temp, size - 1);
}

// ============================================================================
// РЕНДЕРИНГ ОБЩИХ БЛОКОВ (WiFi + MQTT) — ИСПРАВЛЕНО
// ============================================================================

void renderWifiBlock(char* buf,
                     size_t size,
                     const TransportConfig* config,
                     PageMode mode) {
  if (!buf || size == 0)
    return;
  buf[0] = '\0';
  (void)mode;

  char temp[256];

  snprintf(temp, sizeof(temp), "<h2>WiFi Settings</h2>");
  SAFE_STRCAT(buf, temp, size);

  // SSID
  TextField ssidField;
  ssidField.label = "WiFi SSID";
  ssidField.name = "wifiSsid";
  ssidField.note = "Required";
  ssidField.value = config ? config->wifiSsid : "";
  ssidField.placeholder = "Enter WiFi name";
  ssidField.hideInput = false;
  ssidField.required = true;
  render(temp, sizeof(temp), ssidField);
  SAFE_STRCAT(buf, temp, size);

  // Password
  TextField passField;
  passField.label = "WiFi Password";
  passField.name = "wifiPassword";
  passField.note = "Leave empty if not changed";
  passField.value = "";
  passField.placeholder = "Enter WiFi password";
  passField.hideInput = true;
  passField.required = false;
  render(temp, sizeof(temp), passField);
  SAFE_STRCAT(buf, temp, size);
}

void renderMQTTBlock(char* buf, size_t size, const TransportConfig* config) {
  if (!buf || size == 0)
    return;
  buf[0] = '\0';

#if FEATURE_MQTT_ENABLED == 1
  char temp[256];

  snprintf(temp, sizeof(temp), "<h2>MQTT Settings</h2>");
  SAFE_STRCAT(buf, temp, size);

  // Broker
  TextField brokerField;
  brokerField.label = "MQTT Broker";
  brokerField.name = "mqttBroker";
  brokerField.note = "IP or hostname";
  brokerField.value = config ? config->mqttBroker : "";
  brokerField.placeholder = "192.168.1.100";
  brokerField.hideInput = false;
  brokerField.required = false;
  render(temp, sizeof(temp), brokerField);
  SAFE_STRCAT(buf, temp, size);

  // Port
  NumberField portField;
  portField.label = "MQTT Port";
  portField.name = "mqttPort";
  portField.note = "1-65535";
  char portStr[8];
  if (config && config->mqttPort > 0) {
    snprintf(portStr, sizeof(portStr), "%d", config->mqttPort);
  } else {
    portStr[0] = '\0';
  }
  portField.value = portStr;
  portField.placeholder = "1883";
  portField.min = "1";
  portField.max = "65535";
  portField.step = "1";
  portField.required = false;
  render(temp, sizeof(temp), portField);
  SAFE_STRCAT(buf, temp, size);

  // User
  TextField userField;
  userField.label = "MQTT User";
  userField.name = "mqttUser";
  userField.note = "Optional";
  userField.value = config ? config->mqttUser : "";
  userField.placeholder = "Username";
  userField.hideInput = false;
  userField.required = false;
  render(temp, sizeof(temp), userField);
  SAFE_STRCAT(buf, temp, size);

  // Password
  TextField mqttPassField;
  mqttPassField.label = "MQTT Password";
  mqttPassField.name = "mqttPassword";
  mqttPassField.note = "Optional";
  mqttPassField.value = "";
  mqttPassField.placeholder = "Password";
  mqttPassField.hideInput = true;
  mqttPassField.required = false;
  render(temp, sizeof(temp), mqttPassField);
  SAFE_STRCAT(buf, temp, size);
#endif
}

// ============================================================================
// ОТПРАВКА СТРАНИЦ
// ============================================================================

void webSendContent(const char* chunk, void* context) {
  WebServerClass* srv = (WebServerClass*)context;
  if (srv && chunk) {
    srv->sendContent(chunk);
  }
}

void web_sendPageStart(WebSendCallback send,
                       void* context,
                       const char* title,
                       PageMode mode) {
  (void)mode;
  if (!send)
    return;

  send((const char*)FPSTR(HTML_PAGE_START), context);

  if (title && strlen(title) > 0) {
    char temp[64];
    snprintf(temp, sizeof(temp), "<title>%s</title>", title);
    send(temp, context);
  } else {
    send((const char*)"<title>Device</title>", context);
  }

  send((const char*)FPSTR(HTML_STYLE), context);
  send((const char*)"</head><body><div class='container'>", context);
}

void web_sendPageEnd(WebSendCallback send, void* context) {
  if (send) {
    send((const char*)FPSTR(HTML_PAGE_END), context);
  }
}

void web_sendRefreshMeta(WebSendCallback send,
                         void* context,
                         int refreshSeconds,
                         const char* url) {
  if (!send || refreshSeconds <= 0)
    return;

  char refresh[128];
  if (url && strlen(url) > 0) {
    snprintf_P(refresh, sizeof(refresh),
               PSTR("<meta http-equiv='refresh' content='%d;url=%s'>"),
               refreshSeconds, url);
  } else {
    snprintf_P(refresh, sizeof(refresh),
               PSTR("<meta http-equiv='refresh' content='%d'>"),
               refreshSeconds);
  }
  send(refresh, context);
}

void web_send_result_page(WebSendCallback send,
                          void* context,
                          const char* action,
                          bool success) {
  if (!send)
    return;

  web_sendPageStart(send, context, success ? "Success" : "Error",
                    PAGE_MODE_RESULT);

  if (success) {
    web_sendRefreshMeta(send, context, 2, "/");
  }

  char temp[256];
  const char* className = success ? "success" : "error";
  snprintf(temp, sizeof(temp),
           "<div class='block center %s'>"
           "<h2>%s %s</h2>"
           "<p>%s</p>"
           "</div>",
           className, action, success ? "successful" : "failed",
           success ? "Redirecting..." : "Please try again.");
  send(temp, context);
  web_sendPageEnd(send, context);
}

#endif  // TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI