/*
 * BAB 53 - Program 2: Kontrol Relay dari HP (Write)
 * Service Layanan Bluino berisi dua characteristic:
 *   - Data Sensor (READ + NOTIFY)      : {"t":28.6,"h":70} setiap 2 detik
 *   - Relay (READ + WRITE + NOTIFY)    : HP menulis ON / OFF / TOGGLE (atau 1 / 0),
 *                                        ESP32 membalas status "ON" atau "OFF"
 * Pengaman: relay otomatis mati saat sambungan BLE terputus.
 * Rangkaian: DHT -> IO27, RELAY -> IO26.
 * LED biru bawaan board (IO2): berkedip = menunggu HP, menyala = HP tersambung.
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLE2902.h>  // descriptor CCCD: sakelar notifikasi
#include <BLE2901.h>  // descriptor label teks
#include <DHT.h>

const char *NAMA_BLE = "Bluino-BLE";  // nama yang muncul di HP (boleh diganti)

// UUID buatan sendiri (lihat 53.4), huruf kecil semua
#define UUID_LAYANAN "2ed00001-fc4c-400d-bb3c-1953ec992b13"
#define UUID_SENSOR "2ed00002-fc4c-400d-bb3c-1953ec992b13"
#define UUID_RELAY "2ed00003-fc4c-400d-bb3c-1953ec992b13"

#define PIN_DHT 27
#define TIPE_DHT DHT11
#define PIN_RELAY 26  // RELAY, aktif HIGH (BAB 37)
#define LED_BIRU 2    // LED biru bawaan board ESP32, aktif HIGH

// Jenis perintah dari HP (tipe data ditulis di atas, sebelum fungsi pertama)
enum Perintah { TIDAK_ADA, NYALAKAN, MATIKAN, BALIK, TIDAK_DIKENAL };

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

DHT dht(PIN_DHT, TIPE_DHT);
BLECharacteristic *karSensor = nullptr;  // characteristic Data Sensor
BLECharacteristic *karRelay = nullptr;   // characteristic Relay
BLE2902 *cccdSensor = nullptr;           // sakelar notifikasi Data Sensor
BLE2902 *cccdRelay = nullptr;            // sakelar notifikasi Relay

Timer timerSensor = {2000, 0};  // baca DHT11 setiap 2 detik
Timer timerKedip = {500, 0};    // kedip LED selama menunggu HP

// Diubah oleh callback (task Bluetooth), dibaca oleh loop()
volatile bool hpTersambung = false;
volatile Perintah perintahMasuk = TIDAK_ADA;

bool tersambungSebelumnya = false;
bool ledNyala = false;
bool relayNyala = false;
unsigned long jumlahNotifikasi = 0;

// Callback sambungan: cukup ubah penanda, sisanya dikerjakan loop()
class PenanganServer : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    hpTersambung = true;
  }
  void onDisconnect(BLEServer *server) override {
    hpTersambung = false;
  }
};

// Dipanggil saat HP menulis ke characteristic Relay
class PenanganRelay : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *kar) override {
    String teks = kar->getValue();  // core 3.x: getValue() mengembalikan String
    teks.trim();                    // buang spasi/Enter di awal dan akhir
    teks.toUpperCase();             // "on", "On", dan "ON" dianggap sama
    if (teks == "ON" || teks == "1") {
      perintahMasuk = NYALAKAN;
    } else if (teks == "OFF" || teks == "0") {
      perintahMasuk = MATIKAN;
    } else if (teks == "TOGGLE") {
      perintahMasuk = BALIK;
    } else {
      perintahMasuk = TIDAK_DIKENAL;
    }
  }
};

// Membuat characteristic beserta CCCD dan labelnya
BLECharacteristic *buatCharacteristic(BLEService *layanan, const char *uuid, uint32_t sifat,
                                      BLE2902 *&cccd, const char *keterangan) {
  BLECharacteristic *kar = layanan->createCharacteristic(uuid, sifat);
  cccd = new BLE2902();  // wajib untuk NOTIFY di ESP32 (Bluedroid)
  kar->addDescriptor(cccd);
  BLE2901 *label = new BLE2901();
  label->setDescription(keterangan);
  label->setAccessPermissions(ESP_GATT_PERM_READ);  // label hanya boleh dibaca
  kar->addDescriptor(label);
  return kar;
}

void siapkanBLE() {
  if (!BLEDevice::init(NAMA_BLE)) {
    Serial.println("[BLE] Gagal menyalakan BLE! ESP32 restart dalam 5 detik...");
    delay(5000);
    ESP.restart();
  }

  BLEServer *server = BLEDevice::createServer();
  server->setCallbacks(new PenanganServer());
  server->advertiseOnDisconnect(true);  // beriklan lagi setelah HP memutus

  BLEService *layanan = server->createService(UUID_LAYANAN);

  karSensor = buatCharacteristic(layanan, UUID_SENSOR,
                                 BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY,
                                 cccdSensor, "Suhu (C) dan kelembapan (%)");
  karSensor->setValue("Menunggu data");

  karRelay = buatCharacteristic(layanan, UUID_RELAY,
                                BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE |
                                    BLECharacteristic::PROPERTY_NOTIFY,
                                cccdRelay, "Relay: tulis ON, OFF, atau TOGGLE");
  karRelay->setCallbacks(new PenanganRelay());
  karRelay->setValue("OFF");

  layanan->start();

  BLEAdvertising *iklan = BLEDevice::getAdvertising();
  iklan->addServiceUUID(UUID_LAYANAN);
  iklan->setScanResponse(true);
  BLEDevice::startAdvertising();
}

// Mengubah relay, menyimpan statusnya di characteristic, lalu mengabari HP
void aturRelay(bool nyala) {
  relayNyala = nyala;
  digitalWrite(PIN_RELAY, nyala ? HIGH : LOW);
  karRelay->setValue(nyala ? "ON" : "OFF");
  if (hpTersambung && cccdRelay->getNotifications()) {
    karRelay->notify();
  }
  Serial.printf("[RELAY] %s\n", nyala ? "AKTIF" : "MATI");
}

void jalankanPerintah(Perintah perintah) {
  if (perintah == NYALAKAN) {
    Serial.println("[BLE] Perintah diterima: ON");
    aturRelay(true);
  } else if (perintah == MATIKAN) {
    Serial.println("[BLE] Perintah diterima: OFF");
    aturRelay(false);
  } else if (perintah == BALIK) {
    Serial.println("[BLE] Perintah diterima: TOGGLE");
    aturRelay(!relayNyala);
  } else {
    Serial.printf("[BLE] Perintah tidak dikenal: \"%s\". Pakai ON, OFF, atau TOGGLE.\n",
                  karRelay->getValue().c_str());
    aturRelay(relayNyala);  // kembalikan isi characteristic ke status sebenarnya
  }
}

void bacaDanKirimSensor() {
  dht.read(true);  // paksa membaca sensor sekarang (BAB 26)
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  char teks[24];  // isi maksimal 20 karakter (batas notifikasi, lihat 53.3.3)
  if (isnan(h) || isnan(t)) {
    snprintf(teks, sizeof(teks), "{\"t\":null,\"h\":null}");
    Serial.println("[DHT11] Gagal membaca. Periksa kabel DHT -> IO27.");
  } else {
    snprintf(teks, sizeof(teks), "{\"t\":%.1f,\"h\":%.0f}", t, h);
  }

  karSensor->setValue((uint8_t *)teks, strlen(teks));  // nilai terbaru untuk READ

  Serial.printf("[DHT11] %s -> ", teks);
  if (!hpTersambung) {
    Serial.println("belum ada HP tersambung");
  } else if (!cccdSensor->getNotifications()) {
    Serial.println("HP belum menyalakan notifikasi");
  } else {
    karSensor->notify();  // kirim nilai sekarang ke HP
    jumlahNotifikasi++;
    Serial.printf("notifikasi #%lu terkirim\n", jumlahNotifikasi);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nBAB 53 - Program 2: Kontrol Relay dari HP");

  pinMode(PIN_RELAY, OUTPUT);  // pinMode dulu, baru digitalWrite (core 3.x)
  digitalWrite(PIN_RELAY, LOW);
  pinMode(LED_BIRU, OUTPUT);
  digitalWrite(LED_BIRU, LOW);
  dht.begin();

  siapkanBLE();

  Serial.printf("[BLE] Siap! Nama perangkat: %s\n", NAMA_BLE);
  Serial.printf("[BLE] Alamat BLE (MAC): %s\n", BLEDevice::getAddress().toString().c_str());
  Serial.println("[BLE] Tulis ON, OFF, atau TOGGLE ke characteristic 2ed00003-...");
}

void loop() {
  unsigned long sekarang = millis();

  // 1. Perubahan sambungan (penandanya diubah oleh callback)
  bool tersambung = hpTersambung;
  if (tersambung != tersambungSebelumnya) {
    tersambungSebelumnya = tersambung;
    if (tersambung) {
      Serial.println("[BLE] HP tersambung.");
    } else {
      cccdSensor->setNotifications(false);  // HP berikutnya harus berlangganan lagi
      cccdRelay->setNotifications(false);
      Serial.println("[BLE] HP terputus. ESP32 beriklan lagi.");
      if (relayNyala) {
        Serial.println("[PENGAMAN] Sambungan putus, relay dimatikan.");
        aturRelay(false);
      }
    }
  }

  // 2. Perintah dari HP (penandanya diubah oleh callback onWrite)
  Perintah perintah = perintahMasuk;
  if (perintah != TIDAK_ADA) {
    perintahMasuk = TIDAK_ADA;
    jalankanPerintah(perintah);
  }

  // 3. LED biru: menyala terus saat tersambung, berkedip saat menunggu
  if (tersambung) {
    digitalWrite(LED_BIRU, HIGH);
  } else if (timerKedip.sudahWaktunya(sekarang)) {
    ledNyala = !ledNyala;
    digitalWrite(LED_BIRU, ledNyala ? HIGH : LOW);
  }

  // 4. Baca sensor dan kirim setiap 2 detik
  if (timerSensor.sudahWaktunya(sekarang)) {
    bacaDanKirimSensor();
  }
}
