# 🌐 BAB 56 — Protokol Modern: Matter & Thread

> ✅ **Prasyarat:** Selesaikan **BAB 55 (ESP-MESH)**. Kamu harus sudah memahami konsep mesh networking dan komunikasi antar node IoT.

> 📖 **Glosarium Singkatan BAB ini:**
> | Singkatan | Kepanjangan |
> |-----------|-------------|
> | **Matter** | Standar IoT open-source dari CSA — interoperabel lintas brand (Apple/Google/Amazon) |
> | **Thread** | Protokol mesh IPv6 berbasis IEEE 802.15.4 untuk perangkat IoT berdaya rendah |
> | **CSA** | *Connectivity Standards Alliance* — konsorsium yang mengembangkan Matter |
> | **Node** | Satu perangkat fisik ESP32 dalam jaringan Matter |
> | **Endpoint** | Perangkat logis dalam sebuah Node (contoh: 1 Node bisa punya 3 Endpoint) |
> | **Cluster** | Kelompok fungsi dalam Endpoint (OnOff, LevelControl, Thermostat, dll.) |
> | **Attribute** | Nilai data dalam Cluster (contoh: OnOff Cluster punya Attribute `OnOff=true`) |
> | **Command** | Aksi yang bisa dipicu di Cluster (contoh: `Toggle`, `MoveToLevel`) |
> | **Fabric** | Jaringan Matter yang terdiri dari satu atau lebih Node |
> | **Commissioner** | Aplikasi yang onboarding perangkat ke Fabric (Google Home, Apple Home) |
> | **PASE** | *Passcode Authenticated Session Establishment* — sesi awal saat commissioning |
> | **CASE** | *Certificate Authenticated Session Establishment* — sesi operasional Matter |
> | **Border Router** | Node yang menghubungkan jaringan Thread ke WiFi/Ethernet/Internet |
> | **OpenThread** | Implementasi open-source protokol Thread (digunakan di esp-matter SDK) |

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami arsitektur Matter: Node, Endpoint, Cluster, Attribute, dan Command.
- Memahami posisi Thread sebagai transport layer mesh IPv6 di bawah Matter.
- Mengetahui perbedaan hardware ESP32 yang mendukung Matter/Thread.
- Mensimulasikan Matter Data Model di ESP32 Classic via HTTP REST API.
- Memahami alur commissioning perangkat Matter ke ekosistem smart home.

---

## 56.1 Apa itu Matter?

```
SEBELUM MATTER (Problem):
  Perangkat Xiaomi ──► Hanya kompatibel dengan MI Home
  Perangkat Philips ──► Hanya kompatibel dengan Philips Hue app
  Perangkat Apple  ──► Hanya kompatibel dengan HomeKit
  (Ekosistem TERFRAGMENTASI — tidak bisa saling bicara!)

SETELAH MATTER (Solusi):
  Semua Perangkat Matter ──► Apple Home ✅
                          ──► Google Home ✅
                          ──► Amazon Alexa ✅
                          ──► Samsung SmartThings ✅
  (SATU standar, semua platform kompatibel!)

Matter adalah:
  ► Standar IoT open-source berbasis IP (IPv4 & IPv6)
  ► Dikembangkan CSA (beranggotakan Apple, Google, Amazon, Samsung, dll.)
  ► Berjalan di atas transport: WiFi, Thread, atau Ethernet
  ► Menggunakan model data yang terstandarisasi (Cluster Specification)
  ► End-to-end terenkripsi (TLS) sejak awal commissioning
  ► Tidak memerlukan cloud untuk operasi lokal!
```

---

## 56.2 Apa itu Thread?

```
Thread vs WiFi vs Bluetooth:

  WiFi:    2.4/5 GHz, boros daya, cocok untuk data besar
  BLE:     2.4 GHz, hemat daya, range pendek, tidak mesh native
  Thread:  2.4 GHz (IEEE 802.15.4), ultra-hemat daya, mesh native!

Thread adalah:
  ► Protokol mesh IPv6 untuk IoT (bukan WiFi!)
  ► Menggunakan radio IEEE 802.15.4 (sama seperti Zigbee)
  ► Setiap node bisa jadi router (self-healing!)
  ► Sangat hemat daya → cocok untuk sensor baterai bertahun-tahun
  ► Butuh Border Router untuk terhubung ke jaringan WiFi/Internet

Hubungan Matter & Thread:
  Matter  = Protokol aplikasi (WHAT yang dikomunikasikan)
  Thread  = Transport layer (HOW data dikirim)
  Matter over Thread = Kombinasi ideal untuk smart home masa depan!

  Matter over WiFi   → Cocok untuk ESP32 Classic/S3/C6
  Matter over Thread → Butuh radio 802.15.4 (ESP32-H2, ESP32-C6)
```

---

## 56.3 Arsitektur Matter — Node, Endpoint, Cluster, Attribute

```
HIERARKI MATTER DATA MODEL:

  NODE (ESP32 fisik)
  ├── Endpoint 0: Root (Wajib — berisi info Node)
  ├── Endpoint 1: Lampu (On/Off Light)
  │   └── Cluster: OnOff
  │       ├── Attribute: OnOff (bool)        ← baca/tulis
  │       ├── Attribute: StartUpOnOff (enum) ← baca/tulis
  │       └── Command: Toggle                ← aksi
  ├── Endpoint 2: Sensor Suhu
  │   └── Cluster: Temperature Measurement
  │       ├── Attribute: MeasuredValue (int16, unit 0.01°C)
  │       ├── Attribute: MinMeasuredValue
  │       └── Attribute: MaxMeasuredValue
  └── Endpoint 3: Relay (Generic Switch)
      └── Cluster: Switch
          └── Command: InitialPress, ShortRelease, etc.

ALUR COMMISSIONING (Onboarding Perangkat Baru):
  1. ESP32 broadcasting BLE advertisement → "Saya Matter device baru!"
  2. Commissioner (Google Home app) scan QR code → dapatkan PIN
  3. PASE session → verifikasi PIN → ESP32 dapat sertifikat digital
  4. ESP32 masuk ke Fabric → siap dikendalikan via Matter
  5. CASE session → komunikasi terenkripsi untuk operasi sehari-hari

FORMAT PAIRING CODE MATTER:
  QR Code: MT:YNJV70000K64G00 (encode info commissioning)
  Manual: XXXX-XXX-XXXX (11 digit PIN)
```

---

## 56.4 Kompatibilitas Hardware ESP32

> ⚠️ **PENTING DIBACA SEBELUM MULAI!** Tidak semua ESP32 mendukung Matter/Thread!

| Chip | WiFi | Bluetooth | 802.15.4 (Thread) | Matter over WiFi | Matter over Thread |
|------|------|-----------|-------------------|-----------------|-------------------|
| **ESP32 Classic** | ✅ | BT+BLE | ❌ | ✅ (dengan esp-matter) | ❌ |
| **ESP32-S3** | ✅ | BLE | ❌ | ✅ | ❌ |
| **ESP32-C3** | ✅ | BLE | ❌ | ✅ | ❌ |
| **ESP32-C6** | WiFi 6 | BLE | **✅** | ✅ | ✅ |
| **ESP32-H2** | ❌ | BLE | **✅** | ❌ | ✅ |

> 💡 **Program di BAB ini:**
> - **Program 1 (Matter Simulator)** → Berjalan di **ESP32 Classic** (simulasi conceptual)
> - **Program 2 (Matter Light)** → Butuh **ESP32-C6 atau ESP32-H2** + esp-matter SDK
> - **Program 3 (Thread Router)** → Butuh **ESP32-H2** + OpenThread API

---

## 56.5 Setup & Development Environment

### Untuk Program 1 (ESP32 Classic — Arduino IDE)
Tidak ada library tambahan yang dibutuhkan — menggunakan `WiFi.h`, `WebServer.h`, dan `ArduinoJson` (sudah terinstall sejak BAB 55).

### Untuk Program 2 & 3 (ESP32-H2/C6 — esp-matter SDK)
> ⚠️ **Program 2 & 3 membutuhkan esp-matter SDK yang TIDAK bisa diinstall via Library Manager biasa!**

```
CARA INSTALL esp-matter SDK (ringkasan):
  1. Install ESP-IDF v5.1.2+: https://docs.espressif.com/projects/esp-idf/
  2. Clone esp-matter: git clone --recursive https://github.com/espressif/esp-matter
  3. cd esp-matter && . ./install.sh
  4. source ./export.sh
  5. Build project: idf.py set-target esp32h2 && idf.py build && idf.py flash

Atau gunakan ESP-IDF extension di VS Code:
  1. Install "ESP-IDF" extension di VS Code
  2. Ikuti panduan setup di: https://github.com/espressif/vscode-esp-idf-extension

Versi yang direkomendasikan:
  esp-idf    : v5.1.2 atau lebih baru
  esp-matter : v1.3 atau lebih baru
  Target     : ESP32-H2 atau ESP32-C6
```

---

## 56.6 Program 1: Matter Data Model Simulator (ESP32 Classic)

**Topologi:** ESP32 Classic terhubung ke WiFi dan menjalankan HTTP server yang mengekspos API berformat Matter Data Model. Mahasiswa dapat berinteraksi via browser dan memahami konsep Endpoint, Cluster, dan Attribute secara hands-on.

> ✅ **Berjalan di ESP32 Classic** — tidak butuh hardware khusus!

### Tujuan Pembelajaran
- Memahami hierarki Matter: Node → Endpoint → Cluster → Attribute
- Berinteraksi dengan "Matter device" via REST API dari browser
- Melihat format data JSON yang menyerupai Matter JSON representasi

### Hardware
- ESP32 Classic (bawaan kit)
- DHT11 (IO27) — mewakili Temperature/Humidity Cluster
- Relay (IO4) — mewakili OnOff Cluster

```cpp
/*
 * BAB 56 - Program 1: Matter Data Model Simulator
 *
 * ESP32 Classic yang mensimulasikan Matter Device via HTTP REST API
 *
 * Matter Data Model yang disimulasikan:
 *   Node: ESP32 (NodeId = MAC address)
 *   ├── Endpoint 0: Root (info node)
 *   ├── Endpoint 1: On/Off Light (OnOff Cluster) → Relay IO4
 *   └── Endpoint 2: Temp/Humidity Sensor → DHT11 IO27
 *
 * Cara uji:
 *   1. Ganti WIFI_SSID dan WIFI_PASS
 *   2. Upload ke ESP32 → buka Serial Monitor → catat IP address
 *   3. Buka browser: http://<IP_ESP32>
 *   4. Eksplorasi API: http://<IP_ESP32>/matter
 *
 * Hardware: DHT11 → IO27 | Relay → IO4
 * Library : WiFi.h, WebServer.h, ArduinoJson (sudah ada dari BAB 55)
 */

#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <ArduinoJson.h>

// ── Konfigurasi WiFi — GANTI dengan jaringan kamu! ────────────────────
const char* WIFI_SSID = "SSID_KAMU";
const char* WIFI_PASS = "PASSWORD_WIFI";

// ── Hardware ──────────────────────────────────────────────────────────
#define DHT_PIN   27
#define RELAY_PIN  4
DHT dht(DHT_PIN, DHT11);

WebServer server(80);

// ── Matter Data Model (disederhanakan) ────────────────────────────────
struct {
  char   nodeId[18];       // MAC address sebagai NodeID
  bool   ep1_onOff;        // Endpoint 1: OnOff Cluster → Attribute OnOff
  float  ep2_temp;         // Endpoint 2: Temperature Measurement Cluster
  float  ep2_hum;          // Endpoint 2: Relative Humidity Cluster
} matterNode;

uint32_t cmdCount   = 0;
uint32_t readCount  = 0;

// ── Baca sensor DHT11 ──────────────────────────────────────────────────
void readSensor() {
  float t = dht.readTemperature(), h = dht.readHumidity();
  if (!isnan(t)) matterNode.ep2_temp = t;
  if (!isnan(h)) matterNode.ep2_hum  = h;
}

// ── Set relay & sync dengan Matter state ──────────────────────────────
void setOnOff(bool on) {
  matterNode.ep1_onOff = on;
  digitalWrite(RELAY_PIN, on ? HIGH : LOW);
  cmdCount++;
  Serial.printf("[MATTER] Cmd #%u: Endpoint1/OnOff → %s\n", cmdCount, on?"ON":"OFF");
}

// ── Handler: GET / (dashboard HTML) ──────────────────────────────────
void handleDashboard() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width'>";
  html += "<title>Matter Simulator — Bluino IoT Kit</title>";
  html += "<style>*{box-sizing:border-box}body{font-family:monospace;background:#0d1117;color:#c9d1d9;padding:20px;max-width:600px;margin:auto}";
  html += "h1{color:#58a6ff}h2{color:#3fb950;border-bottom:1px solid #30363d;padding-bottom:4px}";
  html += ".card{background:#161b22;border:1px solid #30363d;border-radius:6px;padding:16px;margin:12px 0}";
  html += ".badge{display:inline-block;padding:2px 8px;border-radius:4px;font-size:12px}";
  html += ".on{background:#1a4731;color:#3fb950}.off{background:#2d1c1c;color:#f85149}";
  html += "button{background:#238636;color:#fff;border:none;padding:8px 20px;border-radius:6px;cursor:pointer;font-size:14px}";
  html += "button:hover{background:#2ea043}pre{background:#0d1117;padding:12px;border-radius:4px;overflow-x:auto;font-size:12px}</style></head><body>";
  html += "<h1>🔌 Matter Device Simulator</h1>";
  html += "<div class='card'><b>NodeID:</b> " + String(matterNode.nodeId) + "<br>";
  html += "<b>Vendor:</b> Bluino &nbsp;|&nbsp; <b>Product:</b> IoT Kit 2026</div>";

  html += "<h2>Endpoint 1 — On/Off Light (Relay)</h2><div class='card'>";
  html += "<b>Cluster:</b> OnOff &nbsp; <b>Attribute:</b> OnOff = ";
  html += matterNode.ep1_onOff ? "<span class='badge on'>TRUE (ON)</span>" : "<span class='badge off'>FALSE (OFF)</span>";
  html += "<br><br><button onclick=\"fetch('/matter/1/onoff',{method:'PUT',headers:{'Content-Type':'application/json'},body:JSON.stringify({toggle:true})}).then(()=>location.reload())\">⚡ TOGGLE</button></div>";

  html += "<h2>Endpoint 2 — Temperature & Humidity Sensor</h2><div class='card'>";
  html += "<b>Cluster:</b> TemperatureMeasurement<br>";
  html += "<b>MeasuredValue:</b> " + String((int)(matterNode.ep2_temp * 100)) + " (= " + String(matterNode.ep2_temp, 1) + "°C)<br>";
  html += "<b>Cluster:</b> RelativeHumidityMeasurement<br>";
  html += "<b>MeasuredValue:</b> " + String((int)(matterNode.ep2_hum * 100)) + " (= " + String(matterNode.ep2_hum, 0) + "%)";
  html += "</div>";

  html += "<h2>Matter REST API</h2><div class='card'><pre>";
  html += "GET  /matter            → Semua endpoint (Node info)\n";
  html += "GET  /matter/1/onoff    → Baca OnOff attribute\n";
  html += "PUT  /matter/1/onoff    → Tulis: {\"value\":true}\n";
  html += "PUT  /matter/1/onoff    → Toggle: {\"toggle\":true}\n";
  html += "GET  /matter/2/temp     → Baca suhu (MeasuredValue)\n";
  html += "GET  /matter/2/humidity → Baca kelembaban</pre></div>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

// ── Handler: GET /matter (semua endpoint) ─────────────────────────────
void handleMatterRoot() {
  readCount++;
  DynamicJsonDocument doc(512);
  doc["nodeId"]       = matterNode.nodeId;
  doc["vendorName"]   = "Bluino";
  doc["productName"]  = "IoT Kit 2026";
  doc["swVersion"]    = "1.0.0";
  doc["readCount"]    = readCount;

  JsonArray eps = doc.createNestedArray("endpoints");

  JsonObject ep0 = eps.createNestedObject();
  ep0["endpointId"] = 0; ep0["deviceType"] = "Root Node";

  JsonObject ep1 = eps.createNestedObject();
  ep1["endpointId"] = 1; ep1["deviceType"] = "On/Off Light";
  ep1["cluster"]    = "OnOff";
  ep1["OnOff"]      = matterNode.ep1_onOff;

  JsonObject ep2 = eps.createNestedObject();
  ep2["endpointId"]    = 2; ep2["deviceType"] = "Temperature Sensor";
  ep2["tempCelsius"]   = round(matterNode.ep2_temp * 10) / 10.0;
  ep2["humidityPct"]   = round(matterNode.ep2_hum);
  ep2["tempRaw"]       = (int)(matterNode.ep2_temp * 100); // Matter unit: 0.01°C

  String resp;
  serializeJsonPretty(doc, resp);
  server.send(200, "application/json", resp);
  Serial.printf("[HTTP] GET /matter → %u bytes\n", resp.length());
}

// ── Handler: GET /matter/1/onoff ──────────────────────────────────────
void handleReadOnOff() {
  DynamicJsonDocument doc(128);
  doc["endpointId"] = 1;
  doc["cluster"]    = "OnOff";
  doc["attribute"]  = "OnOff";
  doc["value"]      = matterNode.ep1_onOff;
  String resp; serializeJson(doc, resp);
  server.send(200, "application/json", resp);
}

// ── Handler: PUT /matter/1/onoff ──────────────────────────────────────
void handleWriteOnOff() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"Body kosong\"}");
    return;
  }
  DynamicJsonDocument req(64);
  if (deserializeJson(req, server.arg("plain")) != DeserializationError::Ok) {
    server.send(400, "application/json", "{\"error\":\"JSON tidak valid\"}");
    return;
  }
  bool newState;
  if (req.containsKey("toggle") && req["toggle"].as<bool>()) {
    newState = !matterNode.ep1_onOff;
  } else {
    newState = req["value"] | false;
  }
  setOnOff(newState);

  DynamicJsonDocument resp(64);
  resp["endpointId"] = 1;
  resp["attribute"]  = "OnOff";
  resp["value"]      = matterNode.ep1_onOff;
  String respStr; serializeJson(resp, respStr);
  server.send(200, "application/json", respStr);
}

// ── Handler: GET /matter/2/temp & /matter/2/humidity ──────────────────
void handleReadTemp() {
  DynamicJsonDocument doc(128);
  doc["endpointId"]   = 2;
  doc["cluster"]      = "TemperatureMeasurement";
  doc["attribute"]    = "MeasuredValue";
  doc["value"]        = (int)(matterNode.ep2_temp * 100); // Matter unit: 0.01°C
  doc["displayValue"] = String(matterNode.ep2_temp, 1) + "°C";
  String resp; serializeJson(doc, resp);
  server.send(200, "application/json", resp);
}

void handleReadHumidity() {
  DynamicJsonDocument doc(128);
  doc["endpointId"]   = 2;
  doc["cluster"]      = "RelativeHumidityMeasurement";
  doc["attribute"]    = "MeasuredValue";
  doc["value"]        = (int)(matterNode.ep2_hum * 100); // Matter unit: 0.01%
  doc["displayValue"] = String(matterNode.ep2_hum, 0) + "%";
  String resp; serializeJson(doc, resp);
  server.send(200, "application/json", resp);
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  dht.begin();
  Serial.println("\n=== BAB 56 Program 1: Matter Data Model Simulator ===");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("[WiFi] Menghubungkan ke '" + String(WIFI_SSID) + "'");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.printf("\n[WiFi] ✅ Terhubung! IP: %s\n", WiFi.localIP().toString().c_str());

  // Init Matter Node info
  String mac = WiFi.macAddress();
  mac.toCharArray(matterNode.nodeId, sizeof(matterNode.nodeId));
  matterNode.ep1_onOff = false;

  // Baca sensor awal
  readSensor();
  Serial.printf("[DHT] ✅ Awal: %.1f°C | %.0f%%\n", matterNode.ep2_temp, matterNode.ep2_hum);

  // Daftarkan semua route HTTP
  server.on("/",                  HTTP_GET,  handleDashboard);
  server.on("/matter",            HTTP_GET,  handleMatterRoot);
  server.on("/matter/1/onoff",    HTTP_GET,  handleReadOnOff);
  server.on("/matter/1/onoff",    HTTP_PUT,  handleWriteOnOff);
  server.on("/matter/2/temp",     HTTP_GET,  handleReadTemp);
  server.on("/matter/2/humidity", HTTP_GET,  handleReadHumidity);
  server.begin();

  Serial.println("[MATTER] ✅ Simulator aktif!");
  Serial.printf("[MATTER] Dashboard : http://%s\n", WiFi.localIP().toString().c_str());
  Serial.printf("[MATTER] Node Info : http://%s/matter\n", WiFi.localIP().toString().c_str());
  Serial.printf("[MATTER] OnOff API : http://%s/matter/1/onoff\n", WiFi.localIP().toString().c_str());
  Serial.printf("[MATTER] Temp API  : http://%s/matter/2/temp\n", WiFi.localIP().toString().c_str());
}

void loop() {
  server.handleClient();

  // Baca sensor tiap 5 detik
  static unsigned long tSensor = 0;
  if (millis() - tSensor >= 5000UL) {
    tSensor = millis();
    readSensor();
  }

  // Heartbeat
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] OnOff:%s | T:%.1f | H:%.0f | Cmd:%u | Heap:%u B\n",
      matterNode.ep1_onOff?"ON":"OFF", matterNode.ep2_temp,
      matterNode.ep2_hum, cmdCount, ESP.getFreeHeap());
  }
}
```

**Output Serial Monitor:**
```
=== BAB 56 Program 1: Matter Data Model Simulator ===
[WiFi] Menghubungkan ke 'MyHomeWiFi'..........
[WiFi] ✅ Terhubung! IP: 192.168.1.105
[DHT] ✅ Awal: 29.1°C | 65%
[MATTER] ✅ Simulator aktif!
[MATTER] Dashboard : http://192.168.1.105
[MATTER] Node Info : http://192.168.1.105/matter
[MATTER] OnOff API : http://192.168.1.105/matter/1/onoff
[MATTER] Temp API  : http://192.168.1.105/matter/2/temp
--- (buka browser, klik TOGGLE) ---
[MATTER] Cmd #1: Endpoint1/OnOff → ON
[MATTER] Cmd #2: Endpoint1/OnOff → OFF
[HTTP] GET /matter → 348 bytes
[HB] OnOff:OFF | T:29.3 | H:64 | Cmd:2 | Heap:216000 B
```

> 💡 **Pengamatan Kunci:**
> - Nilai temperature di Matter adalah `2910` (= 29.10°C × 100) — Matter menyimpan suhu dalam satuan **0.01°C**
> - Setiap aksi (toggle) melalui endpoint yang terstruktur: `/matter/{endpointId}/{clusterId}`
> - Inilah pola yang sama yang digunakan oleh perangkat Matter nyata!

---

## 56.7 Program 2: Matter Light Bulb — Real Matter dengan esp-matter SDK

> ⚠️ **Hardware yang dibutuhkan: ESP32-H2 atau ESP32-C6**
> ⚠️ **Development Environment: ESP-IDF v5.1.2+ dengan esp-matter component**
> ℹ️ Lihat bagian **56.5** untuk cara install esp-matter SDK.

**Topologi:** ESP32-H2/C6 menjalankan Matter device nyata yang bisa di-commission ke Google Home, Apple Home, atau Amazon Alexa. Setelah terhubung, relay bisa dikontrol langsung dari aplikasi smart home tanpa server cloud!

### Tujuan Pembelajaran
- Memahami alur commissioning Matter (QR Code → PASE → CASE → Operational)
- Melihat cara esp-matter SDK mengimplementasikan OnOff Cluster
- Memahami perbedaan antara simulasi (P1) dan implementasi Matter nyata (P2)

### Hardware
- **ESP32-H2 atau ESP32-C6** (wajib — ESP32 Classic tidak mendukung!)
- Relay atau LED (IO4)
- Smartphone dengan Google Home / Apple Home (iOS 16+) / Amazon Alexa

```cpp
/*
 * BAB 56 - Program 2: Matter Light Bulb — REAL MATTER
 *
 * PLATFORM: ESP32-H2 atau ESP32-C6 SAJA!
 * BUKAN untuk ESP32 Classic!
 *
 * Development Environment:
 *   - ESP-IDF v5.1.2+
 *   - esp-matter v1.3+
 *   - Build: idf.py set-target esp32h2 && idf.py build && idf.py flash
 *
 * Setelah flash:
 *   1. Buka Google Home / Apple Home
 *   2. Tambah perangkat baru
 *   3. Scan QR Code yang tampil di Serial Monitor
 *   4. Ikuti alur commissioning
 *   5. Relay bisa dikontrol dari aplikasi!
 *
 * Hardware: Relay/LED → IO4
 */

// ── Includes esp-matter SDK ────────────────────────────────────────────
#include <esp_matter.h>
#include <esp_matter_cluster.h>
#include <esp_matter_endpoint.h>
#include <esp_log.h>
#include <driver/gpio.h>

static const char *TAG = "BAB56_P2";

// ── Konfigurasi Pin ────────────────────────────────────────────────────
#define RELAY_GPIO  GPIO_NUM_4

// ── Matter Node & Endpoint handle ─────────────────────────────────────
static esp_matter::node::config_t  node_cfg;
static esp_matter::endpoint::on_off_light::config_t light_cfg;
static uint16_t light_endpoint_id = 0;

// ── Callback: dipanggil saat attribute berubah (dari Commissioner) ─────
static esp_err_t app_attribute_update_cb(
    esp_matter::attribute::callback_type_t type,
    uint16_t endpoint_id, uint32_t cluster_id,
    uint32_t attribute_id, esp_matter_attr_val_t *val, void *priv_data)
{
  // Filter: hanya proses OnOff cluster (0x0006), attribute OnOff (0x0000)
  if (endpoint_id == light_endpoint_id
      && cluster_id    == chip::app::Clusters::OnOff::Id
      && attribute_id  == chip::app::Clusters::OnOff::Attributes::OnOff::Id
      && type          == esp_matter::attribute::PRE_UPDATE)
  {
    bool new_state = val->val.b;
    gpio_set_level(RELAY_GPIO, new_state ? 1 : 0);
    ESP_LOGI(TAG, "OnOff → %s", new_state ? "ON" : "OFF");
  }
  return ESP_OK;
}

// ── Callback: device events (commissioning, connectivity) ─────────────
static void app_event_cb(
    const chip::DeviceLayer::ChipDeviceEvent *event, intptr_t arg)
{
  switch (event->Type) {
    case chip::DeviceLayer::DeviceEventType::kCommissioningComplete:
      ESP_LOGI(TAG, "✅ Commissioning selesai! Device tergabung ke Fabric.");
      break;
    case chip::DeviceLayer::DeviceEventType::kFabricRemoved:
      ESP_LOGW(TAG, "⚠️ Device dilepas dari Fabric.");
      break;
    default: break;
  }
}

extern "C" void app_main() {
  // ── Init GPIO ──────────────────────────────────────────────────────
  gpio_config_t io_conf = {
    .pin_bit_mask = (1ULL << RELAY_GPIO),
    .mode         = GPIO_MODE_OUTPUT,
  };
  gpio_config(&io_conf);
  gpio_set_level(RELAY_GPIO, 0);  // Mulai OFF

  ESP_LOGI(TAG, "\n=== BAB 56 Program 2: Matter Light Bulb ===");

  // ── Buat Matter Node ───────────────────────────────────────────────
  esp_matter::node_t *node = esp_matter::node::create(
    &node_cfg, app_attribute_update_cb, app_event_cb);

  if (!node) {
    ESP_LOGE(TAG, "Gagal membuat Matter node!");
    return;
  }

  // ── Buat Endpoint: On/Off Light ────────────────────────────────────
  light_cfg.on_off.on_off            = false; // Default: OFF
  light_cfg.on_off.lighting.start_up_on_off = 0;

  esp_matter::endpoint_t *endpoint = esp_matter::endpoint::on_off_light::create(
    node, &light_cfg, ENDPOINT_FLAG_NONE, NULL);

  light_endpoint_id = esp_matter::endpoint::get_id(endpoint);
  ESP_LOGI(TAG, "OnOff Light Endpoint ID: %u", light_endpoint_id);

  // ── Start Matter Stack ─────────────────────────────────────────────
  esp_matter::start(app_event_cb);

  // ── Tampilkan QR Code Commissioning di Serial ──────────────────────
  // (esp-matter otomatis print QR code saat startup)
  ESP_LOGI(TAG, "✅ Matter device aktif! Scan QR Code di atas untuk commissioning.");
  ESP_LOGI(TAG, "Buka Google Home / Apple Home → Tambah Perangkat → Scan QR Code");
}
```

**Output Serial Monitor saat startup:**
```
=== BAB 56 Program 2: Matter Light Bulb ===
I (1234) chip[DL]: Device Configuration:
I (1234) chip[DL]:   Serial Number: TEST_SN
I (1234) chip[DL]:   Vendor Id: 65521 (0xFFF1)
I (1234) chip[DL]:   Product Id: 32768 (0x8000)
I (1234) chip[DL]:   Product Name: ESP Matter Light
I (1234) chip[SVR]: SetupQRCode:  [MT:YNJV70000K64G00]
I (1234) chip[SVR]: Copy/paste the code below to a commissioning app:
I (1234) chip[SVR]: Manual pairing code: [749-428-4481]
OnOff Light Endpoint ID: 1
✅ Matter device aktif! Scan QR Code di atas untuk commissioning.
--- (setelah commissioning via Google Home) ---
✅ Commissioning selesai! Device tergabung ke Fabric.
--- (saat di-toggle dari Google Home) ---
OnOff → ON
OnOff → OFF
```

> 💡 **Catatan Penting esp-matter:**
> - QR Code otomatis digenerate dan ditampilkan di Serial Monitor saat pertama boot
> - Matter device bekerja **secara lokal** — tidak butuh cloud Espressif!
> - Setelah commissioning, relay bisa dikontrol via Google Home / Apple Home / Siri
> - Data tersimpan di NVS flash — device ingat status komisioning setelah reboot

---

## 56.8 Program 3: Thread Router Basic (ESP32-H2 + OpenThread)

> ⚠️ **Hardware yang dibutuhkan: ESP32-H2 SAJA**
> ⚠️ **Development Environment: ESP-IDF v5.1.2+ dengan OpenThread component**

**Topologi:** ESP32-H2 menjalankan OpenThread sebagai **Thread Router** — node yang mampu meneruskan paket antar node Thread dan bertindak sebagai bagian dari jaringan mesh IPv6 Thread.

### Tujuan Pembelajaran
- Memahami cara kerja Thread mesh (Leader, Router, End Device)
- Membentuk Thread network dan melihat network data
- Mengirim pesan UDP antar node Thread (peer-to-peer via IPv6)

### Hardware
- **ESP32-H2** (wajib — radio IEEE 802.15.4 diperlukan)
- Minimal 2 ESP32-H2 untuk membentuk Thread network
- Opsional: Border Router (Raspberry Pi + OpenThread Border Router) untuk koneksi ke internet

```cpp
/*
 * BAB 56 - Program 3: Thread Router Basic
 *
 * PLATFORM: ESP32-H2 SAJA!
 * ESP32-H2 memiliki radio IEEE 802.15.4 yang dibutuhkan Thread.
 *
 * Development Environment:
 *   - ESP-IDF v5.1.2+
 *   - Aktifkan komponen OpenThread di sdkconfig
 *   - Build: idf.py set-target esp32h2 && idf.py build && idf.py flash
 *
 * Fungsi:
 *   ► Inisiasi OpenThread stack
 *   ► Bergabung ke Thread network (atau buat network baru jika belum ada)
 *   ► Tampilkan Thread network info: Leader, Router Table, IPv6 addresses
 *   ► Kirim/terima UDP message antar node Thread
 *   ► Heartbeat tampilkan status Thread tiap 30 detik
 *
 * Hardware: ESP32-H2 (tidak ada pin tambahan yang dibutuhkan)
 */

// ── Includes OpenThread (ESP-IDF) ─────────────────────────────────────
#include "esp_openthread.h"
#include "esp_openthread_netif_glue.h"
#include "esp_openthread_types.h"
#include "openthread/cli.h"
#include "openthread/dataset.h"
#include "openthread/dataset_ftd.h"
#include "openthread/instance.h"
#include "openthread/logging.h"
#include "openthread/thread.h"
#include "openthread/thread_ftd.h"
#include "openthread/udp.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "BAB56_P3";

// ── Thread Network Dataset (gunakan nilai unik untuk jaringanmu!) ──────
// Hasilkan dataset baru: idf.py monitor → ketik 'dataset init new' → 'dataset'
#define THREAD_NETWORK_NAME   "BluinoMesh"
#define THREAD_CHANNEL        15           // Channel 802.15.4 (11-26)
#define THREAD_PANID          0x1234       // PAN ID (hex, unik per jaringan)

// ── UDP Port untuk komunikasi antar node ──────────────────────────────
#define THREAD_UDP_PORT       5683

static otUdpSocket  sUdpSocket;
static otInstance  *sInstance = NULL;
uint32_t txCount = 0, rxCount = 0;

// ── Callback: terima pesan UDP dari node lain ──────────────────────────
void udpReceiveCallback(void *aContext, otMessage *aMessage,
                        const otMessageInfo *aMessageInfo) {
  char buf[128] = {0};
  int  len = otMessageRead(aMessage, otMessageGetOffset(aMessage), buf, sizeof(buf) - 1);
  buf[len] = '\0';
  rxCount++;

  char srcAddr[OT_IP6_ADDRESS_STRING_SIZE];
  otIp6AddressToString(&aMessageInfo->mPeerAddr, srcAddr, sizeof(srcAddr));
  ESP_LOGI(TAG, "[UDP] Terima #%u dari %s: '%s'", rxCount, srcAddr, buf);
}

// ── Fungsi: kirim UDP broadcast ke semua node Thread ──────────────────
void sendThreadBroadcast(const char *message) {
  otMessageInfo msgInfo;
  memset(&msgInfo, 0, sizeof(msgInfo));

  // Alamat multicast Thread semua node: ff03::1
  otIp6AddressFromString("ff03::1", &msgInfo.mPeerAddr);
  msgInfo.mPeerPort    = THREAD_UDP_PORT;
  msgInfo.mSockPort    = THREAD_UDP_PORT;

  otMessage *msg = otUdpNewMessage(sInstance, NULL);
  if (!msg) { ESP_LOGE(TAG, "Gagal alokasi UDP message!"); return; }

  otMessageAppend(msg, message, strlen(message));
  otError err = otUdpSend(sInstance, &sUdpSocket, msg, &msgInfo);

  if (err == OT_ERROR_NONE) {
    txCount++;
    ESP_LOGI(TAG, "[UDP] Broadcast #%u: '%s'", txCount, message);
  } else {
    otMessageFree(msg);
    ESP_LOGE(TAG, "[UDP] Gagal kirim: %d", err);
  }
}

// ── Fungsi: tampilkan info Thread network ─────────────────────────────
void printThreadStatus() {
  if (!sInstance) return;

  otDeviceRole role = otThreadGetDeviceRole(sInstance);
  const char *roleStr;
  switch (role) {
    case OT_DEVICE_ROLE_LEADER:     roleStr = "Leader";     break;
    case OT_DEVICE_ROLE_ROUTER:     roleStr = "Router";     break;
    case OT_DEVICE_ROLE_CHILD:      roleStr = "Child (End Device)"; break;
    case OT_DEVICE_ROLE_DETACHED:   roleStr = "Detached";   break;
    default:                         roleStr = "Disabled";   break;
  }

  uint16_t rloc16 = otThreadGetRloc16(sInstance);
  uint8_t  channel = otLinkGetChannel(sInstance);

  ESP_LOGI(TAG, "[THREAD] Role:%s | RLOC16:0x%04X | Channel:%u | Tx:%u Rx:%u",
    roleStr, rloc16, channel, txCount, rxCount);

  // Tampilkan alamat IPv6 node ini
  const otNetifAddress *addr = otIp6GetUnicastAddresses(sInstance);
  int addrIdx = 0;
  while (addr && addrIdx < 3) {
    char addrStr[OT_IP6_ADDRESS_STRING_SIZE];
    otIp6AddressToString(&addr->mAddress, addrStr, sizeof(addrStr));
    ESP_LOGI(TAG, "[THREAD] IPv6[%d]: %s", addrIdx++, addrStr);
    addr = addr->mNext;
  }
}

// ── Init Thread dataset & start stack ─────────────────────────────────
void initThreadDataset() {
  otOperationalDataset dataset;
  memset(&dataset, 0, sizeof(dataset));

  // Cek apakah dataset sudah ada di NVS
  if (otDatasetIsCommissioned(sInstance)) {
    ESP_LOGI(TAG, "[THREAD] Dataset sudah ada, bergabung ke jaringan...");
  } else {
    // Buat dataset baru
    otDatasetCreateNewNetwork(sInstance, &dataset);
    // Set nama jaringan
    strncpy(dataset.mNetworkName.m8, THREAD_NETWORK_NAME,
      sizeof(dataset.mNetworkName.m8));
    dataset.mComponents.mIsNetworkNamePresent = true;
    // Set channel
    dataset.mChannel                          = THREAD_CHANNEL;
    dataset.mComponents.mIsChannelPresent     = true;
    // Set PAN ID
    dataset.mPanId                            = THREAD_PANID;
    dataset.mComponents.mIsPanIdPresent       = true;

    otDatasetSetActive(sInstance, &dataset);
    ESP_LOGI(TAG, "[THREAD] Dataset baru dibuat: '%s' ch%u PAN:0x%04X",
      THREAD_NETWORK_NAME, THREAD_CHANNEL, THREAD_PANID);
  }

  // Start Thread interface & jaringan
  otIp6SetEnabled(sInstance, true);
  otThreadSetEnabled(sInstance, true);
  ESP_LOGI(TAG, "[THREAD] ✅ Stack Thread aktif!");
}

extern "C" void app_main() {
  ESP_LOGI(TAG, "\n=== BAB 56 Program 3: Thread Router Basic ===");

  // ── Init OpenThread platform ───────────────────────────────────────
  esp_openthread_platform_config_t config = {
    .radio_config = ESP_OPENTHREAD_DEFAULT_RADIO_CONFIG(),
    .host_config  = ESP_OPENTHREAD_DEFAULT_HOST_CONFIG(),
    .port_config  = ESP_OPENTHREAD_DEFAULT_PORT_CONFIG(),
  };
  esp_openthread_init(&config);

  sInstance = esp_openthread_get_instance();

  // ── Buka UDP socket ────────────────────────────────────────────────
  otSockAddr sockAddr;
  memset(&sockAddr, 0, sizeof(sockAddr));
  sockAddr.mPort = THREAD_UDP_PORT;
  otUdpOpen(sInstance, &sUdpSocket, udpReceiveCallback, NULL);
  otUdpBind(sInstance, &sUdpSocket, &sockAddr, OT_NETIF_THREAD);
  ESP_LOGI(TAG, "[UDP] Socket terbuka di port %u", THREAD_UDP_PORT);

  // ── Init Thread dataset & start network ───────────────────────────
  initThreadDataset();

  // ── Main loop ─────────────────────────────────────────────────────
  ESP_LOGI(TAG, "[THREAD] Menunggu join ke jaringan Thread...");
  unsigned long lastStatus  = 0;
  unsigned long lastBcast   = 0;

  while (true) {
    // Proses OpenThread events
    esp_openthread_process_next_event();

    uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);

    // Tampilkan status Thread tiap 30 detik
    if (now - lastStatus >= 30000) {
      lastStatus = now;
      printThreadStatus();
    }

    // Kirim broadcast ke semua node tiap 10 detik
    if (now - lastBcast >= 10000) {
      lastBcast = now;
      otDeviceRole role = otThreadGetDeviceRole(sInstance);
      if (role == OT_DEVICE_ROLE_LEADER || role == OT_DEVICE_ROLE_ROUTER) {
        char msg[64];
        snprintf(msg, sizeof(msg), "Hello from %04X! Up:%lu s",
          otThreadGetRloc16(sInstance), now / 1000);
        sendThreadBroadcast(msg);
      }
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
```

**Output Serial Monitor (2 ESP32-H2 aktif):**
```
=== BAB 56 Program 3: Thread Router Basic ===
[UDP] Socket terbuka di port 5683
[THREAD] Dataset baru dibuat: 'BluinoMesh' ch15 PAN:0x1234
[THREAD] ✅ Stack Thread aktif!
[THREAD] Menunggu join ke jaringan Thread...
--- (setelah berhasil join network) ---
[THREAD] Role:Leader | RLOC16:0x0400 | Channel:15 | Tx:0 Rx:0
[THREAD] IPv6[0]: fdde:ad00:beef::1
[THREAD] IPv6[1]: fe80::1234:5678:abcd:ef01
[UDP] Broadcast #1: 'Hello from 0400! Up:30 s'
[UDP] Terima #1 dari fe80::5678:1234:ef01:abcd: 'Hello from 3C00! Up:32 s'
[THREAD] Role:Router | RLOC16:0x0400 | Channel:15 | Tx:3 Rx:6
```

> 💡 **Peran Node dalam Thread Network:**
> - **Leader:** Node pertama yang membentuk jaringan, mengelola Router Table
> - **Router:** Node yang meneruskan paket (full routing capability)
> - **End Device (Child):** Node yang hanya komunikasi ke Parent-nya (cocok untuk baterai)
> - Peran ditentukan otomatis oleh OpenThread berdasarkan kondisi jaringan

---

## 56.9 Troubleshooting Matter & Thread

| Gejala | Penyebab | Solusi |
|--------|----------|--------|
| ESP32 Classic tidak compile Program 2/3 | ESP32 Classic tidak ada radio 802.15.4 | Gunakan **ESP32-H2 atau ESP32-C6** untuk Program 2 & 3 |
| QR Code tidak terbaca di Google Home | App Google Home belum update / Matter belum diaktifkan | Update Google Home ke versi terbaru; pastikan fitur Matter diaktifkan |
| Commissioning gagal di tengah jalan | Koneksi WiFi lemah saat proses PASE | Dekatkan ESP32 ke router WiFi; pastikan sinyal stabil |
| `esp_matter.h` not found | esp-matter SDK belum diinstall | Ikuti panduan install di bagian 56.5 (butuh ESP-IDF, bukan Arduino) |
| Thread node tidak bisa join network | `THREAD_CHANNEL` atau `THREAD_PANID` berbeda | Pastikan semua node menggunakan nilai `CHANNEL` dan `PANID` yang SAMA |
| `otThreadGetDeviceRole` selalu `Detached` | Belum ada node Leader di jaringan | Node pertama yang aktif otomatis menjadi Leader; tunggu 10-30 detik |
| UDP tidak terkirim via Thread | Node belum bergabung ke jaringan (role masih Detached) | Cek role dulu sebelum kirim; kirim hanya jika role = Leader atau Router |
| Matter device hilang dari Google Home setelah reboot | NVS flash terhapus saat re-flash firmware | Gunakan `idf.py flash` tanpa `--erase-flash`; atau commission ulang |
| Program 1 tidak bisa akses /matter dari HP | HP dan ESP32 di jaringan WiFi yang berbeda | Pastikan HP dan ESP32 terhubung ke **WiFi router yang sama** |

> ⚠️ **RINGKASAN KOMPATIBILITAS HARDWARE:**
> ```
> Program 1 (Matter Simulator):
>   ✅ ESP32 Classic   ✅ ESP32-S3   ✅ ESP32-C3   ✅ ESP32-C6   ✅ ESP32-H2
>
> Program 2 (Real Matter Light):
>   ❌ ESP32 Classic   ❌ ESP32-S3   ❌ ESP32-C3   ✅ ESP32-C6   ✅ ESP32-H2
>   (Butuh esp-matter SDK + ESP-IDF v5.1.2+)
>
> Program 3 (Thread Router):
>   ❌ ESP32 Classic   ❌ ESP32-S3   ❌ ESP32-C3   ✅ ESP32-C6   ✅ ESP32-H2
>   (Butuh radio IEEE 802.15.4 + ESP-IDF v5.1.2+)
> ```

---

## 56.10 Perbandingan Protokol IoT BAB 50-56

| Protokol | BAB | Open Standard | Cloud | Daya | Interoperable | Cocok Untuk |
|----------|-----|--------------|-------|------|--------------|------------|
| MQTT | 50 | ✅ | ✅ | Sedang | Terbatas | Monitoring IoT real-time |
| BLE | 53 | ✅ | ❌ | Rendah | Smartphone | Wearable, sensor |
| ESP-NOW | 54 | ❌ (Espressif) | ❌ | Rendah | Hanya Espressif | Robot, drone |
| ESP-MESH | 55 | ❌ (Espressif) | ❌ | Rendah | Hanya Espressif | Sensor mesh lokal |
| **Matter** | **56** | **✅ (CSA)** | **❌ (Lokal!)** | **Sedang** | **Semua brand** | **Smart home produksi** |
| **Thread** | **56** | **✅ (OpenThread)** | **❌** | **Sangat Rendah** | **Semua brand** | **Sensor baterai mesh** |

---

## 56.11 Ringkasan

```
RINGKASAN BAB 56 — Matter & Thread

MATTER:
  Standar IoT open-source dari CSA (Apple/Google/Amazon/Samsung)
  Interoperable lintas brand — satu perangkat, semua platform!
  Berjalan di atas: WiFi, Thread, atau Ethernet
  Model data: Node → Endpoint → Cluster → Attribute → Command
  Commissioning: BLE → PASE (PIN) → CASE (Sertifikat) → Fabric

THREAD:
  Protokol mesh IPv6 berbasis IEEE 802.15.4
  Sangat hemat daya — cocok untuk sensor baterai bertahun-tahun
  Self-healing mesh: Leader → Router → End Device
  Butuh Border Router untuk akses internet
  Hardware: ESP32-H2 (Thread only) atau ESP32-C6 (WiFi + Thread)

MATTER DATA MODEL (Hierarki):
  Node (ESP32 fisik)
  └── Endpoint N (perangkat logis)
      └── Cluster (kelompok fungsi)
          ├── Attribute (nilai data — baca/tulis)
          └── Command  (aksi — dipicu oleh Commissioner)

MATTER UNIT KHUSUS:
  Suhu   : integer, unit 0.01°C (29.5°C = 2950)
  Kelembaban : integer, unit 0.01% (65% = 6500)
  Level  : uint8_t 0-254 (untuk LevelControl Cluster)

PILIHAN HARDWARE ESP32:
  Matter over WiFi   → ESP32 Classic, S3, C3, C6, H2
  Matter over Thread → ESP32-C6, ESP32-H2 saja!

ANTIPATTERN:
  ❌ Pakai ESP32 Classic untuk Thread → tidak ada radio 802.15.4
  ❌ Flash ulang dengan --erase-flash → kehilangan commissioning data
  ❌ Pakai Arduino IDE untuk esp-matter → butuh ESP-IDF
  ❌ Thread network dengan PANID berbeda → node tidak bisa join
  ❌ Panggil otUdpSend sebelum join Thread network → selalu gagal
```

---

## 56.12 Latihan

### Latihan Dasar
1. Upload **Program 1** ke ESP32 Classic → buka browser → akses `http://<IP>/matter` → amati struktur JSON Node, Endpoint, dan Cluster.
2. Dari browser, klik tombol **TOGGLE** → verifikasi relay menyala dan nilai `OnOff` di JSON berubah menjadi `true`.
3. Gunakan Postman atau `curl` untuk memanggil `PUT /matter/1/onoff` dengan body `{"value":true}` → verifikasi relay menyala via Serial Monitor.

### Latihan Menengah
4. Modifikasi **Program 1**: tambahkan Endpoint 3 untuk "On/Off Fan" (relay IO5) dengan route `/matter/3/fan` yang terpisah dari Endpoint 1 (relay IO4).
5. Modifikasi **Program 1**: tambahkan halaman HTML `/matter/history` yang menampilkan 10 perintah terakhir (timestamp + ON/OFF) menggunakan array `String` di memori.
6. *(Untuk yang punya ESP32-H2/C6)* Flash **Program 2** → commission via Google Home → kendalikan relay langsung dari aplikasi Google Home!

### Tantangan Lanjutan
7. **Matter + MQTT Bridge:** Buat sketch ESP32 Classic yang mensimulasikan Matter endpoint via HTTP dan sekaligus mem-forward perubahan state ke broker MQTT Antares (kombinasi BAB 50 + BAB 56).
8. **Matter Data Model Lengkap:** Tambahkan ke Program 1: Endpoint 4 (Color Temp Light dengan Cluster `ColorControl`) — simulasikan perubahan color temperature 2700K-6500K via HTTP API.
9. *(Advanced)* **Thread + Matter Commission:** Dengan 2 ESP32-H2, buat satu node sebagai Matter Thread device dan satu sebagai Thread Border Router sederhana menggunakan `openthread_border_router` component.

---

## 56.13 Preview: Apa Selanjutnya? (→ BAB 57)

```
BAB 56 (Matter & Thread) — Protokol open standard masa depan:
  Matter = Interoperabilitas lintas brand
  Thread = Mesh IPv6 hemat daya
  ✅ Open standard | ✅ Lokal (tanpa cloud) | ❌ Hardware khusus

BAB 57 (Blynk) — IoT Platform cloud siap pakai:
  ESP32 ──► Blynk Server (cloud) ──► Blynk App (smartphone)
  ✅ Dashboard siap pakai | ✅ No coding for mobile app
  ✅ Real-time monitoring | ✅ Virtual Pins (mudah!)
  ✅ Notifikasi push | ✅ Bekerja di luar jaringan lokal (via internet)

Kapan pilih Matter vs Blynk?
  Matter : Smart home produksi, perlu interoperabilitas, tanpa cloud
  Blynk  : Prototyping cepat, monitoring remote, dashboard instan
```

---

## 56.14 Referensi

| Sumber | Link |
|--------|------|
| Matter Specification (CSA) | https://csa-iot.org/developer-resource/specifications-download-request/ |
| esp-matter SDK (Espressif) | https://github.com/espressif/esp-matter |
| esp-matter Docs | https://docs.espressif.com/projects/esp-matter/en/latest/ |
| OpenThread.io | https://openthread.io/ |
| ESP-IDF OpenThread Guide | https://docs.espressif.com/projects/esp-idf/en/latest/esp32h2/api-guides/openthread.html |
| Matter Cluster Specification | https://github.com/CHIP-Specifications/connectedhomeip-spec |
| Thread Group | https://www.threadgroup.org/ |
| Google Home Matter Setup | https://developers.home.google.com/matter/integration/setup |

---

## ☕ Dukung Proyek Ini

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

---

*Sebelumnya → [BAB 55: ESP-MESH](../BAB-55/README.md)*

*Selanjutnya → [BAB 57: Blynk — IoT Platform](../BAB-57/README.md)*
