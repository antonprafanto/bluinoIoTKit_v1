/*
 * BAB 49 - Program 3: HTTPS POST — Data DHT11 ke Antares.id
 * ─────────────────────────────────────────────────────────────────
 * Fitur:
 *   ▶ Baca DHT11 (IO27) — suhu & kelembaban, non-blocking
 *   ▶ HTTPS POST ke platform.antares.id:8443 (Platform IoT Indonesia/TELKOM)
 *   ▶ Format request: oneM2M m2m:cin (standar internasional)
 *   ▶ Autentikasi via header X-M2M-Origin (Access Key)
 *   ▶ Verifikasi server: setCACert() — Root CA DigiCert Global Root G2
 *   ▶ Kelola payload JSON dengan ArduinoJson (stack only, zero heap)
 *   ▶ OLED: tampilkan suhu, status upload, total upload
 *   ▶ Non-blocking: POST tiap POST_INTERVAL (default: 30 detik)
 *   ▶ Heartbeat tiap 60 detik
 *
 * Tentang Antares.id:
 *   Platform IoT cloud buatan TELKOM Indonesia, dibangun di atas
 *   standar internasional oneM2M. Digunakan oleh ribuan perangkat IoT
 *   di Indonesia untuk Smart City, Smart Farming, dan monitoring industri.
 *
 * Format Request oneM2M (m2m:cin):
 *   POST /~/antares-cse/antares-id/{APP}/{DEVICE}
 *   Host: platform.antares.id:8443
 *   X-M2M-Origin: {ACCESS_KEY}
 *   Content-Type: application/json;ty=4
 *   Accept: application/json
 *
 *   Body: { "m2m:cin": { "con": "{\"temp\":29,\"humidity\":65}" } }
 *
 *   Response: 201 Created (bukan 200 OK — kita membuat resource baru!)
 *
 * Cara Membuat Akun & Mendapatkan Access Key:
 *   1. Buka https://antares.id → Daftar (gratis!)
 *   2. Login → Dashboard
 *   3. Klik nama profil (kanan atas) → Salin Access Key
 *   4. Buat Application baru (misal: "bluino-kit")
 *   5. Di dalam Application → buat Device (misal: "sensor-ruangan")
 *   6. Isi ACCESS_KEY, APP_NAME, DEVICE_NAME di bawah
 *
 * Library yang dibutuhkan:
 *   - WiFiClientSecure.h, HTTPClient.h, WiFi.h  (bawaan ESP32 Core)
 *   - ArduinoJson >= 6.x                         (Library Manager)
 *   - DHT sensor library by Adafruit             (Library Manager)
 *   - Adafruit Unified Sensor                    (Library Manager)
 *   - Adafruit SSD1306 + Adafruit GFX            (Library Manager)
 *
 * ⚠️ ESP32 hanya mendukung WiFi 2.4 GHz!
 * ⚠️ Ganti SEMUA konfigurasi di bawah sebelum upload!
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Konfigurasi WiFi ──────────────────────────────────────────────
const char* WIFI_SSID = "NamaWiFiKamu";   // ← Ganti!
const char* WIFI_PASS = "PasswordWiFi";   // ← Ganti!
const char* HOSTNAME  = "bluino";

// ── Konfigurasi Antares.id ────────────────────────────────────────
// Daftar gratis di https://antares.id
const char* ACCESS_KEY  = "ACCESSKEY-ANDA-DISINI";  // ← Ganti!
const char* APP_NAME    = "bluino-kit";              // ← Sesuaikan!
const char* DEVICE_NAME = "sensor-ruangan";          // ← Sesuaikan!

const unsigned long POST_INTERVAL = 30000UL; // 30 detik antar POST

// ── Pin ──────────────────────────────────────────────────────────
#define DHT_PIN  27
#define DHT_TYPE DHT11

// ── OLED ──────────────────────────────────────────────────────────
#define OLED_W    128
#define OLED_H     64
#define OLED_ADDR 0x3C

// ── Root CA — DigiCert Global Root G2 ────────────────────────────
// platform.antares.id menggunakan sertifikat yang ditandatangani oleh
// DigiCert Global Root G2 (per April 2026).
//
// Cara verifikasi (jika sertifikat berubah / expired):
//   openssl s_client -connect platform.antares.id:8443 -showcerts 2>/dev/null
//   → Salin sertifikat Root terakhir (paling atas di chain)
//
// ⚠️ Jika koneksi error kode -1 setelah waktu lama → Root CA mungkin expired!
//    Update sertifikat ini lalu re-upload firmware (atau gunakan OTA).
static const char ANTARES_ROOT_CA[] PROGMEM = R"CERT(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo2YwZDAO
BgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwHwYDVR0jBBgwFoAUTiJUIBiV5uNu5g/6+rkS7QYXjzk
wDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY1Yl9PMWLSn/pvtsrF9+
wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4NeF22d+mQrvHRAiGfzZ
0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NGFdtom/DzMNU+MeKNhJ7
jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ918rGOmaUYqty1wkez/vs
jUAksr6W02EItbgp5CY/v8A2f0Bz3YFOwAiX6YE/pFoiHe4jDeD392faAGANOtkq
iBM5dej7d7E5X9OoUMJnoMzG7GchT2jEObh622sTnZU=
-----END CERTIFICATE-----
)CERT";

// ── Objek ────────────────────────────────────────────────────────
DHT dht(DHT_PIN, DHT_TYPE);
Adafruit_SSD1306 oled(OLED_W, OLED_H, &Wire, -1);
bool oledReady = false;

// ── State ─────────────────────────────────────────────────────────
float    lastTemp   = NAN;
float    lastHumid  = NAN;
bool     sensorOk   = false;
uint32_t uploadCount = 0;
bool     lastUploadOk = false;

// ─────────────────────────────────────────────────────────────────
// OLED Helpers
// ─────────────────────────────────────────────────────────────────
void oledShowBoot() {
  if (!oledReady) return;
  oled.clearDisplay();
  oled.setTextSize(1); oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.println("Bluino IoT Kit");
  oled.println("BAB 49 - Program 3");
  oled.println();
  oled.print("App: "); oled.println(APP_NAME);
  oled.print("Dev: "); oled.println(DEVICE_NAME);
  oled.println("Menghubungkan WiFi...");
  oled.display();
}

void oledShowTelemetry() {
  if (!oledReady) return;
  oled.clearDisplay();
  oled.setTextSize(1); oled.setTextColor(SSD1306_WHITE);

  oled.setCursor(0, 0);
  oled.println("Antares.id Monitor");
  oled.drawLine(0, 10, OLED_W - 1, 10, SSD1306_WHITE);

  if (sensorOk) {
    oled.setTextSize(2);
    oled.setCursor(0, 14);
    oled.printf("%.1fC", lastTemp);
    oled.setTextSize(1);
    oled.setCursor(70, 14);
    oled.printf("H:%.0f%%", lastHumid);
  } else {
    oled.setCursor(0, 14);
    oled.println("Sensor Error!");
    oled.println("Cek IO27");
  }

  oled.setTextSize(1);
  oled.setCursor(0, 38);
  oled.printf("Upload: #%lu", uploadCount);
  oled.setCursor(0, 50);
  oled.print(lastUploadOk ? "Status: OK ✓" : "Status: GAGAL ✗");
  oled.display();
}

// ─────────────────────────────────────────────────────────────────
// Bangun URL endpoint Antares dan kirim data via HTTPS POST
// Format: m2m:cin oneM2M
// ─────────────────────────────────────────────────────────────────
void postToAntares() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HTTPS] ⚠️ WiFi tidak terhubung, skip POST.");
    return;
  }

  // Bangun URL: https://platform.antares.id:8443/~/antares-cse/antares-id/{APP}/{DEV}
  char url[192];
  snprintf(url, sizeof(url),
           "https://platform.antares.id:8443/~/antares-cse/antares-id/%s/%s",
           APP_NAME, DEVICE_NAME);

  // ── Bangun Payload JSON ───────────────────────────────────────
  // Langkah 1: bangun inner JSON (data sensor) — stack allocated
  StaticJsonDocument<128> innerDoc;
  innerDoc["temp"]      = sensorOk ? lastTemp  : (float)NAN;
  innerDoc["humidity"]  = sensorOk ? lastHumid : (float)NAN;
  innerDoc["sensorOk"]  = sensorOk;
  innerDoc["uptime"]    = millis() / 1000UL;
  innerDoc["upload"]    = uploadCount + 1;

  char innerJson[192]; // 192B: headroom aman untuk 5 field sensor (worst-case ~110 chars)
  serializeJson(innerDoc, innerJson, sizeof(innerJson));

  // Langkah 2: bungkus dalam format oneM2M m2m:cin
  // "con" (content) harus berupa string JSON yang di-escape, bukan objek JSON!
  StaticJsonDocument<256> outerDoc;
  outerDoc["m2m:cin"]["con"] = innerJson;

  char body[256];
  size_t bodyLen = serializeJson(outerDoc, body, sizeof(body));

  // ── HTTP POST ke Antares ──────────────────────────────────────
  Serial.printf("[HTTPS] POST → %s\n", url);
  Serial.printf("[HTTPS] Body: %s\n", body);

  WiFiClientSecure secureClient;
  secureClient.setCACert(ANTARES_ROOT_CA);

  HTTPClient http;
  http.begin(secureClient, url);
  http.setTimeout(10000);
  http.addHeader("X-M2M-Origin",  ACCESS_KEY);
  http.addHeader("Content-Type",  "application/json;ty=4");
  http.addHeader("Accept",        "application/json");
  http.addHeader("User-Agent",    "Bluino-IoT/2026 (ESP32)");

  // Menggunakan bodyLen (return value serializeJson) — eliminasi O(N) strlen()
  int code = http.POST((uint8_t*)body, bodyLen);

  if (code != 201) { // Antares merespons 201 Created, bukan 200 OK!
    Serial.printf("[HTTPS] ❌ Error: %d (%s)\n", code, http.errorToString(code).c_str());
    if (code < 0)
      Serial.println("[TLS] ⚠️ Periksa ACCESS_KEY, APP_NAME, DEVICE_NAME, atau Root CA!");
    lastUploadOk = false;
    http.end();
    oledShowTelemetry();
    return;
  }

  http.end();
  uploadCount++;
  lastUploadOk = true;

  Serial.printf("[HTTPS] ✅ 201 Created — Upload #%lu berhasil!\n", uploadCount);
  Serial.println("[INFO] Cek data di: https://antares.id → Dashboard Anda");

  oledShowTelemetry();
}

// ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  dht.begin();
  Wire.begin(21, 22);
  delay(500);

  Serial.println("\n=== BAB 49 Program 3: HTTPS POST — Antares.id ===");
  Serial.printf("[INFO] Endpoint: platform.antares.id:8443\n");
  Serial.printf("[INFO] App: %s | Device: %s\n", APP_NAME, DEVICE_NAME);

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
  Serial.printf("[INFO] POST tiap %lu detik ke Antares\n", POST_INTERVAL / 1000UL);

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

  // ── POST ke Antares Non-Blocking ─────────────────────────────
  static unsigned long tPost = 0;
  if (millis() - tPost >= POST_INTERVAL) {
    tPost = millis();
    Serial.printf("[DHT] %.1f°C | %.0f%%\n", lastTemp, lastHumid);
    postToAntares();
  }

  // ── Deteksi Transisi WiFi ─────────────────────────────────────
  static bool prevWiFi = true;
  bool currWiFi = (WiFi.status() == WL_CONNECTED);
  if (!prevWiFi && currWiFi)
    Serial.printf("[WiFi] ✅ Reconnect! IP: %s\n", WiFi.localIP().toString().c_str());
  if (prevWiFi && !currWiFi)
    Serial.println("[WiFi] ⚠️ Koneksi terputus...");
  prevWiFi = currWiFi;

  // ── Heartbeat (tiap 60 detik) ─────────────────────────────────
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] %.1fC %.0f%% | Upload: %lu | Heap: %u B\n",
                  lastTemp, lastHumid, uploadCount, ESP.getFreeHeap());
  }
}
