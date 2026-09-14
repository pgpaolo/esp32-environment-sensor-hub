#include "WebUi.h"
#include "BuildInfo.h"
#include "NesaConfigStore.h"
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
  o.replace("&", "&amp;");
  o.replace("\"", "&quot;");
  o.replace("<", "&lt;");
  o.replace(">", "&gt;");
  o.replace("'", "&#39;");
  return o;
}

String WebUi::chk(bool v) const { return v ? " checked" : ""; }
String WebUi::sel(bool v) const { return v ? " selected" : ""; }

String WebUi::pageStart(const String &title) const {
  String h;
  h.reserve(9000);
  h += "<!doctype html><html lang='it'><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>" + esc(title) + "</title>";
  h += F(R"CSS(<style>
:root{color-scheme:dark;--bg:#08111f;--panel:#0d1829;--panel2:#101d30;--line:#26384e;--text:#e8eef8;--muted:#8fa7c5;--ok:#30d99a;--warn:#f0b24a;--bad:#ff7070;--blue:#55aef6}
*{box-sizing:border-box}body{margin:0;background:linear-gradient(180deg,#08111f,#07101c);color:var(--text);font-family:system-ui,-apple-system,Segoe UI,Arial,sans-serif}main{max-width:1500px;margin:auto;padding:10px 12px 18px}.top{display:flex;gap:9px;align-items:center;justify-content:space-between;flex-wrap:wrap}.brand{flex:1;min-width:255px}.title{font-size:1.2rem;font-weight:850}.sub,.muted{color:var(--muted);font-size:.74rem}.tools{display:flex;gap:5px;align-items:center;flex-wrap:wrap}.pill{display:inline-flex;gap:5px;align-items:center;border:1px solid var(--line);background:var(--panel);padding:5px 8px;border-radius:99px;font-size:.71rem;font-weight:750;color:var(--muted)}.pill:before{content:'';width:7px;height:7px;border-radius:50%;background:#65758a}.pill.ok:before{background:var(--ok)}.pill.warn:before{background:var(--warn)}.pill.bad:before{background:var(--bad)}.btn{background:#163f63;color:#eef7ff;border:1px solid #2c5e83;border-radius:7px;padding:6px 9px;text-decoration:none;font:inherit;font-size:.78rem;font-weight:750;cursor:pointer}.btn.ok{background:#14583f;border-color:#267557}.btn.warn{background:#6c4717;border-color:#95651e}.btn.bad{background:#672d35;border-color:#8c3d48}.tabs{display:flex;gap:5px;margin-top:8px;padding:4px;border:1px solid var(--line);border-radius:10px;background:#0a1525;overflow:auto}.tab{border:0;background:transparent;color:var(--muted);padding:6px 11px;border-radius:7px;font-weight:800;white-space:nowrap;cursor:pointer}.tab.active{background:#16304a;color:#eef7ff}.page{display:none}.page.active{display:block}.panel{border:1px solid var(--line);border-radius:11px;background:var(--panel);overflow:hidden;margin-top:8px}.head{padding:7px 10px;border-bottom:1px solid var(--line);font-weight:780;display:flex;justify-content:space-between;gap:8px;align-items:center}.grid{display:grid;gap:6px;padding:7px}.g4{grid-template-columns:repeat(4,minmax(0,1fr))}.g3{grid-template-columns:repeat(3,minmax(0,1fr))}.g2{grid-template-columns:repeat(2,minmax(0,1fr))}.card{border:1px solid var(--line);border-radius:9px;background:linear-gradient(180deg,#101d30,#0e1a2b);overflow:hidden}.ct{padding:6px 8px;border-bottom:1px solid var(--line);font-weight:760;font-size:.85rem;display:flex;align-items:center;gap:6px}.ct:before{content:'';width:7px;height:7px;border-radius:50%;background:#65758a;flex:0 0 auto}.card.ok .ct:before{background:var(--ok)}.card.warn .ct:before{background:var(--warn)}.card.bad .ct:before{background:var(--bad)}.card.off{opacity:.68}.tag{margin-left:auto;font-size:.6rem;color:#9db7d4;border:1px solid #35516e;border-radius:99px;padding:2px 5px}.pinbtn{margin-left:auto;border:1px solid #3b607f;background:#112a40;color:#9fd3ff;border-radius:6px;padding:2px 6px;font-size:.61rem;font-weight:850;cursor:pointer}.body{padding:1px 8px 5px}.row{display:flex;justify-content:space-between;gap:8px;padding:4px 0;border-bottom:1px solid #1c2b3e;font-size:.78rem}.row:last-child{border:0}.name{color:#b5c8e1}.value{font-weight:750;text-align:right}.row:first-child .value{font-size:1.03rem}.foot{padding:4px 8px;background:#0a1525;color:var(--muted);font-size:.64rem;min-height:22px}.cfggrid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:7px;margin-top:8px}.cfg{border:1px solid var(--line);border-radius:9px;background:var(--panel);padding:7px 8px}.cfg h2{font-size:.87rem;margin:0 0 5px;padding-bottom:5px;border-bottom:1px solid #1c2b3e}.fields{display:grid;grid-template-columns:repeat(2,minmax(0,1fr));gap:3px 6px}.full{grid-column:1/-1}label{display:block;color:#c8d7ea;font-size:.7rem;margin:3px 0 2px}input,select,textarea{width:100%;padding:5px 6px;border:1px solid #38516d;border-radius:6px;background:#e9edf2;color:#0d1722;font:inherit;font-size:.76rem}input[type=checkbox]{width:auto}.checks{display:flex;gap:4px 10px;flex-wrap:wrap;margin-top:4px}.check{display:inline-flex;gap:5px;align-items:center;font-size:.73rem}.hint{font-size:.66rem;color:var(--muted);margin-top:4px;line-height:1.4}.sticky{position:sticky;bottom:0;display:flex;justify-content:flex-end;gap:5px;margin-top:8px;padding:7px;border:1px solid var(--line);border-radius:9px;background:#0a1525ee}.diag{font-size:.73rem;line-height:1.5}.mono{font-family:ui-monospace,Consolas,monospace;word-break:break-word}.big{font-size:1.12rem;font-weight:800}.modal{display:none;position:fixed;inset:0;background:#020812d9;z-index:50;align-items:center;justify-content:center;padding:18px}.modal.show{display:flex}.modalbox{width:min(560px,96vw);border:1px solid #36516f;border-radius:12px;background:#0d1829;box-shadow:0 18px 70px #000a}.modalhead{display:flex;justify-content:space-between;align-items:center;padding:9px 11px;border-bottom:1px solid var(--line);font-weight:850}.modalbody{padding:10px 12px;font-size:.8rem;line-height:1.65}.pinrow{display:flex;justify-content:space-between;gap:12px;border-bottom:1px solid #1c2b3e;padding:5px 0}.pinrow:last-child{border:0}.pinname{color:#a9bfd9}.pinval{font-weight:800;text-align:right}.xbtn{border:0;background:transparent;color:#dbe8f8;font-size:1.2rem;cursor:pointer}.sensor-toggle{padding:7px 8px;border:1px solid #2a4058;border-radius:8px;background:#0b1727}.sensor-toggle strong{font-size:.75rem}.sensor-toggle span{display:block;color:var(--muted);font-size:.63rem;margin-top:2px}
@media(max-width:1050px){.g4,.g3,.cfggrid{grid-template-columns:repeat(2,minmax(0,1fr))}}@media(max-width:680px){.g4,.g3,.g2,.cfggrid,.fields{grid-template-columns:1fr}.tools{width:100%}.btn,.pill{flex:1;justify-content:center}.sticky{position:static}}
</style>)CSS");
  h += "</head><body><main>";
  return h;
}

String WebUi::pageEnd() const { return F("</main></body></html>"); }

void WebUi::handleRoot() {
  if (!auth()) return;

  String h = pageStart(_cfg.deviceName);
  h.reserve(33000);
  h += "<div class='top'><div class='brand'><div class='title'>ESP32 Environment Sensor Hub</div><div class='sub'>v" + String(FW_VERSION) + " · ESP32 sempre attivo · sleep solo SDS011</div></div><div class='tools'><span id='wifi' class='pill'>Wi-Fi</span><span id='mq' class='pill'>MQTT</span><span id='sds' class='pill'>SDS</span><a class='btn' href='/config'>Configurazione</a><a class='btn' href='/update'>OTA</a></div></div>";
  h += F("<div class='tabs'><button class='tab active' data-p='sens'>Sensori</button><button class='tab' data-p='diag'>Diagnostica</button></div>");

  h += F(R"HTML(<div id='p-sens' class='page active'>
<section class='panel'><div class='head'><span>Ambiente</span><span id='stamp' class='muted'>-</span></div><div class='grid g4'>
<div id='c-bh' class='card'><div class='ct'>BH1750<button class='pinbtn' onclick="showPins('bh1750')">PIN</button></div><div class='body'><div class='row'><span class='name'>Luce</span><span id='bh' class='value'>-</span></div><div class='row'><span class='name'>I2C</span><span id='bha' class='value'>-</span></div></div><div id='bhf' class='foot'>-</div></div>
<div id='c-bme' class='card'><div class='ct'>BME280<button class='pinbtn' onclick="showPins('bme280')">PIN</button></div><div class='body'><div class='row'><span class='name'>Temperatura</span><span id='bt' class='value'>-</span></div><div class='row'><span class='name'>Umidita</span><span id='bhm' class='value'>-</span></div><div class='row'><span class='name'>Pressione</span><span id='bp' class='value'>-</span></div></div><div id='bmf' class='foot'>-</div></div>
<div id='c-dht' class='card'><div class='ct'>DHT11<button class='pinbtn' onclick="showPins('dht11')">PIN</button></div><div class='body'><div class='row'><span class='name'>Temperatura</span><span id='dt' class='value'>-</span></div><div class='row'><span class='name'>Umidita</span><span id='dh' class='value'>-</span></div><div class='row'><span class='name'>Dew point</span><span id='dd' class='value'>-</span></div></div><div id='df' class='foot'>-</div></div>
<div id='c-uv' class='card'><div class='ct'>UV analogico<button class='pinbtn' onclick="showPins('uv')">PIN</button></div><div class='body'><div class='row'><span class='name'>UV Index</span><span id='ui' class='value'>-</span></div><div class='row'><span class='name'>Tensione</span><span id='umv' class='value'>-</span></div><div class='row'><span class='name'>ADC raw</span><span id='ur' class='value'>-</span></div></div><div id='uf' class='foot'>-</div></div>
</div></section>
<section class='panel'><div class='head'><span>Sensori NESA</span><span class='muted'>TA-N + RSG1-N</span></div><div class='grid g2'>
<div id='c-ta' class='card'><div class='ct'>NESA TA-N <span class='tag'>MAX31865 · PT100 4 fili</span><button class='pinbtn' onclick="showPins('nesa_ta')">PIN</button></div><div class='body'><div class='row'><span class='name'>Temperatura</span><span id='tat' class='value'>-</span></div><div class='row'><span class='name'>Resistenza RTD</span><span id='tar' class='value'>-</span></div><div class='row'><span class='name'>Fault</span><span id='taf' class='value'>-</span></div></div><div id='tafoot' class='foot'>-</div></div>
<div id='c-rsg' class='card'><div class='ct'>NESA RSG1-N <span class='tag'>ADS1115 · A0-A1</span><button class='pinbtn' onclick="showPins('nesa_rsg1')">PIN</button></div><div class='body'><div class='row'><span class='name'>Radiazione</span><span id='rgw' class='value'>-</span></div><div class='row'><span class='name'>Ingresso</span><span id='rgmv' class='value'>-</span></div><div class='row'><span class='name'>ADC raw</span><span id='rgr' class='value'>-</span></div></div><div id='rgfoot' class='foot'>-</div></div>
</div></section>
<section class='panel'><div class='head'><span>Qualita aria e fulmini</span></div><div class='grid g2'>
<div id='c-sds' class='card'><div class='ct'>SDS011<button class='pinbtn' onclick="showPins('sds011')">PIN</button></div><div class='body'><div class='row'><span class='name'>PM2.5</span><span id='p25' class='value'>-</span></div><div class='row'><span class='name'>PM10</span><span id='p10' class='value'>-</span></div><div class='row'><span class='name'>Stato</span><span id='ss' class='value'>-</span></div><div class='row'><span class='name'>Timer</span><span id='sn' class='value'>-</span></div><div class='tools'><button class='btn ok' onclick='post("/api/sds/measure")'>Misura ora</button><button class='btn warn' onclick='post("/api/sds/sleep")'>Sleep SDS</button></div></div><div id='sf' class='foot'>-</div></div>
<div id='c-as' class='card'><div class='ct'>AS3935<button class='pinbtn' onclick="showPins('as3935')">PIN</button></div><div class='body'><div class='row'><span class='name'>Evento</span><span id='ae' class='value'>-</span></div><div class='row'><span class='name'>Distanza</span><span id='ad' class='value'>-</span></div><div class='row'><span class='name'>Energia</span><span id='aen' class='value'>-</span></div><div class='row'><span class='name'>Fulmini</span><span id='ac' class='value'>-</span></div></div><div id='af' class='foot'>-</div></div>
</div></section>
<section class='panel'><div class='head'><span>Alimentazione e sistema</span></div><div class='grid g3'>
<div id='c-ina' class='card'><div class='ct'>INA219<button class='pinbtn' onclick="showPins('ina219')">PIN</button></div><div class='body'><div class='row'><span class='name'>Bus</span><span id='iv' class='value'>-</span></div><div class='row'><span class='name'>Corrente</span><span id='ii' class='value'>-</span></div><div class='row'><span class='name'>Potenza</span><span id='ipw' class='value'>-</span></div></div><div id='inf' class='foot'>-</div></div>
<div class='card ok'><div class='ct'>Sistema</div><div class='body'><div class='row'><span class='name'>IP</span><span id='sip' class='value'>-</span></div><div class='row'><span class='name'>RSSI</span><span id='sr' class='value'>-</span></div><div class='row'><span class='name'>Heap</span><span id='sh' class='value'>-</span></div><div class='row'><span class='name'>Uptime</span><span id='su' class='value'>-</span></div></div><div id='sysf' class='foot'>-</div></div>
<div class='card'><div class='ct'>Relay / Power<button class='pinbtn' onclick="showPins('relay')">PIN</button></div><div class='body'><div class='row'><span class='name'>Relay</span><span id='rs' class='value'>-</span></div><button class='btn' onclick='post("/api/relay/toggle")'>Toggle</button></div><div class='foot'>Power management predisposto</div></div>
</div></section></div>
<div id='p-diag' class='page'><section class='panel'><div class='head'><span>Diagnostica</span><div class='tools'><button class='btn' onclick='showPins("all")'>Mappa pin</button><button class='btn' onclick='scan()'>Scansione I2C</button></div></div><div class='grid g3'>
<div class='card'><div class='ct'>Health</div><div class='body diag'><div class='big' id='hc'>-</div><div id='hl'>-</div></div></div>
<div class='card'><div class='ct'>ESP32</div><div class='body diag'><div>Chip: <b id='chip'>-</b></div><div>Reset: <b id='rr'>-</b></div><div>Heap min: <b id='mh'>-</b></div><div>Flash: <b id='fl'>-</b></div></div></div>
<div class='card'><div class='ct'>I2C</div><div class='body diag mono' id='scan'>premere Scansione I2C</div></div>
<div class='card'><div class='ct'>SDS011</div><div class='body diag'><div>Cicli OK/KO: <b id='scy'>-</b></div><div>Campioni: <b id='scp'>-</b></div><div>Errore: <b id='ser'>-</b></div></div></div>
<div class='card'><div class='ct'>NESA</div><div class='body diag'><div>TA-N errori: <b id='tae'>-</b></div><div>RSG1-N errori: <b id='rge'>-</b></div><div>ADS addr: <b id='rga'>-</b></div></div></div>
<div class='card'><div class='ct'>MQTT</div><div class='body diag'><div>Stato: <b id='md'>-</b></div><div>Reconnect: <b id='mr'>-</b></div></div></div>
</div></section></div>
<div id='pinmodal' class='modal' onclick='if(event.target===this)closePins()'><div class='modalbox'><div class='modalhead'><span id='pintitle'>Pin sensore</span><button class='xbtn' onclick='closePins()'>×</button></div><div id='pinbody' class='modalbody'></div></div></div>
<script>
const q=x=>document.getElementById(x),f=(v,d=1)=>v==null||Number.isNaN(Number(v))?'-':Number(v).toFixed(d),hx=v=>v==null||v==255?'-':'0x'+Number(v).toString(16).toUpperCase().padStart(2,'0');
let lastStatus=null;
function sec(s){s=Number(s||0);if(s>=3600)return Math.floor(s/3600)+'h '+Math.floor((s%3600)/60)+'m';if(s>=60)return Math.floor(s/60)+'m '+s%60+'s';return s+'s'}
function state(id,ok,en=true){let e=q(id);e.classList.remove('ok','warn','bad','off');if(!en){e.classList.add('off','warn')}else e.classList.add(ok?'ok':'bad')}
function pill(id,ok,text){let e=q(id);e.className='pill '+(ok?'ok':'bad');e.textContent=text}
async function post(u){await fetch(u,{method:'POST'});setTimeout(refresh,150)}
async function scan(){q('scan').textContent=await(await fetch('/api/i2c')).text()}
function pinLine(n,v){return `<div class='pinrow'><span class='pinname'>${n}</span><span class='pinval'>${v}</span></div>`}
function closePins(){q('pinmodal').classList.remove('show')}
function showPins(key){
  if(!lastStatus)return;
  const d=lastStatus,p=d.pins||{};
  const i2c=()=>pinLine('SDA','GPIO '+p.i2c_sda)+pinLine('SCL','GPIO '+p.i2c_scl);
  let title='Mappa pin',body='';
  if(key==='bh1750'){title='BH1750';body=i2c()+pinLine('Indirizzo I2C',hx(d.bh1750.detected_address||d.bh1750.configured_address));}
  else if(key==='bme280'){title='BME280';body=i2c()+pinLine('Indirizzo I2C',hx(d.bme280.detected_address||d.bme280.configured_address));}
  else if(key==='dht11'){title='DHT11';body=pinLine('DATA','GPIO '+p.dht_data);}
  else if(key==='uv'){title='UV analogico';body=pinLine('ADC','GPIO '+p.uv_adc)+pinLine('Nota','ADC1 - ingresso analogico');}
  else if(key==='ina219'){title='INA219';body=i2c()+pinLine('Indirizzo I2C',hx(d.ina219.detected_address||d.ina219.configured_address));}
  else if(key==='sds011'){title='SDS011';body=pinLine('ESP32 RX','GPIO '+p.sds_rx+' ← SDS TX')+pinLine('ESP32 TX','GPIO '+p.sds_tx+' → SDS RX')+pinLine('UART','9600 8N1');}
  else if(key==='as3935'){title='AS3935';body=i2c()+pinLine('IRQ','GPIO '+p.as3935_irq)+pinLine('Indirizzo I2C',hx(d.as3935.detected_address||d.as3935.configured_address));}
  else if(key==='nesa_ta'){title='NESA TA-N / MAX31865';body=pinLine('SPI SCK','GPIO '+p.spi_sck)+pinLine('SPI MISO','GPIO '+p.spi_miso)+pinLine('SPI MOSI','GPIO '+p.spi_mosi)+pinLine('MAX31865 CS','GPIO '+p.nesa_ta_cs);}
  else if(key==='nesa_rsg1'){title='NESA RSG1-N / ADS1115';body=i2c()+pinLine('Indirizzo ADS1115',hx(d.nesa_rsg1_n.detected_address||d.nesa_rsg1_n.configured_address))+pinLine('Ingresso','A0 - A1 differenziale');}
  else if(key==='relay'){title='Relay / Power';body=pinLine('Relay','GPIO '+p.relay)+pinLine('LED stato','GPIO '+p.status_led);}
  else {title='Mappa pin completa';body=pinLine('I2C SDA/SCL','GPIO '+p.i2c_sda+' / '+p.i2c_scl)+pinLine('DHT11 DATA','GPIO '+p.dht_data)+pinLine('UV ADC','GPIO '+p.uv_adc)+pinLine('SDS011 RX/TX','GPIO '+p.sds_rx+' / '+p.sds_tx)+pinLine('AS3935 IRQ','GPIO '+p.as3935_irq)+pinLine('NESA TA-N SPI','SCK '+p.spi_sck+' · MISO '+p.spi_miso+' · MOSI '+p.spi_mosi+' · CS '+p.nesa_ta_cs)+pinLine('Relay / LED','GPIO '+p.relay+' / '+p.status_led)+pinLine('BOOT config','GPIO '+p.config_button);}
  q('pintitle').textContent=title;q('pinbody').innerHTML=body;q('pinmodal').classList.add('show');
}
async function refresh(){try{
  let d=await(await fetch('/api/status')).json();lastStatus=d;q('stamp').textContent='Aggiornato '+new Date().toLocaleTimeString();pill('wifi',d.system.wifi,'Wi-Fi '+(d.system.wifi?d.system.rssi_dbm+' dBm':'offline'));pill('mq',d.system.mqtt,d.system.mqtt?'MQTT online':'MQTT offline');q('sds').className='pill '+(['sleeping','warming','sampling'].includes(d.sds011.state)?'ok':'warn');q('sds').textContent='SDS '+d.sds011.state;
  state('c-bh',d.bh1750.ok,d.bh1750.enabled);q('bh').textContent=d.bh1750.ok?f(d.bh1750.illuminance_lux)+' lx':'-';q('bha').textContent=hx(d.bh1750.detected_address);q('bhf').textContent=d.bh1750.enabled?(d.bh1750.last_error||'operativo'):'disabilitato';
  state('c-bme',d.bme280.ok,d.bme280.enabled);q('bt').textContent=d.bme280.ok?f(d.bme280.temperature_c)+' °C':'-';q('bhm').textContent=d.bme280.ok?f(d.bme280.humidity_pct)+' %':'-';q('bp').textContent=d.bme280.ok?f(d.bme280.pressure_hpa)+' hPa':'-';q('bmf').textContent=d.bme280.enabled?(d.bme280.last_error||('I2C '+hx(d.bme280.detected_address))):'disabilitato';
  state('c-dht',d.dht11.ok,d.dht11.enabled);q('dt').textContent=d.dht11.ok?f(d.dht11.temperature_c)+' °C':'-';q('dh').textContent=d.dht11.ok?f(d.dht11.humidity_pct)+' %':'-';q('dd').textContent=d.dht11.ok?f(d.dht11.dewpoint_c)+' °C':'-';q('df').textContent=d.dht11.enabled?(d.dht11.last_error||'GPIO '+d.dht11.pin):'disabilitato';
  state('c-uv',d.uv.ok,d.uv.enabled);q('ui').textContent=d.uv.ok?f(d.uv.uv_index,2):'-';q('umv').textContent=d.uv.millivolts+' mV';q('ur').textContent=d.uv.raw_adc;q('uf').textContent=d.uv.enabled?(d.uv.last_error||'ADC coerente'):'disabilitato';
  state('c-ta',d.nesa_ta_n.ok,d.nesa_ta_n.enabled);q('tat').textContent=d.nesa_ta_n.ok?f(d.nesa_ta_n.temperature_c,2)+' °C':'-';q('tar').textContent=d.nesa_ta_n.ok?f(d.nesa_ta_n.resistance_ohm,2)+' Ω':'-';q('taf').textContent='0x'+Number(d.nesa_ta_n.fault||0).toString(16).toUpperCase().padStart(2,'0');q('tafoot').textContent=d.nesa_ta_n.enabled?(d.nesa_ta_n.last_error||('SPI CS GPIO'+d.nesa_ta_n.cs_pin)):'disabilitato';
  state('c-rsg',d.nesa_rsg1_n.ok,d.nesa_rsg1_n.enabled);q('rgw').textContent=d.nesa_rsg1_n.ok?f(d.nesa_rsg1_n.radiation_wm2)+' W/m²':'-';q('rgmv').textContent=d.nesa_rsg1_n.ok?f(d.nesa_rsg1_n.millivolts,3)+' mV':'-';q('rgr').textContent=d.nesa_rsg1_n.raw_adc;q('rgfoot').textContent=d.nesa_rsg1_n.enabled?(d.nesa_rsg1_n.last_error||('I2C '+hx(d.nesa_rsg1_n.detected_address)+' · '+f(d.nesa_rsg1_n.sensitivity_uv_per_wm2,2)+' µV/Wm²')):'disabilitato';
  let sok=d.sds011.ok||['sleeping','warming','sampling'].includes(d.sds011.state);state('c-sds',sok,d.sds011.enabled);q('p25').textContent=d.sds011.pm25_ugm3==null?'-':f(d.sds011.pm25_ugm3)+' µg/m³';q('p10').textContent=d.sds011.pm10_ugm3==null?'-':f(d.sds011.pm10_ugm3)+' µg/m³';q('ss').textContent=d.sds011.state;q('sn').textContent=d.sds011.state==='sleeping'?sec(d.sds011.next_measurement_s):sec(d.sds011.stage_remaining_s);q('sf').textContent=d.sds011.enabled?(d.sds011.last_error||('cicli '+d.sds011.successful_cycles+'/'+d.sds011.failed_cycles)):'disabilitato';
  state('c-as',d.as3935.ok,d.as3935.enabled);q('ae').textContent=d.as3935.last_event;q('ad').textContent=d.as3935.distance_km==null?'-':d.as3935.distance_km+' km';q('aen').textContent=d.as3935.energy;q('ac').textContent=d.as3935.lightning_count;q('af').textContent=d.as3935.enabled?(d.as3935.last_error||('I2C '+hx(d.as3935.detected_address))):'disabilitato';
  state('c-ina',d.ina219.ok,d.ina219.enabled);q('iv').textContent=d.ina219.ok?f(d.ina219.bus_voltage_v,3)+' V':'-';q('ii').textContent=d.ina219.ok?f(d.ina219.current_ma)+' mA':'-';q('ipw').textContent=d.ina219.ok?f(d.ina219.power_mw)+' mW':'-';q('inf').textContent=d.ina219.enabled?(d.ina219.last_error||('I2C '+hx(d.ina219.detected_address))):'disabilitato';
  q('sip').textContent=d.system.ip;q('sr').textContent=d.system.rssi_dbm+' dBm';q('sh').textContent=Math.round(d.system.free_heap/1024)+' KB';q('su').textContent=sec(d.system.uptime_s);q('sysf').textContent='v'+d.system.firmware+' · boot '+d.system.boot_count;q('rs').textContent=d.relay.state?'ON':'OFF';
  let mods=[d.bh1750,d.bme280,d.dht11,d.ina219,d.uv,d.nesa_ta_n,d.nesa_rsg1_n,{enabled:d.sds011.enabled,ok:sok},d.as3935].filter(x=>x.enabled),good=mods.filter(x=>x.ok).length;q('hc').textContent=good+' / '+mods.length;q('hl').textContent=mods.length===0?'Nessun sensore abilitato':good===mods.length?'Tutti i sensori abilitati operativi':'Verificare i sensori in rosso';q('chip').textContent=d.system.chip_model+' r'+d.system.chip_revision;q('rr').textContent=d.system.reset_reason;q('mh').textContent=Math.round(d.system.min_free_heap/1024)+' KB';q('fl').textContent=(d.system.flash_size/1048576).toFixed(1)+' MB';q('scy').textContent=d.sds011.successful_cycles+' / '+d.sds011.failed_cycles;q('scp').textContent=d.sds011.samples_collected+' / '+d.sds011.target_samples;q('ser').textContent=d.sds011.last_error||'nessuno';q('tae').textContent=d.nesa_ta_n.failures;q('rge').textContent=d.nesa_rsg1_n.failures;q('rga').textContent=hx(d.nesa_rsg1_n.detected_address);q('md').textContent=d.system.mqtt?'online':'offline ('+d.system.mqtt_state+')';q('mr').textContent=d.system.mqtt_reconnects;
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
  sys["uptime_s"] = millis() / 1000UL;
  sys["free_heap"] = ESP.getFreeHeap();
  sys["min_free_heap"] = ESP.getMinFreeHeap();
  sys["boot_count"] = _data.bootCount;
  sys["firmware"] = FW_VERSION;
  sys["chip_model"] = ESP.getChipModel();
  sys["chip_revision"] = ESP.getChipRevision();
  sys["flash_size"] = ESP.getFlashChipSize();
  sys["reset_reason"] = resetReasonText(esp_reset_reason());

  JsonObject pins = doc["pins"].to<JsonObject>();
  pins["i2c_sda"] = _cfg.i2cSda;
  pins["i2c_scl"] = _cfg.i2cScl;
  pins["dht_data"] = _cfg.dhtPin;
  pins["uv_adc"] = _cfg.uvPin;
  pins["sds_rx"] = _cfg.sdsRxPin;
  pins["sds_tx"] = _cfg.sdsTxPin;
  pins["as3935_irq"] = _cfg.as3935IrqPin;
  pins["nesa_ta_cs"] = _cfg.nesaTaCsPin;
  pins["spi_sck"] = 18;
  pins["spi_miso"] = 19;
  pins["spi_mosi"] = 23;
  pins["relay"] = _cfg.relayPin;
  pins["status_led"] = _cfg.statusLedPin;
  pins["config_button"] = _cfg.configButtonPin;

  JsonObject relay = doc["relay"].to<JsonObject>();
  relay["enabled"] = _cfg.relayEnabled;
  relay["state"] = _data.relayState;

  JsonObject bh = doc["bh1750"].to<JsonObject>();
  bh["enabled"] = _cfg.bh1750Enabled;
  bh["ok"] = _data.bh1750Ok;
  bh["illuminance_lux"] = _data.bh1750Lux;
  bh["configured_address"] = _cfg.bh1750Address;
  bh["detected_address"] = _data.bh1750DetectedAddress;
  bh["failures"] = _data.bh1750Failures;
  bh["last_error"] = _data.bh1750LastError;

  JsonObject bme = doc["bme280"].to<JsonObject>();
  bme["enabled"] = _cfg.bmeEnabled;
  bme["ok"] = _data.bmeOk;
  bme["temperature_c"] = _data.bmeTempC;
  bme["humidity_pct"] = _data.bmeHumidity;
  bme["pressure_hpa"] = _data.bmePressureHpa;
  bme["dewpoint_c"] = _data.bmeDewPointC;
  bme["configured_address"] = _cfg.bmeAddress;
  bme["detected_address"] = _data.bmeDetectedAddress;
  bme["failures"] = _data.bmeFailures;
  bme["last_error"] = _data.bmeLastError;

  JsonObject dht = doc["dht11"].to<JsonObject>();
  dht["enabled"] = _cfg.dhtEnabled;
  dht["ok"] = _data.dhtOk;
  dht["pin"] = _cfg.dhtPin;
  dht["temperature_c"] = _data.dhtTempC;
  dht["humidity_pct"] = _data.dhtHumidity;
  dht["dewpoint_c"] = _data.dhtDewPointC;
  dht["failures"] = _data.dhtFailures;
  dht["last_error"] = _data.dhtLastError;

  JsonObject ina = doc["ina219"].to<JsonObject>();
  ina["enabled"] = _cfg.inaEnabled;
  ina["ok"] = _data.inaOk;
  ina["bus_voltage_v"] = _data.inaBusVoltageV;
  ina["current_ma"] = _data.inaCurrentMa;
  ina["power_mw"] = _data.inaPowerMw;
  ina["configured_address"] = _cfg.inaAddress;
  ina["detected_address"] = _data.inaDetectedAddress;
  ina["failures"] = _data.inaFailures;
  ina["last_error"] = _data.inaLastError;

  JsonObject uv = doc["uv"].to<JsonObject>();
  uv["enabled"] = _cfg.uvEnabled;
  uv["ok"] = _data.uvOk;
  uv["raw_adc"] = _data.uvRawAdc;
  uv["millivolts"] = _data.uvMilliVolts;
  uv["uv_index"] = _data.uvIndex;
  uv["failures"] = _data.uvFailures;
  uv["last_error"] = _data.uvLastError;

  JsonObject ta = doc["nesa_ta_n"].to<JsonObject>();
  ta["enabled"] = _cfg.nesaTaEnabled;
  ta["ok"] = _data.nesaTaOk;
  ta["temperature_c"] = _data.nesaTaTemperatureC;
  ta["resistance_ohm"] = _data.nesaTaResistanceOhm;
  ta["fault"] = _data.nesaTaFault;
  ta["failures"] = _data.nesaTaFailures;
  ta["last_error"] = _data.nesaTaLastError;
  ta["cs_pin"] = _cfg.nesaTaCsPin;

  JsonObject rsg = doc["nesa_rsg1_n"].to<JsonObject>();
  rsg["enabled"] = _cfg.nesaRsg1Enabled;
  rsg["ok"] = _data.nesaRsg1Ok;
  rsg["raw_adc"] = _data.nesaRsg1Raw;
  rsg["millivolts"] = _data.nesaRsg1MilliVolts;
  rsg["radiation_wm2"] = _data.nesaRsg1RadiationWm2;
  rsg["configured_address"] = _cfg.nesaRsg1AdsAddress;
  rsg["detected_address"] = _data.nesaRsg1DetectedAddress;
  rsg["sensitivity_uv_per_wm2"] = _cfg.nesaRsg1SensitivityUvPerWm2;
  rsg["failures"] = _data.nesaRsg1Failures;
  rsg["last_error"] = _data.nesaRsg1LastError;

  JsonObject sds = doc["sds011"].to<JsonObject>();
  sds["enabled"] = _cfg.sdsEnabled;
  sds["ok"] = _data.sdsOk;
  sds["state"] = _data.sdsState;
  if (!isnan(_data.pm25)) sds["pm25_ugm3"] = _data.pm25;
  if (!isnan(_data.pm10)) sds["pm10_ugm3"] = _data.pm10;
  sds["next_measurement_s"] = _data.sdsNextInSec;
  sds["stage_remaining_s"] = _data.sdsStageRemainingSec;
  sds["samples_collected"] = _data.sdsSamplesCollected;
  sds["target_samples"] = _cfg.sdsSamples;
  sds["successful_cycles"] = _data.sdsSuccessfulCycles;
  sds["failed_cycles"] = _data.sdsFailedCycles;
  sds["last_error"] = _data.sdsLastError;

  JsonObject as = doc["as3935"].to<JsonObject>();
  as["enabled"] = _cfg.as3935Enabled;
  as["ok"] = _data.as3935Ok;
  as["last_event"] = _data.as3935LastEvent;
  if (_data.as3935DistanceKm >= 0) as["distance_km"] = _data.as3935DistanceKm;
  as["energy"] = _data.as3935Energy;
  as["lightning_count"] = _data.as3935EventCount;
  as["configured_address"] = _cfg.as3935Address;
  as["detected_address"] = _data.as3935DetectedAddress;
  as["last_error"] = _data.as3935LastError;

  String out;
  serializeJson(doc, out);
  _server.send(200, "application/json", out);
}

uint8_t WebUi::parseHexByte(const String &value, uint8_t fallback) {
  if (!value.length()) return fallback;
  char *end = nullptr;
  long v = strtol(value.c_str(), &end, 16);
  return (end && *end == 0 && v >= 0 && v <= 255) ? (uint8_t)v : fallback;
}

void WebUi::handleConfig() {
  if (!auth()) return;

  String h = pageStart("Configurazione");
  h.reserve(30000);
  h += "<div class='top'><div class='brand'><div class='title'>Configurazione</div><div class='sub'>v" + String(FW_VERSION) + " · AP manutenzione 192.168.4.1 · autenticazione Web attiva</div></div><div class='tools'><a class='btn' href='/'>Dashboard</a><a class='btn' href='/update'>OTA</a></div></div>";
  h += F("<div class='tabs'><button type='button' class='tab active' data-p='net'>Rete & MQTT</button><button type='button' class='tab' data-p='sen'>Sensori</button><button type='button' class='tab' data-p='sys'>Sistema</button></div><form method='POST' action='/save'>");

  h += "<div id='p-net' class='page active'><div class='cfggrid'>";
  h += "<div class='cfg'><h2>Wi-Fi</h2><div class='fields'><div><label>SSID</label><input name='ssid' value='" + esc(_cfg.wifiSsid) + "'></div><div><label>Password Wi-Fi</label><input type='password' name='wpass' value='" + esc(_cfg.wifiPassword) + "'></div><div><label>Nome dispositivo</label><input name='dev' value='" + esc(_cfg.deviceName) + "'></div><div><label>Timezone</label><input name='tz' value='" + esc(_cfg.timezone) + "'></div></div><div class='checks'><label class='check'><input type='checkbox' name='wstatic'" + chk(_cfg.wifiStaticIp) + ">IPv4 statico</label></div><div class='hint'>Se la STA non si collega, viene avviato l'AP di manutenzione su 192.168.4.1.</div></div>";
  h += "<div class='cfg'><h2>IPv4</h2><div class='fields'><div><label>IP</label><input name='wip' value='" + esc(_cfg.wifiIp) + "'></div><div><label>Gateway</label><input name='wgw' value='" + esc(_cfg.wifiGateway) + "'></div><div><label>Subnet</label><input name='wsub' value='" + esc(_cfg.wifiSubnet) + "'></div><div><label>DNS 1</label><input name='wdns1' value='" + esc(_cfg.wifiDns1) + "'></div><div><label>DNS 2</label><input name='wdns2' value='" + esc(_cfg.wifiDns2) + "'></div></div></div>";
  h += "<div class='cfg'><h2>MQTT</h2><div class='fields'><div><label>Host</label><input name='mqhost' value='" + esc(_cfg.mqttHost) + "'></div><div><label>Porta</label><input name='mqport' type='number' value='" + String(_cfg.mqttPort) + "'></div><div><label>Utente</label><input name='mquser' value='" + esc(_cfg.mqttUser) + "'></div><div><label>Password</label><input type='password' name='mqpass' value='" + esc(_cfg.mqttPassword) + "'></div><div class='full'><label>Base topic</label><input name='mqtopic' value='" + esc(_cfg.mqttBaseTopic) + "'></div></div></div></div></div>";

  h += "<div id='p-sen' class='page'><div class='cfggrid'>";
  h += "<div class='cfg full'><h2>Sensori attivi</h2><div class='grid g3' style='padding:0'>";
  h += "<label class='sensor-toggle'><input type='checkbox' name='bh_en'" + chk(_cfg.bh1750Enabled) + "> <strong>BH1750</strong><span>I2C · luminosita</span></label>";
  h += "<label class='sensor-toggle'><input type='checkbox' name='bme_en'" + chk(_cfg.bmeEnabled) + "> <strong>BME280</strong><span>I2C · T/UR/P</span></label>";
  h += "<label class='sensor-toggle'><input type='checkbox' name='dht_en'" + chk(_cfg.dhtEnabled) + "> <strong>DHT11</strong><span>GPIO · T/UR</span></label>";
  h += "<label class='sensor-toggle'><input type='checkbox' name='uv_en'" + chk(_cfg.uvEnabled) + "> <strong>UV analogico</strong><span>ADC1</span></label>";
  h += "<label class='sensor-toggle'><input type='checkbox' name='ina_en'" + chk(_cfg.inaEnabled) + "> <strong>INA219</strong><span>I2C · V/A/W</span></label>";
  h += "<label class='sensor-toggle'><input type='checkbox' name='sds_en'" + chk(_cfg.sdsEnabled) + "> <strong>SDS011</strong><span>UART · PM2.5/PM10</span></label>";
  h += "<label class='sensor-toggle'><input type='checkbox' name='as_en'" + chk(_cfg.as3935Enabled) + "> <strong>AS3935</strong><span>I2C + IRQ</span></label>";
  h += "<label class='sensor-toggle'><input type='checkbox' name='nta_en'" + chk(_cfg.nesaTaEnabled) + "> <strong>NESA TA-N</strong><span>MAX31865 · SPI</span></label>";
  h += "<label class='sensor-toggle'><input type='checkbox' name='nrg_en'" + chk(_cfg.nesaRsg1Enabled) + "> <strong>NESA RSG1-N</strong><span>ADS1115 · I2C</span></label>";
  h += "</div><div class='hint'>Togliere la spunta disabilita completamente il sensore al riavvio. I sensori disabilitati non concorrono allo stato Health.</div></div>";

  h += "<div class='cfg'><h2>BME280 / BH1750</h2><div class='fields'><div><label>BME addr hex</label><input name='bme_a' value='" + String(_cfg.bmeAddress, HEX) + "'></div><div><label>BH addr hex</label><input name='bh_a' value='" + String(_cfg.bh1750Address, HEX) + "'></div><div><label>BME temp offset °C</label><input name='bme_to' value='" + String(_cfg.bmeTemperatureOffsetC, 2) + "'></div><div><label>BME press offset hPa</label><input name='bme_po' value='" + String(_cfg.bmePressureOffsetHpa, 2) + "'></div><div><label>BME UR offset %</label><input name='bme_ho' value='" + String(_cfg.bmeHumidityOffsetPct, 2) + "'></div><div><label>BH offset lux</label><input name='bh_off' value='" + String(_cfg.bh1750OffsetLux, 1) + "'></div></div><div class='hint'>Pin condivisi: SDA GPIO" + String(_cfg.i2cSda) + " · SCL GPIO" + String(_cfg.i2cScl) + ".</div></div>";
  h += "<div class='cfg'><h2>DHT11 / UV</h2><div class='fields'><div><label>DHT GPIO</label><input name='dht_p' type='number' value='" + String(_cfg.dhtPin) + "'></div><div><label>UV GPIO ADC</label><input name='uv_p' type='number' value='" + String(_cfg.uvPin) + "'></div><div><label>DHT temp offset °C</label><input name='dht_to' value='" + String(_cfg.dhtTemperatureOffsetC, 2) + "'></div><div><label>DHT UR offset %</label><input name='dht_ho' value='" + String(_cfg.dhtHumidityOffsetPct, 2) + "'></div><div><label>UV zero mV</label><input name='uv_z' value='" + String(_cfg.uvZeroMv, 2) + "'></div><div><label>UV mV/UVI</label><input name='uv_k' value='" + String(_cfg.uvMvPerIndex, 2) + "'></div><div><label>UV max index</label><input name='uv_max' value='" + String(_cfg.uvMaxIndex, 1) + "'></div></div></div>";
  h += "<div class='cfg'><h2>INA219</h2><div class='fields'><div><label>Indirizzo hex</label><input name='ina_a' value='" + String(_cfg.inaAddress, HEX) + "'></div><div><label>Offset bus V</label><input name='ina_vo' value='" + String(_cfg.inaBusVoltageOffsetV, 3) + "'></div><div><label>Offset corrente mA</label><input name='ina_io' value='" + String(_cfg.inaCurrentOffsetMa, 2) + "'></div></div><div class='hint'>I2C SDA GPIO" + String(_cfg.i2cSda) + " · SCL GPIO" + String(_cfg.i2cScl) + ".</div></div>";
  h += "<div class='cfg'><h2>SDS011</h2><div class='fields'><div><label>ESP RX GPIO</label><input name='sds_rx' type='number' value='" + String(_cfg.sdsRxPin) + "'></div><div><label>ESP TX GPIO</label><input name='sds_tx' type='number' value='" + String(_cfg.sdsTxPin) + "'></div><div><label>Ciclo min</label><input name='sds_cm' type='number' value='" + String(_cfg.sdsCycleMinutes) + "'></div><div><label>Warm-up s</label><input name='sds_w' type='number' value='" + String(_cfg.sdsWarmupSec) + "'></div><div><label>Campioni</label><input name='sds_n' type='number' value='" + String(_cfg.sdsSamples) + "'></div><div><label>Gap campioni ms</label><input name='sds_g' type='number' value='" + String(_cfg.sdsSampleGapMs) + "'></div><div><label>Max awake s</label><input name='sds_ma' type='number' value='" + String(_cfg.sdsMaxAwakeSec) + "'></div></div></div>";
  h += "<div class='cfg'><h2>AS3935</h2><div class='fields'><div><label>Indirizzo hex</label><input name='as_a' value='" + String(_cfg.as3935Address, HEX) + "'></div><div><label>IRQ GPIO</label><input name='as_irq' type='number' value='" + String(_cfg.as3935IrqPin) + "'></div><div><label>Noise floor 0-7</label><input name='as_nf' type='number' value='" + String(_cfg.as3935NoiseFloor) + "'></div><div><label>Watchdog 0-10</label><input name='as_wd' type='number' value='" + String(_cfg.as3935Watchdog) + "'></div><div><label>Spike 0-15</label><input name='as_sp' type='number' value='" + String(_cfg.as3935SpikeRejection) + "'></div><div><label>Threshold 1/5/9/16</label><input name='as_lt' type='number' value='" + String(_cfg.as3935LightningThreshold) + "'></div></div><div class='checks'><label class='check'><input type='checkbox' name='as_out'" + chk(_cfg.as3935Outdoor) + ">Outdoor</label><label class='check'><input type='checkbox' name='as_md'" + chk(_cfg.as3935MaskDisturber) + ">Mask disturber</label></div></div>";
  h += "<div class='cfg'><h2>NESA TA-N</h2><div class='fields'><div><label>MAX31865 CS</label><input name='nta_cs' type='number' value='" + String(_cfg.nesaTaCsPin) + "'></div><div><label>RTD nominale Ω</label><input name='nta_rtd' value='" + String(_cfg.nesaTaRtdNominalOhm, 2) + "'></div><div><label>RREF Ω</label><input name='nta_ref' value='" + String(_cfg.nesaTaRefResistorOhm, 2) + "'></div><div><label>Offset °C</label><input name='nta_off' value='" + String(_cfg.nesaTaTemperatureOffsetC, 3) + "'></div></div><div class='hint'>SPI VSPI: SCK GPIO18 · MISO GPIO19 · MOSI GPIO23.</div></div>";
  h += "<div class='cfg'><h2>NESA RSG1-N</h2><div class='fields'><div><label>ADS1115 addr hex</label><input name='nrg_a' value='" + String(_cfg.nesaRsg1AdsAddress, HEX) + "'></div><div><label>Sensibilita µV/Wm²</label><input name='nrg_s' value='" + String(_cfg.nesaRsg1SensitivityUvPerWm2, 4) + "'></div><div><label>Offset µV</label><input name='nrg_off' value='" + String(_cfg.nesaRsg1OffsetUv, 3) + "'></div><div><label>Max W/m²</label><input name='nrg_max' value='" + String(_cfg.nesaRsg1MaxWm2, 0) + "'></div></div><div class='checks'><label class='check'><input type='checkbox' name='nrg_cl'" + chk(_cfg.nesaRsg1ClampNegative) + ">Clamp negativo</label></div><div class='hint'>ADS1115 differenziale A0-A1 sul bus I2C condiviso.</div></div></div></div>";

  h += "<div id='p-sys' class='page'><div class='cfggrid'>";
  h += "<div class='cfg'><h2>Web / Sicurezza</h2><div class='fields'><div><label>Web user</label><input name='webu' value='" + esc(_cfg.webUser) + "'></div><div><label>Web password</label><input type='password' name='webp' value='" + esc(_cfg.webPassword) + "'></div></div><div class='hint'>Default/factory: <b>admin / admin</b>. Vale anche sull'AP di manutenzione http://192.168.4.1/.</div></div>";
  h += "<div class='cfg'><h2>Bus / GPIO</h2><div class='fields'><div><label>I2C SDA</label><input name='sda' type='number' value='" + String(_cfg.i2cSda) + "'></div><div><label>I2C SCL</label><input name='scl' type='number' value='" + String(_cfg.i2cScl) + "'></div><div><label>Relay GPIO</label><input name='rel_p' type='number' value='" + String(_cfg.relayPin) + "'></div><div><label>LED GPIO</label><input name='led_p' type='number' value='" + String(_cfg.statusLedPin) + "'></div><div><label>BOOT/config GPIO</label><input name='cfgbtn' type='number' value='" + String(_cfg.configButtonPin) + "'></div></div><div class='checks'><label class='check'><input type='checkbox' name='rel_en'" + chk(_cfg.relayEnabled) + ">Relay abilitato</label><label class='check'><input type='checkbox' name='rel_inv'" + chk(_cfg.relayInverted) + ">Relay invertito</label><label class='check'><input type='checkbox' name='led_en'" + chk(_cfg.statusLedEnabled) + ">LED abilitato</label><label class='check'><input type='checkbox' name='led_inv'" + chk(_cfg.statusLedInverted) + ">LED invertito</label></div></div>";
  h += "<div class='cfg'><h2>Intervalli</h2><div class='fields'><div><label>Lettura sensori s</label><input name='sens_s' type='number' value='" + String(_cfg.sensorIntervalSec) + "'></div><div><label>Telemetria MQTT s</label><input name='tele_s' type='number' value='" + String(_cfg.telemetryIntervalSec) + "'></div></div></div></div></div>";

  h += F("<div class='sticky'><a class='btn bad' href='/factory' onclick='return confirm(\"Cancellare tutte le configurazioni e ripristinare i default?\")'>Factory reset</a><button class='btn ok' type='submit'>Salva e riavvia</button></div></form><script>document.querySelectorAll('.tab').forEach(b=>b.onclick=()=>{document.querySelectorAll('.tab').forEach(x=>x.classList.remove('active'));document.querySelectorAll('.page').forEach(x=>x.classList.remove('active'));b.classList.add('active');document.getElementById('p-'+b.dataset.p).classList.add('active')})</script>");
  h += pageEnd();
  _server.send(200, "text/html; charset=utf-8", h);
}

void WebUi::handleSave() {
  if (!auth()) return;
  auto a = [this](const char *n, const String &d) { return _server.hasArg(n) ? _server.arg(n) : d; };

  _cfg.deviceName = a("dev", _cfg.deviceName);
  _cfg.wifiSsid = a("ssid", _cfg.wifiSsid);
  _cfg.wifiPassword = a("wpass", _cfg.wifiPassword);
  _cfg.wifiStaticIp = _server.hasArg("wstatic");
  _cfg.wifiIp = a("wip", _cfg.wifiIp);
  _cfg.wifiGateway = a("wgw", _cfg.wifiGateway);
  _cfg.wifiSubnet = a("wsub", _cfg.wifiSubnet);
  _cfg.wifiDns1 = a("wdns1", _cfg.wifiDns1);
  _cfg.wifiDns2 = a("wdns2", _cfg.wifiDns2);
  _cfg.timezone = a("tz", _cfg.timezone);

  _cfg.mqttHost = a("mqhost", _cfg.mqttHost);
  _cfg.mqttPort = (uint16_t)constrain(a("mqport", String(_cfg.mqttPort)).toInt(), 1L, 65535L);
  _cfg.mqttUser = a("mquser", _cfg.mqttUser);
  _cfg.mqttPassword = a("mqpass", _cfg.mqttPassword);
  _cfg.mqttBaseTopic = a("mqtopic", _cfg.mqttBaseTopic);

  _cfg.bh1750Enabled = _server.hasArg("bh_en");
  _cfg.bmeEnabled = _server.hasArg("bme_en");
  _cfg.dhtEnabled = _server.hasArg("dht_en");
  _cfg.uvEnabled = _server.hasArg("uv_en");
  _cfg.inaEnabled = _server.hasArg("ina_en");
  _cfg.sdsEnabled = _server.hasArg("sds_en");
  _cfg.as3935Enabled = _server.hasArg("as_en");
  _cfg.nesaTaEnabled = _server.hasArg("nta_en");
  _cfg.nesaRsg1Enabled = _server.hasArg("nrg_en");

  _cfg.bmeAddress = parseHexByte(a("bme_a", String(_cfg.bmeAddress, HEX)), _cfg.bmeAddress);
  _cfg.bh1750Address = parseHexByte(a("bh_a", String(_cfg.bh1750Address, HEX)), _cfg.bh1750Address);
  _cfg.bmeTemperatureOffsetC = a("bme_to", String(_cfg.bmeTemperatureOffsetC)).toFloat();
  _cfg.bmePressureOffsetHpa = a("bme_po", String(_cfg.bmePressureOffsetHpa)).toFloat();
  _cfg.bmeHumidityOffsetPct = a("bme_ho", String(_cfg.bmeHumidityOffsetPct)).toFloat();
  _cfg.bh1750OffsetLux = a("bh_off", String(_cfg.bh1750OffsetLux)).toFloat();

  _cfg.dhtPin = (uint8_t)a("dht_p", String(_cfg.dhtPin)).toInt();
  _cfg.dhtTemperatureOffsetC = a("dht_to", String(_cfg.dhtTemperatureOffsetC)).toFloat();
  _cfg.dhtHumidityOffsetPct = a("dht_ho", String(_cfg.dhtHumidityOffsetPct)).toFloat();
  _cfg.uvPin = (uint8_t)a("uv_p", String(_cfg.uvPin)).toInt();
  _cfg.uvZeroMv = a("uv_z", String(_cfg.uvZeroMv)).toFloat();
  _cfg.uvMvPerIndex = a("uv_k", String(_cfg.uvMvPerIndex)).toFloat();
  _cfg.uvMaxIndex = a("uv_max", String(_cfg.uvMaxIndex)).toFloat();

  _cfg.inaAddress = parseHexByte(a("ina_a", String(_cfg.inaAddress, HEX)), _cfg.inaAddress);
  _cfg.inaBusVoltageOffsetV = a("ina_vo", String(_cfg.inaBusVoltageOffsetV)).toFloat();
  _cfg.inaCurrentOffsetMa = a("ina_io", String(_cfg.inaCurrentOffsetMa)).toFloat();

  _cfg.sdsRxPin = (uint8_t)a("sds_rx", String(_cfg.sdsRxPin)).toInt();
  _cfg.sdsTxPin = (uint8_t)a("sds_tx", String(_cfg.sdsTxPin)).toInt();
  _cfg.sdsCycleMinutes = (uint16_t)constrain(a("sds_cm", String(_cfg.sdsCycleMinutes)).toInt(), 1L, 1440L);
  _cfg.sdsWarmupSec = (uint16_t)constrain(a("sds_w", String(_cfg.sdsWarmupSec)).toInt(), 15L, 180L);
  _cfg.sdsSamples = (uint8_t)constrain(a("sds_n", String(_cfg.sdsSamples)).toInt(), 1L, 30L);
  _cfg.sdsSampleGapMs = (uint16_t)constrain(a("sds_g", String(_cfg.sdsSampleGapMs)).toInt(), 250L, 10000L);
  _cfg.sdsMaxAwakeSec = (uint16_t)constrain(a("sds_ma", String(_cfg.sdsMaxAwakeSec)).toInt(), 30L, 600L);

  _cfg.as3935Address = parseHexByte(a("as_a", String(_cfg.as3935Address, HEX)), _cfg.as3935Address);
  _cfg.as3935IrqPin = (uint8_t)a("as_irq", String(_cfg.as3935IrqPin)).toInt();
  _cfg.as3935Outdoor = _server.hasArg("as_out");
  _cfg.as3935NoiseFloor = (uint8_t)constrain(a("as_nf", String(_cfg.as3935NoiseFloor)).toInt(), 0L, 7L);
  _cfg.as3935Watchdog = (uint8_t)constrain(a("as_wd", String(_cfg.as3935Watchdog)).toInt(), 0L, 10L);
  _cfg.as3935SpikeRejection = (uint8_t)constrain(a("as_sp", String(_cfg.as3935SpikeRejection)).toInt(), 0L, 15L);
  _cfg.as3935LightningThreshold = (uint8_t)a("as_lt", String(_cfg.as3935LightningThreshold)).toInt();
  _cfg.as3935MaskDisturber = _server.hasArg("as_md");

  _cfg.nesaTaCsPin = (uint8_t)a("nta_cs", String(_cfg.nesaTaCsPin)).toInt();
  _cfg.nesaTaRtdNominalOhm = a("nta_rtd", String(_cfg.nesaTaRtdNominalOhm)).toFloat();
  _cfg.nesaTaRefResistorOhm = a("nta_ref", String(_cfg.nesaTaRefResistorOhm)).toFloat();
  _cfg.nesaTaTemperatureOffsetC = a("nta_off", String(_cfg.nesaTaTemperatureOffsetC)).toFloat();

  _cfg.nesaRsg1AdsAddress = parseHexByte(a("nrg_a", String(_cfg.nesaRsg1AdsAddress, HEX)), _cfg.nesaRsg1AdsAddress);
  _cfg.nesaRsg1SensitivityUvPerWm2 = a("nrg_s", String(_cfg.nesaRsg1SensitivityUvPerWm2)).toFloat();
  _cfg.nesaRsg1OffsetUv = a("nrg_off", String(_cfg.nesaRsg1OffsetUv)).toFloat();
  _cfg.nesaRsg1MaxWm2 = a("nrg_max", String(_cfg.nesaRsg1MaxWm2)).toFloat();
  _cfg.nesaRsg1ClampNegative = _server.hasArg("nrg_cl");

  long sens = a("sens_s", String(_cfg.sensorIntervalSec)).toInt();
  if (sens < 2) sens = 2;
  _cfg.sensorIntervalSec = (uint32_t)sens;
  long tele = a("tele_s", String(_cfg.telemetryIntervalSec)).toInt();
  if (tele < 5) tele = 5;
  _cfg.telemetryIntervalSec = (uint32_t)tele;

  _cfg.i2cSda = (uint8_t)a("sda", String(_cfg.i2cSda)).toInt();
  _cfg.i2cScl = (uint8_t)a("scl", String(_cfg.i2cScl)).toInt();
  _cfg.relayEnabled = _server.hasArg("rel_en");
  _cfg.relayPin = (uint8_t)a("rel_p", String(_cfg.relayPin)).toInt();
  _cfg.relayInverted = _server.hasArg("rel_inv");
  _cfg.statusLedEnabled = _server.hasArg("led_en");
  _cfg.statusLedPin = (uint8_t)a("led_p", String(_cfg.statusLedPin)).toInt();
  _cfg.statusLedInverted = _server.hasArg("led_inv");
  _cfg.configButtonPin = (uint8_t)a("cfgbtn", String(_cfg.configButtonPin)).toInt();

  _cfg.webUser = a("webu", _cfg.webUser);
  _cfg.webPassword = a("webp", _cfg.webPassword);
  if (_cfg.webUser.isEmpty()) _cfg.webUser = "admin";
  if (_cfg.webPassword.isEmpty()) _cfg.webPassword = "admin";

  _store.save(_cfg);
  saveNesaConfig(_cfg);

  _server.send(200, "text/html; charset=utf-8", pageStart("Salvata") + F("<section class='panel'><div class='head'>Configurazione salvata</div><div class='body'>Riavvio in corso...</div></section>") + pageEnd());
  delay(600);
  ESP.restart();
}

void WebUi::handleFactory() {
  if (!auth()) return;
  _store.clear();
  clearNesaConfig();
  _server.send(200, "text/plain", "Configurazione cancellata. Al riavvio Web user/password tornano admin/admin.");
  delay(500);
  ESP.restart();
}

void WebUi::setupOta() {
  _server.on("/update", HTTP_GET, [this]() {
    if (!auth()) return;
    String h = pageStart("OTA");
    h += F("<div class='top'><div class='brand'><div class='title'>Firmware OTA</div><div class='sub'>Caricare firmware.bin prodotto da PlatformIO</div></div><a class='btn' href='/'>Dashboard</a></div><section class='panel'><div class='body'><form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='firmware' accept='.bin'><div class='tools' style='margin-top:8px'><button class='btn ok'>Carica firmware</button></div></form></div></section>");
    h += pageEnd();
    _server.send(200, "text/html", h);
  });

  _server.on("/update", HTTP_POST,
    [this]() {
      bool ok = !Update.hasError();
      _server.send(200, "text/plain", ok ? "OK - rebooting" : "UPDATE FAILED");
      if (ok) {
        delay(500);
        ESP.restart();
      }
    },
    [this]() {
      HTTPUpload &u = _server.upload();
      if (u.status == UPLOAD_FILE_START) Update.begin(UPDATE_SIZE_UNKNOWN);
      else if (u.status == UPLOAD_FILE_WRITE) Update.write(u.buf, u.currentSize);
      else if (u.status == UPLOAD_FILE_END) Update.end(true);
    });
}

void WebUi::begin() {
  _server.on("/", HTTP_GET, [this]() { handleRoot(); });
  _server.on("/api/status", HTTP_GET, [this]() { handleApiStatus(); });
  _server.on("/api/i2c", HTTP_GET, [this]() {
    if (!auth()) return;
    _server.send(200, "text/plain", _sensors.scanI2c());
  });
  _server.on("/api/relay/toggle", HTTP_POST, [this]() {
    if (!auth()) return;
    _sensors.toggleRelay();
    _server.send(200, "text/plain", "OK");
  });
  _server.on("/api/sds/measure", HTTP_POST, [this]() {
    if (!auth()) return;
    _server.send(_sensors.requestSdsMeasurement() ? 200 : 409, "text/plain", "OK");
  });
  _server.on("/api/sds/sleep", HTTP_POST, [this]() {
    if (!auth()) return;
    _server.send(_sensors.forceSdsSleep() ? 200 : 409, "text/plain", "OK");
  });
  _server.on("/config", HTTP_GET, [this]() { handleConfig(); });
  _server.on("/save", HTTP_POST, [this]() { handleSave(); });
  _server.on("/factory", HTTP_GET, [this]() { handleFactory(); });
  setupOta();
  _server.begin();
}

void WebUi::loop() {
  _server.handleClient();
}
