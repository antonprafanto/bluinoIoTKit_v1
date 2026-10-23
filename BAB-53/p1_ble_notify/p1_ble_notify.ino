/*
 * BAB 53 - Program 1: Sensor DHT11 lewat BLE (Read dan Notify)
 * ESP32 beriklan dengan nama "Bluino-BLE" dan menyediakan satu service berisi
 * characteristic "Data Sensor" (READ + NOTIFY). Isinya teks JSON pendek,
 * misalnya {"t":28.6,"h":70}, diperbarui setiap 2 detik dan dikirim sebagai
 * notifikasi ke HP yang berlangganan.
 * Uji dengan aplikasi nRF Connect for Mobile (Android/iOS).
 * Rangkaian: DHT -> IO27.
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

#define PIN_DHT 27
#define TIPE_DHT DHT11
#define LED_BIRU 2  // LED biru bawaan board ESP32, aktif HIGH

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
BLE2902 *cccdSensor = nullptr;           // sakelar notifikasi milik Data Sensor

Timer timerSensor = {2000, 0};  // baca DHT11 setiap 2 detik
Timer timerKedip = {500, 0};    // kedip LED selama menunggu HP

// Diubah oleh callback (task Bluetooth), dibaca oleh loop()
volatile bool hpTersambung = false;
volatile uint16_t mtuBaru = 0;  // 0 = tidak ada kabar MTU baru

bool tersambungSebelumnya = false;
bool ledNyala = false;
unsigned long jumlahNotifikasi = 0;

// Callback sambungan: cukup ubah penanda, sisanya dikerjakan loop()
class PenanganServer : public BLEServerCallbacks {
  void onConnect(BLEServer *server) override {
    hpTersambung = true;
  }
  void onDisconnect(BLEServer *server) override {
    hpTersambung = false;
  }
  void onMtuChanged(BLEServer *server, esp_ble_gatts_cb_param_t *param) override {
    mtuBaru = param->mtu.mtu;
  }
};

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

  karSensor = layanan->createCharacteristic(
      UUID_SENSOR, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);

  cccdSensor = new BLE2902();  // wajib untuk NOTIFY di ESP32 (Bluedroid)
  karSensor->addDescriptor(cccdSensor);

  BLE2901 *label = new BLE2901();  // label teks (opsional)
  label->setDescription("Suhu (C) dan kelembapan (%)");
  label->setAccessPermissions(ESP_GATT_PERM_READ);  // label hanya boleh dibaca
  karSensor->addDescriptor(label);

  karSensor->setValue("Menunggu data");
  layanan->start();

  BLEAdvertising *iklan = BLEDevice::getAdvertising();
  iklan->addServiceUUID(UUID_LAYANAN);  // HP/browser bisa menyaring berdasarkan UUID ini
  iklan->setScanResponse(true);         // nama yang tidak muat dikirim di scan response
  BLEDevice::startAdvertising();
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
  Serial.println("\nBAB 53 - Program 1: Sensor DHT11 lewat BLE");

  pinMode(LED_BIRU, OUTPUT);  // pinMode dulu, baru digitalWrite (core 3.x)
  digitalWrite(LED_BIRU, LOW);
  dht.begin();

  siapkanBLE();

  Serial.printf("[BLE] Siap! Nama perangkat: %s\n", NAMA_BLE);
  Serial.printf("[BLE] Alamat BLE (MAC): %s\n", BLEDevice::getAddress().toString().c_str());
  Serial.println("[BLE] Buka nRF Connect di HP, SCAN, lalu CONNECT ke perangkat ini.");
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
      Serial.println("[BLE] HP terputus. ESP32 beriklan lagi.");
    }
  }

  // 2. Kabar MTU baru dari callback
  uint16_t mtu = mtuBaru;
  if (mtu != 0) {
    mtuBaru = 0;
    Serial.printf("[BLE] MTU disepakati: %u byte\n", mtu);
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
