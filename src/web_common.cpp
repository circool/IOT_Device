/**
 * @file web_common.cpp
 * @brief Реализация общих Web-функций
 * @date 2026-07-28
 */

#include "web_common.h"
#include "html_templates.h"

#include <cstring>
#include "logger.h"
#include "settings.h"
#include "system_state.h"

#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI

static WebServerClass* g_commonServer = nullptr;

void web_renderField(char* buf, size_t size, const FieldDef* field) {
  if (!buf || size == 0 || !field)
    return;
  buf[0] = '\0';

  switch (field->type) {
    case FIELD_TYPE_TEXT:
    case FIELD_TYPE_PASSWORD:
    case FIELD_TYPE_NUMBER: {
      const char* inputType = (field->type == FIELD_TYPE_PASSWORD) ? "password"
                              : (field->type == FIELD_TYPE_NUMBER) ? "number"
                                                                   : "text";
      char temp[256];
      temp[0] = '\0';

      if (field->label) {
        snprintf(temp, sizeof(temp), "<label>%s</label>", field->label);
        strncat(buf, temp, size - 1);
      }

      snprintf(temp, sizeof(temp), "<input type='%s' name='%s'", inputType,
               field->name ? field->name : "");
      if (field->value) {
        char val[64];
        snprintf(val, sizeof(val), " value='%s'", field->value);
        strncat(temp, val, sizeof(temp) - strlen(temp) - 1);
      }
      if (field->placeholder) {
        char ph[64];
        snprintf(ph, sizeof(ph), " placeholder='%s'", field->placeholder);
        strncat(temp, ph, sizeof(temp) - strlen(temp) - 1);
      }
      if (field->min) {
        char mn[32];
        snprintf(mn, sizeof(mn), " min='%s'", field->min);
        strncat(temp, mn, sizeof(temp) - strlen(temp) - 1);
      }
      if (field->max) {
        char mx[32];
        snprintf(mx, sizeof(mx), " max='%s'", field->max);
        strncat(temp, mx, sizeof(temp) - strlen(temp) - 1);
      }
      if (field->step) {
        char st[32];
        snprintf(st, sizeof(st), " step='%s'", field->step);
        strncat(temp, st, sizeof(temp) - strlen(temp) - 1);
      }
      if (field->required) {
        strncat(temp, " required", sizeof(temp) - strlen(temp) - 1);
      }
      strncat(temp, ">", sizeof(temp) - strlen(temp) - 1);
      strncat(buf, temp, size - 1);
      break;
    }

    case FIELD_TYPE_CHECKBOX: {
      char temp[128];
      temp[0] = '\0';
      snprintf(temp, sizeof(temp),
               "<label><input type='checkbox' name='%s' value='1'",
               field->name ? field->name : "");
      if (field->checked) {
        strncat(temp, " checked", sizeof(temp) - strlen(temp) - 1);
      }
      strncat(temp, ">", sizeof(temp) - strlen(temp) - 1);
      if (field->label) {
        strncat(temp, " ", sizeof(temp) - strlen(temp) - 1);
        strncat(temp, field->label, sizeof(temp) - strlen(temp) - 1);
      }
      strncat(temp, "</label>", sizeof(temp) - strlen(temp) - 1);
      strncat(buf, temp, size - 1);
      break;
    }

    case FIELD_TYPE_LABEL: {
      char temp[128];
      temp[0] = '\0';
      snprintf(temp, sizeof(temp), "<div class='info'>");
      if (field->label) {
        strncat(temp, field->label, sizeof(temp) - strlen(temp) - 1);
      }
      if (field->value) {
        char val[64];
        snprintf(val, sizeof(val), ": <strong>%s</strong>", field->value);
        strncat(temp, val, sizeof(temp) - strlen(temp) - 1);
      }
      strncat(temp, "</div>", sizeof(temp) - strlen(temp) - 1);
      strncat(buf, temp, size - 1);
      break;
    }

    case FIELD_TYPE_CARD: {
      char temp[256];
      temp[0] = '\0';

      snprintf(temp, sizeof(temp), "<div class='block error'>");
      if (field->label) {
        char lbl[128];
        snprintf(lbl, sizeof(lbl), "<div class='text_header'>%s</div>",
                 field->label);
        strncat(temp, lbl, sizeof(temp) - strlen(temp) - 1);
      }
      if (field->note) {
        char nt[128];
        snprintf(nt, sizeof(nt), "<div class='text_small'>%s</div>",
                 field->note);
        strncat(temp, nt, sizeof(temp) - strlen(temp) - 1);
      }
      strncat(temp, "</div>", sizeof(temp) - strlen(temp) - 1);
      strncat(buf, temp, size - 1);
      break;
    }

    case FIELD_TYPE_BUTTON: {
      char temp[128];
      temp[0] = '\0';
      const char* url = field->link ? field->link : "#";
      const char* btnText = field->buttonText ? field->buttonText : "Button";
      const char* style = field->value ? field->value : "link-btn";
      snprintf(temp, sizeof(temp),
               "<a href='%s'><button class='%s'>%s</button></a>", url, style,
               btnText);
      strncat(buf, temp, size - 1);
      break;
    }

    case FIELD_TYPE_LINK: {
      char temp[128];
      temp[0] = '\0';
      const char* url = field->link ? field->link : "#";
      const char* btnText = field->buttonText ? field->buttonText : "Link";
      snprintf(temp, sizeof(temp), "<a href='%s' class='link-btn'>%s</a>", url,
               btnText);
      strncat(buf, temp, size - 1);
      break;
    }

    default:
      break;
  }

  if (field->note && field->type != FIELD_TYPE_CARD) {
    char note[128];
    note[0] = '\0';
    snprintf(note, sizeof(note), "<div class='note'>%s</div>", field->note);
    strncat(buf, note, size - 1);
  }
}

void web_renderSensorCard(char* buf,
                          size_t size,
                          float value,
                          const char* label,
                          const char* unit,
                          const char* colorClass,
                          const char* note) {
  if (!buf || size == 0)
    return;
  buf[0] = '\0';

  char valueStr[16];
  snprintf(valueStr, sizeof(valueStr), "%.1f", value);

  char temp[256];
  snprintf(temp, sizeof(temp),
           "<div class='sensor-card block %s'>"
           "<div class='sensor-value'>%s %s</div>"
           "<div class='sensor-label'>%s",
           colorClass, valueStr, unit, label);

  if (note) {
    char nt[128];
    snprintf(nt, sizeof(nt), "<br><span class='text_small'>%s</span>", note);
    strncat(temp, nt, sizeof(temp) - strlen(temp) - 1);
  }

  strncat(temp, "</div></div>", sizeof(temp) - strlen(temp) - 1);
  strncat(buf, temp, size - 1);
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

void web_renderStatusCard(char* buf, size_t size, const char* status, bool isOn) {
  if (!buf || size == 0)
    return;
  buf[0] = '\0';

  char temp[256];
  const char* colorClass = isOn ? "error" : "info";
  snprintf(temp, sizeof(temp),
           "<div class='block center %s'>"
           "<div class='text_header'>%s</div></div>",
           colorClass, status);
  strncat(buf, temp, size - 1);
}

void web_renderButton(char* buf, size_t size, const char* text, const char* url, const char* style) {
  if (!buf || size == 0)
    return;
  buf[0] = '\0';

  const char* btnStyle = style ? style : "link-btn";
  char temp[128];
  snprintf(temp, sizeof(temp),
           "<a href='%s'><button class='%s'>%s</button></a>", url, btnStyle,
           text);
  strncat(buf, temp, size - 1);
}

void web_renderSpeedBar(char* buf, size_t size, int speed) {
  if (!buf || size == 0)
    return;
  buf[0] = '\0';

  char temp[128];
  snprintf(temp, sizeof(temp),
           "<div class='duty-bar'><div class='duty-fill' "
           "style='width:%d%%;'></div></div>",
           speed);
  strncat(buf, temp, size - 1);
}

void webSendContent(const char* chunk, void* context) {
  WebServerClass* srv = (WebServerClass*)context;
  if (srv && chunk) {
    srv->sendContent(chunk);
  }
}

void web_sendPageStart(WebSendCallback send, void* context, const char* title, PageMode mode) {
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
void web_send_result_page(WebSendCallback send, void* context, const char* action, bool success) {
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

void web_sendApProvisioningPage(WebSendCallback send, void* context) {
  if (!send)
    return;

  web_sendPageStart(send, context, "WiFi Setup", PAGE_MODE_AP);

  send((const char*)"<div class='container'>"
         "<h1>WiFi Setup</h1>"
         "<form method='POST' action='/savewifi'>"
         "<label>WiFi SSID</label>"
         "<input type='text' name='wifiSsid' required placeholder='Enter WiFi name'>"
         "<label>WiFi Password</label>"
         "<input type='password' name='wifiPassword' placeholder='Leave empty for open network'>"
         "<input type='submit' value='Save and Reboot'>"
         "</form>"
         "<div class='note'>Device will reboot and connect to your WiFi network.</div>"
         "</div>", context);

  web_sendPageEnd(send, context);
}

#endif  // TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI