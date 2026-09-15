#include "WebUI.h"
#include "configs.h"
#include "WiFiScan.h"
#include "ReconMission.h"

extern WiFiScan wifi_scan_obj;
extern ReconMission recon_obj;

WebUI webui_obj;

namespace {
    const char WEBUI_SSID[] = "MarauderUI";
    const char WEBUI_PASSWORD[] = "justcallmekoko";

    // Main UI is embedded in flash so no LittleFS/SD setup is required.
    const char WEBUI_PAGE[] PROGMEM = R"rawliteral(
<!doctype html>
<html lang="pt-BR">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#050708">
<title>MARAUDER // WEBUI</title>
<style>
:root{
  --bg:#050708;
  --panel:#0b1012;
  --panel2:#0f171a;
  --line:#1c2c30;
  --green:#72ff8b;
  --cyan:#35e7ff;
  --text:#e8f2f3;
  --muted:#789096;
  --danger:#ff5268;
}
*{box-sizing:border-box}
body{
  margin:0;background:radial-gradient(circle at top,#102024 0,#050708 48%);
  color:var(--text);font-family:ui-monospace,SFMono-Regular,Menlo,monospace;
}
.app{max-width:900px;margin:auto;padding:14px 14px 90px}
.top{
  display:flex;align-items:center;justify-content:space-between;
  padding:10px 4px 18px;border-bottom:1px solid var(--line);
}
.brand{font-weight:900;letter-spacing:2px;color:var(--green);font-size:18px}
.sub{font-size:10px;color:var(--muted);margin-top:4px}
.dot{width:10px;height:10px;border-radius:50%;background:var(--green);box-shadow:0 0 14px var(--green)}
.grid{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:10px;margin-top:14px}
.card{
  background:linear-gradient(180deg,var(--panel2),var(--panel));
  border:1px solid var(--line);border-radius:14px;padding:14px;
  box-shadow:0 8px 30px #0007;
}
.card.full{grid-column:1/-1}
.label{font-size:10px;color:var(--muted);letter-spacing:1px;text-transform:uppercase}
.value{font-size:20px;margin-top:7px;color:var(--cyan);font-weight:800}
button{
  width:100%;padding:13px 12px;margin-top:10px;border:1px solid #24535a;
  border-radius:10px;background:#0b1c20;color:var(--cyan);
  font:inherit;font-weight:800;cursor:pointer
}
button.primary{border-color:#397e46;color:var(--green);background:#0b1c10}
button.danger{border-color:#6d2734;color:var(--danger)}
button:active{transform:translateY(1px)}
.scanhead{display:flex;align-items:center;justify-content:space-between;gap:10px}
.scanhead button{width:auto;margin:0;padding:9px 12px}
.network{
  margin-top:9px;padding:11px;border:1px solid var(--line);border-radius:10px;
  background:#080d0f
}
.row{display:flex;justify-content:space-between;gap:12px}
.ssid{font-weight:800;color:#fff;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.meta{font-size:10px;color:var(--muted);margin-top:5px}
.rssi{color:var(--green);font-weight:800;white-space:nowrap}
.nav{
  position:fixed;left:0;right:0;bottom:0;padding:10px;
  background:#050708ee;border-top:1px solid var(--line);backdrop-filter:blur(12px)
}
.navin{max-width:900px;margin:auto;display:grid;grid-template-columns:repeat(4,1fr);gap:7px}
.nav button{margin:0;padding:10px 5px;font-size:10px}
.log{
  min-height:70px;max-height:150px;overflow:auto;background:#030506;
  border-radius:9px;padding:10px;color:#8da9ad;font-size:10px;white-space:pre-wrap
}
@media(min-width:700px){.grid{grid-template-columns:repeat(4,minmax(0,1fr))}}
</style>
</head>
<body>
<div class="app">
  <div class="top">
    <div>
      <div class="brand">MARAUDER // WEBUI</div>
      <div class="sub">ESP32-WROOM-32 • HEADLESS CONTROL PANEL</div>
    </div>
    <div class="dot"></div>
  </div>

  <div class="grid">
    <div class="card"><div class="label">Firmware</div><div class="value" id="fw">...</div></div>
    <div class="card"><div class="label">IP</div><div class="value" id="ip">...</div></div>
    <div class="card"><div class="label">Free RAM</div><div class="value" id="ram">...</div></div>
    <div class="card"><div class="label">Wi-Fi APs</div><div class="value" id="apcount">0</div></div>

    <div class="card full">
      <div class="scanhead">
        <div>
          <div class="label">Wi-Fi Recon</div>
          <div class="sub">Passive access-point discovery</div>
        </div>
        <button onclick="scanWifi()">SCAN</button>
      </div>
      <div id="networks" class="meta" style="margin-top:12px">Press SCAN to search.</div>
    </div>

    <div class="card full">
      <div class="label">System</div>
      <button class="primary" onclick="location.reload()">REFRESH DASHBOARD</button>
      <button class="danger" onclick="rebootEsp()">REBOOT ESP32</button>
    </div>

    <div class="card full">
      <div class="label">Console</div>
      <div id="log" class="log">[WEBUI] Ready.</div>
    </div>
  </div>
</div>

<div class="nav"><div class="navin">
  <button onclick="window.scrollTo({top:0,behavior:'smooth'})">HOME</button>
  <button onclick="scanWifi()">WI-FI</button>
  <button onclick="document.getElementById('log').scrollIntoView({behavior:'smooth'})">LOG</button>
  <button onclick="location.reload()">REFRESH</button>
</div></div>

<script>
const $=id=>document.getElementById(id);
function log(s){$('log').textContent+='\\n'+s;$('log').scrollTop=$('log').scrollHeight}
async function status(){
  try{
    const r=await fetch('/api/status'); const j=await r.json();
    $('fw').textContent=j.version;
    $('ip').textContent=j.ip;
    $('ram').textContent=j.freeHeap+' B';
    $('apcount').textContent=j.aps;
  }catch(e){log('[ERR] status '+e)}
}
async function scanWifi(){
  $('networks').innerHTML='<div class="meta">Scanning...</div>';
  log('[WEBUI] Wi-Fi scan started');
  try{
    const r=await fetch('/api/wifi/scan'); const j=await r.json();
    $('apcount').textContent=j.count;
    if(!j.networks.length){
      $('networks').innerHTML='<div class="meta">No networks found.</div>';
      return;
    }
    $('networks').innerHTML=j.networks.map(n=>
      '<div class="network"><div class="row"><div class="ssid">'+esc(n.ssid||'[hidden]')+
      '</div><div class="rssi">'+n.rssi+' dBm</div></div><div class="meta">CH '+n.channel+
      ' • '+esc(n.auth)+' • '+esc(n.bssid)+'</div></div>'
    ).join('');
    log('[WEBUI] '+j.count+' AP(s) found');
  }catch(e){$('networks').innerHTML='<div class="meta">Scan error.</div>';log('[ERR] scan '+e)}
}
function esc(s){return String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#039;'}[c]))}
async function rebootEsp(){
  if(!confirm('Reboot ESP32?'))return;
  try{await fetch('/api/reboot',{method:'POST'});log('[WEBUI] Rebooting...')}
  catch(e){log('[WEBUI] Connection closed: reboot expected')}
}
status();
</script>
</body>
</html>
)rawliteral";

    String authName(wifi_auth_mode_t mode) {
        switch (mode) {
            case WIFI_AUTH_OPEN: return "OPEN";
            case WIFI_AUTH_WEP: return "WEP";
            case WIFI_AUTH_WPA_PSK: return "WPA";
            case WIFI_AUTH_WPA2_PSK: return "WPA2";
            case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
            case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-ENT";
            case WIFI_AUTH_WPA3_PSK: return "WPA3";
            case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3";
            default: return "OTHER";
        }
    }

    String jsonEscape(const String &in) {
        String out;
        out.reserve(in.length() + 8);
        for (size_t i = 0; i < in.length(); ++i) {
            char c = in[i];
            if (c == '"' || c == '\\') {
                out += '\\';
                out += c;
            } else if (c == '\n') {
                out += "\\n";
            } else if (c == '\r') {
                out += "\\r";
            } else {
                out += c;
            }
        }
        return out;
    }
}

WebUI::WebUI() : server(80), active(false) {}

bool WebUI::running() const {
    return active;
}

void WebUI::start() {
    if (active) return;

    // Stop Marauder activity before putting the ESP32 into AP mode.
    recon_obj.stop();
    wifi_scan_obj.StartScan(WIFI_SCAN_OFF);
    delay(100);

    WiFi.mode(WIFI_AP);
    delay(100);
    WiFi.softAP(WEBUI_SSID, WEBUI_PASSWORD);

    setupRoutes();
    server.begin();
    active = true;

    Serial.println(F("[WEBUI] Wi-Fi: MarauderUI"));
    Serial.println(F("[WEBUI] Senha: justcallmekoko"));
    Serial.print(F("[WEBUI] Acesse: http://"));
    Serial.println(WiFi.softAPIP());
}

void WebUI::setupRoutes() {
    server.on("/", HTTP_GET, handleRoot);
    server.on("/api/status", HTTP_GET, handleStatus);
    server.on("/api/wifi/scan", HTTP_GET, handleWifiScan);
    server.on("/api/reboot", HTTP_POST, handleReboot);
}

void WebUI::handleRoot(AsyncWebServerRequest *request) {
    request->send_P(200, "text/html; charset=utf-8", WEBUI_PAGE);
}

void WebUI::handleStatus(AsyncWebServerRequest *request) {
    String json = "{";
    json += "\"version\":\"" + jsonEscape(MARAUDER_VERSION) + "\",";
    json += "\"ip\":\"" + WiFi.softAPIP().toString() + "\",";
    json += "\"freeHeap\":" + String(ESP.getFreeHeap()) + ",";
    json += "\"aps\":" + String((unsigned)wifi_scan_obj.retainedAccessPointCount()) + ",";
    json += "\"stations\":" + String((unsigned)wifi_scan_obj.retainedStationCount()) + ",";
    json += "\"ble\":" + String((unsigned)wifi_scan_obj.retainedBleDeviceCount());
    json += "}";
    request->send(200, "application/json; charset=utf-8", json);
}

void WebUI::handleWifiScan(AsyncWebServerRequest *request) {
    // Passive scan only: no packet injection or attack operation is performed.
    int n = WiFi.scanNetworks(false, true);

    String json = "{\"count\":" + String(n < 0 ? 0 : n) + ",\"networks\":[";
    for (int i = 0; i < n; ++i) {
        if (i) json += ",";
        int ch = WiFi.channel(i);
        json += "{";
        json += "\"ssid\":\"" + jsonEscape(WiFi.SSID(i)) + "\",";
        json += "\"rssi\":" + String(WiFi.RSSI(i)) + ",";
        json += "\"channel\":" + String(ch) + ",";
        json += "\"bssid\":\"" + WiFi.BSSIDstr(i) + "\",";
        json += "\"auth\":\"" + authName(WiFi.encryptionType(i)) + "\"";
        json += "}";
    }
    json += "]}";

    WiFi.scanDelete();
    request->send(200, "application/json; charset=utf-8", json);
}

void WebUI::handleReboot(AsyncWebServerRequest *request) {
    request->send(200, "text/plain; charset=utf-8", "Rebooting");
    delay(250);
    ESP.restart();
}
