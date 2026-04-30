# 📡 BAB 50 — MQTT: Message Queuing Telemetry Transport

> ✅ **Prasyarat:** Selesaikan **BAB 49 (HTTPS & Keamanan)**. Kamu harus sudah memahami WiFi, HTTP Client, dan ArduinoJson.

> 📖 **Glosarium Singkatan BAB ini:**
> | Singkatan | Kepanjangan |
> |-----------|-------------|
> | **MQTT** | *Message Queuing Telemetry Transport* — protokol ringan Pub/Sub IoT |
> | **Broker** | Server pusat yang menerima dan mendistribusikan pesan MQTT |
> | **Pub** | *Publisher* — pengirim pesan ke topik |
> | **Sub** | *Subscriber* — penerima pesan dari topik |
> | **QoS** | *Quality of Service* — jaminan pengiriman pesan |
> | **LWT** | *Last Will & Testament* — pesan otomatis saat perangkat putus |
> | **Topic** | Alamat/saluran pesan dalam MQTT |

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami perbedaan arsitektur **Pub/Sub** (MQTT) vs **Request-Response** (HTTP).
- Menggunakan library `PubSubClient` untuk connect, publish, dan subscribe ke broker MQTT.
- Membangun payload **oneM2M** (`m2m:cin`) untuk integrasi dengan **Antares.id**.
- Mengimplementasi **non-blocking reconnect** untuk WiFi dan MQTT broker.
- Memahami dan menerapkan **LWT** untuk monitoring ketersediaan perangkat IoT.

---

## 50.1 Pergeseran Paradigma: HTTP → MQTT

```
HTTP (BAB 48-49) — Request-Response:

  ESP32 ──── GET/POST ────► Server
  ESP32 ◄─── Response ─────  Server
  (ESP32 harus memulai setiap komunikasi)

MQTT (BAB 50) — Publish-Subscribe:

         ┌──────────────────────────┐
         │    MQTT Broker (Antares) │
         └────────┬─────────────────┘
      PUBLISH     │          SUBSCRIBE
  ESP32 ─────────►│◄─────── Dashboard/App
  (kirim data)    │         (terima data)
  
  Broker mendistribusikan pesan ke semua subscriber!
  ESP32 tidak perlu tahu siapa yang mendengarkan.
```

| Aspek | HTTP/HTTPS (BAB 48-49) | MQTT (BAB 50) |
|-------|------------------------|---------------|
| **Model** | Request-Response | Publish-Subscribe |
| **Inisiator** | Client selalu memulai | Broker mendistribusikan |
| **Koneksi** | Buka-tutup per request | **Persisten** — tetap terhubung |
| **Real-time** | ❌ Polling berkala | ✅ Push instan ke subscriber |
| **Overhead** | Header besar per request | Header sangat kecil (min 2 byte) |
| **Cocok untuk** | REST API, upload berkala | Monitoring real-time, kontrol IoT |

---

## 50.2 Komponen MQTT

### 1. Broker
Server pusat yang menerima dan mendistribusikan semua pesan. Di BAB 50 kita gunakan **Antares.id** sebagai broker (port 1338).

### 2. Topic (Topik)
Alamat pesan dalam MQTT, menggunakan format hierarki dengan `/` sebagai pemisah:
```
bluino/sensor/suhu          ← topik sederhana
bluino/+/data               ← wildcard satu level
bluino/#                    ← wildcard semua level

Format oneM2M Antares:
  Publish : /oneM2M/req/{ACCESS_KEY}/antares-cse/{APP}/{DEVICE}/json
  Subscribe: /oneM2M/resp/antares-cse/{ACCESS_KEY}/json
```

### 3. Quality of Service (QoS)
| QoS | Nama | Jaminan |
|-----|------|---------|
| 0 | At most once | Pesan dikirim sekali, mungkin hilang |
| 1 | At least once | Pesan dijamin terkirim, mungkin duplikat |
| 2 | Exactly once | Tepat sekali (overhead tertinggi) |

> `PubSubClient` mendukung QoS 0 dan 1. Untuk monitoring sensor IoT, QoS 0 sudah cukup.

### 4. Last Will & Testament (LWT)
LWT adalah pesan yang dititipkan ESP32 ke broker **saat pertama kali terhubung**. Jika ESP32 putus secara tidak normal, broker otomatis mempublish pesan LWT ini.

```
Skenario Normal:
  ESP32 terhubung → publish "ONLINE" ke topik status
  ESP32 disconnect normal → broker TIDAK kirim LWT

Skenario Darurat (ESP32 mati mendadak):
  Broker deteksi koneksi terputus tanpa disconnect proper
  Broker otomatis publish "OFFLINE" → semua subscriber tahu!
```

---

## 50.3 Library & API Reference

```
LIBRARY YANG DIGUNAKAN:

  PubSubClient.h  → Install via Library Manager:
                    Cari "PubSubClient" → pilih by Nick O'Leary → install
  ArduinoJson.h   → Install: Cari "ArduinoJson" by bblanchon → versi >= 6.x

API PubSubClient ESENSIAL:
  mqtt.setServer(host, port)           → Set alamat broker
  mqtt.setCallback(fn)                 → Daftarkan callback untuk Sub
  mqtt.setBufferSize(512)              → ← WAJIB! Default 256 byte terlalu kecil
  mqtt.connect(id, user, pass)         → Connect ke broker
  mqtt.connect(id, user, pass,         → Connect dengan LWT
               willTopic, willQos,
               willRetain, willMsg)
  mqtt.publish(topic, payload)         → Publish pesan (String)
  mqtt.publish(topic, buf, len, retain)→ Publish binary dengan opsi retain
  mqtt.subscribe(topic)                → Subscribe ke topik
  mqtt.loop()                          → ← WAJIB di setiap loop()!
  mqtt.connected()                     → Cek status koneksi
  mqtt.state()                         → Kode error koneksi

MQTT STATE CODE (jika connected() = false):
  -4 → MQTT_CONNECTION_TIMEOUT       → Broker tidak merespons
  -3 → MQTT_CONNECTION_LOST          → Koneksi terputus
  -2 → MQTT_CONNECT_FAILED           → Gagal connect (salah host/port?)
  -1 → MQTT_DISCONNECTED             → Belum pernah connect
   1 → MQTT_CONNECT_BAD_PROTOCOL     → Versi protokol tidak cocok
   2 → MQTT_CONNECT_BAD_CLIENT_ID    → Client ID ditolak broker
   3 → MQTT_CONNECT_UNAVAILABLE      → Broker sedang tidak tersedia
   4 → MQTT_CONNECT_BAD_CREDENTIALS  → Username/password salah
   5 → MQTT_CONNECT_UNAUTHORIZED     → Tidak diizinkan connect
```

---

## 50.4 Konfigurasi Antares MQTT

### Parameter Koneksi
| Parameter | Nilai |
|-----------|-------|
| Host | `platform.antares.id` |
| Port | `1338` (plain MQTT, tanpa enkripsi) |
| Username | Access Key Antares Anda |
| Password | Access Key Antares Anda (sama dengan username) |
| Client ID | String unik, misal `"BluinoKit-A1B2C3"` |

> ⚠️ **Catatan:** Antares menggunakan port **1338** untuk MQTT (bukan port standar 1883). Port ini plain text — untuk produksi pertimbangkan MQTT over TLS (port 8883).

### Cara Siapkan Antares (jika belum dari BAB 49)
1. Buka [https://antares.id](https://antares.id) → **Daftar** (gratis)
2. Login → Klik profil (kanan atas) → Salin **Access Key**
3. Buat **Application** (misal: `bluino-kit`)
4. Di dalam Application → buat **Device** (misal: `sensor-ruangan`)

---

## 50.5 Program 1: MQTT Publish — Kirim Data DHT11 ke Antares

ESP32 membaca DHT11 lalu **mempublish** data sensor ke Antares setiap 15 detik. Data langsung muncul di Dashboard Antares secara real-time tanpa perlu polling.

```cpp
/*
 * BAB 50 - Program 1: MQTT Publish — Kirim Data DHT11 ke Antares.id
 *
 * Fitur:
 *   ▶ Connect ke MQTT Broker Antares.id (port 1338)
 *   ▶ Publish data DHT11 (suhu & kelembaban) setiap 15 detik
 *   ▶ Payload format oneM2M (m2m:cin) — standar Antares
 *   ▶ Non-blocking reconnect WiFi & MQTT via millis()
 *   ▶ setBufferSize(512) — WAJIB agar payload oneM2M tidak terpotong
 *   ▶ Tampil status di OLED & Serial Monitor
 *   ▶ Heartbeat tiap 60 detik
 *
 * Hardware:
 *   DHT11 → IO27 (hardwired Bluino Kit)
 *   OLED  → SDA=IO21, SCL=IO22 (hardwired)
 *
 * Library:
 *   PubSubClient by Nick O'Leary (Library Manager)
 *   ArduinoJson by bblanchon >= 6.x
 *
 * Cara Uji:
 *   1. Install PubSubClient via Library Manager
 *   2. Isi WIFI_SSID, WIFI_PASS, ANTARES_ACCESS_KEY, ANTARES_APP, ANTARES_DEVICE
 *   3. Upload → Serial Monitor 115200 baud
 *   4. Buka Dashboard Antares → lihat data sensor masuk tiap 15 detik
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Konfigurasi WiFi ──────────────────────────────────────────────────
const char* WIFI_SSID = "NamaWiFiKamu";   // ← Ganti!
const char* WIFI_PASS = "PasswordWiFi";   // ← Ganti!

// ── Konfigurasi Antares MQTT ──────────────────────────────────────────
const char* ANTARES_HOST       = "platform.antares.id";
const int   ANTARES_PORT       = 1338;
const char* ANTARES_ACCESS_KEY = "1234567890abcdef:1234567890abcdef"; // ← Ganti!
const char* ANTARES_APP        = "bluino-kit";       // ← Ganti!
const char* ANTARES_DEVICE     = "sensor-ruangan";   // ← Ganti!

const unsigned long PUB_INTERVAL = 15000UL;  // 15 detik

// ── Topik (dibangun saat setup) ───────────────────────────────────────
char TOPIC_PUB[160];

// ── Hardware ──────────────────────────────────────────────────────────
#define DHT_PIN 27
DHT dht(DHT_PIN, DHT11);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);

// ── State ─────────────────────────────────────────────────────────────
float    lastT    = NAN;
float    lastH    = NAN;
uint32_t pubCount = 0;
char     clientId[24];

// ── Objek MQTT ────────────────────────────────────────────────────────
WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);

// ── Fungsi: Connect ke broker MQTT ───────────────────────────────────
bool mqttConnect() {
  Serial.printf("[MQTT] Menghubungkan ke broker... (ID: %s)\n", clientId);
  // connect(clientId, username, password)
  // Antares: username = password = ACCESS_KEY
  bool ok = mqtt.connect(clientId, ANTARES_ACCESS_KEY, ANTARES_ACCESS_KEY);
  if (ok) {
    Serial.printf("[MQTT] ✅ Terhubung! Client ID: %s\n", clientId);
  } else {
    Serial.printf("[MQTT] ❌ Gagal, rc=%d — lihat tabel state code di 50.3\n", mqtt.state());
  }
  return ok;
}

// ── Fungsi: Publish data sensor ke Antares ───────────────────────────
void publishSensor() {
  if (isnan(lastT)) {
    Serial.println("[PUB] ⚠️ Skip — belum ada data sensor valid.");
    return;
  }

  // Bangun 'con': JSON data sensor yang sebenarnya
  char conStr[64];
  snprintf(conStr, sizeof(conStr), "{\"temp\":%.1f,\"humidity\":%.0f}", lastT, lastH);

  // Bangun body oneM2M di stack (zero heap fragmentation)
  StaticJsonDocument<256> doc;
  JsonObject cin = doc.createNestedObject("m2m:cin");
  cin["con"] = conStr;

  char body[256];
  size_t bodyLen = serializeJson(doc, body, sizeof(body));

  Serial.printf("[MQTT] Publish #%u → %s\n", pubCount + 1, TOPIC_PUB);
  Serial.printf("[MQTT] Body: %s\n", body);

  // publish(topic, payload, length, retained)
  bool ok = mqtt.publish(TOPIC_PUB, (uint8_t*)body, bodyLen, false);
  if (ok) {
    pubCount++;
    Serial.printf("[MQTT] ✅ Publish berhasil! Heap: %u B\n", ESP.getFreeHeap());

    oled.clearDisplay();
    oled.setTextSize(1); oled.setTextColor(WHITE);
    oled.setCursor(0, 0);  oled.print("MQTT Pub OK!");
    oled.setCursor(0, 12); oled.print("T:"); oled.print(lastT, 1);
    oled.print("C H:"); oled.print(lastH, 0); oled.print("%");
    oled.setCursor(0, 24); oled.print("Pub: #"); oled.print(pubCount);
    oled.setCursor(0, 36); oled.print("Heap: "); oled.print(ESP.getFreeHeap()); oled.print("B");
    oled.display();
  } else {
    Serial.println("[MQTT] ❌ Publish gagal! Cek koneksi broker atau bufferSize.");
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  Wire.begin(21, 22);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.clearDisplay(); oled.setTextColor(WHITE);

  // Client ID unik dari 3 byte terakhir MAC address
  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(clientId, sizeof(clientId), "BluinoKit-%02X%02X%02X", mac[3], mac[4], mac[5]);

  // Bangun topik publish
  snprintf(TOPIC_PUB, sizeof(TOPIC_PUB),
    "/oneM2M/req/%s/antares-cse/%s/%s/json",
    ANTARES_ACCESS_KEY, ANTARES_APP, ANTARES_DEVICE);

  Serial.println("\n=== BAB 50 Program 1: MQTT Publish — Antares.id ===");
  Serial.printf("[INFO] Broker: %s:%d\n", ANTARES_HOST, ANTARES_PORT);
  Serial.printf("[INFO] App: %s | Device: %s\n", ANTARES_APP, ANTARES_DEVICE);
  Serial.printf("[INFO] Client ID: %s\n", clientId);
  Serial.printf("[INFO] Publish topic: %s\n", TOPIC_PUB);

  // OLED splash
  oled.setCursor(0, 0);  oled.print("BAB 50 MQTT Pub");
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
  Serial.printf("\n[WiFi] ✅ Terhubung! IP: %s | SSID: %s\n",
    WiFi.localIP().toString().c_str(), WiFi.SSID().c_str());

  // MQTT setup — WAJIB setBufferSize sebelum connect!
  mqtt.setServer(ANTARES_HOST, ANTARES_PORT);
  mqtt.setBufferSize(512);  // ← Default 256 byte terlalu kecil untuk payload oneM2M

  // Baca sensor pertama
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    lastT = t; lastH = h;
    Serial.printf("[DHT] ✅ Baca OK: %.1f°C | %.0f%%\n", lastT, lastH);
  } else {
    Serial.println("[DHT] ⚠️ Gagal baca awal. Akan coba di loop().");
  }

  // Connect MQTT pertama kali
  mqttConnect();
  Serial.printf("[INFO] Publish tiap %lu detik ke Antares\n", PUB_INTERVAL / 1000UL);
}

void loop() {
  // ── Cek WiFi ────────────────────────────────────────────────────────
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] ⚠️ Terputus! Reconnect...");
    WiFi.reconnect();
    delay(5000); return;
  }

  // ── Reconnect MQTT (non-blocking, max 1 percobaan tiap 5 detik) ─────
  static unsigned long tRecon = 0;
  if (!mqtt.connected() && millis() - tRecon >= 5000UL) {
    tRecon = millis();
    mqttConnect();
  }

  // ── WAJIB: proses keep-alive & incoming data ─────────────────────────
  mqtt.loop();

  // ── Baca sensor + publish tiap PUB_INTERVAL ──────────────────────────
  static unsigned long tPub = 0;
  static bool firstRun = true;
  if (firstRun || millis() - tPub >= PUB_INTERVAL) {
    firstRun = false;
    tPub = millis();

    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      lastT = t; lastH = h;
      Serial.printf("[DHT] ✅ Baca OK: %.1f°C | %.0f%%\n", lastT, lastH);
    } else {
      Serial.println("[DHT] ⚠️ Gagal baca! Cek IO27.");
    }

    if (mqtt.connected()) {
      publishSensor();
    } else {
      Serial.println("[PUB] ⚠️ MQTT belum tersambung, skip publish.");
    }
  }

  // ── Heartbeat tiap 60 detik ───────────────────────────────────────────
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] %.1fC %.0f%% | Pub: %u | Heap: %u B | Uptime: %lu s\n",
      isnan(lastT) ? 0.0f : lastT,
      isnan(lastH) ? 0.0f : lastH,
      pubCount, ESP.getFreeHeap(), millis() / 1000UL);
  }
}
```

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 50 Program 1: MQTT Publish — Antares.id ===
[INFO] Broker: platform.antares.id:1338
[INFO] App: bluino-kit | Device: sensor-ruangan
[INFO] Client ID: BluinoKit-A1B2C3
[WiFi] Menghubungkan ke 'WiFiRumah'..............
[WiFi] ✅ Terhubung! IP: 192.168.1.105 | SSID: WiFiRumah
[DHT] ✅ Baca OK: 29.1°C | 65%
[MQTT] Menghubungkan ke broker... (ID: BluinoKit-A1B2C3)
[MQTT] ✅ Terhubung! Client ID: BluinoKit-A1B2C3
[INFO] Publish tiap 15 detik ke Antares
[DHT] ✅ Baca OK: 29.2°C | 65%
[MQTT] Publish #1 → /oneM2M/req/.../json
[MQTT] Body: {"m2m:cin":{"con":"{\"temp\":29.2,\"humidity\":65}"}}
[MQTT] ✅ Publish berhasil! Heap: 195200 B
[HB] 29.2C 65% | Pub: 4 | Heap: 194800 B | Uptime: 61 s
```

> ⚠️ **Antipattern WAJIB Dihindari:**
> - **Lupa `mqtt.setBufferSize(512)`** → payload oneM2M akan terpotong diam-diam → data tidak masuk Antares
> - **Lupa `mqtt.loop()`** → broker akan memutus koneksi setelah keep-alive timeout → reconnect terus
> - **Client ID duplikat** → dua device dengan ID sama akan saling kick dari broker

---

## 50.6 Program 2: MQTT Subscribe — Kontrol Relay via Antares

ESP32 **subscribe** ke topik Antares. Saat perintah dikirim dari Dashboard Antares, ESP32 menerima dan mengeksekusi perintah relay secara instan — tanpa perlu polling!

### Cara Mengirim Perintah dari Antares Dashboard
1. Login ke [https://antares.id](https://antares.id) → Application → Device Anda
2. Klik **Send Message** → isi payload:
   ```json
   {"m2m:cin":{"con":"{\"relay\":\"ON\"}"}}
   ```
   atau untuk mematikan: `{"m2m:cin":{"con":"{\"relay\":\"OFF\"}"}}`

```cpp
/*
 * BAB 50 - Program 2: MQTT Subscribe — Kontrol Relay via Antares.id
 *
 * Fitur:
 *   ► Subscribe ke topik response Antares (oneM2M)
 *   ► Parse payload nested JSON (m2m:rsp → m2m:cin → con)
 *   ► Kendalikan Relay IO4 berdasarkan perintah {"relay":"ON"/"OFF"}
 *   ► Non-blocking reconnect WiFi & MQTT
 *   ► Re-subscribe otomatis setelah MQTT reconnect
 *
 * Hardware:
 *   Relay → IO4 (hardwired Bluino Kit)
 *   OLED  → SDA=IO21, SCL=IO22
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Konfigurasi ────────────────────────────────────────────────────
const char* WIFI_SSID          = "NamaWiFiKamu";
const char* WIFI_PASS          = "PasswordWiFi";
const char* ANTARES_HOST       = "platform.antares.id";
const int   ANTARES_PORT       = 1338;
const char* ANTARES_ACCESS_KEY = "1234567890abcdef:1234567890abcdef"; // ← Ganti!
const char* ANTARES_APP        = "bluino-kit";       // ← Ganti!
const char* ANTARES_DEVICE     = "sensor-ruangan";   // ← Ganti!

// Topik subscribe: /oneM2M/resp/antares-cse/{ACCESS_KEY}/json
char TOPIC_SUB[160];

// ── Hardware ────────────────────────────────────────────────────────
#define RELAY_PIN 4
Adafruit_SSD1306 oled(128, 64, &Wire, -1);

// ── State ────────────────────────────────────────────────────────────
bool     relayState = false;
uint32_t msgCount   = 0;
char     clientId[24];

WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);

// ── OLED update ──────────────────────────────────────────────────────
void updateOLED() {
  oled.clearDisplay();
  oled.setTextSize(1); oled.setTextColor(WHITE);
  oled.setCursor(0, 0);  oled.print("MQTT Subscribe");
  oled.setCursor(0, 12); oled.print("Relay: ");
  oled.setTextSize(2);
  oled.setCursor(40, 10); oled.print(relayState ? "ON " : "OFF");
  oled.setTextSize(1);
  oled.setCursor(0, 36); oled.print("Msg masuk: #"); oled.print(msgCount);
  oled.setCursor(0, 48); oled.print("Heap: "); oled.print(ESP.getFreeHeap());
  oled.display();
}

// ── Callback: dipanggil tiap ada pesan masuk ─────────────────────────
void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  msgCount++;
  Serial.printf("[MQTT] Pesan masuk di: %s\n", topic);

  // Salin ke buffer null-terminated — payload TIDAK null-terminated!
  char buf[512];
  if (length >= sizeof(buf)) {
    Serial.println("[MQTT] ⚠️ Payload terlalu besar, diabaikan.");
    return;
  }
  memcpy(buf, payload, length);
  buf[length] = '\0';
  Serial.printf("[MQTT] Payload: %s\n", buf);

  // Parse JSON response oneM2M dari Antares
  // Struktur: {"m2m:rsp":{"pc":{"m2m:cin":{"con":"{\"relay\":\"ON\"}"}}}}
  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, buf)) {
    Serial.println("[MQTT] ❌ JSON parse gagal.");
    return;
  }

  // Ambil 'con' dari nested path
  const char* conStr = doc["m2m:rsp"]["pc"]["m2m:cin"]["con"];
  if (!conStr) {
    Serial.println("[MQTT] ⚠️ Field 'con' tidak ditemukan.");
    return;
  }

  // Parse JSON di dalam 'con' (double-encoded JSON)
  StaticJsonDocument<128> inner;
  if (deserializeJson(inner, conStr)) {
    Serial.println("[MQTT] ❌ Parse 'con' gagal.");
    return;
  }

  const char* relayCmd = inner["relay"];
  if (!relayCmd) {
    Serial.println("[MQTT] ⚠️ Field 'relay' tidak ada.");
    return;
  }

  if (strcmp(relayCmd, "ON") == 0) {
    relayState = true;
    digitalWrite(RELAY_PIN, HIGH);
    Serial.println("[RELAY] ✅ Relay → ON");
  } else if (strcmp(relayCmd, "OFF") == 0) {
    relayState = false;
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("[RELAY] ✅ Relay → OFF");
  } else {
    Serial.printf("[RELAY] ⚠️ Perintah tidak dikenal: '%s'\n", relayCmd);
  }

  updateOLED();
}

// ── Non-blocking MQTT reconnect (+ re-subscribe otomatis) ────────────
void mqttConnect() {
  Serial.printf("[MQTT] Menghubungkan... (ID: %s)\n", clientId);
  bool ok = mqtt.connect(clientId, ANTARES_ACCESS_KEY, ANTARES_ACCESS_KEY);
  if (ok) {
    Serial.println("[MQTT] ✅ Terhubung!");
    bool subOk = mqtt.subscribe(TOPIC_SUB);
    Serial.printf("[MQTT] %s Subscribe: %s\n", subOk ? "✅" : "❌", TOPIC_SUB);
  } else {
    Serial.printf("[MQTT] ❌ Gagal, rc=%d\n", mqtt.state());
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  Wire.begin(21, 22);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.clearDisplay(); oled.setTextColor(WHITE);

  uint8_t mac[6]; WiFi.macAddress(mac);
  snprintf(clientId, sizeof(clientId), "BluinoKit-%02X%02X%02X", mac[3], mac[4], mac[5]);

  snprintf(TOPIC_SUB, sizeof(TOPIC_SUB),
    "/oneM2M/resp/antares-cse/%s/json", ANTARES_ACCESS_KEY);

  Serial.println("\n=== BAB 50 Program 2: MQTT Subscribe — Antares.id ===");
  Serial.printf("[INFO] Broker: %s:%d\n", ANTARES_HOST, ANTARES_PORT);
  Serial.printf("[INFO] Subscribe: %s\n", TOPIC_SUB);

  oled.setCursor(0, 0);  oled.print("BAB 50 MQTT Sub");
  oled.setCursor(0, 12); oled.print("WiFi..."); oled.display();

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

  mqtt.setServer(ANTARES_HOST, ANTARES_PORT);
  mqtt.setBufferSize(512);
  mqtt.setCallback(onMqttMessage);  // ← Daftarkan callback SEBELUM connect

  mqttConnect();
}

void loop() {
  // Cek WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] ⚠️ Terputus! Reconnect...");
    WiFi.reconnect(); delay(5000); return;
  }

  // Reconnect MQTT non-blocking
  static unsigned long tRecon = 0;
  if (!mqtt.connected() && millis() - tRecon >= 5000UL) {
    tRecon = millis();
    mqttConnect(); // ← Re-subscribe otomatis di dalam fungsi ini
  }

  // WAJIB: proses incoming message & keep-alive
  mqtt.loop();

  // Heartbeat
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] Relay: %s | Msg: %u | Heap: %u B | Uptime: %lu s\n",
      relayState ? "ON" : "OFF", msgCount, ESP.getFreeHeap(), millis() / 1000UL);
  }
}
```

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 50 Program 2: MQTT Subscribe — Antares.id ===
[INFO] Broker: platform.antares.id:1338
[WiFi] ✅ Terhubung! IP: 192.168.1.105
[MQTT] Menghubungkan... (ID: BluinoKit-A1B2C3)
[MQTT] ✅ Terhubung!
[MQTT] ✅ Subscribe: /oneM2M/resp/antares-cse/.../json
--- (kirim perintah dari Dashboard Antares) ---
[MQTT] Pesan masuk di: /oneM2M/resp/antares-cse/.../json
[MQTT] Payload: {"m2m:rsp":{"pc":{"m2m:cin":{"con":"{\"relay\":\"ON\"}"}},...}}
[RELAY] ✅ Relay → ON
[HB] Relay: ON | Msg: 1 | Heap: 196400 B | Uptime: 60 s
```

> 💡 **Penting:** `mqtt.setCallback()` **WAJIB** dipanggil sebelum `mqtt.connect()`. Jika dipanggil setelah, callback tidak akan bekerja.

---

## 50.7 Program 3: MQTT Full IoT — Publish + Subscribe + LWT

Program puncak BAB 50: ESP32 **sekaligus** mengirim data sensor (Publisher) **dan** menerima perintah relay (Subscriber), dilengkapi **Last Will & Testament (LWT)** untuk monitoring ketersediaan perangkat secara otomatis.

```cpp
/*
 * BAB 50 - Program 3: MQTT Full IoT — Publish + Subscribe + LWT
 *
 * Arsitektur: Pub + Sub + Status Monitoring via LWT
 *
 * Fitur:
 *   ► PUBLISH: Data DHT11 ke Antares setiap 15 detik
 *   ► SUBSCRIBE: Terima & eksekusi perintah relay via callback
 *   ► LWT: Broker auto-publish "OFFLINE" jika ESP32 mati mendadak
 *   ► ONLINE: Publish status "ONLINE" setelah connect berhasil
 *   ► Non-blocking reconnect WiFi & MQTT
 *   ► Heartbeat tiap 30 detik
 *
 * Hardware:
 *   DHT11 → IO27, Relay → IO4, OLED → SDA=IO21, SCL=IO22
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Konfigurasi ────────────────────────────────────────────────────
const char* WIFI_SSID          = "NamaWiFiKamu";
const char* WIFI_PASS          = "PasswordWiFi";
const char* ANTARES_HOST       = "platform.antares.id";
const int   ANTARES_PORT       = 1338;
const char* ANTARES_ACCESS_KEY = "1234567890abcdef:1234567890abcdef"; // ← Ganti!
const char* ANTARES_APP        = "bluino-kit";       // ← Ganti!
const char* ANTARES_DEVICE     = "sensor-ruangan";   // ← Ganti!

const unsigned long PUB_INTERVAL = 15000UL;

// ── Topik ───────────────────────────────────────────────────────────
char TOPIC_PUB[160];     // publish data sensor
char TOPIC_SUB[160];     // subscribe perintah
char TOPIC_STATUS[160];  // LWT status (ONLINE / OFFLINE)

// ── Hardware ────────────────────────────────────────────────────────
#define DHT_PIN   27
#define RELAY_PIN 4
DHT dht(DHT_PIN, DHT11);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);

// ── State ─────────────────────────────────────────────────────────────
float    lastT      = NAN;
float    lastH      = NAN;
bool     relayState = false;
uint32_t pubCount   = 0;
uint32_t msgCount   = 0;
char     clientId[24];

WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);

// ── OLED update ──────────────────────────────────────────────────────
void updateOLED() {
  oled.clearDisplay();
  oled.setTextColor(WHITE); oled.setTextSize(1);

  // Baris 1: Status broker
  oled.setCursor(0, 0);
  oled.print(mqtt.connected() ? "MQTT:OK " : "MQTT:-- ");
  oled.print(relayState ? "[RLY:ON] " : "[RLY:OFF]");

  // Baris 2-3: Suhu besar
  oled.setTextSize(2); oled.setCursor(0, 12);
  if (!isnan(lastT)) {
    oled.print(lastT, 1); oled.print("C");
  } else { oled.print("---"); }

  // Baris 4: Kelembaban
  oled.setTextSize(1); oled.setCursor(0, 36);
  if (!isnan(lastH)) { oled.print("H: "); oled.print(lastH, 0); oled.print("%"); }

  // Baris 5: Counter
  oled.setCursor(0, 48);
  oled.print("Pub:#"); oled.print(pubCount);
  oled.print(" Msg:#"); oled.print(msgCount);
  oled.display();
}

// ── Callback: pesan masuk dari broker ───────────────────────────────
void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  msgCount++;
  char buf[512];
  if (length >= sizeof(buf)) return;
  memcpy(buf, payload, length); buf[length] = '\0';
  Serial.printf("[MQTT] Pesan masuk: %s\n", buf);

  // Parse oneM2M response
  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, buf)) { Serial.println("[MQTT] ❌ Parse gagal."); return; }

  const char* conStr = doc["m2m:rsp"]["pc"]["m2m:cin"]["con"];
  if (!conStr) { Serial.println("[MQTT] ⚠️ 'con' tidak ditemukan."); return; }

  StaticJsonDocument<128> inner;
  if (deserializeJson(inner, conStr)) { Serial.println("[MQTT] ❌ Parse 'con' gagal."); return; }

  const char* relayCmd = inner["relay"];
  if (!relayCmd) return;

  if (strcmp(relayCmd, "ON") == 0) {
    relayState = true; digitalWrite(RELAY_PIN, HIGH);
    Serial.println("[RELAY] ✅ Relay → ON");
  } else if (strcmp(relayCmd, "OFF") == 0) {
    relayState = false; digitalWrite(RELAY_PIN, LOW);
    Serial.println("[RELAY] ✅ Relay → OFF");
  }
  updateOLED();
}

// ── Connect MQTT dengan LWT ──────────────────────────────────────────
void mqttConnect() {
  Serial.printf("[MQTT] Menghubungkan... (ID: %s)\n", clientId);

  // LWT: jika ESP32 mati mendadak, broker publish "OFFLINE"
  bool ok = mqtt.connect(
    clientId,
    ANTARES_ACCESS_KEY,   // username
    ANTARES_ACCESS_KEY,   // password
    TOPIC_STATUS,         // willTopic
    0,                    // willQos
    true,                 // willRetain — pesan LWT disimpan broker
    "OFFLINE"             // willMessage
  );

  if (ok) {
    Serial.printf("[MQTT] ✅ Terhubung! LWT aktif di: %s\n", TOPIC_STATUS);

    // Publish status ONLINE
    mqtt.publish(TOPIC_STATUS, "ONLINE", true);  // retained
    Serial.println("[MQTT] Publish ONLINE → topik status");

    // Subscribe perintah
    bool subOk = mqtt.subscribe(TOPIC_SUB);
    Serial.printf("[MQTT] %s Subscribe: %s\n", subOk ? "✅" : "❌", TOPIC_SUB);
  } else {
    Serial.printf("[MQTT] ❌ Gagal, rc=%d\n", mqtt.state());
  }
}

// ── Publish sensor ────────────────────────────────────────────────────
void publishSensor() {
  if (isnan(lastT)) { Serial.println("[PUB] ⚠️ Skip — sensor belum valid."); return; }

  char conStr[64];
  snprintf(conStr, sizeof(conStr), "{\"temp\":%.1f,\"humidity\":%.0f}", lastT, lastH);

  StaticJsonDocument<256> doc;
  doc.createNestedObject("m2m:cin")["con"] = conStr;

  char body[256];
  size_t len = serializeJson(doc, body, sizeof(body));

  Serial.printf("[MQTT] Publish #%u → data sensor\n", pubCount + 1);
  if (mqtt.publish(TOPIC_PUB, (uint8_t*)body, len, false)) {
    pubCount++;
    Serial.printf("[MQTT] ✅ Publish berhasil! Heap: %u B\n", ESP.getFreeHeap());
  } else {
    Serial.println("[MQTT] ❌ Publish gagal!");
  }
  updateOLED();
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT); digitalWrite(RELAY_PIN, LOW);
  dht.begin();
  Wire.begin(21, 22);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.clearDisplay(); oled.setTextColor(WHITE);

  // Client ID dari MAC
  uint8_t mac[6]; WiFi.macAddress(mac);
  snprintf(clientId, sizeof(clientId), "BluinoKit-%02X%02X%02X", mac[3], mac[4], mac[5]);

  // Bangun semua topik
  snprintf(TOPIC_PUB, sizeof(TOPIC_PUB),
    "/oneM2M/req/%s/antares-cse/%s/%s/json",
    ANTARES_ACCESS_KEY, ANTARES_APP, ANTARES_DEVICE);
  snprintf(TOPIC_SUB, sizeof(TOPIC_SUB),
    "/oneM2M/resp/antares-cse/%s/json", ANTARES_ACCESS_KEY);
  snprintf(TOPIC_STATUS, sizeof(TOPIC_STATUS),
    "/oneM2M/req/%s/antares-cse/%s/status/json",
    ANTARES_ACCESS_KEY, ANTARES_APP);

  Serial.println("\n=== BAB 50 Program 3: MQTT Full IoT — Pub + Sub + LWT ===");
  Serial.printf("[INFO] Broker: %s:%d\n", ANTARES_HOST, ANTARES_PORT);
  Serial.printf("[INFO] App: %s | Device: %s\n", ANTARES_APP, ANTARES_DEVICE);

  oled.setCursor(0, 0);  oled.print("BAB 50 Full IoT");
  oled.setCursor(0, 12); oled.print("WiFi..."); oled.display();

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

  mqtt.setServer(ANTARES_HOST, ANTARES_PORT);
  mqtt.setBufferSize(512);
  mqtt.setCallback(onMqttMessage);

  // Baca sensor awal
  float t = dht.readTemperature(); float h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) { lastT = t; lastH = h; }

  mqttConnect();
  Serial.printf("[INFO] Publish tiap %lu detik | LWT aktif\n", PUB_INTERVAL / 1000UL);
  updateOLED();
}

void loop() {
  // WiFi check
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] ⚠️ Terputus! Reconnect...");
    WiFi.reconnect(); delay(5000); return;
  }

  // MQTT reconnect non-blocking
  static unsigned long tRecon = 0;
  if (!mqtt.connected() && millis() - tRecon >= 5000UL) {
    tRecon = millis(); mqttConnect();
  }

  // WAJIB: proses semua incoming messages & keep-alive
  mqtt.loop();

  // Publish tiap PUB_INTERVAL
  static unsigned long tPub = 0;
  static bool firstRun = true;
  if (firstRun || millis() - tPub >= PUB_INTERVAL) {
    firstRun = false; tPub = millis();

    float t = dht.readTemperature(); float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      lastT = t; lastH = h;
      Serial.printf("[DHT] ✅ Baca OK: %.1f°C | %.0f%%\n", lastT, lastH);
    } else {
      Serial.println("[DHT] ⚠️ Gagal baca! Cek IO27.");
    }

    if (mqtt.connected()) publishSensor();
    else Serial.println("[PUB] ⚠️ MQTT terputus, skip publish.");
  }

  // Heartbeat
  static unsigned long tHb = 0;
  if (millis() - tHb >= 30000UL) {
    tHb = millis();
    Serial.printf("[HB] %.1fC %.0f%% | Relay:%s | Pub:%u | Heap:%u B | Up:%lu s\n",
      isnan(lastT) ? 0.0f : lastT, isnan(lastH) ? 0.0f : lastH,
      relayState ? "ON" : "OFF", pubCount, ESP.getFreeHeap(), millis() / 1000UL);
  }
}
```

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 50 Program 3: MQTT Full IoT — Pub + Sub + LWT ===
[INFO] Broker: platform.antares.id:1338
[WiFi] ✅ Terhubung! IP: 192.168.1.105
[MQTT] Menghubungkan... (ID: BluinoKit-A1B2C3)
[MQTT] ✅ Terhubung! LWT aktif di: /oneM2M/req/.../status/json
[MQTT] Publish ONLINE → topik status
[MQTT] ✅ Subscribe: /oneM2M/resp/antares-cse/.../json
[DHT] ✅ Baca OK: 29.1°C | 65%
[INFO] Publish tiap 15 detik | LWT aktif
[MQTT] Publish #1 → data sensor
[MQTT] ✅ Publish berhasil! Heap: 193200 B
[MQTT] Pesan masuk: {"m2m:rsp":{"pc":{"m2m:cin":{"con":"{\"relay\":\"ON\"}"}},...}}
[RELAY] ✅ Relay → ON
[HB] 29.1C 65% | Relay:ON | Pub:2 | Heap:193200 B | Up:31 s
```

---

## 50.8 Troubleshooting MQTT

| Gejala | Penyebab | Solusi |
|--------|----------|--------|
| `rc=-4` (timeout) | Alamat broker atau port salah | Cek `ANTARES_HOST` dan `ANTARES_PORT=1338` |
| `rc=-2` (connect failed) | Client ID sudah dipakai device lain | Gunakan ID unik dari MAC address |
| `rc=4` (bad credentials) | Access Key salah | Salin ulang Access Key dari profil Antares |
| Publish berhasil tapi data tidak muncul di Dashboard | Topik salah, atau payload terpotong | Cek format `TOPIC_PUB` dan pastikan `setBufferSize(512)` |
| Subscribe tidak menerima pesan | `setCallback()` dipanggil setelah `connect()` | Panggil `setCallback()` SEBELUM `connect()` |
| MQTT disconnect terus setiap ~15 detik | `mqtt.loop()` tidak dipanggil di `loop()` | Pastikan `mqtt.loop()` ada di setiap iterasi |
| Payload terpotong / JSON tidak valid | Buffer terlalu kecil | Tambah `mqtt.setBufferSize(512)` atau lebih |

> ⚠️ **POLA WAJIB — Urutan inisiasi yang benar:**
> ```cpp
> mqtt.setServer(host, port);   // 1. Set broker
> mqtt.setBufferSize(512);      // 2. Set buffer (WAJIB sebelum connect!)
> mqtt.setCallback(fn);         // 3. Set callback (WAJIB sebelum connect!)
> mqtt.connect(...);            // 4. Connect
> mqtt.subscribe(topic);        // 5. Subscribe (setelah connect berhasil)
> // Di loop():
> mqtt.loop();                  // 6. WAJIB setiap iterasi!
> ```

---

## 50.9 Perbandingan BAB 48–50

| Aspek | BAB 48 (HTTP) | BAB 49 (HTTPS) | BAB 50 (MQTT) |
|-------|--------------|----------------|---------------|
| **Protokol** | HTTP plaintext | HTTP + TLS | MQTT Pub/Sub |
| **Koneksi** | Per-request | Per-request | Persisten |
| **Real-time** | ❌ Polling | ❌ Polling | ✅ Push instan |
| **Enkripsi** | ❌ | ✅ | ❌ (port 1338) |
| **Library** | HTTPClient | HTTPClient + WiFiClientSecure | PubSubClient |
| **Kompleksitas** | Rendah | Sedang | Sedang |
| **Cocok untuk** | REST API, upload berkala | API aman/produksi | Kontrol IoT real-time |

---

## 50.10 Ringkasan

```
RINGKASAN BAB 50 — MQTT ESP32

INISIALISASI (urutan WAJIB):
  mqtt.setServer(host, port)      ← Set broker
  mqtt.setBufferSize(512)         ← WAJIB: default 256 terlalu kecil
  mqtt.setCallback(fn)            ← WAJIB sebelum connect (untuk Sub)
  mqtt.connect(id, user, pass)    ← Connect ke broker
  mqtt.subscribe(topic)           ← Subscribe (setelah connect berhasil)

DI DALAM loop():
  mqtt.loop()                     ← WAJIB di setiap iterasi!

PUBLISH:
  mqtt.publish(topic, payload)
  mqtt.publish(topic, buf, len, retained)

SUBSCRIBE (melalui callback):
  void fn(char* topic, byte* payload, unsigned int length) {
    // WAJIB: payload tidak null-terminated!
    char buf[512]; memcpy(buf, payload, length); buf[length] = '\0';
    // ... proses pesan
  }

LWT (Last Will & Testament):
  mqtt.connect(id, user, pass,
    willTopic, willQos, willRetain, "OFFLINE")

FORMAT PAYLOAD ANTARES (oneM2M):
  {"m2m:cin":{"con":"{\"temp\":29.1,\"humidity\":65}"}}

ANTIPATTERN:
  ❌ Lupa mqtt.loop()              → koneksi timeout setiap ~15 detik
  ❌ Lupa setBufferSize(512)       → payload terpotong diam-diam
  ❌ setCallback() setelah connect → callback tidak bekerja
  ❌ Client ID duplikat            → saling kick antar device
```

---

## 50.11 Latihan

### Latihan Dasar
1. Upload **Program 1**, buka Dashboard Antares → verifikasi data suhu & kelembaban muncul tiap 15 detik.
2. Upload **Program 2**, kirim perintah `relay:ON` dari Dashboard → amati Relay IO4 aktif.
3. Ubah `PUB_INTERVAL` di Program 1 menjadi 5 detik. Amati apakah Antares menerima semua data.

### Latihan Menengah
4. Modifikasi **Program 1**: tambahkan field `"rssi": WiFi.RSSI()` ke payload JSON.
5. Modifikasi **Program 2**: tambahkan perintah baru `"relay":"TOGGLE"` yang membalik status relay.
6. Jalankan **Program 3**, matikan ESP32 mendadak (cabut USB). Amati apakah status "OFFLINE" muncul di Dashboard Antares.

### Tantangan Lanjutan
7. **Multi-Device:** Jalankan Program 3 di dua ESP32 dengan `ANTARES_DEVICE` berbeda. Verifikasi keduanya publish ke topik masing-masing tanpa interferensi.
8. **Threshold Alert:** Modifikasi Program 3 agar publish ke topik `alerts` jika suhu > 35°C.
9. **MQTT over TLS:** Ganti port ke `8883` dengan `WiFiClientSecure` + `setInsecure()` untuk koneksi terenkripsi ke Antares.

---

## 50.12 Preview: Apa Selanjutnya? (→ BAB 51)

```
BAB 50 (MQTT) — Protokol push real-time, cocok untuk IoT:
  ESP32 ── publish/subscribe ──► Antares Broker

BAB 51 (CoAP) — Protokol ringan berbasis UDP, cocok untuk
  perangkat dengan resource sangat terbatas:
  ESP32 ──── UDP Request ──────► CoAP Server
  ESP32 ◄─── UDP Response ──────

Perbedaan utama: MQTT persistent TCP, CoAP stateless UDP.
CoAP sangat efisien untuk sensor baterai (low-power devices).
```

---

## 50.13 Referensi

| Sumber | Link |
|--------|------|
| Antares MQTT Documentation | https://antares.id/id/docs.html |
| PubSubClient Library | https://github.com/knolleary/pubsubclient |
| MQTT Specification v3.1.1 | https://mqtt.org/mqtt-specification/ |
| ArduinoJson Documentation | https://arduinojson.org |
| MQTT Explorer (tool debug) | https://mqtt-explorer.com |

---

## ☕ Dukung Proyek Ini

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

---

*Sebelumnya → [BAB 49: HTTPS & Keamanan](../BAB-49/README.md)*

*Selanjutnya → [BAB 51: CoAP](../BAB-51/README.md)*
