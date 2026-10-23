/*
 * BAB 55 - Program 3B: Node Pekerja
 * Menunggu perintah LED dari controller (Program 3A) atau gateway MQTT
 * (Program 4), menjalankannya, lalu membalas ke pengirim perintah.
 * Perintah: {"jenis":"perintah","led":"ON" / "OFF" / "TOGGLE"}
 *
 * Rangkaian: LED3 (biru) -> IO2 (opsional; LED biru bawaan board juga di IO2)
 * Library  : Painless Mesh (+ TaskScheduler, ArduinoJson 7, Async TCP)
 */

#include <painlessMesh.h>

// Pengaturan mesh: harus SAMA PERSIS di semua node
#define MESH_PREFIX   "BluinoMesh-andi"  // ← ganti "andi" dengan nama panggilanmu
#define MESH_PASSWORD "rahasia123"       // minimal 8 karakter
#define MESH_PORT     5555
#define MESH_CHANNEL  1                  // Program 4: samakan dengan kanal router

#define LED_BIRU 2  // LED3 (biru) di kit dan LED bawaan board

painlessMesh mesh;
bool ledNyala = false;
unsigned long jumlahPerintah = 0;

void kirimBalasan(uint32_t tujuan) {
  JsonDocument balasan;
  balasan["jenis"] = "balasan";
  balasan["led"] = ledNyala ? "ON" : "OFF";
  balasan["perintah_ke"] = jumlahPerintah;
  String teks;
  serializeJson(balasan, teks);

  if (mesh.sendSingle(tujuan, teks)) {
    Serial.printf("[BALAS] ke %lu: %s\n", (unsigned long)tujuan, teks.c_str());
  } else {
    Serial.printf("[BALAS] Gagal, tidak ada jalur ke %lu\n", (unsigned long)tujuan);
  }
}

void pesanMasuk(uint32_t dari, String &pesan) {
  JsonDocument doc;
  if (deserializeJson(doc, pesan)) {
    return;  // bukan JSON, abaikan
  }
  if (doc["jenis"] != "perintah") {
    return;  // bukan perintah (misalnya data sensor), abaikan
  }

  const char *isi = doc["led"] | "";
  if (strcasecmp(isi, "ON") == 0) {
    ledNyala = true;
  } else if (strcasecmp(isi, "OFF") == 0) {
    ledNyala = false;
  } else if (strcasecmp(isi, "TOGGLE") == 0) {
    ledNyala = !ledNyala;
  } else {
    Serial.printf("[PERINTAH] Tidak dikenal dari %lu: %s\n", (unsigned long)dari, pesan.c_str());
    return;
  }

  digitalWrite(LED_BIRU, ledNyala ? HIGH : LOW);
  jumlahPerintah = jumlahPerintah + 1;
  Serial.printf("[PERINTAH] dari %lu: %s -> LED %s\n",
                (unsigned long)dari, isi, ledNyala ? "ON" : "OFF");
  kirimBalasan(dari);  // balas ke pengirim perintah
}

void susunanBerubah() {
  Serial.printf("[MESH] Susunan berubah. Jumlah node: %u\n",
                (unsigned)mesh.getNodeList(true).size());
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BIRU, OUTPUT);  // pinMode dulu, baru digitalWrite
  digitalWrite(LED_BIRU, LOW);
  Serial.println("\nBAB 55 - Program 3B: Node Pekerja");

  mesh.setDebugMsgTypes(ERROR);  // hanya pesan error dari library
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, MESH_CHANNEL);
  mesh.onReceive(&pesanMasuk);
  mesh.onChangedConnections(&susunanBerubah);

  Serial.printf("Node ID pekerja: %lu\n", (unsigned long)mesh.getNodeId());
  Serial.println("Menunggu perintah...");
}

void loop() {
  mesh.update();  // wajib, sesering mungkin
}
