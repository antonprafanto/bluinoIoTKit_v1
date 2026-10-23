/*
 * BAB 37 - Program 3: Penjadwal Siklus Otomatis
 * Contoh penyiram tanaman. Tekan S1 untuk mulai: relay aktif 5 detik
 * (menyiram), mati 10 detik (jeda), diulang 3 kali, lalu berhenti sendiri.
 * Tekan S1 saat jadwal berjalan untuk membatalkan.
 *
 * Rangkaian:
 *   RELAY (header biru, kanan BUZZ)         → IO26
 *   SW1 (header kuning, pin ke-3 dari kiri) → IO15 (ditekan = HIGH)
 */

#define RELAY_PIN 26
#define TOMBOL_S1 15

const unsigned long LAMA_SIRAM = 5000;   // ms: relay aktif (pompa menyala)
const unsigned long LAMA_JEDA  = 10000;  // ms: relay mati di antara dua siraman
const int JUMLAH_SIKLUS = 3;             // berapa kali menyiram
const unsigned long WAKTU_DEBOUNCE = 50;

// Tipe enum ditulis di atas, sebelum fungsi mana pun (lihat BAB 17)
enum Keadaan { SIAGA, SIRAM, JEDA };

Keadaan keadaan = SIAGA;
unsigned long waktuMasuk = 0;  // kapan state sekarang dimulai
int siklusKe = 0;              // siklus yang sedang berjalan (1..JUMLAH_SIKLUS)

int bacaanSebelumnya = LOW;
int statusStabil = LOW;
unsigned long waktuBacaanBerubah = 0;

bool tombolBaruDitekan(unsigned long sekarang) {
  int bacaan = digitalRead(TOMBOL_S1);
  if (bacaan != bacaanSebelumnya) {
    waktuBacaanBerubah = sekarang;
    bacaanSebelumnya = bacaan;
  }
  if (sekarang - waktuBacaanBerubah > WAKTU_DEBOUNCE && bacaan != statusStabil) {
    statusStabil = bacaan;
    return statusStabil == HIGH;
  }
  return false;
}

// Pindah state. Relay HANYA aktif di state SIRAM, jadi pin relay cukup diatur di sini.
void masukKeadaan(Keadaan baru, unsigned long sekarang) {
  keadaan = baru;
  waktuMasuk = sekarang;
  digitalWrite(RELAY_PIN, baru == SIRAM ? HIGH : LOW);
}

void cetakWaktu(unsigned long sekarang) {
  Serial.printf("[%5.1f s] ", sekarang / 1000.0);
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // relay mati sejak awal
  pinMode(TOMBOL_S1, INPUT);

  Serial.println("BAB 37 - Program 3: Penjadwal Siklus Otomatis");
  Serial.printf("Jadwal: relay ON %lu detik, OFF %lu detik, %d siklus.\n",
                LAMA_SIRAM / 1000, LAMA_JEDA / 1000, JUMLAH_SIKLUS);
  Serial.println("Tekan S1 untuk mulai, tekan lagi untuk membatalkan.");
}

void loop() {
  unsigned long sekarang = millis();
  bool ditekan = tombolBaruDitekan(sekarang);

  switch (keadaan) {
    case SIAGA:
      if (ditekan) {
        siklusKe = 1;
        masukKeadaan(SIRAM, sekarang);
        cetakWaktu(sekarang);
        Serial.printf("Mulai! Siklus 1/%d: relay ON (menyiram)\n", JUMLAH_SIKLUS);
      }
      break;

    case SIRAM:
      if (ditekan) {
        masukKeadaan(SIAGA, sekarang);
        cetakWaktu(sekarang);
        Serial.println("Dibatalkan dengan S1. Relay OFF.");
      } else if (sekarang - waktuMasuk >= LAMA_SIRAM) {
        if (siklusKe >= JUMLAH_SIKLUS) {
          masukKeadaan(SIAGA, sekarang);
          cetakWaktu(sekarang);
          Serial.println("Relay OFF. Jadwal selesai. Tekan S1 untuk mulai lagi.");
        } else {
          masukKeadaan(JEDA, sekarang);
          cetakWaktu(sekarang);
          Serial.printf("Siklus %d/%d: relay OFF (jeda %lu detik)\n",
                        siklusKe, JUMLAH_SIKLUS, LAMA_JEDA / 1000);
        }
      }
      break;

    case JEDA:
      if (ditekan) {
        masukKeadaan(SIAGA, sekarang);
        cetakWaktu(sekarang);
        Serial.println("Dibatalkan dengan S1. Relay OFF.");
      } else if (sekarang - waktuMasuk >= LAMA_JEDA) {
        siklusKe = siklusKe + 1;
        masukKeadaan(SIRAM, sekarang);
        cetakWaktu(sekarang);
        Serial.printf("Siklus %d/%d: relay ON (menyiram)\n", siklusKe, JUMLAH_SIKLUS);
      }
      break;
  }
}
