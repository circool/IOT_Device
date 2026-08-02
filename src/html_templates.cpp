/**
 * @file html_templates.cpp
 * @brief HTML-константы в PROGMEM
 */

#include "html_templates.h"
#include "settings.h"

#if TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI

extern const char HTML_PAGE_START[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head><meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1'>
)rawliteral";

extern const char HTML_STYLE[] PROGMEM = R"rawliteral(
<style>
body{font-family:Arial;margin:20px;background: #f0f0f0;}
.container{max-width:700px;margin:auto;background:white;padding:20px;border-radius:10px;}
h1{color: #2c3e50;text-align:center;}
h3{color: #2c3e50;border-bottom:1px solid #ccc;padding-bottom:5px;}
label{display:block;margin-top:10px;font-weight:bold;}
input[type=text],input[type=password],input[type=number]{width:100%;padding:8px;margin:5px 0;border:1px solid #ccc;border-radius:4px;font-size:1.2em;box-sizing:border-box;}
input[type=checkbox]{width:20px;height:20px;margin-right:10px;vertical-align:middle;cursor:pointer;transform:scale(1.5);}
input[type=submit],button,.link-btn{color:white;background: #050505;padding:10px 20px;margin-top:20px;border:none;border-radius:4px;cursor:pointer;width:100%;font-size:1em;text-align:center;text-decoration:none;display:block;box-sizing:border-box;}
input[type=submit]:hover,button:hover,.link-btn:hover{background: #030303;}
.block{padding:15px;border-radius:5px;margin:10px 0;}
.info{background:#e7f3ff;}
.warning{background:#fff3cd;color:#856404;}
.error{background:#ffebee;color:#c62828;border:1px solid #ef9a9a;}
.success{background:#e8f5e9;color:#2e7d32;border:1px solid #a5d6a7;}
.center{text-align:center;}
.text_small{font-size:0.9em;}
.text_header{font-size:1.2em;font-weight:bold;}
.row{display:flex;gap:10px;}.row>div{flex:1;}
.password-hint{color:#7f8c8d;margin-top:-2px;margin-bottom:8px;}
.note{margin-top:10px;font-size:0.9em;color:#050505;display:flex;gap:8px;align-items:flex-start;}
.note::before{content:"ℹ️";font-weight:bold;flex-shrink:0;display:inline-block;}
.flex-container{display:flex;flex-wrap:wrap;justify-content:center;}
.sensor-card{display:inline-block;width:45%;margin:10px;padding:15px;border-radius:10px;text-align:center;}
.sensor-value{font-size:2em;font-weight:bold;}
.sensor-label{margin-top:5px;}
.status-card{padding:15px;border-radius:10px;text-align:center;margin:10px;}
.sensor-error{background:#ffebee;padding:15px;border-radius:8px;margin:15px 10px;color:#c62828;text-align:center;border:2px solid #ef9a9a;}
.duty-bar{background:#e0e0e0;border-radius:10px;margin:10px 0;height:20px;overflow:hidden;}
.duty-fill{background:#2c3e50;height:100%;border-radius:10px;transition:width 0.3s;}
.button-group{display:flex;justify-content:center;gap:10px;margin-top:20px;flex-wrap:wrap;}
a{text-decoration:none;}
</style>
)rawliteral";

extern const char HTML_PAGE_END[] PROGMEM = R"rawliteral(
</div></body></html>
)rawliteral";

#endif  // TRANSPORT_TYPE == TRANSPORT_TYPE_WIFI