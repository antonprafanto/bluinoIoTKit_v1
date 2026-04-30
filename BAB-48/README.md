# BAB 48: HTTP Client & REST API — ESP32 Pergi ke Internet

> ✅ **Prasyarat:** Selesaikan **BAB 47 (WebSocket)**. Kamu harus sudah memahami WiFi, mDNS, dan JSON.

> 📖 **Glosarium Singkatan BAB ini:**
> | Singkatan | Kepanjangan |
> |-----------|-------------|
> | **HTTP** | *HyperText Transfer Protocol* — protokol permintaan/respons web |
> | **REST** | *Representational State Transfer* — arsitektur API berbasis HTTP |
> | **GET** | Metode HTTP untuk *mengambil* data dari server |
> | **POST** | Metode HTTP untuk *mengirim* data ke server |
> | **JSON** | *JavaScript Object Notation* — format pertukaran data ringan |
> | **API** | *Application Programming Interface* — antarmuka layanan data |
> | **DNS** | *Domain Name System* — penerjemah nama domain ke IP address |
> | **TLS** | *Transport Layer Security* — protokol enkripsi (dibahas BAB 49) |

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami perbedaan peran ESP32 sebagai **Server** (BAB 46–47) vs **Client** (BAB 48).
- Menggunakan `HTTPClient.h` untuk mengirim request **GET** dan **POST**.
- Meng-*handle* error HTTP: DNS failure, timeout, HTTP error code.
- Mem-*parse* JSON response dari API publik menggunakan `ArduinoJson`.
- Membangun pola **Telemetri IoT**: baca sensor → format JSON → kirim ke cloud.
- Mengimplementasi **State Machine** sederhana untuk siklus request non-blocking.

---

## 48.1 Pergeseran Peran: Server → Client

```
BAB 46–47 — ESP32 sebagai SERVER:

  Browser/App                ESP32
  ──────────                 ─────
  │── GET /api/sensors ─────►│  ← ESP32 menunggu
  │◄─ JSON response ─────────│  ← ESP32 menjawab

  ESP32 hanya menunggu. Ia tidak bisa inisiasi komunikasi ke internet.


BAB 48 — ESP32 sebagai CLIENT:

  ESP32                      Server Internet
  ─────                      ──────────────
  │── GET api.weather.com ──►│  ← ESP32 yang meminta
  │◄─ JSON cuaca ────────────│  ← Server menjawab

  │── POST httpbin.org ─────►│  ← Kirim data sensor
  │◄─ HTTP 200 OK ───────────│  ← Konfirmasi diterima

  ESP32 AKTIF pergi ke internet untuk ambil atau kirim data.
  Ini adalah fondasi SEMUA perangkat IoT dunia nyata!
```

| Aspek | BAB 46–47 (Server) | BAB 48 (Client) |
|-------|-------------------|-----------------|
| **Inisiator** | Browser/Device lain | ESP32 sendiri |
| **Peran ESP32** | Menunggu permintaan | Aktif meminta |
| **Arah data** | Browser → ESP32 | ESP32 → Internet |
| **Buka port?** | Ya (port 80/81) | Tidak perlu |
| **Cocok untuk** | Dashboard lokal | Telemetri cloud, cuaca, API |

---

## 48.2 Anatomi HTTP Request & Response

```
HTTP GET REQUEST (ESP32 → Server):
──────────────────────────────────
GET /Jakarta?format=j1 HTTP/1.1
Host: wttr.in
User-Agent: ESP32HTTPClient/1.0
Connection: close
                     ↑ Tidak ada body pada GET

HTTP POST REQUEST (ESP32 → Server):
────────────────────────────────────
POST /post HTTP/1.1
Host: httpbin.org
Content-Type: application/json
Content-Length: 47
Connection: close

{"device":"bluino","temp":28.5,"upload":1}
       ↑ Body JSON dikirim ke server


HTTP RESPONSE (Server → ESP32):
────────────────────────────────
HTTP/1.1 200 OK
Content-Type: application/json
Content-Length: 312

{"temp":28.5,"humidity":65,...}

HTTP STATUS CODE PENTING:
  200 OK          → Sukses — respons valid
  201 Created     → Data baru berhasil dibuat di server
  400 Bad Request → Request kita salah format
  401 Unauthorized→ Perlu API key / autentikasi
  404 Not Found   → URL salah
  429 Too Many Req→ Rate limit kena — kurangi frekuensi
  500 Server Error→ Masalah di sisi server
```

---

## 48.3 Library & API Reference

```
LIBRARY YANG DIGUNAKAN:

  HTTPClient.h   → Bawaan ESP32 Arduino Core (tidak perlu install)
  ArduinoJson.h  → Install via Library Manager:
                   Cari "ArduinoJson" → pilih by bblanchon → versi >= 6.x


API HTTPClient ESENSIAL:
  http.begin(url)                      → Set URL tujuan
  http.setTimeout(ms)                  → Set batas waktu (WAJIB!)
  http.addHeader("key", "value")       → Tambah HTTP header
  int code = http.GET()                → Kirim GET request
  int code = http.POST(body)           → Kirim POST request
  String body = http.getString()       → Ambil body response (String)
  int size = http.getSize()            → Ukuran response
  http.end()                           → Tutup koneksi (WAJIB!)

HTTP ERROR CODE (code < 0):
  -1  → HTTPC_ERROR_CONNECTION_REFUSED  → Server tolak koneksi
  -2  → HTTPC_ERROR_SEND_HEADER_FAILED  → Gagal kirim header
  -3  → HTTPC_ERROR_SEND_PAYLOAD_FAILED → Gagal kirim body
  -4  → HTTPC_ERROR_NOT_CONNECTED       → Tidak terkoneksi
  -5  → HTTPC_ERROR_CONNECTION_LOST     → Koneksi putus di tengah
  -11 → HTTPC_ERROR_READ_TIMEOUT        → Timeout — tambah setTimeout


API ArduinoJson ESENSIAL:
  StaticJsonDocument<N> doc             → Alokasi di STACK (tidak heap)
  deserializeJson(doc, string)          → Parse JSON string → doc
  float v = doc["key"]                  → Akses nilai
  const char* s = doc["key"]            → Akses string
  doc["arr"][0]["field"]                → Akses array bersarang
  serializeJson(doc, Serial)            → Print JSON ke Serial
  serializeJson(doc, buf, sizeof(buf))  → Serialize ke char buffer


ATURAN StaticJsonDocument:
  StaticJsonDocument<256>   → JSON kecil (config, ACK)
  StaticJsonDocument<1024>  → JSON sedang (data cuaca 1 kota)
  StaticJsonDocument<4096>  → JSON besar (list data, forecast)
  Gunakan ArduinoJson Assistant untuk hitung ukuran yang tepat:
  → https://arduinojson.org/v6/assistant/
```

---

## 48.4 Program 1: HTTP GET — Cuaca Real-Time di Serial & OLED

ESP32 mengambil data cuaca kota dari API publik `wttr.in` setiap 10 menit dan menampilkannya di OLED serta Serial Monitor.

```cpp
/*
 * BAB 48 - Program 1: HTTP GET — Cuaca Real-Time
 *
 * Fitur:
 *   ▶ HTTP GET ke wttr.in (API cuaca gratis, tanpa API key)
 *   ▶ Parse JSON response dengan ArduinoJson
 *   ▶ Tampil di OLED SSD1306 128x64 (I2C 0x3C)
 *   ▶ Non-blocking: fetch tiap 10 menit via millis()
 *   ▶ Error handling: DNS fail, timeout, HTTP error code
 *   ▶ Heartbeat Serial tiap 60 detik
 *
 * Library yang dibutuhkan (Library Manager):
 *   → ArduinoJson by bblanchon (>= 6.x)
 *   → Adafruit SSD1306
 *   → Adafruit GFX Library
 *
 * Hardware:
 *   OLED SSD1306 128x64 → I2C SDA=IO21, SCL=IO22 (hardwired Bluino Kit)
 *
 * API yang digunakan:
 *   URL : http://wttr.in/{CITY}?format=j1
 *   Docs: https://wttr.in/:help
 *   Free: tidak perlu API key, batas 1 req/menit per IP
 *
 * Cara Uji:
 *   1. Install library di atas via Library Manager
 *   2. Ganti WIFI_SSID, WIFI_PASS, dan CITY
 *   3. Upload → Serial Monitor 115200 baud
 *   4. Tunggu ~5 detik → data cuaca muncul di OLED & Serial
 *   5. Coba CITY = "Surabaya", "Bali", "London"
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Konfigurasi ───────────────────────────────────────────────────
const char* WIFI_SSID      = "NamaWiFiKamu";   // ← Ganti!
const char* WIFI_PASS      = "PasswordWiFi";   // ← Ganti!
const char* CITY           = "Jakarta";         // ← Nama kota (English)
const unsigned long FETCH_INTERVAL = 600000UL;  // 10 menit (ms)
const int    HTTP_TIMEOUT   = 8000;             // 8 detik
const int    OLED_ADDR      = 0x3C;            // Coba 0x3D jika blank

// ── OLED ─────────────────────────────────────────────────────────
#define SCREEN_W 128
#define SCREEN_H 64
Adafruit_SSD1306 oled(SCREEN_W, SCREEN_H, &Wire, -1);

// ── State ────────────────────────────────────────────────────────
char   g_city[32]     = "-";
char   g_condition[48]= "-";
int    g_tempC        = 0;
int    g_humidity     = 0;
int    g_feelsLike    = 0;
bool   g_dataOk       = false;
uint32_t g_fetchCount = 0;

// ── Forward declarations ──────────────────────────────────────────
void fetchWeather();
void updateOLED();

// ── OLED: tampilkan data cuaca ────────────────────────────────────
void updateOLED() {
  oled.clearDisplay();
  oled.setTextColor(WHITE);

  if (!g_dataOk) {
    oled.setTextSize(1);
    oled.setCursor(0, 0);  oled.println("Bluino IoT Kit");
    oled.setCursor(0, 12); oled.println("BAB 48: HTTP GET");
    oled.setCursor(0, 28); oled.println("Mengambil data...");
    oled.setCursor(0, 44); oled.println("Harap tunggu ~5 detik");
    oled.display();
    return;
  }

  // Baris 1: Kota
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print("📍 "); oled.print(g_city);

  // Baris 2: Suhu besar
  oled.setTextSize(3);
  oled.setCursor(0, 12);
  oled.print(g_tempC); oled.print((char)247); oled.print("C");

  // Baris 3: Kondisi
  oled.setTextSize(1);
  oled.setCursor(0, 42);
  oled.print(g_condition);

  // Baris 4: Kelembaban & Feels Like
  oled.setCursor(0, 54);
  oled.print("H:"); oled.print(g_humidity); oled.print("%");
  oled.print(" FL:"); oled.print(g_feelsLike); oled.print((char)247); oled.print("C");

  oled.display();
}

// ── Fetch cuaca dari wttr.in ──────────────────────────────────────
void fetchWeather() {
  // Bangun URL: http://wttr.in/Jakarta?format=j1
  char url[128];
  snprintf(url, sizeof(url), "http://wttr.in/%s?format=j1", CITY);
  Serial.printf("[HTTP] GET %s\n", url);

  HTTPClient http;
  http.begin(url);
  http.setTimeout(HTTP_TIMEOUT);
  http.addHeader("User-Agent", "BluinoIoTKit/1.0");  // wttr.in butuh User-Agent

  int code = http.GET();

  if (code != HTTP_CODE_OK) {
    if (code < 0) {
      Serial.printf("[HTTP] ❌ Error koneksi: %s\n", http.errorToString(code).c_str());
    } else {
      Serial.printf("[HTTP] ❌ HTTP Code: %d\n", code);
    }
    http.end();
    g_dataOk = false;
    return;
  }

  // Ambil payload JSON sebagai String (aman untuk ukuran data cuaca ~2KB)
  String payload = http.getString();
  http.end();

  Serial.printf("[HTTP] ✅ %d OK | Payload: %d bytes\n", code, payload.length());

  // Parse JSON
  // wttr.in format=j1 response struktur:
  // {"current_condition":[{"temp_C":"29","FeelsLikeC":"32",
  //   "humidity":"74","weatherDesc":[{"value":"Partly cloudy"}]}]}
  StaticJsonDocument<2048> doc;
  DeserializationError err = deserializeJson(doc, payload);

  if (err) {
    Serial.printf("[JSON] ❌ Parse gagal: %s\n", err.c_str());
    g_dataOk = false;
    return;
  }

  // Ekstrak data
  JsonObject cur = doc["current_condition"][0];
  g_tempC     = cur["temp_C"].as<int>();
  g_humidity  = cur["humidity"].as<int>();
  g_feelsLike = cur["FeelsLikeC"].as<int>();

  const char* desc = cur["weatherDesc"][0]["value"];
  if (desc) {
    strncpy(g_condition, desc, sizeof(g_condition) - 1);
    g_condition[sizeof(g_condition) - 1] = '\0';
  }
  strncpy(g_city, CITY, sizeof(g_city) - 1);

  g_dataOk = true;
  g_fetchCount++;

  Serial.printf("[HTTP] ✅ Suhu: %d°C | Kondisi: %s | Kelembaban: %d%%\n",
    g_tempC, g_condition, g_humidity);

  updateOLED();
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== BAB 48 Program 1: HTTP GET — Cuaca Real-Time ===");

  // OLED Init
  Wire.begin(21, 22);
  if (oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.printf("[OLED] ✅ Display terdeteksi di 0x%02X\n", OLED_ADDR);
  } else {
    Serial.println("[OLED] ⚠️ Tidak terdeteksi! Cek kabel SDA/SCL atau alamat I2C.");
  }
  oled.clearDisplay();
  oled.setTextColor(WHITE);
  oled.setTextSize(1);
  oled.setCursor(0, 0); oled.println("Bluino IoT Kit");
  oled.setCursor(0, 12); oled.println("BAB 48: HTTP GET");
  oled.setCursor(0, 28); oled.println("Menghubungkan WiFi...");
  oled.display();

  // WiFi
  Serial.printf("[WiFi] Menghubungkan ke '%s'...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t tStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - tStart >= 15000UL) {
      Serial.println("\n[ERR] WiFi timeout! Restart.");
      delay(2000); ESP.restart();
    }
    Serial.print("."); delay(500);
  }
  Serial.printf("\n[WiFi] ✅ Terhubung! IP: %s | SSID: %s\n",
    WiFi.localIP().toString().c_str(), WiFi.SSID().c_str());

  // Fetch pertama langsung setelah konek
  updateOLED();
  fetchWeather();
}

void loop() {
  // Fetch cuaca setiap FETCH_INTERVAL (non-blocking)
  // tFetch diinisialisasi dengan millis() sehingga fetch berikutnya
  // dihitung dari waktu fetch pertama selesai di setup()
  static unsigned long tFetch = 0;
  static bool firstLoop = true;
  if (firstLoop) { tFetch = millis(); firstLoop = false; }

  if (millis() - tFetch >= FETCH_INTERVAL) {
    tFetch = millis();
    fetchWeather();
  }

  // Reconnect WiFi jika putus (non-blocking: cukup panggil reconnect,
  // WiFi.setAutoReconnect(true) di setup() akan handle sisanya)
  static unsigned long tRecon = 0;
  if (WiFi.status() != WL_CONNECTED && millis() - tRecon >= 10000UL) {
    tRecon = millis();
    Serial.println("[WiFi] ⚠️ Terputus! Mencoba reconnect...");
    WiFi.reconnect();
  }

  // Heartbeat setiap 60 detik
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    if (g_dataOk) {
      Serial.printf("[HB] Cuaca: %d°C %s | Fetch: %u | Heap: %u B | Uptime: %lu s\n",
        g_tempC, g_condition, g_fetchCount, ESP.getFreeHeap(), millis() / 1000UL);
    } else {
      Serial.printf("[HB] Data belum tersedia | Heap: %u B | Uptime: %lu s\n",
        ESP.getFreeHeap(), millis() / 1000UL);
    }
  }
}
```

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 48 Program 1: HTTP GET — Cuaca Real-Time ===
[OLED] ✅ Display terdeteksi di 0x3C
[WiFi] Menghubungkan ke 'WiFiRumah'..............
[WiFi] ✅ Terhubung! IP: 192.168.1.105 | SSID: WiFiRumah
[HTTP] GET http://wttr.in/Jakarta?format=j1
[HTTP] ✅ 200 OK | Payload: 1847 bytes
[HTTP] ✅ Suhu: 29°C | Kondisi: Partly cloudy | Kelembaban: 74%
[HB] Cuaca: 29°C Partly cloudy | Fetch: 1 | Heap: 189432 B | Uptime: 61 s
```

> ⚠️ **Catatan Penting:**
> - API `wttr.in` gratis dan tidak butuh API key.
> - Batas: **1 request per menit** per IP. Jangan set `FETCH_INTERVAL` di bawah 60 detik.
> - Jika OLED blank: jalankan sketch I2C Scanner untuk cek apakah alamat `0x3C` atau `0x3D`.

---

## 48.5 Program 2: HTTP POST — Telemetri Sensor ke Cloud

ESP32 membaca DHT11 dan mengirim data ke `httpbin.org/post` (echo server gratis) setiap 30 detik — pola *IoT Telemetry* yang digunakan di seluruh industri.

```cpp
/*
 * BAB 48 - Program 2: HTTP POST — Sensor Telemetry
 *
 * Fitur:
 *   ▶ Baca DHT11 (suhu + kelembaban) setiap 30 detik
 *   ▶ Bangun JSON body dengan ArduinoJson (stack-allocated)
 *   ▶ HTTP POST ke httpbin.org/post (echo server gratis)
 *   ▶ Parse response server untuk konfirmasi data diterima
 *   ▶ Error handling lengkap: timeout, DNS fail, HTTP error
 *   ▶ Counter upload untuk tracking jumlah pengiriman
 *   ▶ Heartbeat Serial tiap 60 detik
 *
 * Hardware:
 *   DHT11 → IO27 (hardwired Bluino Kit, pull-up 4K7Ω)
 *
 * Cara Uji:
 *   1. Ganti WIFI_SSID dan WIFI_PASS
 *   2. Upload → Serial Monitor 115200 baud
 *   3. Tiap 30 detik → ESP32 POST data DHT11 ke httpbin.org
 *   4. httpbin.org memantulkan JSON → tampil di Serial Monitor
 *   5. Buka https://httpbin.org/#/HTTP_Methods/post_post di browser
 *      untuk memahami format response
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// ── Konfigurasi ───────────────────────────────────────────────────
const char* WIFI_SSID    = "NamaWiFiKamu";  // ← Ganti!
const char* WIFI_PASS    = "PasswordWiFi";  // ← Ganti!
const char* POST_URL     = "http://httpbin.org/post";
const unsigned long POST_INTERVAL = 30000UL;  // 30 detik
const int    HTTP_TIMEOUT = 8000;

// ── Pin ──────────────────────────────────────────────────────────
#define DHT_PIN  27
#define DHT_TYPE DHT11

// ── Objek ────────────────────────────────────────────────────────
DHT dht(DHT_PIN, DHT_TYPE);

// ── State ────────────────────────────────────────────────────────
float    lastTemp   = NAN;
float    lastHumid  = NAN;
bool     sensorOk   = false;
uint32_t uploadCount = 0;

// ── Baca DHT11 ────────────────────────────────────────────────────
void readSensor() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  bool  ok = (!isnan(t) && !isnan(h));

  if (ok && !sensorOk) Serial.println("[DHT] ✅ Sensor pulih!");
  if (!ok && sensorOk) Serial.println("[DHT] ⚠️ Gagal baca! Cek wiring IO27.");

  if (ok) { lastTemp = t; lastHumid = h; }
  sensorOk = ok;

  if (sensorOk) {
    Serial.printf("[DHT] %.1f°C | %.0f%%\n", lastTemp, lastHumid);
  }
}

// ── HTTP POST ke endpoint ─────────────────────────────────────────
void postTelemetry() {
  if (!sensorOk) {
    Serial.println("[TLM] ⚠️ Skip POST — sensor error.");
    return;
  }

  // Bangun JSON body di stack (zero heap fragmentation)
  StaticJsonDocument<256> doc;
  doc["device"]   = "bluino";
  doc["temp"]     = serialized(String(lastTemp, 1));  // presisi 1 desimal
  doc["humidity"] = (int)lastHumid;
  doc["sensorOk"] = sensorOk;
  doc["uptime"]   = millis() / 1000UL;
  doc["freeHeap"] = ESP.getFreeHeap();
  doc["upload"]   = uploadCount + 1;

  char body[256];
  size_t bodyLen = serializeJson(doc, body, sizeof(body));
  Serial.printf("[HTTP] POST → %s\n", POST_URL);
  Serial.printf("[HTTP] Body: %s\n", body);

  HTTPClient http;
  http.begin(POST_URL);
  http.setTimeout(HTTP_TIMEOUT);
  http.addHeader("Content-Type", "application/json");
  http.addHeader("User-Agent", "BluinoIoTKit/1.0");

  int code = http.POST((uint8_t*)body, bodyLen);

  if (code < 0) {
    Serial.printf("[HTTP] ❌ Error: %s\n", http.errorToString(code).c_str());
    http.end();
    return;
  }

  Serial.printf("[HTTP] %d %s\n", code, (code == 200) ? "✅ OK" : "❌ Error");

  if (code == HTTP_CODE_OK) {
    String resp = http.getString();

    // Parse response httpbin.org — field "json" berisi apa yang kita kirim
    StaticJsonDocument<512> respDoc;
    DeserializationError err = deserializeJson(respDoc, resp);
    if (!err && respDoc.containsKey("json")) {
      char confirmed[256];
      serializeJson(respDoc["json"], confirmed, sizeof(confirmed));
      Serial.printf("[HTTP] ✅ Server menerima payload:\n  %s\n", confirmed);
    }
    uploadCount++;
    Serial.printf("[HTTP] Upload #%u berhasil!\n", uploadCount);
  }

  http.end();
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  delay(1000);

  Serial.println("\n=== BAB 48 Program 2: HTTP POST — Sensor Telemetry ===");
  Serial.printf("[INFO] Endpoint : %s\n", POST_URL);
  Serial.printf("[INFO] Interval : %lu detik\n", POST_INTERVAL / 1000UL);

  Serial.printf("[WiFi] Menghubungkan ke '%s'...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t tStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - tStart >= 15000UL) { delay(2000); ESP.restart(); }
    Serial.print("."); delay(500);
  }
  Serial.printf("\n[WiFi] ✅ Terhubung! IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("[INFO] Telemetri pertama dalam %lu detik...\n", POST_INTERVAL / 1000UL);
}

void loop() {
  // Cek WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] ⚠️ Terputus! Reconnect...");
    WiFi.reconnect(); delay(5000); return;
  }

  // Baca sensor + POST tiap POST_INTERVAL
  static unsigned long tPost = 0;
  if (millis() - tPost >= POST_INTERVAL) {
    tPost = millis();
    readSensor();
    postTelemetry();
  }

  // Heartbeat tiap 60 detik
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] %.1fC %.0f%% | Heap: %u B | Upload: #%u\n",
      sensorOk ? lastTemp : 0.0f, sensorOk ? lastHumid : 0.0f,
      ESP.getFreeHeap(), uploadCount);
  }
}
```

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 48 Program 2: HTTP POST — Sensor Telemetry ===
[INFO] Endpoint : http://httpbin.org/post
[INFO] Interval : 30 detik
[WiFi] ✅ Terhubung! IP: 192.168.1.105
[INFO] Telemetri pertama dalam 30 detik...
[DHT] 28.5°C | 65%
[HTTP] POST → http://httpbin.org/post
[HTTP] Body: {"device":"bluino","temp":28.5,"humidity":65,"sensorOk":true,"uptime":31,"freeHeap":198440,"upload":1}
[HTTP] 200 ✅ OK
[HTTP] ✅ Server menerima payload:
  {"device":"bluino","freeHeap":198440,"humidity":65,"sensorOk":true,"temp":28.5,"upload":1,"uptime":31}
[HTTP] Upload #1 berhasil!
[HB] 28.5C 65% | Heap: 197800 B | Upload: #1
```

> 💡 **Kenapa `httpbin.org`?** Ini adalah echo server gratis yang memantulkan kembali apa pun yang kita kirim dalam format JSON. Sempurna untuk belajar tanpa harus setup server sendiri. Saat sudah siap integrasi ke cloud sungguhan (ThingSpeak, Firebase, Supabase), cukup ganti URL dan sesuaikan format JSON.

---

## 48.6 Program 3: Siklus IoT Cloud Penuh — Pull Config + Push Telemetry

Program puncak BAB 48: **State Machine** yang secara bergantian mengambil konfigurasi dari server (GET) lalu mengirim data sensor (POST) — arsitektur **"Pull Config, Push Telemetry"** standar industri IoT.

```cpp
/*
 * BAB 48 - Program 3: Siklus IoT Cloud Penuh
 *
 * Arsitektur: Pull Config → Push Telemetry → IDLE → Ulangi
 *
 * State Machine:
 *   IDLE          → Menunggu interval terpenuhi
 *   FETCH_CONFIG  → GET config dari server, terapkan ke relay
 *   POST_TELEMETRY→ POST data sensor + status aktuator
 *   (kembali ke IDLE)
 *
 * Fitur:
 *   ▶ GET konfigurasi dari JSONPlaceholder (simulasi config server)
 *   ▶ Terapkan config ke Relay IO4 berdasarkan response
 *   ▶ POST telemetri DHT11 + status relay + uptime ke httpbin.org
 *   ▶ State Machine non-blocking — loop() tidak pernah berhenti
 *   ▶ Serial CLI: status | relay on/off | restart | help
 *   ▶ Heartbeat tiap 30 detik
 *
 * Hardware:
 *   DHT11 → IO27  (hardwired Bluino Kit)
 *   Relay → IO4   (kabel jumper dari header pin)
 *
 * Endpoint:
 *   Config    : http://jsonplaceholder.typicode.com/todos/1
 *   Telemetry : http://httpbin.org/post
 *
 * Cara Uji:
 *   1. Sambungkan Relay ke IO4 dengan kabel jumper
 *   2. Ganti WIFI_SSID dan WIFI_PASS
 *   3. Upload → Serial Monitor 115200 baud
 *   4. Amati state machine: FETCH_CONFIG → POST_TELEMETRY → IDLE
 *   5. Ketik "status" di Serial Monitor → lihat state lengkap
 *   6. Ketik "relay on" → relay aktif, amati log di Serial
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

// ── Konfigurasi ───────────────────────────────────────────────────
const char* WIFI_SSID    = "NamaWiFiKamu";  // ← Ganti!
const char* WIFI_PASS    = "PasswordWiFi";  // ← Ganti!
const char* CONFIG_URL   = "http://jsonplaceholder.typicode.com/todos/1";
const char* TELEMETRY_URL= "http://httpbin.org/post";
const int    HTTP_TIMEOUT = 8000;

// ── Pin ──────────────────────────────────────────────────────────
#define DHT_PIN   27
#define RELAY_PIN 4

// ── State Machine ────────────────────────────────────────────────
enum AppState { IDLE, FETCH_CONFIG, POST_TELEMETRY };
AppState appState   = IDLE;
unsigned long tCycle = 0;

// ── Objek ────────────────────────────────────────────────────────
DHT dht(DHT_PIN, DHT11);

// ── Data ─────────────────────────────────────────────────────────
float    lastTemp    = NAN;
float    lastHumid   = NAN;
bool     sensorOk    = false;
bool     relayState  = false;
uint32_t cycleCount  = 0;
uint32_t tlmCount    = 0;

// ── Interval dari config server (bisa diubah server) ─────────────
unsigned long configInterval = 30000UL;

// ── Serial CLI ────────────────────────────────────────────────────
static char cliBuf[80];
static uint8_t cliLen = 0;

// ── Baca sensor ──────────────────────────────────────────────────
void readSensor() {
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  bool  ok = (!isnan(t) && !isnan(h));
  if (ok && !sensorOk) Serial.println("[DHT] ✅ Sensor pulih!");
  if (!ok && sensorOk) Serial.println("[DHT] ⚠️ Gagal baca! Cek IO27.");
  if (ok) { lastTemp = t; lastHumid = h; }
  sensorOk = ok;
}

// ── STATE: FETCH_CONFIG ──────────────────────────────────────────
// Simulasi: ambil config dari JSONPlaceholder
// Response: {"userId":1,"id":1,"title":"delectus aut autem","completed":false}
// Di proyek nyata: server mengirim {"relayOn":true,"interval":60}
void stateFetchConfig() {
  Serial.println("[CFG] GET config dari server...");

  HTTPClient http;
  http.begin(CONFIG_URL);
  http.setTimeout(HTTP_TIMEOUT);
  int code = http.GET();

  if (code != HTTP_CODE_OK) {
    Serial.printf("[CFG] ❌ HTTP %d — skip config\n", code);
    http.end();
    appState = POST_TELEMETRY;
    return;
  }

  String resp = http.getString();
  http.end();

  // Parse config
  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, resp);
  if (err) {
    Serial.printf("[CFG] ❌ JSON parse gagal: %s\n", err.c_str());
    appState = POST_TELEMETRY;
    return;
  }

  // JSONPlaceholder mengembalikan field "completed" (boolean)
  // Kita gunakan sebagai simulasi "relayOn"
  bool cfgRelay    = doc["completed"] | false;
  int  cfgInterval = doc["id"].as<int>() * 1000; // simulasi: id*1000 ms
  if (cfgInterval < 10000) cfgInterval = 30000;  // minimal 10 detik
  if (cfgInterval > 300000) cfgInterval = 300000; // maksimal 5 menit

  // Terapkan config
  bool relayChanged = (cfgRelay != relayState);
  relayState = cfgRelay;
  digitalWrite(RELAY_PIN, relayState ? HIGH : LOW);
  configInterval = (unsigned long)cfgInterval;

  Serial.printf("[CFG] ✅ interval=%lus | relayOn=%s%s\n",
    configInterval / 1000UL,
    relayState ? "true" : "false",
    relayChanged ? " (CHANGED!)" : "");

  appState = POST_TELEMETRY;
}

// ── STATE: POST_TELEMETRY ────────────────────────────────────────
void statePostTelemetry() {
  readSensor();

  // Bangun JSON body
  StaticJsonDocument<256> doc;
  doc["device"]     = "bluino";
  if (sensorOk) {
    doc["temp"]     = serialized(String(lastTemp, 1));
    doc["humidity"] = (int)lastHumid;
  } else {
    doc["temp"]     = nullptr;
    doc["humidity"] = nullptr;
  }
  doc["sensorOk"]   = sensorOk;
  doc["relayState"] = relayState;
  doc["uptime"]     = millis() / 1000UL;
  doc["freeHeap"]   = ESP.getFreeHeap();
  doc["cycle"]      = cycleCount;
  doc["tlm"]        = tlmCount + 1;

  char body[256];
  serializeJson(doc, body, sizeof(body));
  Serial.printf("[TLM] POST → %s\n", TELEMETRY_URL);
  Serial.printf("[TLM] Body: %s\n", body);

  HTTPClient http;
  http.begin(TELEMETRY_URL);
  http.setTimeout(HTTP_TIMEOUT);
  http.addHeader("Content-Type", "application/json");
  int code = http.POST((uint8_t*)body, strlen(body));

  if (code < 0) {
    Serial.printf("[TLM] ❌ Error: %s\n", http.errorToString(code).c_str());
  } else if (code == HTTP_CODE_OK) {
    tlmCount++;
    Serial.printf("[TLM] ✅ %d OK — Telemetri terkirim #%u\n", code, tlmCount);
  } else {
    Serial.printf("[TLM] ❌ HTTP %d\n", code);
  }

  http.end();
  appState = IDLE;
  Serial.printf("[STATE] IDLE — siklus berikutnya dalam %lu detik...\n",
    configInterval / 1000UL);
}

// ── Serial CLI Handler ────────────────────────────────────────────
void handleCLI(const char* cmd) {
  if (strcmp(cmd, "help") == 0) {
    Serial.println("Perintah: status | relay on/off | cycle | restart");
  } else if (strcmp(cmd, "status") == 0) {
    Serial.printf("  State : %s\n", appState==IDLE?"IDLE":appState==FETCH_CONFIG?"FETCH_CONFIG":"POST_TELEMETRY");
    Serial.printf("  Suhu  : %.1f°C | Kelembaban: %.0f%%\n", lastTemp, lastHumid);
    Serial.printf("  Relay : %s | Siklus: %u | TLM: %u\n", relayState?"ON":"OFF", cycleCount, tlmCount);
    Serial.printf("  Heap  : %u B | Uptime: %lu s\n", ESP.getFreeHeap(), millis()/1000UL);
  } else if (strcmp(cmd, "relay on") == 0) {
    relayState = true; digitalWrite(RELAY_PIN, HIGH);
    Serial.println("[CLI] Relay ON");
  } else if (strcmp(cmd, "relay off") == 0) {
    relayState = false; digitalWrite(RELAY_PIN, LOW);
    Serial.println("[CLI] Relay OFF");
  } else if (strcmp(cmd, "cycle") == 0) {
    Serial.println("[CLI] Memulai siklus manual...");
    appState = FETCH_CONFIG; tCycle = 0;
  } else if (strcmp(cmd, "restart") == 0) {
    ESP.restart();
  } else {
    Serial.printf("[CLI] Tidak dikenal: '%s' — ketik 'help'\n", cmd);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  dht.begin();
  delay(1000);

  Serial.println("\n=== BAB 48 Program 3: IoT Cloud Cyclic ===");
  Serial.println("[INFO] Ketik 'help' di Serial Monitor.");
  Serial.printf("[INFO] Config URL   : %s\n", CONFIG_URL);
  Serial.printf("[INFO] Telemetry URL: %s\n", TELEMETRY_URL);
  Serial.printf("[INFO] Relay PIN    : IO%d\n", RELAY_PIN);

  Serial.printf("[WiFi] Menghubungkan ke '%s'...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t tStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - tStart >= 15000UL) { delay(2000); ESP.restart(); }
    Serial.print("."); delay(500);
  }
  Serial.printf("\n[WiFi] ✅ Terhubung! IP: %s\n", WiFi.localIP().toString().c_str());

  // Mulai siklus pertama segera — langsung set FETCH_CONFIG
  // cycleCount di-increment di sini agar siklus pertama terhitung
  cycleCount = 1;
  appState   = FETCH_CONFIG;
  Serial.println("[STATE] === Siklus #1 === Mulai → FETCH_CONFIG");
}

void loop() {
  // ── State Machine ────────────────────────────────────────────────
  switch (appState) {
    case IDLE:
      if (millis() - tCycle >= configInterval) {
        tCycle = millis();
        cycleCount++;
        appState = FETCH_CONFIG;
        Serial.printf("\n[STATE] === Siklus #%u === → FETCH_CONFIG\n", cycleCount);
      }
      break;

    case FETCH_CONFIG:
      if (WiFi.status() == WL_CONNECTED) {
        stateFetchConfig();
        // appState sudah diset ke POST_TELEMETRY di dalam stateFetchConfig()
      } else {
        Serial.println("[STATE] ⚠️ WiFi tidak tersedia, skip FETCH_CONFIG");
        appState = POST_TELEMETRY;
      }
      break;

    case POST_TELEMETRY:
      if (WiFi.status() == WL_CONNECTED) {
        statePostTelemetry();
        // appState sudah diset ke IDLE di dalam statePostTelemetry()
      } else {
        Serial.println("[STATE] ⚠️ WiFi tidak tersedia, skip POST_TELEMETRY");
        appState = IDLE;
      }
      break;
  }

  // ── Reconnect WiFi ───────────────────────────────────────────────
  if (WiFi.status() != WL_CONNECTED && appState == IDLE) {
    Serial.println("[WiFi] ⚠️ Terputus! Reconnect...");
    WiFi.reconnect();
    delay(3000);
  }

  // ── Serial CLI ───────────────────────────────────────────────────
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (cliLen > 0) {
        cliBuf[cliLen] = '\0';
        Serial.printf("> %s\n", cliBuf);
        handleCLI(cliBuf);
        cliLen = 0;
      }
    } else if (cliLen < sizeof(cliBuf) - 1) {
      cliBuf[cliLen++] = c;
    }
  }

  // ── Heartbeat ────────────────────────────────────────────────────
  static unsigned long tHb = 0;
  if (millis() - tHb >= 30000UL) {
    tHb = millis();
    Serial.printf("[HB] %.1fC %.0f%% Relay:%s State:%s Siklus:%u TLM:%u Heap:%u\n",
      sensorOk ? lastTemp : 0.0f, sensorOk ? lastHumid : 0.0f,
      relayState ? "ON" : "OFF",
      appState == IDLE ? "IDLE" : appState == FETCH_CONFIG ? "CFG" : "TLM",
      cycleCount, tlmCount, ESP.getFreeHeap());
  }
}
```

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 48 Program 3: IoT Cloud Cyclic ===
[INFO] Config URL   : http://jsonplaceholder.typicode.com/todos/1
[INFO] Telemetry URL: http://httpbin.org/post
[WiFi] ✅ Terhubung! IP: 192.168.1.105
[STATE] Mulai siklus pertama → FETCH_CONFIG

[STATE] === Siklus #1 === → FETCH_CONFIG
[CFG] GET config dari server...
[CFG] ✅ interval=30s | relayOn=false
[DHT] 28.5°C | 65%
[TLM] POST → http://httpbin.org/post
[TLM] ✅ 200 OK — Telemetri terkirim #1
[STATE] IDLE — siklus berikutnya dalam 30 detik...

[HB] 28.5C 65% Relay:OFF State:IDLE Siklus:1 TLM:1 Heap:182440
```

> 💡 **Pola Industri:** Arsitektur *"Pull Config, Push Telemetry"* digunakan di AWS IoT Core, Azure IoT Hub, dan Google Cloud IoT. ESP32 *pull* konfigurasi dari server (misal: interval, threshold, relay state) lalu *push* telemetri data ke platform cloud secara periodik.

---

## 48.7 Troubleshooting HTTP Client

| Gejala | Penyebab | Solusi |
|--------|----------|--------|
| `Error: -1` (connection refused) | DNS gagal atau server down | Cek internet ESP32, coba ping dari PC |
| `Error: -11` (read timeout) | Server lambat | Naikkan `HTTP_TIMEOUT` ke 10000+ |
| `HTTP Code: 301/302` (redirect) | Server redirect ke HTTPS | Pakai URL `https://` → lihat BAB 49 |
| `HTTP Code: 429` (too many req) | Rate limit API terlampaui | Besarkan `FETCH_INTERVAL` |
| `deserializeJson` error | Buffer `StaticJsonDocument<N>` terlalu kecil | Naikkan nilai `N` — gunakan ArduinoJson Assistant |
| OLED blank (Program 1) | Alamat I2C salah | Jalankan I2C Scanner sketch |
| Heap turun terus | `http.end()` tidak dipanggil | Pastikan `http.end()` dipanggil di setiap jalur kode |

> ⚠️ **WAJIB: Selalu panggil `http.end()`** — bahkan saat error! Tanpanya, koneksi TCP tidak ditutup dan heap akan bocor. Gunakan pola:
> ```cpp
> http.begin(url);
> int code = http.GET();
> String body = (code == 200) ? http.getString() : "";
> http.end();  // ← WAJIB di sini, BUKAN di dalam if
> ```

---

## 48.8 Perbandingan: BAB 47 vs BAB 48

| Aspek | BAB 47 (WebSocket) | BAB 48 (HTTP Client) |
|-------|-------------------|----------------------|
| **Peran ESP32** | Server — menunggu browser | Client — aktif ke internet |
| **Jangkauan** | Jaringan lokal (LAN) | Internet global |
| **Koneksi** | Persistent (TCP stream) | Per-request (buka-tutup) |
| **Library** | WebSocketsServer | HTTPClient (bawaan) |
| **Push data** | Instan via broadcastTXT() | Interval via POST |
| **Ambil data** | Tidak bisa ambil dari server | Ya — via GET |
| **Cocok untuk** | Dashboard real-time lokal | Telemetri cloud, cuaca, API |

---

## 48.9 Ringkasan

```
RINGKASAN BAB 48 — HTTP Client ESP32

SIKLUS GET:
  http.begin(url)
  http.setTimeout(ms)         ← WAJIB!
  int code = http.GET()
  if (code == 200) body = http.getString()
  http.end()                  ← WAJIB!

SIKLUS POST:
  http.begin(url)
  http.setTimeout(ms)         ← WAJIB!
  http.addHeader("Content-Type", "application/json")
  int code = http.POST(body)
  http.end()                  ← WAJIB!

PARSE JSON (ArduinoJson):
  StaticJsonDocument<N> doc   ← Alokasi di stack, bukan heap
  deserializeJson(doc, string)
  float v = doc["key"]

BUILD JSON (ArduinoJson):
  StaticJsonDocument<N> doc
  doc["key"] = value
  serializeJson(doc, buf, sizeof(buf))

PATTERN TERBUKTI:
  ✅ Selalu set setTimeout() sebelum GET/POST
  ✅ Selalu panggil http.end() di setiap jalur kode
  ✅ Gunakan StaticJsonDocument (stack) bukan DynamicJsonDocument (heap)
  ✅ Non-blocking: gunakan millis() timer, bukan delay()
  ✅ State Machine untuk siklus multi-langkah

ANTIPATTERN:
  ❌ Lupa http.end() → TCP connection leak → heap bocor
  ❌ Tidak set setTimeout() → loop() freeze jika server down
  ❌ DynamicJsonDocument tanpa batas → heap fragmentation
  ❌ Polling terlalu cepat → rate limit API → HTTP 429
```

---

## 48.10 Latihan

### Latihan Dasar
1. Upload **Program 1**, ganti `CITY` ke `"Bali"`. Verifikasi data cuaca berbeda muncul di Serial.
2. Upload **Program 2**, amati body yang dipantulkan `httpbin.org`. Cocokkan dengan yang dikirim ESP32.
3. Ubah `POST_INTERVAL` di Program 2 menjadi 10 detik. Apa yang terjadi pada heap setiap 60 detik?

### Latihan Menengah
4. Modifikasi **Program 1**: tambahkan kecepatan angin (`windspeedKmph`) ke tampilan OLED baris ke-4.
5. Modifikasi **Program 2**: tambahkan field `"rssi": WiFi.RSSI()` ke JSON body yang dikirim.
6. Jalankan **Program 3**, lalu ketik `relay on` di Serial Monitor. Apakah state machine tetap berjalan normal?

### Tantangan Lanjutan
7. **ThingSpeak Integration**: Daftar akun ThingSpeak gratis → ganti URL di Program 2 ke endpoint ThingSpeak → kirim data suhu ke channel publik.
8. **Retry Logic**: Modifikasi `postTelemetry()` agar mencoba ulang maksimal 3 kali jika request gagal, dengan jeda 2 detik antar percobaan.
9. **Config Server Nyata**: Buat file `config.json` di Google Drive (publish as web) → ganti `CONFIG_URL` di Program 3 → ESP32 baca config dari internet sungguhan.

---

## 48.11 Preview: Mengapa HTTP Tidak Aman? (→ BAB 49)

```
HTTP PLAIN TEXT (BAB 48) — TIDAK AMAN:

  ESP32 ──[plain text]──► Router ──► Internet ──► Server
                  ↑
          🕵️ Siapa pun di WiFi yang sama BISA membaca:
          {"device":"bluino","temp":28.5,"apiKey":"secret123"}

HTTPS TLS (BAB 49) — AMAN:

  ESP32 ──[TLS encrypted]──► Router ──► Internet ──► Server
                  ↑
          🔒 Data terenkripsi — tidak bisa dibaca tanpa kunci
          ÿ∂Ω∑†¬µ≈√∫˜≤≥÷  ← Yang terlihat di jaringan
```

Di BAB 49, kita akan menggunakan `WiFiClientSecure` dan sertifikat CA untuk berkomunikasi dengan server HTTPS secara aman.

---

## 48.12 Referensi

| Sumber | Link |
|--------|------|
| Arduino HTTPClient docs | https://github.com/espressif/arduino-esp32/tree/master/libraries/HTTPClient |
| ArduinoJson dokumentasi | https://arduinojson.org/v6/doc/ |
| ArduinoJson Assistant (ukuran buffer) | https://arduinojson.org/v6/assistant/ |
| wttr.in API | https://github.com/chubin/wttr.in |
| httpbin.org (echo server) | https://httpbin.org |

---

## ☕ Dukung Proyek Ini

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

---

*Sebelumnya → [BAB 47: WebSocket](../BAB-47/README.md)*

*Selanjutnya → [BAB 49: HTTPS & Keamanan](../BAB-49/README.md)*
