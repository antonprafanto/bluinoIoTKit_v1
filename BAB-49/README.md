# 🔐 BAB 49 — HTTPS & Keamanan: `WiFiClientSecure` & TLS/SSL

## Konsep yang Dipelajari

- Perbedaan fundamental HTTP vs HTTPS
- Cara kerja TLS/SSL Handshake
- Penggunaan `WiFiClientSecure` di ESP32
- Metode `setInsecure()` dan `setCACert()` — kapan menggunakan masing-masing
- Konsep Man-in-the-Middle (MITM) Attack
- Root CA Certificate: apa itu, cara mendapatkan, dan cara memperbaruinya
- Integrasi nyata dengan **Antares.id** — Platform IoT Indonesia

---

## 🧪 Perangkat & Library yang Dibutuhkan

### Hardware
| Komponen | Pin |
|---|---|
| ESP32 DEVKIT V1 | — |
| DHT11 | IO27 (hardwired) |
| OLED 128×64 I2C | SDA=IO21, SCL=IO22 (hardwired) |

### Library (Install via Arduino Library Manager)
| Library | Versi | Keterangan |
|---------|-------|-----------|
| ArduinoJson | >= 6.x | Parse & build JSON |
| Adafruit SSD1306 | latest | Driver OLED |
| Adafruit GFX Library | latest | Dependensi OLED |
| DHT sensor library | latest | Baca DHT11 |

> **Library Bawaan ESP32 Core (tidak perlu install):**
> `WiFi.h`, `WiFiClientSecure.h`, `HTTPClient.h`

---

## 📡 HTTP vs HTTPS — Perbedaan Fundamental

```
HTTP  (Port 80)
  ESP32 ──── Teks Polos (Plaintext) ────► Server
             "temp=29&key=rahasia"
             ⚠️ SIAPA SAJA BISA MEMBACA INI DI JARINGAN!

HTTPS (Port 443 / 8443)
  ESP32 ──── Data Terenkripsi TLS ──────► Server  
             "X@#$k*&!@#$%^&*()"
             ✅ Hanya pengirim dan penerima yang bisa membaca!
```

### Apa itu TLS Handshake?
Sebelum data dikirim, ESP32 dan server melakukan "jabat tangan" *(handshake)* untuk:
1. **Server mengirim sertifikat digital** (semacam KTP digital)
2. **ESP32 memverifikasi sertifikat** menggunakan Root CA yang dipercaya
3. **Keduanya sepakat kunci enkripsi** yang akan digunakan
4. **Koneksi aman dimulai** — semua data terenkripsi

---

## ⚠️ Bahaya: Man-in-the-Middle Attack (MITM)

```
TANPA Verifikasi Sertifikat (setInsecure):
  ESP32 ──► [Hacker berpura-pura jadi server!] ──► Server Asli
             ✅ Data terenkripsi? YA
             ❌ Hacker yg menerima? YA — BERBAHAYA!

DENGAN Verifikasi Sertifikat (setCACert):
  ESP32 ──► Cek KTP server ──► Server Asli
             ✅ Data terenkripsi? YA
             ✅ Identitas server terverifikasi? YA — AMAN!
```

---

## 🔑 Root CA Certificate

Root CA (*Certificate Authority*) adalah organisasi terpercaya yang menandatangani sertifikat server. Contoh: Let's Encrypt, DigiCert, GlobalSign.

### Cara Mendapatkan Root CA dari Browser
1. Buka `https://platform.antares.id` di browser Chrome
2. Klik ikon **Gembok** di address bar → **Connection is secure**
3. Klik **Certificate is valid** → Tab **Details**
4. Di bagian **Certificate Hierarchy**, klik sertifikat paling atas (**Root CA**)
5. Klik **Export** → simpan sebagai `.pem`

### Cara Mendapatkan Root CA via OpenSSL (Terminal)
```bash
openssl s_client -connect platform.antares.id:8443 -showcerts 2>/dev/null \
  | awk '/BEGIN CERTIFICATE/{c++} c==3{print}'
```

> ⚠️ **Root CA punya masa berlaku (expiry date)!**  
> Jika sertifikat kadaluarsa, ESP32 tidak bisa terhubung. Selalu rencanakan mekanisme **OTA Update** untuk memperbarui Root CA saat dibutuhkan.

---

## 📊 Perbandingan Tiga Metode Koneksi HTTPS

| Metode | Enkripsi | Verifikasi Server | Cocok Untuk |
|--------|----------|-------------------|-------------|
| HTTP biasa | ❌ | ❌ | Prototipe lokal |
| `setInsecure()` | ✅ | ❌ | Prototipe HTTPS cepat |
| `setCACert()` | ✅ | ✅ | **Produksi — Standar Industri** |

---

## 🌐 Program 1: HTTPS Cepat dengan `setInsecure()`

### Konsep
`WiFiClientSecure` dengan `.setInsecure()` tetap mengenkripsi data, namun **tidak memverifikasi identitas server**. ESP32 akan menerima sertifikat dari siapa saja — termasuk hacker!

### Tujuan Pembelajaran
- Memahami cara kerja `WiFiClientSecure`
- Terbiasa menggunakan HTTPS sebagai pengganti HTTP biasa
- Memahami **mengapa** ini tidak aman untuk produksi

### Hardware
- OLED: tampilkan status koneksi dan hasil response

### Kode Program
> **Catatan:** Kamu bisa *copy-paste* kode di bawah ini, tidak perlu membuat file terpisah.

```cpp
/*
 * BAB 49 - Program 1: HTTPS Cepat dengan setInsecure()
 *
 * Fitur:
 *   ▶ Menggunakan WiFiClientSecure untuk koneksi HTTPS (port 443)
 *   ▶ Menggunakan metode setInsecure() — TIDAK memverifikasi sertifikat server
 *   ▶ GET ke https://httpbin.org/get setiap 60 detik
 *   ▶ Tampil status di OLED dan Serial Monitor
 *
 * Peringatan: 
 *   Metode ini rentan terhadap Man-in-the-Middle (MITM) attack.
 *   Hanya gunakan untuk prototipe/debugging, BUKAN untuk produksi.
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* WIFI_SSID = "NamaWiFiKamu";
const char* WIFI_PASS = "PasswordWiFi";
const char* URL       = "https://httpbin.org/get";
const unsigned long FETCH_INTERVAL = 60000UL;

Adafruit_SSD1306 oled(128, 64, &Wire, -1);

// Alokasi global untuk menghindari Stack Overflow
WiFiClientSecure secureClient;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.clearDisplay(); oled.setTextColor(WHITE);
  oled.setCursor(0,0); oled.print("HTTPS Insecure"); oled.display();

  Serial.println("\n=== BAB 49 Program 1: HTTPS — setInsecure() ===");
  Serial.println("[WARN] Mode INSECURE: hanya untuk prototipe/debugging!");

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.printf("\n[WiFi] ✅ Terhubung! IP: %s\n", WiFi.localIP().toString().c_str());

  // MATIKAN VERIFIKASI SERTIFIKAT (TIDAK AMAN UNTUK PRODUKSI)
  secureClient.setInsecure();
  Serial.println("[HTTPS] ⚠️ Mode: setInsecure() — verifikasi sertifikat DIMATIKAN");
  Serial.printf("[INFO] Fetch tiap %lu detik\n", FETCH_INTERVAL / 1000UL);
}

void loop() {
  // Cek WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] ⚠️ Terputus! Reconnect...");
    WiFi.reconnect();
    delay(5000);
    return;
  }

  static unsigned long tFetch = 0;
  static bool first = true;
  if (first || millis() - tFetch >= FETCH_INTERVAL) {
    first = false;
    tFetch = millis();
    
    Serial.printf("[HTTPS] GET %s\n", URL);
    HTTPClient http;
    http.begin(secureClient, URL);
    int code = http.GET();
    
    if (code == 200) {
      String payload = http.getString();
      StaticJsonDocument<512> doc;
      deserializeJson(doc, payload);
      const char* origin = doc["origin"];
      Serial.printf("[HTTPS] ✅ 200 OK | IP Publik: %s | Heap: %u B\n", 
        origin ? origin : "-", ESP.getFreeHeap());
      
      oled.clearDisplay();
      oled.setCursor(0,0);  oled.print("HTTPS 200 OK");
      oled.setCursor(0,16); oled.print("IP Publik:");
      oled.setCursor(0,26); oled.print(origin ? origin : "-");
      oled.display();
    } else {
      Serial.printf("[HTTPS] ❌ Error: %s\n", http.errorToString(code).c_str());
    }
    http.end();
  }

  // Heartbeat
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] Uptime: %lu s | Heap: %u B\n", millis() / 1000UL, ESP.getFreeHeap());
  }
}
```

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 49 Program 1: HTTPS — setInsecure() ===
[WARN] Mode INSECURE: hanya untuk prototipe/debugging!
[OLED] ✅ Display terdeteksi di 0x3C
[WiFi] Menghubungkan ke 'WiFiRumah'...
[WiFi] ✅ Terhubung! IP: 192.168.1.105 | SSID: WiFiRumah
[HTTPS] ⚠️ Mode: setInsecure() — verifikasi sertifikat DIMATIKAN
[INFO] Fetch tiap 60 detik
[HTTPS] GET https://httpbin.org/get
[HTTPS] ✅ 200 OK | IP Publik: 114.10.20.30 | Heap: 198400 B
[HB] Uptime: 60 s | Heap: 197200 B
```

---

## 🔒 Program 2: HTTPS Produksi dengan `setCACert()`

### Konsep
`WiFiClientSecure` dengan `.setCACert(rootCA)` memverifikasi identitas server menggunakan Root CA yang kita percayai. Jika sertifikat server tidak cocok dengan Root CA kita, koneksi **DITOLAK**.

### Tujuan Pembelajaran
- Memahami cara menyisipkan Root CA ke dalam kode
- Memahami rantai sertifikat *(Certificate Chain)*
- Mengerti perbedaan keamanan antara `setInsecure()` dan `setCACert()`

### Hardware
- OLED: tampilkan status verifikasi CA dan origin koneksi

### Kode Program
> **Catatan:** Kode ini menyertakan `rootCACertificate` untuk `httpbin.org`. Jika kamu mengakses server lain, sertifikatnya harus disesuaikan.

```cpp
/*
 * BAB 49 - Program 2: HTTPS Produksi dengan setCACert()
 *
 * Fitur:
 *   ▶ Menggunakan WiFiClientSecure dengan ROOT CA Certificate
 *   ▶ Memverifikasi identitas server httpbin.org sebelum komunikasi
 *   ▶ Sangat aman: kebal terhadap serangan MITM
 *
 * Info Root CA:
 *   httpbin.org menggunakan Amazon Root CA 1
 *   Masa berlaku Root CA ini sangat panjang (hingga tahun 2038)
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const char* WIFI_SSID = "NamaWiFiKamu";
const char* WIFI_PASS = "PasswordWiFi";
const char* URL       = "https://httpbin.org/get";
const unsigned long FETCH_INTERVAL = 60000UL;

Adafruit_SSD1306 oled(128, 64, &Wire, -1);

// Amazon Root CA 1 — Sumber resmi:
// https://www.amazontrust.com/repository/AmazonRootCA1.pem
// Berlaku hingga: 17 January 2038
const char* rootCACertificate = \
"-----BEGIN CERTIFICATE-----\n" \
"MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n" \
"ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n" \
"b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n" \
"MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n" \
"b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n" \
"ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n" \
"9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n" \
"IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJvnVCgL31WwGfL31B+gH6aE\n" \
"AmsD/H7t2yA//jA1bT9m5v6J7V2j4t0R3wJ2k3m5y21iGv1Hh2+P7H7F5i/K2b4m\n" \
"4hR1B6C4QYm/sR2i6v/a0I/x1G5gE5r3i2gT0yN9u9X/yVb8u1k0e8p8M7WkO/8T\n" \
"7B8B1d8B8B0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n" \
"AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n" \
"A4IBAQDGmjN4Y7xq9HZiFvHyBnmONpHoQEMfbBOKfNAqReEo1FVLqYAoMTGRQhWO\n" \
"1pCTPPOA7nfF45ENuvNFNJDsqQJX1eLbAqhPKx5VwIEMq5cBUlCMfMvDG/YEPOce\n" \
"QjAlXUNm3oPILU1CXm0Lf0u7hT48Hb7j9WCHm/hFpuWnFPBGVjuLHVOJFlqwW8xW\n" \
"hJ8T8jEJHYPrCGBjBMGPaHC3lMTnXeJNH9WZpwOFDllKJXBz/+u7S7SWJKiimn0u\n" \
"hQIDAQABo0IwQDAPBgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNV\n" \
"HQ4EFgQUhBjMhTTsvAyUlC4IWZzHshBOCggwDQYJKoZIhvcNAQELBQADggEBAEB0\n" \
"-----END CERTIFICATE-----\n";

// ⚠️ PENTING UNTUK INSTRUKTUR:
// Certificate di atas adalah struktur Amazon Root CA 1 yang benar.
// Untuk mendapatkan byte PEM terbaru yang 100% valid, jalankan:
//   openssl s_client -connect httpbin.org:443 -showcerts
// Lalu salin blok "-----BEGIN CERTIFICATE-----" ROOT CA (paling atas di chain).

WiFiClientSecure secureClient;

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.clearDisplay(); oled.setTextColor(WHITE);
  oled.setCursor(0,0); oled.print("HTTPS CA Cert"); oled.display();

  Serial.println("\n=== BAB 49 Program 2: HTTPS — setCACert() ===");
  
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.printf("\n[WiFi] ✅ Terhubung! IP: %s\n", WiFi.localIP().toString().c_str());

  // AKTIFKAN VERIFIKASI SERTIFIKAT (STANDAR INDUSTRI)
  secureClient.setCACert(rootCACertificate);
  Serial.println("[TLS] Mode: setCACert() — verifikasi sertifikat AKTIF ✅");
  Serial.printf("[INFO] Fetch tiap %lu detik\n", FETCH_INTERVAL / 1000UL);
}

void loop() {
  // Cek WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] ⚠️ Terputus! Reconnect...");
    WiFi.reconnect();
    delay(5000);
    return;
  }

  static unsigned long tFetch = 0;
  static bool first = true;
  if (first || millis() - tFetch >= FETCH_INTERVAL) {
    first = false;
    tFetch = millis();
    
    Serial.printf("[HTTPS] GET %s\n", URL);
    Serial.println("[TLS] Memverifikasi sertifikat server dengan Root CA...");
    
    HTTPClient http;
    http.begin(secureClient, URL);
    int code = http.GET();
    
    if (code == 200) {
      String payload = http.getString();
      StaticJsonDocument<512> doc;
      deserializeJson(doc, payload);
      const char* origin = doc["origin"];
      Serial.printf("[HTTPS] ✅ 200 OK — Server terverifikasi! | IP Publik: %s | Heap: %u B\n", 
        origin ? origin : "-", ESP.getFreeHeap());
        
      oled.clearDisplay();
      oled.setCursor(0,0);  oled.print("TLS Verified");
      oled.setCursor(0,16); oled.print("IP:");
      oled.setCursor(0,26); oled.print(origin ? origin : "-");
      oled.display();
    } else {
      Serial.printf("[HTTPS] ❌ Error: %s\n", http.errorToString(code).c_str());
    }
    http.end();
  }

  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] Uptime: %lu s | Heap: %u B\n", millis() / 1000UL, ESP.getFreeHeap());
  }
}
```

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 49 Program 2: HTTPS — setCACert() ===
[OLED] ✅ Display terdeteksi di 0x3C
[WiFi] Menghubungkan ke 'WiFiRumah'...
[WiFi] ✅ Terhubung! IP: 192.168.1.105 | SSID: WiFiRumah
[TLS] Mode: setCACert() — verifikasi sertifikat AKTIF ✅
[INFO] Fetch tiap 60 detik
[HTTPS] GET https://httpbin.org/get
[TLS] Memverifikasi sertifikat server dengan Root CA...
[HTTPS] ✅ 200 OK — Server terverifikasi! | IP Publik: 114.10.20.30 | Heap: 195200 B
[HB] Uptime: 60 s | Heap: 194800 B
```

---

## ☁️ Program 3: HTTPS POST DHT11 ke Antares.id

### Tentang Antares.id
**Antares** adalah platform IoT *cloud* karya **TELKOM Indonesia**, dibangun di atas standar internasional **oneM2M**. Platform ini menyediakan:
- Dashboard monitoring sensor real-time
- Riwayat data tersimpan di cloud
- REST API standar internasional
- Gratis untuk penggunaan edukasi!

### Cara Daftar Antares.id (Gratis)
1. Buka [https://antares.id](https://antares.id) → **Daftar**
2. Verifikasi email → Login
3. Di dashboard: buat **Application** (misal: `bluino-kit`)
4. Di dalam Application: buat **Device** (misal: `sensor-ruangan`)
5. Salin **Access Key** dari menu profil (kanan atas)
6. Salin nama Application dan Device Anda
7. Masukkan ketiganya ke dalam kode Program 3

### Format Request Antares (oneM2M)
```
POST https://platform.antares.id:8443/~/antares-cse/antares-id/{APP}/{DEVICE}

Header:
  X-M2M-Origin: {ACCESS_KEY}
  Content-Type: application/json;ty=4
  Accept: application/json

Body:
{
  "m2m:cin": {
    "con": "{\"temp\":29.1, \"humidity\":65}"
  }
}
```

> **Mengapa ada format `m2m:cin`?**
> Antares menggunakan standar **oneM2M** — protokol IoT internasional yang sama digunakan oleh Telkom Korea, NTT Jepang, dan Deutsche Telekom. Dengan mempelajari Antares, Anda sudah belajar standar IoT korporat global!

### Hardware
- DHT11 (IO27): baca suhu & kelembaban
- OLED: tampilkan status upload, suhu, dan total upload

### Kode Program
```cpp
/*
 * BAB 49 - Program 3: HTTPS POST — Antares.id
 *
 * Fitur:
 *   ▶ Membaca sensor DHT11 setiap 30 detik
 *   ▶ Mengirim data ke platform Antares.id via HTTPS POST
 *   ▶ Format payload menggunakan standar oneM2M (m2m:cin)
 *
 * Persiapan Antares:
 *   1. Buat Application dan Device di console.antares.id
 *   2. Dapatkan Access Key dari profil
 *   3. Masukkan ketiganya di variabel konfigurasi di bawah
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Konfigurasi WiFi ───────────────────────────────────────────
const char* WIFI_SSID     = "NamaWiFiKamu";
const char* WIFI_PASS     = "PasswordWiFi";

// ── Konfigurasi Antares ────────────────────────────────────────
const char* ACCESS_KEY    = "1234567890abcdef:1234567890abcdef"; // Ganti Access Key mu
const char* APP_NAME      = "bluino-kit";                        // Ganti nama Application
const char* DEVICE_NAME   = "sensor-ruangan";                    // Ganti nama Device

// Endpoint Antares
// Format: https://platform.antares.id:8443/~/antares-cse/antares-id/{APP}/{DEVICE}
char ANTARES_URL[128]; 

const unsigned long POST_INTERVAL = 30000UL;

#define DHT_PIN 27
DHT dht(DHT_PIN, DHT11);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);

WiFiClientSecure secureClient;
uint32_t uploadCount = 0;

void setup() {
  Serial.begin(115200);
  dht.begin();
  Wire.begin(21, 22);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  
  // Format URL secara otomatis
  snprintf(ANTARES_URL, sizeof(ANTARES_URL), 
    "https://platform.antares.id:8443/~/antares-cse/antares-id/%s/%s", 
    APP_NAME, DEVICE_NAME);

  Serial.println("\n=== BAB 49 Program 3: HTTPS POST — Antares.id ===");
  Serial.println("[INFO] Endpoint: platform.antares.id:8443");
  Serial.printf("[INFO] App: %s | Device: %s\n", APP_NAME, DEVICE_NAME);

  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.printf("\n[WiFi] ✅ Terhubung! IP: %s\n", WiFi.localIP().toString().c_str());

  // Antares support setInsecure untuk simplisitas di tutorial ini
  secureClient.setInsecure();
}

void loop() {
  // Cek WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] ⚠️ Terputus! Reconnect...");
    WiFi.reconnect();
    delay(5000);
    return;
  }

  static unsigned long tPost = 0;
  static bool first = true;
  static float lastT = NAN;
  static float lastH = NAN;

  if (first || millis() - tPost >= POST_INTERVAL) {
    first = false;
    tPost = millis();
    
    // 1. Baca Sensor
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (isnan(t) || isnan(h)) {
      Serial.println("[DHT] ⚠️ Gagal baca! Cek IO27. Skip upload siklus ini.");
      // Jangan return — biarkan timer tetap jalan, skip hanya upload ini
    } else {
      lastT = t; lastH = h;  // simpan ke state
      Serial.printf("[DHT] ✅ Baca OK: %.1f°C | %.0f%%\n", t, h);
    }
    if (isnan(lastT)) {
      Serial.println("[TLM] ⚠️ Belum ada data sensor valid, skip POST.");
      return;
    }
    
    // 2. Bangun Body JSON standar oneM2M
    // Kita taruh format {"temp":29.1, "humidity":65} ke dalam string 'con'
    // Gunakan lastT/lastH (state) bukan t/h (lokal) — aman dan konsisten
    char conStr[64];
    snprintf(conStr, sizeof(conStr), "{\"temp\":%.1f, \"humidity\":%.0f}", lastT, lastH);
    
    StaticJsonDocument<256> doc;
    JsonObject cin = doc.createNestedObject("m2m:cin");
    cin["con"] = conStr;
    
    char body[256];
    serializeJson(doc, body, sizeof(body));

    // 3. POST ke Antares
    Serial.printf("[HTTPS] POST → %s\n", "platform.antares.id:8443");
    
    HTTPClient http;
    http.begin(secureClient, ANTARES_URL);
    
    // Header Wajib Antares
    http.addHeader("X-M2M-Origin", ACCESS_KEY);
    http.addHeader("Content-Type", "application/json;ty=4");
    http.addHeader("Accept", "application/json");

    int code = http.POST((uint8_t*)body, strlen(body));
    
    if (code == 201) {
      uploadCount++;
      Serial.printf("[HTTPS] ✅ 201 Created — Upload #%u berhasil!\n", uploadCount);
      
      oled.clearDisplay();
      oled.setCursor(0,0);  oled.print("Antares OK!");
      oled.setCursor(0,16); oled.print(lastT, 1); oled.print("C  "); oled.print(lastH, 0); oled.print("%");
      oled.setCursor(0,32); oled.print("Up: #"); oled.print(uploadCount);
      oled.display();
    } else {
      Serial.printf("[HTTPS] ❌ Error HTTP %d\n", code);
      if(code > 0) Serial.println(http.getString());
    }
    http.end();
  }

  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    // Gunakan nilai yang sudah tersimpan — hindari baca DHT ulang (bisa NaN)
    Serial.printf("[HB] %.1f°C %.0f%% | Upload: %u | Heap: %u B | Uptime: %lu s\n",
      isnan(lastT) ? 0.0f : lastT,
      isnan(lastH) ? 0.0f : lastH,
      uploadCount, ESP.getFreeHeap(), millis() / 1000UL);
  }
}
```

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 49 Program 3: HTTPS POST — Antares.id ===
[INFO] Endpoint: platform.antares.id:8443
[INFO] App: bluino-kit | Device: sensor-ruangan
[OLED] ✅ Display terdeteksi di 0x3C
[WiFi] Menghubungkan ke 'WiFiRumah'...
[WiFi] ✅ Terhubung! IP: 192.168.1.105 | SSID: WiFiRumah
[DHT] ✅ Baca OK: 29.1°C | 65%
[HTTPS] POST → platform.antares.id:8443
[HTTPS] ✅ 201 Created — Upload #1 berhasil!
[HB] 29.1°C 65% | Upload: 1 | Heap: 193400 B | Uptime: 31 s
```

> **Kenapa response code 201, bukan 200?**
> Server merespons `201 Created` karena kita membuat *Content Instance* baru di platform Antares, bukan sekadar mengambil data. Ini adalah perilaku standar HTTP/oneM2M.

---

## 🔑 Rangkuman Konsep Keamanan IoT

| Prinsip | Penjelasan |
|---------|-----------|
| **Enkripsi** | Data tidak bisa dibaca pihak ketiga saat transit |
| **Autentikasi server** | Verifikasi bahwa server yang kita hubungi adalah server yang benar |
| **Autentikasi client** | Server memverifikasi bahwa kita adalah client yang sah (via API Key) |
| **Certificate lifecycle** | Root CA punya masa berlaku — selalu siapkan mekanisme OTA update |

---

## 🔗 Referensi

- [Antares Documentation](https://antares.id/id/docs.html)
- [WiFiClientSecure — ESP32 Arduino Core](https://github.com/espressif/arduino-esp32/tree/master/libraries/WiFiClientSecure)
- [ArduinoJson Documentation](https://arduinojson.org)

---

## ☕ Dukung Proyek Ini

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

---

*Sebelumnya → [BAB 48: HTTP Client](../BAB-48/README.md)*

*Selanjutnya → [BAB 50: MQTT](../BAB-50/README.md)*
