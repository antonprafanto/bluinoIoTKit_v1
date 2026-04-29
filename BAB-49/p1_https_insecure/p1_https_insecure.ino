/*
 * BAB 49 - Program 1: HTTPS dengan setInsecure() — Prototipe Cepat
 * ─────────────────────────────────────────────────────────────────
 * Fitur:
 *   ▶ HTTPS GET ke https://httpbin.org/get menggunakan WiFiClientSecure
 *   ▶ setInsecure() — koneksi terenkripsi TAPI identitas server tidak diverifikasi
 *   ▶ Parse response JSON dengan ArduinoJson (stack only, zero heap)
 *   ▶ Output ke OLED I2C (IO21/IO22) dan Serial Monitor
 *   ▶ Non-blocking: fetch tiap FETCH_INTERVAL (default: 60 detik)
 *   ▶ Heartbeat log tiap 60 detik
 *
 * ⚠️ PERINGATAN KEAMANAN:
 *   setInsecure() hanya untuk PROTOTIPE dan DEBUGGING!
 *   Di lingkungan produksi, gunakan setCACert() seperti di Program 2.
 *   Risiko: serangan Man-in-the-Middle (MITM) — hacker bisa meniru server
 *   dan mencuri/mengubah data Anda!
 *
 * Tentang httpbin.org:
 *   Echo server HTTPS gratis. Memantulkan kembali semua header dan
 *   informasi request yang kita kirim — sempurna untuk belajar HTTPS!
 *
 * Library yang dibutuhkan:
 *   - WiFiClientSecure.h, HTTPClient.h, WiFi.h  (bawaan ESP32 Core)
 *   - ArduinoJson >= 6.x                         (Library Manager)
 *   - Adafruit SSD1306 + Adafruit GFX            (Library Manager)
 *
 * ⚠️ ESP32 hanya mendukung WiFi 2.4 GHz!
 * ⚠️ Ganti SSID dan PASSWORD sebelum upload!
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Konfigurasi WiFi ──────────────────────────────────────────────
const char* WIFI_SSID = "NamaWiFiKamu";  // ← Ganti!
const char* WIFI_PASS = "PasswordWiFi";  // ← Ganti!
const char* HOSTNAME  = "bluino";

// ── API Endpoint ──────────────────────────────────────────────────
const char* GET_URL = "https://httpbin.org/get";
const unsigned long FETCH_INTERVAL = 60000UL; // 60 detik

// ── OLED ──────────────────────────────────────────────────────────
#define OLED_W    128
#define OLED_H     64
#define OLED_ADDR 0x3C

Adafruit_SSD1306 oled(OLED_W, OLED_H, &Wire, -1);
bool oledReady = false;

// ── State ─────────────────────────────────────────────────────────
uint32_t fetchCount = 0;
char     lastOrigin[32] = "---"; // IP publik kita (dari response httpbin)

// ─────────────────────────────────────────────────────────────────
// OLED Helpers
// ─────────────────────────────────────────────────────────────────
void oledShowBoot() {
  if (!oledReady) return;
  oled.clearDisplay();
  oled.setTextSize(1); oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.println("Bluino IoT Kit");
  oled.println("BAB 49 - Program 1");
  oled.println();
  oled.println("HTTPS setInsecure()");
  oled.println();
  oled.println("Menghubungkan WiFi...");
  oled.display();
}

void oledShowStatus(bool ok) {
  if (!oledReady) return;
  oled.clearDisplay();
  oled.setTextSize(1); oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.println("HTTPS: setInsecure");
  oled.println("(Prototipe saja!)");
  oled.drawLine(0, 18, OLED_W - 1, 18, SSD1306_WHITE);
  oled.setCursor(0, 22);
  oled.println(ok ? "Status: OK" : "Status: ERROR");
  oled.setCursor(0, 34);
  oled.print("GET #"); oled.println(fetchCount);
  oled.setCursor(0, 46);
  oled.print("IP: "); oled.println(lastOrigin);
  oled.display();
}

// ─────────────────────────────────────────────────────────────────
// Fetch HTTPS — menggunakan setInsecure()
// ─────────────────────────────────────────────────────────────────
void fetchHttps() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HTTPS] ⚠️ WiFi tidak terhubung, skip fetch.");
    return;
  }

  Serial.printf("[HTTPS] GET %s\n", GET_URL);

  WiFiClientSecure secureClient;
  secureClient.setInsecure(); // ← Enkripsi aktif, verifikasi sertifikat DIMATIKAN

  HTTPClient http;
  http.begin(secureClient, GET_URL);
  http.setTimeout(10000); // TLS handshake butuh lebih lama dari HTTP biasa
  http.addHeader("User-Agent", "Bluino-IoT/2026 (ESP32)");

  int code = http.GET();

  if (code != HTTP_CODE_OK) {
    Serial.printf("[HTTPS] ❌ Error: %d (%s)\n", code, http.errorToString(code).c_str());
    oledShowStatus(false);
    http.end();
    return;
  }

  fetchCount++;

  // Parse: ambil hanya field "origin" (IP publik kita) dari response
  StaticJsonDocument<32> filter;
  filter["origin"] = true;

  StaticJsonDocument<128> doc;
  DeserializationError err = deserializeJson(doc, http.getStream(),
                                              DeserializationOption::Filter(filter));
  http.end();

  if (err) {
    Serial.printf("[HTTPS] ✅ %d OK | Heap: %u B (parse skip: %s)\n",
                  code, ESP.getFreeHeap(), err.c_str());
    oledShowStatus(true);
    return;
  }

  const char* origin = doc["origin"] | "?";
  strncpy(lastOrigin, origin, sizeof(lastOrigin) - 1);
  lastOrigin[sizeof(lastOrigin) - 1] = '\0';

  Serial.printf("[HTTPS] ✅ %d OK | IP Publik: %s | Heap: %u B\n",
                code, lastOrigin, ESP.getFreeHeap());

  oledShowStatus(true);
}

// ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  Serial.println("\n=== BAB 49 Program 1: HTTPS — setInsecure() ===");
  Serial.println("[WARN] Mode INSECURE: hanya untuk prototipe/debugging!");

  // ── OLED ──────────────────────────────────────────────────────
  if (oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    oledReady = true;
    Serial.println("[OLED] ✅ Display terdeteksi di 0x3C");
    oledShowBoot();
  } else {
    Serial.println("[OLED] ⚠️ Display tidak ditemukan — lanjut tanpa OLED.");
  }

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

  Serial.println("[HTTPS] ⚠️ Mode: setInsecure() — verifikasi sertifikat DIMATIKAN");
  Serial.printf("[INFO] Fetch tiap %lu detik\n", FETCH_INTERVAL / 1000UL);

  fetchHttps(); // Fetch pertama saat boot
}

// ─────────────────────────────────────────────────────────────────
void loop() {
  // ── Fetch Non-Blocking ────────────────────────────────────────
  static unsigned long tFetch = 0;
  if (millis() - tFetch >= FETCH_INTERVAL) {
    tFetch = millis();
    fetchHttps();
  }

  // ── Deteksi Transisi WiFi ─────────────────────────────────────
  static bool prevWiFi = true;
  bool currWiFi = (WiFi.status() == WL_CONNECTED);
  if (!prevWiFi && currWiFi) {
    Serial.printf("[WiFi] ✅ Reconnect! IP: %s\n", WiFi.localIP().toString().c_str());
    fetchHttps();
  }
  if (prevWiFi && !currWiFi)
    Serial.println("[WiFi] ⚠️ Koneksi terputus...");
  prevWiFi = currWiFi;

  // ── Heartbeat (tiap 60 detik) ─────────────────────────────────
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] Uptime: %lu s | Heap: %u B\n",
                  millis() / 1000UL, ESP.getFreeHeap());
  }
}
