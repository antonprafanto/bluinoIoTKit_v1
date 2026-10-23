/*
 * BAB 55 - Program 1: Mesh Broadcast
 * Sketch yang SAMA di-upload ke semua board (minimal 2 ESP32).
 * Setiap 5 detik, setiap node mengirim sapaan ke semua node lain (broadcast)
 * dan mencetak sapaan yang diterimanya. Perubahan susunan mesh juga dicetak.
 *
 * Rangkaian: tidak perlu
 * Library  : Painless Mesh (+ TaskScheduler, ArduinoJson 7, Async TCP)
 */

#include <painlessMesh.h>

// Pengaturan mesh: harus SAMA PERSIS di semua node
#define MESH_PREFIX   "BluinoMesh-andi"  // ← ganti "andi" dengan nama panggilanmu
#define MESH_PASSWORD "rahasia123"       // minimal 8 karakter
#define MESH_PORT     5555
#define MESH_CHANNEL  1

// struct Timer dari BAB 16
struct Timer {
  unsigned long interval;   // ms
  unsigned long waktuLalu;  // hasil millis() saat terakhir dijalankan

  // Mengembalikan true kalau sudah waktunya, lalu mencatat waktunya
  bool sudahWaktunya(unsigned long sekarang) {
    if (sekarang - waktuLalu >= interval) {
      waktuLalu = sekarang;
      return true;
    }
    return false;
  }
};

painlessMesh mesh;

Timer timerSapa = {5000, 0};      // kirim sapaan setiap 5 detik
Timer timerLaporan = {30000, 0};  // ringkasan setiap 30 detik
unsigned long nomorSapaan = 0;
unsigned long jumlahDiterima = 0;

// ---------- Fungsi callback (dipanggil dari dalam mesh.update()) ----------

void pesanMasuk(uint32_t dari, String &pesan) {
  jumlahDiterima = jumlahDiterima + 1;
  Serial.printf("[TERIMA] dari %lu: %s\n", (unsigned long)dari, pesan.c_str());
}

void nodeBaru(uint32_t nodeId) {
  Serial.printf("[MESH] Tersambung langsung dengan node %lu\n", (unsigned long)nodeId);
}

void nodePutus(uint32_t nodeId) {
  Serial.printf("[MESH] Sambungan langsung ke node %lu putus\n", (unsigned long)nodeId);
}

void susunanBerubah() {
  // getNodeList(true) = semua node yang dikenal, termasuk node ini
  Serial.printf("[MESH] Susunan mesh berubah. Jumlah node: %u\n",
                (unsigned)mesh.getNodeList(true).size());
  Serial.printf("[MESH] Susunan: %s\n", mesh.subConnectionJson().c_str());
}

void jamDisesuaikan(int32_t selisih) {
  Serial.printf("[JAM] Jam mesh disesuaikan %ld mikrodetik\n", (long)selisih);
}

// ---------- Tugas berkala ----------

void kirimSapaan() {
  nomorSapaan = nomorSapaan + 1;
  String pesan = "Halo dari " + String(mesh.getNodeId()) + ", sapaan ke-" + String(nomorSapaan);

  if (mesh.sendBroadcast(pesan)) {
    Serial.printf("[KIRIM] %s\n", pesan.c_str());
  } else {
    Serial.println("[KIRIM] Belum ada node lain, sapaan tidak terkirim");
  }
}

void cetakLaporan() {
  std::list<uint32_t> daftar = mesh.getNodeList();  // node lain (tanpa node ini)
  Serial.printf("[LAPORAN] Node ini: %lu | node lain: %u | terkirim: %lu | diterima: %lu\n",
                (unsigned long)mesh.getNodeId(), (unsigned)daftar.size(),
                nomorSapaan, jumlahDiterima);
  for (uint32_t id : daftar) {
    Serial.printf("          - %lu\n", (unsigned long)id);
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nBAB 55 - Program 1: Mesh Broadcast");

  mesh.setDebugMsgTypes(ERROR | STARTUP);  // panggil sebelum init()
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, MESH_CHANNEL);

  mesh.onReceive(&pesanMasuk);
  mesh.onNewConnection(&nodeBaru);
  mesh.onDroppedConnection(&nodePutus);
  mesh.onChangedConnections(&susunanBerubah);
  mesh.onNodeTimeAdjusted(&jamDisesuaikan);

  Serial.printf("Node ID saya: %lu\n", (unsigned long)mesh.getNodeId());
  Serial.printf("Nama mesh   : %s (kanal %d)\n", MESH_PREFIX, MESH_CHANNEL);
  Serial.println("Mencari node lain...");
}

void loop() {
  mesh.update();  // wajib, sesering mungkin

  unsigned long sekarang = millis();
  if (timerSapa.sudahWaktunya(sekarang)) {
    kirimSapaan();
  }
  if (timerLaporan.sudahWaktunya(sekarang)) {
    cetakLaporan();
  }
}
