/*
 * BAB 37 - Program 4: Termostat Kipas dengan DHT11
 * Relay (misalnya untuk kipas) aktif kalau suhu >= SUHU_NYALA dan baru mati
 * kalau suhu <= SUHU_MATI (histeresis). Setiap keadaan juga harus bertahan
 * minimal LAMA_MIN_AKTIF / LAMA_MIN_MATI sebelum relay boleh berganti lagi.
 * Kalau DHT11 gagal dibaca BATAS_GAGAL kali berturut-turut, relay dimatikan.
 *
 * Rangkaian:
 *   RELAY (header biru, kanan BUZZ)        → IO26
 *   DHT (header hijau, pin ke-3 dari atas) → IO27 (pull-up 4,7 kΩ sudah ada di PCB)
 *
 * Library: DHT sensor library by Adafruit (+ Adafruit Unified Sensor)
 */

#include <DHT.h>

#define RELAY_PIN 26
#define DHT_PIN   27
#define DHT_TYPE  DHT11

// ← Sesuaikan kedua batas ini dengan suhu ruanganmu (lihat langkah 6)
const float SUHU_NYALA = 31.0;  // °C: relay aktif kalau suhu >= angka ini
const float SUHU_MATI  = 29.0;  // °C: relay mati kalau suhu <= angka ini

const unsigned long LAMA_MIN_AKTIF = 10000;  // ms: setelah aktif, minimal aktif selama ini
const unsigned long LAMA_MIN_MATI  = 10000;  // ms: setelah mati, minimal mati selama ini
const unsigned long INTERVAL_BACA  = 2000;   // ms: DHT11 paling cepat dibaca setiap 2 detik
const int BATAS_GAGAL = 3;                   // gagal berturut-turut sebelum relay dimatikan

// Pola timer dari BAB 16 (tipe ditulis di atas, sebelum fungsi mana pun)
struct Timer {
  unsigned long interval;
  unsigned long waktuLalu;
  bool sudahWaktunya(unsigned long sekarang) {
    if (sekarang - waktuLalu >= interval) {
      waktuLalu = sekarang;
      return true;
    }
    return false;
  }
};

DHT dht(DHT_PIN, DHT_TYPE);
Timer timerBaca = {INTERVAL_BACA, 0};

bool relayAktif = false;
unsigned long waktuRelayBerubah = 0;  // kapan relay terakhir berganti keadaan
int jumlahGagal = 0;                  // berapa kali DHT11 gagal dibaca berturut-turut

void aturRelay(bool aktif, unsigned long sekarang) {
  relayAktif = aktif;
  digitalWrite(RELAY_PIN, aktif ? HIGH : LOW);
  waktuRelayBerubah = sekarang;
}

void periksaSuhu(unsigned long sekarang) {
  float suhu = dht.readTemperature();

  // 1. Bacaan gagal? Setelah BATAS_GAGAL kali berturut-turut, matikan relay.
  if (isnan(suhu)) {
    jumlahGagal = jumlahGagal + 1;
    Serial.printf("Gagal membaca DHT11 (%d kali berturut-turut)\n", jumlahGagal);
    if (jumlahGagal >= BATAS_GAGAL && relayAktif) {
      aturRelay(false, sekarang);
      Serial.println(">> Relay OFF demi keamanan: suhu tidak diketahui");
    }
    return;
  }
  jumlahGagal = 0;

  // 2. Histeresis: tentukan keadaan yang diinginkan
  bool inginAktif = relayAktif;  // di antara dua batas: pertahankan keadaan
  if (suhu >= SUHU_NYALA) inginAktif = true;
  if (suhu <= SUHU_MATI) inginAktif = false;

  Serial.printf("Suhu: %.1f °C | Relay: %s", suhu, relayAktif ? "ON" : "OFF");

  // 3. Waktu minimum: ganti keadaan hanya kalau keadaan sekarang sudah cukup lama
  if (inginAktif != relayAktif) {
    unsigned long lamaKeadaan = sekarang - waktuRelayBerubah;
    unsigned long lamaMinimum = relayAktif ? LAMA_MIN_AKTIF : LAMA_MIN_MATI;
    if (lamaKeadaan >= lamaMinimum) {
      aturRelay(inginAktif, sekarang);
      Serial.printf(" -> %s\n", relayAktif ? "ON" : "OFF");
    } else {
      unsigned long sisa = (lamaMinimum - lamaKeadaan + 999) / 1000;  // dibulatkan ke atas
      Serial.printf(" (ingin %s, tunggu %lu detik lagi)\n", inginAktif ? "ON" : "OFF", sisa);
    }
  } else {
    Serial.println();
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  aturRelay(false, millis());  // relay mati; waktu mati minimum mulai dihitung dari sini
  dht.begin();

  Serial.println("BAB 37 - Program 4: Termostat Kipas dengan DHT11");
  Serial.printf("Relay ON kalau suhu >= %.1f °C, OFF kalau suhu <= %.1f °C\n",
                SUHU_NYALA, SUHU_MATI);
  Serial.printf("Waktu minimum: ON %lu detik, OFF %lu detik\n",
                LAMA_MIN_AKTIF / 1000, LAMA_MIN_MATI / 1000);
}

void loop() {
  unsigned long sekarang = millis();

  if (timerBaca.sudahWaktunya(sekarang)) {
    periksaSuhu(sekarang);
  }

  // Tugas lain (tombol, layar, WiFi) bisa ditambahkan di sini tanpa terganggu
}
