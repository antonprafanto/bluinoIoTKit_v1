/*
 * BAB 47 - Program 2: Real-Time Sensor Broadcasting
 * ──────────────────────────────────────────────────
 * Fitur:
 *   ▶ HTTP port 80 → Dashboard HTML (PROGMEM)
 *   ▶ WS   port 81 → Push JSON sensor ke SEMUA browser tiap 2 detik
 *   ▶ DHT11 non-blocking via millis() (IO27, hardwired Bluino Kit)
 *   ▶ Data: suhu, kelembaban, RSSI, uptime, free heap, client count
 *   ▶ State-transition logging: cetak Serial hanya saat status berubah
 *
 * Cara Uji:
 *   1. Ganti WIFI_SSID dan WIFI_PASS di bawah
 *   2. Upload, buka Serial Monitor (115200 baud)
 *   3. Browser: http://bluino.local — angka berubah TIAP 2 DETIK tanpa refresh!
 *   4. Buka 2+ browser sekaligus — semua update bersamaan (1 broadcast = semua dapat)
 *   5. Cabut kabel DHT11 — indikator "Error" muncul instan di semua browser
 *   6. Colok kembali — "✅ OK" muncul otomatis
 *
 * Library yang dibutuhkan:
 *   - "WebSockets" by Markus Sattler >= 2.4.0  (install via Library Manager)
 *   - "DHT sensor library" by Adafruit          (install via Library Manager)
 *   - "Adafruit Unified Sensor"                 (dependensi DHT, install otomatis)
 *   - WiFi.h, ESPmDNS.h, WebServer.h            (bawaan ESP32 Core)
 *
 * ⚠️ ESP32 hanya mendukung WiFi 2.4 GHz!
 * ⚠️ Ganti SSID dan PASSWORD sesuai jaringan WiFi kamu!
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <DHT.h>

// ── Konfigurasi ───────────────────────────────────────────────────
const char* WIFI_SSID = "NamaWiFiKamu";  // ← Ganti!
const char* WIFI_PASS = "PasswordWiFi";  // ← Ganti!
const char* MDNS_NAME = "bluino";

// ── Pin ──────────────────────────────────────────────────────────
#define LED_PIN  2    // LED built-in IO2 (active HIGH)
#define DHT_PIN  27   // DHT11 IO27 (hardwired di Bluino Kit, pull-up 4K7Ω)
#define DHT_TYPE DHT11

// ── Objek ────────────────────────────────────────────────────────
WebServer        server(80);
WebSocketsServer ws(81);
DHT              dht(DHT_PIN, DHT_TYPE);

// ── State Sensor (cache non-blocking) ────────────────────────────
float   lastTemp  = NAN;
float   lastHumid = NAN;
bool    sensorOk  = false;
uint8_t wsClients = 0;

// ─────────────────────────────────────────────────────────────────
// Dashboard HTML (PROGMEM → Flash memory, zero heap consumption)
// JavaScript menggunakan WebSocket — bukan fetch() polling
// ─────────────────────────────────────────────────────────────────
const char DASH_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Bluino IoT — Sensor Real-Time</title>
  <style>
    *{box-sizing:border-box;margin:0;padding:0}
    body{font-family:'Segoe UI',Arial,sans-serif;background:#0a0a14;
         color:#e0e0e0;min-height:100vh;padding:16px}
    .c{max-width:500px;margin:0 auto}
    h1{color:#4fc3f7;font-size:1.3rem;padding:16px 0 4px}
    .sub{color:#444;font-size:.78rem;margin-bottom:8px}
    #live{float:right;font-size:.72rem;padding-top:18px;font-weight:600}
    .ok{color:#81c784}.err{color:#ef5350}.wait{color:#666}
    .sec{color:#555;font-size:.7rem;text-transform:uppercase;
         letter-spacing:1px;margin:14px 0 8px}
    .g2{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:14px}
    .card{background:#141424;border-radius:12px;padding:18px;text-align:center}
    .cl{color:#444;font-size:.72rem;text-transform:uppercase;margin-bottom:8px}
    .cv{font-size:2rem;font-weight:700;transition:color .5s}
    .cu{font-size:.82rem;color:#666}
    .hot{color:#ef5350}.warm{color:#ff9800}.cool{color:#4fc3f7}.humid{color:#26c6da}
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
  <h1>&#127777; Sensor Real-Time</h1>
  <p class="sub">BAB 47 &#183; WebSocket Push &#8212; update otomatis tanpa polling!</p>

  <p class="sec">&#127777; DHT11 &#8212; IO27</p>
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

  <p class="sec">&#128268; Status Sistem</p>
  <div class="info"><table>
    <tr><td>Status Sensor</td><td id="sok">&#8212;</td></tr>
    <tr><td>Sinyal WiFi</td><td id="rssi">&#8212;</td></tr>
    <tr><td>Heap Bebas</td><td id="heap">&#8212;</td></tr>
    <tr><td>Uptime</td><td id="upt">&#8212;</td></tr>
    <tr><td>Browser Terhubung</td><td id="cli">&#8212;</td></tr>
  </table></div>
  <p class="footer">Bluino IoT Kit 2026 &#8212; BAB 47: WebSocket</p>
</div>
<script>
let ws;
let watchdog;

function setLive(ok, txt) {
  const el = document.getElementById('live');
  el.textContent = '\u25cf ' + txt;
  el.className = ok ? 'ok' : 'err';
}
function connect() {
  ws = new WebSocket('ws://' + location.hostname + ':81/');
  function resetWatchdog() {
    clearTimeout(watchdog);
    // Watchdog: Jika blank (tidak ada push) > 6 detik, anggap router hang/putus gaib
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
    setTimeout(connect, 3000);  // Auto-reconnect hardware loop
  };
  ws.onmessage = e => {
    resetWatchdog(); // Reset timer nyawa pelacak TCP
    const d = JSON.parse(e.data);
    if (d.type !== 'sensors') return;

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

    // Status sensor
    document.getElementById('sok').textContent = d.sensorOk ? '\u2705 OK' : '\u274c Error / Cek Kabel';

    // RSSI + emoji
    const ri = d.rssi >= -65 ? '\ud83d\udfe2' : d.rssi >= -80 ? '\ud83d\udfe1' : '\ud83d\udd34';
    document.getElementById('rssi').textContent = ri + ' ' + d.rssi + ' dBm';

    // Heap
    document.getElementById('heap').textContent = (d.freeHeap / 1024).toFixed(0) + ' KB';

    // Uptime
    const u = d.uptime;
    const uh = Math.floor(u / 3600), um = Math.floor((u % 3600) / 60), us = u % 60;
    document.getElementById('upt').textContent = uh + 'j ' + um + 'm ' + us + 'd';

    // Client count
    document.getElementById('cli').textContent = d.clients + ' browser';
  };
}
connect();
</script>
</body></html>
)rawhtml";

// ─────────────────────────────────────────────────────────────────
// Bangun & Broadcast JSON Sensor ke semua client WebSocket
// Stack-allocated: char json[300] — zero heap fragmentation
// ─────────────────────────────────────────────────────────────────
void broadcastSensors() {
  char tS[8] = "null", hS[8] = "null";
  if (sensorOk && !isnan(lastTemp))   snprintf(tS, sizeof(tS), "%.1f", lastTemp);
  if (sensorOk && !isnan(lastHumid))  snprintf(hS, sizeof(hS), "%.0f", lastHumid);

  char json[300];
  snprintf(json, sizeof(json),
    "{\"type\":\"sensors\","
    "\"temp\":%s,\"humidity\":%s,"
    "\"rssi\":%d,\"uptime\":%lu,"
    "\"freeHeap\":%u,\"sensorOk\":%s,"
    "\"clients\":%d}",
    tS, hS,
    WiFi.RSSI(), millis() / 1000UL,
    ESP.getFreeHeap(),
    sensorOk ? "true" : "false",
    wsClients
  );
  ws.broadcastTXT(json);
}

// ── WebSocket Event Handler ───────────────────────────────────────
void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  if (type == WStype_CONNECTED) {
    wsClients++;
    IPAddress ip = ws.remoteIP(num);
    Serial.printf("[WS] #%d terhubung dari %s | Total: %d\n",
                  num, ip.toString().c_str(), wsClients);
    digitalWrite(LED_PIN, HIGH);
    // Langsung push data sensor terkini ke client baru (tidak perlu tunggu 2 detik)
    broadcastSensors();
  }
  else if (type == WStype_DISCONNECTED) {
    if (wsClients > 0) wsClients--;
    Serial.printf("[WS] #%d terputus | Sisa: %d\n", num, wsClients);
    if (wsClients == 0) {
      digitalWrite(LED_PIN, LOW);
    } else {
      // Pedantic: Langsung informasikan "Sisa Browser" ke client yang masih hidup
      // tanpa harus menunggu siklus sensor 2 detik berikutnya.
      broadcastSensors();
    }
  }
}

// ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  dht.begin();
  delay(500);

  Serial.println("\n=== BAB 47 Program 2: Real-Time Sensor Broadcasting ===");

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
    Serial.print("."); delay(500);
  }
  Serial.println();
  Serial.printf("[WiFi] ✅ Terhubung! IP: %s | SSID: %s\n",
                WiFi.localIP().toString().c_str(), WiFi.SSID().c_str());

  // ── mDNS ──────────────────────────────────────────────────────
  if (MDNS.begin(MDNS_NAME)) {
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws",   "tcp", 81);
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
      server.send(204); return;
    }
    server.send(404, "text/plain", "404");
  });
  server.begin();
  Serial.println("[HTTP] Server aktif di port 80.");

  // ── WebSocket ─────────────────────────────────────────────────
  ws.begin();
  ws.onEvent(onWsEvent);
  Serial.println("[WS]   Server aktif di port 81.");
  Serial.printf("[INFO] Dashboard → http://%s.local\n", MDNS_NAME);
}

// ─────────────────────────────────────────────────────────────────
void loop() {
  server.handleClient();
  ws.loop();

  // ── DHT11 Non-Blocking (setiap 2 detik) ──────────────────────
  // millis() timer — loop() TIDAK di-block sama sekali
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
    // ── Zero-Client Optimization ──
    // Hemat CPU & Energi: Jangan repot merakit JSON jika tak ada browser yang menonton!
    if (wsClients > 0) {
      broadcastSensors();
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
    Serial.printf("[HB] %.1fC %.0f%% | Clients: %d | Heap: %u B\n",
                  lastTemp, lastHumid, wsClients, ESP.getFreeHeap());
  }
}
