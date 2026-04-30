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
