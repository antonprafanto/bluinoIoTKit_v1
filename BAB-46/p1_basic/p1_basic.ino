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
