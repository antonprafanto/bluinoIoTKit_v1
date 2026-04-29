/*
 * BAB 48 - Program 3: IoT Cloud Cyclic — GET Config + POST Telemetry
 * ─────────────────────────────────────────────────────────────────────
 * Fitur Lengkap:
 *   ▶ State Machine IoT: IDLE → FETCH_CONFIG → POST_TELEMETRY → IDLE
 *   ▶ GET config dari server mock (jsonplaceholder.typicode.com)
 *      → Parse field: interval, relayOn
 *      → Terapkan: set interval POST, kontrol Relay IO4
 *   ▶ POST telemetri (suhu, kelembaban, relay, heap, uptime) ke httpbin.org
 *   ▶ DHT11 non-blocking IO27 — baca suhu & kelembaban
 *   ▶ Relay IO4 dikontrol dari config server
 *   ▶ Connection timeout: http.setTimeout(5000) di setiap request
 *   ▶ Error recovery: skip cycle jika request gagal, coba lagi di cycle berikut
 *   ▶ Heartbeat log tiap 30 detik
 *
 * Pola Arsitektur "Pull Config, Push Telemetry":
 *   Setiap siklus ESP32 melakukan dua hal:
 *     1. PULL: Ambil perintah/konfigurasi dari server cloud
 *     2. PUSH: Kirim data sensor ke server cloud
 *   Ini adalah pola yang digunakan oleh jutaan perangkat IoT industri.
 *
 * Mock Server:
 *   GET  → https://jsonplaceholder.typicode.com/todos/1
 *          (server gratis, selalu tersedia, response stabil untuk pembelajaran)
 *          Kita mapping field: "completed" → relayOn, "id" → interval
 *
 *   POST → http://httpbin.org/post
 *          (echo server, memantulkan payload kita)
 *
 * Cara Uji:
 *   1. Sambungkan Relay ke IO4 dengan kabel jumper
 *   2. Ganti WIFI_SSID dan WIFI_PASS di bawah
 *   3. Upload → amati state machine berpindah di Serial Monitor
 *   4. Observasi: relay aktif/nonaktif sesuai field "completed" dari API mock
 *
 * Library yang dibutuhkan:
 *   - HTTPClient.h, WiFi.h             (bawaan ESP32 Core)
 *   - ArduinoJson >= 6.x               (install via Library Manager)
 *   - DHT sensor library by Adafruit   (install via Library Manager)
 *   - Adafruit Unified Sensor          (dependensi DHT, install otomatis)
 *
 * ⚠️ Program 3 menggunakan HTTP biasa (bukan HTTPS). Untuk koneksi ke server HTTPS,
 *    lihat BAB 49 yang membahas WiFiClientSecure & TLS/SSL.
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// ── Konfigurasi WiFi ─────────────────────────────────────────────
const char* WIFI_SSID = "NamaWiFiKamu";  // ← Ganti!
const char* WIFI_PASS = "PasswordWiFi";  // ← Ganti!
const char* HOSTNAME  = "bluino";

// ── Pin ──────────────────────────────────────────────────────────
#define DHT_PIN   27   // DHT11 IO27 (hardwired di Bluino Kit)
#define DHT_TYPE  DHT11
#define RELAY_PIN  4   // ← Sambungkan kabel jumper ke pin ini!

// ── API Endpoints ────────────────────────────────────────────────
const char* CONFIG_URL = "http://jsonplaceholder.typicode.com/todos/1";
const char* POST_URL   = "http://httpbin.org/post";

// ── State Machine ─────────────────────────────────────────────────
// Menggunakan enum untuk keterbacaan dan keamanan type
enum class IoTState : uint8_t {
  IDLE,           // Menunggu siklus berikutnya
  FETCH_CONFIG,   // Sedang mengambil konfigurasi dari server
  POST_TELEMETRY  // Sedang mengirim data sensor ke server
};

IoTState iotState     = IoTState::FETCH_CONFIG; // Mulai dari fetch saat boot
uint32_t cycleTimer   = 0;
uint32_t cycleInterval = 30000UL; // Default 30 detik, bisa diubah server
uint32_t cycleCount   = 0;

// ── State Sensor & Aktuator ───────────────────────────────────────
float   lastTemp   = NAN;
float   lastHumid  = NAN;
bool    sensorOk   = false;
bool    relayState = false; // Dikendalikan oleh server config

// ── Objek ────────────────────────────────────────────────────────
DHT dht(DHT_PIN, DHT_TYPE);

// ─────────────────────────────────────────────────────────────────
// STATE 1: Ambil konfigurasi dari server
// Parsing: "completed" → relayOn | "id" → faktor interval
// ─────────────────────────────────────────────────────────────────
bool fetchConfig() {
  Serial.println("[CFG] GET config dari server...");

  HTTPClient http;
  WiFiClient client;
  http.begin(client, CONFIG_URL);
  http.setTimeout(5000); // 5 detik — jika server lambat, jangan tunggu selamanya

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("[CFG] ❌ Error: %d (%s)\n", code, http.errorToString(code).c_str());
    http.end();
    return false;
  }

  // Filter: hanya ambil field yang kita butuhkan dari response
  StaticJsonDocument<32> filter;
  filter["completed"] = true;
  filter["id"]        = true;

  StaticJsonDocument<128> cfg;
  DeserializationError err = deserializeJson(cfg, http.getStream(),
                                              DeserializationOption::Filter(filter));
  http.end();

  if (err) {
    Serial.printf("[CFG] ❌ JSON Parse Error: %s\n", err.c_str());
    return false;
  }

  // Terapkan config: "completed" → relay | "id" (1..200) × 5000ms → interval
  bool  newRelay    = cfg["completed"].as<bool>();
  int   cfgId       = cfg["id"].as<int>();
  uint32_t newInterval = max(10000UL, (uint32_t)(cfgId % 10 + 1) * 10000UL);

  // Hanya catat perubahan (state-transition logging — anti spam)
  if (newRelay != relayState) {
    relayState = newRelay;
    digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
    Serial.printf("[CFG] 🔄 Relay berubah → %s\n", relayState ? "ON" : "OFF");
  }
  if (newInterval != cycleInterval) {
    cycleInterval = newInterval;
    Serial.printf("[CFG] 🔄 Interval berubah → %lu s\n", cycleInterval / 1000UL);
  }

  Serial.printf("[CFG] ✅ interval=%lus | relayOn=%s\n",
                cycleInterval / 1000UL, relayState ? "true" : "false");
  return true;
}

// ─────────────────────────────────────────────────────────────────
// STATE 2: Kirim data sensor ke server
// Payload: JSON dengan suhu, kelembaban, relay, heap, uptime
// ─────────────────────────────────────────────────────────────────
bool postTelemetry() {
  Serial.println("[TLM] POST telemetri → server...");

  // Bangun payload menggunakan ArduinoJson (stack-allocated)
  StaticJsonDocument<256> payload;
  payload["device"]    = "bluino";
  payload["cycle"]     = cycleCount;
  payload["temp"]      = sensorOk ? lastTemp  : (float)NAN;
  payload["humidity"]  = sensorOk ? lastHumid : (float)NAN;
  payload["sensorOk"]  = sensorOk;
  payload["relayState"]= relayState;
  payload["uptime"]    = millis() / 1000UL;
  payload["freeHeap"]  = ESP.getFreeHeap();

  char body[256];
  size_t jsonLen = serializeJson(payload, body, sizeof(body));

  HTTPClient http;
  WiFiClient client;
  http.begin(client, POST_URL);
  http.setTimeout(5000);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("User-Agent",   "Bluino-IoT/2026 (ESP32)");

  int code = http.POST((uint8_t*)body, jsonLen);

  if (code != HTTP_CODE_OK) {
    Serial.printf("[TLM] ❌ Error: %d (%s)\n", code, http.errorToString(code).c_str());
    http.end();
    return false;
  }
  
  http.end();

  cycleCount++;
  Serial.printf("[TLM] ✅ %d OK — Telemetri terkirim #%lu\n", code, cycleCount);
  return true;
}

// ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  dht.begin();
  delay(500);

  Serial.println("\n=== BAB 48 Program 3: IoT Cloud Cyclic ===");
  Serial.printf("[INFO] Relay PIN: IO%d — sambungkan kabel jumper!\n", RELAY_PIN);
  Serial.printf("[INFO] Config  URL: %s\n", CONFIG_URL);
  Serial.printf("[INFO] Telemetry URL: %s\n", POST_URL);

  // ── Koneksi WiFi ──────────────────────────────────────────────
  Serial.printf("[WiFi] Menghubungkan ke '%s'...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(HOSTNAME);
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

  Serial.println("[STATE] Mulai siklus pertama → FETCH_CONFIG");
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

  // ── State Machine IoT (Hanya jalan jika WiFi terhubung) ───────
  if (WiFi.status() == WL_CONNECTED) {
    switch (iotState) {
      case IoTState::FETCH_CONFIG:
        fetchConfig(); // Tidak peduli berhasil atau gagal, lanjut ke POST
        iotState = IoTState::POST_TELEMETRY;
        break;

      case IoTState::POST_TELEMETRY:
        postTelemetry();
        iotState    = IoTState::IDLE;
        cycleTimer  = millis();
        Serial.printf("[STATE] IDLE — tunggu %lu detik...\n", cycleInterval / 1000UL);
        break;

      case IoTState::IDLE:
        // Tunggu timer sebelum mulai siklus berikutnya
        if (millis() - cycleTimer >= cycleInterval) {
          Serial.printf("\n[STATE] Siklus #%lu dimulai → FETCH_CONFIG\n", cycleCount + 1);
          iotState = IoTState::FETCH_CONFIG;
        }
        break;
    }
  }

  // ── Deteksi Transisi WiFi ─────────────────────────────────────
  static bool prevWiFi = true;
  bool currWiFi = (WiFi.status() == WL_CONNECTED);
  if (!prevWiFi && currWiFi) {
    Serial.printf("[WiFi] ✅ Reconnect! IP: %s\n", WiFi.localIP().toString().c_str());
    iotState = IoTState::FETCH_CONFIG; // Reset state machine saat WiFi pulih
  }
  if (prevWiFi && !currWiFi)
    Serial.println("[WiFi] ⚠️ Koneksi terputus — state machine ditahan.");
  prevWiFi = currWiFi;

  // ── Heartbeat (tiap 30 detik) ──────────────────────────────────
  static unsigned long tHb = 0;
  if (millis() - tHb >= 30000UL) {
    tHb = millis();
    Serial.printf("[HB] %.1fC %.0f%% Relay:%s Cycle:%lu Heap:%u B\n",
                  lastTemp, lastHumid,
                  relayState ? "ON" : "OFF",
                  cycleCount, ESP.getFreeHeap());
  }
}
