# 🌐 BAB 51 — CoAP: Constrained Application Protocol

> ✅ **Prasyarat:** Selesaikan **BAB 50 (MQTT)**. Kamu harus sudah memahami konsep Publisher-Subscriber dan perbedaan UDP vs TCP.

> 📖 **Glosarium Singkatan BAB ini:**
> | Singkatan | Kepanjangan |
> |-----------|-------------|
> | **CoAP** | *Constrained Application Protocol* — protokol IoT ringan berbasis UDP |
> | **CON** | *Confirmable* — pesan yang memerlukan ACK dari penerima |
> | **NON** | *Non-Confirmable* — pesan tanpa ACK (best-effort, seperti UDP biasa) |
> | **ACK** | *Acknowledgement* — konfirmasi penerimaan pesan CON |
> | **RST** | *Reset* — penolakan pesan yang tidak dapat diproses |
> | **Observe** | Ekstensi CoAP untuk push notification (mirip subscribe MQTT) |
> | **DTLS** | *Datagram TLS* — enkripsi untuk CoAP over UDP (port 5684) |

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami mengapa CoAP lahir dan perbedaan fundamentalnya dengan HTTP & MQTT.
- Menggunakan library `coap-simple` untuk membuat CoAP Client dan CoAP Server di ESP32.
- Mengirim data IoT ke Antares menggunakan metode CoAP POST.
- Membuat ESP32 sebagai **CoAP Server** — perangkat IoT yang melayani request dari client.
- Memahami CoAP Message Types (CON, NON, ACK, RST) dan Response Codes.

---

## 51.1 Mengapa CoAP? — Protokol untuk Perangkat Terbatas

```
Skenario: Sensor baterai di lapangan — baterai hanya 1.5V AA

  HTTP  : TCP handshake (3-way) + TLS + Header 1-2KB per request
          → Baterai habis dalam hitungan hari ❌

  MQTT  : TCP persistent connection + keepalive ping tiap N detik
          → Cocok untuk device selalu ON, boros daya ❌

  CoAP  : UDP — tidak perlu handshake TCP!
          Header hanya 4 BYTE! RESTful seperti HTTP.
          → Baterai bertahan berbulan-bulan ✅
```

| Aspek | HTTP/HTTPS | MQTT | CoAP |
|-------|-----------|------|------|
| **Transport** | TCP | TCP | **UDP** |
| **Min Header** | ~200 byte | 2 byte | **4 byte** |
| **Handshake** | ✅ 3-way TCP | ✅ TCP | ❌ Tidak ada |
| **Model** | Request-Response | Pub-Sub | **Request-Response** |
| **Push (real-time)** | ❌ | ✅ | ✅ (via Observe) |
| **Enkripsi** | TLS (port 443) | TLS (port 8883) | DTLS (port 5684) |
| **Cocok untuk** | Server/Web API | Monitoring IoT | **Sensor baterai, NB-IoT** |

---

## 51.2 Arsitektur CoAP

```
CoAP Message Types:

  ┌─────────────────────────────────────────────────────────────┐
  │  CON (Confirmable) — pesan penting, harus dapat ACK        │
  │                                                             │
  │  Client ─── CON GET /sensor ──────────────► Server         │
  │  Client ◄── ACK 2.05 Content {temp:29.1} ── Server         │
  │                                                             │
  │  NON (Non-Confirmable) — kirim dan lupakan, seperti UDP     │
  │                                                             │
  │  Client ─── NON GET /sensor ──────────────► Server         │
  │  Client ◄── NON 2.05 Content {temp:29.1} ── Server         │
  │             (mungkin tidak sampai — best effort)            │
  └─────────────────────────────────────────────────────────────┘

CoAP Methods (mirip HTTP REST):
  GET    → Baca resource
  POST   → Buat resource baru
  PUT    → Update resource
  DELETE → Hapus resource

CoAP Response Codes (mirip HTTP status):
  2.01 Created       → POST berhasil membuat resource
  2.04 Changed       → PUT berhasil mengubah resource
  2.05 Content       → GET berhasil, payload dalam response
  4.04 Not Found     → Resource tidak ditemukan
  4.05 Not Allowed   → Method tidak diizinkan
  5.00 Internal Server Error → Error di server
```

### CoAP Observe (Ekstensi Push)
```
Tanpa Observe (polling biasa):
  Client ─── GET /sensor ──► Server   (tiap 10 detik)
  Client ◄── 2.05 Content ── Server

Dengan Observe (push otomatis):
  Client ─── GET /sensor [Observe:0] ──► Server  (daftar sekali)
  Client ◄── 2.05 {temp:29.1} ─────────  Server  (langsung)
  Client ◄── 2.05 {temp:30.2} ─────────  Server  (saat berubah)
  Client ◄── 2.05 {temp:31.5} ─────────  Server  (saat berubah)
  (Server push data ke client tanpa diminta!)
```

---

## 51.3 Library & API Reference

```
LIBRARY YANG DIGUNAKAN:

  coap-simple.h  → Install via Library Manager:
                   Cari "coap-simple" → pilih by hirotakaster → Install
  WiFiUdp.h      → Bawaan ESP32 Core (tidak perlu install)
  ArduinoJson.h  → Cari "ArduinoJson" by bblanchon >= 6.x

API coap-simple ESENSIAL:

  SETUP:
    WiFiUDP udp;
    Coap coap(udp);
    coap.response(callbackFn);      → Daftarkan callback CLIENT (sebelum start!)
    coap.server(callbackFn, "path") → Daftarkan endpoint SERVER
    coap.start();                   → Mulai UDP listener (port 5683)
    coap.start(port);               → Mulai di port custom

  CLIENT:
    coap.get(ip, port, "/path")     → Kirim GET, return msgId (0=gagal)
    coap.post(ip, port, "/path", payload)   → Kirim POST
    coap.put(ip, port, "/path", payload)    → Kirim PUT
    coap.del(ip, port, "/path")     → Kirim DELETE

  DI DALAM loop():
    coap.loop()                     → ← WAJIB! Proses semua UDP packets

  CALLBACK CLIENT (menerima response):
    void fn(CoapPacket &packet, IPAddress ip, int port) {
      uint8_t cls = packet.code >> 5;     // Kelas: 2=Success, 4=Client Error, 5=Server Error
      uint8_t dtl = packet.code & 0x1F;  // Detail kode
      // packet.payload  → uint8_t* (TIDAK null-terminated!)
      // packet.payloadlen → panjang payload
      char buf[256];
      memcpy(buf, packet.payload, packet.payloadlen);
      buf[packet.payloadlen] = '\0';  // ← WAJIB tambahkan null terminator!
    }

  CALLBACK SERVER (melayani request masuk):
    void fn(CoapPacket &packet, IPAddress ip, int port) {
      // packet.code == COAP_GET (1), COAP_POST (2), COAP_PUT (3)
      // Kirim response:
      coap.sendResponse(ip, port, packet.messageid,
        payload, payloadLen,
        COAP_RESPONSE_CODE(CONTENT),      // 2.05 OK
        COAP_CONTENT_TYPE(APPLICATION_JSON),
        packet.token, packet.tokenlen);
    }

RESPONSE CODE:
  cls=2: Success  → cls=4: Client Error  → cls=5: Server Error
  2.01 Created    → 4.00 Bad Request     → 5.00 Internal Error
  2.04 Changed    → 4.04 Not Found       → 5.01 Not Implemented
  2.05 Content    → 4.05 Not Allowed
```

---

## 51.4 Konfigurasi Antares CoAP

| Parameter | Nilai |
|-----------|-------|
| Host | `platform.antares.id` |
| Port | `5683` (CoAP standar, tanpa enkripsi) |
| Path | `/~/{ACCESS_KEY}/{APP_NAME}/{DEVICE_NAME}` |
| Method | `POST` |
| Payload | `{"m2m:cin":{"con":"{\"temp\":29.1}"}}` |

> ⚠️ **Catatan Penting:** Antares CoAP menggunakan format path oneM2M yang sama seperti MQTT, namun berbeda dengan HTTP. Pastikan `ACCESS_KEY` tidak mengandung karakter `/` dalam path URL. Gunakan URL-encoding jika perlu.

---

## 51.5 Program 1: CoAP GET — Baca Data dari Server Publik

ESP32 sebagai **CoAP Client** — mengambil data dari server publik `coap.me`. Tidak perlu akun/registrasi! Cocok untuk pertama kali memahami cara kerja CoAP.

### Tujuan Pembelajaran
- Memahami model Request-Response CoAP
- Mempraktikkan `coap.get()` dan response callback asinkron
- Memahami CoAP response codes (2.05 Content = sukses)

### Hardware
- OLED: tampilkan response dan counter request

```cpp
/*
 * BAB 51 - Program 1: CoAP GET — Baca Data dari Server Publik (coap.me)
 *
 * Fitur:
 *   ► ESP32 sebagai CoAP Client (UDP, bukan TCP!)
 *   ► CoAP GET ke coap.me/hello tiap 30 detik
 *   ► Response callback asinkron — UDP stateless
 *   ► Decode CoAP response code (cls.dtl format)
 *   ► OLED tampilkan response & counter
 *   ► Non-blocking reconnect WiFi
 *   ► Heartbeat tiap 60 detik
 *
 * Hardware:
 *   OLED → SDA=IO21, SCL=IO22 (hardwired Bluino Kit)
 *
 * Library:
 *   coap-simple by hirotakaster (Library Manager)
 *
 * Server: coap.me — server publik Eclipse untuk testing CoAP
 *   Endpoint tersedia: /hello, /test, /sink (terima semua POST)
 *   Tidak perlu akun atau API key!
 *
 * Perbedaan dari HTTP GET (BAB 48):
 *   HTTP : TCP connect → request → response → disconnect (sinkron)
 *   CoAP : UDP packet → kirim → callback saat response tiba (asinkron)
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Konfigurasi WiFi ──────────────────────────────────────────────────
const char* WIFI_SSID = "NamaWiFiKamu";  // ← Ganti!
const char* WIFI_PASS = "PasswordWiFi";  // ← Ganti!

// ── CoAP Server (coap.me — public test server) ───────────────────────
const char* COAP_SERVER   = "coap.me";
const int   COAP_PORT     = 5683;
const char* COAP_RESOURCE = "/hello";

const unsigned long GET_INTERVAL = 30000UL;  // 30 detik

// ── Hardware ──────────────────────────────────────────────────────────
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool oledReady = false;

// ── State ─────────────────────────────────────────────────────────────
char     lastResp[128] = "Menunggu...";
uint32_t getCount = 0;
uint32_t okCount  = 0;

// ── Objek CoAP (UDP) ──────────────────────────────────────────────────
WiFiUDP udp;
Coap    coap(udp);

// ── OLED Helper ───────────────────────────────────────────────────────
void updateOLED() {
  if (!oledReady) return;
  oled.clearDisplay(); oled.setTextColor(WHITE); oled.setTextSize(1);
  oled.setCursor(0, 0);  oled.print("CoAP GET Client");
  oled.drawLine(0, 10, 127, 10, WHITE);
  oled.setCursor(0, 14); oled.print("coap.me/hello");
  oled.setCursor(0, 26); oled.print("Req:"); oled.print(getCount);
  oled.print("  OK:"); oled.print(okCount);
  oled.setCursor(0, 38); oled.print("Response:");
  oled.setCursor(0, 50); oled.print(lastResp);
  oled.display();
}

// ── Callback: dipanggil saat response CoAP diterima (ASINKRON!) ───────
void callbackResponse(CoapPacket &packet, IPAddress ip, int port) {
  // WAJIB: payload tidak null-terminated!
  memset(lastResp, 0, sizeof(lastResp));
  size_t copyLen = min((size_t)packet.payloadlen, sizeof(lastResp) - 1);
  if (copyLen > 0) memcpy(lastResp, packet.payload, copyLen);

  // Decode response code: high 3 bits = class, low 5 bits = detail
  uint8_t cls = packet.code >> 5;
  uint8_t dtl = packet.code & 0x1F;

  Serial.printf("[CoAP] ← Response dari %s\n", ip.toString().c_str());
  Serial.printf("[CoAP] Code: %u.%02u | Payload: %s\n", cls, dtl, lastResp);

  if (cls == 2) {
    okCount++;
    Serial.printf("[CoAP] ✅ Sukses! Heap: %u B\n", ESP.getFreeHeap());
  } else {
    Serial.printf("[CoAP] ⚠️ Error response: %u.%02u\n", cls, dtl);
  }
  updateOLED();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  Serial.println("\n=== BAB 51 Program 1: CoAP GET — coap.me ===");
  Serial.printf("[INFO] Target: coap://%s:%d%s\n", COAP_SERVER, COAP_PORT, COAP_RESOURCE);

  // OLED
  if (oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledReady = true;
    Serial.println("[OLED] ✅ Display terdeteksi di 0x3C");
    oled.clearDisplay(); oled.setTextColor(WHITE); oled.setTextSize(1);
    oled.setCursor(0, 0);  oled.print("BAB 51 CoAP GET");
    oled.setCursor(0, 12); oled.print("WiFi..."); oled.display();
  } else {
    Serial.println("[OLED] ⚠️ Display tidak terdeteksi — lanjut tanpa OLED.");
  }

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[WiFi] Menghubungkan ke '%s'...\n", WIFI_SSID);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - t0 >= 15000UL) { delay(2000); ESP.restart(); }
    Serial.print("."); delay(500);
  }
  Serial.printf("\n[WiFi] ✅ Terhubung! IP: %s\n", WiFi.localIP().toString().c_str());

  // CoAP setup — WAJIB: daftarkan callback SEBELUM coap.start()!
  coap.response(callbackResponse);
  coap.start();  // Buka UDP listener di port 5683
  Serial.println("[CoAP] ✅ Client siap (UDP port 5683)");
  Serial.printf("[INFO] GET tiap %lu detik\n", GET_INTERVAL / 1000UL);
}

void loop() {
  // Cek WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] ⚠️ Terputus! Reconnect...");
    WiFi.reconnect(); delay(5000); return;
  }

  // WAJIB: proses semua incoming UDP (response CoAP)
  coap.loop();

  // CoAP GET tiap GET_INTERVAL
  static unsigned long tGet = 0;
  static bool firstRun = true;
  if (firstRun || millis() - tGet >= GET_INTERVAL) {
    firstRun = false;
    tGet = millis();
    getCount++;

    // Resolve hostname ke IP (DNS)
    IPAddress serverIP;
    if (!WiFi.hostByName(COAP_SERVER, serverIP)) {
      Serial.printf("[CoAP] ❌ DNS gagal resolve '%s'\n", COAP_SERVER);
      return;
    }

    Serial.printf("[CoAP] → GET #%u: coap://%s%s\n", getCount, COAP_SERVER, COAP_RESOURCE);
    // coap.get() mengirim paket UDP dan langsung return msgId
    // Response akan datang ASINKRON via callbackResponse()
    uint16_t msgId = coap.get(serverIP, COAP_PORT, COAP_RESOURCE);
    if (msgId > 0) {
      Serial.printf("[CoAP] Terkirim (msgId:%u) — tunggu response...\n", msgId);
    } else {
      Serial.println("[CoAP] ❌ Gagal kirim — cek koneksi WiFi.");
    }
    updateOLED();
  }

  // Heartbeat
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] GET:%u OK:%u | Heap:%u B | Uptime:%lu s\n",
      getCount, okCount, ESP.getFreeHeap(), millis() / 1000UL);
  }
}
```

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 51 Program 1: CoAP GET — coap.me ===
[INFO] Target: coap://coap.me:5683/hello
[OLED] ✅ Display terdeteksi di 0x3C
[WiFi] Menghubungkan ke 'WiFiRumah'...............
[WiFi] ✅ Terhubung! IP: 192.168.1.105
[CoAP] ✅ Client siap (UDP port 5683)
[INFO] GET tiap 30 detik
[CoAP] → GET #1: coap://coap.me/hello
[CoAP] Terkirim (msgId:1) — tunggu response...
[CoAP] ← Response dari 5.196.95.208
[CoAP] Code: 2.05 | Payload: World!
[CoAP] ✅ Sukses! Heap: 212400 B
[HB] GET:2 OK:2 | Heap:212000 B | Uptime:61 s
```

> ⚠️ **Perbedaan Kritis CoAP vs HTTP:**
> - **HTTP:** `client.GET()` **blocking** — program berhenti menunggu response sebelum lanjut.
> - **CoAP:** `coap.get()` **non-blocking** — langsung return, response datang via callback.
> - Akibatnya: kamu **WAJIB** memanggil `coap.loop()` di setiap iterasi loop() agar callback terpanggil.

---

## 51.6 Program 2: CoAP POST — Kirim Data DHT11 ke Antares

ESP32 membaca DHT11 lalu **mengirim data** ke Antares menggunakan CoAP POST. Jauh lebih hemat baterai dibanding HTTP karena tidak ada TCP handshake.

### Tujuan Pembelajaran
- Memahami CoAP POST dan respons `2.01 Created`
- Membangun payload oneM2M dengan `ArduinoJson` (stack-allocated)
- Membandingkan overhead CoAP vs HTTP untuk kasus IoT baterai

### Hardware
- DHT11 (IO27): baca suhu & kelembaban
- OLED: tampilkan suhu, kelembaban, counter POST sukses

```cpp
/*
 * BAB 51 - Program 2: CoAP POST — Kirim Data DHT11 ke Antares.id
 *
 * Fitur:
 *   ► CoAP POST data DHT11 ke Antares IoT Platform tiap 15 detik
 *   ► Payload format oneM2M (m2m:cin) — standar Antares
 *   ► Response callback asinkron (2.01 Created = sukses)
 *   ► ArduinoJson stack-allocated (zero heap fragmentation)
 *   ► Non-blocking reconnect WiFi
 *   ► OLED tampilkan suhu, kelembaban, counter POST
 *
 * Hardware:
 *   DHT11 → IO27 (hardwired Bluino Kit)
 *   OLED  → SDA=IO21, SCL=IO22 (hardwired)
 *
 * Format Antares CoAP (oneM2M):
 *   URL    : coap://platform.antares.id:5683/~/ACCESS_KEY/APP/DEVICE
 *   Method : POST
 *   Payload: {"m2m:cin":{"con":"{\"temp\":29.1,\"humidity\":65}"}}
 *   Sukses : Response code 2.01 Created
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Konfigurasi WiFi ──────────────────────────────────────────────────
const char* WIFI_SSID = "NamaWiFiKamu";  // ← Ganti!
const char* WIFI_PASS = "PasswordWiFi";  // ← Ganti!

// ── Konfigurasi Antares CoAP ──────────────────────────────────────────
const char* ANTARES_HOST = "platform.antares.id";
const int   ANTARES_PORT = 5683;
const char* ACCESS_KEY   = "ACCESSKEY-ANDA-DISINI"; // ← Ganti!
const char* APP_NAME     = "bluino-kit";             // ← Ganti!
const char* DEVICE_NAME  = "sensor-ruangan";         // ← Ganti!

const unsigned long POST_INTERVAL = 15000UL;

// ── Hardware ──────────────────────────────────────────────────────────
#define DHT_PIN 27
DHT dht(DHT_PIN, DHT11);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool oledReady = false;

// ── State ─────────────────────────────────────────────────────────────
float    lastT     = NAN;
float    lastH     = NAN;
uint32_t postCount = 0;
uint32_t okCount   = 0;
char     coapPath[200];

// ── Objek CoAP ────────────────────────────────────────────────────────
WiFiUDP udp;
Coap    coap(udp);

// ── OLED Helper ───────────────────────────────────────────────────────
void updateOLED() {
  if (!oledReady) return;
  oled.clearDisplay(); oled.setTextColor(WHITE);
  oled.setTextSize(1); oled.setCursor(0, 0); oled.print("CoAP POST Antares");
  oled.drawLine(0, 10, 127, 10, WHITE);
  oled.setTextSize(2); oled.setCursor(0, 14);
  if (!isnan(lastT)) { oled.print(lastT, 1); oled.print("C"); }
  else oled.print("---");
  oled.setTextSize(1); oled.setCursor(0, 36);
  if (!isnan(lastH)) { oled.print("Humid: "); oled.print(lastH, 0); oled.print("%"); }
  oled.setCursor(0, 48);
  oled.print("Post:#"); oled.print(postCount);
  oled.print(" OK:#"); oled.print(okCount);
  oled.display();
}

// ── Callback: response dari Antares ───────────────────────────────────
void callbackResponse(CoapPacket &packet, IPAddress ip, int port) {
  uint8_t cls = packet.code >> 5;
  uint8_t dtl = packet.code & 0x1F;

  char respBuf[128] = "";
  size_t copyLen = min((size_t)packet.payloadlen, sizeof(respBuf) - 1);
  if (copyLen > 0) { memcpy(respBuf, packet.payload, copyLen); }

  Serial.printf("[CoAP] ← Response: %u.%02u", cls, dtl);
  if (strlen(respBuf)) Serial.printf(" | %s", respBuf);
  Serial.println();

  if (cls == 2) {
    okCount++;
    Serial.printf("[CoAP] ✅ Antares menerima data! Total OK: %u | Heap: %u B\n",
      okCount, ESP.getFreeHeap());
  } else {
    // 4.01 = Unauthorized (cek ACCESS_KEY)
    // 4.04 = Not Found    (cek APP_NAME / DEVICE_NAME)
    // 5.00 = Server Error (coba lagi nanti)
    Serial.printf("[CoAP] ⚠️ Error %u.%02u — cek konfigurasi Antares\n", cls, dtl);
  }
  updateOLED();
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  Wire.begin(21, 22);

  // Bangun path CoAP Antares
  snprintf(coapPath, sizeof(coapPath), "/~/%s/%s/%s",
    ACCESS_KEY, APP_NAME, DEVICE_NAME);

  Serial.println("\n=== BAB 51 Program 2: CoAP POST — Antares.id ===");
  Serial.printf("[INFO] Host: %s:%d\n", ANTARES_HOST, ANTARES_PORT);
  Serial.printf("[INFO] Path: %s\n", coapPath);

  if (oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledReady = true;
    Serial.println("[OLED] ✅ Display terdeteksi di 0x3C");
  } else {
    Serial.println("[OLED] ⚠️ Display tidak terdeteksi.");
  }
  oled.clearDisplay(); oled.setTextColor(WHITE);
  oled.setCursor(0, 0);  oled.print("BAB 51 CoAP POST");
  oled.setCursor(0, 12); oled.print("WiFi..."); oled.display();

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[WiFi] Menghubungkan ke '%s'...\n", WIFI_SSID);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - t0 >= 15000UL) { delay(2000); ESP.restart(); }
    Serial.print("."); delay(500);
  }
  Serial.printf("\n[WiFi] ✅ Terhubung! IP: %s\n", WiFi.localIP().toString().c_str());

  // Baca sensor awal
  float t = dht.readTemperature(), h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    lastT = t; lastH = h;
    Serial.printf("[DHT] ✅ Baca OK: %.1f°C | %.0f%%\n", lastT, lastH);
  } else {
    Serial.println("[DHT] ⚠️ Gagal baca awal.");
  }

  // CoAP setup
  coap.response(callbackResponse);
  coap.start();
  Serial.println("[CoAP] ✅ Client siap");
  Serial.printf("[INFO] POST tiap %lu detik ke Antares\n", POST_INTERVAL / 1000UL);
}

void loop() {
  // Cek WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] ⚠️ Terputus! Reconnect...");
    WiFi.reconnect(); delay(5000); return;
  }

  // WAJIB: proses incoming UDP
  coap.loop();

  // Baca DHT11 tiap 2 detik
  static unsigned long tDHT = 0;
  if (millis() - tDHT >= 2000UL) {
    tDHT = millis();
    float t = dht.readTemperature(), h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) { lastT = t; lastH = h; }
  }

  // POST tiap POST_INTERVAL
  static unsigned long tPost = 0;
  static bool firstRun = true;
  if (firstRun || millis() - tPost >= POST_INTERVAL) {
    firstRun = false;
    tPost = millis();

    if (isnan(lastT)) {
      Serial.println("[POST] ⚠️ Skip — sensor belum valid.");
    } else {
      // Resolve Antares hostname
      IPAddress antaresIP;
      if (!WiFi.hostByName(ANTARES_HOST, antaresIP)) {
        Serial.println("[CoAP] ❌ DNS gagal — coba lagi nanti.");
      } else {
        // Bangun payload oneM2M (stack-allocated)
        char conStr[64];
        snprintf(conStr, sizeof(conStr),
          "{\"temp\":%.1f,\"humidity\":%.0f}", lastT, lastH);

        StaticJsonDocument<256> doc;
        doc["m2m:cin"]["con"] = conStr;
        char payload[256];
        size_t payLen = serializeJson(doc, payload, sizeof(payload));

        postCount++;
        Serial.printf("[DHT] %.1f°C | %.0f%%\n", lastT, lastH);
        Serial.printf("[CoAP] → POST #%u: %s\n", postCount, coapPath);
        Serial.printf("[CoAP] Payload: %s\n", payload);

        uint16_t msgId = coap.post(antaresIP, ANTARES_PORT, coapPath, payload);
        if (msgId > 0) {
          Serial.printf("[CoAP] Terkirim (msgId:%u) — tunggu response...\n", msgId);
        } else {
          Serial.println("[CoAP] ❌ Gagal kirim POST!");
        }
        updateOLED();
      }
    }
  }

  // Heartbeat
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] %.1fC %.0f%% | Post:%u OK:%u | Heap:%u B | Up:%lu s\n",
      isnan(lastT)?0.0f:lastT, isnan(lastH)?0.0f:lastH,
      postCount, okCount, ESP.getFreeHeap(), millis()/1000UL);
  }
}
```

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 51 Program 2: CoAP POST — Antares.id ===
[INFO] Host: platform.antares.id:5683
[INFO] Path: /~/ACCESSKEY.../bluino-kit/sensor-ruangan
[OLED] ✅ Display terdeteksi di 0x3C
[WiFi] ✅ Terhubung! IP: 192.168.1.105
[DHT] ✅ Baca OK: 29.1°C | 65%
[CoAP] ✅ Client siap
[INFO] POST tiap 15 detik ke Antares
[DHT] 29.1°C | 65%
[CoAP] → POST #1: /~/ACCESSKEY.../bluino-kit/sensor-ruangan
[CoAP] Payload: {"m2m:cin":{"con":"{\"temp\":29.1,\"humidity\":65}"}}
[CoAP] Terkirim (msgId:1) — tunggu response...
[CoAP] ← Response: 2.01
[CoAP] ✅ Antares menerima data! Total OK: 1 | Heap: 210800 B
[HB] 29.1C 65% | Post:4 OK:4 | Heap:210400 B | Up:61 s
```

> 💡 **CoAP vs HTTP untuk IoT Baterai:**
> HTTP POST membutuhkan TCP 3-way handshake (~3 round trips) + header besar.
> CoAP POST hanya **1 UDP packet** → hemat daya secara signifikan.
> Pada perangkat NB-IoT dengan baterai AA, ini bisa berarti perbedaan bertahan **3 bulan vs 18 bulan**.

---

## 51.7 Program 3: CoAP Server — ESP32 sebagai Endpoint IoT

Program paling unik di BAB 51: **ESP32 bertindak sebagai CoAP Server**. Perangkat lain (laptop, smartphone, ESP32 lain) dapat langsung *query* data sensor atau kontrol relay ke ESP32 menggunakan CoAP — tanpa perlu cloud!

### Skenario Penggunaan
```
Laptop/HP (CoAP Client)                ESP32 (CoAP Server)
      │                                       │
      ├── GET coap://192.168.1.105/sensor ───►│ → balas JSON suhu & kelembaban
      ├── GET coap://192.168.1.105/relay  ───►│ → balas status relay
      └── PUT coap://192.168.1.105/relay  ───►│ → ubah status relay
          payload: ON / OFF                   │
```

```cpp
/*
 * BAB 51 - Program 3: CoAP Server — ESP32 sebagai Endpoint IoT
 *
 * Arsitektur: Edge IoT — ESP32 melayani request langsung, tanpa cloud
 *
 * Fitur:
 *   ► ESP32 berjalan sebagai CoAP Server di port 5683
 *   ► Endpoint /sensor → GET: return JSON suhu & kelembaban DHT11
 *   ► Endpoint /relay  → GET: return status relay
 *                      → PUT: ubah status relay (payload: "ON" / "OFF")
 *   ► Semua CoAP client di jaringan LAN yang sama dapat query ESP32 ini
 *   ► OLED tampilkan IP, request counter, suhu
 *   ► Non-blocking DHT11 read tiap 2 detik
 *   ► Heartbeat tiap 60 detik
 *
 * Hardware:
 *   DHT11 → IO27, Relay → IO4, OLED → SDA=IO21, SCL=IO22
 *
 * Cara Uji (dari laptop/PC di jaringan yang sama):
 *   Install coap-client: sudo apt install libcoap-bin  (Linux)
 *                        brew install libcoap           (macOS)
 *
 *   GET sensor: coap-client -m get coap://192.168.1.105/sensor
 *   GET relay : coap-client -m get coap://192.168.1.105/relay
 *   SET relay : coap-client -m put coap://192.168.1.105/relay -e "ON"
 *
 *   Atau gunakan MQTT Explorer versi CoAP: Copper (Firefox addon)
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Konfigurasi WiFi ──────────────────────────────────────────────────
const char* WIFI_SSID = "NamaWiFiKamu";  // ← Ganti!
const char* WIFI_PASS = "PasswordWiFi";  // ← Ganti!

// ── Hardware ──────────────────────────────────────────────────────────
#define DHT_PIN   27
#define RELAY_PIN 4
DHT dht(DHT_PIN, DHT11);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool oledReady = false;

// ── State ─────────────────────────────────────────────────────────────
float    lastT      = NAN;
float    lastH      = NAN;
bool     relayState = false;
uint32_t reqCount   = 0;

// ── Objek CoAP Server ─────────────────────────────────────────────────
WiFiUDP udp;
Coap    coap(udp);

// ── OLED Helper ───────────────────────────────────────────────────────
void updateOLED() {
  if (!oledReady) return;
  oled.clearDisplay(); oled.setTextColor(WHITE); oled.setTextSize(1);
  oled.setCursor(0, 0); oled.print("CoAP Server");
  oled.setCursor(0, 10); oled.print("IP:"); oled.print(WiFi.localIP());
  oled.drawLine(0, 20, 127, 20, WHITE);
  oled.setTextSize(2); oled.setCursor(0, 24);
  if (!isnan(lastT)) { oled.print(lastT, 1); oled.print("C"); }
  else oled.print("---");
  oled.setTextSize(1); oled.setCursor(0, 46);
  oled.print("Req:#"); oled.print(reqCount);
  oled.print(" RLY:"); oled.print(relayState ? "ON" : "OFF");
  oled.display();
}

// ── Callback: Endpoint /sensor ────────────────────────────────────────
// Melayani GET /sensor → return JSON suhu & kelembaban
void callbackSensor(CoapPacket &packet, IPAddress clientIP, int clientPort) {
  reqCount++;
  Serial.printf("[CoAP] ← %s GET /sensor (#%u)\n",
    clientIP.toString().c_str(), reqCount);

  // Bangun JSON response (float langsung → tidak ada ambiguitas type)
  StaticJsonDocument<128> doc;
  if (!isnan(lastT)) {
    doc["temp"]     = lastT;           // → 29.1 (number, bukan "29.1" string)
    doc["humidity"] = (int)lastH;      // → 65   (integer)
  } else {
    doc["error"] = "sensor_unavailable";
  }
  doc["uptime"] = millis() / 1000UL;

  char payload[128];
  size_t payLen = serializeJson(doc, payload, sizeof(payload));

  Serial.printf("[CoAP] → Response: %s\n", payload);

  // Kirim response 2.05 Content
  coap.sendResponse(clientIP, clientPort, packet.messageid,
    payload, payLen,
    COAP_RESPONSE_CODE(CONTENT),
    COAP_CONTENT_TYPE(APPLICATION_JSON),
    packet.token, packet.tokenlen);

  updateOLED();
}

// ── Callback: Endpoint /relay ─────────────────────────────────────────
// Melayani GET /relay → return status
//           PUT /relay → ubah status (payload: "ON" / "OFF")
void callbackRelay(CoapPacket &packet, IPAddress clientIP, int clientPort) {
  reqCount++;

  if (packet.code == COAP_GET) {
    // GET: balas status relay
    Serial.printf("[CoAP] ← %s GET /relay (#%u)\n",
      clientIP.toString().c_str(), reqCount);

    char payload[32];
    snprintf(payload, sizeof(payload), "{\"relay\":\"%s\"}",
      relayState ? "ON" : "OFF");

    coap.sendResponse(clientIP, clientPort, packet.messageid,
      payload, strlen(payload),
      COAP_RESPONSE_CODE(CONTENT),
      COAP_CONTENT_TYPE(APPLICATION_JSON),
      packet.token, packet.tokenlen);

    Serial.printf("[CoAP] → Response: %s\n", payload);

  } else if (packet.code == COAP_PUT) {
    // PUT: ubah status relay
    char cmd[16] = "";
    size_t copyLen = min((size_t)packet.payloadlen, sizeof(cmd) - 1);
    if (copyLen > 0) { memcpy(cmd, packet.payload, copyLen); }

    Serial.printf("[CoAP] ← %s PUT /relay cmd='%s' (#%u)\n",
      clientIP.toString().c_str(), cmd, reqCount);

    if (strcmp(cmd, "ON") == 0) {
      relayState = true;
      digitalWrite(RELAY_PIN, HIGH);
      Serial.println("[RELAY] ✅ Relay → ON");
      coap.sendResponse(clientIP, clientPort, packet.messageid,
        "OK", 2, COAP_RESPONSE_CODE(CHANGED),
        COAP_CONTENT_TYPE(TEXT_PLAIN),
        packet.token, packet.tokenlen);
    } else if (strcmp(cmd, "OFF") == 0) {
      relayState = false;
      digitalWrite(RELAY_PIN, LOW);
      Serial.println("[RELAY] ✅ Relay → OFF");
      coap.sendResponse(clientIP, clientPort, packet.messageid,
        "OK", 2, COAP_RESPONSE_CODE(CHANGED),
        COAP_CONTENT_TYPE(TEXT_PLAIN),
        packet.token, packet.tokenlen);
    } else {
      Serial.printf("[RELAY] ⚠️ Perintah tidak dikenal: '%s'\n", cmd);
      coap.sendResponse(clientIP, clientPort, packet.messageid,
        "BAD_REQUEST", 11, COAP_RESPONSE_CODE(BAD_REQUEST),
        COAP_CONTENT_TYPE(TEXT_PLAIN),
        packet.token, packet.tokenlen);
    }
  }
  updateOLED();
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT); digitalWrite(RELAY_PIN, LOW);
  dht.begin();
  Wire.begin(21, 22);

  Serial.println("\n=== BAB 51 Program 3: CoAP Server — ESP32 Endpoint ===");

  if (oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledReady = true;
    Serial.println("[OLED] ✅ Display terdeteksi di 0x3C");
  }
  oled.clearDisplay(); oled.setTextColor(WHITE);
  oled.setCursor(0, 0);  oled.print("BAB 51 CoAP Srv");
  oled.setCursor(0, 12); oled.print("WiFi..."); oled.display();

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[WiFi] Menghubungkan ke '%s'...\n", WIFI_SSID);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - t0 >= 15000UL) { delay(2000); ESP.restart(); }
    Serial.print("."); delay(500);
  }
  Serial.printf("\n[WiFi] ✅ Terhubung! IP: %s\n",
    WiFi.localIP().toString().c_str());

  // CoAP Server setup — daftarkan endpoint SEBELUM coap.start()
  coap.server(callbackSensor, "sensor");  // → coap://IP/sensor
  coap.server(callbackRelay,  "relay");   // → coap://IP/relay
  coap.start();                           // Buka port UDP 5683

  Serial.println("[CoAP] ✅ Server siap di port 5683");
  Serial.printf("[CoAP] Endpoint: coap://%s/sensor\n", WiFi.localIP().toString().c_str());
  Serial.printf("[CoAP] Endpoint: coap://%s/relay\n", WiFi.localIP().toString().c_str());
  Serial.println("[CoAP] Cara uji: coap-client -m get coap://IP/sensor");

  // Baca sensor awal
  float t = dht.readTemperature(), h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) { lastT = t; lastH = h; }
  updateOLED();
}

void loop() {
  // Cek WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] ⚠️ Terputus! Reconnect...");
    WiFi.reconnect(); delay(5000); return;
  }

  // WAJIB: proses semua incoming CoAP request
  coap.loop();

  // Baca DHT11 non-blocking tiap 2 detik
  static unsigned long tDHT = 0;
  if (millis() - tDHT >= 2000UL) {
    tDHT = millis();
    float t = dht.readTemperature(), h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) { lastT = t; lastH = h; }
  }

  // Heartbeat
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] %.1fC %.0f%% | Relay:%s | Req:%u | Heap:%u B | Up:%lu s\n",
      isnan(lastT)?0.0f:lastT, isnan(lastH)?0.0f:lastH,
      relayState?"ON":"OFF", reqCount, ESP.getFreeHeap(), millis()/1000UL);
  }
}
```

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 51 Program 3: CoAP Server — ESP32 Endpoint ===
[OLED] ✅ Display terdeteksi di 0x3C
[WiFi] ✅ Terhubung! IP: 192.168.1.105
[CoAP] ✅ Server siap di port 5683
[CoAP] Endpoint: coap://192.168.1.105/sensor
[CoAP] Endpoint: coap://192.168.1.105/relay
[CoAP] Cara uji: coap-client -m get coap://IP/sensor
--- (request dari client) ---
[CoAP] ← 192.168.1.10 GET /sensor (#1)
[CoAP] → Response: {"temp":29.1,"humidity":65,"uptime":45}
[CoAP] ← 192.168.1.10 PUT /relay cmd='ON' (#2)
[RELAY] ✅ Relay → ON
[HB] 29.1C 65% | Relay:ON | Req:2 | Heap:208000 B | Up:61 s
```

---

## 51.8 Troubleshooting CoAP

| Gejala | Penyebab | Solusi |
|--------|----------|--------|
| `coap.get()` return 0 | UDP gagal dikirim | Cek WiFi masih terhubung sebelum kirim |
| Callback tidak pernah dipanggil | `coap.loop()` tidak ada di `loop()` | Pastikan `coap.loop()` dipanggil setiap iterasi |
| DNS gagal resolve hostname | DNS timeout atau server salah | Cek koneksi, coba resolve manual dg `WiFi.hostByName()` |
| Response `4.01 Unauthorized` | ACCESS_KEY salah | Salin ulang Access Key dari dashboard Antares |
| Response `4.04 Not Found` | Path CoAP salah | Cek format `/~/{KEY}/{APP}/{DEVICE}` |
| POST berhasil (msgId>0) tapi tidak ada response | Firewall UDP atau server tidak ACK | Coba dengan NON message atau cek firewall router |
| `coap.start()` tidak bisa panggil `coap.response()` setelah | Callback harus didaftarkan SEBELUM `start()` | Panggil `coap.response(fn)` sebelum `coap.start()` |
| Server endpoint tidak merespons | `coap.server()` harus sebelum `coap.start()` | Daftarkan semua endpoint sebelum `coap.start()` |

> ⚠️ **POLA WAJIB — Urutan inisiasi CoAP yang benar:**
> ```cpp
> // CLIENT:
> coap.response(callbackFn);    // 1. Daftarkan response callback
> coap.start();                 // 2. Mulai UDP listener
> // Di loop():
> coap.loop();                  // 3. WAJIB setiap iterasi!
> coap.get(ip, port, "/path");  // 4. Kirim request (response via callback)
>
> // SERVER:
> coap.server(fn1, "endpoint1");  // 1. Daftarkan endpoint
> coap.server(fn2, "endpoint2");  // 2. Daftarkan endpoint lain
> coap.start();                   // 3. Mulai server
> // Di loop():
> coap.loop();                    // 4. WAJIB setiap iterasi!
> ```

---

## 51.9 Perbandingan HTTP vs MQTT vs CoAP

| Aspek | HTTP (BAB 48) | HTTPS (BAB 49) | MQTT (BAB 50) | CoAP (BAB 51) |
|-------|--------------|----------------|---------------|---------------|
| **Transport** | TCP | TCP+TLS | TCP | **UDP** |
| **Model** | Req-Resp | Req-Resp | Pub-Sub | Req-Resp |
| **Push** | ❌ Polling | ❌ Polling | ✅ Instan | ✅ (Observe) |
| **Min Header** | ~200 byte | ~200 byte | 2 byte | **4 byte** |
| **Handshake** | ✅ TCP | ✅ TCP+TLS | ✅ TCP | ❌ Tidak ada |
| **Enkripsi** | ❌ | ✅ TLS | ❌ (port 1338) | DTLS (port 5684) |
| **Cocok untuk** | REST API | API produksi | Monitoring IoT | **Sensor baterai** |
| **Library ESP32** | HTTPClient | WiFiClientSecure | PubSubClient | coap-simple |

---

## 51.10 Ringkasan

```
RINGKASAN BAB 51 — CoAP ESP32

PRINSIP DASAR:
  CoAP = HTTP yang diringkas untuk UDP
  UDP = tidak ada handshake → hemat daya, hemat bandwidth
  Response datang ASINKRON via callback — bukan blocking seperti HTTP!

INISIASI CLIENT (urutan WAJIB):
  WiFiUDP udp;
  Coap coap(udp);
  coap.response(callbackFn);  ← Daftarkan callback SEBELUM start!
  coap.start();               ← Buka UDP listener port 5683
  // Di loop():
  coap.loop();                ← WAJIB setiap iterasi!

INISIASI SERVER (urutan WAJIB):
  coap.server(fn, "endpoint");  ← Daftarkan SEBELUM start!
  coap.start();
  // Di loop():
  coap.loop();                  ← WAJIB setiap iterasi!

REQUEST (Client):
  uint16_t id = coap.get(ip, port, "/path");   // 0=gagal
  uint16_t id = coap.post(ip, port, "/path", payload);
  uint16_t id = coap.put(ip, port, "/path", payload);

RESPONSE CALLBACK (Client):
  void fn(CoapPacket &packet, IPAddress ip, int port) {
    uint8_t cls = packet.code >> 5;    // 2=OK, 4=ClientErr, 5=ServerErr
    uint8_t dtl = packet.code & 0x1F;
    char buf[256];
    memcpy(buf, packet.payload, packet.payloadlen);
    buf[packet.payloadlen] = '\0';  // WAJIB! Tidak null-terminated
  }

KIRIM RESPONSE (Server):
  coap.sendResponse(ip, port, packet.messageid,
    payload, payloadLen,
    COAP_RESPONSE_CODE(CONTENT),           // 2.05
    COAP_CONTENT_TYPE(APPLICATION_JSON),
    packet.token, packet.tokenlen);

FORMAT ANTARES CoAP:
  Path   : /~/{ACCESS_KEY}/{APP_NAME}/{DEVICE_NAME}
  Method : POST
  Payload: {"m2m:cin":{"con":"{\"temp\":29.1}"}}
  Sukses : Response 2.01 Created

ANTIPATTERN:
  ❌ Lupa coap.loop()               → callback tidak pernah terpanggil
  ❌ coap.response() setelah start() → callback tidak terdaftar
  ❌ Tidak null-terminate payload    → String corrupt / crash
  ❌ Blocking di callback            → UDP packets lain terlewat
```

---

## 51.11 Latihan

### Latihan Dasar
1. Upload **Program 1**, buka Serial Monitor → verifikasi response `World!` dari `coap.me/hello`.
2. Ganti `COAP_RESOURCE` di Program 1 menjadi `/test` → amati perbedaan response.
3. Upload **Program 2**, kirim data DHT11 ke Antares → verifikasi data muncul di Dashboard.

### Latihan Menengah
4. Modifikasi **Program 2**: tambahkan field `"uptime": millis()/1000` ke payload JSON.
5. Modifikasi **Program 3**: tambahkan endpoint `/info` yang mengembalikan JSON berisi IP, MAC, dan uptime ESP32.
6. Dari laptop dengan `coap-client` terinstall, kirim PUT `/relay ON` ke ESP32 Program 3 → verifikasi Relay IO4 aktif.

### Tantangan Lanjutan
7. **CoAP Observe:** Riset ekstensi Observe di library `coap-simple` — implementasikan agar client mendapat update sensor otomatis saat suhu berubah > 1°C.
8. **CoAP over DTLS:** Ganti port ke 5684, tambahkan `WiFiClientSecure` dengan `setInsecure()` untuk enkripsi UDP (DTLS).
9. **Multi-Client:** Jalankan Program 3, lalu query dari dua device berbeda secara bersamaan → amati apakah keduanya dilayani dengan benar.

---

## 51.12 Preview: Apa Selanjutnya? (→ BAB 52)

```
BAB 51 (CoAP) — Protokol UDP ringan, cocok untuk sensor baterai:
  ESP32 ── UDP CoAP ──► Server/Client lokal atau cloud

BAB 52 (Bluetooth Classic) — Komunikasi nirkabel jarak dekat:
  ESP32 ── Bluetooth SPP ──► Smartphone/PC/Arduino lain
  Tidak perlu WiFi! Jarak hingga ~10 meter.

Perbedaan utama:
  CoAP → membutuhkan WiFi/IP network
  Bluetooth Classic → peer-to-peer langsung, tanpa router
```

---

## 51.13 Referensi

| Sumber | Link |
|--------|------|
| coap-simple Library | https://github.com/hirotakaster/coap-simple |
| CoAP Specification (RFC 7252) | https://datatracker.ietf.org/doc/html/rfc7252 |
| CoAP Observe (RFC 7641) | https://datatracker.ietf.org/doc/html/rfc7641 |
| Antares CoAP Documentation | https://antares.id/id/docs.html |
| Eclipse coap.me (public server) | https://coap.me |
| coap-client tool (Linux/macOS) | https://libcoap.net |

---

## ☕ Dukung Proyek Ini

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

---

*Sebelumnya → [BAB 50: MQTT](../BAB-50/README.md)*

*Selanjutnya → [BAB 52: Bluetooth Classic](../BAB-52/README.md)*
