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
