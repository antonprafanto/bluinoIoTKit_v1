# BAB 47: WebSocket — Komunikasi Real-Time Dua Arah

> ✅ **Prasyarat:** Selesaikan **BAB 46 (Web Server)**. Kamu harus sudah memahami HTTP, REST API, dan dashboard berbasis `fetch()` polling.

> 📖 **Glosarium Singkatan BAB ini:**
> | Singkatan | Kepanjangan |
> |-----------|-------------|
> | **WS** | *WebSocket* — protokol komunikasi dua arah berbasis TCP yang persisten |
> | **TCP** | *Transmission Control Protocol* — protokol transport yang menjamin pengiriman paket |
> | **JSON** | *JavaScript Object Notation* — format data teks ringan berbasis key-value |
> | **mDNS** | *Multicast DNS* — resolusi hostname lokal tanpa DNS server |
> | **ACK** | *Acknowledgement* — pesan konfirmasi bahwa perintah diterima dan dieksekusi |
> | **NVS** | *Non-Volatile Storage* — penyimpanan Flash ESP32 yang persisten lintas reboot |
> | **RSSI** | *Received Signal Strength Indicator* — kekuatan sinyal WiFi dalam dBm |

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami **keterbatasan HTTP polling** dibanding WebSocket.
- Menjelaskan mekanisme **WebSocket Handshake** (HTTP Upgrade → persistent TCP).
- Menggunakan library `arduinoWebSockets` untuk membangun WebSocket server di ESP32.
- Membuat dashboard **Server-Side Push**: data muncul otomatis tanpa browser harus meminta.
- Mengirim **perintah kontrol JSON** dari browser ke ESP32 secara instan.
- Mengimplementasi **JSON protocol** dua arah yang terstruktur.
- Menangani **auto-reconnect** agar koneksi tahan terhadap gangguan jaringan.

---

## 47.1 Masalah: Polling vs Push

```
MASALAH HTTP POLLING (BAB 46):

  Browser                    ESP32
  ───────                    ─────
  │── GET /api/sensors ─────►│  ← Request #1 (detik ke-0)
  │◄─ JSON response ─────────│
  │   [tunggu 5 detik]       │  ← ESP32 & browser sama-sama idle
  │── GET /api/sensors ─────►│  ← Request #2 (detik ke-5)
  │◄─ JSON response ─────────│

  Masalah nyata:
  ✗ Data terupdate hanya tiap 5 detik
  ✗ Overhead TCP + HTTP header ~500 bytes per request
  ✗ 5 browser = 5× request/5 detik = beban tinggi ESP32
  ✗ ESP32 TIDAK BISA kirim data tanpa diminta terlebih dahulu


SOLUSI WEBSOCKET (BAB 47):

  Browser                    ESP32
  ───────                    ─────
  │── HTTP Upgrade ─────────►│  ← Handshake SEKALI saja
  │◄─ 101 Switching ─────────│
  │                          │  ← Koneksi TCP tetap terbuka
  │◄─ {sensors data} ────────│  ← ESP32 PUSH tiap 2 detik
  │◄─ {sensors data} ────────│     tanpa diminta!
  │── {cmd:led, state:1} ───►│  ← Browser kirim perintah kapan saja
  │◄─ {ack:led, state:1} ────│

  Keunggulan:
  ✅ Data muncul INSTAN saat ESP32 siap mengirim
  ✅ Header hanya 2-14 bytes per frame (vs ~500 bytes HTTP)
  ✅ 1 broadcastTXT() → semua browser dapat sekaligus
  ✅ Server bisa menginisiasi pengiriman kapan saja
```

---

## 47.2 Anatomi WebSocket Protocol

```
FASE 1: HANDSHAKE (HTTP → WebSocket Upgrade)

  Client → Server:
  GET / HTTP/1.1
  Host: bluino.local:81
  Upgrade: websocket
  Connection: Upgrade
  Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==
  Sec-WebSocket-Version: 13

  Server → Client:
  HTTP/1.1 101 Switching Protocols
  Upgrade: websocket
  Connection: Upgrade
  Sec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=

  → Setelah ini: bukan lagi HTTP, melainkan binary frame stream!


FASE 2: DATA FRAME

  ┌─────┬────────┬────────────────┬─────────┐
  │ FIN │ Opcode │ Payload Length │ Payload │
  └─────┴────────┴────────────────┴─────────┘

  Opcode umum:
  0x1 = Text frame  ← yang kita pakai (JSON string)
  0x8 = Close frame
  0x9 = Ping frame  ← library otomatis balas dengan Pong
  0xA = Pong frame


PORT YANG DIGUNAKAN:
  Port 80  → HTTP (halaman HTML dashboard)
  Port 81  → WebSocket (data real-time)
  Keduanya berjalan BERSAMAAN pada satu ESP32!
```

---

## 47.3 Library & Instalasi

```
LIBRARY TAMBAHAN (install via Arduino IDE Library Manager):

  Nama    : WebSockets
  Author  : Markus Sattler (Links2004)
  Versi   : >= 2.4.0
  Header  : WebSocketsServer.h
  Install : Library Manager → Cari "WebSockets" → pilih by Markus Sattler

LIBRARY DARI BAB SEBELUMNYA (tidak perlu install ulang):
  WiFi.h, ESPmDNS.h, WebServer.h, Preferences.h  → Bawaan ESP32 Core
  DHT.h                                           → Adafruit DHT sensor library


FUNGSI UTAMA WebSocketsServer:
  ws.begin()              → Mulai server
  ws.loop()               → Proses frame (panggil di loop(), non-blocking)
  ws.onEvent(callback)    → Daftarkan event handler
  ws.sendTXT(num, data)   → Kirim teks ke client tertentu (num = ID client)
  ws.broadcastTXT(data)   → Kirim teks ke SEMUA client sekaligus
  ws.remoteIP(num)        → Dapatkan IP client

EVENT CALLBACK:
  WStype_CONNECTED      → Client baru terhubung
  WStype_DISCONNECTED   → Client terputus
  WStype_TEXT           → Frame teks diterima (JSON kita)
  WStype_PING           → Ping diterima (library auto-reply Pong)
  WStype_PONG           → Konfirmasi Pong dari client
```

---

## 47.4 Program 1: WebSocket Echo Server

Program pertama: bangun WebSocket server sederhana — browser kirim pesan, ESP32 memantulkan kembali (*echo*). Ini membuktikan koneksi dua arah berjalan.

```cpp
/*
 * BAB 47 - Program 1: WebSocket Echo Server
 *
 * Fitur:
 *   ▶ HTTP server (port 80) menyajikan halaman HTML chat
 *   ▶ WebSocket server (port 81) menerima & echo pesan
 *   ▶ Menampilkan jumlah client aktif secara real-time
 *   ▶ Heartbeat log setiap 30 detik
 *
 * Library Tambahan:
 *   → Install "WebSockets" by Markus Sattler via Library Manager
 *
 * Cara Uji:
 *   1. Ganti WIFI_SSID dan WIFI_PASS
 *   2. Upload → buka Serial Monitor (115200 baud)
 *   3. Browser: http://bluino.local
 *   4. Ketik pesan → amati echo dari ESP32
 *   5. Buka browser ke-2 → amati counter "Client Aktif" berubah
 *
 * Hardware: Hanya ESP32 + WiFi
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

// ── Konfigurasi ───────────────────────────────────────────────────
const char* WIFI_SSID = "NamaWiFiKamu";   // ← Ganti!
const char* WIFI_PASS = "PasswordWiFi";   // ← Ganti!
const char* MDNS_NAME = "bluino";

// ── Objek ────────────────────────────────────────────────────────
WebServer        httpServer(80);
WebSocketsServer ws(81);

// ── State ────────────────────────────────────────────────────────
uint8_t  wsClients  = 0;
uint32_t msgCount   = 0;

// ── HTML Dashboard (disimpan di Flash via PROGMEM) ────────────────
static const char HTML_PAGE[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Bluino WS — Echo Chat</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body { font-family: 'Segoe UI', Arial, sans-serif; background: #0d0d1a;
           color: #e0e0e0; min-height: 100vh; padding: 20px; }
    .container { max-width: 520px; margin: 0 auto; }
    h1 { color: #4fc3f7; font-size: 1.4rem; padding-top: 20px; margin-bottom: 4px; }
    .sub { color: #555; font-size: 0.8rem; margin-bottom: 20px; }
    .status-bar { display: flex; gap: 12px; margin-bottom: 16px; }
    .badge { padding: 6px 14px; border-radius: 20px; font-size: 0.8rem; font-weight: bold; }
    .badge-ws { background: #1b5e20; color: #81c784; }
    .badge-cnt { background: #0d47a1; color: #90caf9; }
    #log { background: #060610; border-radius: 12px; padding: 16px;
           height: 300px; overflow-y: auto; font-family: monospace;
           font-size: 0.82rem; margin-bottom: 12px; }
    .msg-out { color: #90caf9; }
    .msg-in  { color: #81c784; }
    .msg-sys { color: #888; font-style: italic; }
    .input-row { display: flex; gap: 8px; }
    #msgInput { flex: 1; background: #1a1a2e; border: 1px solid #333;
                color: #eee; border-radius: 8px; padding: 10px 14px; font-size: 0.9rem; }
    #msgInput:focus { outline: none; border-color: #4fc3f7; }
    button { background: #0f3460; color: #4fc3f7; border: none; border-radius: 8px;
             padding: 10px 20px; cursor: pointer; font-size: 0.9rem; }
    button:hover { background: #1565c0; }
    .footer { text-align: center; color: #333; font-size: 0.72rem; margin-top: 16px; }
  </style>
</head>
<body>
<div class="container">
  <h1>💬 WebSocket Echo Chat</h1>
  <p class="sub">BAB 47 Program 1 — Bluino IoT Kit</p>
  <div class="status-bar">
    <span class="badge badge-ws" id="wsStatus">⚫ Menghubungkan...</span>
    <span class="badge badge-cnt" id="clientCount">👥 0 client</span>
  </div>
  <div id="log"></div>
  <div class="input-row">
    <input id="msgInput" type="text" placeholder="Ketik pesan, tekan Enter..." maxlength="64">
    <button onclick="sendMsg()">Kirim</button>
  </div>
  <p class="footer">Bluino IoT Kit 2026 — BAB 47: WebSocket</p>
</div>
<script>
  const log = document.getElementById('log');
  const wsStatus = document.getElementById('wsStatus');
  const clientCount = document.getElementById('clientCount');

  function addLog(text, cls) {
    const d = document.createElement('div');
    d.className = cls;
    d.textContent = text;
    log.appendChild(d);
    log.scrollTop = log.scrollHeight;
  }

  const ws = new WebSocket('ws://' + location.hostname + ':81');

  ws.onopen = () => {
    wsStatus.textContent = '🟢 Terhubung';
    wsStatus.style.background = '#1b5e20';
    addLog('[SISTEM] Terhubung ke ESP32 via WebSocket!', 'msg-sys');
  };

  ws.onclose = () => {
    wsStatus.textContent = '🔴 Terputus';
    wsStatus.style.background = '#b71c1c';
    addLog('[SISTEM] Koneksi terputus. Refresh halaman untuk reconnect.', 'msg-sys');
    setTimeout(() => location.reload(), 3000);
  };

  ws.onmessage = (e) => {
    try {
      const d = JSON.parse(e.data);
      if (d.type === 'echo') {
        addLog('◀ ECHO: ' + d.msg, 'msg-in');
      } else if (d.type === 'info') {
        clientCount.textContent = '👥 ' + d.clients + ' client';
        addLog('[INFO] Client aktif: ' + d.clients + ' | Pesan diterima: ' + d.total, 'msg-sys');
      }
    } catch(err) {
      addLog('◀ ' + e.data, 'msg-in');
    }
  };

  function sendMsg() {
    const inp = document.getElementById('msgInput');
    const msg = inp.value.trim();
    if (!msg || ws.readyState !== WebSocket.OPEN) return;
    const payload = JSON.stringify({cmd: 'echo', msg: msg});
    ws.send(payload);
    addLog('▶ KIRIM: ' + msg, 'msg-out');
    inp.value = '';
  }

  document.getElementById('msgInput').addEventListener('keydown', e => {
    if (e.key === 'Enter') sendMsg();
  });
</script>
</body>
</html>
)rawhtml";

// ── WebSocket Event Handler ───────────────────────────────────────
void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {

    case WStype_CONNECTED:
      wsClients++;
      Serial.printf("[WS] Client #%u terhubung dari %s | Total: %u\n",
        num, ws.remoteIP(num).toString().c_str(), wsClients);
      {
        // Broadcast info ke semua client: jumlah client terbaru
        char info[64];
        snprintf(info, sizeof(info),
          "{\"type\":\"info\",\"clients\":%u,\"total\":%lu}", wsClients, (unsigned long)msgCount);
        ws.broadcastTXT(info);
      }
      break;

    case WStype_DISCONNECTED:
      if (wsClients > 0) wsClients--;
      Serial.printf("[WS] Client #%u terputus | Sisa: %u\n", num, wsClients);
      {
        char info[64];
        snprintf(info, sizeof(info),
          "{\"type\":\"info\",\"clients\":%u,\"total\":%lu}", wsClients, (unsigned long)msgCount);
        ws.broadcastTXT(info);
      }
      break;

    case WStype_TEXT: {
      // Terminasi null-safety: pastikan payload berakhir null
      if (length >= 128) { ws.sendTXT(num, "{\"err\":\"payload terlalu panjang\"}"); return; }
      char buf[129];
      memcpy(buf, payload, length);
      buf[length] = '\0';

      Serial.printf("[WS] #%u → %s\n", num, buf);
      msgCount++;

      // Cari field "msg" secara manual (zero-heap pointer scan)
      const char* msgKey = "\"msg\":\"";
      const char* found  = strstr(buf, msgKey);
      char echoMsg[64]   = "(pesan tidak terbaca)";
      if (found) {
        found += strlen(msgKey);
        const char* end = strchr(found, '"');
        if (end) {
          size_t len = (size_t)(end - found);
          if (len >= sizeof(echoMsg)) len = sizeof(echoMsg) - 1;
          memcpy(echoMsg, found, len);
          echoMsg[len] = '\0';
        }
      }

      // Echo kembali ke pengirim + broadcast info ke semua
      char resp[192];
      snprintf(resp, sizeof(resp),
        "{\"type\":\"echo\",\"msg\":\"%s\"}", echoMsg);
      ws.sendTXT(num, resp);

      char info[64];
      snprintf(info, sizeof(info),
        "{\"type\":\"info\",\"clients\":%u,\"total\":%lu}", wsClients, (unsigned long)msgCount);
      ws.broadcastTXT(info);
      break;
    }

    default:
      break;
  }
}

// ── Setup ─────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== BAB 47 Program 1: WebSocket Echo Server ===");

  // WiFi
  Serial.printf("[WiFi] Menghubungkan ke '%s'...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(MDNS_NAME);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t tStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - tStart >= 15000UL) { Serial.println("\n[ERR] Timeout!"); delay(2000); ESP.restart(); }
    Serial.print(".");
    delay(500);
  }
  Serial.printf("\n[WiFi] ✅ IP: %s | SSID: %s\n",
    WiFi.localIP().toString().c_str(), WiFi.SSID().c_str());

  // mDNS
  if (MDNS.begin(MDNS_NAME)) {
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws",   "tcp", 81);
    Serial.printf("[mDNS] ✅ http://%s.local | ws://%s.local:81\n", MDNS_NAME, MDNS_NAME);
  }

  // HTTP: sajikan halaman HTML
  httpServer.on("/", HTTP_GET, []() {
    httpServer.send_P(200, "text/html", HTML_PAGE);
  });
  httpServer.onNotFound([]() {
    httpServer.send(404, "text/plain", "404 Not Found");
  });
  httpServer.begin();
  Serial.println("[HTTP] Web server aktif di port 80.");

  // WebSocket
  ws.begin();
  ws.onEvent(onWsEvent);
  Serial.println("[WS]   WebSocket server aktif di port 81.");
  Serial.printf("[INFO] Buka browser → http://%s.local\n", MDNS_NAME);
}

// ── Loop ──────────────────────────────────────────────────────────
void loop() {
  ws.loop();
  httpServer.handleClient();

  // Heartbeat setiap 30 detik
  static unsigned long tHb = 0;
  if (millis() - tHb >= 30000UL) {
    tHb = millis();
    Serial.printf("[HB] Clients: %u | Pesan: %lu | Heap: %u B | Uptime: %lu s\n",
      wsClients, (unsigned long)msgCount, ESP.getFreeHeap(), millis() / 1000UL);
  }
}
```

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 47 Program 1: WebSocket Echo Server ===
[WiFi] ✅ IP: 192.168.1.105 | SSID: WiFiRumah
[mDNS] ✅ http://bluino.local | ws://bluino.local:81
[HTTP] Web server aktif di port 80.
[WS]   WebSocket server aktif di port 81.
[WS] Client #0 terhubung dari 192.168.1.10 | Total: 1
[WS] #0 → {"cmd":"echo","msg":"Halo ESP32!"}
[WS] Client #1 terhubung dari 192.168.1.11 | Total: 2
[HB] Clients: 2 | Pesan: 5 | Heap: 228544 B | Uptime: 32 s
```

---

## 47.5 Program 2: Real-Time Sensor Broadcasting

ESP32 membaca DHT11 setiap 2 detik dan **mem-push data ke semua browser** secara otomatis — tanpa browser harus meminta. Ini adalah pola *server-side push* fundamental pada IoT.

```cpp
/*
 * BAB 47 - Program 2: Real-Time Sensor Broadcasting
 *
 * Fitur:
 *   ▶ HTTP port 80: dashboard HTML auto-update via WebSocket
 *   ▶ WS  port 81: push data DHT11 ke semua browser tiap 2 detik
 *   ▶ Zero-Client Optimization: skip JSON jika tidak ada client
 *   ▶ JS Watchdog: browser deteksi koneksi stale (silent TCP drop)
 *
 * Hardware: DHT11 → IO27 (hardwired Bluino Kit, pull-up 4K7Ω)
 * Library : "WebSockets" by Markus Sattler + DHT by Adafruit
 *
 * Cara Uji:
 *   1. Ganti WIFI_SSID dan WIFI_PASS
 *   2. Upload → buka browser: http://bluino.local
 *   3. Amati angka berubah otomatis tiap 2 detik tanpa refresh!
 *   4. Buka 2 browser → keduanya update bersamaan di detik yang sama
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <DHT.h>

const char* WIFI_SSID = "NamaWiFiKamu";   // ← Ganti!
const char* WIFI_PASS = "PasswordWiFi";   // ← Ganti!
const char* MDNS_NAME = "bluino";

#define DHT_PIN  27
#define DHT_TYPE DHT11

WebServer        httpServer(80);
WebSocketsServer ws(81);
DHT              dht(DHT_PIN, DHT_TYPE);

uint8_t wsClients = 0;
float   lastTemp  = NAN;
float   lastHumid = NAN;
bool    sensorOk  = false;

static const char HTML_PAGE[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Bluino WS — Sensor Real-Time</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body { font-family: 'Segoe UI', Arial, sans-serif; background: #0d0d1a;
           color: #e0e0e0; min-height: 100vh; padding: 20px; }
    .container { max-width: 480px; margin: 0 auto; }
    h1 { color: #4fc3f7; font-size: 1.4rem; padding-top: 20px; margin-bottom: 4px; }
    .sub { color: #555; font-size: 0.8rem; margin-bottom: 20px; }
    .status-row { display: flex; gap: 10px; margin-bottom: 16px; }
    .badge { padding: 5px 12px; border-radius: 16px; font-size: 0.78rem; font-weight: bold; }
    .b-ws  { background: #1b5e20; color: #81c784; }
    .b-cli { background: #0d47a1; color: #90caf9; }
    .b-disc{ background: #b71c1c; color: #ef9a9a; }
    .grid  { display: grid; grid-template-columns: 1fr 1fr; gap: 12px; margin-bottom: 16px; }
    .card  { background: #1a1a2e; border-radius: 14px; padding: 24px; text-align: center; }
    .lbl   { color: #666; font-size: 0.72rem; text-transform: uppercase; margin-bottom: 8px; }
    .val   { font-size: 2.4rem; font-weight: bold; }
    .unit  { font-size: 0.85rem; color: #888; }
    .tc    { color: #4fc3f7; } .tw { color: #ff9800; } .th { color: #ef5350; }
    .hc    { color: #26c6da; }
    .inf   { background: #1a1a2e; border-radius: 12px; padding: 16px; margin-bottom: 12px; }
    .ir    { display: flex; justify-content: space-between; padding: 5px 0;
             border-bottom: 1px solid #1e2a3a; font-size: 0.82rem; }
    .ir:last-child { border: none; }
    .ik { color: #888; } .iv { color: #eee; font-weight: bold; }
    .err { color: #ef5350; font-size: 0.8rem; text-align: center; padding: 8px;
           background: #1a0000; border-radius: 8px; margin-bottom: 12px; }
    .footer { text-align: center; color: #333; font-size: 0.7rem; margin-top: 16px; }
  </style>
</head>
<body>
<div class="container">
  <h1>🌡️ Sensor Real-Time</h1>
  <p class="sub">BAB 47 Program 2 — Server-Side Push via WebSocket</p>
  <div class="status-row">
    <span class="badge b-ws"  id="wsStat">⚫ Menghubungkan...</span>
    <span class="badge b-cli" id="cliStat">👥 0 client</span>
  </div>
  <div id="errBanner" class="err" style="display:none">❌ Sensor Error — Cek kabel DHT11 IO27</div>
  <div class="grid">
    <div class="card"><div class="lbl">🌡️ Suhu</div>
      <div class="val tc" id="temp">--</div><div class="unit">°C</div></div>
    <div class="card"><div class="lbl">💧 Kelembaban</div>
      <div class="val hc"  id="hum">--</div><div class="unit">%</div></div>
  </div>
  <div class="inf">
    <div class="ir"><span class="ik">RSSI WiFi</span><span class="iv" id="rssi">--</span></div>
    <div class="ir"><span class="ik">Heap Bebas</span><span class="iv" id="heap">--</span></div>
    <div class="ir"><span class="ik">Uptime</span><span class="iv" id="upt">--</span></div>
    <div class="ir"><span class="ik">Update terakhir</span><span class="iv" id="ts">--</span></div>
  </div>
  <p class="footer">Bluino IoT Kit 2026 — BAB 47: WebSocket Push</p>
</div>
<script>
  let staleTimer;
  const wsStat = document.getElementById('wsStat');
  const cliStat = document.getElementById('cliStat');

  function resetStale() {
    clearTimeout(staleTimer);
    staleTimer = setTimeout(() => {
      wsStat.textContent = '🟡 Data Stale';
      wsStat.style.background = '#e65100';
    }, 6000);
  }

  const sock = new WebSocket('ws://' + location.hostname + ':81');

  sock.onopen = () => {
    wsStat.textContent = '🟢 Terhubung';
    wsStat.className = 'badge b-ws';
    resetStale();
  };

  sock.onclose = () => {
    wsStat.textContent = '🔴 Terputus';
    wsStat.className = 'badge b-disc';
    clearTimeout(staleTimer);
    setTimeout(() => location.reload(), 3000);
  };

  sock.onmessage = (e) => {
    resetStale();
    try {
      const d = JSON.parse(e.data);
      if (d.type !== 'sensors') return;
      const tEl = document.getElementById('temp');
      if (d.sensorOk && d.temp !== null) {
        tEl.textContent = d.temp.toFixed(1);
        tEl.className = 'val ' + (d.temp > 35 ? 'th' : d.temp > 28 ? 'tw' : 'tc');
      } else { tEl.textContent = '--'; }
      document.getElementById('hum').textContent  = (d.sensorOk && d.humidity !== null) ? Math.round(d.humidity) : '--';
      document.getElementById('rssi').textContent = d.rssi + ' dBm';
      document.getElementById('heap').textContent = Math.round(d.freeHeap/1024) + ' KB';
      const s=d.uptime; const h=Math.floor(s/3600); const m=Math.floor((s%3600)/60); const sc=s%60;
      document.getElementById('upt').textContent = h+'j '+m+'m '+sc+'d';
      document.getElementById('ts').textContent  = new Date().toLocaleTimeString('id-ID');
      cliStat.textContent = '👥 ' + d.clients + ' client';
      document.getElementById('errBanner').style.display = d.sensorOk ? 'none' : 'block';
    } catch(err) {}
  };
</script>
</body>
</html>
)rawhtml";

// ── WebSocket Event ───────────────────────────────────────────────
void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  (void)payload; (void)length;
  if (type == WStype_CONNECTED) {
    wsClients++;
    Serial.printf("[WS] #%u terhubung dari %s | Total: %u\n",
      num, ws.remoteIP(num).toString().c_str(), wsClients);
  } else if (type == WStype_DISCONNECTED) {
    if (wsClients > 0) wsClients--;
    Serial.printf("[WS] #%u terputus | Sisa: %u\n", num, wsClients);
  }
}

// ── Broadcast JSON sensor ke semua browser ────────────────────────
void broadcastSensors() {
  if (wsClients == 0) return; // Zero-Client Optimization

  char tStr[8] = "null", hStr[8] = "null";
  if (sensorOk && !isnan(lastTemp))  snprintf(tStr, sizeof(tStr), "%.1f", lastTemp);
  if (sensorOk && !isnan(lastHumid)) snprintf(hStr, sizeof(hStr), "%.0f", lastHumid);

  char json[192];
  snprintf(json, sizeof(json),
    "{\"type\":\"sensors\",\"temp\":%s,\"humidity\":%s,"
    "\"rssi\":%d,\"uptime\":%lu,\"freeHeap\":%u,"
    "\"sensorOk\":%s,\"clients\":%u}",
    tStr, hStr, WiFi.RSSI(), millis()/1000UL,
    ESP.getFreeHeap(), sensorOk ? "true" : "false", wsClients);

  ws.broadcastTXT(json);
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  delay(1000);

  Serial.println("\n=== BAB 47 Program 2: Real-Time Sensor Broadcasting ===");
  Serial.printf("[WiFi] Menghubungkan ke '%s'...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA); WiFi.setHostname(MDNS_NAME);
  WiFi.setAutoReconnect(true); WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t tStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - tStart >= 15000UL) { delay(2000); ESP.restart(); }
    Serial.print("."); delay(500);
  }
  Serial.printf("\n[WiFi] ✅ Terhubung! IP: %s | SSID: %s\n",
    WiFi.localIP().toString().c_str(), WiFi.SSID().c_str());

  if (MDNS.begin(MDNS_NAME)) {
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws",   "tcp", 81);
    Serial.printf("[mDNS] ✅ http://%s.local\n", MDNS_NAME);
  }

  httpServer.on("/", HTTP_GET, []() { httpServer.send_P(200, "text/html", HTML_PAGE); });
  httpServer.onNotFound([]() { httpServer.send(404, "text/plain", "404"); });
  httpServer.begin();
  Serial.println("[HTTP] Server aktif di port 80.");

  ws.begin(); ws.onEvent(onWsEvent);
  Serial.println("[WS]   Server aktif di port 81.");
  Serial.printf("[INFO] Dashboard → http://%s.local\n", MDNS_NAME);
}

void loop() {
  ws.loop();
  httpServer.handleClient();

  // Baca DHT11 non-blocking setiap 2 detik
  static unsigned long tDHT = 0;
  if (millis() - tDHT >= 2000UL) {
    tDHT = millis();
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    bool  ok = (!isnan(t) && !isnan(h));
    if ( ok && !sensorOk) Serial.println("[DHT] ✅ Sensor pulih!");
    if (!ok &&  sensorOk) Serial.println("[DHT] ⚠️ Gagal baca! Cek wiring IO27.");
    if (ok) { lastTemp = t; lastHumid = h; }
    sensorOk = ok;
    broadcastSensors();
  }

  // Heartbeat Serial setiap 30 detik
  static unsigned long tHb = 0;
  if (millis() - tHb >= 30000UL) {
    tHb = millis();
    Serial.printf("[HB] %.1fC %.0f%% | Clients: %u | Heap: %u B\n",
      sensorOk ? lastTemp : 0.0f, sensorOk ? lastHumid : 0.0f,
      wsClients, ESP.getFreeHeap());
  }
}
```

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 47 Program 2: Real-Time Sensor Broadcasting ===
[WiFi] ✅ Terhubung! IP: 192.168.1.105 | SSID: WiFiRumah
[mDNS] ✅ http://bluino.local
[HTTP] Server aktif di port 80.
[WS]   Server aktif di port 81.
[WS] #0 terhubung dari 192.168.1.10 | Total: 1
[DHT] ⚠️ Gagal baca! Cek wiring IO27.
[DHT] ✅ Sensor pulih!
[HB] 28.5C 65% | Clients: 2 | Heap: 212480 B
```

> 💡 **Zero-Client Optimization:** `if (wsClients == 0) return;` — ESP32 tidak merakit JSON sama sekali jika tidak ada browser yang terhubung. Hemat CPU dan RAM!

---

## 47.6 Program 3: Dashboard IoT Real-Time Interaktif

Program puncak BAB 47: dashboard dua arah penuh — ESP32 push sensor tiap 2 detik, browser kontrol LED dan Relay secara instan, state tersimpan di NVS.

```cpp
/*
 * BAB 47 - Program 3: Dashboard IoT Real-Time Interaktif
 *
 * Fitur:
 *   ▶ Push data sensor (suhu, kelembaban, RSSI) tiap 2 detik
 *   ▶ Kontrol LED (IO2) dan Relay (IO4) via WebSocket
 *   ▶ Broadcast ACK: semua browser sinkron otomatis
 *   ▶ NVS Persistence: state relay diingat setelah reboot
 *   ▶ Zero-Heap JSON Parser: pointer scan tanpa String fragmentation
 *   ▶ Serial CLI: help | status | led on/off | relay on/off | restart
 *
 * Hardware:
 *   DHT11  → IO27  (hardwired Bluino Kit)
 *   LED    → IO2   (built-in LED, active HIGH)
 *   Relay  → IO4   (kabel jumper dari header pin ke relay board)
 *
 * Library: WebSockets by Markus Sattler, DHT by Adafruit
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Preferences.h>
#include <DHT.h>

const char* WIFI_SSID = "NamaWiFiKamu";  // <- Ganti!
const char* WIFI_PASS = "PasswordWiFi";  // <- Ganti!
const char* MDNS_NAME = "bluino";

#define LED_PIN   2
#define RELAY_PIN 4
#define DHT_PIN   27
#define DHT_TYPE  DHT11

WebServer        httpServer(80);
WebSocketsServer ws(81);
Preferences      prefs;
DHT              dht(DHT_PIN, DHT_TYPE);

uint8_t  wsClients  = 0;
bool     ledState   = false;
bool     relayState = false;
float    lastTemp   = NAN;
float    lastHumid  = NAN;
bool     sensorOk   = false;

// ── HTML: satu halaman, embedded di Flash ────────────────────────
static const char HTML_PAGE[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Bluino — Dashboard IoT</title>
  <style>
    *{box-sizing:border-box;margin:0;padding:0}
    body{font-family:'Segoe UI',Arial,sans-serif;background:#0d0d1a;
         color:#e0e0e0;min-height:100vh;padding:20px}
    .con{max-width:500px;margin:0 auto}
    h1{color:#4fc3f7;font-size:1.4rem;padding-top:20px;margin-bottom:4px}
    .sub{color:#555;font-size:.8rem;margin-bottom:20px}
    .sr{display:flex;gap:8px;margin-bottom:16px;flex-wrap:wrap}
    .badge{padding:5px 12px;border-radius:16px;font-size:.76rem;font-weight:700}
    .bw{background:#1b5e20;color:#81c784}.bd{background:#b71c1c;color:#ef9a9a}
    .bc{background:#0d47a1;color:#90caf9}
    .grid{display:grid;grid-template-columns:1fr 1fr;gap:10px;margin-bottom:14px}
    .card{background:#1a1a2e;border-radius:12px;padding:20px;text-align:center}
    .lbl{color:#666;font-size:.7rem;text-transform:uppercase;margin-bottom:6px}
    .val{font-size:2.2rem;font-weight:700}.unit{font-size:.82rem;color:#888}
    .tc{color:#4fc3f7}.tw{color:#ff9800}.th{color:#ef5350}.hc{color:#26c6da}
    .ctrl{background:#1a1a2e;border-radius:12px;padding:16px;margin-bottom:14px}
    .ctrl-row{display:flex;justify-content:space-between;align-items:center;
              padding:10px 0;border-bottom:1px solid #1e2a3a}
    .ctrl-row:last-child{border:none}
    .ctrl-lbl{font-size:.88rem;color:#bbb}
    .btn{padding:8px 20px;border-radius:8px;border:none;cursor:pointer;
         font-size:.82rem;font-weight:700;transition:all .15s}
    .btn-on {background:#1b5e20;color:#81c784}
    .btn-off{background:#37474f;color:#90a4ae}
    .btn:hover{filter:brightness(1.2)}
    .inf{background:#1a1a2e;border-radius:12px;padding:14px;margin-bottom:12px}
    .ir{display:flex;justify-content:space-between;padding:5px 0;
        border-bottom:1px solid #1e2a3a;font-size:.82rem}
    .ir:last-child{border:none}.ik{color:#888}.iv{color:#eee;font-weight:700}
    .footer{text-align:center;color:#333;font-size:.7rem;margin-top:14px}
  </style>
</head>
<body>
<div class="con">
  <h1>⚡ Dashboard IoT Real-Time</h1>
  <p class="sub">BAB 47 Program 3 — Bluino IoT Kit</p>
  <div class="sr">
    <span class="badge bw" id="wsStat">⚫ Menghubungkan...</span>
    <span class="badge bc" id="cliStat">👥 0 client</span>
  </div>
  <div class="grid">
    <div class="card"><div class="lbl">🌡️ Suhu</div>
      <div class="val tc" id="temp">--</div><div class="unit">°C</div></div>
    <div class="card"><div class="lbl">💧 Kelembaban</div>
      <div class="val hc" id="hum">--</div><div class="unit">%</div></div>
  </div>
  <div class="ctrl">
    <div class="ctrl-row">
      <span class="ctrl-lbl">💡 LED (IO2)</span>
      <button class="btn btn-off" id="btnLed" onclick="toggle('led')">OFF</button>
    </div>
    <div class="ctrl-row">
      <span class="ctrl-lbl">🔌 Relay (IO4)</span>
      <button class="btn btn-off" id="btnRelay" onclick="toggle('relay')">OFF</button>
    </div>
  </div>
  <div class="inf">
    <div class="ir"><span class="ik">RSSI</span><span class="iv" id="rssi">--</span></div>
    <div class="ir"><span class="ik">Heap</span><span class="iv" id="heap">--</span></div>
    <div class="ir"><span class="ik">Uptime</span><span class="iv" id="upt">--</span></div>
    <div class="ir"><span class="ik">WS Clients</span><span class="iv" id="cli2">--</span></div>
  </div>
  <p class="footer">Bluino IoT Kit 2026 — BAB 47: WebSocket Dashboard</p>
</div>
<script>
  const wsStat=document.getElementById('wsStat');
  const cliStat=document.getElementById('cliStat');
  let staleTimer;

  function resetStale(){
    clearTimeout(staleTimer);
    staleTimer=setTimeout(()=>{wsStat.textContent='🟡 Stale';},6000);
  }

  function setBtn(id,state){
    const b=document.getElementById(id);
    b.textContent=state?'ON':'OFF';
    b.className='btn '+(state?'btn-on':'btn-off');
  }

  function toggle(cmd){
    // Optimistic UI: update button sebelum konfirmasi server
    const btnId=cmd==='led'?'btnLed':'btnRelay';
    const cur=document.getElementById(btnId).textContent==='ON';
    setBtn(btnId,!cur);
    sock.send(JSON.stringify({cmd:cmd,state:cur?0:1}));
  }

  const sock=new WebSocket('ws://'+location.hostname+':81');

  sock.onopen=()=>{
    wsStat.textContent='🟢 Terhubung';
    wsStat.className='badge bw';
    resetStale();
  };
  sock.onclose=()=>{
    wsStat.textContent='🔴 Terputus';
    wsStat.className='badge bd';
    clearTimeout(staleTimer);
    setTimeout(()=>location.reload(),3000);
  };
  sock.onmessage=(e)=>{
    resetStale();
    try{
      const d=JSON.parse(e.data);
      if(d.type==='sensors'){
        const tEl=document.getElementById('temp');
        if(d.sensorOk&&d.temp!==null){
          tEl.textContent=d.temp.toFixed(1);
          tEl.className='val '+(d.temp>35?'th':d.temp>28?'tw':'tc');
        } else tEl.textContent='--';
        document.getElementById('hum').textContent=(d.sensorOk&&d.humidity!==null)?Math.round(d.humidity):'--';
        document.getElementById('rssi').textContent=d.rssi+' dBm';
        document.getElementById('heap').textContent=Math.round(d.freeHeap/1024)+' KB';
        const s=d.uptime;
        document.getElementById('upt').textContent=Math.floor(s/3600)+'j '+Math.floor((s%3600)/60)+'m '+s%60+'d';
        document.getElementById('cli2').textContent=d.clients+' client';
        cliStat.textContent='👥 '+d.clients+' client';
        setBtn('btnLed',d.ledState);
        setBtn('btnRelay',d.relayState);
      } else if(d.type==='ack'){
        // Sinkron semua browser saat state berubah
        if(d.cmd==='led')   setBtn('btnLed',  d.state);
        if(d.cmd==='relay') setBtn('btnRelay', d.state);
      }
    }catch(err){}
  };
</script>
</body>
</html>
)rawhtml";

// ── Helper: ekstrak field string dari JSON (zero-heap) ────────────
bool getStrField(const char* json, const char* key, char* out, size_t maxLen) {
  char search[32];
  snprintf(search, sizeof(search), "\"%s\":\"", key);
  const char* p = strstr(json, search);
  if (!p) return false;
  p += strlen(search);
  const char* e = strchr(p, '"');
  if (!e) return false;
  size_t len = (size_t)(e - p);
  if (len >= maxLen) len = maxLen - 1;
  memcpy(out, p, len);
  out[len] = '\0';
  return true;
}

bool getIntField(const char* json, const char* key, int* out) {
  char search[32];
  snprintf(search, sizeof(search), "\"%s\":", key);
  const char* p = strstr(json, search);
  if (!p) return false;
  p += strlen(search);
  *out = atoi(p);
  return true;
}

// ── Broadcast sensor JSON ke semua browser ────────────────────────
void broadcastSensors() {
  if (wsClients == 0) return;
  char tStr[8]="null", hStr[8]="null";
  if (sensorOk && !isnan(lastTemp))  snprintf(tStr, sizeof(tStr), "%.1f", lastTemp);
  if (sensorOk && !isnan(lastHumid)) snprintf(hStr, sizeof(hStr), "%.0f", lastHumid);
  char json[256];
  snprintf(json, sizeof(json),
    "{\"type\":\"sensors\",\"temp\":%s,\"humidity\":%s,"
    "\"rssi\":%d,\"uptime\":%lu,\"freeHeap\":%u,"
    "\"sensorOk\":%s,\"ledState\":%s,\"relayState\":%s,\"clients\":%u}",
    tStr, hStr, WiFi.RSSI(), millis()/1000UL, ESP.getFreeHeap(),
    sensorOk?"true":"false", ledState?"true":"false",
    relayState?"true":"false", wsClients);
  ws.broadcastTXT(json);
}

// ── WebSocket Event Handler ───────────────────────────────────────
void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  if (type == WStype_CONNECTED) {
    wsClients++;
    Serial.printf("[WS] #%u terhubung dari %s | Total: %u\n",
      num, ws.remoteIP(num).toString().c_str(), wsClients);
    // Kirim state awal ke client baru
    broadcastSensors();
  } else if (type == WStype_DISCONNECTED) {
    if (wsClients > 0) wsClients--;
    Serial.printf("[WS] #%u terputus | Sisa: %u\n", num, wsClients);
  } else if (type == WStype_TEXT) {
    if (length >= 128) return;
    char buf[129];
    memcpy(buf, payload, length);
    buf[length] = '\0';
    Serial.printf("[WS] #%u -> %s\n", num, buf);

    char cmd[16] = "";
    int  state   = 0;
    getStrField(buf, "cmd", cmd, sizeof(cmd));
    getIntField(buf, "state", &state);

    char ack[64];
    if (strcmp(cmd, "led") == 0) {
      ledState = (state != 0);
      digitalWrite(LED_PIN, ledState ? HIGH : LOW);
      Serial.printf("[WS] LED %s via WebSocket\n", ledState ? "ON" : "OFF");
      snprintf(ack, sizeof(ack),
        "{\"type\":\"ack\",\"cmd\":\"led\",\"state\":%s}",
        ledState ? "true" : "false");
      ws.broadcastTXT(ack);
    } else if (strcmp(cmd, "relay") == 0) {
      relayState = (state != 0);
      digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
      // Simpan ke NVS
      prefs.begin("dash", false);
      prefs.putBool("relay", relayState);
      prefs.end();
      Serial.printf("[WS] Relay %s via WebSocket -- disimpan NVS\n", relayState ? "ON" : "OFF");
      snprintf(ack, sizeof(ack),
        "{\"type\":\"ack\",\"cmd\":\"relay\",\"state\":%s}",
        relayState ? "true" : "false");
      ws.broadcastTXT(ack);
    } else if (strcmp(cmd, "status") == 0) {
      broadcastSensors();
    }
  }
}

// ── Serial CLI ────────────────────────────────────────────────────
static char cliBuf[80];
static uint8_t cliLen = 0;

void handleCLI(const char* cmd) {
  if      (strcmp(cmd, "help")     == 0) {
    Serial.println("Perintah: status | led on/off | relay on/off | restart");
  } else if (strcmp(cmd, "status") == 0) {
    Serial.printf("  LED: %s | Relay: %s | WS: %u | Suhu: %.1f | Heap: %u\n",
      ledState?"ON":"OFF", relayState?"ON":"OFF", wsClients, lastTemp, ESP.getFreeHeap());
  } else if (strcmp(cmd, "led on")  == 0) {
    ledState=true;  digitalWrite(LED_PIN,HIGH);
    char ack[48]; snprintf(ack,sizeof(ack),"{\"type\":\"ack\",\"cmd\":\"led\",\"state\":true}");
    ws.broadcastTXT(ack); // Sinkron semua browser
  } else if (strcmp(cmd, "led off") == 0) {
    ledState=false; digitalWrite(LED_PIN,LOW);
    char ack[48]; snprintf(ack,sizeof(ack),"{\"type\":\"ack\",\"cmd\":\"led\",\"state\":false}");
    ws.broadcastTXT(ack);
  } else if (strcmp(cmd, "relay on")  == 0) {
    relayState=true; digitalWrite(RELAY_PIN,HIGH);
    prefs.begin("dash",false); prefs.putBool("relay",true); prefs.end();
    char ack[48]; snprintf(ack,sizeof(ack),"{\"type\":\"ack\",\"cmd\":\"relay\",\"state\":true}");
    ws.broadcastTXT(ack);
  } else if (strcmp(cmd, "relay off") == 0) {
    relayState=false; digitalWrite(RELAY_PIN,LOW);
    prefs.begin("dash",false); prefs.putBool("relay",false); prefs.end();
    char ack[48]; snprintf(ack,sizeof(ack),"{\"type\":\"ack\",\"cmd\":\"relay\",\"state\":false}");
    ws.broadcastTXT(ack);
  } else if (strcmp(cmd, "restart") == 0) { ESP.restart(); }
  else { Serial.printf("[CLI] Tidak dikenal: '%s'\n", cmd); }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN,   OUTPUT); digitalWrite(LED_PIN,   LOW);
  pinMode(RELAY_PIN, OUTPUT); digitalWrite(RELAY_PIN, LOW);
  dht.begin();
  delay(1000);

  Serial.println("\n=== BAB 47 Program 3: Dashboard IoT Real-Time Interaktif ===");
  Serial.println("[INFO] Ketik 'help' di Serial Monitor.");
  Serial.printf("[INFO] Relay PIN: IO%d\n", RELAY_PIN);

  // Muat state relay dari NVS
  prefs.begin("dash", true);
  relayState = prefs.getBool("relay", false);
  prefs.end();
  digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
  Serial.printf("[NVS] Relay state dimuat: %s\n", relayState ? "ON" : "OFF");

  Serial.printf("[WiFi] Menghubungkan ke '%s'...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA); WiFi.setHostname(MDNS_NAME);
  WiFi.setAutoReconnect(true); WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t tStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - tStart >= 15000UL) { delay(2000); ESP.restart(); }
    Serial.print("."); delay(500);
  }
  Serial.printf("\n[WiFi] Terhubung! IP: %s\n", WiFi.localIP().toString().c_str());

  if (MDNS.begin(MDNS_NAME)) {
    MDNS.addService("http","tcp",80); MDNS.addService("ws","tcp",81);
    Serial.printf("[mDNS] http://%s.local\n", MDNS_NAME);
  }

  httpServer.on("/", HTTP_GET, [](){httpServer.send_P(200,"text/html",HTML_PAGE);});
  httpServer.onNotFound([](){httpServer.send(404,"text/plain","404");});
  httpServer.begin(); Serial.println("[HTTP] Port 80 aktif.");

  ws.begin(); ws.onEvent(onWsEvent);
  Serial.println("[WS]   Port 81 aktif.");
  Serial.printf("\n  Dashboard : http://%s.local\n  WebSocket : ws://%s.local:81\n\n",
    MDNS_NAME, MDNS_NAME);
}

void loop() {
  ws.loop();
  httpServer.handleClient();

  // Baca DHT11 + broadcast setiap 2 detik
  static unsigned long tDHT = 0;
  if (millis() - tDHT >= 2000UL) {
    tDHT = millis();
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    bool  ok = (!isnan(t) && !isnan(h));
    if ( ok && !sensorOk) Serial.println("[DHT] ✅ Sensor pulih!");
    if (!ok &&  sensorOk) Serial.println("[DHT] ⚠️ Gagal baca! Cek wiring IO27.");
    if (ok)  { lastTemp=t; lastHumid=h; }
    sensorOk = ok;
    broadcastSensors();
  }

  // Serial CLI
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c=='\n'||c=='\r') {
      if (cliLen > 0) { cliBuf[cliLen]='\0'; Serial.printf("> %s\n",cliBuf); handleCLI(cliBuf); cliLen=0; }
    } else if (cliLen < sizeof(cliBuf)-1) { cliBuf[cliLen++]=c; }
  }

  // Heartbeat setiap 30 detik
  static unsigned long tHb = 0;
  if (millis() - tHb >= 30000UL) {
    tHb = millis();
    Serial.printf("[HB] %.1fC %.0f%% LED:%s Relay:%s WS:%u Heap:%u B\n",
      sensorOk?lastTemp:0.0f, sensorOk?lastHumid:0.0f,
      ledState?"ON":"OFF", relayState?"ON":"OFF",
      wsClients, ESP.getFreeHeap());
  }
}
```

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 47 Program 3: Dashboard IoT Real-Time Interaktif ===
[NVS] Relay state dimuat: OFF
[WiFi] Terhubung! IP: 192.168.1.105
[mDNS] http://bluino.local
[HTTP] Port 80 aktif.
[WS]   Port 81 aktif.
[WS] #0 terhubung dari 192.168.1.10 | Total: 1
[WS] #0 -> {"cmd":"led","state":1}
[WS] LED ON via WebSocket
[HB] 28.5C 65% LED:ON Relay:OFF WS:1 Heap:191240 B
```

---

## 47.7 Perbandingan BAB 46 vs BAB 47

| Aspek | BAB 46 (HTTP Polling) | BAB 47 (WebSocket Push) |
|-------|----------------------|------------------------|
| **Protokol** | HTTP request/response | WS persistent TCP |
| **Update data** | Tiap 5 detik (polling) | Push instan saat siap |
| **Header overhead** | ~500 bytes/request | 2–14 bytes/frame |
| **5 browser aktif** | 5× request/5 detik | 1× broadcast/2 detik |
| **Latensi kontrol** | + waktu HTTP roundtrip | < 10ms |
| **Inisiator data** | Client (browser) | Server (ESP32) |
| **Multi-browser sync** | Tidak sinkron | Otomatis — broadcast ACK |
| **Library** | Bawaan ESP32 Core | Install: WebSockets by Markus Sattler |

---

## 47.8 Ringkasan

```
RINGKASAN BAB 47 — WebSocket ESP32

API ESENSIAL:
  ws.begin()              → Mulai WebSocket server di port yang dipilih
  ws.loop()               → Proses frame WAJIB dipanggil di loop()
  ws.onEvent(cb)          → Daftarkan event handler
  ws.sendTXT(num, data)   → Kirim ke client tertentu (unicast)
  ws.broadcastTXT(data)   → Kirim ke SEMUA client (broadcast)

PATTERN TERBUKTI:
  ✅ Broadcast setiap N detik (push sensor)
  ✅ Broadcast ACK setelah menerima perintah (sinkronisasi UI)
  ✅ Zero-Client Check: if (wsClients==0) return; sebelum JSON build
  ✅ JS Watchdog: setTimeout 6 detik → deteksi silent TCP drop
  ✅ Auto-reconnect: sock.onclose -> setTimeout location.reload()
  ✅ JSON Zero-Heap: snprintf ke stack char[], bukan String()

ANTIPATTERN:
  ❌ delay() di dalam event callback WebSocket → freeze koneksi
  ❌ String() object untuk JSON build → heap fragmentation
  ❌ Lupa ws.loop() di loop() → client tidak pernah diproses
  ❌ Tidak broadcast ACK → browser lain tidak sinkron
```

---

## 47.9 Latihan

### Latihan Dasar
1. Upload **Program 1**, kirim pesan dari 2 browser. Berapa Client ID masing-masing browser?
2. Upload **Program 2**, tiup hangat ke DHT11. Berapa detik sebelum perubahan suhu muncul di browser?
3. Bandingkan Program 2 dengan BAB 46: delay update polling vs push — berapa perbedaannya?

### Latihan Menengah
4. Upload **Program 3**, tekan LED dari browser A — berapa milidetik sebelum browser B juga update?
5. Uji NVS: ketik `relay on` di Serial CLI, power off ESP32, colok lagi — apakah relay langsung aktif?
6. Modifikasi Program 2: tambahkan field `"ldr": analogRead(35)` ke JSON broadcast.

### Tantangan Lanjutan
7. **Alert Suhu**: jika suhu > 35°C, kirim `{"type":"alert","msg":"Suhu kritis!"}` ke semua browser. Tampilkan popup merah di dashboard.
8. **Timestamp**: tambahkan `"ts": millis()` ke broadcast. Hitung di JS berapa ms antar dua frame — ini adalah latency aktual WebSocket.
9. **Grafik Real-Time**: gunakan library Chart.js (CDN) untuk menampilkan grafik suhu bergulir 60 detik terakhir di browser.

---

## 47.10 Referensi

| Sumber | Link |
|--------|------|
| arduinoWebSockets by Markus Sattler | https://github.com/Links2004/arduinoWebSockets |
| RFC 6455 — WebSocket Protocol | https://datatracker.ietf.org/doc/html/rfc6455 |
| MDN WebSocket API | https://developer.mozilla.org/en-US/docs/Web/API/WebSocket |

---

## ☕ Dukung Proyek Ini

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

---

*Sebelumnya → [BAB 46: Web Server](../BAB-46/README.md)*

*Selanjutnya → [BAB 48: HTTP Client & REST API](../BAB-48/README.md)*
