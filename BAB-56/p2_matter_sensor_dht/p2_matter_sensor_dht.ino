/*
 * BAB 56 - Program 2: Sensor Suhu & Kelembapan Matter (DHT11)
 * ESP32 menjadi perangkat Matter dengan dua endpoint:
 *   - Temperature Sensor : suhu dari DHT11
 *   - Humidity Sensor    : kelembapan dari DHT11
 * Nilainya muncul di aplikasi rumah pintar dan diperbarui setiap 10 detik.
 *
 * Rangkaian: DHT -> IO27, SW1 -> IO15
 * Board    : ESP32 Dev Module
 *            Tools -> Partition Scheme -> Huge APP (3MB No OTA/1MB SPIFFS)
 * Library  : Matter, WiFi (bawaan core esp32 3.3.x)
 *            DHT sensor library + Adafruit Unified Sensor (Adafruit)
 *
 * Tombol S1: tekan sebentar -> baca DHT11 sekarang
 *            tahan 5 detik  -> hapus pendaftaran Matter (decommission)
 */

#include <WiFi.h>
#include <Matter.h>
#include <DHT.h>

const char *NAMA_WIFI = "NamaWiFiKamu";          // ← ganti dengan WiFi kamu
const char *PASSWORD_WIFI = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu

#define PIN_DHT 27        // DHT11 (header hijau DHT)
#define PIN_TOMBOL_S1 15  // SW1, ditekan = HIGH (pull-down 10 kOhm di PCB)

const unsigned long BATAS_TUNGGU_DAFTAR = 10UL * 60UL * 1000UL;  // 10 menit

// Pola struct Timer dari BAB 16, diberi nama Pewaktu karena nama
// "Timer" sudah dipakai library Matter (cluster Timer)
struct Pewaktu {
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

DHT dht(PIN_DHT, DHT11);
MatterTemperatureSensor sensorSuhu;     // endpoint sensor suhu
MatterHumiditySensor sensorKelembapan;  // endpoint sensor kelembapan
MatterButton tombolS1;
Pewaktu timerBaca = {10000, 0};  // baca DHT11 setiap 10 detik

bool sudahTerdaftar = false;
bool sedangReset = false;
unsigned long waktuReset = 0;

void sambungWiFi() {
  Serial.printf("[WiFi] Menyambung ke %s", NAMA_WIFI);
  WiFi.mode(WIFI_STA);
  WiFi.enableIPv6(true);  // Matter berjalan di atas IPv6
  WiFi.begin(NAMA_WIFI, PASSWORD_WIFI);
  unsigned long mulai = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - mulai >= 20000) {
      Serial.println();
      Serial.println("[WiFi] Belum tersambung. Cek nama/password dan pastikan WiFi 2,4 GHz.");
      mulai = millis();
    }
  }
  Serial.println();
  Serial.printf("[WiFi] Tersambung, IP: %s\n", WiFi.localIP().toString().c_str());
}

void tampilkanKodePairing() {
  Serial.println();
  Serial.println("[MATTER] Belum terdaftar. Daftarkan dari aplikasi rumah pintar:");
  Serial.printf("[MATTER] Kode pairing manual: %s\n", Matter.getManualPairingCode().c_str());
  Serial.println("[MATTER] Buka URL ini di browser untuk melihat kode QR:");
  Serial.println(Matter.getOnboardingQRCodeUrl());
  Serial.println();
}

void cekStatusMatter() {
  if (sedangReset) {
    if (millis() - waktuReset >= 10000) {
      ESP.restart();  // cadangan kalau belum restart sendiri
    }
    return;
  }

  bool terdaftar = Matter.isDeviceCommissioned();
  if (terdaftar && !sudahTerdaftar) {
    Serial.println("[MATTER] Berhasil didaftarkan ke aplikasi rumah pintar!");
  } else if (!terdaftar && sudahTerdaftar) {
    Serial.println("[MATTER] Perangkat dihapus dari aplikasi. Restart...");
    delay(500);
    ESP.restart();
  } else if (!terdaftar && millis() >= BATAS_TUNGGU_DAFTAR) {
    Serial.println("[MATTER] 10 menit belum didaftarkan. Restart supaya jendela pendaftaran terbuka lagi...");
    delay(500);
    ESP.restart();
  }
  sudahTerdaftar = terdaftar;
}

// Membaca DHT11 lalu mengirim hasilnya ke kedua endpoint Matter
void bacaDanKirim() {
  float suhu = dht.readTemperature();
  float kelembapan = dht.readHumidity();
  if (isnan(suhu) || isnan(kelembapan)) {
    Serial.println("[DHT11] Gagal membaca. Cek kabel DHT -> IO27.");
    return;  // nilai lama tetap dipakai di Matter
  }
  sensorSuhu.setTemperature(suhu);
  sensorKelembapan.setHumidity(kelembapan);
  Serial.printf("[DHT11] Suhu %.1f °C | Kelembapan %.0f %% -> dikirim ke Matter\n",
                suhu, kelembapan);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("=== BAB 56 - Program 2: Sensor Suhu & Kelembapan Matter ===");

  tombolS1.begin(PIN_TOMBOL_S1, INPUT);  // kit sudah punya pull-down
  tombolS1.setActiveLow(false);          // ditekan = HIGH
  dht.begin();

  sambungWiFi();
  delay(1000);  // beri waktu DHT11 siap setelah dinyalakan

  // Nilai awal untuk endpoint (harus ada sebelum Matter.begin())
  float suhuAwal = dht.readTemperature();
  float kelembapanAwal = dht.readHumidity();
  if (isnan(suhuAwal) || isnan(kelembapanAwal)) {
    Serial.println("[DHT11] Pembacaan awal gagal, sementara dipakai 0. Cek kabel DHT -> IO27.");
    suhuAwal = 0;
    kelembapanAwal = 0;
  } else {
    Serial.printf("[DHT11] Nilai awal: %.1f °C, %.0f %%\n", suhuAwal, kelembapanAwal);
  }

  sensorSuhu.begin(suhuAwal);              // endpoint 1: suhu
  sensorKelembapan.begin(kelembapanAwal);  // endpoint 2: kelembapan
  Matter.begin();                          // selalu terakhir

  sudahTerdaftar = Matter.isDeviceCommissioned();
  if (sudahTerdaftar) {
    Serial.println("[MATTER] Sudah terdaftar. Lihat suhu dan kelembapan di aplikasi.");
  } else {
    tampilkanKodePairing();
  }
}

void loop() {
  cekStatusMatter();

  if (!sedangReset && timerBaca.sudahWaktunya(millis())) {
    bacaDanKirim();
  }

  matterButtonEvent_t kejadian;
  while ((kejadian = tombolS1.poll()) != MATTER_BUTTON_NONE) {
    if (kejadian == MATTER_BUTTON_CLICK && !sedangReset) {
      Serial.println("[TOMBOL] S1 ditekan -> baca DHT11 sekarang");
      bacaDanKirim();
    } else if (kejadian == MATTER_BUTTON_LONG_HOLD && !sedangReset) {
      Serial.println("[TOMBOL] S1 ditahan 5 detik -> hapus pendaftaran Matter");
      sedangReset = true;
      waktuReset = millis();
      Matter.decommission();  // hapus data Matter, lalu ESP32 restart
    }
  }
  delay(10);
}
