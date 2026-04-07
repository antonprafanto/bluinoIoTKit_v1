/*
 * BAB 48 - Program 1: HTTP GET — Cuaca Real-Time di OLED
 * ────────────────────────────────────────────────────────
 * Fitur:
 *   ▶ HTTP GET ke wttr.in → ambil data cuaca kota dalam format JSON
 *   ▶ Parse JSON dengan ArduinoJson (StaticJsonDocument — stack only)
 *   ▶ Tampilkan suhu, kondisi, kelembaban di OLED 128×64 I2C (IO21/IO22)
 *   ▶ Non-blocking: fetch tiap FETCH_INTERVAL (default: 10 menit)
 *   ▶ Error handling: timeout, DNS fail, HTTP code != 200
 *   ▶ mDNS: http://bluino.local
 *
 * Cara Uji:
 *   1. Install library berikut via Arduino Library Manager:
 *      - "ArduinoJson" by bblanchon >= 6.x
 *      - "Adafruit SSD1306" by Adafruit
 *      - "Adafruit GFX Library" by Adafruit
 *   2. Ganti WIFI_SSID, WIFI_PASS, dan CITY di bawah
 *   3. Upload, buka Serial Monitor (115200 baud)
 *   4. Tunggu 3–5 detik → data cuaca muncul di OLED dan Serial
 *
 * Library yang dibutuhkan:
 *   - HTTPClient.h, WiFi.h, ESPmDNS.h     (bawaan ESP32 Core)
 *   - ArduinoJson    >= 6.x               (install via Library Manager)
 *   - Adafruit_SSD1306 + Adafruit_GFX    (install via Library Manager)
 *
 * ⚠️ ESP32 hanya mendukung WiFi 2.4 GHz!
 * ⚠️ Ganti SSID dan PASSWORD sebelum upload!
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Konfigurasi ───────────────────────────────────────────────────
const char* WIFI_SSID  = "NamaWiFiKamu";  // ← Ganti!
const char* WIFI_PASS  = "PasswordWiFi";  // ← Ganti!
const char* MDNS_NAME  = "bluino";
const char* CITY       = "Jakarta";       // ← Ganti nama kota!

// ── API Endpoint ──────────────────────────────────────────────────
// wttr.in: layanan cuaca gratis, tanpa API key, format JSON
// Dokumentasi: https://wttr.in/:help
const unsigned long FETCH_INTERVAL = 600000UL; // 10 menit antar fetch

// ── OLED ──────────────────────────────────────────────────────────
#define OLED_W    128
#define OLED_H     64
#define OLED_ADDR 0x3C  // Coba 0x3D jika blank

Adafruit_SSD1306 oled(OLED_W, OLED_H, &Wire, -1);

// ── State Cuaca ───────────────────────────────────────────────────
struct WeatherData {
  int     tempC    = -999;
  int     humidity = -1;
  char    desc[32] = "---";
  bool    valid    = false;
};

WeatherData wx;

// ── Helper ────────────────────────────────────────────────────────
void oledShowBooting() {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.println("Bluino IoT Kit");
  oled.println("BAB 48 - HTTP GET");
  oled.println();
  oled.println("Menghubungkan WiFi...");
  oled.display();
}

void oledShowWeather() {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);

  // Judul
  oled.setCursor(0, 0);
  oled.print("Cuaca: ");
  oled.println(CITY);
  oled.drawLine(0, 10, OLED_W - 1, 10, SSD1306_WHITE);

  if (!wx.valid) {
    oled.setCursor(0, 16);
    oled.println("Mengambil data...");
    oled.display();
    return;
  }

  // Suhu (besar)
  oled.setTextSize(3);
  oled.setCursor(0, 16);
  oled.print(wx.tempC);
  oled.print((char)247); // derajat ° (char 247 di font Adafruit)
  oled.print("C");

  // Kelembaban
  oled.setTextSize(1);
  oled.setCursor(0, 48);
  oled.printf("Hum: %d%%  ", wx.humidity);

  // Kondisi
  oled.setCursor(0, 56);
  oled.print(wx.desc);

  oled.display();
}

void oledShowError(const char* msg) {
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.println("HTTP Error!");
  oled.println();
  oled.println(msg);
  oled.display();
}

// ─────────────────────────────────────────────────────────────────
// Fetch cuaca dari wttr.in dan parse JSON
// Menggunakan StaticJsonDocument (stack) — Zero Heap Fragmentation
// ─────────────────────────────────────────────────────────────────
void fetchWeather() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HTTP] ⚠️ WiFi tidak terhubung, skip fetch.");
    return;
  }

  // Bangun URL: http://wttr.in/Jakarta?format=j1
  char url[96];
  snprintf(url, sizeof(url), "http://wttr.in/%s?format=j1", CITY);
  Serial.printf("[HTTP] GET %s\n", url);

  HTTPClient http;
  WiFiClient client;
  http.begin(client, url);
  http.setTimeout(8000);                   // 8 detik timeout — WAJIB!
  http.addHeader("User-Agent", "Bluino-IoT/2026 (ESP32)"); // Sopan ke server gratis :)

  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("[HTTP] ❌ Error: %d (%s)\n", code, http.errorToString(code).c_str());
    oledShowError(http.errorToString(code).c_str());
    http.end();
    return;
  }

  // ── Parse JSON dengan ArduinoJson ────────────────────────────
  // Ukuran buffer: hitung dengan ArduinoJson Assistant di arduinojson.org/v6/assistant
  // wttr.in/?format=j1 menghasilkan response ~3KB; kita hanya butuh field teratas
  // Filter digunakan agar hanya field yang dibutuhkan dimasukkan ke dokumen
  StaticJsonDocument<256> filter;
  filter["current_condition"][0]["temp_C"]         = true;
  filter["current_condition"][0]["humidity"]       = true;
  filter["current_condition"][0]["weatherDesc"][0]["value"] = true;

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter));
  http.end();

  if (err) {
    Serial.printf("[HTTP] ❌ JSON Parse Error: %s\n", err.c_str());
    oledShowError("JSON parse error");
    return;
  }

  // Ekstrak data dari JSON bersarang
  JsonObject cur = doc["current_condition"][0];
  wx.tempC    = cur["temp_C"].as<int>();
  wx.humidity = cur["humidity"].as<int>();

  // Salin deskripsi cuaca dengan aman ke char[] yang dibatasi panjangnya
  const char* rawDesc = cur["weatherDesc"][0]["value"] | "Unknown";
  strncpy(wx.desc, rawDesc, sizeof(wx.desc) - 1);
  wx.desc[sizeof(wx.desc) - 1] = '\0';
  wx.valid = true;

  Serial.printf("[HTTP] ✅ 200 OK | Suhu: %d°C | Kondisi: %s | Kelembaban: %d%%\n",
                wx.tempC, wx.desc, wx.humidity);

  oledShowWeather();
}

// ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // SDA=IO21, SCL=IO22 (hardwired di Bluino Kit)

  Serial.println("\n=== BAB 48 Program 1: HTTP GET — Cuaca Real-Time ===");

  // ── OLED ──────────────────────────────────────────────────────
  if (oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("[OLED] ✅ Display terdeteksi di 0x3C");
    oledShowBooting();
  } else {
    Serial.println("[OLED] ⚠️ Display tidak ditemukan — lanjut tanpa OLED.");
  }

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

  // Fetch pertama saat boot
  fetchWeather();
}

// ─────────────────────────────────────────────────────────────────
void loop() {
  // ── Fetch cuaca Non-Blocking (tiap FETCH_INTERVAL) ───────────
  static unsigned long tFetch = 0;
  if (millis() - tFetch >= FETCH_INTERVAL) {
    tFetch = millis();
    fetchWeather();
  }

  // ── Deteksi Transisi WiFi ─────────────────────────────────────
  static bool prevWiFi = true;
  bool currWiFi = (WiFi.status() == WL_CONNECTED);
  if (!prevWiFi && currWiFi) {
    Serial.printf("[WiFi] ✅ Reconnect! IP: %s\n", WiFi.localIP().toString().c_str());
    fetchWeather(); // Fetch ulang begitu WiFi pulih
  }
  if (prevWiFi && !currWiFi)
    Serial.println("[WiFi] ⚠️ Koneksi terputus...");
  prevWiFi = currWiFi;

  // ── Heartbeat (tiap 60 detik) ──────────────────────────────────
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] Cuaca: %d°C %s | Heap: %u B | Uptime: %lu s\n",
                  wx.tempC, wx.desc, ESP.getFreeHeap(), millis() / 1000UL);
  }
}
