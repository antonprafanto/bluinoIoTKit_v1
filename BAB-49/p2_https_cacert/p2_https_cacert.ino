/*
 * BAB 49 - Program 2: HTTPS dengan setCACert() — Keamanan Produksi
 * ─────────────────────────────────────────────────────────────────
 * Fitur:
 *   ▶ HTTPS GET ke https://httpbin.org/get menggunakan WiFiClientSecure
 *   ▶ setCACert(HTTPBIN_ROOT_CA) — verifikasi identitas server secara kriptografis
 *   ▶ Jika sertifikat server tidak cocok → koneksi DITOLAK (anti MITM!)
 *   ▶ Parse response JSON dengan ArduinoJson (stack only, zero heap)
 *   ▶ Output ke OLED dan Serial Monitor
 *   ▶ Non-blocking: fetch tiap 60 detik
 *   ▶ Heartbeat log tiap 60 detik
 *
 * Root CA Certificate yang Digunakan:
 *   "ISRG Root X1" — Root CA dari Let's Encrypt (digunakan oleh httpbin.org)
 *   Diperoleh melalui: https://letsencrypt.org/certificates/
 *   Valid sampai: 2035-09-30 (cukup lama — tapi selalu siapkan OTA update!)
 *
 * Cara Mendapatkan Root CA Baru (jika expired):
 *   Opsi 1: Browser
 *     1. Buka https://httpbin.org di Chrome
 *     2. Klik gembok → Certificate is valid → Details
 *     3. Pilih sertifikat Root (paling atas) → Export as PEM
 *
 *   Opsi 2: OpenSSL (Terminal/PowerShell)
 *     openssl s_client -connect httpbin.org:443 -showcerts 2>/dev/null
 *     → Salin blok "-----BEGIN CERTIFICATE-----" yang terakhir (Root CA)
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
const unsigned long FETCH_INTERVAL = 60000UL;

// ── Root CA Certificate (ISRG Root X1 — Let's Encrypt) ───────────
// Sertifikat ini memverifikasi bahwa httpbin.org adalah server yang sah.
// Tanpa ini, ESP32 tidak bisa membedakan server asli dengan hacker!
//
// Cara membacanya: ESP32 memegang "KTP" penerbit sertifikat (Root CA)
// dan mencocokkannya dengan "KTP" yang ditunjukkan oleh server saat
// TLS handshake. Jika tidak cocok → koneksi GAGAL secara sengaja!
static const char HTTPBIN_ROOT_CA[] PROGMEM = R"CERT(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)CERT";

// ── OLED ──────────────────────────────────────────────────────────
#define OLED_W    128
#define OLED_H     64
#define OLED_ADDR 0x3C

Adafruit_SSD1306 oled(OLED_W, OLED_H, &Wire, -1);
bool oledReady = false;

// ── State ─────────────────────────────────────────────────────────
uint32_t fetchCount = 0;
char     lastOrigin[32] = "---";

// ─────────────────────────────────────────────────────────────────
// OLED Helpers
// ─────────────────────────────────────────────────────────────────
void oledShowBoot() {
  if (!oledReady) return;
  oled.clearDisplay();
  oled.setTextSize(1); oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.println("Bluino IoT Kit");
  oled.println("BAB 49 - Program 2");
  oled.println();
  oled.println("HTTPS setCACert()");
  oled.println("Anti-MITM Aktif!");
  oled.display();
}

void oledShowStatus(bool ok) {
  if (!oledReady) return;
  oled.clearDisplay();
  oled.setTextSize(1); oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.println("HTTPS: setCACert");
  oled.println(ok ? "[CA VERIFIED] ✓" : "[CA ERROR] ✗");
  oled.drawLine(0, 18, OLED_W - 1, 18, SSD1306_WHITE);
  oled.setCursor(0, 22);
  oled.println(ok ? "Server TERVERIFIKASI" : "Server DIBLOKIR!");
  oled.setCursor(0, 34);
  oled.print("GET #"); oled.println(fetchCount);
  oled.setCursor(0, 46);
  oled.print("IP Publik: ");
  oled.setCursor(0, 56);
  oled.println(lastOrigin);
  oled.display();
}

// ─────────────────────────────────────────────────────────────────
// Fetch HTTPS — menggunakan setCACert() [Standar Produksi]
// ─────────────────────────────────────────────────────────────────
void fetchHttps() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[HTTPS] ⚠️ WiFi tidak terhubung, skip fetch.");
    return;
  }

  Serial.printf("[HTTPS] GET %s\n", GET_URL);
  Serial.println("[TLS] Memverifikasi sertifikat server dengan Root CA...");

  WiFiClientSecure secureClient;
  // ← Perbedaan UTAMA dari Program 1: kita menyerahkan Root CA kita
  //   kepada mesin TLS untuk dicocokkan dengan sertifikat server
  secureClient.setCACert(HTTPBIN_ROOT_CA);

  HTTPClient http;
  http.begin(secureClient, GET_URL);
  http.setTimeout(10000);
  http.addHeader("User-Agent", "Bluino-IoT/2026 (ESP32)");

  int code = http.GET();

  if (code != HTTP_CODE_OK) {
    // Jika code == -1 (connection refused), kemungkinan Root CA tidak cocok!
    Serial.printf("[HTTPS] ❌ Error: %d (%s)\n", code, http.errorToString(code).c_str());
    if (code < 0) Serial.println("[TLS] ⚠️ Cek apakah Root CA sudah expired atau tidak sesuai!");
    oledShowStatus(false);
    http.end();
    return;
  }

  fetchCount++;

  // Parse: ambil hanya field "origin" dari response
  StaticJsonDocument<32> filter;
  filter["origin"] = true;

  StaticJsonDocument<128> doc;
  DeserializationError err = deserializeJson(doc, http.getStream(),
                                              DeserializationOption::Filter(filter));
  http.end();

  if (err) {
    Serial.printf("[HTTPS] ✅ %d OK — Server terverifikasi! (parse skip: %s)\n",
                  code, err.c_str());
    oledShowStatus(true);
    return;
  }

  const char* origin = doc["origin"] | "?";
  strncpy(lastOrigin, origin, sizeof(lastOrigin) - 1);
  lastOrigin[sizeof(lastOrigin) - 1] = '\0';

  Serial.printf("[HTTPS] ✅ %d OK — Server terverifikasi! | IP Publik: %s | Heap: %u B\n",
                code, lastOrigin, ESP.getFreeHeap());

  oledShowStatus(true);
}

// ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  Serial.println("\n=== BAB 49 Program 2: HTTPS — setCACert() ===");

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

  Serial.println("[TLS] Mode: setCACert() — verifikasi sertifikat AKTIF ✅");
  Serial.printf("[INFO] Fetch tiap %lu detik\n", FETCH_INTERVAL / 1000UL);

  fetchHttps();
}

// ─────────────────────────────────────────────────────────────────
void loop() {
  static unsigned long tFetch = 0;
  if (millis() - tFetch >= FETCH_INTERVAL) {
    tFetch = millis();
    fetchHttps();
  }

  static bool prevWiFi = true;
  bool currWiFi = (WiFi.status() == WL_CONNECTED);
  if (!prevWiFi && currWiFi) {
    Serial.printf("[WiFi] ✅ Reconnect! IP: %s\n", WiFi.localIP().toString().c_str());
    fetchHttps();
  }
  if (prevWiFi && !currWiFi)
    Serial.println("[WiFi] ⚠️ Koneksi terputus...");
  prevWiFi = currWiFi;

  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] Uptime: %lu s | Heap: %u B\n",
                  millis() / 1000UL, ESP.getFreeHeap());
  }
}
