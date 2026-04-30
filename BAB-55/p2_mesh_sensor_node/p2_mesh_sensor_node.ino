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
