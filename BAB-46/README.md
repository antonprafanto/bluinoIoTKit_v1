# BAB 46: Web Server — Dashboard IoT Berbasis Browser

> ✅ **Prasyarat:** Selesaikan **BAB 42 (WiFi Station Mode)** dan **BAB 45 (mDNS)**. ESP32 harus sudah bisa terhubung ke WiFi dan diakses via `bluino.local`.

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami arsitektur **request-response HTTP** pada jaringan lokal.
- Membangun **Web Server** ESP32 dengan beberapa halaman dan routing URL.
- Membuat **REST API** endpoint JSON yang dapat dibaca oleh aplikasi, browser, atau perangkat lain.
- Membaca sensor **DHT11** secara *non-blocking* dan menyajikan datanya secara real-time.
- Membangun **dashboard IoT interaktif** yang dapat mengontrol LED dan Relay dari browser tanpa aplikasi tambahan.

---

## 46.1 Masalah: "Bagaimana Mengontrol ESP32 dari Browser?"

```
SKENARIO IoT TANPA WEB SERVER:

  Kamu punya ESP32 → Terhubung WiFi → Bisa di-ping via bluino.local
  TAPI: Tidak ada cara untuk melihat data sensor atau mengontrol relay
  tanpa membuka Serial Monitor atau upload kode baru!

  Masalah 1: Tidak ada antarmuka pengguna (UI)
  Masalah 2: Tidak bisa dikontrol dari HP atau Browser
  Masalah 3: Tidak ada cara untuk mengambil data sensor dari jarak jauh


SOLUSI: Web Server Tertanam (Embedded Web Server)

  ┌─────────────────────────────────────────────────────────────┐
  │                    JARINGAN WiFi LOKAL                       │
  │                                                              │
  │   Browser HP/PC  ──── GET / ────►  ESP32 Web Server         │
  │                  ◄── HTML Page ──  (Port 80)                 │
  │                                                              │
  │   App / curl     ── GET /api ──►   /api/sensors             │
  │                  ◄─ JSON Data ──   {"temp":28.5,"hum":65}   │
  │                                                              │
  │   Browser        ─ POST /relay ►   /api/relay               │
  │                  ◄─ {"ok":true} ─  Relay ON!                │
  └─────────────────────────────────────────────────────────────┘

  KEUNGGULAN:
  ✅ Tidak butuh aplikasi khusus — browser bawaan HP sudah cukup
  ✅ Platform-independent: Windows, Mac, Android, iOS
  ✅ Data sensor bisa diakses dari mana saja di jaringan lokal
  ✅ Kontrol aktuator (LED, Relay) secara real-time
```

---

## 46.2 Anatomi HTTP & Web Server ESP32

```
SIKLUS REQUEST-RESPONSE HTTP:

  Browser                           ESP32 Web Server
  ───────                           ────────────────
    │                                      │
    │  GET / HTTP/1.1                      │
    │  Host: bluino.local                  │
    │─────────────────────────────────────►│
    │                                      │  server.on("/", handler)
    │                                      │  → buildHomePage()
    │  HTTP/1.1 200 OK                     │
    │  Content-Type: text/html             │
    │  <html>...</html>                    │
    │◄─────────────────────────────────────│
    │                                      │


HTTP METHODS YANG DIGUNAKAN DALAM BAB INI:

  ┌────────────┬────────────────────────────┬──────────────────────┐
  │ Method     │ Kegunaan                   │ Contoh               │
  ├────────────┼────────────────────────────┼──────────────────────┤
  │ GET        │ Ambil data / halaman       │ GET /api/sensors     │
  │ POST       │ Kirim perintah / data      │ POST /api/relay      │
  └────────────┴────────────────────────────┴──────────────────────┘

HTTP STATUS CODES PENTING:

  200 OK          → Sukses, ada konten yang dikembalikan
  302 Found       → Redirect ke URL lain
  404 Not Found   → URL tidak ditemukan di server
  405 Method Not Allowed → Method salah (GET ke endpoint POST)


ARSITEKTUR ROUTING WebServer.h:

  WiFi Client Request
         │
         ▼
  server.handleClient()   ← Dipanggil di loop() setiap iterasi
         │
         ├── server.on("/",            HTTP_GET,  handleRoot)
         ├── server.on("/about",       HTTP_GET,  handleAbout)
         ├── server.on("/api/sensors", HTTP_GET,  handleSensors)
         ├── server.on("/api/led",     HTTP_POST, handleLed)
         └── server.onNotFound(handleNotFound)
```

---

## 46.3 Library & Pin yang Tersedia

```
Library yang digunakan (semua bawaan / via Library Manager):

  WiFi.h       → Bawaan ESP32 Arduino Core
  ESPmDNS.h    → Bawaan ESP32 Arduino Core (lihat BAB 45)
  WebServer.h  → Bawaan ESP32 Arduino Core
  Preferences.h→ Bawaan ESP32 Arduino Core (NVS, untuk Program 3)
  DHT.h        → Install via Library Manager:
                  "DHT sensor library" by Adafruit
                  (Dependensi: "Adafruit Unified Sensor" — install otomatis)
```

### GPIO yang Tersedia untuk Relay & Aktuator Eksternal

```
BLUINO KIT — PIN YANG AMAN UNTUK RELAY / AKTUATOR (via kabel jumper):

  ┌──────────┬──────────────────────────────────────────────────┐
  │ GPIO     │ Keterangan                                        │
  ├──────────┼──────────────────────────────────────────────────┤
  │ IO4      │ ✅ AMAN — Digital I/O biasa, tidak ada restricsi  │
  │ IO13     │ ✅ AMAN — Digital I/O, tidak ada restricsi        │
  │ IO16     │ ✅ AMAN — Digital I/O, tidak ada restricsi        │
  │ IO17     │ ✅ AMAN — Digital I/O, tidak ada restricsi        │
  │ IO25     │ ✅ AMAN — Digital I/O / DAC1                      │
  │ IO26     │ ✅ AMAN — Digital I/O / DAC2                      │
  │ IO32     │ ✅ AMAN — Digital I/O / ADC1_CH4                  │
  │ IO33     │ ✅ AMAN — Digital I/O / ADC1_CH5                  │
  ├──────────┼──────────────────────────────────────────────────┤
  │ IO0      │ ⚠️ HINDARI — Strapping pin (boot mode)           │
  │ IO2      │ ⚠️ DIPAKAI — LED built-in kit (IO2)              │
  │ IO5      │ ⚠️ DIPAKAI — MicroSD CS                          │
  │ IO6-IO11 │ 🚫 DILARANG — Flash SPI internal                  │
  │ IO12     │ ⚠️ DIPAKAI — WS2812 RGB LED                      │
  │ IO14     │ ⚠️ DIPAKAI — DS18B20 sensor                      │
  │ IO18,19  │ ⚠️ DIPAKAI — MicroSD CLK/MISO                    │
  │ IO21,22  │ ⚠️ DIPAKAI — I2C Bus (OLED, MPU6050, BMP180)     │
  │ IO23     │ ⚠️ DIPAKAI — MicroSD MOSI                        │
  │ IO27     │ ⚠️ DIPAKAI — DHT11 sensor                        │
  │ IO34-39  │ ℹ️ INPUT ONLY — Tidak bisa output                │
  └──────────┴──────────────────────────────────────────────────┘

  💡 REKOMENDASI untuk relay di Program 3:
     Gunakan IO4, IO16, atau IO26 — paling aman dan bebas konflik.
     Hubungkan dengan kabel jumper dari pin relay board ke IO yang dipilih.
```

---

## 46.4 Program 1: Web Server Statis Dasar

Program paling sederhana: bangun web server dengan 2 halaman (`/` dan `/about`) dan handler 404 yang informatif.

```cpp
/*
 * BAB 46 - Program 1: Web Server Statis Dasar
 *
 * Fitur:
 *   ▶ GET /      → Halaman beranda: status perangkat (auto-refresh 10s)
 *   ▶ GET /about → Identitas chip ESP32 lengkap
 *   ▶ 404 handler yang informatif (bukan redirect)
 *   ▶ Integrasi mDNS: http://bluino.local
 *   ▶ LED IO2 berkedip saat ada HTTP request masuk
 *
 * Cara Uji:
 *   1. Upload, buka Serial Monitor (115200 baud)
 *   2. Browser: http://bluino.local  ATAU  http://<IP dari Serial>
 *   3. Klik "Tentang" untuk melihat halaman kedua
 *   4. Coba URL aneh misal http://bluino.local/xyz → lihat halaman 404
 *   5. Amati LED IO2 berkedip singkat setiap ada request masuk
 *
 * Library: WiFi.h, ESPmDNS.h, WebServer.h — Bawaan ESP32 Arduino Core
 *
 * ⚠️ ESP32 hanya mendukung WiFi 2.4 GHz!
 * ⚠️ Ganti SSID dan PASSWORD sesuai jaringan WiFi kamu!
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>

// ── Konfigurasi ───────────────────────────────────────────────────
const char* WIFI_SSID    = "NamaWiFiKamu";    // ← Ganti!
const char* WIFI_PASS    = "PasswordWiFi";    // ← Ganti!
const char* MDNS_NAME    = "bluino";          // Akses via bluino.local
const char* DEVICE_MODEL = "Bluino IoT Kit v3.2";

// ── Pin ──────────────────────────────────────────────────────────
#define LED_PIN 2   // LED built-in ESP32 (active HIGH)

// ── Objek ────────────────────────────────────────────────────────
WebServer server(80);

// ─────────────────────────────────────────────────────────────────
// HTML Builder: Halaman Beranda (/)
// ─────────────────────────────────────────────────────────────────
String buildHomePage() {
  // ── Kalkulasi uptime tanpa String() constructor (zero-copy) ──
  String uptime;
  uint32_t s = millis() / 1000;
  uint32_t h = s / 3600; s %= 3600;
  uint32_t m = s / 60;   s %= 60;
  uptime.reserve(20);
  uptime += h; uptime += "j "; uptime += m; uptime += "m "; uptime += s; uptime += "d";

  // ── Single-Shot Heap Allocation ───────────────────────────────
  // Memesan seluruh memori buffer di awal untuk menghindari
  // heap fragmentation akibat realokasi berulang.
  String html;
  html.reserve(2800);

  html += R"rawhtml(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="refresh" content="10">
  <title>Bluino IoT — Beranda</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body { font-family: 'Segoe UI', Arial, sans-serif; background: #0d0d1a;
           color: #e0e0e0; min-height: 100vh;
           display: flex; align-items: center; justify-content: center; padding: 20px; }
    .card { background: #1a1a2e; border-radius: 16px; padding: 32px;
            width: 100%; max-width: 440px; box-shadow: 0 8px 32px rgba(0,0,0,0.5); }
    h1 { color: #4fc3f7; font-size: 1.4rem; margin-bottom: 4px; }
    .sub { color: #555; font-size: 0.8rem; margin-bottom: 24px; }
    .row { display: flex; justify-content: space-between; align-items: center;
           padding: 10px 0; border-bottom: 1px solid #1e2a3a; font-size: 0.88rem; }
    .row:last-child { border-bottom: none; }
    .label { color: #888; }
    .value { color: #eee; font-weight: bold; }
    .badge { background: #0f3460; color: #4fc3f7; padding: 2px 8px;
             border-radius: 6px; font-size: 0.75rem; }
    .online { color: #81c784; }
    nav { margin-top: 20px; display: flex; gap: 10px; }
    nav a { display: block; flex: 1; text-align: center; padding: 10px;
            background: #0f3460; color: #4fc3f7; text-decoration: none;
            border-radius: 8px; font-size: 0.85rem; }
    nav a:hover { background: #1565c0; }
    .footer { text-align: center; color: #333; font-size: 0.72rem; margin-top: 16px; }
  </style>
</head>
<body>
<div class="card">
  <h1>⚡ Bluino IoT Kit</h1>
  <p class="sub">Web Server Dasar — BAB 46 · Auto-refresh 10s</p>
  <div class="row">
    <span class="label">Status Jaringan</span>
    <span class="value online">🟢 Online</span>
  </div>
)rawhtml";

  // ── Bagian Dinamis: disuntikkan ke buffer yang sudah dipesan ──
  html += "  <div class=\"row\"><span class=\"label\">Hostname mDNS</span>";
  html += "<span class=\"value\"><span class=\"badge\">";
  html += MDNS_NAME;
  html += ".local</span></span></div>\n";

  html += "  <div class=\"row\"><span class=\"label\">Alamat IP</span>";
  html += "<span class=\"value\">";
  html += WiFi.localIP().toString();
  html += "</span></div>\n";

  // Sanitasi XSS: escape karakter berbahaya pada nilai SSID
  String safeSSID = WiFi.SSID();
  safeSSID.replace("&", "&amp;");
  safeSSID.replace("<", "&lt;");
  safeSSID.replace(">", "&gt;");
  html += "  <div class=\"row\"><span class=\"label\">SSID WiFi</span>";
  html += "<span class=\"value\">";
  html += safeSSID;
  html += "</span></div>\n";

  int rssi = WiFi.RSSI();
  const char* rssiEmoji = (rssi >= -65) ? "🟢" : (rssi >= -80) ? "🟡" : "🔴";
  html += "  <div class=\"row\"><span class=\"label\">Sinyal WiFi</span>";
  html += "<span class=\"value\">";
  html += rssiEmoji;
  html += " ";
  html += rssi;
  html += " dBm</span></div>\n";

  html += "  <div class=\"row\"><span class=\"label\">Uptime</span>";
  html += "<span class=\"value\">";
  html += uptime;
  html += "</span></div>\n";

  html += "  <div class=\"row\"><span class=\"label\">Heap Bebas</span>";
  html += "<span class=\"value\">";
  html += (ESP.getFreeHeap() / 1024);
  html += " KB</span></div>\n";

  html += "  <div class=\"row\"><span class=\"label\">Model Perangkat</span>";
  html += "<span class=\"value\">";
  html += DEVICE_MODEL;
  html += "</span></div>\n";

  html += R"rawhtml(
  <nav>
    <a href="/">🏠 Beranda</a>
    <a href="/about">ℹ️ Tentang</a>
  </nav>
  <p class="footer">Bluino IoT Kit 2026 — BAB 46: Web Server</p>
</div>
</body>
</html>
)rawhtml";

  return html;
}

// ─────────────────────────────────────────────────────────────────
// HTML Builder: Halaman Tentang (/about)
// ─────────────────────────────────────────────────────────────────
String buildAboutPage() {
  String html;
  html.reserve(2000);

  // Chip ID: 48-bit dari eFuse MAC Address (unik per chip)
  uint64_t chipId = ESP.getEfuseMac();
  char cidBuf[13];
  snprintf(cidBuf, 13, "%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);

  html += R"rawhtml(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Bluino IoT — Tentang</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body { font-family: 'Segoe UI', Arial, sans-serif; background: #0d0d1a;
           color: #e0e0e0; min-height: 100vh;
           display: flex; align-items: center; justify-content: center; padding: 20px; }
    .card { background: #1a1a2e; border-radius: 16px; padding: 32px;
            width: 100%; max-width: 440px; box-shadow: 0 8px 32px rgba(0,0,0,0.5); }
    h1 { color: #4fc3f7; font-size: 1.4rem; margin-bottom: 4px; }
    .sub { color: #555; font-size: 0.8rem; margin-bottom: 24px; }
    .row { display: flex; justify-content: space-between; align-items: center;
           padding: 10px 0; border-bottom: 1px solid #1e2a3a; font-size: 0.88rem; }
    .row:last-child { border-bottom: none; }
    .label { color: #888; }
    .value { color: #eee; font-weight: bold; word-break: break-all; text-align: right; max-width: 60%; }
    nav { margin-top: 20px; }
    nav a { display: block; text-align: center; padding: 10px;
            background: #0f3460; color: #4fc3f7; text-decoration: none; border-radius: 8px; font-size: 0.85rem; }
    nav a:hover { background: #1565c0; }
  </style>
</head>
<body>
<div class="card">
  <h1>ℹ️ Tentang Perangkat</h1>
  <p class="sub">Identitas Lengkap ESP32 Ini</p>
)rawhtml";

  html += "  <div class=\"row\"><span class=\"label\">Model Kit</span><span class=\"value\">";
  html += DEVICE_MODEL;
  html += "</span></div>\n";

  html += "  <div class=\"row\"><span class=\"label\">Chip ID (MAC)</span><span class=\"value\">";
  html += cidBuf;
  html += "</span></div>\n";

  html += "  <div class=\"row\"><span class=\"label\">MAC Address</span><span class=\"value\">";
  html += WiFi.macAddress();
  html += "</span></div>\n";

  html += "  <div class=\"row\"><span class=\"label\">SDK Version</span><span class=\"value\">";
  html += ESP.getSdkVersion();
  html += "</span></div>\n";

  html += "  <div class=\"row\"><span class=\"label\">CPU Freq.</span><span class=\"value\">";
  html += ESP.getCpuFreqMHz();
  html += " MHz</span></div>\n";

  html += "  <div class=\"row\"><span class=\"label\">Flash Size</span><span class=\"value\">";
  html += (ESP.getFlashChipSize() / 1024 / 1024);
  html += " MB</span></div>\n";

  html += R"rawhtml(
  <nav><a href="/">← Kembali ke Beranda</a></nav>
</div>
</body>
</html>
)rawhtml";

  return html;
}

// ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  delay(500);

  Serial.println("\n=== BAB 46 Program 1: Web Server Statis Dasar ===");

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
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.printf("[WiFi] ✅ IP: %s | SSID: %s\n",
    WiFi.localIP().toString().c_str(), WiFi.SSID().c_str());

  // ── mDNS ──────────────────────────────────────────────────────
  if (MDNS.begin(MDNS_NAME)) {
    MDNS.addService("http", "tcp", 80);
    Serial.printf("[mDNS] ✅ http://%s.local\n", MDNS_NAME);
  } else {
    Serial.println("[mDNS] ⚠️ Gagal — gunakan alamat IP langsung.");
  }

  // ── Routes ────────────────────────────────────────────────────
  server.on("/", HTTP_GET, []() {
    digitalWrite(LED_PIN, LOW);    // Kedip: indikator request masuk
    server.send(200, "text/html", buildHomePage());
    digitalWrite(LED_PIN, HIGH);
  });

  server.on("/about", HTTP_GET, []() {
    digitalWrite(LED_PIN, LOW);
    server.send(200, "text/html", buildAboutPage());
    digitalWrite(LED_PIN, HIGH);
  });

  // 404 Handler: tampilkan halaman error yang informatif
  server.onNotFound([]() {
    String body;
    body.reserve(500);
    body += "<!DOCTYPE html><html lang=\"id\"><head><meta charset=\"UTF-8\">";
    body += "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">";
    body += "<title>404 — Tidak Ditemukan</title>";
    body += "<style>body{background:#0d0d1a;color:#e0e0e0;font-family:Arial,sans-serif;";
    body += "text-align:center;padding-top:80px;}h1{color:#ef5350;margin-bottom:12px;}";
    body += "p{color:#888;margin-bottom:20px;}a{color:#4fc3f7;text-decoration:none;}</style></head>";
    body += "<body><h1>404 — Halaman Tidak Ditemukan</h1>";

    // XSS sanitasi pada URI sebelum tampil ke HTML
    String safeUri = server.uri();
    safeUri.replace("&", "&amp;");
    safeUri.replace("<", "&lt;");
    safeUri.replace(">", "&gt;");

    body += "<p>URL <b style=\"color:#ef9a9a\">";
    body += safeUri;
    body += "</b> tidak tersedia di server ini.</p>";
    body += "<a href=\"/\">← Kembali ke Beranda</a></body></html>";
    server.send(404, "text/html", body);
  });

  server.begin();
  Serial.println("[HTTP] Web Server aktif di port 80.");
  Serial.printf("[INFO] Buka browser → http://%s.local\n", MDNS_NAME);

  digitalWrite(LED_PIN, HIGH);
}

// ─────────────────────────────────────────────────────────────────
void loop() {
  server.handleClient();

  // ── Deteksi Transisi WiFi ─────────────────────────────────────
  // prevWiFiState = true: sudah konek sejak setup() selesai
  // sehingga tidak memicu print "Reconnect!" palsu saat loop pertama
  static bool prevWiFiState = true;
  bool currWiFiState = (WiFi.status() == WL_CONNECTED);

  if (!prevWiFiState && currWiFiState) {
    Serial.printf("[WiFi] ✅ Reconnect! IP: %s\n",
      WiFi.localIP().toString().c_str());
    digitalWrite(LED_PIN, HIGH);
  }
  if (prevWiFiState && !currWiFiState) {
    Serial.println("[WiFi] ⚠️ Koneksi terputus...");
    digitalWrite(LED_PIN, LOW);
  }
  prevWiFiState = currWiFiState;

  // ── Heartbeat Log (tiap 15 detik) ───────────────────────────
  static unsigned long tHb = 0;
  if (millis() - tHb >= 15000UL) {
    tHb = millis();
    Serial.printf("[Heartbeat] IP: %s | RSSI: %d | Heap: %u B | Uptime: %lu s\n",
      WiFi.localIP().toString().c_str(), WiFi.RSSI(),
      ESP.getFreeHeap(), millis() / 1000UL);
  }
}
```

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 46 Program 1: Web Server Statis Dasar ===
[WiFi] Menghubungkan ke 'WiFiRumah'...
....
[WiFi] ✅ IP: 192.168.1.105 | SSID: WiFiRumah
[mDNS] ✅ http://bluino.local
[HTTP] Web Server aktif di port 80.
[INFO] Buka browser → http://bluino.local
[Heartbeat] IP: 192.168.1.105 | RSSI: -55 | Heap: 234560 B | Uptime: 15 s
```

---

## 46.5 Program 2: REST API + Dashboard Sensor Real-Time

Program ini menambahkan pembacaan **sensor DHT11 secara non-blocking** dan endpoint **REST API JSON**, sehingga data sensor dapat diakses oleh browser, curl, Node-RED, atau aplikasi apapun.

```cpp
/*
 * BAB 46 - Program 2: REST API + Dashboard Sensor Real-Time
 *
 * Fitur:
 *   ▶ GET /            → Dashboard HTML sensor (auto-refresh 5s)
 *   ▶ GET /api/sensors → Data JSON: suhu, kelembaban, RSSI, uptime, heap
 *   ▶ DHT11 dibaca secara NON-BLOCKING (setiap 2 detik via millis())
 *   ▶ Integrasi mDNS: http://bluino.local
 *   ▶ Format float via snprintf() — zero heap allocation
 *
 * Library yang Dibutuhkan (install via Arduino Library Manager):
 *   → "DHT sensor library" by Adafruit
 *   → "Adafruit Unified Sensor" (dependensi, install otomatis)
 *
 * Cara Uji:
 *   1. Upload, buka Serial Monitor (115200 baud)
 *   2. Browser: http://bluino.local → lihat dashboard auto-refresh
 *   3. API JSON: http://bluino.local/api/sensors
 *   4. Terminal: curl http://bluino.local/api/sensors
 *
 * ⚠️ ESP32 hanya mendukung WiFi 2.4 GHz!
 * ⚠️ Ganti SSID dan PASSWORD sesuai jaringan WiFi kamu!
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <DHT.h>

// ── Konfigurasi ───────────────────────────────────────────────────
const char* WIFI_SSID    = "NamaWiFiKamu";   // ← Ganti!
const char* WIFI_PASS    = "PasswordWiFi";   // ← Ganti!
const char* MDNS_NAME    = "bluino";
const char* DEVICE_MODEL = "Bluino IoT Kit v3.2";

// ── Pin ──────────────────────────────────────────────────────────
#define LED_PIN  2    // LED built-in ESP32
#define DHT_PIN  27   // DHT11 — hardwired di Bluino Kit (pull-up 4K7Ω)
#define DHT_TYPE DHT11

// ── Objek ────────────────────────────────────────────────────────
WebServer server(80);
DHT       dht(DHT_PIN, DHT_TYPE);

// ── State Sensor (Cache Non-Blocking) ────────────────────────────
float lastTemp  = NAN;
float lastHumid = NAN;
bool  sensorOk  = false;

// ─────────────────────────────────────────────────────────────────
// HTML Builder: Dashboard Sensor
// ─────────────────────────────────────────────────────────────────
String buildDashboard() {
  String uptime;
  uint32_t s = millis() / 1000;
  uint32_t h = s / 3600; s %= 3600;
  uint32_t m = s / 60;   s %= 60;
  uptime.reserve(20);
  uptime += h; uptime += "j "; uptime += m; uptime += "m "; uptime += s; uptime += "d";

  String html;
  html.reserve(3200);

  html += R"rawhtml(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="refresh" content="5">
  <title>Bluino IoT — Dashboard Sensor</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body { font-family: 'Segoe UI', Arial, sans-serif; background: #0d0d1a;
           color: #e0e0e0; min-height: 100vh; padding: 20px; }
    .container { max-width: 480px; margin: 0 auto; }
    h1 { color: #4fc3f7; font-size: 1.4rem; margin-bottom: 4px; padding-top: 20px; }
    .sub { color: #555; font-size: 0.8rem; margin-bottom: 20px; }
    .grid { display: grid; grid-template-columns: 1fr 1fr; gap: 12px; margin-bottom: 16px; }
    .mc { background: #1a1a2e; border-radius: 12px; padding: 20px; text-align: center; }
    .ml { color: #666; font-size: 0.75rem; text-transform: uppercase; margin-bottom: 8px; }
    .mv { font-size: 2rem; font-weight: bold; }
    .mu { font-size: 0.85rem; color: #888; }
    .hot  { color: #ef5350; } .warm { color: #ff9800; }
    .cool { color: #4fc3f7; } .humid{ color: #26c6da; }
    .table-card { background: #1a1a2e; border-radius: 12px; padding: 20px; margin-bottom: 16px; }
    table { width: 100%; border-collapse: collapse; }
    td { padding: 8px 4px; border-bottom: 1px solid #1e2a3a; font-size: 0.85rem; }
    td:first-child { color: #777; }
    td:last-child { color: #eee; font-weight: bold; text-align: right; }
    .badge { background: #0f3460; color: #4fc3f7; padding: 2px 8px; border-radius: 6px; font-size: 0.75rem; }
    .api-hint { background: #060610; border-radius: 8px; padding: 10px 14px; margin-top: 12px; }
    .api-hint code { color: #81c784; font-size: 0.8rem; display: block; }
    .footer { text-align: center; color: #333; font-size: 0.7rem; margin-top: 20px; padding-bottom: 20px; }
  </style>
</head>
<body><div class="container">
  <h1>🌡️ Dashboard Sensor</h1>
  <p class="sub">Bluino IoT Kit — BAB 46 · Auto-refresh 5 detik</p>
  <div class="grid">
)rawhtml";

  // ── Card Suhu ─────────────────────────────────────────────────
  // tempBuf: eksklusif untuk float "±NNN.N" (max 7 chars + null).
  // Nilai fallback "&mdash;" ditetapkan saat inisialisasi — tidak
  // mencampur tujuan buffer float dengan HTML entity di satu snprintf().
  char tempBuf[8] = "&mdash;";
  const char* tempClass = "cool";
  if (sensorOk && !isnan(lastTemp)) {
    snprintf(tempBuf, sizeof(tempBuf), "%.1f", lastTemp);
    tempClass = (lastTemp > 35) ? "hot" : (lastTemp > 28) ? "warm" : "cool";
  }
  html += "    <div class=\"mc\"><div class=\"ml\">🌡️ Suhu</div>\n";
  html += "    <div class=\"mv "; html += tempClass; html += "\">";
  html += tempBuf; html += "<span class=\"mu\">°C</span></div></div>\n";

  // ── Card Kelembaban ───────────────────────────────────────────
  char humBuf[8] = "&mdash;";
  if (sensorOk && !isnan(lastHumid)) snprintf(humBuf, sizeof(humBuf), "%.0f", lastHumid);
  html += "    <div class=\"mc\"><div class=\"ml\">💧 Kelembaban</div>\n";
  html += "    <div class=\"mv humid\">"; html += humBuf;
  html += "<span class=\"mu\">%%</span></div></div>\n";
  html += "  </div>\n";

  // ── Tabel Info Jaringan ───────────────────────────────────────
  html += "  <div class=\"table-card\"><table>\n";
  html += "    <tr><td>Status</td><td>🟢 Online</td></tr>\n";
  html += "    <tr><td>Hostname mDNS</td><td><span class=\"badge\">";
  html += MDNS_NAME; html += ".local</span></td></tr>\n";
  html += "    <tr><td>Alamat IP</td><td>";
  html += WiFi.localIP().toString(); html += "</td></tr>\n";

  String safeSSID = WiFi.SSID();
  safeSSID.replace("&","&amp;"); safeSSID.replace("<","&lt;"); safeSSID.replace(">","&gt;");
  html += "    <tr><td>SSID WiFi</td><td>"; html += safeSSID; html += "</td></tr>\n";

  int rssi = WiFi.RSSI();
  const char* re = (rssi >= -65) ? "🟢" : (rssi >= -80) ? "🟡" : "🔴";
  html += "    <tr><td>Sinyal WiFi</td><td>"; html += re; html += " "; html += rssi; html += " dBm</td></tr>\n";
  html += "    <tr><td>Uptime</td><td>"; html += uptime; html += "</td></tr>\n";
  html += "    <tr><td>Heap Bebas</td><td>"; html += (ESP.getFreeHeap() / 1024); html += " KB</td></tr>\n";
  html += "    <tr><td>Sensor DHT11</td><td>";
  html += sensorOk ? "🟢 OK" : "🔴 Error/Belum Baca";
  html += "</td></tr>\n  </table>\n";
  html += R"rawhtml(
    <div class="api-hint">
      <code>GET /api/sensors → JSON real-time (curl, Node-RED, dll)</code>
    </div>
  </div>
  <p class="footer">Bluino IoT Kit 2026 — BAB 46: Web Server</p>
</div></body></html>
)rawhtml";
  return html;
}

// ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  dht.begin();
  delay(500);

  Serial.println("\n=== BAB 46 Program 2: REST API + Dashboard Sensor ===");

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
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.printf("[WiFi] ✅ IP: %s\n", WiFi.localIP().toString().c_str());

  // ── mDNS ──────────────────────────────────────────────────────
  if (MDNS.begin(MDNS_NAME)) {
    MDNS.addService("http", "tcp", 80);
    MDNS.addServiceTxt("http", "tcp", "api", "/api/sensors");
    Serial.printf("[mDNS] ✅ http://%s.local\n", MDNS_NAME);
  }

  // ── Routes ────────────────────────────────────────────────────
  server.on("/", HTTP_GET, []() {
    digitalWrite(LED_PIN, LOW);
    server.send(200, "text/html", buildDashboard());
    digitalWrite(LED_PIN, HIGH);
  });

  server.on("/api/sensors", HTTP_GET, []() {
    // snprintf ke stack → zero heap allocation
    char tempStr[8] = "null", humStr[8] = "null";
    if (sensorOk && !isnan(lastTemp))   snprintf(tempStr, sizeof(tempStr), "%.1f", lastTemp);
    if (sensorOk && !isnan(lastHumid))  snprintf(humStr,  sizeof(humStr),  "%.0f", lastHumid);

    char json[256];
    snprintf(json, sizeof(json),
      "{\"temp\":%s,\"humidity\":%s,\"rssi\":%d,"
      "\"uptime\":%lu,\"freeHeap\":%u,\"cpuMHz\":%u,\"sensorOk\":%s}",
      tempStr, humStr, WiFi.RSSI(), millis() / 1000UL,
      ESP.getFreeHeap(), ESP.getCpuFreqMHz(),
      sensorOk ? "true" : "false"
    );

    // Streaming murni — bypass String() implisit di server.send()
    server.sendHeader("Access-Control-Allow-Origin", "*");
    server.sendHeader("Cache-Control", "no-store, no-cache, must-revalidate");
    server.setContentLength(strlen(json));
    server.send(200, "application/json", "");
    server.sendContent(json);
  });

  // 404 & CORS Preflight (OPTIONS) Handler
  server.onNotFound([]() {
    if (server.method() == HTTP_OPTIONS) {
      server.sendHeader("Access-Control-Allow-Origin", "*");
      server.sendHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
      server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
      server.send(204);
      return;
    }
    server.send(404, "text/plain", "404 Not Found");
  });

  server.begin();
  Serial.println("[HTTP] Web Server aktif.");
  Serial.printf("[INFO] Dashboard: http://%s.local\n", MDNS_NAME);
  Serial.printf("[INFO] API JSON : http://%s.local/api/sensors\n", MDNS_NAME);
  digitalWrite(LED_PIN, HIGH);
}

// ─────────────────────────────────────────────────────────────────
void loop() {
  server.handleClient();

  // ── Baca DHT11 Non-Blocking (setiap 2 detik) ─────────────────
  // DHT11 butuh min. 1 detik antar pembacaan.
  // Loop() tidak boleh di-block — kita gunakan millis() timer.
  static unsigned long tDHT = 0;
  if (millis() - tDHT >= 2000UL) {
    tDHT = millis();
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      if (!sensorOk) Serial.println("[DHT] ✅ Sensor kembali terbaca!");
      lastTemp  = t;
      lastHumid = h;
      sensorOk  = true;
    } else {
      if (sensorOk) Serial.println("[DHT] ⚠️ Gagal baca sensor! Cek wiring IO27.");
      sensorOk = false;
    }
  }

  // ── Deteksi Transisi WiFi ─────────────────────────────────────
  static bool prevWiFiState = true;
  bool currWiFiState = (WiFi.status() == WL_CONNECTED);
  if (!prevWiFiState && currWiFiState)
    Serial.printf("[WiFi] ✅ Reconnect! IP: %s\n", WiFi.localIP().toString().c_str());
  if (prevWiFiState && !currWiFiState) {
    Serial.println("[WiFi] ⚠️ Koneksi terputus...");
    digitalWrite(LED_PIN, LOW);
  }
  prevWiFiState = currWiFiState;

  // ── Heartbeat Log (tiap 30 detik) ────────────────────────────
  static unsigned long tHb = 0;
  if (millis() - tHb >= 30000UL) {
    tHb = millis();
    Serial.printf("[Heartbeat] Temp: %.1f°C | Hum: %.0f%% | RSSI: %d | Heap: %u B\n",
      lastTemp, lastHumid, WiFi.RSSI(), ESP.getFreeHeap());
  }
}
```

**Contoh Response API JSON:**
```json
{
  "temp": 28.5,
  "humidity": 65,
  "rssi": -58,
  "uptime": 1234,
  "freeHeap": 218240,
  "cpuMHz": 240,
  "sensorOk": true
}
```

**Cara Uji dengan curl:**
```bash
# Windows PowerShell / Terminal Mac/Linux:
curl http://bluino.local/api/sensors
```

---


## 46.6 Program 3: Dashboard IoT Interaktif

Program lengkap: sensor real-time, JS `fetch()` untuk kontrol aktuator tanpa reload, NVS state persistence, dan Serial CLI.

```
Routing:
  GET  /            -> HTML dashboard (PROGMEM, zero heap)
  GET  /api/sensors -> JSON suhu, kelembaban, ledState, relayState
  POST /api/led     -> Kontrol LED IO2  (?state=1 / ?state=0)
  POST /api/relay   -> Kontrol Relay    (?state=1 / ?state=0)
```

```cpp
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
// Keuntungan vs html.reserve(N):
//   - HTML statis sama sekali tidak memakan heap
//   - JavaScript fetch() memperbarui sensor tiap 5 detik tanpa reload
// ---------------------------------------------------------------------
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
    setTimeout(()=>abort.abort(), 3000); // Batasi timeout 3 detik
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
  setLedUI(ns); // Optimistic UI update!
  try{
    const d=await(await fetch("/api/led?state="+ns,{method:"POST"})).json();
    setLedUI(d.state);
  }catch(e){ setLedUI(!ns); }
}
async function toggleRelay(){
  const ns=relayOn?0:1;
  setRelayUI(ns); // Optimistic UI update!
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

  inputBuffer.reserve(128); // Pre-alokasi CLI buffer

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
      // CORS header sudah dikirim — browser bisa baca respons 400 ini
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
      // CORS header sudah dikirim — browser bisa baca respons 400 ini
      server.send(400,"application/json","{\"error\":\"missing state\"}"); return;
    }
    relayState=(server.arg("state")=="1");
    digitalWrite(RELAY_PIN, relayState?HIGH:LOW);
    saveRelayState(relayState); // Simpan NVS -- bertahan setelah reboot!
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

  // Deteksi transisi WiFi (prevWiFiState=true: sudah konek di setup)
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
```

**Contoh Output Serial Monitor:**
```
=== BAB 46 Program 3: Dashboard IoT Interaktif ===
[INFO] Ketik 'help' di Serial Monitor.
[INFO] Relay PIN: IO4 -- sambungkan kabel jumper!
[NVS] Relay state dimuat: OFF
[WiFi] Terhubung! IP: 192.168.1.105 | SSID: WiFiRumah
[mDNS] http://bluino.local
==========================================
 Dashboard  : http://bluino.local
 Sensor API : http://bluino.local/api/sensors
==========================================
[HTTP] LED ON via API
[HTTP] Relay ON via API -- disimpan NVS
[HB] 28.5C 65% LED:ON Relay:ON Heap:195240 B
```

---

## 46.7 Konsep Lanjutan: WebServer.h vs AsyncWebServer

```
PERBANDINGAN LIBRARY WEB SERVER ESP32:

  Fitur               WebServer.h     ESPAsyncWebServer
  ─────────────────   ───────────     ─────────────────
  Terinstall default  Bawaan          Install manual
  Arsitektur          Synchronous     Asynchronous (RTOS)
  Concurrent request  1 per loop      Multiple sekaligus
  RAM usage           Lebih hemat     Lebih boros
  Cocok untuk         IoT dasar-mng   Web app kompleks


TEKNIK PROGMEM (digunakan di Program 3):

  HTML di heap (Program 1 dan 2):
    String html; html.reserve(3000); html += "...";  // Pakai heap!

  HTML di Flash PROGMEM (Program 3):
    const char HTML[] PROGMEM = R"rawhtml(...)rawhtml";
    server.send_P(200, PSTR("text/html"), HTML);     // ZERO heap!

  Keunggulan:
  - HTML 3-5KB tidak memakan heap sama sekali
  - Flash 4MB jauh lebih besar dari 320KB DRAM ESP32
  - Ideal: halaman statis besar + API JSON dinamis terpisah
```

---

## 46.8 Tabel Fungsi `WebServer.h`

| Fungsi | Keterangan | Contoh |
|--------|------------|--------|
| `WebServer server(80)` | Buat server port 80 | `WebServer server(80)` |
| `server.begin()` | Mulai server | `server.begin()` |
| `server.handleClient()` | Proses request (panggil di loop) | `server.handleClient()` |
| `server.on(url, method, fn)` | Daftarkan route handler | `server.on("/", HTTP_GET, fn)` |
| `server.onNotFound(fn)` | Handler 404 | `server.onNotFound(fn)` |
| `server.send(code, type, body)` | Kirim respons | `server.send(200, "text/html", html)` |
| `server.sendHeader(k, v)` | Tambah HTTP header | `server.sendHeader("Allow-Origin","*")` |
| `server.setContentLength(n)` | Set panjang respons | `server.setContentLength(len)` |
| `server.sendContent(data)` | Streaming zero-copy | `server.sendContent(json)` |
| `server.send_P(code, type, pgm)` | Kirim dari PROGMEM | `server.send_P(200, type, HTML)` |
| `server.hasArg(key)` | Cek query parameter ada? | `server.hasArg("state")` |
| `server.arg(key)` | Ambil nilai query parameter | `server.arg("state")` |
| `server.uri()` | URL dari request aktif | `server.uri()` |
| `server.client().remoteIP()` | IP klien pengirim | `server.client().remoteIP()` |

---

## 46.9 Latihan & Tantangan

### Latihan Dasar
1. **Upload Program 1**, buka `http://bluino.local` di browser HP. Klik "Tentang" — bandingkan Chip ID dengan yang tampil di Serial Monitor.
2. **Upload Program 2**, buka `http://bluino.local/api/sensors` di browser. Tiup hangat ke DHT11 dan refresh — amati perubahan nilai suhu.
3. **Uji curl**: Di PowerShell/Terminal, ketik `curl http://bluino.local/api/sensors`.

### Latihan Menengah
4. **Upload Program 3**, sambungkan kabel jumper dari relay board ke IO4. Buka dashboard di HP dan kontrol relay. Restart ESP32 — apakah relay langsung menyala sesuai state terakhir?
5. **Uji NVS Persistence**: Ketik `relay on` di Serial Monitor CLI, lalu power off ESP32. Nyalakan kembali — apakah relay tetap ON?
6. **Modifikasi**: Tambahkan field `"model":"Bluino IoT Kit v3.2"` ke response JSON `/api/sensors` pada Program 2.

### Tantangan Lanjutan
7. **Endpoint chip info**: Buat `GET /api/chip` yang merespons JSON berisi `chipId`, `sdkVersion`, `cpuMHz`, `flashSize`. Gunakan `char json[]` stack-allocated.
8. **Multi-sensor**: Tambahkan pembacaan LDR (IO35, jumper + resistor 10K) ke Program 2. Tampilkan nilai ADC dan persentase kecerahan di dashboard.

### Kata Kunci Penelitian Mandiri
- `REST API` — Arsitektur standar komunikasi client-server
- `PROGMEM` — Menyimpan data statis di Flash ESP32 alih-alih RAM
- `ESPAsyncWebServer` — Library web server asinkron (preview BAB 47)
- `CORS` — Cross-Origin Resource Sharing, keamanan fetch lintas domain

---

## 46.10 Referensi

| Sumber | Link |
|--------|------|
| WebServer.h Source | https://github.com/espressif/arduino-esp32/tree/master/libraries/WebServer |
| ESP-IDF HTTP Server | https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/esp_http_server.html |
| RFC 7230 HTTP/1.1 | https://datatracker.ietf.org/doc/html/rfc7230 |
| PROGMEM Reference | https://www.arduino.cc/reference/en/language/variables/utilities/progmem/ |
| MDN Fetch API | https://developer.mozilla.org/en-US/docs/Web/API/Fetch_API |

---

> Selesai! ESP32 Bluino Kit kamu sekarang adalah web server penuh: menyajikan halaman HTML, REST API JSON, dan mengontrol aktuator dari browser manapun di jaringan lokal tanpa aplikasi tambahan!
>
> **Langkah Selanjutnya:** [BAB 47 — WebSocket](../BAB-47/) untuk komunikasi dua arah *real-time* antara browser dan ESP32 tanpa perlu polling setiap 5 detik.
