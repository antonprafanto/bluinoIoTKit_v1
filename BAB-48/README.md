# BAB 48 — HTTP Client & REST API

> **Posisi dalam Kurikulum:**
> BAB 46 (Web Server) → BAB 47 (WebSocket) → **BAB 48 (HTTP Client)** → BAB 49 (HTTPS)

Pada BAB 42–47, ESP32 berperan sebagai **Server** — ia menunggu permintaan dari browser.  
Kini ESP32 bertransisi peran menjadi **Client** — ia yang aktif *pergi* ke internet untuk mengambil atau mengirim data.

Ini adalah fondasi dari hampir semua perangkat IoT dunia nyata: sensor membaca data, lalu *mengirimnya* ke cloud, platform monitoring, atau API backend.

---

## 48.1 Apa itu HTTP Client?

Ketika Anda membuka `http://api.weather.com/data` di browser, browser Anda berperan sebagai **HTTP Client**. Ia mengirim *request*, server membalas dengan *response*.

ESP32 bisa melakukan hal yang sama menggunakan library `HTTPClient.h` yang sudah terbundel dalam ESP32 Arduino Core.

```
ESP32 → (HTTP GET)  → Server Eksternal → (JSON Response) → ESP32
ESP32 → (HTTP POST) → Server Eksternal → (HTTP 200 OK)   → ESP32
```

---

## 48.2 Mindset Shift: Server vs Client

| Aspek | BAB 46–47 (Server) | BAB 48 (Client) |
|---|---|---|
| Inisiator | Browser/Device lain | ESP32 sendiri |
| Peran ESP32 | Menunggu (`handleClient`) | Aktif meminta (`http.GET()`) |
| Arah data | Browser → ESP32 | ESP32 → Internet |
| Port | Buka port 80/81 | Tidak perlu buka port |
| Cocok untuk | Dashboard lokal | Telemetri cloud, ambil data online |

---

## 48.3 HTTPClient Library — Panduan API

```cpp
#include <HTTPClient.h>

HTTPClient http;

// ── GET ────────────────────────────────────────────────────────────
http.begin("http://api.example.com/data"); // URL tujuan
http.setTimeout(5000);                     // Batas waktu 5 detik (WAJIB!)
int code = http.GET();                     // Kirim request
if (code == HTTP_CODE_OK) {
  String body = http.getString();          // Ambil isi response
}
http.end();                                // Tutup koneksi

// ── POST ──────────────────────────────────────────────────────────
http.begin("http://api.example.com/data");
http.setTimeout(5000);
http.addHeader("Content-Type", "application/json");
int code = http.POST("{\"temp\":28.5}");   // Kirim JSON body
http.end();
```

> ⚠️ **`http.setTimeout(5000)` WAJIB dipanggil!**  
> Tanpa timeout, jika server tidak merespons, `http.GET()` akan **memblokir `loop()` selamanya** hingga ESP32 freeze.

---

## 48.4 ArduinoJson — Parser JSON Profesional

**Library:** `ArduinoJson` by bblanchon (install via Library Manager)

```cpp
#include <ArduinoJson.h>

// Stack-allocated — Zero heap fragmentation
StaticJsonDocument<1024> doc;

// Parse string JSON
DeserializationError err = deserializeJson(doc, jsonString);
if (!err) {
  float temp = doc["temperature"];
  const char* city = doc["city"];
}
```

> 💡 **`StaticJsonDocument<N>`** mengalokasikan memori di **stack** (bukan heap), sehingga tidak menyebabkan fragmentasi memori pada mikrokontroler.
> Pilih ukuran `N` yang cukup besar untuk menampung seluruh JSON response Anda.

---

## 48.5 Program 1: HTTP GET — Cuaca Real-Time di OLED

📂 **Kode:** [`p1_get_json/p1_get_json.ino`](p1_get_json/p1_get_json.ino)

**Apa yang dipelajari:**
- `http.begin(url)` → `http.GET()` → `http.getString()` — siklus lengkap GET request
- Parsing JSON bersarang (`doc["current_condition"][0]["temp_C"]`) dengan ArduinoJson
- Menampilkan data dari internet langsung ke OLED I2C 128×64
- Non-blocking request menggunakan `millis()` timer (fetch tiap 10 menit)
- Handling error: DNS failure, timeout, HTTP response code bukan 200

**Library yang dibutuhkan:**
- `HTTPClient.h`, `WiFi.h` — bawaan ESP32 Core
- `ArduinoJson` by bblanchon ≥ 6.x — install via Library Manager
- `Adafruit_SSD1306` + `Adafruit_GFX` — install via Library Manager (sudah terinstall jika selesai BAB 35)

**Cara Uji:**
1. Install library yang dibutuhkan di Library Manager
2. Ganti `WIFI_SSID`, `WIFI_PASS`, dan `CITY` (default: `Jakarta`)
3. Upload → buka Serial Monitor (115200 baud)
4. Tunggu 3–5 detik → data cuaca Jakarta muncul di OLED dan Serial Monitor
5. Coba ubah `CITY` ke `Bandung`, `Surabaya`, atau nama kota lain!
6. Cabut WiFi → amati pesan error di Serial Monitor

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 48 Program 1: HTTP GET — Cuaca Real-Time ===
[OLED] ✅ Display terdeteksi di 0x3C
[WiFi] Menghubungkan ke 'WiFiRumah'...
[WiFi] ✅ Terhubung! IP: 192.168.1.105 | SSID: WiFiRumah
[mDNS] ✅ http://bluino.local
[HTTP] GET http://wttr.in/Jakarta?format=j1
[HTTP] ✅ 200 OK | Suhu: 29°C | Kondisi: Partly cloudy | Kelembaban: 74%
[HB] Cuaca: 29°C Partly cloudy | Heap: 231456 B | Uptime: 60 s
```

---

## 48.6 Program 2: HTTP POST — Kirim Data Sensor ke Server

📂 **Kode:** [`p2_post_sensor/p2_post_sensor.ino`](p2_post_sensor/p2_post_sensor.ino)

**Apa yang dipelajari:**
- Membangun JSON body string dengan `ArduinoJson`
- `http.addHeader("Content-Type", "application/json")` — HTTP header wajib untuk POST JSON
- `http.POST(body)` — mengirim data sensor ke server
- Membaca dan parsing response body dari server
- Pola **Telemetri IoT**: baca sensor → format JSON → kirim ke cloud (tiap N detik)

**Library yang dibutuhkan:**
- Sama dengan Program 1 (tanpa `Adafruit_SSD1306`)
- `DHT sensor library` by Adafruit + `Adafruit Unified Sensor`

**Cara Uji:**
1. Upload → buka Serial Monitor
2. Tiap 30 detik → ESP32 mengirim data DHT11 ke `httpbin.org/post`
3. Server `httpbin.org` memantulkan kembali JSON yang dikirim → bisa dibaca di Serial
4. Buka `http://httpbin.org/post` di browser untuk memahami response format

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 48 Program 2: HTTP POST — Sensor Telemetry ===
[WiFi] Menghubungkan ke 'WiFiRumah'...
[WiFi] ✅ Terhubung! IP: 192.168.1.105 | SSID: WiFiRumah
[mDNS] ✅ http://bluino.local
[INFO] Telemetri akan dikirim tiap 30 detik
[INFO] Endpoint: http://httpbin.org/post
[DHT] ✅ Baca OK: 28.5°C | 65%
[DHT] 28.5°C | 65%
[HTTP] POST → http://httpbin.org/post
[HTTP] Body: {"device":"bluino","temp":28.5,"humidity":65,"sensorOk":true,"uptime":5,"freeHeap":198200,"upload":1}
[HTTP] ✅ 200 OK — Server menerima payload:
  {"device":"bluino","freeHeap":198200,"humidity":65,"sensorOk":true,"temp":28.5,"upload":1,"uptime":5}
[HB] 28.5C 65% | Heap: 198200 B | Upload #1
```

---

## 48.7 Program 3: Siklus IoT Cloud Penuh (GET Config + POST Telemetry)

📂 **Kode:** [`p3_iot_cloud/p3_iot_cloud.ino`](p3_iot_cloud/p3_iot_cloud.ino)

**Apa yang dipelajari:**
- **State Machine** untuk manajemen request bertahap:
  `IDLE → FETCH_CONFIG → POST_TELEMETRY → IDLE`
- `GET` konfigurasi dari server → terapkan ke perangkat (interval, relay state)
- `POST` telemetri → kirim data sensor + status aktuator secara periodik
- Menerapkan config yang diterima untuk mengontrol Relay (IO4)
- Pola arsitektur **"Pull Config, Push Telemetry"** — standar industri IoT

**Cara Uji:**
1. Sambungkan Relay ke IO4 (kabel jumper)
2. Upload → amati state machine berpindah di Serial Monitor
3. Tiap siklus: ESP32 fetch config → kontrol relay → kirim telemetri
4. Observasi: jika konfigurasi `"relayOn": true` → relay berbunyi klik!

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 48 Program 3: IoT Cloud Cyclic ===
[INFO] Relay PIN: IO4 — sambungkan kabel jumper!
[INFO] Config  URL: http://jsonplaceholder.typicode.com/todos/1
[INFO] Telemetry URL: http://httpbin.org/post
[WiFi] Menghubungkan ke 'WiFiRumah'...
[WiFi] ✅ Terhubung! IP: 192.168.1.105 | SSID: WiFiRumah
[STATE] Mulai siklus pertama → FETCH_CONFIG
[CFG] GET config dari server...
[CFG] ✅ interval=30s | relayOn=false
[TLM] POST telemetri → server...
[TLM] ✅ 200 OK — Telemetri terkirim #1
[STATE] IDLE — tunggu 30 detik...
```

---

## 48.8 Troubleshooting HTTP Client

| Gejala | Penyebab | Solusi |
|---|---|---|
| `[HTTP] Error: -1` | Koneksi gagal / DNS tidak resolve | Cek SSID, cek koneksi internet ESP32 |
| `[HTTP] Error: -11` | Timeout koneksi | Tambah `http.setTimeout(8000)` |
| `HTTP Code: 301/302` | Redirect — server pindah ke HTTPS | Gunakan URL HTTPS (lihat BAB 49) |
| `HTTP Code: 429` | Rate limit API | Perbesar interval `FETCH_INTERVAL` |
| OLED blank | Alamat I2C salah | Scan I2C: coba `0x3C` atau `0x3D` |
| `deserializeJson` error | Buffer terlalu kecil | Naikkan `StaticJsonDocument<N>` |

---

## 48.9 Preview: Mengapa HTTP Tidak Aman? (→ BAB 49)

Request HTTP yang kita kirim di BAB ini berjalan **tanpa enkripsi**. Artinya, siapa pun yang berada di jaringan WiFi yang sama bisa *menyadap* data yang dikirim:

```
ESP32 ──[HTTP plain text]──► Router ──► Internet
                    ↑
              🕵️ Siapa pun bisa baca ini!
              {"device":"bluino","temp":28.5}
```

Solusinya adalah **HTTPS (HTTP Secure)** yang menggunakan enkripsi **TLS/SSL**. Kita akan mempelajarinya di BAB 49 menggunakan `WiFiClientSecure`.

---

*Bluino IoT Kit 2026 — BAB 48: HTTP Client & REST API*
