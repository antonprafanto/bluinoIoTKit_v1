/*
 * BAB 55 - Program 3A: Controller (Unicast)
 * Mengirim perintah LED ke node pekerja (Program 3B) lewat Serial Monitor:
 *   daftar   -> tampilkan node lain di mesh beserta nomornya
 *   1, 2, .. -> balik (TOGGLE) LED node bernomor itu dengan sendSingle()
 *   on / off -> nyalakan/matikan LED semua node dengan sendBroadcast()
 * Serial Monitor: baud 115200, line ending "New Line"
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

#define MAKS_DAFTAR 10

painlessMesh mesh;

uint32_t nodeTerdaftar[MAKS_DAFTAR];  // hasil perintah "daftar" terakhir
int jumlahTerdaftar = 0;

char baris[32];           // tempat menampung ketikan dari Serial Monitor
size_t panjangBaris = 0;

// Menyusun pesan perintah, misalnya {"jenis":"perintah","led":"TOGGLE"}
String buatPerintah(const char *isiLed) {
  JsonDocument doc;
  doc["jenis"] = "perintah";
  doc["led"] = isiLed;
  String teks;
  serializeJson(doc, teks);
  return teks;
}

void tampilkanDaftar() {
  std::list<uint32_t> daftar = mesh.getNodeList();  // node lain (tanpa node ini)
  daftar.sort();  // urutkan supaya nomornya tidak berubah-ubah

  jumlahTerdaftar = 0;
  for (uint32_t id : daftar) {
    if (jumlahTerdaftar >= MAKS_DAFTAR) {
      break;
    }
    nodeTerdaftar[jumlahTerdaftar] = id;
    jumlahTerdaftar = jumlahTerdaftar + 1;
  }

  if (jumlahTerdaftar == 0) {
    Serial.println("[DAFTAR] Belum ada node lain di mesh.");
    return;
  }
  Serial.printf("[DAFTAR] %d node lain di mesh:\n", jumlahTerdaftar);
  for (int i = 0; i < jumlahTerdaftar; i++) {
    Serial.printf("  %d. %lu\n", i + 1, (unsigned long)nodeTerdaftar[i]);
  }
  Serial.println("Ketik nomornya untuk membalik LED node itu, atau on / off untuk semua.");
}

void prosesPerintah(const char *teks) {
  if (strlen(teks) == 0) {
    return;  // baris kosong
  }
  if (strcasecmp(teks, "daftar") == 0) {
    tampilkanDaftar();
    return;
  }
  if (strcasecmp(teks, "on") == 0 || strcasecmp(teks, "off") == 0) {
    String pesan = buatPerintah(strcasecmp(teks, "on") == 0 ? "ON" : "OFF");
    if (mesh.sendBroadcast(pesan)) {
      Serial.printf("[BROADCAST] %s\n", pesan.c_str());
    } else {
      Serial.println("[BROADCAST] Gagal: belum ada node lain.");
    }
    return;
  }

  int nomor = atoi(teks);  // "2" -> 2; teks yang bukan angka -> 0
  if (nomor >= 1 && nomor <= jumlahTerdaftar) {
    uint32_t tujuan = nodeTerdaftar[nomor - 1];
    if (!mesh.isConnected(tujuan)) {
      Serial.printf("[UNICAST] Node %lu sudah tidak ada di mesh. Ketik daftar lagi.\n",
                    (unsigned long)tujuan);
      return;
    }
    String pesan = buatPerintah("TOGGLE");
    if (mesh.sendSingle(tujuan, pesan)) {
      Serial.printf("[UNICAST] ke %lu: %s\n", (unsigned long)tujuan, pesan.c_str());
    } else {
      Serial.printf("[UNICAST] Gagal mengirim ke %lu\n", (unsigned long)tujuan);
    }
    return;
  }

  Serial.printf("Perintah \"%s\" tidak dikenal. Ketik: daftar, nomor node, on, atau off\n", teks);
}

// Membaca ketikan Serial Monitor tanpa menunggu (non-blocking)
void bacaSerial() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\r') {
      continue;  // abaikan CR kalau line ending "Both NL & CR"
    }
    if (c == '\n') {
      baris[panjangBaris] = '\0';  // akhiri teks
      prosesPerintah(baris);
      panjangBaris = 0;
    } else if (panjangBaris < sizeof(baris) - 1) {
      baris[panjangBaris] = c;
      panjangBaris = panjangBaris + 1;
    }
  }
}

// Balasan dari node pekerja
void pesanMasuk(uint32_t dari, String &pesan) {
  JsonDocument doc;
  if (deserializeJson(doc, pesan)) {
    return;  // bukan JSON, abaikan
  }
  if (doc["jenis"] == "balasan") {
    const char *led = doc["led"] | "?";
    Serial.printf("[BALASAN] Node %lu: LED %s\n", (unsigned long)dari, led);
  }
}

void susunanBerubah() {
  Serial.printf("[MESH] Susunan berubah. Jumlah node: %u (ketik daftar untuk nomor terbaru)\n",
                (unsigned)mesh.getNodeList(true).size());
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nBAB 55 - Program 3A: Controller");

  mesh.setDebugMsgTypes(ERROR);  // hanya pesan error dari library
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, MESH_CHANNEL);
  mesh.onReceive(&pesanMasuk);
  mesh.onChangedConnections(&susunanBerubah);

  Serial.printf("Node ID controller: %lu\n", (unsigned long)mesh.getNodeId());
  Serial.println("Perintah (line ending New Line):");
  Serial.println("  daftar   -> tampilkan node lain beserta nomornya");
  Serial.println("  1, 2, .. -> balik LED node bernomor itu (unicast)");
  Serial.println("  on / off -> nyalakan/matikan LED semua node (broadcast)");
}

void loop() {
  mesh.update();  // wajib, sesering mungkin
  bacaSerial();   // tidak pernah menunggu
}
