/*
 * BAB 37 - Program 2: Sakelar Relay dengan Tombol S1
 * Tekan S1 sekali → relay aktif, tekan lagi → relay mati.
 * Pengaman:
 *   - relay mati sendiri setelah BATAS_AKTIF (30 detik)
 *   - tekanan yang datang kurang dari JEDA_MIN setelah pergantian terakhir diabaikan
 *
 * Rangkaian:
 *   RELAY (header biru, kanan BUZZ)         → IO26
 *   SW1 (header kuning, pin ke-3 dari kiri) → IO15 (pull-down di PCB, ditekan = HIGH)
 */

#define RELAY_PIN 26
#define TOMBOL_S1 15

const unsigned long WAKTU_DEBOUNCE = 50;     // ms: bacaan tombol harus stabil selama ini
const unsigned long JEDA_MIN       = 1000;   // ms: jarak minimal antara dua pergantian relay
const unsigned long BATAS_AKTIF    = 30000;  // ms: relay mati sendiri setelah 30 detik
const unsigned long INTERVAL_SISA  = 10000;  // ms: laporkan sisa waktu setiap 10 detik

// Variabel debounce (pola dari BAB 9)
int bacaanSebelumnya = LOW;
int statusStabil = LOW;
unsigned long waktuBacaanBerubah = 0;

// Keadaan relay
bool relayAktif = false;
unsigned long waktuRelayBerubah = 0;  // kapan relay terakhir berganti keadaan
unsigned long waktuLaporanLalu = 0;   // kapan sisa waktu terakhir dilaporkan

// true satu kali saja, tepat saat S1 baru ditekan (sudah lolos debounce)
bool tombolBaruDitekan(unsigned long sekarang) {
  int bacaan = digitalRead(TOMBOL_S1);
  if (bacaan != bacaanSebelumnya) {
    waktuBacaanBerubah = sekarang;
    bacaanSebelumnya = bacaan;
  }
  if (sekarang - waktuBacaanBerubah > WAKTU_DEBOUNCE && bacaan != statusStabil) {
    statusStabil = bacaan;
    return statusStabil == HIGH;  // hanya tepi naik (LOW → HIGH) yang dihitung
  }
  return false;
}

// Satu-satunya tempat yang menyentuh pin relay
void aturRelay(bool aktif, unsigned long sekarang, const char *alasan) {
  relayAktif = aktif;
  digitalWrite(RELAY_PIN, aktif ? HIGH : LOW);
  waktuRelayBerubah = sekarang;
  waktuLaporanLalu = sekarang;
  Serial.printf("Relay %s (%s)\n", aktif ? "ON" : "OFF", alasan);
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // relay mati sejak awal
  pinMode(TOMBOL_S1, INPUT);     // tombol kit: pull-down 10 kΩ sudah ada di PCB

  Serial.println("BAB 37 - Program 2: Sakelar Relay dengan Tombol S1");
  Serial.println("Tekan S1 untuk ON/OFF. Relay mati sendiri setelah 30 detik.");
}

void loop() {
  unsigned long sekarang = millis();  // satu nilai waktu untuk seluruh putaran

  // 1. S1 baru ditekan → balik keadaan relay, kecuali terlalu cepat
  if (tombolBaruDitekan(sekarang)) {
    if (sekarang - waktuRelayBerubah < JEDA_MIN) {
      Serial.println("Terlalu cepat! Tunggu sebentar, lalu tekan lagi.");
    } else {
      aturRelay(!relayAktif, sekarang, "tombol S1");
    }
  }

  // 2. Pengaman: relay sudah aktif terlalu lama → matikan sendiri
  if (relayAktif) {
    unsigned long lamaAktif = sekarang - waktuRelayBerubah;
    if (lamaAktif >= BATAS_AKTIF) {
      aturRelay(false, sekarang, "batas waktu habis");
    } else if (sekarang - waktuLaporanLalu >= INTERVAL_SISA) {
      waktuLaporanLalu = sekarang;
      Serial.printf("  Sisa waktu: %lu detik\n", (BATAS_AKTIF - lamaAktif + 500) / 1000);
    }
  }
}
