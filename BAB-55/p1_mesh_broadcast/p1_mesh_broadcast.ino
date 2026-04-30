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
