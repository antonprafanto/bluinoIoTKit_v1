/*
 * BAB 46 - Program 3: Dashboard IoT Interaktif
 *
 * Fitur:
 *   > GET /            -> Dashboard HTML via PROGMEM (zero heap!)
 *   > GET /api/sensors -> JSON sensor + state aktuator
 *   > POST /api/led    -> Kontrol LED IO2 (?state=0/1)
 *   > POST /api/relay  -> Kontrol Relay (?state=0/1)
 *   > NVS: state relay disimpan, bertahan setelah reboot
 *   > DHT11 non-blocking (IO27, hardwired di Bluino Kit)
 *   > Serial CLI: help, status, led on/off, relay on/off, restart
 *   > mDNS: http://bluino.local
 *
 * Pin Relay (CUSTOM):
 *   Pilih dari: IO4, IO13, IO16, IO17, IO25, IO26, IO32, IO33
 *   Sambungkan kabel jumper dari pin relay ke GPIO yang dipilih.
 *   Ubah #define RELAY_PIN sesuai pin kamu!
 *
 * Library (install via Arduino Library Manager):
 *   "DHT sensor library" by Adafruit
 *   "Adafruit Unified Sensor" (dependensi, install otomatis)
 *
 * PERINGATAN: ESP32 hanya mendukung WiFi 2.4 GHz!
 * PERINGATAN: Ganti SSID, PASSWORD, dan RELAY_PIN!
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <Preferences.h>
#include <DHT.h>

// -- Konfigurasi WiFi -------------------------------------------------
const char* WIFI_SSID = "NamaWiFiKamu";   // <- Ganti!
const char* WIFI_PASS = "PasswordWiFi";   // <- Ganti!
const char* MDNS_NAME = "bluino";

// -- Pin --------------------------------------------------------------
#define LED_PIN   2     // LED built-in IO2 (hardwired)
#define DHT_PIN   27    // DHT11 IO27 (hardwired di Bluino Kit)
#define DHT_TYPE  DHT11

// Ganti sesuai kabel jumper relay kamu!
// Pilih dari: 4, 13, 16, 17, 25, 26, 32, 33
#define RELAY_PIN 4     // <- Ganti jika perlu!

// -- NVS --------------------------------------------------------------
const char* NVS_NS        = "iot-ctrl";
const char* NVS_KEY_RELAY = "relay";

// -- Objek Global -----------------------------------------------------
WebServer   server(80);
DHT         dht(DHT_PIN, DHT_TYPE);
Preferences prefs;

// -- State ------------------------------------------------------------
float  lastTemp   = NAN;
float  lastHumid  = NAN;
bool   sensorOk   = false;
bool   ledState   = false;
bool   relayState = false;
String inputBuffer;          // CLI buffer (pre-alokasi di setup)

// -- NVS: Muat & Simpan -----------------------------------------------
void loadRelayState() {
  prefs.begin(NVS_NS, true);
  relayState = prefs.getBool(NVS_KEY_RELAY, false);
  prefs.end();
}
void saveRelayState(bool state) {
  prefs.begin(NVS_NS, false);
  prefs.putBool(NVS_KEY_RELAY, state);
  prefs.end();
}

// -- HTML Dashboard disimpan di PROGMEM (Flash Memory) ----------------
const char DASHBOARD_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Bluino IoT Control</title>
  <style>
    *{box-sizing:border-box;margin:0;padding:0}
    body{font-family:'Segoe UI',Arial,sans-serif;background:#0a0a14;
         color:#e0e0e0;min-height:100vh;padding:16px}
    .c{max-width:500px;margin:0 auto}
    h1{color:#4fc3f7;font-size:1.3rem;padding:16px 0 4px}
    .sub{color:#444;font-size:.78rem;margin-bottom:18px}
    .sec{color:#555;font-size:.7rem;text-transform:uppercase;
         letter-spacing:1px;margin:16px 0 8px}
    .g2{display:grid;grid-template-columns:1fr 1fr;gap:10px}
    .card{background:#141424;border-radius:12px;padding:16px}
    .cl{color:#444;font-size:.72rem;text-transform:uppercase;margin-bottom:6px}
    .cv{font-size:1.9rem;font-weight:700}
    .cu{font-size:.8rem;color:#666}
    .hot{color:#ef5350}.warm{color:#ff9800}
    .cool{color:#4fc3f7}.humid{color:#26c6da}
    .ctrl{background:#141424;border-radius:12px;padding:16px;
          display:flex;justify-content:space-between;
          align-items:center;margin-bottom:10px}
    .ci{flex:1}.cn{font-size:.95rem;font-weight:600;color:#ddd}
    .cp{font-size:.7rem;color:#444;margin-top:2px}
    .cs{font-size:.8rem;margin-top:4px}
    .on{color:#81c784}.off{color:#444}
    .btn{border:none;border-radius:8px;padding:10px 18px;
         font-size:.82rem;font-weight:600;cursor:pointer;
         transition:all .15s;min-width:70px;
         user-select:none;-webkit-tap-highlight-color:transparent}
    .bon{background:#1b5e20;color:#81c784}
    .boff{background:#b71c1c;color:#ef9a9a}
    .btn:active{transform:scale(.95);opacity:.8}
    .info{background:#141424;border-radius:12px;padding:14px;margin-bottom:10px}
    table{width:100%;border-collapse:collapse}
    td{padding:7px 4px;border-bottom:1px solid #18182a;font-size:.82rem}
    td:first-child{color:#555}td:last-child{color:#ccc;text-align:right}
    #live{float:right;font-size:.72rem;color:#81c784;padding-top:20px}
    .footer{text-align:center;color:#1e1e2e;font-size:.65rem;padding:20px 0}
  </style>
</head>
<body>
<div class="c">
  <span id="live">&#9679; Live</span>
  <h1>&#127911; Bluino IoT &#8212; Control Dashboard</h1>
  <p class="sub">BAB 46 &#183; Update tiap 5 detik via fetch()</p>

  <p class="sec">&#127777; Sensor</p>
  <div class="g2">
    <div class="card">
      <div class="cl">Suhu</div>
      <div id="temp" class="cv cool">&#8212;<span class="cu">&#176;C</span></div>
    </div>
    <div class="card">
      <div class="cl">Kelembaban</div>
      <div id="hum" class="cv humid">&#8212;<span class="cu">%</span></div>
    </div>
  </div>

  <p class="sec">&#128268; Kontrol Aktuator</p>
  <div class="ctrl">
    <div class="ci">
      <div class="cn">&#128161; LED Built-in</div>
      <div class="cp">Pin IO2 (hardwired board)</div>
      <div id="ls" class="cs off">&#9679; Mati</div>
    </div>
    <button id="lb" class="btn bon" onclick="toggleLed()">Nyalakan</button>
  </div>
  <div class="ctrl">
    <div class="ci">
      <div class="cn">&#9889; Relay</div>
      <div class="cp">Pin Custom (lihat kode)</div>
      <div id="rs" class="cs off">&#9679; Mati</div>
    </div>
    <button id="rb" class="btn bon" onclick="toggleRelay()">Aktifkan</button>
  </div>

  <p class="sec">&#128225; Jaringan &amp; Sistem</p>
  <div class="info">
    <table>
      <tr><td>Sinyal WiFi</td><td id="rssi">&#8212;</td></tr>
      <tr><td>Uptime</td><td id="uptime">&#8212;</td></tr>
      <tr><td>Heap Bebas</td><td id="heap">&#8212;</td></tr>
      <tr><td>Sensor DHT11</td><td id="sok">&#8212;</td></tr>
    </table>
  </div>
  <p class="footer">Bluino IoT Kit 2026 &#8212; BAB 46: Web Server</p>
</div>
<script>
let ledOn=false,relayOn=false;
function setLedUI(s){
  ledOn=s;
  document.getElementById("ls").textContent=s?"\u25cf Menyala":"\u25cf Mati";
  document.getElementById("ls").className="cs "+(s?"on":"off");
  document.getElementById("lb").textContent=s?"Matikan":"Nyalakan";
  document.getElementById("lb").className="btn "+(s?"boff":"bon");
}
function setRelayUI(s){
  relayOn=s;
  document.getElementById("rs").textContent=s?"\u25cf Aktif":"\u25cf Mati";
  document.getElementById("rs").className="cs "+(s?"on":"off");
  document.getElementById("rb").textContent=s?"Nonaktifkan":"Aktifkan";
  document.getElementById("rb").className="btn "+(s?"boff":"bon");
}
async function update(){
  try{
    const abort = new AbortController();
    setTimeout(()=>abort.abort(), 3000);
    const d=await(await fetch("/api/sensors?_t="+Date.now(), {signal: abort.signal})).json();
    const te=document.getElementById("temp");
    if(d.temp!==null){
      te.innerHTML=d.temp.toFixed(1)+"\u00b0C";
      te.className="cv "+(d.temp>35?"hot":d.temp>28?"warm":"cool");
    }
    if(d.humidity!==null)
      document.getElementById("hum").innerHTML=d.humidity.toFixed(0)+"%";
    let ri="\u1F534";
    if (d.rssi>=-65) ri="\u1F7E2";
    else if (d.rssi>=-80) ri="\u1F7E1";
    document.getElementById("rssi").textContent=ri+" "+d.rssi+" dBm";
    document.getElementById("heap").textContent=(d.freeHeap/1024).toFixed(0)+" KB";
    document.getElementById("sok").textContent=d.sensorOk?"\u2705 OK":"\u274C Error";
    const u=d.uptime,uh=Math.floor(u/3600),um=Math.floor((u%3600)/60),us=u%60;
    document.getElementById("uptime").textContent=uh+"j "+um+"m "+us+"d";
    setLedUI(d.ledState);setRelayUI(d.relayState);
    document.getElementById("live").textContent="\u25cf Live";
    document.getElementById("live").style.color="#81c784";
  }catch(e){
    document.getElementById("live").textContent="\u26a0 Offline";
    document.getElementById("live").style.color="#ef5350";
  }
}
async function toggleLed(){
  const ns=ledOn?0:1;
  setLedUI(ns);
  try{
    const d=await(await fetch("/api/led?state="+ns,{method:"POST"})).json();
    setLedUI(d.state);
  }catch(e){ setLedUI(!ns); }
}
async function toggleRelay(){
  const ns=relayOn?0:1;
  setRelayUI(ns);
  try{
    const d=await(await fetch("/api/relay?state="+ns,{method:"POST"})).json();
    setRelayUI(d.state);
  }catch(e){ setRelayUI(!ns); }
}
(async function loop(){ await update(); setTimeout(loop, 5000); })();
</script>
</body>
</html>
)rawhtml";

// -- Serial CLI -------------------------------------------------------
void processSerialCommand(const String& raw){
  String cmd=raw; cmd.trim();
  if(cmd.length()==0) return;
  Serial.println("[CLI] > "+cmd);
  String lc=cmd; lc.toLowerCase();

  if(lc=="help"||lc=="?"){
    Serial.println("Perintah: status | led on | led off | relay on | relay off | restart | help");
  } else if(lc=="status"){
    Serial.println("-- Status ------------------------------------------");
    Serial.printf("  WiFi    : %s\n", WiFi.status()==WL_CONNECTED?"Terhubung":"Terputus");
    Serial.printf("  IP      : %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("  RSSI    : %d dBm\n", WiFi.RSSI());
    Serial.printf("  LED     : %s\n", ledState?"ON":"OFF");
    Serial.printf("  Relay   : %s (IO%d)\n", relayState?"ON":"OFF", RELAY_PIN);
    Serial.printf("  Suhu    : %.1f C\n", lastTemp);
    Serial.printf("  Hum     : %.0f %%\n", lastHumid);
    Serial.printf("  Heap    : %u bytes\n", ESP.getFreeHeap());
    Serial.printf("  Uptime  : %lu s\n", millis()/1000UL);
    Serial.println("----------------------------------------------------");
  } else if(lc=="led on"){
    ledState=true; digitalWrite(LED_PIN,HIGH);
    Serial.println("[CLI] LED ON");
  } else if(lc=="led off"){
    ledState=false; digitalWrite(LED_PIN,LOW);
    Serial.println("[CLI] LED OFF");
  } else if(lc=="relay on"){
    relayState=true; digitalWrite(RELAY_PIN,HIGH); saveRelayState(true);
    Serial.println("[CLI] Relay ON -- disimpan ke NVS");
  } else if(lc=="relay off"){
    relayState=false; digitalWrite(RELAY_PIN,LOW); saveRelayState(false);
    Serial.println("[CLI] Relay OFF -- disimpan ke NVS");
  } else if(lc=="restart"){
    Serial.println("[CLI] Restart dalam 2 detik...");
    MDNS.end(); delay(2000); ESP.restart();
  } else {
    Serial.printf("[CLI] Tidak dikenal: '%s'. Ketik 'help'.\n", cmd.c_str());
  }
}

// -- setup ------------------------------------------------------------
void setup(){
  Serial.begin(115200);
  pinMode(LED_PIN,OUTPUT);   digitalWrite(LED_PIN,LOW);
  pinMode(RELAY_PIN,OUTPUT); digitalWrite(RELAY_PIN,LOW);
  dht.begin(); delay(500);

  Serial.println("\n=== BAB 46 Program 3: Dashboard IoT Interaktif ===");
  Serial.println("[INFO] Ketik 'help' di Serial Monitor.");
  Serial.printf("[INFO] Relay PIN: IO%d -- sambungkan kabel jumper!\n", RELAY_PIN);

  inputBuffer.reserve(128);

  // Muat state relay dari NVS
  loadRelayState();
  digitalWrite(RELAY_PIN, relayState?HIGH:LOW);
  Serial.printf("[NVS] Relay state dimuat: %s\n", relayState?"ON":"OFF");

  // Koneksi WiFi
  Serial.printf("[WiFi] Menghubungkan ke '%s'...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA); WiFi.setHostname(MDNS_NAME);
  WiFi.setAutoReconnect(true); WiFi.begin(WIFI_SSID,WIFI_PASS);

  uint32_t tS=millis();
  while(WiFi.status()!=WL_CONNECTED){
    if(millis()-tS>=20000UL){
      Serial.println("\n[ERR] Timeout WiFi. Restart...");
      delay(2000); ESP.restart();
    }
    if(Serial.available()){
      String inp=Serial.readStringUntil('\n'); inp.trim();
      if(inp.length()>0) Serial.println("\n[CLI] Tunggu WiFi terhubung...");
    }
    Serial.print("."); delay(500);
  }
  Serial.println();
  Serial.printf("[WiFi] Terhubung! IP: %s | SSID: %s\n",
    WiFi.localIP().toString().c_str(), WiFi.SSID().c_str());

  // mDNS
  if(MDNS.begin(MDNS_NAME)){
    MDNS.addService("http","tcp",80);
    MDNS.addServiceTxt("http","tcp","api","/api/sensors");
    Serial.printf("[mDNS] http://%s.local\n", MDNS_NAME);
  }

  // GET / -- HTML dari PROGMEM, zero heap!
  server.on("/",HTTP_GET,[](){
    server.send_P(200, PSTR("text/html"), DASHBOARD_HTML);
  });

  // GET /api/sensors -- JSON stack-allocated
  server.on("/api/sensors",HTTP_GET,[](){
    char tS[8]="null", hS[8]="null";
    if(sensorOk&&!isnan(lastTemp))   snprintf(tS,sizeof(tS),"%.1f",lastTemp);
    if(sensorOk&&!isnan(lastHumid))  snprintf(hS,sizeof(hS),"%.0f",lastHumid);
    char json[300];
    snprintf(json,sizeof(json),
      "{\"temp\":%s,\"humidity\":%s,\"rssi\":%d,\"uptime\":%lu,"
      "\"freeHeap\":%u,\"cpuMHz\":%u,\"sensorOk\":%s,"
      "\"ledState\":%s,\"relayState\":%s}",
      tS,hS, WiFi.RSSI(), millis()/1000UL,
      ESP.getFreeHeap(), ESP.getCpuFreqMHz(),
      sensorOk  ?"true":"false",
      ledState  ?"true":"false",
      relayState?"true":"false"
    );
    server.sendHeader("Access-Control-Allow-Origin","*");
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
    server.setContentLength(strlen(json));
    server.send(200,"application/json","");
    server.sendContent(json);
  });

  // POST /api/led?state=1 atau ?state=0
  server.on("/api/led",HTTP_POST,[](){
    server.sendHeader("Access-Control-Allow-Origin","*");
    server.sendHeader("Cache-Control","no-store");
    if(!server.hasArg("state")){
      server.send(400,"application/json","{\"error\":\"missing state\"}"); return;
    }
    ledState=(server.arg("state")=="1");
    digitalWrite(LED_PIN, ledState?HIGH:LOW);
    Serial.printf("[HTTP] LED %s via API\n", ledState?"ON":"OFF");
    char resp[40];
    snprintf(resp,sizeof(resp),"{\"ok\":true,\"state\":%s}", ledState?"true":"false");
    server.setContentLength(strlen(resp));
    server.send(200,"application/json",""); server.sendContent(resp);
  });

  // POST /api/relay?state=1 atau ?state=0
  server.on("/api/relay",HTTP_POST,[](){
    server.sendHeader("Access-Control-Allow-Origin","*");
    server.sendHeader("Cache-Control","no-store");
    if(!server.hasArg("state")){
      server.send(400,"application/json","{\"error\":\"missing state\"}"); return;
    }
    relayState=(server.arg("state")=="1");
    digitalWrite(RELAY_PIN, relayState?HIGH:LOW);
    saveRelayState(relayState);
    Serial.printf("[HTTP] Relay %s via API -- disimpan NVS\n", relayState?"ON":"OFF");
    char resp[40];
    snprintf(resp,sizeof(resp),"{\"ok\":true,\"state\":%s}", relayState?"true":"false");
    server.setContentLength(strlen(resp));
    server.send(200,"application/json",""); server.sendContent(resp);
  });

  // 404 & CORS Preflight (OPTIONS) Handler
  server.onNotFound([](){
    if(server.method() == HTTP_OPTIONS){
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
      server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
      server.send(204);
      return;
    }
    server.send(404,"text/plain","404 Not Found");
  });
  server.begin();

  Serial.println("\n==========================================");
  Serial.printf(" Dashboard  : http://%s.local\n", MDNS_NAME);
  Serial.printf(" Sensor API : http://%s.local/api/sensors\n", MDNS_NAME);
  Serial.printf(" LED API    : POST http://%s.local/api/led?state=1\n", MDNS_NAME);
  Serial.printf(" Relay API  : POST http://%s.local/api/relay?state=1\n", MDNS_NAME);
  Serial.println("==========================================");
}

// -- loop -------------------------------------------------------------
void loop(){
  server.handleClient();

  // DHT11 Non-Blocking (setiap 2 detik)
  static unsigned long tDHT=0;
  if(millis()-tDHT>=2000UL){
    tDHT=millis();
    float t=dht.readTemperature(), h=dht.readHumidity();
    if(!isnan(t)&&!isnan(h)){
      if(!sensorOk) Serial.println("[DHT] Sensor pulih!");
      lastTemp=t; lastHumid=h; sensorOk=true;
    } else {
      if(sensorOk) Serial.println("[DHT] Gagal baca! Cek wiring IO27.");
      sensorOk=false;
    }
  }

  // Serial CLI
  while(Serial.available()){
    char c=(char)Serial.read();
    if(c=='\n'||c=='\r'){
      if(inputBuffer.length()>0){ processSerialCommand(inputBuffer); inputBuffer=""; }
    } else if(inputBuffer.length()<128){ inputBuffer+=c; }
  }

  // Deteksi transisi WiFi
  static bool prev=true;
  bool curr=(WiFi.status()==WL_CONNECTED);
  if(!prev&&curr) Serial.printf("[WiFi] Reconnect! IP: %s\n", WiFi.localIP().toString().c_str());
  if(prev&&!curr) Serial.println("[WiFi] Koneksi terputus...");
  prev=curr;

  // Heartbeat tiap 30 detik
  static unsigned long tHb=0;
  if(millis()-tHb>=30000UL){
    tHb=millis();
    Serial.printf("[HB] %.1fC %.0f%% LED:%s Relay:%s Heap:%u B\n",
      lastTemp, lastHumid,
      ledState?"ON":"OFF", relayState?"ON":"OFF",
      ESP.getFreeHeap());
  }
}
