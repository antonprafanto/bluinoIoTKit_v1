/*
 * BAB 47 - Program 3: Dashboard IoT Real-Time Interaktif (WebSocket)
 * ───────────────────────────────────────────────────────────────────
 * Fitur Lengkap:
 *   ▶ HTTP port 80 → Dashboard HTML (PROGMEM, zero heap)
 *   ▶ WS   port 81 → Bidirectional JSON protocol:
 *       Server→Client : push sensor tiap 2 detik
 *       Client→Server : perintah led/relay/status/restart
 *       Server→Client : ack konfirmasi perintah
 *   ▶ DHT11 non-blocking (IO27, hardwired Bluino Kit)
 *   ▶ LED IO2 dikontrol via WS + physical Serial CLI
 *   ▶ Relay (IO custom) dikontrol via WS + NVS persistence
 *   ▶ NVS: state relay tersimpan, bertahan setelah reboot
 *   ▶ Serial CLI: help | status | led on/off | relay on/off | restart
 *   ▶ mDNS: http://bluino.local
 *
 * JSON Protocol:
 *   ESP32 → Browser (push):
 *     {"type":"sensors","temp":28.5,"humidity":65,"rssi":-55,
 *      "uptime":1234,"freeHeap":195000,"sensorOk":true,
 *      "ledState":true,"relayState":false,"clients":2}
 *
 *   Browser → ESP32 (perintah):
 *     {"cmd":"led","state":1}
 *     {"cmd":"relay","state":0}
 *     {"cmd":"status"}
 *     {"cmd":"restart"}
 *
 *   ESP32 → Browser (konfirmasi):
 *     {"type":"ack","cmd":"led","state":true,"ok":true}
 *
 * Pin Relay (CUSTOM):
 *   Pilih dari: IO4, IO13, IO16, IO17, IO25, IO26, IO32, IO33
 *   Sambungkan kabel jumper relay ke #define RELAY_PIN di bawah
 *
 * Library yang dibutuhkan:
 *   - "WebSockets" by Markus Sattler >= 2.4.0  (install via Library Manager)
 *   - "DHT sensor library" by Adafruit          (install via Library Manager)
 *   - "Adafruit Unified Sensor"                 (dependensi DHT, install otomatis)
 *   - WiFi.h, ESPmDNS.h, WebServer.h, Preferences.h (bawaan ESP32 Core)
 *
 * ⚠️ PERINGATAN: Ganti SSID, PASSWORD, dan RELAY_PIN sebelum upload!
 * ⚠️ ESP32 hanya mendukung WiFi 2.4 GHz!
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Preferences.h>
#include <DHT.h>

// ── Konfigurasi WiFi ─────────────────────────────────────────────
const char* WIFI_SSID = "NamaWiFiKamu";  // ← Ganti!
const char* WIFI_PASS = "PasswordWiFi";  // ← Ganti!
const char* MDNS_NAME = "bluino";

// ── Pin ──────────────────────────────────────────────────────────
#define LED_PIN   2    // LED built-in IO2 (hardwired)
#define DHT_PIN   27   // DHT11 IO27 (hardwired di Bluino Kit)
#define DHT_TYPE  DHT11

// Relay: pilih dari IO4, IO13, IO16, IO17, IO25, IO26, IO32, IO33
#define RELAY_PIN 4    // ← Ganti jika perlu! Sambungkan jumper ke pin ini.

// ── NVS Namespace & Key ───────────────────────────────────────────
const char* NVS_NS        = "iot47";
const char* NVS_KEY_RELAY = "relay";

// ── Objek Global ─────────────────────────────────────────────────
WebServer        server(80);
WebSocketsServer ws(81);
DHT              dht(DHT_PIN, DHT_TYPE);
Preferences      prefs;

// ── State ─────────────────────────────────────────────────────────
float   lastTemp   = NAN;
float   lastHumid  = NAN;
bool    sensorOk   = false;
bool    ledState   = false;
bool    relayState = false;
uint8_t wsClients  = 0;

// ── NVS Helper ────────────────────────────────────────────────────
void loadRelayState() {
  prefs.begin(NVS_NS, true);           // Read-only
  relayState = prefs.getBool(NVS_KEY_RELAY, false);
  prefs.end();
}
void saveRelayState(bool s) {
  prefs.begin(NVS_NS, false);          // Read-Write
  prefs.putBool(NVS_KEY_RELAY, s);
  prefs.end();
}

// ─────────────────────────────────────────────────────────────────
// Dashboard HTML (PROGMEM — Flash memory, tidak memakan heap)
// JS menggunakan WebSocket — tidak ada fetch() polling sama sekali
// ─────────────────────────────────────────────────────────────────
const char DASH_HTML[] PROGMEM = R"rawhtml(
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
    .sub{color:#444;font-size:.78rem;margin-bottom:8px}
    #live{float:right;font-size:.72rem;padding-top:18px;font-weight:600}
    .ok{color:#81c784}.err{color:#ef5350}.wait{color:#555}
    .sec{color:#555;font-size:.7rem;text-transform:uppercase;
         letter-spacing:1px;margin:14px 0 8px}
    .g2{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:14px}
    .card{background:#141424;border-radius:12px;padding:18px;text-align:center}
    .cl{color:#444;font-size:.72rem;text-transform:uppercase;margin-bottom:8px}
    .cv{font-size:2rem;font-weight:700;transition:color .5s}
    .cu{font-size:.82rem;color:#666}
    .hot{color:#ef5350}.warm{color:#ff9800}.cool{color:#4fc3f7}.humid{color:#26c6da}
    .ctrl{background:#141424;border-radius:12px;padding:14px 16px;
          display:flex;justify-content:space-between;align-items:center;margin-bottom:10px}
    .ci{flex:1}.cn{font-size:.95rem;font-weight:600;color:#ddd}
    .cp{font-size:.7rem;color:#444;margin-top:2px}
    .cs{font-size:.8rem;margin-top:4px}
    .on{color:#81c784}.off{color:#444}
    .btn{border:none;border-radius:8px;padding:10px 18px;font-size:.82rem;
         font-weight:600;cursor:pointer;transition:all .15s;min-width:80px;
         user-select:none;-webkit-tap-highlight-color:transparent}
    .bon{background:#1b5e20;color:#81c784}
    .boff{background:#b71c1c;color:#ef9a9a}
    .btn:active{transform:scale(.94);opacity:.8}
    .info{background:#141424;border-radius:12px;padding:14px;margin-bottom:12px}
    table{width:100%;border-collapse:collapse}
    td{padding:7px 4px;border-bottom:1px solid #18182a;font-size:.82rem}
    td:first-child{color:#555}td:last-child{color:#ccc;text-align:right}
    tr:last-child td{border-bottom:none}
    .footer{text-align:center;color:#1e1e2e;font-size:.65rem;padding:20px 0}
  </style>
</head>
<body><div class="c">
  <span id="live" class="wait">&#9679; Menghubungkan...</span>
  <h1>&#127926; Bluino IoT &#8212; Control Dashboard</h1>
  <p class="sub">BAB 47 &#183; WebSocket Real-Time &#8212; sensor push + kontrol instan</p>

  <p class="sec">&#127777; Sensor DHT11 &#8212; IO27</p>
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
      <div class="cp">IO2 &#8212; Built-in</div>
      <div id="ls" class="cs off">&#9679; Mati</div>
    </div>
    <button id="lb" class="btn bon" onclick="toggleLed()">Nyalakan</button>
  </div>
  <div class="ctrl">
    <div class="ci">
      <div class="cn">&#128268; Relay</div>
      <div class="cp" id="rpin">IO4 &#8212; Kabel Jumper</div>
      <div id="rs" class="cs off">&#9679; Mati</div>
    </div>
    <button id="rb" class="btn bon" onclick="toggleRelay()">Aktifkan</button>
  </div>

  <p class="sec">&#128246; Status Sistem</p>
  <div class="info"><table>
    <tr><td>Status Sensor</td><td id="sok">&#8212;</td></tr>
    <tr><td>Sinyal WiFi</td><td id="rssi">&#8212;</td></tr>
    <tr><td>Heap Bebas</td><td id="heap">&#8212;</td></tr>
    <tr><td>Uptime</td><td id="upt">&#8212;</td></tr>
    <tr><td>Browser Aktif</td><td id="cli">&#8212;</td></tr>
  </table></div>
  <p class="footer">Bluino IoT Kit 2026 &#8212; BAB 47: WebSocket</p>
</div>
<script>
let ws, ledOn = false, relayOn = false;
let watchdog;

function setLive(ok, txt) {
  const el = document.getElementById('live');
  el.textContent = '\u25cf ' + txt;
  el.className = ok ? 'ok' : 'err';
}

function setLedUI(s) {
  ledOn = !!s;
  document.getElementById('ls').textContent = s ? '\u25cf Menyala' : '\u25cf Mati';
  document.getElementById('ls').className = 'cs ' + (s ? 'on' : 'off');
  document.getElementById('lb').textContent = s ? 'Matikan' : 'Nyalakan';
  document.getElementById('lb').className = 'btn ' + (s ? 'boff' : 'bon');
}

function setRelayUI(s) {
  relayOn = !!s;
  document.getElementById('rs').textContent = s ? '\u25cf Aktif' : '\u25cf Mati';
  document.getElementById('rs').className = 'cs ' + (s ? 'on' : 'off');
  document.getElementById('rb').textContent = s ? 'Nonaktifkan' : 'Aktifkan';
  document.getElementById('rb').className = 'btn ' + (s ? 'boff' : 'bon');
}

function send(obj) {
  if (ws && ws.readyState === WebSocket.OPEN)
    ws.send(JSON.stringify(obj));
}

function toggleLed() {
  const ns = ledOn ? 0 : 1;
  setLedUI(ns);   // Optimistic update — ubah UI SEKETIKA
  send({ cmd: 'led', state: ns });
}

function toggleRelay() {
  const ns = relayOn ? 0 : 1;
  setRelayUI(ns); // Optimistic update
  send({ cmd: 'relay', state: ns });
}

function connect() {
  ws = new WebSocket('ws://' + location.hostname + ':81/');

  function resetWatchdog() {
    clearTimeout(watchdog);
    watchdog = setTimeout(() => {
      setLive(false, 'Stale Data \u2014 reset TCP...');
      if (ws.readyState === WebSocket.OPEN) ws.close();
    }, 6000);
  }

  ws.onopen  = () => { setLive(true, 'Live \u2014 WebSocket'); resetWatchdog(); };
  ws.onerror = () => setLive(false, 'Error');
  ws.onclose = () => {
    clearTimeout(watchdog);
    setLive(false, 'Offline \u2014 retry 3 detik...');
    setTimeout(connect, 3000);
  };
  ws.onmessage = e => {
    resetWatchdog();
    const d = JSON.parse(e.data);

    if (d.type === 'sensors') {
      // Suhu
      const te = document.getElementById('temp');
      if (d.temp !== null) {
        te.innerHTML = d.temp.toFixed(1) + '<span class="cu">&#176;C</span>';
        te.className = 'cv ' + (d.temp > 35 ? 'hot' : d.temp > 28 ? 'warm' : 'cool');
      }
      // Kelembaban
      if (d.humidity !== null)
        document.getElementById('hum').innerHTML =
          d.humidity.toFixed(0) + '<span class="cu">%</span>';
      // Sensor OK
      document.getElementById('sok').textContent =
        d.sensorOk ? '\u2705 OK' : '\u274c Error / Cek IO27';
      // RSSI
      const ri = d.rssi >= -65 ? '\ud83d\udfe2' : d.rssi >= -80 ? '\ud83d\udfe1' : '\ud83d\udd34';
      document.getElementById('rssi').textContent = ri + ' ' + d.rssi + ' dBm';
      // Heap
      document.getElementById('heap').textContent = (d.freeHeap / 1024).toFixed(0) + ' KB';
      // Uptime
      const u = d.uptime, uh = Math.floor(u/3600), um = Math.floor((u%3600)/60), us = u%60;
      document.getElementById('upt').textContent = uh + 'j ' + um + 'm ' + us + 'd';
      // Clients
      document.getElementById('cli').textContent = d.clients + ' browser';
      // Aktuator (sync state dari server)
      setLedUI(d.ledState);
      setRelayUI(d.relayState);
    }

    if (d.type === 'ack') {
      // Konfirmasi dari server — update UI dengan nilai aktual hardware
      if (d.cmd === 'led')   setLedUI(d.state);
      if (d.cmd === 'relay') setRelayUI(d.state);
    }
  };
}
connect();
</script>
</body></html>
)rawhtml";

// ─────────────────────────────────────────────────────────────────
// Broadcast JSON sensor ke semua client WebSocket
// ─────────────────────────────────────────────────────────────────
void broadcastSensors() {
  char tS[8] = "null", hS[8] = "null";
  if (sensorOk && !isnan(lastTemp))   snprintf(tS, sizeof(tS), "%.1f", lastTemp);
  if (sensorOk && !isnan(lastHumid))  snprintf(hS, sizeof(hS), "%.0f", lastHumid);

  char json[350];
  snprintf(json, sizeof(json),
    "{\"type\":\"sensors\","
    "\"temp\":%s,\"humidity\":%s,"
    "\"rssi\":%d,\"uptime\":%lu,"
    "\"freeHeap\":%u,\"sensorOk\":%s,"
    "\"ledState\":%s,\"relayState\":%s,"
    "\"clients\":%d}",
    tS, hS,
    WiFi.RSSI(), millis() / 1000UL,
    ESP.getFreeHeap(),
    sensorOk   ? "true" : "false",
    ledState   ? "true" : "false",
    relayState ? "true" : "false",
    wsClients
  );
  ws.broadcastTXT(json);
}

// ─────────────────────────────────────────────────────────────────
// WebSocket Event Handler
// ─────────────────────────────────────────────────────────────────
void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  if (type == WStype_CONNECTED) {
    wsClients++;
    Serial.printf("[WS] #%d terhubung dari %s | Total: %d\n",
                  num, ws.remoteIP(num).toString().c_str(), wsClients);
    digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    broadcastSensors(); // Kirim state terkini ke client baru
  }
  else if (type == WStype_DISCONNECTED) {
    if (wsClients > 0) wsClients--;
    Serial.printf("[WS] #%d terputus | Sisa: %d\n", num, wsClients);
    if (wsClients > 0) broadcastSensors(); // Sync UI Sisa Browser seketika
  }
  else if (type == WStype_TEXT) {
    // ── Parse JSON command dari browser (Zero-Heap Parsing) ────────
    // Menggunakan murni C-String pointer untuk membasmi memory fragmentation
    Serial.printf("[WS] #%d → %s\n", num, (char*)payload);

    // Ekstrak field JSON (Pedantic Escaped Quote Handler Pointer)
    auto getStr = [&](const char* key) -> String {
      char searchBuf[32];
      snprintf(searchBuf, sizeof(searchBuf), "\"%s\":\"", key);
      char* ptr = strstr((char*)payload, searchBuf);
      if (!ptr) return "";
      int s = (ptr - (char*)payload) + strlen(searchBuf);
      int e = s;
      while (e < length) {
        if (payload[e] == '"') {
          int count = 0, temp = e - 1;
          while (temp >= s && payload[temp] == '\\') { count++; temp--; }
          if (count % 2 == 0) break; // Kutip penutup asli (parity genap)
        }
        e++;
      }
      if (e > s) {
        char val[32] = {0};
        int len = e - s;
        if (len > 31) len = 31;
        strncpy(val, (char*)&payload[s], len);
        return String(val); // Hanya return kopian kecil ke pemanggil
      }
      return "";
    };
    
    // Ekstrak numerik (Murni Pointer, 100% bebas String)
    auto getInt = [&](const char* key) -> int {
      char searchBuf[32];
      snprintf(searchBuf, sizeof(searchBuf), "\"%s\":", key);
      char* ptr = strstr((char*)payload, searchBuf);
      if (!ptr) return -1;
      return atoi(ptr + strlen(searchBuf));
    };

    String cmd = getStr("cmd");

    if (cmd == "led") {
      ledState = (getInt("state") == 1);
      digitalWrite(LED_PIN, ledState ? HIGH : LOW);
      Serial.printf("[WS] LED %s via WebSocket\n", ledState ? "ON" : "OFF");
      char ack[64];
      snprintf(ack, sizeof(ack),
               "{\"type\":\"ack\",\"cmd\":\"led\",\"state\":%s,\"ok\":true}",
               ledState ? "true" : "false");
      ws.broadcastTXT(ack);  // Broadcast ack ke semua (sync multi-browser)
    }
    else if (cmd == "relay") {
      relayState = (getInt("state") == 1);
      digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
      saveRelayState(relayState);
      Serial.printf("[WS] Relay %s via WebSocket — disimpan NVS\n",
                    relayState ? "ON" : "OFF");
      char ack[64];
      snprintf(ack, sizeof(ack),
               "{\"type\":\"ack\",\"cmd\":\"relay\",\"state\":%s,\"ok\":true}",
               relayState ? "true" : "false");
      ws.broadcastTXT(ack);
    }
    else if (cmd == "status") {
      // Push snapshot status ke peminta saja
      broadcastSensors();
    }
    else if (cmd == "restart") {
      ws.sendTXT(num, "{\"type\":\"ack\",\"cmd\":\"restart\",\"ok\":true}");
      Serial.println("[WS] Restart via WebSocket...");
      delay(500); MDNS.end(); ESP.restart();
    }
  }
}

// ─────────────────────────────────────────────────────────────────
// Serial CLI
// ─────────────────────────────────────────────────────────────────
void processCliCommand(const String& raw) {
  String cmd = raw; cmd.trim();
  if (cmd.isEmpty()) return;
  Serial.println("[CLI] > " + cmd);
  String lc = cmd; lc.toLowerCase();

  if (lc == "help" || lc == "?") {
    Serial.println("Perintah: status | led on | led off | relay on | relay off | restart | help");
  } else if (lc == "status") {
    Serial.println("-- Status ----------------------------------------");
    Serial.printf("  WiFi   : %s\n", WiFi.status()==WL_CONNECTED?"Terhubung":"Terputus");
    Serial.printf("  IP     : %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("  RSSI   : %d dBm\n", WiFi.RSSI());
    Serial.printf("  LED    : %s\n", ledState?"ON":"OFF");
    Serial.printf("  Relay  : %s (IO%d)\n", relayState?"ON":"OFF", RELAY_PIN);
    Serial.printf("  Suhu   : %.1f C\n", lastTemp);
    Serial.printf("  Hum    : %.0f %%\n", lastHumid);
    Serial.printf("  Heap   : %u bytes\n", ESP.getFreeHeap());
    Serial.printf("  Uptime : %lu s\n", millis()/1000UL);
    Serial.printf("  WS     : %d client\n", wsClients);
    Serial.println("-------------------------------------------------");
  } else if (lc == "led on") {
    ledState = true; digitalWrite(LED_PIN, HIGH);
    Serial.println("[CLI] LED ON");
    broadcastSensors();
  } else if (lc == "led off") {
    ledState = false; digitalWrite(LED_PIN, LOW);
    Serial.println("[CLI] LED OFF");
    broadcastSensors();
  } else if (lc == "relay on") {
    relayState = true; digitalWrite(RELAY_PIN, HIGH); saveRelayState(true);
    Serial.println("[CLI] Relay ON — disimpan NVS");
    broadcastSensors();
  } else if (lc == "relay off") {
    relayState = false; digitalWrite(RELAY_PIN, LOW); saveRelayState(false);
    Serial.println("[CLI] Relay OFF — disimpan NVS");
    broadcastSensors();
  } else if (lc == "restart") {
    Serial.println("[CLI] Restart dalam 2 detik...");
    MDNS.end(); delay(2000); ESP.restart();
  } else {
    Serial.printf("[CLI] Tidak dikenal: '%s'. Ketik 'help'.\n", cmd.c_str());
  }
}

// ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN,   OUTPUT); digitalWrite(LED_PIN,   LOW);
  pinMode(RELAY_PIN, OUTPUT); digitalWrite(RELAY_PIN, LOW);
  dht.begin();
  delay(500);

  Serial.println("\n=== BAB 47 Program 3: Dashboard IoT Real-Time Interaktif ===");
  Serial.println("[INFO] Ketik 'help' di Serial Monitor.");
  Serial.printf("[INFO] Relay PIN: IO%d — sambungkan kabel jumper!\n", RELAY_PIN);

  // ── Muat state relay dari NVS ──────────────────────────────────
  loadRelayState();
  digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
  Serial.printf("[NVS] Relay state dimuat: %s\n", relayState ? "ON" : "OFF");

  // ── Koneksi WiFi ──────────────────────────────────────────────
  Serial.printf("[WiFi] Menghubungkan ke '%s'...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(MDNS_NAME);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  uint32_t tStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - tStart >= 15000UL) {
      Serial.println("\n[ERR] Timeout WiFi. Restart...");
      delay(2000); ESP.restart();
    }
    // Terima perintah CLI bahkan saat menunggu WiFi
    if (Serial.available()) {
      String inp = Serial.readStringUntil('\n'); inp.trim();
      if (inp.length() > 0) Serial.println("[CLI] Tunggu WiFi terhubung...");
    }
    Serial.print("."); delay(500);
  }
  Serial.println();
  Serial.printf("[WiFi] ✅ Terhubung! IP: %s | SSID: %s\n",
                WiFi.localIP().toString().c_str(), WiFi.SSID().c_str());

  // ── mDNS ──────────────────────────────────────────────────────
  if (MDNS.begin(MDNS_NAME)) {
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws",   "tcp", 81);
    MDNS.addServiceTxt("http", "tcp", "api-ws", "ws://bluino.local:81");
    Serial.printf("[mDNS] ✅ http://%s.local\n", MDNS_NAME);
  } else {
    Serial.println("[mDNS] ⚠️ Gagal — gunakan IP langsung.");
  }

  // ── HTTP ──────────────────────────────────────────────────────
  server.on("/", HTTP_GET, []() {
    server.send_P(200, PSTR("text/html"), DASH_HTML);
  });
  server.onNotFound([]() {
    if (server.method() == HTTP_OPTIONS) {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.sendHeader("Access-Control-Allow-Methods", "GET, OPTIONS");
      server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
      server.send(204); return;
    }
    server.send(404, "text/plain", "404 Not Found");
  });
  server.begin();
  Serial.println("[HTTP] Server aktif di port 80.");

  // ── WebSocket ──────────────────────────────────────────────────
  ws.begin();
  ws.onEvent(onWsEvent);
  Serial.println("[WS]   Server aktif di port 81.");

  Serial.println("\n==========================================");
  Serial.printf(" Dashboard : http://%s.local\n",      MDNS_NAME);
  Serial.printf(" WebSocket : ws://%s.local:81\n",     MDNS_NAME);
  Serial.printf(" Relay PIN : IO%d (kabel jumper)\n",  RELAY_PIN);
  Serial.println("==========================================");
}

// ─────────────────────────────────────────────────────────────────
void loop() {
  server.handleClient();
  ws.loop();

  // ── DHT11 Non-Blocking (tiap 2 detik) ────────────────────────
  static unsigned long tDHT = 0;
  if (millis() - tDHT >= 2000UL) {
    tDHT = millis();
    float t = dht.readTemperature(), h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      if (!sensorOk) Serial.println("[DHT] ✅ Sensor pulih!");
      lastTemp = t; lastHumid = h; sensorOk = true;
    } else {
      if (sensorOk) Serial.println("[DHT] ⚠️ Gagal baca! Cek wiring IO27.");
      sensorOk = false;
    }
    // Hemat CPU & Energi: Bangun JSON hanya jika ada browser aktif
    if (wsClients > 0) broadcastSensors();
  }

  // ── Serial CLI (Zero-Heap Buffer) ──────────────────────────────
  static char cliArr[128];
  static uint8_t cliIdx = 0;
  
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (cliIdx > 0) {
        cliArr[cliIdx] = '\0';
        processCliCommand(String(cliArr));
        cliIdx = 0;
      }
    } else if (cliIdx < 127) {
      cliArr[cliIdx++] = c;
    }
  }

  // ── Deteksi Transisi WiFi ─────────────────────────────────────
  static bool prevWiFi = true;
  bool currWiFi = (WiFi.status() == WL_CONNECTED);
  if (!prevWiFi && currWiFi)
    Serial.printf("[WiFi] ✅ Reconnect! IP: %s\n",
                  WiFi.localIP().toString().c_str());
  if (prevWiFi && !currWiFi)
    Serial.println("[WiFi] ⚠️ Koneksi terputus...");
  prevWiFi = currWiFi;

  // ── Heartbeat (tiap 30 detik) ──────────────────────────────────
  static unsigned long tHb = 0;
  if (millis() - tHb >= 30000UL) {
    tHb = millis();
    Serial.printf("[HB] %.1fC %.0f%% LED:%s Relay:%s WS:%d Heap:%u B\n",
                  lastTemp, lastHumid,
                  ledState   ? "ON" : "OFF",
                  relayState ? "ON" : "OFF",
                  wsClients, ESP.getFreeHeap());
  }
}
