/*
 * BAB 47 - Program 1: WebSocket Echo Server
 * ──────────────────────────────────────────
 * Fitur:
 *   ▶ HTTP port 80 → Halaman HTML dengan form kirim pesan
 *   ▶ WS   port 81 → Echo balik setiap pesan yang diterima
 *   ▶ Multi-client: bisa dibuka dari 2+ browser sekaligus
 *   ▶ LED IO2 berkedip saat pesan diterima
 *   ▶ Serial Monitor menampilkan semua event (connect/msg/disconnect)
 *
 * Cara Uji:
 *   1. Install library: Arduino IDE → Library Manager → "WebSockets" by Markus Sattler
 *   2. Ganti WIFI_SSID dan WIFI_PASS di bawah
 *   3. Upload, buka Serial Monitor (115200 baud)
 *   4. Browser: http://bluino.local (atau IP dari Serial Monitor)
 *   5. Ketik pesan, tekan Enter atau klik Kirim
 *   6. Buka browser kedua — amati jumlah client berubah
 *
 * Library yang dibutuhkan:
 *   - "WebSockets" by Markus Sattler >= 2.4.0  (install manual)
 *   - WiFi.h, ESPmDNS.h, WebServer.h           (bawaan ESP32 Core)
 *
 * ⚠️ ESP32 hanya mendukung WiFi 2.4 GHz!
 * ⚠️ Ganti SSID dan PASSWORD sesuai jaringan WiFi kamu!
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

// ── Konfigurasi ───────────────────────────────────────────────────
const char* WIFI_SSID = "NamaWiFiKamu";  // ← Ganti!
const char* WIFI_PASS = "PasswordWiFi";  // ← Ganti!
const char* MDNS_NAME = "bluino";

// ── Pin ──────────────────────────────────────────────────────────
#define LED_PIN 2   // LED built-in (active HIGH)

// ── Objek ────────────────────────────────────────────────────────
WebServer        server(80);  // HTTP di port 80 → melayani halaman HTML
WebSocketsServer ws(81);      // WebSocket di port 81 → komunikasi real-time

// ── State ────────────────────────────────────────────────────────
uint8_t  wsClients  = 0;    // Jumlah browser yang sedang terhubung
uint32_t msgTotal   = 0;    // Akumulasi pesan diterima sejak boot
uint32_t blinkTimer = 0;    // Timer non-blocking LED (anti-delay)
bool     isBlinking = false;

// ─────────────────────────────────────────────────────────────────
// Halaman HTML (PROGMEM → disimpan di Flash, tidak memakan heap)
// ─────────────────────────────────────────────────────────────────
const char PAGE_HTML[] PROGMEM = R"rawhtml(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Bluino IoT — WebSocket Echo</title>
  <style>
    *{box-sizing:border-box;margin:0;padding:0}
    body{font-family:'Segoe UI',Arial,sans-serif;background:#0a0a14;
         color:#e0e0e0;min-height:100vh;padding:16px}
    .c{max-width:520px;margin:0 auto}
    h1{color:#4fc3f7;font-size:1.3rem;padding:16px 0 4px}
    .sub{color:#444;font-size:.78rem;margin-bottom:16px}
    .badge{display:inline-block;padding:3px 10px;border-radius:16px;
           font-size:.75rem;font-weight:600;margin-bottom:14px}
    .conn{background:#1b5e20;color:#81c784}
    .disc{background:#4a0000;color:#ef9a9a}
    .card{background:#141424;border-radius:12px;padding:16px;margin-bottom:10px}
    .sec{color:#555;font-size:.7rem;text-transform:uppercase;
         letter-spacing:1px;margin-bottom:8px}
    #log{height:210px;overflow-y:auto;font-family:monospace;font-size:.78rem;line-height:1.7}
    .in{color:#81c784}.out{color:#4fc3f7}.sys{color:#555}.err{color:#ef5350}
    .row{display:flex;gap:8px;margin-top:10px}
    input{flex:1;background:#0d0d1a;border:1px solid #1e2a3a;border-radius:8px;
          padding:10px 12px;color:#e0e0e0;font-size:.85rem;outline:none}
    input:focus{border-color:#4fc3f7}
    button{background:#0f3460;color:#4fc3f7;border:none;border-radius:8px;
           padding:10px 18px;font-size:.82rem;font-weight:600;cursor:pointer;
           user-select:none;-webkit-tap-highlight-color:transparent}
    button:hover{background:#1565c0}
    button:active{transform:scale(.95)}
    .g2{display:grid;grid-template-columns:1fr 1fr;gap:8px}
    .stat{background:#0d0d1a;border-radius:8px;padding:10px;text-align:center}
    .sv{font-size:1.4rem;font-weight:700;color:#4fc3f7}
    .sl{font-size:.68rem;color:#555}
  </style>
</head>
<body><div class="c">
  <h1>&#128311; WebSocket Echo</h1>
  <p class="sub">BAB 47 &#183; Bluino IoT Kit &#8212; Pesan dikirim &amp; di-echo ESP32 secara instan</p>
  <span id="st" class="badge disc">&#9679; Menghubungkan...</span>

  <div class="card">
    <div class="sec">Log Percakapan</div>
    <div id="log"></div>
    <div class="row">
      <input id="inp" type="text" placeholder="Ketik pesan, tekan Enter..." maxlength="120">
      <button onclick="kirim()">Kirim</button>
    </div>
  </div>

  <div class="card">
    <div class="sec">Statistik Sesi</div>
    <div class="g2">
      <div class="stat"><div class="sv" id="sc">0</div><div class="sl">Client Aktif</div></div>
      <div class="stat"><div class="sv" id="mc">0</div><div class="sl">Total Pesan</div></div>
    </div>
  </div>
</div>
<script>
let ws;
const log = document.getElementById('log');
const inp = document.getElementById('inp');

function addLog(txt, cls) {
  const d = document.createElement('div');
  d.className = cls;
  d.textContent = new Date().toLocaleTimeString('id') + '  ' + txt;
  log.appendChild(d);
  log.scrollTop = log.scrollHeight;
}

function setSt(ok) {
  const s = document.getElementById('st');
  s.textContent = ok ? '\u25cf Terhubung' : '\u25cf Terputus';
  s.className = 'badge ' + (ok ? 'conn' : 'disc');
}

function connect() {
  ws = new WebSocket('ws://' + location.hostname + ':81/');
  ws.onopen  = () => { setSt(true);  addLog('WebSocket terhubung ke ESP32!', 'sys'); };
  ws.onerror = () => addLog('Koneksi error.', 'err');
  ws.onclose = () => {
    setSt(false);
    addLog('Terputus \u2014 mencoba kembali dalam 3 detik...', 'sys');
    setTimeout(connect, 3000);
  };
  ws.onmessage = e => {
    const d = JSON.parse(e.data);
    if (d.type === 'echo')
      addLog('\u2190 Echo ESP32: ' + d.msg, 'in');
    if (d.type === 'info') {
      document.getElementById('sc').textContent = d.clients;
      document.getElementById('mc').textContent = d.total;
    }
  };
}

function kirim() {
  const msg = inp.value.trim();
  if (!msg || ws.readyState !== WebSocket.OPEN) return;
  ws.send(JSON.stringify({ cmd: 'echo', msg }));
  addLog('\u2192 Kamu: ' + msg, 'out');
  inp.value = '';
}

inp.addEventListener('keydown', e => { if (e.key === 'Enter') kirim(); });
connect();
</script>
</body></html>
)rawhtml";

// ─────────────────────────────────────────────────────────────────
// WebSocket Event Handler
// Single callback menangani semua event: connect, disconnect, text
// ─────────────────────────────────────────────────────────────────
void onWsEvent(uint8_t num, WStype_t type, uint8_t* payload, size_t length) {
  switch (type) {

    case WStype_CONNECTED: {
      wsClients++;
      IPAddress ip = ws.remoteIP(num);
      Serial.printf("[WS] Client #%d terhubung dari %s | Total: %d\n",
                    num, ip.toString().c_str(), wsClients);
      digitalWrite(LED_PIN, HIGH);  // LED ON = ada client
      // Broadcast info ke semua (update counter di semua browser)
      char info[64];
      snprintf(info, sizeof(info),
               "{\"type\":\"info\",\"clients\":%d,\"total\":%lu}",
               wsClients, (unsigned long)msgTotal);
      ws.broadcastTXT(info);
      break;
    }

    case WStype_DISCONNECTED: {
      if (wsClients > 0) wsClients--;
      Serial.printf("[WS] Client #%d terputus | Sisa: %d\n", num, wsClients);
      if (wsClients == 0) digitalWrite(LED_PIN, LOW);  // LED OFF = tidak ada client
      char info[64];
      snprintf(info, sizeof(info),
               "{\"type\":\"info\",\"clients\":%d,\"total\":%lu}",
               wsClients, (unsigned long)msgTotal);
      ws.broadcastTXT(info);
      break;
    }

    case WStype_TEXT: {
      msgTotal++;
      // Casting aman karena library WebSockets selalu null-terminated
      String raw = (char*)payload;
      Serial.printf("[WS] #%d → %s\n", num, raw.c_str());

      // ── Parse "msg" dari JSON (Penanganan Escaped Quotes Akurat) ──
      String extracted = "";
      int mi = raw.indexOf("\"msg\":\"");
      if (mi >= 0) {
        int s = mi + 7; // Mulai setelah "msg":"
        int e = s;
        while (e < raw.length()) {
          if (raw[e] == '"') {
            // Hitung runtutan karakter backslash sebelumnya
            int backslashCount = 0;
            int temp = e - 1;
            while (temp >= s && raw[temp] == '\\') {
              backslashCount++;
              temp--;
            }
            // Jika jumlah backslash genap (atau 0), ini adalah kutip penutup tulen
            if (backslashCount % 2 == 0) break; 
          }
          e++;
        }
        if (e > s) {
          extracted = raw.substring(s, e);
          if (extracted.length() > 100) extracted = extracted.substring(0, 100);
        }
      }

      // ── Echo kembali ke pengirim ───────────────────────────────
      char resp[200];
      snprintf(resp, sizeof(resp),
               "{\"type\":\"echo\",\"msg\":\"%s\",\"total\":%lu}",
               extracted.c_str(), (unsigned long)msgTotal);
      ws.sendTXT(num, resp);

      // ── Update statistik ke semua client ──────────────────────
      char info[64];
      snprintf(info, sizeof(info),
               "{\"type\":\"info\",\"clients\":%d,\"total\":%lu}",
               wsClients, (unsigned long)msgTotal);
      ws.broadcastTXT(info);

      // ── Trigger Kedip LED (Non-blocking) ──────────────────────
      // ⚠️ HARAM menggunakan delay() di dalam event handler TCP!
      // delay(25) akan mem-blokir tumpukan async. Gunakan millis timer.
      digitalWrite(LED_PIN, LOW);
      blinkTimer = millis() + 40; // Jadwalkan nyala kembali di loop()
      isBlinking = true;
      break;
    }

    case WStype_PING:
      Serial.printf("[WS] Ping dari #%d\n", num);
      break;

    case WStype_PONG:
      Serial.printf("[WS] Pong dari #%d\n", num);
      break;

    default: break;
  }
}

// ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  delay(500);

  Serial.println("\n=== BAB 47 Program 1: WebSocket Echo Server ===");

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
  Serial.printf("[WiFi] ✅ IP: %s | SSID: %s\n",
                WiFi.localIP().toString().c_str(), WiFi.SSID().c_str());

  // ── mDNS ──────────────────────────────────────────────────────
  if (MDNS.begin(MDNS_NAME)) {
    MDNS.addService("http", "tcp", 80);
    MDNS.addService("ws",   "tcp", 81);
    Serial.printf("[mDNS] ✅ http://%s.local | ws://%s.local:81\n",
                  MDNS_NAME, MDNS_NAME);
  } else {
    Serial.println("[mDNS] ⚠️ Gagal — gunakan alamat IP langsung.");
  }

  // ── HTTP: Sajikan halaman HTML ─────────────────────────────────
  server.on("/", HTTP_GET, []() {
    server.send_P(200, PSTR("text/html"), PAGE_HTML);
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
  Serial.println("[HTTP] Web server aktif di port 80.");

  // ── WebSocket Server ───────────────────────────────────────────
  ws.begin();
  ws.onEvent(onWsEvent);
  Serial.println("[WS]   WebSocket server aktif di port 81.");
  Serial.printf("[INFO] Buka browser → http://%s.local\n", MDNS_NAME);
}

// ─────────────────────────────────────────────────────────────────
void loop() {
  server.handleClient();  // Layani HTTP request (halaman HTML)
  ws.loop();              // Proses WebSocket frame masuk/keluar

  // ── Non-blocking LED Blink ────────────────────────────────────
  if (isBlinking && millis() >= blinkTimer) {
    isBlinking = false;
    digitalWrite(LED_PIN, wsClients > 0 ? HIGH : LOW);
  }

  // ── Deteksi Transisi WiFi (State-Transition Logging) ──────────
  static bool prevWiFi = true;  // true: sudah konek di setup()
  bool currWiFi = (WiFi.status() == WL_CONNECTED);
  if (!prevWiFi && currWiFi)
    Serial.printf("[WiFi] ✅ Reconnect! IP: %s\n",
                  WiFi.localIP().toString().c_str());
  if (prevWiFi && !currWiFi)
    Serial.println("[WiFi] ⚠️ Koneksi terputus...");
  prevWiFi = currWiFi;

  // ── Heartbeat Log (tiap 30 detik) ────────────────────────────
  static unsigned long tHb = 0;
  if (millis() - tHb >= 30000UL) {
    tHb = millis();
    Serial.printf("[HB] Clients: %d | Pesan: %lu | Heap: %u B | Uptime: %lu s\n",
                  wsClients, (unsigned long)msgTotal,
                  ESP.getFreeHeap(), millis() / 1000UL);
  }
}
