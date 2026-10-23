/*
 * BAB 55 - Program 2B: Gateway OLED
 * Menerima data JSON dari semua node sensor (Program 2A), menyimpan data
 * terakhir setiap node, lalu menampilkannya di Serial Monitor dan OLED.
 * OLED opsional: tanpa OLED, data tetap tampil di Serial Monitor.
 *
 * Rangkaian: OLED lewat jumper cap JP (SDA IO21, SCL IO22) - hanya di kit
 * Library  : Painless Mesh (+ TaskScheduler, ArduinoJson 7, Async TCP),
 *            Adafruit SSD1306 + Adafruit GFX Library
 */

#include <painlessMesh.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Pengaturan mesh: harus SAMA PERSIS di semua node
#define MESH_PREFIX   "BluinoMesh-andi"  // ← ganti "andi" dengan nama panggilanmu
#define MESH_PASSWORD "rahasia123"       // minimal 8 karakter
#define MESH_PORT     5555
#define MESH_CHANNEL  1

#define ALAMAT_OLED     0x3C
#define MAKS_NODE       4         // jumlah node sensor yang dicatat
#define BATAS_DATA_LAMA 30000UL   // data lebih tua dari 30 detik = "hilang"

// struct Timer dari BAB 16
struct Timer {
  unsigned long interval;   // ms
  unsigned long waktuLalu;  // hasil millis() saat terakhir dijalankan

  bool sudahWaktunya(unsigned long sekarang) {
    if (sekarang - waktuLalu >= interval) {
      waktuLalu = sekarang;
      return true;
    }
    return false;
  }
};

// Data terakhir dari satu node sensor
struct DataNode {
  uint32_t nodeId;
  float suhu;
  float kelembapan;
  bool dhtOk;
  bool simulasi;
  unsigned long waktuTerima;  // millis() saat data terakhir diterima
};

painlessMesh mesh;
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool adaOled = false;

DataNode daftarNode[MAKS_NODE];
int jumlahNode = 0;
unsigned long jumlahData = 0;

Timer timerLayar = {2000, 0};       // perbarui OLED setiap 2 detik
Timer timerRingkasan = {30000, 0};  // ringkasan Serial setiap 30 detik

// Cari posisi node di daftar; tambahkan kalau belum ada. Hasil -1 kalau penuh.
int cariAtauTambahNode(uint32_t nodeId) {
  for (int i = 0; i < jumlahNode; i++) {
    if (daftarNode[i].nodeId == nodeId) {
      return i;
    }
  }
  if (jumlahNode >= MAKS_NODE) {
    return -1;
  }
  daftarNode[jumlahNode].nodeId = nodeId;
  jumlahNode = jumlahNode + 1;
  Serial.printf("[GATEWAY] Node sensor baru: %lu (total %d)\n", (unsigned long)nodeId, jumlahNode);
  return jumlahNode - 1;
}

bool dataMasihBaru(const DataNode &d) {
  return millis() - d.waktuTerima < BATAS_DATA_LAMA;
}

// Dipanggil setiap ada pesan dari node lain
void pesanMasuk(uint32_t dari, String &pesan) {
  JsonDocument doc;
  DeserializationError galat = deserializeJson(doc, pesan);
  if (galat) {
    Serial.printf("[GATEWAY] Bukan JSON dari %lu: %s\n", (unsigned long)dari, pesan.c_str());
    return;
  }
  if (doc["jenis"] != "sensor") {
    return;  // bukan data sensor, abaikan
  }

  int i = cariAtauTambahNode(dari);
  if (i < 0) {
    Serial.printf("[GATEWAY] Daftar penuh, data node %lu diabaikan\n", (unsigned long)dari);
    return;
  }

  DataNode &d = daftarNode[i];
  d.suhu = doc["suhu"] | NAN;  // null -> NAN
  d.kelembapan = doc["kelembapan"] | NAN;
  d.dhtOk = (doc["dht_ok"] | false) && !isnan(d.suhu) && !isnan(d.kelembapan);
  d.simulasi = doc["simulasi"] | false;
  d.waktuTerima = millis();
  jumlahData = jumlahData + 1;

  if (d.dhtOk) {
    Serial.printf("[DATA] Node %lu: suhu %.1f °C, kelembapan %.0f %%%s\n",
                  (unsigned long)dari, d.suhu, d.kelembapan, d.simulasi ? " (simulasi)" : "");
  } else {
    Serial.printf("[DATA] Node %lu: DHT11 gagal dibaca\n", (unsigned long)dari);
  }
}

void susunanBerubah() {
  Serial.printf("[MESH] Susunan berubah. Jumlah node: %u\n",
                (unsigned)mesh.getNodeList(true).size());
}

void perbaruiLayar() {
  if (!adaOled) {
    return;
  }
  oled.clearDisplay();
  oled.setCursor(0, 0);
  oled.printf("Gateway Mesh  node:%u", (unsigned)mesh.getNodeList(true).size());
  oled.drawLine(0, 10, 127, 10, SSD1306_WHITE);

  float suhuMaks = NAN;
  for (int i = 0; i < jumlahNode; i++) {
    DataNode &d = daftarNode[i];
    oled.setCursor(0, 14 + i * 10);
    oled.printf("%04lu ", (unsigned long)(d.nodeId % 10000));  // 4 angka terakhir Node ID
    if (!dataMasihBaru(d)) {
      oled.print("hilang");
    } else if (!d.dhtOk) {
      oled.print("DHT gagal");
    } else {
      oled.printf("%.1f", d.suhu);
      oled.write(248);  // simbol derajat (cp437)
      oled.printf("C %.0f%%", d.kelembapan);
      if (d.simulasi) {
        oled.print(" S");
      }
      if (isnan(suhuMaks) || d.suhu > suhuMaks) {
        suhuMaks = d.suhu;
      }
    }
  }
  if (jumlahNode == 0) {
    oled.setCursor(0, 14);
    oled.print("Menunggu data...");
  }

  oled.setCursor(0, 56);
  if (isnan(suhuMaks)) {
    oled.print("Maks: -");
  } else {
    oled.printf("Maks: %.1f", suhuMaks);
    oled.write(248);
    oled.print("C");
  }
  oled.display();
}

void cetakRingkasan() {
  Serial.printf("[RINGKASAN] Node di mesh: %u | node sensor tercatat: %d | data diterima: %lu\n",
                (unsigned)mesh.getNodeList(true).size(), jumlahNode, jumlahData);
  for (int i = 0; i < jumlahNode; i++) {
    DataNode &d = daftarNode[i];
    if (!dataMasihBaru(d)) {
      Serial.printf("  %lu: tidak ada data 30 detik terakhir\n", (unsigned long)d.nodeId);
    } else if (!d.dhtOk) {
      Serial.printf("  %lu: DHT11 gagal dibaca\n", (unsigned long)d.nodeId);
    } else {
      Serial.printf("  %lu: %.1f °C, %.0f %%\n", (unsigned long)d.nodeId, d.suhu, d.kelembapan);
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nBAB 55 - Program 2B: Gateway OLED");

  // OLED opsional: periksa dulu apakah ada yang menjawab di 0x3C
  Wire.begin(21, 22);
  Wire.beginTransmission(ALAMAT_OLED);
  if (Wire.endTransmission() == 0 && oled.begin(SSD1306_SWITCHCAPVCC, ALAMAT_OLED)) {
    adaOled = true;
    oled.cp437(true);  // supaya write(248) = simbol derajat
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    Serial.println("OLED ditemukan di 0x3C.");
  } else {
    Serial.println("OLED tidak ditemukan, data hanya tampil di Serial Monitor.");
  }

  mesh.setDebugMsgTypes(ERROR);  // hanya pesan error dari library
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, MESH_CHANNEL);
  mesh.onReceive(&pesanMasuk);
  mesh.onChangedConnections(&susunanBerubah);

  Serial.printf("Node ID gateway: %lu\n", (unsigned long)mesh.getNodeId());
  Serial.println("Menunggu data dari node sensor...");
  perbaruiLayar();
}

void loop() {
  mesh.update();  // wajib, sesering mungkin

  unsigned long sekarang = millis();
  if (timerLayar.sudahWaktunya(sekarang)) {
    perbaruiLayar();
  }
  if (timerRingkasan.sudahWaktunya(sekarang)) {
    cetakRingkasan();
  }
}
