/*
 * BAB 48 - Program 2: HTTP POST — Kirim Data Sensor ke Server
 * ────────────────────────────────────────────────────────────
 * Fitur:
 *   ▶ Baca DHT11 (IO27) — suhu & kelembaban, non-blocking
 *   ▶ Bangun JSON payload dengan ArduinoJson (stack-allocated)
 *   ▶ HTTP POST ke httpbin.org/post (echo server gratis, tanpa registrasi)
 *   ▶ Parse response server → cetak ke Serial (validasi payload)
 *   ▶ Upload telemetri tiap POST_INTERVAL (default: 30 detik)
 *   ▶ Error handling: timeout, WiFi putus, HTTP code != 200
 *   ▶ Heartbeat log tiap 60 detik
 *
 * Tentang httpbin.org/post:
 *   Server ini memantulkan kembali (echo) semua data yang kita POST.
 *   Sangat berguna untuk memastikan format JSON yang dikirim sudah benar
 *   sebelum dikirim ke server produksi seperti Firebase atau thingspeak.
 *
 * Cara Uji:
 *   1. Install: "ArduinoJson" dan "DHT sensor library" via Library Manager
 *   2. Ganti WIFI_SSID dan WIFI_PASS di bawah
 *   3. Upload → buka Serial Monitor (115200 baud)
 *   4. Tiap 30 detik → data DHT11 dikirim ke httpbin.org
 *   5. Server membalas dengan kopi JSON yang kita kirim → validasi di Serial!
 *
 * Library yang dibutuhkan:
 *   - HTTPClient.h, WiFi.h, ESPmDNS.h  (bawaan ESP32 Core)
 *   - ArduinoJson >= 6.x               (install via Library Manager)
 *   - DHT sensor library by Adafruit   (install via Library Manager)
 *   - Adafruit Unified Sensor          (dependensi DHT, install otomatis)
 *
 * ⚠️ ESP32 hanya mendukung WiFi 2.4 GHz!
 * ⚠️ Ganti SSID dan PASSWORD sebelum upload!
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// ── Konfigurasi ───────────────────────────────────────────────────
const char* WIFI_SSID = "NamaWiFiKamu";  // ← Ganti!
const char* WIFI_PASS = "PasswordWiFi";  // ← Ganti!
const char* MDNS_NAME = "bluino";

// ── API Endpoint ──────────────────────────────────────────────────
// httpbin.org/post: echo server gratis — memantulkan kembali body POST
const char* POST_URL = "http://httpbin.org/post";
const unsigned long POST_INTERVAL = 30000UL; // Upload tiap 30 detik

// ── Pin ──────────────────────────────────────────────────────────
#define DHT_PIN  27
#define DHT_TYPE DHT11

// ── Objek ────────────────────────────────────────────────────────
DHT dht(DHT_PIN, DHT_TYPE);

// ── State Sensor (cache non-blocking) ────────────────────────────
float   lastTemp  = NAN;
float   lastHumid = NAN;
bool    sensorOk  = false;
uint32_t uploadCount = 0;

// ─────────────────────────────────────────────────────────────────
// POST data sensor ke server
// Payload dibangun dengan ArduinoJson (stack-allocated)
// ─────────────────────────────────────────────────────────────────
void postTelemetry() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HTTP] ⚠️ WiFi tidak terhubung, skip POST.");
    return;
  }

  // ── Bangun JSON Payload ───────────────────────────────────────
  // StaticJsonDocument di stack — Zero Heap Fragmentation
  StaticJsonDocument<256> payload;
  payload["device"]   = "bluino";
  payload["temp"]     = sensorOk ? lastTemp   : (float)NAN;
  payload["humidity"] = sensorOk ? lastHumid  : (float)NAN;
  payload["sensorOk"] = sensorOk;
  payload["uptime"]   = millis() / 1000UL;
  payload["freeHeap"] = ESP.getFreeHeap();
  payload["upload"]   = uploadCount + 1;

  // Serialisasi JSON ke char buffer (stack) — hindari String heap
  char body[256];
  size_t jsonLen = serializeJson(payload, body, sizeof(body));

  // ── HTTP POST ─────────────────────────────────────────────────
  Serial.printf("[HTTP] POST → %s\n", POST_URL);
  Serial.printf("[HTTP] Body: %s\n", body);

  HTTPClient http;
  WiFiClient client;
  http.begin(client, POST_URL);
  http.setTimeout(8000);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("User-Agent",   "Bluino-IoT/2026 (ESP32)");

  // Menggunakan jsonLen untuk menghilangkan overhead O(N) dari strlen()
  int code = http.POST((uint8_t*)body, jsonLen);

  if (code != HTTP_CODE_OK) {
    Serial.printf("[HTTP] ❌ Error: %d (%s)\n", code, http.errorToString(code).c_str());
    http.end();
    return;
  }

  uploadCount++;

  // ── Parse Response (httpbin memantulkan body kita di "json" key) ──
  // Filter: kita hanya butuh field "json" dari response yang panjang
  StaticJsonDocument<64> filter;
  filter["json"] = true;

  StaticJsonDocument<512> resp;
  DeserializationError err = deserializeJson(resp, http.getStream(),
                                              DeserializationOption::Filter(filter));
  http.end();

  if (err) {
    Serial.printf("[HTTP] ✅ %d OK — (response parse skip: %s)\n", code, err.c_str());
    return;
  }

  // Cetak echo payload yang diterima server (validasi format JSON kita)
  char echo[256];
  serializeJson(resp["json"], echo, sizeof(echo));
  Serial.printf("[HTTP] ✅ %d OK — Server menerima payload:\n  %s\n", code, echo);
}

// ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  dht.begin();
  delay(500); // Warm-up DHT11

  Serial.println("\n=== BAB 48 Program 2: HTTP POST — Sensor Telemetry ===");

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
    Serial.printf("[mDNS] ✅ http://%s.local\n", MDNS_NAME);
  } else {
    Serial.println("[mDNS] ⚠️ Gagal — lanjut tanpa mDNS.");
  }

  Serial.printf("[INFO] Telemetri akan dikirim tiap %lu detik\n", POST_INTERVAL / 1000UL);
  Serial.printf("[INFO] Endpoint: %s\n", POST_URL);

  // Baca sensor pertama saat boot
  float t = dht.readTemperature(), h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    lastTemp = t; lastHumid = h; sensorOk = true;
    Serial.printf("[DHT] ✅ Baca OK: %.1f°C | %.0f%%\n", t, h);
  }
}

// ─────────────────────────────────────────────────────────────────
void loop() {
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
  }

  // ── HTTP POST Non-Blocking (tiap POST_INTERVAL) ───────────────
  static unsigned long tPost = 0;
  if (millis() - tPost >= POST_INTERVAL) {
    tPost = millis();
    Serial.printf("[DHT] %.1f°C | %.0f%%\n", lastTemp, lastHumid);
    postTelemetry();
  }

  // ── Deteksi Transisi WiFi ─────────────────────────────────────
  static bool prevWiFi = true;
  bool currWiFi = (WiFi.status() == WL_CONNECTED);
  if (!prevWiFi && currWiFi)
    Serial.printf("[WiFi] ✅ Reconnect! IP: %s\n", WiFi.localIP().toString().c_str());
  if (prevWiFi && !currWiFi)
    Serial.println("[WiFi] ⚠️ Koneksi terputus...");
  prevWiFi = currWiFi;

  // ── Heartbeat (tiap 60 detik) ──────────────────────────────────
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] %.1fC %.0f%% | Heap: %u B | Upload #%lu\n",
                  lastTemp, lastHumid, ESP.getFreeHeap(), uploadCount);
  }
}
