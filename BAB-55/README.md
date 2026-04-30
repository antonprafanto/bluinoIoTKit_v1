# 🕸️ BAB 55 — ESP-MESH: Jaringan Mesh Self-Healing Antar ESP32

> ✅ **Prasyarat:** Selesaikan **BAB 54 (ESP-NOW)**. Kamu harus sudah memahami komunikasi peer-to-peer antar ESP32 dan konsep MAC address.

> 📖 **Glosarium Singkatan BAB ini:**
> | Singkatan | Kepanjangan |
> |-----------|-------------|
> | **Mesh** | Jaringan di mana setiap node bisa berkomunikasi dengan node lain via multi-hop |
> | **Node** | Satu perangkat ESP32 dalam jaringan mesh |
> | **Root** | Node utama (opsional) yang bisa terhubung ke internet/router |
> | **Gateway** | Node yang menjadi penghubung antara mesh dan jaringan luar |
> | **Self-healing** | Kemampuan mesh untuk otomatis rute ulang jika ada node yang mati |
> | **Topology** | Peta koneksi semua node dalam mesh — bisa berubah dinamis |
> | **painlessMesh** | Library Arduino untuk ESP32/ESP8266 yang menyederhanakan ESP-MESH |
> | **NodeID** | ID unik 32-bit setiap node (derived dari MAC address) |
> | **Broadcast** | Kirim pesan ke SEMUA node dalam mesh sekaligus |
> | **Unicast** | Kirim pesan ke SATU node spesifik via NodeID |
> | **TaskScheduler** | Library timer non-blocking yang digunakan painlessMesh untuk penjadwalan |

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami perbedaan arsitektur ESP-MESH vs ESP-NOW (multi-hop vs point-to-point).
- Menginstal dan menggunakan library `painlessMesh` untuk membangun jaringan mesh.
- Mengirim pesan broadcast ke seluruh node dan unicast ke node tertentu.
- Membangun jaringan sensor mesh yang mengirim data DHT11 dari beberapa node ke satu gateway.
- Menerapkan topology change callback untuk monitoring kesehatan jaringan mesh.

---

## 55.1 Apa itu ESP-MESH?

```
ESP-MESH vs ESP-NOW vs WiFi:

  WiFi:      ESP32 ──► Router ──► Internet  (butuh infrastruktur)
  ESP-NOW:   ESP32 ──► ESP32               (1 hop, max 20 peer)
  ESP-MESH:  ESP32 ──► ESP32 ──► ESP32 ──► ESP32
                               (multi-hop, self-healing, ratusan node!)

Kelebihan ESP-MESH (painlessMesh):
  ► Tidak butuh router/AP — mesh membuat jaringan sendiri!
  ► Multi-hop: pesan diteruskan otomatis antar node
  ► Self-healing: jika node mati, rute otomatis dicari ulang
  ► Semua node setara (tidak ada "master") — peer-to-peer sejati
  ► NodeID unik otomatis (dari MAC address, tidak perlu dicatat manual)
  ► Sinkronisasi waktu otomatis antar semua node!
  ► Satu sketch untuk semua node — tidak perlu modifikasi per-node!
```

| Aspek | ESP-NOW (BAB 54) | **ESP-MESH (BAB 55)** |
|-------|-----------------|----------------------|
| **Hop** | 1 hop saja | **Multi-hop** |
| **Max Node** | 20 peer | **Ratusan node** |
| **Self-healing** | ❌ Tidak | **✅ Otomatis** |
| **NodeID** | MAC address manual | **Otomatis (32-bit)** |
| **Sinkronisasi Waktu** | ❌ | **✅ Otomatis** |
| **Library** | esp_now.h (bawaan) | **painlessMesh (install)** |
| **Kerumitan** | Sedang | **Lebih tinggi** |

---

## 55.2 Arsitektur painlessMesh

```
Jaringan Mesh dengan painlessMesh:

  Node A (NodeID: 111111)
      │
      ├──── Node B (NodeID: 222222) ───── Node D (NodeID: 444444)
      │                                         │
      └──── Node C (NodeID: 333333) ────────────┘

  ► Jika Node B mati:
    Sebelum: A → B → D
    Sesudah: A → C → D  (self-healing otomatis!)

  ► Semua node menggunakan SSID + Password mesh yang SAMA:
    MESH_PREFIX   = "BluinoMesh"   (nama jaringan WiFi mesh)
    MESH_PASSWORD = "bluino2026"  (password jaringan mesh)
    MESH_PORT     = 5555           (port TCP internal mesh)

  ► Cara kerja internal painlessMesh:
    1. Setiap node membuat AP (Access Point) + Station sekaligus
    2. Node secara otomatis scan & connect ke node terdekat
    3. Pesan dikirim via TCP dan di-relay antar node otomatis
    4. Topology berubah → semua node diberitahu via callback
```

---

## 55.3 Library & API Reference

```
LIBRARY YANG DIGUNAKAN:
  painlessMesh  → Install via Library Manager (TIDAK bawaan ESP32!)
                  Dependencies yang otomatis ikut:
                  - TaskScheduler (>= 3.0.0)
                  - ArduinoJson   (>= 6.0.0)

  TaskScheduler → Untuk penjadwalan task non-blocking di mesh

CARA INSTALL (Library Manager):
  Arduino IDE → Sketch → Include Library → Manage Libraries...
  Cari: "painlessMesh" → Install (by Johny Goedhart, Ivo Pullens, Thomas Carpenter)
  ✅ Semua dependency akan otomatis terinstall!

API PAINLESSMESH ESENSIAL:

  INISIASI (di setup()):
    Scheduler userScheduler;               // Scheduler TaskScheduler
    painlessMesh mesh;                     // Objek mesh

    mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION); // Log level
    mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);

  CALLBACK (daftarkan di setup() sebelum mesh.update()):
    mesh.onReceive(&receivedCallback);          // Terima pesan
    mesh.onNewConnection(&newConnCallback);     // Node baru masuk
    mesh.onChangedConnections(&changedConnCb);  // Topologi berubah
    mesh.onNodeTimeAdjusted(&timeCb);          // Waktu disinkronkan

  KIRIM PESAN:
    mesh.sendBroadcast(String msg);            // Ke semua node
    mesh.sendSingle(uint32_t nodeId, String); // Ke satu node
    mesh.getNodeId();                          // NodeID node ini

  CEK TOPOLOGI:
    mesh.getNodeList();  // std::list<uint32_t> semua NodeID yang terkoneksi
    mesh.getNodeList().size() + 1; // Total node (termasuk diri sendiri)

  PENJADWALAN TASK (pengganti millis() di mesh):
    Task myTask(interval_ms, jumlah_repeat, &fungsiCallback);
    userScheduler.addTask(myTask);
    myTask.enable();

  DI loop() — WAJIB dipanggil keduanya!:
    mesh.update();
    userScheduler.execute();
```

---

## 55.4 Setup Library painlessMesh

> ⚠️ **Langkah ini WAJIB sebelum mencoba Program apapun di BAB 55!**

1. Buka **Arduino IDE** → **Sketch** → **Include Library** → **Manage Libraries...**
2. Cari `painlessMesh` → Klik **Install** pada versi terbaru.
3. Jika muncul popup "Install all dependencies?" → klik **Install All**.
4. Verifikasi: Sketch → Include Library → lihat `painlessMesh` ada di daftar.

> 💡 **Verifikasi instalasi:** Buka File → Examples → painlessMesh → basic — jika ada, berarti library sudah terinstall.

---

## 55.5 Program 1: Mesh Broadcast — Semua Node Saling Berkirim Pesan

**Topologi:** Semua node ESP32 menjalankan **satu sketch yang sama**. Setiap node mem-broadcast pesan JSON (berisi NodeID + uptime) setiap 5 detik, dan setiap node menerima pesan dari semua node lain dalam mesh.

> ✅ **Keunggulan mesh:** Cukup upload sketch yang sama ke semua ESP32 — tidak perlu ubah kode per node!

### Tujuan Pembelajaran
- Memahami cara kerja `mesh.init()`, callback `onReceive`, dan `sendBroadcast`
- Mempraktikkan penggunaan `Task` dan `Scheduler` non-blocking bersama mesh
- Mengamati self-healing: matikan satu node → amati pesan tetap mengalir via node lain

### Hardware
- Minimal 2 ESP32 (direkomendasikan 3+ node untuk melihat multi-hop)
- OLED opsional pada salah satu node

```cpp
/*
 * BAB 55 - Program 1: Mesh Broadcast — Semua Node Saling Berkirim Pesan
 *
 * Sketch ini IDENTIK untuk semua node — upload ke semua ESP32!
 * Setiap node akan otomatis mendapat NodeID unik dari MAC address-nya.
 *
 * Fitur:
 *   ► Setiap node broadcast pesan JSON tiap 5 detik
 *   ► Setiap node terima & print pesan dari node lain
 *   ► Callback onNewConnection: log saat ada node baru masuk
 *   ► Callback onChangedConnections: log topologi & jumlah node
 *   ► Heartbeat ke Serial Monitor tiap 60 detik
 *
 * Library yang dibutuhkan:
 *   painlessMesh (install via Library Manager — lihat 55.4)
 *
 * Cara uji:
 *   1. Upload sketch ini ke ESP32 #1, #2, #3 (semua sama!)
 *   2. Buka Serial Monitor di ESP32 #1 (115200 baud)
 *   3. Amati pesan dari node lain muncul setiap 5 detik
 *   4. Matikan ESP32 #2 → amati pesan dari #3 masih diterima #1 (self-healing!)
 */

#include "painlessMesh.h"

// ── Konfigurasi Mesh — HARUS SAMA di semua node! ──────────────────────
#define MESH_PREFIX   "BluinoMesh"
#define MESH_PASSWORD "bluino2026"
#define MESH_PORT     5555

// ── Objek Mesh ────────────────────────────────────────────────────────
Scheduler     userScheduler;
painlessMesh  mesh;

// ── State ─────────────────────────────────────────────────────────────
uint32_t broadcastCount = 0;
uint32_t recvCount      = 0;

// ── Fungsi Broadcast (dipanggil oleh Task) ────────────────────────────
void broadcastMessage() {
  broadcastCount++;
  String msg = "{\"nodeId\":" + String(mesh.getNodeId()) +
               ",\"uptime\":" + String(millis() / 1000) +
               ",\"seq\":" + String(broadcastCount) +
               ",\"nodes\":" + String(mesh.getNodeList().size() + 1) + "}";
  mesh.sendBroadcast(msg);
  Serial.printf("[MESH] Broadcast #%u: %s\n", broadcastCount, msg.c_str());
}

// ── Task non-blocking: broadcast tiap 5 detik ─────────────────────────
Task taskBroadcast(5000, TASK_FOREVER, &broadcastMessage);

// ── Callback: terima pesan dari node lain ─────────────────────────────
void receivedCallback(uint32_t from, String &msg) {
  recvCount++;
  Serial.printf("[MESH] Terima dari %u: %s\n", from, msg.c_str());
}

// ── Callback: ada node baru bergabung ─────────────────────────────────
void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("[MESH] ✅ Node baru bergabung: %u | Total node: %u\n",
    nodeId, mesh.getNodeList().size() + 1);
}

// ── Callback: topologi mesh berubah (node masuk/keluar) ───────────────
void changedConnectionsCallback() {
  Serial.printf("[MESH] 🔄 Topologi berubah! Total node aktif: %u\n",
    mesh.getNodeList().size() + 1);
}

// ── Callback: sinkronisasi waktu mesh selesai ─────────────────────────
void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("[MESH] ⏱️ Waktu disinkronkan. Offset: %d µs\n", offset);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== BAB 55 Program 1: Mesh Broadcast ===");

  // Set debug level (gunakan ERROR saja saat sudah stabil)
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);

  // Inisiasi mesh — satu baris untuk semua!
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);

  // Daftarkan semua callback
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionsCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  // Tambahkan task broadcast ke scheduler
  userScheduler.addTask(taskBroadcast);
  taskBroadcast.enable();

  Serial.printf("[MESH] ✅ NodeID saya: %u\n", mesh.getNodeId());
  Serial.println("[MESH] Menunggu node lain bergabung...");
}

void loop() {
  mesh.update();           // WAJIB — proses semua event mesh
  userScheduler.execute(); // WAJIB — jalankan semua task terjadwal

  // Heartbeat (tidak pakai millis() standalone agar aman dengan mesh timing)
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] NodeID:%u | Nodes:%u | Sent:%u | Rcv:%u | Heap:%u B\n",
      mesh.getNodeId(), mesh.getNodeList().size() + 1,
      broadcastCount, recvCount, ESP.getFreeHeap());
  }
}
```

**Output Serial Monitor (3 node aktif):**
```
=== BAB 55 Program 1: Mesh Broadcast ===
[MESH] ✅ NodeID saya: 2887297537
[MESH] Menunggu node lain bergabung...
[MESH] ✅ Node baru bergabung: 3232236578 | Total node: 2
[MESH] 🔄 Topologi berubah! Total node aktif: 2
[MESH] ✅ Node baru bergabung: 1092712345 | Total node: 3
[MESH] Broadcast #1: {"nodeId":2887297537,"uptime":5,"seq":1,"nodes":3}
[MESH] Terima dari 3232236578: {"nodeId":3232236578,"uptime":5,"seq":1,"nodes":3}
[MESH] Terima dari 1092712345: {"nodeId":1092712345,"uptime":5,"seq":1,"nodes":3}
--- (ESP32 #2 dimatikan) ---
[MESH] 🔄 Topologi berubah! Total node aktif: 2
[MESH] Terima dari 1092712345: {"nodeId":1092712345,"uptime":35,"seq":6,"nodes":2}
[HB] NodeID:2887297537 | Nodes:2 | Sent:12 | Rcv:24 | Heap:190000 B
```

> ⚠️ **Dua Aturan Kritis painlessMesh yang Sering Dilupakan:**
> 1. **`mesh.update()` + `userScheduler.execute()`** — KEDUANYA WAJIB di `loop()`. Jika salah satu hilang, mesh freeze.
> 2. **`MESH_PREFIX`, `MESH_PASSWORD`, `MESH_PORT`** — HARUS IDENTIK di semua node. Satu huruf beda = tidak bisa bergabung.

---

## 55.6 Program 2: Mesh Sensor Network — Sensor Node + Gateway Node

**Topologi:** Beberapa ESP32 Sensor Node membaca DHT11 dan mengirim data JSON ke seluruh mesh setiap 10 detik. Satu ESP32 Gateway Node menerima semua data, menampilkannya di OLED, dan merangkum laporan suhu tertinggi.

> ⚠️ **Program 2 terdiri dari DUA sketch:**
> - `p2_mesh_sensor_node.ino` → upload ke **semua ESP32 Sensor** (ada DHT11)
> - `p2_mesh_gateway.ino` → upload ke **ESP32 Gateway** (ada OLED, tanpa DHT11)

> ✅ **Catatan:** Semua node menggunakan `MESH_PREFIX`, `MESH_PASSWORD`, dan `MESH_PORT` yang sama — mereka saling bergabung otomatis!

### Tujuan Pembelajaran
- Membangun jaringan sensor mesh nyata dengan peran berbeda (sensor vs gateway)
- Mem-parse JSON dari pesan mesh menggunakan ArduinoJson
- Melacak data dari beberapa NodeID yang berbeda secara dinamis

### Hardware
- **ESP32 Sensor Node (1-3 unit):** DHT11 (IO27)
- **ESP32 Gateway (1 unit):** OLED SDA=IO21, SCL=IO22

### Sketch 2A — Sensor Node (`p2_mesh_sensor_node.ino`)

```cpp
/*
 * BAB 55 - Program 2A: Mesh Sensor Network — SENSOR NODE
 *
 * Fungsi: Baca DHT11 → kirim data JSON ke seluruh mesh tiap 10 detik
 * Upload sketch ini ke SEMUA ESP32 yang ada sensor DHT11!
 *
 * Setiap node terima data dari sensor node lain juga (mesh sifatnya broadcast)
 *
 * Hardware: DHT11 → IO27
 * Library:  painlessMesh, DHT sensor library (Adafruit)
 */

#include "painlessMesh.h"
#include <DHT.h>
#include <ArduinoJson.h>

// ── Konfigurasi Mesh ──────────────────────────────────────────────────
#define MESH_PREFIX   "BluinoMesh"
#define MESH_PASSWORD "bluino2026"
#define MESH_PORT     5555

// ── Hardware ──────────────────────────────────────────────────────────
#define DHT_PIN 27
DHT dht(DHT_PIN, DHT11);

// ── Objek Mesh ────────────────────────────────────────────────────────
Scheduler    userScheduler;
painlessMesh mesh;

// ── State ─────────────────────────────────────────────────────────────
float    lastT     = NAN;
float    lastH     = NAN;
uint32_t sendCount = 0;

// ── Fungsi: kirim data sensor ke mesh ─────────────────────────────────
void sendSensorData() {
  float t = dht.readTemperature(), h = dht.readHumidity();
  if (isnan(t) || isnan(h)) {
    Serial.println("[DHT] ⚠️ Gagal baca sensor, skip kirim.");
    return;
  }
  lastT = t; lastH = h;
  sendCount++;

  // Buat JSON payload
  StaticJsonDocument<128> doc;
  doc["type"]   = "sensor";
  doc["nodeId"] = mesh.getNodeId();
  doc["temp"]   = round(t * 10) / 10.0;
  doc["hum"]    = round(h);
  doc["seq"]    = sendCount;
  doc["uptime"] = millis() / 1000;

  String msg;
  serializeJson(doc, msg);
  mesh.sendBroadcast(msg);
  Serial.printf("[MESH] → Kirim #%u: %s\n", sendCount, msg.c_str());
}

// ── Task: kirim sensor tiap 10 detik ─────────────────────────────────
Task taskSendSensor(10000, TASK_FOREVER, &sendSensorData);

// ── Callback: terima data dari node lain ─────────────────────────────
void receivedCallback(uint32_t from, String &msg) {
  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, msg) != DeserializationError::Ok) return;
  if (doc["type"] != "sensor") return;

  float t = doc["temp"] | 0.0f;
  float h = doc["hum"]  | 0.0f;
  Serial.printf("[MESH] ← Dari Node %u: %.1f°C | %.0f%%\n", from, t, h);
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("[MESH] ✅ Node baru: %u | Total: %u\n",
    nodeId, mesh.getNodeList().size() + 1);
}

void changedConnectionsCallback() {
  Serial.printf("[MESH] 🔄 Topologi berubah. Aktif: %u node\n",
    mesh.getNodeList().size() + 1);
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  Serial.println("\n=== BAB 55 Program 2A: Mesh Sensor Node ===");

  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionsCallback);

  userScheduler.addTask(taskSendSensor);
  taskSendSensor.enable();

  Serial.printf("[MESH] ✅ NodeID: %u\n", mesh.getNodeId());

  // Baca sensor awal
  float t = dht.readTemperature(), h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    lastT = t; lastH = h;
    Serial.printf("[DHT] ✅ Baca OK: %.1f°C | %.0f%%\n", t, h);
  }
}

void loop() {
  mesh.update();
  userScheduler.execute();

  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] Node:%u | Sent:%u | T:%.1f H:%.0f | Heap:%u B\n",
      mesh.getNodeId(), sendCount, lastT, lastH, ESP.getFreeHeap());
  }
}
```

### Sketch 2B — Gateway Node (`p2_mesh_gateway.ino`)

```cpp
/*
 * BAB 55 - Program 2B: Mesh Sensor Network — GATEWAY NODE
 *
 * Fungsi: Terima data sensor dari semua Sensor Node → tampilkan di OLED & Serial
 * Upload sketch ini ke ESP32 Gateway (dengan OLED, tanpa DHT11)
 *
 * Fitur:
 *   ► Terima & parse JSON dari semua sensor node
 *   ► Lacak suhu & kelembaban dari NodeID yang berbeda (max 4 node)
 *   ► Tampilkan laporan ringkasan di OLED tiap 5 detik
 *   ► Log suhu tertinggi dari seluruh jaringan
 *
 * Hardware: OLED → SDA=IO21, SCL=IO22
 */

#include "painlessMesh.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <ArduinoJson.h>

// ── Konfigurasi Mesh ──────────────────────────────────────────────────
#define MESH_PREFIX   "BluinoMesh"
#define MESH_PASSWORD "bluino2026"
#define MESH_PORT     5555

// ── Hardware ──────────────────────────────────────────────────────────
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool oledReady = false;

// ── Objek Mesh ────────────────────────────────────────────────────────
Scheduler    userScheduler;
painlessMesh mesh;

// ── Struktur untuk menyimpan data tiap sensor node (max 4) ────────────
struct SensorRecord {
  uint32_t nodeId;
  float    temp;
  float    hum;
  uint32_t lastSeen; // millis()
};

const int MAX_NODES = 4;
SensorRecord knownNodes[MAX_NODES];
int nodeCount = 0;
uint32_t totalRecv = 0;

// ── Simpan / update data dari sensor node ─────────────────────────────
void updateNodeData(uint32_t nodeId, float t, float h) {
  // Cari apakah nodeId sudah ada
  for (int i = 0; i < nodeCount; i++) {
    if (knownNodes[i].nodeId == nodeId) {
      knownNodes[i].temp     = t;
      knownNodes[i].hum      = h;
      knownNodes[i].lastSeen = millis();
      return;
    }
  }
  // Node baru — tambahkan jika masih ada slot
  if (nodeCount < MAX_NODES) {
    knownNodes[nodeCount] = {nodeId, t, h, millis()};
    nodeCount++;
    Serial.printf("[GW] Node baru terdaftar: %u (total: %d)\n", nodeId, nodeCount);
  }
}

// ── Update tampilan OLED ──────────────────────────────────────────────
void refreshOLED() {
  if (!oledReady) return;
  oled.clearDisplay(); oled.setTextColor(WHITE); oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.printf("Mesh Gateway [%d node]", nodeCount);
  oled.drawLine(0, 10, 127, 10, WHITE);

  float maxTemp = -999;
  int   yPos    = 14;
  for (int i = 0; i < nodeCount && i < 3; i++) {
    oled.setCursor(0, yPos);
    // Tampilkan 6 digit terakhir NodeID saja agar muat layar
    oled.printf("N%06lu %.1fC %.0f%%",
      knownNodes[i].nodeId % 1000000UL,
      knownNodes[i].temp, knownNodes[i].hum);
    if (knownNodes[i].temp > maxTemp) maxTemp = knownNodes[i].temp;
    yPos += 10;
  }
  oled.setCursor(0, 54);
  oled.printf("Max:%.1fC Rcv:%u", maxTemp, totalRecv);
  oled.display();
}

// ── Task: refresh OLED tiap 5 detik ──────────────────────────────────
Task taskRefreshOLED(5000, TASK_FOREVER, &refreshOLED);

// ── Callback: terima pesan dari sensor node ───────────────────────────
void receivedCallback(uint32_t from, String &msg) {
  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, msg) != DeserializationError::Ok) return;
  if (doc["type"] != "sensor") return;

  float t = doc["temp"] | 0.0f;
  float h = doc["hum"]  | 0.0f;
  totalRecv++;
  updateNodeData(from, t, h);
  Serial.printf("[GW] Terima #%u dari Node %u: %.1f°C | %.0f%%\n",
    totalRecv, from, t, h);

  // Log suhu tertinggi
  static float maxTempEver = 0;
  if (t > maxTempEver) {
    maxTempEver = t;
    Serial.printf("[GW] 🌡️ Suhu tertinggi baru: %.1f°C dari Node %u\n", t, from);
  }
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("[GW] ✅ Node bergabung: %u | Total: %u\n",
    nodeId, mesh.getNodeList().size() + 1);
}

void changedConnectionsCallback() {
  Serial.printf("[GW] 🔄 Topologi berubah. Aktif: %u node\n",
    mesh.getNodeList().size() + 1);
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  Serial.println("\n=== BAB 55 Program 2B: Mesh Gateway Node ===");

  if (oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledReady = true;
    Serial.println("[OLED] ✅ Display terdeteksi di 0x3C");
    oled.clearDisplay(); oled.setTextColor(WHITE);
    oled.setCursor(0, 0); oled.print("Mesh Gateway");
    oled.setCursor(0, 16); oled.print("Menunggu sensor...");
    oled.display();
  } else {
    Serial.println("[OLED] ⚠️ Display tidak terdeteksi.");
  }

  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionsCallback);

  userScheduler.addTask(taskRefreshOLED);
  taskRefreshOLED.enable();

  Serial.printf("[GW] ✅ NodeID Gateway: %u\n", mesh.getNodeId());
  Serial.println("[GW] Siap menerima data dari Sensor Node...");
}

void loop() {
  mesh.update();
  userScheduler.execute();

  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] GW NodeID:%u | KnownNodes:%d | Rcv:%u | Heap:%u B\n",
      mesh.getNodeId(), nodeCount, totalRecv, ESP.getFreeHeap());
  }
}
```

**Output Serial Monitor Gateway (2 Sensor Node aktif):**
```
=== BAB 55 Program 2B: Mesh Gateway Node ===
[OLED] ✅ Display terdeteksi di 0x3C
[GW] ✅ NodeID Gateway: 4294967295
[GW] Siap menerima data dari Sensor Node...
[GW] ✅ Node bergabung: 2887297537 | Total: 2
[GW] ✅ Node bergabung: 1092712345 | Total: 3
[GW] Node baru terdaftar: 2887297537 (total: 1)
[GW] Terima #1 dari Node 2887297537: 29.1°C | 65%
[GW] 🌡️ Suhu tertinggi baru: 29.1°C dari Node 2887297537
[GW] Node baru terdaftar: 1092712345 (total: 2)
[GW] Terima #2 dari Node 1092712345: 31.3°C | 58%
[GW] 🌡️ Suhu tertinggi baru: 31.3°C dari Node 1092712345
[HB] GW NodeID:4294967295 | KnownNodes:2 | Rcv:12 | Heap:176000 B
```

---

## 55.7 Program 3: Mesh LED Controller — Unicast ke Node Tertentu

**Topologi:** Satu ESP32 Controller mengirim perintah `LED ON`/`LED OFF` ke NodeID spesifik menggunakan `mesh.sendSingle()`. ESP32 Worker menerima perintah dan toggle LED onboard (IO2).

> ⚠️ **Program 3 terdiri dari DUA sketch:**
> - `p3_mesh_controller.ino` → upload ke **ESP32 Controller**
> - `p3_mesh_worker.ino` → upload ke **semua ESP32 Worker**

### Tujuan Pembelajaran
- Memahami perbedaan `sendBroadcast()` vs `sendSingle()` untuk unicast
- Menggunakan `mesh.getNodeList()` untuk mendapatkan daftar NodeID aktif
- Mengirim perintah ke node tertentu berdasarkan urutan join atau NodeID

### Sketch 3A — Controller (`p3_mesh_controller.ino`)

```cpp
/*
 * BAB 55 - Program 3A: Mesh LED Controller — CONTROLLER
 *
 * Fungsi: Kirim perintah LED ON/OFF ke node tertentu via sendSingle()
 * Setiap 5 detik, toggle LED pada node pertama yang terdaftar di mesh.
 *
 * Hardware: Tidak butuh sensor/actuator khusus (Serial Monitor untuk monitor)
 */

#include "painlessMesh.h"
#include <ArduinoJson.h>

// ── Konfigurasi Mesh ──────────────────────────────────────────────────
#define MESH_PREFIX   "BluinoMesh"
#define MESH_PASSWORD "bluino2026"
#define MESH_PORT     5555

Scheduler    userScheduler;
painlessMesh mesh;

uint32_t cmdCount  = 0;
bool     ledTarget = false;

// ── Fungsi: kirim perintah toggle LED ke semua node worker ────────────
void sendLedCommand() {
  auto nodeList = mesh.getNodeList();
  if (nodeList.empty()) {
    Serial.println("[CTRL] Tidak ada node aktif di mesh.");
    return;
  }

  ledTarget = !ledTarget;
  cmdCount++;

  StaticJsonDocument<64> doc;
  doc["type"]  = "cmd";
  doc["cmd"]   = "LED";
  doc["state"] = ledTarget ? "ON" : "OFF";
  doc["seq"]   = cmdCount;

  String msg;
  serializeJson(doc, msg);

  // Kirim ke SEMUA node worker via loop (untuk demonstrasi sendSingle)
  int sent = 0;
  for (uint32_t nodeId : nodeList) {
    mesh.sendSingle(nodeId, msg);
    Serial.printf("[CTRL] sendSingle ke %u: LED %s\n",
      nodeId, ledTarget ? "ON" : "OFF");
    sent++;
  }
  Serial.printf("[CTRL] Perintah #%u dikirim ke %d node\n", cmdCount, sent);
}

// ── Task: kirim perintah tiap 5 detik ─────────────────────────────────
Task taskSendCmd(5000, TASK_FOREVER, &sendLedCommand);

// ── Callback: terima balasan ACK dari Worker ──────────────────────────
void receivedCallback(uint32_t from, String &msg) {
  StaticJsonDocument<64> doc;
  if (deserializeJson(doc, msg) != DeserializationError::Ok) return;
  if (doc["type"] == "ack") {
    Serial.printf("[CTRL] ✅ ACK dari Node %u: LED=%s\n",
      from, doc["state"].as<const char*>());
  }
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("[CTRL] ✅ Worker bergabung: %u | Total: %u\n",
    nodeId, mesh.getNodeList().size() + 1);
}

void changedConnectionsCallback() {
  Serial.printf("[CTRL] 🔄 Topologi berubah. Aktif: %u node\n",
    mesh.getNodeList().size() + 1);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== BAB 55 Program 3A: Mesh LED Controller ===");

  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionsCallback);

  userScheduler.addTask(taskSendCmd);
  taskSendCmd.enable();

  Serial.printf("[CTRL] ✅ NodeID Controller: %u\n", mesh.getNodeId());
  Serial.println("[CTRL] Menunggu Worker bergabung...");
}

void loop() {
  mesh.update();
  userScheduler.execute();
}
```

### Sketch 3B — Worker (`p3_mesh_worker.ino`)

```cpp
/*
 * BAB 55 - Program 3B: Mesh LED Controller — WORKER
 *
 * Fungsi: Terima perintah LED ON/OFF dari Controller → eksekusi → kirim ACK
 * Upload ke semua ESP32 yang akan menjadi node worker!
 *
 * Hardware: LED onboard di IO2 (built-in semua ESP32)
 */

#include "painlessMesh.h"
#include <ArduinoJson.h>

// ── Konfigurasi Mesh ──────────────────────────────────────────────────
#define MESH_PREFIX   "BluinoMesh"
#define MESH_PASSWORD "bluino2026"
#define MESH_PORT     5555

#define LED_PIN 2  // LED onboard ESP32

Scheduler    userScheduler;
painlessMesh mesh;

bool     ledState  = false;
uint32_t recvCount = 0;

// ── Set LED dan log ───────────────────────────────────────────────────
void setLED(bool on) {
  ledState = on;
  digitalWrite(LED_PIN, on ? HIGH : LOW);
  Serial.printf("[WORK] LED → %s\n", on ? "ON" : "OFF");
}

// ── Callback: terima perintah dari Controller ─────────────────────────
void receivedCallback(uint32_t from, String &msg) {
  StaticJsonDocument<64> doc;
  if (deserializeJson(doc, msg) != DeserializationError::Ok) return;
  if (doc["type"] != "cmd" || doc["cmd"] != "LED") return;

  recvCount++;
  String stateStr = doc["state"].as<String>();
  stateStr.toUpperCase();
  bool newState = (stateStr == "ON");
  Serial.printf("[WORK] Terima #%u perintah dari %u: LED=%s\n",
    recvCount, from, stateStr.c_str());

  setLED(newState);

  // Kirim ACK balik ke Controller
  StaticJsonDocument<64> ack;
  ack["type"]  = "ack";
  ack["state"] = ledState ? "ON" : "OFF";
  ack["seq"]   = doc["seq"] | 0;
  String ackMsg;
  serializeJson(ack, ackMsg);
  mesh.sendSingle(from, ackMsg);
  Serial.printf("[WORK] ACK → Node %u: %s\n", from, ackMsg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("[WORK] ✅ Node bergabung: %u\n", nodeId);
}

void changedConnectionsCallback() {
  Serial.printf("[WORK] 🔄 Topologi berubah. Aktif: %u node\n",
    mesh.getNodeList().size() + 1);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  setLED(false); // Mulai dalam keadaan OFF

  Serial.println("\n=== BAB 55 Program 3B: Mesh LED Worker ===");

  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionsCallback);

  Serial.printf("[WORK] ✅ NodeID Worker: %u\n", mesh.getNodeId());
  Serial.println("[WORK] Menunggu perintah dari Controller...");
}

void loop() {
  mesh.update();
  userScheduler.execute();

  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] Worker:%u | LED:%s | Rcv:%u | Heap:%u B\n",
      mesh.getNodeId(), ledState?"ON":"OFF", recvCount, ESP.getFreeHeap());
  }
}
```

**Output Serial Monitor Controller:**
```
=== BAB 55 Program 3A: Mesh LED Controller ===
[CTRL] ✅ NodeID Controller: 3735928559
[CTRL] Menunggu Worker bergabung...
[CTRL] ✅ Worker bergabung: 2887297537 | Total: 2
[CTRL] ✅ Worker bergabung: 1092712345 | Total: 3
[CTRL] sendSingle ke 2887297537: LED ON
[CTRL] sendSingle ke 1092712345: LED ON
[CTRL] Perintah #1 dikirim ke 2 node
[CTRL] ✅ ACK dari Node 2887297537: LED=ON
[CTRL] ✅ ACK dari Node 1092712345: LED=ON
[CTRL] sendSingle ke 2887297537: LED OFF
[CTRL] sendSingle ke 1092712345: LED OFF
[CTRL] Perintah #2 dikirim ke 2 node
```

---

## 55.8 Troubleshooting ESP-MESH (painlessMesh)

| Gejala | Penyebab | Solusi |
|--------|----------|--------|
| Node tidak bisa bergabung ke mesh | `MESH_PREFIX`, `MESH_PASSWORD`, atau `MESH_PORT` berbeda | Pastikan ketiga konstanta IDENTIK di semua sketch node |
| Mesh freeze / tidak responsif | `mesh.update()` atau `userScheduler.execute()` tidak dipanggil di `loop()` | Pastikan **KEDUANYA** ada di setiap iterasi `loop()` |
| Compile error: `painlessMesh.h not found` | Library belum terinstall | Library Manager → cari "painlessMesh" → Install All Dependencies |
| Pesan tidak diterima antar node | Node terlalu jauh atau terlalu banyak obstacle | Dekatkan ESP32 (< 30m dalam ruangan); mesh perlu minimal 2 node aktif |
| `mesh.getNodeList()` selalu kosong | Node baru bergabung butuh waktu (biasanya 5-10 detik) | Tunggu setelah `newConnectionCallback` dipanggil sebelum akses NodeList |
| Nilai JSON tidak terbaca / 0 semua | ArduinoJson doc terlalu kecil | Naikkan ukuran `StaticJsonDocument<256>` atau gunakan `DynamicJsonDocument` |
| Crash / reboot terus saat mesh aktif | Heap habis — painlessMesh cukup boros RAM (~60KB) | Kurangi ukuran JSON, hindari `String` di global scope, monitor `ESP.getFreeHeap()` |
| Node tidak bisa bergabung setelah reboot | Node lama masih di cache mesh | Tunggu 10-15 detik; mesh akan detect node mati via timeout |

> ⚠️ **POLA WAJIB — Struktur program painlessMesh yang benar:**
> ```cpp
> Scheduler    userScheduler;           // 1. Deklarasi scheduler
> painlessMesh mesh;                    // 2. Deklarasi mesh
>
> void myTask() { /* Kirim pesan, baca sensor, dll */ }
> Task taskMy(5000, TASK_FOREVER, &myTask); // 3. Deklarasi task
>
> void receivedCallback(uint32_t from, String &msg) { /* Terima */ }
> void newConnectionCallback(uint32_t nodeId)       { /* Join  */ }
> void changedConnectionsCallback()                 { /* Topo  */ }
>
> void setup() {
>   mesh.setDebugMsgTypes(ERROR | STARTUP);
>   mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
>   mesh.onReceive(&receivedCallback);           // 4. Callback
>   mesh.onNewConnection(&newConnectionCallback);
>   mesh.onChangedConnections(&changedConnectionsCallback);
>   userScheduler.addTask(taskMy);               // 5. Tambah task
>   taskMy.enable();                             // 6. Enable task
> }
>
> void loop() {
>   mesh.update();           // 7. WAJIB — keduanya!
>   userScheduler.execute(); // 8. WAJIB — jangan pernah hilangkan!
> }
> ```

---

## 55.9 Perbandingan Protokol Komunikasi Nirkabel BAB 44-55

| Protokol | BAB | Router | Jarak | Node Maks | Self-Healing | Cocok Untuk |
|----------|-----|--------|-------|-----------|-------------|------------|
| WiFi MQTT | 50 | ✅ | 30-100m | Unlimited | ❌ | IoT Cloud |
| BLE | 53 | ❌ | 10-30m | Multi | ❌ | Wearable |
| ESP-NOW | 54 | ❌ | ~200m | 20 peer | ❌ | Drone, robot, sensor |
| **ESP-MESH** | **55** | **❌** | **~200m/hop** | **Ratusan** | **✅** | **Sensor mesh, smart building** |

---

## 55.10 Ringkasan

```
RINGKASAN BAB 55 — ESP-MESH dengan painlessMesh

PRINSIP DASAR:
  painlessMesh = library Arduino untuk ESP-MESH self-healing
  Semua node setara — tidak ada master/slave
  NodeID otomatis dari MAC address (uint32_t)
  Satu sketch bisa diupload ke semua node (tidak perlu modifikasi)!

KONFIGURASI (WAJIB SAMA di semua node):
  #define MESH_PREFIX   "NamaMesh"  // Nama jaringan mesh
  #define MESH_PASSWORD "pass"      // Password jaringan mesh
  #define MESH_PORT     5555        // Port TCP internal (bebas)

INISIASI (urutan WAJIB):
  mesh.setDebugMsgTypes(ERROR | STARTUP);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCb);
  mesh.onNewConnection(&newConnCb);
  mesh.onChangedConnections(&changedCb);

KIRIM PESAN:
  mesh.sendBroadcast(String msg);           // Ke semua node
  mesh.sendSingle(uint32_t nodeId, msg);   // Ke 1 node spesifik

TERIMA PESAN:
  void receivedCallback(uint32_t from, String &msg) {
    // Parse msg (JSON direkomendasikan)
  }

CEK TOPOLOGI:
  mesh.getNodeId();          // NodeID node ini
  mesh.getNodeList();        // std::list<uint32_t> semua node lain
  mesh.getNodeList().size(); // Jumlah node SELAIN diri sendiri

LOOP — WAJIB KEDUANYA!:
  mesh.update();
  userScheduler.execute();

ANTIPATTERN:
  ❌ MESH_PREFIX berbeda antar node → tidak bisa bergabung
  ❌ Tidak panggil mesh.update() di loop → mesh freeze
  ❌ Operasi blocking (delay) di loop → mesh kehilangan paket
  ❌ String global terlalu besar → heap habis, crash
  ❌ Lupa addTask + enable → task tidak pernah jalan
```

---

## 55.11 Latihan

### Latihan Dasar
1. Install library `painlessMesh` via Library Manager → compile **Program 1** → upload ke 2 ESP32 → verifikasi keduanya saling mengirim dan menerima pesan di Serial Monitor.
2. Ganti `MESH_PREFIX` di salah satu ESP32 → amati: node tersebut tidak bisa bergabung ke mesh → perbaiki kembali.
3. Upload **Program 1** ke 3 ESP32 → amati berapa NodeID yang terdaftar → matikan satu node → verifikasi `changedConnectionsCallback` terpanggil.

### Latihan Menengah
4. Modifikasi **Program 2 Gateway**: tambahkan alert ke Serial jika suhu dari node manapun melebihi 35°C (`[ALERT] ⚠️ Suhu kritis!`).
5. Modifikasi **Program 1**: tambahkan field `"rssi"` di JSON payload menggunakan `WiFi.RSSI()` — Gateway dapat mengetahui kekuatan sinyal tiap sensor node.
6. Upload **Program 3** (Controller + 2 Worker) → amati LED menyala/mati bergantian setiap 5 detik → matikan salah satu Worker → verifikasi mesh tetap mengirim ke Worker yang masih aktif.

### Tantangan Lanjutan
7. **Mesh + Cloud Gateway:** Sambungkan salah satu node ke WiFi router (dual mode: mesh + WiFi station) → forward data sensor mesh ke Antares/MQTT broker via WiFi.
8. **Mesh Time Sync Logger:** Manfaatkan `mesh.getNodeTime()` — buat semua node mencatat timestamp yang sama → kumpulkan di Gateway → verifikasi sinkronisasi waktu antar node.
9. **Mesh Alert System:** Buat sistem: jika salah satu sensor node deteksi suhu > 35°C, node itu broadcast pesan darurat `{type: "alert"}` → semua node lain nyalakan LED dan kirim pesan balik konfirmasi.

---

## 55.12 Preview: Apa Selanjutnya? (→ BAB 56)

```
BAB 55 (ESP-MESH) — Jaringan mesh proprietary Espressif:
  ESP32 ──► ESP32 ──► ESP32 (self-healing, multi-hop, ratusan node)
  ✅ Self-healing | ✅ Ratusan node | ❌ Proprietary (hanya Espressif)
  ❌ Interoperabilitas terbatas dengan perangkat non-Espressif

BAB 56 (Matter & Thread) — Protokol open-standard smart home baru:
  Matter: Standar terbuka untuk Smart Home (Apple/Google/Amazon)
  Thread: Protokol mesh IPv6 untuk IoT (IP-based, open standard)
  ✅ Interoperable dengan SEMUA brand | ✅ Open standard
  ✅ Matter over Thread = mesh IoT masa depan
  ❌ Dukungan ESP32 masih terbatas (ESP32-H2, ESP32-C6)

Kapan pilih ESP-MESH vs Matter/Thread?
  ESP-MESH : Proyek internal, hanya perangkat Espressif, performa tinggi
  Matter   : Smart Home produksi, butuh interoperabilitas lintas brand
```

---

## 55.13 Referensi

| Sumber | Link |
|--------|------|
| painlessMesh Library (GitHub) | https://github.com/gmag11/painlessMesh |
| painlessMesh Documentation | https://painlessmesh.gitlab.io/painlessMesh/ |
| TaskScheduler Library | https://github.com/arkhipenko/TaskScheduler |
| ESP-MESH Architecture (Espressif) | https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/esp-wifi-mesh.html |
| painlessMesh Examples (Arduino) | https://github.com/gmag11/painlessMesh/tree/master/examples |

---

## ☕ Dukung Proyek Ini

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

---

*Sebelumnya → [BAB 54: ESP-NOW](../BAB-54/README.md)*

*Selanjutnya → [BAB 56: Protokol Modern — Matter & Thread](../BAB-56/README.md)*
