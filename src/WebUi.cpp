#include "WebUi.h"
#include "BuildInfo.h"
#include <WiFi.h>
#include <time.h>
#include <esp_system.h>

namespace {
const char* resetReasonText(esp_reset_reason_t reason) {
  switch (reason) {
    case ESP_RST_POWERON: return "power_on";
    case ESP_RST_EXT: return "external_reset";
    case ESP_RST_SW: return "software_reset";
    case ESP_RST_PANIC: return "panic";
    case ESP_RST_INT_WDT: return "interrupt_watchdog";
    case ESP_RST_TASK_WDT: return "task_watchdog";
    case ESP_RST_WDT: return "watchdog";
    case ESP_RST_DEEPSLEEP: return "deep_sleep";
    case ESP_RST_BROWNOUT: return "brownout";
    case ESP_RST_SDIO: return "sdio";
    default: return "unknown";
  }
}
}

WebUi::WebUi(AppConfig &cfg, RuntimeData &data, ConfigStore &store, MqttManager &mqtt, SensorHub &sensors)
  : _cfg(cfg), _data(data), _store(store), _mqtt(mqtt), _sensors(sensors), _server(80) {}

uint32_t WebUi::nowEpoch() {
  time_t t = time(nullptr);
  return t > 1700000000 ? (uint32_t)t : 0;
}

bool WebUi::auth() {
  if (_cfg.webPassword.isEmpty()) return true;
  if (_server.authenticate(_cfg.webUser.c_str(), _cfg.webPassword.c_str())) return true;
  _server.requestAuthentication();
  return false;
}

String WebUi::esc(const String &s) const {
  String o = s;
  o.replace("&", "&amp;"); o.replace("\"", "&quot;"); o.replace("<", "&lt;");
  o.replace(">", "&gt;"); o.replace("'", "&#39;");
  return o;
}

String WebUi::chk(bool v) const { return v ? " checked" : ""; }
String WebUi::sel(bool v) const { return v ? " selected" : ""; }

String WebUi::pageStart(const String &title) const {
  String h;
  h.reserve(10000);
  h += F("<!doctype html><html lang='it'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>");
  h += "<title>" + esc(title) + "</title>";
  h += F(R"CSS(<style>
:root{color-scheme:dark;--bg:#08111f;--panel:#0d1829;--panel2:#101d30;--line:#26384e;--text:#e8eef8;--muted:#8fa7c5;--ok:#30d99a;--warn:#f0b24a;--bad:#ff7070;--blue:#55aef6;--green:#3fd39b}
*{box-sizing:border-box}body{margin:0;background:linear-gradient(180deg,#08111f,#07101c);color:var(--text);font-family:system-ui,-apple-system,Segoe UI,Arial,sans-serif}main{max-width:1500px;margin:auto;padding:11px 13px 18px}.top{display:flex;gap:10px;align-items:center;justify-content:space-between;flex-wrap:wrap}.brand{flex:1;min-width:250px}.title{font-size:1.2rem;font-weight:850}.sub,.muted{color:var(--muted);font-size:.78rem}.tools{display:flex;gap:6px;align-items:center;flex-wrap:wrap}.pill{display:inline-flex;gap:6px;align-items:center;border:1px solid var(--line);background:var(--panel);padding:6px 9px;border-radius:99px;font-size:.73rem;font-weight:750;color:var(--muted)}.pill:before{content:'';width:7px;height:7px;border-radius:50%;background:#65758a}.pill.ok:before{background:var(--ok)}.pill.warn:before{background:var(--warn)}.pill.bad:before{background:var(--bad)}.btn{background:#163f63;color:#eef7ff;border:1px solid #2c5e83;border-radius:8px;padding:7px 10px;text-decoration:none;font:inherit;font-size:.82rem;font-weight:750;cursor:pointer}.btn.ok{background:#14583f;border-color:#267557}.btn.warn{background:#6c4717;border-color:#95651e}.btn.bad{background:#672d35;border-color:#8c3d48}.tabs{display:flex;gap:6px;margin-top:10px;padding:4px;border:1px solid var(--line);border-radius:11px;background:#0a1525;overflow:auto}.tab{border:0;background:transparent;color:var(--muted);padding:7px 13px;border-radius:8px;font-weight:800;white-space:nowrap;cursor:pointer}.tab.active{background:#16304a;color:#eef7ff;box-shadow:inset 0 0 0 1px #3c6b91}.page{display:none}.page.active{display:block}.panel{border:1px solid var(--line);border-radius:12px;background:var(--panel);overflow:hidden;margin-top:9px}.head{padding:9px 12px;border-bottom:1px solid var(--line);font-weight:780;display:flex;justify-content:space-between;gap:8px;align-items:center}.grid{display:grid;gap:8px;padding:9px}.g4{grid-template-columns:repeat(4,minmax(0,1fr))}.g3{grid-template-columns:repeat(3,minmax(0,1fr))}.g2{grid-template-columns:repeat(2,minmax(0,1fr))}.card{border:1px solid var(--line);border-radius:11px;background:linear-gradient(180deg,#101d30,#0e1a2b);overflow:hidden}.ct{padding:8px 10px;border-bottom:1px solid var(--line);font-weight:760;font-size:.91rem;display:flex;align-items:center;gap:7px}.ct:before{content:'';width:7px;height:7px;border-radius:50%;background:#65758a}.card.ok .ct:before{background:var(--ok)}.card.warn .ct:before{background:var(--warn)}.card.bad .ct:before{background:var(--bad)}.body{padding:2px 10px 7px}.row{display:flex;justify-content:space-between;gap:8px;padding:6px 0;border-bottom:1px solid #1c2b3e;font-size:.83rem}.row:last-child{border:0}.name{color:#b5c8e1}.value{font-weight:750;text-align:right}.row:first-child .value{font-size:1.12rem;color:#f4f8ff}.foot{padding:5px 10px;background:#0a1525;color:var(--muted);font-size:.67rem;min-height:26px}.cfggrid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:9px;margin-top:9px}.cfg{border:1px solid var(--line);border-radius:11px;background:var(--panel);padding:9px 10px}.cfg h2{font-size:.93rem;margin:0 0 6px;padding-bottom:6px;border-bottom:1px solid #1c2b3e}.fields{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:4px 7px}.full{grid-column:1/-1}label{display:block;color:#c8d7ea;font-size:.74rem;margin:4px 0 2px}input,select,textarea{width:100%;padding:6px 7px;border:1px solid #38516d;border-radius:7px;background:#e9edf2;color:#0d1722;font:inherit;font-size:.8rem}input[type=checkbox]{width:auto}.checks{display:flex;gap:5px 12px;flex-wrap:wrap;margin-top:6px}.check{display:inline-flex;gap:6px;align-items:center;font-size:.76rem}.hint{font-size:.68rem;color:var(--muted);margin-top:5px}.sticky{position:sticky;bottom:0;display:flex;justify-content:flex-end;gap:6px;margin-top:9px;padding:8px;border:1px solid var(--line);border-radius:11px;background:#0a1525ee}.diag{font-size:.76rem;line-height:1.55}.mono{font-family:ui-monospace,Consolas,monospace;word-break:break-word}.big{font-size:1.18rem;font-weight:800}.goodtxt{color:#91e8c4}.badtxt{color:#ff9b9b}
@media(max-width:1050px){.g4,.g3,.cfggrid{grid-template-columns:repeat(2,minmax(0,1fr))}}@media(max-width:680px){.g4,.g3,.g2,.cfggrid,.fields{grid-template-columns:1fr}.tools{width:100%}.btn,.pill{flex:1;justify-content:center}.sticky{position:static}}
</style>)CSS");
  h += F("</head><body><main>");
  return h;
}

String WebUi::pageEnd() const { return F("</main></body></html>"); }

void WebUi::handleRoot() {
  if (!auth()) return;
  String h = pageStart(_cfg.deviceName);
  h.reserve(24000);
  h += "<div class='top'><div class='brand'><div class='title'>" + esc(_cfg.deviceName) + "</div><div class='sub'>" + String(FW_NAME) + " v" + FW_VERSION + " · ESP32 sempre attivo · sleep solo SDS011</div></div>";
  h += F("<div class='tools'><span id='wifi' class='pill'>Wi-Fi</span><span id='mq' class='pill'>MQTT</span><span id='sds' class='pill'>SDS011</span><a class='btn' href='/config'>Configurazione</a><a class='btn' href='/update'>OTA</a></div></div>");
  h += F("<div class='tabs'><button class='tab active' data-p='sens'>Sensori</button><button class='tab' data-p='diag'>Diagnostica</button></div>");

  h += F(R"HTML(<div id='p-sens' class='page active'>
<section class='panel'><div class='head'><span>Ambiente</span><span id='stamp' class='muted'>-</span></div><div class='grid g4'>
<div id='c-bh' class='card'><div class='ct'>BH1750</div><div class='body'><div class='row'><span class='name'>Luce</span><span id='bh' class='value'>-</span></div><div class='row'><span class='name'>I2C</span><span id='bha' class='value'>-</span></div></div><div id='bhf' class='foot'>-</div></div>
<div id='c-bme' class='card'><div class='ct'>BME280</div><div class='body'><div class='row'><span class='name'>Temperatura</span><span id='bt' class='value'>-</span></div><div class='row'><span class='name'>Umidita</span><span id='bhm' class='value'>-</span></div><div class='row'><span class='name'>Pressione</span><span id='bp' class='value'>-</span></div></div><div id='bmf' class='foot'>-</div></div>
<div id='c-dht' class='card'><div class='ct'>DHT11</div><div class='body'><div class='row'><span class='name'>Temperatura</span><span id='dt' class='value'>-</span></div><div class='row'><span class='name'>Umidita</span><span id='dh' class='value'>-</span></div><div class='row'><span class='name'>Dew point</span><span id='dd' class='value'>-</span></div></div><div id='df' class='foot'>-</div></div>
<div id='c-uv' class='card'><div class='ct'>UV analogico</div><div class='body'><div class='row'><span class='name'>UV Index</span><span id='ui' class='value'>-</span></div><div class='row'><span class='name'>Tensione</span><span id='umv' class='value'>-</span></div><div class='row'><span class='name'>ADC raw</span><span id='ur' class='value'>-</span></div></div><div id='uf' class='foot'>-</div></div>
</div></section>
<section class='panel'><div class='head'><span>Qualita aria e fulmini</span></div><div class='grid g2'>
<div id='c-sds' class='card'><div class='ct'>SDS011</div><div class='body'><div class='row'><span class='name'>PM2.5</span><span id='p25' class='value'>-</span></div><div class='row'><span class='name'>PM10</span><span id='p10' class='value'>-</span></div><div class='row'><span class='name'>Stato</span><span id='ss' class='value'>-</span></div><div class='row'><span class='name'>Prossima</span><span id='sn' class='value'>-</span></div><div class='tools'><button class='btn ok' onclick='post("/api/sds/measure")'>Misura ora</button><button class='btn warn' onclick='post("/api/sds/sleep")'>Sleep</button></div></div><div id='sf' class='foot'>-</div></div>
<div id='c-as' class='card'><div class='ct'>AS3935</div><div class='body'><div class='row'><span class='name'>Evento</span><span id='ae' class='value'>-</span></div><div class='row'><span class='name'>Distanza</span><span id='ad' class='value'>-</span></div><div class='row'><span class='name'>Energia</span><span id='aen' class='value'>-</span></div><div class='row'><span class='name'>Fulmini</span><span id='ac' class='value'>-</span></div></div><div id='af' class='foot'>-</div></div>
</div></section>
<section class='panel'><div class='head'><span>Alimentazione e sistema</span></div><div class='grid g3'>
<div id='c-ina' class='card'><div class='ct'>INA219</div><div class='body'><div class='row'><span class='name'>Bus</span><span id='iv' class='value'>-</span></div><div class='row'><span class='name'>Corrente</span><span id='ii' class='value'>-</span></div><div class='row'><span class='name'>Potenza</span><span id='ipw' class='value'>-</span></div></div><div id='inf' class='foot'>-</div></div>
<div class='card ok'><div class='ct'>Sistema</div><div class='body'><div class='row'><span class='name'>IP</span><span id='sip' class='value'>-</span></div><div class='row'><span class='name'>RSSI</span><span id='sr' class='value'>-</span></div><div class='row'><span class='name'>Heap</span><span id='sh' class='value'>-</span></div><div class='row'><span class='name'>Uptime</span><span id='su' class='value'>-</span></div></div><div id='sysf' class='foot'>-</div></div>
<div class='card'><div class='ct'>Relay</div><div class='body'><div class='row'><span class='name'>Stato</span><span id='rs' class='value'>-</span></div><button class='btn' onclick='post("/api/relay/toggle")'>Toggle</button></div><div class='foot'>GPIO configurabile</div></div>
</div></section></div>
<div id='p-diag' class='page'><section class='panel'><div class='head'><span>Diagnostica</span><button class='btn' onclick='scan()'>Scansione I2C</button></div><div class='grid g3'>
<div class='card'><div class='ct'>Health</div><div class='body diag'><div class='big' id='hc'>-</div><div id='hl'>-</div></div></div>
<div class='card'><div class='ct'>ESP32</div><div class='body diag'><div>Chip: <b id='chip'>-</b></div><div>Reset: <b id='rr'>-</b></div><div>Heap min: <b id='mh'>-</b></div><div>Flash: <b id='fl'>-</b></div></div></div>
<div class='card'><div class='ct'>Rete / MQTT</div><div class='body diag'><div>SSID: <b id='ssid'>-</b></div><div>MQTT: <b id='md'>-</b></div><div>Reconnect: <b id='mr'>-</b></div></div></div>
<div class='card'><div class='ct'>I2C</div><div class='body diag mono' id='scan'>premere Scansione I2C</div></div>
<div class='card'><div class='ct'>SDS011</div><div class='body diag'><div>Cicli OK/KO: <b id='scy'>-</b></div><div>Campioni: <b id='scp'>-</b></div><div>Errore: <b id='ser'>-</b></div></div></div>
<div class='card'><div class='ct'>Errori sensori</div><div class='body diag'><div>BH/BME/DHT/INA/UV</div><div class='mono' id='errs'>-</div></div></div>
</div></section></div>
<script>
const q=x=>document.getElementById(x),f=(v,d=1)=>v==null?'-':Number(v).toFixed(d),hx=v=>v==null?'-':'0x'+Number(v).toString(16).toUpperCase().padStart(2,'0');
function sec(s){s=Number(s||0);if(s>=3600)return Math.floor(s/3600)+'h '+Math.floor((s%3600)/60)+'m';if(s>=60)return Math.floor(s/60)+'m '+s%60+'s';return s+'s'}
function state(id,ok,en=true){let e=q(id);e.classList.remove('ok','warn','bad');e.classList.add(!en?'warn':ok?'ok':'bad')}
function pill(id,ok,text){let e=q(id);e.className='pill '+(ok?'ok':'bad');e.textContent=text}
async function post(u){await fetch(u,{method:'POST'});setTimeout(refresh,150)}async function scan(){q('scan').textContent=await(await fetch('/api/i2c')).text()}
async function refresh(){try{let d=await(await fetch('/api/status')).json();q('stamp').textContent='Aggiornato '+new Date().toLocaleTimeString();pill('wifi',d.system.wifi,'Wi-Fi '+(d.system.wifi?d.system.rssi_dbm+' dBm':'offline'));pill('mq',d.system.mqtt,d.system.mqtt?'MQTT online':'MQTT offline');q('sds').className='pill '+(['sleeping','warming','sampling'].includes(d.sds011.state)?'ok':'warn');q('sds').textContent='SDS '+d.sds011.state;
state('c-bh',d.bh1750.ok,d.bh1750.enabled);q('bh').textContent=d.bh1750.ok?f(d.bh1750.illuminance_lux)+' lx':'-';q('bha').textContent=hx(d.bh1750.detected_address);q('bhf').textContent=d.bh1750.last_error||'operativo';
state('c-bme',d.bme280.ok,d.bme280.enabled);q('bt').textContent=d.bme280.ok?f(d.bme280.temperature_c)+' °C':'-';q('bhm').textContent=d.bme280.ok?f(d.bme280.humidity_pct)+' %':'-';q('bp').textContent=d.bme280.ok?f(d.bme280.pressure_hpa)+' hPa':'-';q('bmf').textContent=d.bme280.last_error||('I2C '+hx(d.bme280.detected_address));
state('c-dht',d.dht11.ok,d.dht11.enabled);q('dt').textContent=d.dht11.ok?f(d.dht11.temperature_c)+' °C':'-';q('dh').textContent=d.dht11.ok?f(d.dht11.humidity_pct)+' %':'-';q('dd').textContent=d.dht11.ok?f(d.dht11.dewpoint_c)+' °C':'-';q('df').textContent=d.dht11.last_error||'GPIO '+d.dht11.pin;
state('c-uv',d.uv.ok,d.uv.enabled);q('ui').textContent=d.uv.ok?f(d.uv.uv_index,2):'-';q('umv').textContent=d.uv.millivolts+' mV';q('ur').textContent=d.uv.raw_adc;q('uf').textContent=d.uv.last_error||'ADC coerente';
let sok=d.sds011.ok||['sleeping','warming','sampling'].includes(d.sds011.state);state('c-sds',sok,d.sds011.enabled);q('p25').textContent=d.sds011.pm25_ugm3==null?'-':f(d.sds011.pm25_ugm3)+' µg/m³';q('p10').textContent=d.sds011.pm10_ugm3==null?'-':f(d.sds011.pm10_ugm3)+' µg/m³';q('ss').textContent=d.sds011.state;q('sn').textContent=d.sds011.state==='sleeping'?sec(d.sds011.next_measurement_s):sec(d.sds011.stage_remaining_s);q('sf').textContent=d.sds011.last_error||('cicli '+d.sds011.successful_cycles+'/'+d.sds011.failed_cycles);
state('c-as',d.as3935.ok,d.as3935.enabled);q('ae').textContent=d.as3935.last_event;q('ad').textContent=d.as3935.distance_km==null?'-':d.as3935.distance_km+' km';q('aen').textContent=d.as3935.energy;q('ac').textContent=d.as3935.lightning_count;q('af').textContent=d.as3935.last_error||('I2C '+hx(d.as3935.detected_address));
state('c-ina',d.ina219.ok,d.ina219.enabled);q('iv').textContent=d.ina219.ok?f(d.ina219.bus_voltage_v,3)+' V':'-';q('ii').textContent=d.ina219.ok?f(d.ina219.current_ma)+' mA':'-';q('ipw').textContent=d.ina219.ok?f(d.ina219.power_mw)+' mW':'-';q('inf').textContent=d.ina219.last_error||('I2C '+hx(d.ina219.detected_address));
q('sip').textContent=d.system.ip;q('sr').textContent=d.system.rssi_dbm+' dBm';q('sh').textContent=Math.round(d.system.free_heap/1024)+' KB';q('su').textContent=sec(d.system.uptime_s);q('sysf').textContent='v'+d.system.firmware+' · boot '+d.system.boot_count;q('rs').textContent=d.relay.state?'ON':'OFF';
let mods=[d.bh1750,d.bme280,d.dht11,d.ina219,d.uv,{enabled:d.sds011.enabled,ok:sok},d.as3935].filter(x=>x.enabled),good=mods.filter(x=>x.ok).length;q('hc').textContent=good+' / '+mods.length;q('hl').textContent=good===mods.length?'Tutti i sensori abilitati operativi':'Verificare i sensori in rosso';q('chip').textContent=d.system.chip_model+' r'+d.system.chip_revision;q('rr').textContent=d.system.reset_reason;q('mh').textContent=Math.round(d.system.min_free_heap/1024)+' KB';q('fl').textContent=(d.system.flash_size/1048576).toFixed(1)+' MB';q('ssid').textContent=d.system.ssid;q('md').textContent=d.system.mqtt?'online':'offline ('+d.system.mqtt_state+')';q('mr').textContent=d.system.mqtt_reconnects;q('scy').textContent=d.sds011.successful_cycles+' / '+d.sds011.failed_cycles;q('scp').textContent=d.sds011.samples_collected+' / '+d.sds011.target_samples;q('ser').textContent=d.sds011.last_error||'nessuno';q('errs').textContent=[d.bh1750.failures,d.bme280.failures,d.dht11.failures,d.ina219.failures,d.uv.failures].join(' / ');
}catch(e){q('stamp').textContent='Errore: '+e.message}}
document.querySelectorAll('.tab').forEach(b=>b.onclick=()=>{document.querySelectorAll('.tab').forEach(x=>x.classList.remove('active'));document.querySelectorAll('.page').forEach(x=>x.classList.remove('active'));b.classList.add('active');q('p-'+b.dataset.p).classList.add('active')});refresh();setInterval(refresh,2000);
</script>)HTML");
  h += pageEnd();
  _server.send(200, "text/html; charset=utf-8", h);
}

void WebUi::handleApiStatus() {
  if (!auth()) return;
  JsonDocument doc;
  JsonObject sys = doc["system"].to<JsonObject>();
  sys["wifi"] = WiFi.status() == WL_CONNECTED;
  sys["ip"] = WiFi.status() == WL_CONNECTED ? WiFi.localIP().toString() : WiFi.softAPIP().toString();
  sys["rssi_dbm"] = WiFi.status() == WL_CONNECTED ? WiFi.RSSI() : 0;
  sys["mqtt"] = _mqtt.connected();
  sys["mqtt_state"] = _mqtt.state();
  sys["mqtt_reconnects"] = _data.mqttReconnects;
  sys["uptime_s"] = millis()/1000UL;
  sys["free_heap"] = ESP.getFreeHeap();
  sys["min_free_heap"] = ESP.getMinFreeHeap();
  sys["boot_count"] = _data.bootCount;
  sys["firmware"] = FW_VERSION;
  sys["ssid"] = WiFi.status()==WL_CONNECTED ? WiFi.SSID() : String("AP");
  sys["chip_model"] = ESP.getChipModel();
  sys["chip_revision"] = ESP.getChipRevision();
  sys["flash_size"] = ESP.getFlashChipSize();
  sys["reset_reason"] = resetReasonText(esp_reset_reason());

  JsonObject relay = doc["relay"].to<JsonObject>(); relay["enabled"]=_cfg.relayEnabled; relay["state"]=_data.relayState;
  JsonObject bh = doc["bh1750"].to<JsonObject>(); bh["enabled"]=_cfg.bh1750Enabled; bh["ok"]=_data.bh1750Ok; bh["illuminance_lux"]=_data.bh1750Lux; bh["detected_address"]=_data.bh1750DetectedAddress; bh["failures"]=_data.bh1750Failures; bh["last_error"]=_data.bh1750LastError;
  JsonObject bme = doc["bme280"].to<JsonObject>(); bme["enabled"]=_cfg.bmeEnabled; bme["ok"]=_data.bmeOk; bme["temperature_c"]=_data.bmeTempC; bme["humidity_pct"]=_data.bmeHumidity; bme["pressure_hpa"]=_data.bmePressureHpa; bme["dewpoint_c"]=_data.bmeDewPointC; bme["detected_address"]=_data.bmeDetectedAddress; bme["failures"]=_data.bmeFailures; bme["last_error"]=_data.bmeLastError;
  JsonObject dht = doc["dht11"].to<JsonObject>(); dht["enabled"]=_cfg.dhtEnabled; dht["ok"]=_data.dhtOk; dht["pin"]=_cfg.dhtPin; dht["temperature_c"]=_data.dhtTempC; dht["humidity_pct"]=_data.dhtHumidity; dht["dewpoint_c"]=_data.dhtDewPointC; dht["failures"]=_data.dhtFailures; dht["last_error"]=_data.dhtLastError;
  JsonObject ina = doc["ina219"].to<JsonObject>(); ina["enabled"]=_cfg.inaEnabled; ina["ok"]=_data.inaOk; ina["bus_voltage_v"]=_data.inaBusVoltageV; ina["load_voltage_v"]=_data.inaLoadVoltageV; ina["current_ma"]=_data.inaCurrentMa; ina["power_mw"]=_data.inaPowerMw; ina["detected_address"]=_data.inaDetectedAddress; ina["failures"]=_data.inaFailures; ina["last_error"]=_data.inaLastError;
  JsonObject uv = doc["uv"].to<JsonObject>(); uv["enabled"]=_cfg.uvEnabled; uv["ok"]=_data.uvOk; uv["raw_adc"]=_data.uvRawAdc; uv["millivolts"]=_data.uvMilliVolts; uv["uv_index"]=_data.uvIndex; uv["consistent"]=_data.uvConsistent; uv["failures"]=_data.uvFailures; uv["last_error"]=_data.uvLastError;
  JsonObject sds = doc["sds011"].to<JsonObject>(); sds["enabled"]=_cfg.sdsEnabled; sds["ok"]=_data.sdsOk; sds["state"]=_data.sdsState; sds["pm25_ugm3"]=_data.pm25; sds["pm10_ugm3"]=_data.pm10; sds["next_measurement_s"]=_data.sdsNextInSec; sds["stage_remaining_s"]=_data.sdsStageRemainingSec; sds["samples_collected"]=_data.sdsSamplesCollected; sds["target_samples"]=_cfg.sdsSamples; sds["successful_cycles"]=_data.sdsSuccessfulCycles; sds["failed_cycles"]=_data.sdsFailedCycles; sds["last_error"]=_data.sdsLastError;
  JsonObject as = doc["as3935"].to<JsonObject>(); as["enabled"]=_cfg.as3935Enabled; as["ok"]=_data.as3935Ok; as["last_event"]=_data.as3935LastEvent; if(_data.as3935DistanceKm>=0) as["distance_km"]=_data.as3935DistanceKm; as["energy"]=_data.as3935Energy; as["lightning_count"]=_data.as3935EventCount; as["detected_address"]=_data.as3935DetectedAddress; as["last_error"]=_data.as3935LastError;

  String out; serializeJson(doc,out); _server.send(200,"application/json",out);
}

void WebUi::handleConfig() {
  if (!auth()) return;
  String h=pageStart("Configurazione"); h.reserve(22000);
  h += "<div class='top'><div class='brand'><div class='title'>Configurazione</div><div class='sub'>"+String(FW_NAME)+" v"+FW_VERSION+" · layout compatto</div></div><div class='tools'><a class='btn' href='/'>Dashboard</a><a class='btn' href='/update'>OTA</a></div></div>";
  h += F("<div class='tabs'><button type='button' class='tab active' data-p='net'>Rete & MQTT</button><button type='button' class='tab' data-p='sen'>Sensori</button><button type='button' class='tab' data-p='adv'>Avanzate</button></div><form method='POST' action='/save'>");
  h += "<div id='p-net' class='page active'><div class='cfggrid'>";
  h += "<div class='cfg'><h2>Wi-Fi</h2><div class='fields'><div><label>SSID</label><input name='ssid' value='"+esc(_cfg.wifiSsid)+"'></div><div><label>Password</label><input type='password' name='wpass' value='"+esc(_cfg.wifiPassword)+"'></div><div><label>Nome dispositivo</label><input name='dev' value='"+esc(_cfg.deviceName)+"'></div><div><label>Timezone</label><input name='tz' value='"+esc(_cfg.timezone)+"'></div></div><div class='checks'><label class='check'><input type='checkbox' name='wstatic'"+chk(_cfg.wifiStaticIp)+">IPv4 statico</label></div></div>";
  h += "<div class='cfg'><h2>IPv4</h2><div class='fields'><div><label>IP</label><input name='wip' value='"+esc(_cfg.wifiIp)+"'></div><div><label>Gateway</label><input name='wgw' value='"+esc(_cfg.wifiGateway)+"'></div><div><label>Subnet</label><input name='wsub' value='"+esc(_cfg.wifiSubnet)+"'></div><div><label>DNS</label><input name='wdns1' value='"+esc(_cfg.wifiDns1)+"'></div></div></div>";
  h += "<div class='cfg'><h2>MQTT</h2><div class='fields'><div><label>Host</label><input name='mqhost' value='"+esc(_cfg.mqttHost)+"'></div><div><label>Porta</label><input name='mqport' type='number' value='"+String(_cfg.mqttPort)+"'></div><div><label>Utente</label><input name='mquser' value='"+esc(_cfg.mqttUser)+"'></div><div><label>Password</label><input type='password' name='mqpass' value='"+esc(_cfg.mqttPassword)+"'></div><div class='full'><label>Base topic</label><input name='mqtopic' value='"+esc(_cfg.mqttBaseTopic)+"'></div></div></div></div></div>";
  h += "<div id='p-sen' class='page'><div class='cfggrid'>";
  h += "<div class='cfg'><h2>BME280 / BH1750</h2><div class='checks'><label class='check'><input type='checkbox' name='bme_en'"+chk(_cfg.bmeEnabled)+">BME280</label><label class='check'><input type='checkbox' name='bh_en'"+chk(_cfg.bh1750Enabled)+">BH1750</label></div><div class='fields'><div><label>BME addr hex</label><input name='bme_a' value='"+String(_cfg.bmeAddress,HEX)+"'></div><div><label>BH addr hex</label><input name='bh_a' value='"+String(_cfg.bh1750Address,HEX)+"'></div><div><label>Temp offset</label><input name='bme_to' value='"+String(_cfg.bmeTemperatureOffsetC,2)+"'></div><div><label>Press offset</label><input name='bme_po' value='"+String(_cfg.bmePressureOffsetHpa,2)+"'></div></div></div>";
  h += "<div class='cfg'><h2>DHT11 / UV</h2><div class='checks'><label class='check'><input type='checkbox' name='dht_en'"+chk(_cfg.dhtEnabled)+">DHT11</label><label class='check'><input type='checkbox' name='uv_en'"+chk(_cfg.uvEnabled)+">UV</label></div><div class='fields'><div><label>DHT GPIO</label><input name='dht_p' type='number' value='"+String(_cfg.dhtPin)+"'></div><div><label>UV GPIO</label><input name='uv_p' type='number' value='"+String(_cfg.uvPin)+"'></div><div><label>UV mV/UVI</label><input name='uv_k' value='"+String(_cfg.uvMvPerIndex,1)+"'></div><div><label>UV zero mV</label><input name='uv_z' value='"+String(_cfg.uvZeroMv,1)+"'></div></div></div>";
  h += "<div class='cfg'><h2>INA219</h2><div class='checks'><label class='check'><input type='checkbox' name='ina_en'"+chk(_cfg.inaEnabled)+">Abilitato</label></div><div class='fields'><div><label>Addr hex</label><input name='ina_a' value='"+String(_cfg.inaAddress,HEX)+"'></div><div><label>Offset V</label><input name='ina_vo' value='"+String(_cfg.inaBusVoltageOffsetV,3)+"'></div><div><label>Offset mA</label><input name='ina_io' value='"+String(_cfg.inaCurrentOffsetMa,2)+"'></div></div></div>";
  h += "<div class='cfg'><h2>SDS011</h2><div class='checks'><label class='check'><input type='checkbox' name='sds_en'"+chk(_cfg.sdsEnabled)+">Abilitato</label></div><div class='fields'><div><label>RX</label><input name='sds_rx' type='number' value='"+String(_cfg.sdsRxPin)+"'></div><div><label>TX</label><input name='sds_tx' type='number' value='"+String(_cfg.sdsTxPin)+"'></div><div><label>Ciclo min</label><input name='sds_cm' type='number' value='"+String(_cfg.sdsCycleMinutes)+"'></div><div><label>Warm-up s</label><input name='sds_w' type='number' value='"+String(_cfg.sdsWarmupSec)+"'></div><div><label>Campioni</label><input name='sds_n' type='number' value='"+String(_cfg.sdsSamples)+"'></div></div></div>";
  h += "<div class='cfg'><h2>AS3935</h2><div class='checks'><label class='check'><input type='checkbox' name='as_en'"+chk(_cfg.as3935Enabled)+">Abilitato</label><label class='check'><input type='checkbox' name='as_out'"+chk(_cfg.as3935Outdoor)+">Outdoor</label></div><div class='fields'><div><label>Addr hex</label><input name='as_a' value='"+String(_cfg.as3935Address,HEX)+"'></div><div><label>IRQ GPIO</label><input name='as_irq' type='number' value='"+String(_cfg.as3935IrqPin)+"'></div><div><label>Noise floor</label><input name='as_nf' type='number' value='"+String(_cfg.as3935NoiseFloor)+"'></div><div><label>Watchdog</label><input name='as_wd' type='number' value='"+String(_cfg.as3935Watchdog)+"'></div></div></div></div></div>";
  h += "<div id='p-adv' class='page'><div class='cfggrid'><div class='cfg'><h2>Tempi</h2><div class='fields'><div><label>Sensor interval s</label><input name='sens_s' type='number' value='"+String(_cfg.sensorIntervalSec)+"'></div><div><label>MQTT interval s</label><input name='tele_s' type='number' value='"+String(_cfg.telemetryIntervalSec)+"'></div></div></div><div class='cfg'><h2>I2C / GPIO</h2><div class='fields'><div><label>SDA</label><input name='sda' type='number' value='"+String(_cfg.i2cSda)+"'></div><div><label>SCL</label><input name='scl' type='number' value='"+String(_cfg.i2cScl)+"'></div><div><label>Relay GPIO</label><input name='rel_p' type='number' value='"+String(_cfg.relayPin)+"'></div><div><label>LED GPIO</label><input name='led_p' type='number' value='"+String(_cfg.statusLedPin)+"'></div></div></div><div class='cfg'><h2>Web</h2><div class='fields'><div><label>Utente</label><input name='webu' value='"+esc(_cfg.webUser)+"'></div><div><label>Password</label><input type='password' name='webp' value='"+esc(_cfg.webPassword)+"'></div></div></div></div></div>";
  h += F("<div class='sticky'><a class='btn bad' href='/factory' onclick='return confirm(\"Cancellare la configurazione?\")'>Factory reset</a><button class='btn ok' type='submit'>Salva e riavvia</button></div></form><script>document.querySelectorAll('.tab').forEach(b=>b.onclick=()=>{document.querySelectorAll('.tab').forEach(x=>x.classList.remove('active'));document.querySelectorAll('.page').forEach(x=>x.classList.remove('active'));b.classList.add('active');document.getElementById('p-'+b.dataset.p).classList.add('active')})</script>");
  h += pageEnd(); _server.send(200,"text/html; charset=utf-8",h);
}

uint8_t WebUi::parseHexByte(const String &value, uint8_t fallback) {
  if (!value.length()) return fallback;
  char *end=nullptr; long v=strtol(value.c_str(),&end,16);
  return (end && *end==0 && v>=0 && v<=255)?(uint8_t)v:fallback;
}

void WebUi::handleSave() {
  if (!auth()) return;
  auto arg=[this](const char*n,const String&d){return _server.hasArg(n)?_server.arg(n):d;};
  _cfg.deviceName=arg("dev",_cfg.deviceName); _cfg.wifiSsid=arg("ssid",_cfg.wifiSsid); _cfg.wifiPassword=arg("wpass",_cfg.wifiPassword); _cfg.wifiStaticIp=_server.hasArg("wstatic"); _cfg.wifiIp=arg("wip",_cfg.wifiIp); _cfg.wifiGateway=arg("wgw",_cfg.wifiGateway); _cfg.wifiSubnet=arg("wsub",_cfg.wifiSubnet); _cfg.wifiDns1=arg("wdns1",_cfg.wifiDns1); _cfg.timezone=arg("tz",_cfg.timezone);
  _cfg.mqttHost=arg("mqhost",_cfg.mqttHost); _cfg.mqttPort=(uint16_t)constrain(arg("mqport",String(_cfg.mqttPort)).toInt(),1,65535); _cfg.mqttUser=arg("mquser",_cfg.mqttUser); _cfg.mqttPassword=arg("mqpass",_cfg.mqttPassword); _cfg.mqttBaseTopic=arg("mqtopic",_cfg.mqttBaseTopic);
  _cfg.bmeEnabled=_server.hasArg("bme_en"); _cfg.bh1750Enabled=_server.hasArg("bh_en"); _cfg.bmeAddress=parseHexByte(arg("bme_a",String(_cfg.bmeAddress,HEX)),_cfg.bmeAddress); _cfg.bh1750Address=parseHexByte(arg("bh_a",String(_cfg.bh1750Address,HEX)),_cfg.bh1750Address); _cfg.bmeTemperatureOffsetC=arg("bme_to",String(_cfg.bmeTemperatureOffsetC)).toFloat(); _cfg.bmePressureOffsetHpa=arg("bme_po",String(_cfg.bmePressureOffsetHpa)).toFloat();
  _cfg.dhtEnabled=_server.hasArg("dht_en"); _cfg.uvEnabled=_server.hasArg("uv_en"); _cfg.dhtPin=(uint8_t)arg("dht_p",String(_cfg.dhtPin)).toInt(); _cfg.uvPin=(uint8_t)arg("uv_p",String(_cfg.uvPin)).toInt(); _cfg.uvMvPerIndex=arg("uv_k",String(_cfg.uvMvPerIndex)).toFloat(); _cfg.uvZeroMv=arg("uv_z",String(_cfg.uvZeroMv)).toFloat();
  _cfg.inaEnabled=_server.hasArg("ina_en"); _cfg.inaAddress=parseHexByte(arg("ina_a",String(_cfg.inaAddress,HEX)),_cfg.inaAddress); _cfg.inaBusVoltageOffsetV=arg("ina_vo",String(_cfg.inaBusVoltageOffsetV)).toFloat(); _cfg.inaCurrentOffsetMa=arg("ina_io",String(_cfg.inaCurrentOffsetMa)).toFloat();
  _cfg.sdsEnabled=_server.hasArg("sds_en"); _cfg.sdsRxPin=(uint8_t)arg("sds_rx",String(_cfg.sdsRxPin)).toInt(); _cfg.sdsTxPin=(uint8_t)arg("sds_tx",String(_cfg.sdsTxPin)).toInt(); _cfg.sdsCycleMinutes=(uint16_t)constrain(arg("sds_cm",String(_cfg.sdsCycleMinutes)).toInt(),1,1440); _cfg.sdsWarmupSec=(uint16_t)constrain(arg("sds_w",String(_cfg.sdsWarmupSec)).toInt(),15,180); _cfg.sdsSamples=(uint8_t)constrain(arg("sds_n",String(_cfg.sdsSamples)).toInt(),1,30);
  _cfg.as3935Enabled=_server.hasArg("as_en"); _cfg.as3935Outdoor=_server.hasArg("as_out"); _cfg.as3935Address=parseHexByte(arg("as_a",String(_cfg.as3935Address,HEX)),_cfg.as3935Address); _cfg.as3935IrqPin=(uint8_t)arg("as_irq",String(_cfg.as3935IrqPin)).toInt(); _cfg.as3935NoiseFloor=(uint8_t)constrain(arg("as_nf",String(_cfg.as3935NoiseFloor)).toInt(),0,7); _cfg.as3935Watchdog=(uint8_t)constrain(arg("as_wd",String(_cfg.as3935Watchdog)).toInt(),0,10);
  _cfg.sensorIntervalSec=max(2,arg("sens_s",String(_cfg.sensorIntervalSec)).toInt()); _cfg.telemetryIntervalSec=max(5,arg("tele_s",String(_cfg.telemetryIntervalSec)).toInt()); _cfg.i2cSda=(uint8_t)arg("sda",String(_cfg.i2cSda)).toInt(); _cfg.i2cScl=(uint8_t)arg("scl",String(_cfg.i2cScl)).toInt(); _cfg.relayPin=(uint8_t)arg("rel_p",String(_cfg.relayPin)).toInt(); _cfg.statusLedPin=(uint8_t)arg("led_p",String(_cfg.statusLedPin)).toInt(); _cfg.webUser=arg("webu",_cfg.webUser); _cfg.webPassword=arg("webp",_cfg.webPassword);
  _store.save(_cfg);
  _server.send(200,"text/html; charset=utf-8",pageStart("Salvata")+F("<div class='panel'><div class='head'>Configurazione salvata</div><div class='body'>Riavvio in corso...</div></div>")+pageEnd());
  delay(600); ESP.restart();
}

void WebUi::handleFactory() {
  if (!auth()) return;
  _store.clear(); _server.send(200,"text/plain","Configurazione cancellata. Riavvio..."); delay(500); ESP.restart();
}

void WebUi::setupOta() {
  _server.on("/update",HTTP_GET,[this](){if(!auth())return;String h=pageStart("OTA");h+=F("<div class='top'><div class='brand'><div class='title'>Firmware OTA</div><div class='sub'>Caricare firmware.bin prodotto da PlatformIO</div></div><a class='btn' href='/'>Dashboard</a></div><section class='panel'><div class='body'><form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='firmware' accept='.bin'><div class='tools' style='margin-top:8px'><button class='btn ok'>Carica firmware</button></div></form></div></section>");h+=pageEnd();_server.send(200,"text/html",h);});
  _server.on("/update",HTTP_POST,[this](){bool ok=!Update.hasError();_server.send(200,"text/plain",ok?"OK - rebooting":"UPDATE FAILED");if(ok){delay(500);ESP.restart();}},[this](){HTTPUpload &u=_server.upload();if(u.status==UPLOAD_FILE_START)Update.begin(UPDATE_SIZE_UNKNOWN);else if(u.status==UPLOAD_FILE_WRITE)Update.write(u.buf,u.currentSize);else if(u.status==UPLOAD_FILE_END)Update.end(true);});
}

void WebUi::begin() {
  _server.on("/",HTTP_GET,[this](){handleRoot();});
  _server.on("/api/status",HTTP_GET,[this](){handleApiStatus();});
  _server.on("/api/i2c",HTTP_GET,[this](){if(!auth())return;_server.send(200,"text/plain",_sensors.scanI2c());});
  _server.on("/api/relay/toggle",HTTP_POST,[this](){if(!auth())return;_sensors.toggleRelay();_server.send(200,"text/plain","OK");});
  _server.on("/api/sds/measure",HTTP_POST,[this](){if(!auth())return;_server.send(_sensors.requestSdsMeasurement()?200:409,"text/plain","OK");});
  _server.on("/api/sds/sleep",HTTP_POST,[this](){if(!auth())return;_server.send(_sensors.forceSdsSleep()?200:409,"text/plain","OK");});
  _server.on("/config",HTTP_GET,[this](){handleConfig();});
  _server.on("/save",HTTP_POST,[this](){handleSave();});
  _server.on("/factory",HTTP_GET,[this](){handleFactory();});
  setupOta(); _server.begin();
}

void WebUi::loop() { _server.handleClient(); }
