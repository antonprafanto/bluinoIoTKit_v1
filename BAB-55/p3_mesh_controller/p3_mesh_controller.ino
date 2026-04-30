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
