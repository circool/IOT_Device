#ifndef WEB_TEMPLATES_H
#define WEB_TEMPLATES_H

#include <Arduino.h>

// ========== ОБЩИЙ ШАБЛОН СТРАНИЦЫ (НАЧАЛО) ==========
const char HTML_PAGE_START[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html><head><meta charset='UTF-8'>
<meta name='viewport' content='width=device-width, initial-scale=1'>
)rawliteral";

// ========== СТИЛИ ==========
const char HTML_STYLE[] PROGMEM = R"rawliteral(
<style>
body{font-family:Arial;margin:20px;background:#f0f0f0;}
.container{max-width:700px;margin:auto;background:white;padding:20px;border-radius:10px;}
h1{color:#2c3e50;text-align:center;}
h3{color:#2c3e50;border-bottom:1px solid #ccc;padding-bottom:5px;}
label{display:block;margin-top:10px;font-weight:bold;}
input[type=text],input[type=password],input[type=number]{width:100%;padding:8px;margin:5px 0;border:1px solid #ccc;border-radius:4px;box-sizing:border-box;}
input[type=checkbox]{width:20px;height:20px;margin-right:10px;vertical-align:middle;cursor:pointer;transform:scale(1.5);}
input[type=submit],button,.link-btn{background:#555;padding:10px 20px;margin-top:20px;border:none;border-radius:4px;cursor:pointer;width:100%;color:white;text-align:center;text-decoration:none;display:block;box-sizing:border-box;}
input[type=submit]:hover,button:hover,.link-btn:hover{background:#333;}
.info{background:#e7f3ff;padding:10px;border-radius:5px;margin:10px 0;}
.warning{background:#fff3cd;padding:10px;border-radius:5px;margin:10px 0;color:#856404;}
.error{background:#ffebee;padding:10px;border-radius:5px;margin:10px 0;color:#c62828;}
.success{background:#e8f5e9;padding:10px;border-radius:5px;margin:10px 0;color:#2e7d32;}
.row{display:flex;gap:10px;}.row>div{flex:1;}
.password-hint{color:#7f8c8d;margin-top:-2px;margin-bottom:8px;}
.flex-container{display:flex;flex-wrap:wrap;justify-content:center;}
.sensor-card{display:inline-block;width:45%;margin:10px;padding:15px;border-radius:10px;text-align:center;}
.sensor-value{font-size:2em;font-weight:bold;}
.sensor-label{margin-top:5px;}
.status-card{padding:15px;border-radius:10px;text-align:center;margin:10px;}
.duty-bar{background:#e0e0e0;border-radius:10px;margin:10px 0;height:20px;overflow:hidden;}
.duty-fill{background:#2c3e50;height:100%;border-radius:10px;transition:width 0.3s;}
.button-group{display:flex;justify-content:center;gap:10px;margin-top:20px;flex-wrap:wrap;}
a{text-decoration:none;}
</style>
)rawliteral";

// ========== ОБЩИЙ ШАБЛОН СТРАНИЦЫ (КОНЕЦ) ==========
const char HTML_PAGE_END[] PROGMEM = R"rawliteral(
</div></body></html>
)rawliteral";

#endif