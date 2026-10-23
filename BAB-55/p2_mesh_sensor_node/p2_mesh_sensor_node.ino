/*
 * BAB 55 - Program 2A: Node Sensor (DHT11)
 * Setiap 10 detik membaca DHT11 lalu menyiarkan hasilnya (JSON) ke semua
 * node di mesh. Gateway (Program 2B atau 4) yang mengolah datanya.
 *
 * Rangkaian: DHT (header hijau) -> IO27 (board kit)
 * Board tanpa DHT11: ubah MODE_SIMULASI menjadi true (angka buatan)
 * Library  : Painless Mesh (+ TaskScheduler, ArduinoJson 7, Async TCP),
 *            DHT sensor library + Adafruit Unified Sensor
 */

#include <painlessMesh.h>
#include <DHT.h>

// Pengaturan mesh: harus SAMA PERSIS di semua node
#define MESH_PREFIX   "BluinoMesh-andi"  // ← ganti "andi" dengan nama panggilanmu
#define MESH_PASSWORD "rahasia123"       // minimal 8 karakter
#define MESH_PORT     5555
#define MESH_CHANNEL  1                  // Program 4: samakan dengan kanal router

#define PIN_DHT  27
#define TIPE_DHT DHT11
#define MODE_SIMULASI false  // true = kirim angka buatan (untuk board tanpa DHT11)

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

painlessMesh mesh;
DHT dht(PIN_DHT, TIPE_DHT);

Timer timerKirim = {10000, 0};  // kirim data setiap 10 detik
unsigned long nomorKirim = 0;

void kirimDataSensor() {
  float suhu;
  float kelembapan;
  if (MODE_SIMULASI) {
    suhu = 25.0 + random(0, 51) / 10.0;  // angka buatan 25,0 - 30,0
    kelembapan = random(60, 81);         // angka buatan 60 - 80
  } else {
    kelembapan = dht.readHumidity();
    suhu = dht.readTemperature();
  }
  bool dhtOk = !isnan(suhu) && !isnan(kelembapan);
  nomorKirim = nomorKirim + 1;

  // Susun pesan JSON dengan ArduinoJson 7
  JsonDocument doc;
  doc["jenis"] = "sensor";
  if (dhtOk) {
    doc["suhu"] = suhu;              // °C
    doc["kelembapan"] = kelembapan;  // %RH
  } else {
    doc["suhu"] = nullptr;  // null = tidak ada data valid
    doc["kelembapan"] = nullptr;
    Serial.println("[DHT11] Gagal membaca. Periksa kabel DHT -> IO27.");
  }
  doc["dht_ok"] = dhtOk;
  doc["simulasi"] = MODE_SIMULASI;
  doc["urutan"] = nomorKirim;

  String pesan;
  serializeJson(doc, pesan);

  if (mesh.sendBroadcast(pesan)) {
    Serial.printf("[KIRIM] %s\n", pesan.c_str());
  } else {
    Serial.printf("[KIRIM] Belum ada node lain, tidak terkirim: %s\n", pesan.c_str());
  }
}

// Node sensor ikut menerima siaran node lain; cukup dicetak
void pesanMasuk(uint32_t dari, String &pesan) {
  Serial.printf("[TERIMA] dari %lu: %s\n", (unsigned long)dari, pesan.c_str());
}

void susunanBerubah() {
  Serial.printf("[MESH] Susunan berubah. Jumlah node: %u\n",
                (unsigned)mesh.getNodeList(true).size());
}

void setup() {
  Serial.begin(115200);
  Serial.println("\nBAB 55 - Program 2A: Node Sensor");
  dht.begin();

  mesh.setDebugMsgTypes(ERROR);  // hanya pesan error dari library
  mesh.init(MESH_PREFIX, MESH_PASSWORD, MESH_PORT, WIFI_AP_STA, MESH_CHANNEL);
  mesh.onReceive(&pesanMasuk);
  mesh.onChangedConnections(&susunanBerubah);

  Serial.printf("Node ID saya: %lu\n", (unsigned long)mesh.getNodeId());
  Serial.printf("Mode simulasi: %s\n", MODE_SIMULASI ? "YA (angka buatan)" : "tidak (DHT11 asli)");
}

void loop() {
  mesh.update();  // wajib, sesering mungkin

  if (timerKirim.sudahWaktunya(millis())) {
    kirimDataSensor();
  }
}
