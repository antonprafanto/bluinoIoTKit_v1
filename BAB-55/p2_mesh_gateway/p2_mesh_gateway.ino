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
