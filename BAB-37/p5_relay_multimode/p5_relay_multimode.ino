/*
 * BAB 37 - Program 5: Relay Multi-Mode dengan State Machine
 * Empat mode kerja relay:
 *   MANUAL → tekan S1 sebentar untuk ON/OFF
 *   JADWAL → relay ON 5 detik, OFF 10 detik, berulang
 *   PIR    → relay ON saat ada gerakan, OFF 10 detik setelah gerakan terakhir
 *   KUNCI  → relay dikunci OFF
 * Tahan S1 selama 1 detik untuk pindah ke mode berikutnya (relay dimatikan dulu).
 *
 * Pengaman untuk semua mode:
 *   - relay harus mati minimal 2 detik sebelum boleh aktif lagi
 *   - relay yang aktif lebih dari 1 menit dimatikan, lalu sistem masuk mode KUNCI
 *
 * Rangkaian:
 *   RELAY (header biru, kanan BUZZ)         → IO26
 *   SW1 (header kuning, pin ke-3 dari kiri) → IO15 (ditekan = HIGH)
 *   PIR (header hijau, pin ke-5 dari atas)  → IO33 (HIGH saat ada gerakan)
 */

#define RELAY_PIN 26
#define TOMBOL_S1 15
#define PIR_PIN   33

const unsigned long WAKTU_DEBOUNCE     = 50;     // ms
const unsigned long LAMA_TEKAN_PANJANG = 1000;   // ms: ditahan selama ini = pindah mode
const unsigned long LAMA_MIN_MATI      = 2000;   // ms: relay harus mati minimal selama ini
const unsigned long BATAS_AKTIF        = 60000;  // ms: relay aktif lebih lama → dimatikan
const unsigned long JADWAL_AKTIF       = 5000;   // ms: mode JADWAL, lama relay aktif
const unsigned long JADWAL_MATI        = 10000;  // ms: mode JADWAL, lama relay mati
const unsigned long PIR_TAHAN          = 10000;  // ms: mode PIR, lama relay aktif setelah gerakan terakhir

// Tipe enum ditulis di atas, sebelum fungsi mana pun
enum JenisTekan { TIDAK_ADA, TEKAN_PENDEK, TEKAN_PANJANG };
enum Mode { MODE_MANUAL, MODE_JADWAL, MODE_PIR, MODE_KUNCI, JUMLAH_MODE };

const char *NAMA_MODE[JUMLAH_MODE] = {"MANUAL", "JADWAL", "PIR", "KUNCI"};

Mode mode = MODE_MANUAL;

bool relayAktif = false;
unsigned long waktuRelayBerubah = 0;   // kapan relay terakhir berganti keadaan
unsigned long waktuJadwal = 0;         // mode JADWAL: kapan fase sekarang dimulai
unsigned long waktuGerakTerakhir = 0;  // mode PIR: kapan gerakan terakhir terdeteksi

// Variabel tombol
int bacaanSebelumnya = LOW;
int statusStabil = LOW;
unsigned long waktuBacaanBerubah = 0;
unsigned long waktuMulaiTekan = 0;
bool sudahTekanPanjang = false;

// Tekan pendek dilaporkan saat tombol DILEPAS (sebelum 1 detik).
// Tekan panjang dilaporkan begitu tombol sudah ditahan 1 detik, tanpa menunggu dilepas.
JenisTekan bacaTombol(unsigned long sekarang) {
  int bacaan = digitalRead(TOMBOL_S1);
  if (bacaan != bacaanSebelumnya) {
    waktuBacaanBerubah = sekarang;
    bacaanSebelumnya = bacaan;
  }
  if (sekarang - waktuBacaanBerubah > WAKTU_DEBOUNCE && bacaan != statusStabil) {
    statusStabil = bacaan;
    if (statusStabil == HIGH) {       // baru ditekan: mulai hitung lamanya
      waktuMulaiTekan = sekarang;
      sudahTekanPanjang = false;
    } else if (!sudahTekanPanjang) {  // baru dilepas sebelum menjadi tekan panjang
      return TEKAN_PENDEK;
    }
  }
  if (statusStabil == HIGH && !sudahTekanPanjang &&
      sekarang - waktuMulaiTekan >= LAMA_TEKAN_PANJANG) {
    sudahTekanPanjang = true;         // dilaporkan sekali saja per tekanan
    return TEKAN_PANJANG;
  }
  return TIDAK_ADA;
}

void cetakMode() {
  Serial.printf("[%s] ", NAMA_MODE[mode]);
}

// Satu-satunya tempat yang menyentuh pin relay. Mengembalikan false kalau permintaan ditolak.
bool aturRelay(bool aktif, unsigned long sekarang, const char *alasan) {
  if (aktif == relayAktif) {
    return true;  // keadaannya sudah sesuai, tidak ada yang perlu diubah
  }
  if (aktif && sekarang - waktuRelayBerubah < LAMA_MIN_MATI) {
    return false;  // baru saja mati: belum boleh aktif lagi
  }
  relayAktif = aktif;
  digitalWrite(RELAY_PIN, aktif ? HIGH : LOW);
  waktuRelayBerubah = sekarang;
  cetakMode();
  Serial.printf("Relay %s (%s)\n", aktif ? "ON" : "OFF", alasan);
  return true;
}

void tampilkanInfoMode() {
  cetakMode();
  switch (mode) {
    case MODE_MANUAL:
      Serial.println("Tekan S1 sebentar untuk ON/OFF.");
      break;
    case MODE_JADWAL:
      Serial.printf("Otomatis: ON %lu detik, OFF %lu detik, berulang.\n",
                    JADWAL_AKTIF / 1000, JADWAL_MATI / 1000);
      break;
    case MODE_PIR:
      Serial.printf("Relay ON saat ada gerakan, OFF %lu detik setelah gerakan terakhir.\n",
                    PIR_TAHAN / 1000);
      break;
    case MODE_KUNCI:
      Serial.println("Relay dikunci OFF. Tahan S1 1 detik untuk pindah mode.");
      break;
    default:
      break;
  }
}

// Pindah mode: relay SELALU dimatikan dulu, supaya mode baru mulai dari keadaan aman
void masukMode(Mode baru, unsigned long sekarang) {
  aturRelay(false, sekarang, "ganti mode");
  mode = baru;
  waktuJadwal = sekarang;  // mode JADWAL selalu mulai dari fase mati
  Serial.println();
  tampilkanInfoMode();
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // relay mati sejak awal
  pinMode(TOMBOL_S1, INPUT);
  pinMode(PIR_PIN, INPUT);

  Serial.println("BAB 37 - Program 5: Relay Multi-Mode dengan State Machine");
  Serial.println("Tahan S1 1 detik = pindah mode. Tekan S1 sebentar = ON/OFF di mode MANUAL.");
  tampilkanInfoMode();
}

void loop() {
  unsigned long sekarang = millis();
  JenisTekan tekan = bacaTombol(sekarang);

  // 1. S1 ditahan → pindah ke mode berikutnya (setelah KUNCI kembali ke MANUAL)
  if (tekan == TEKAN_PANJANG) {
    masukMode((Mode)((mode + 1) % JUMLAH_MODE), sekarang);
  }

  // 2. Jalankan perilaku mode yang sedang aktif
  switch (mode) {
    case MODE_MANUAL:
      if (tekan == TEKAN_PENDEK) {
        bool berhasil = aturRelay(!relayAktif, sekarang, "tombol S1");
        if (!berhasil) {
          cetakMode();
          Serial.println("Relay baru saja mati, tunggu 2 detik.");
        }
      }
      break;

    case MODE_JADWAL: {
      unsigned long lamaFase = relayAktif ? JADWAL_AKTIF : JADWAL_MATI;
      if (sekarang - waktuJadwal >= lamaFase) {
        waktuJadwal = sekarang;
        aturRelay(!relayAktif, sekarang, "jadwal");
      }
      break;
    }

    case MODE_PIR:
      if (digitalRead(PIR_PIN) == HIGH) {
        waktuGerakTerakhir = sekarang;
        aturRelay(true, sekarang, "ada gerakan");  // kalau ditolak, dicoba lagi di putaran berikutnya
      } else if (relayAktif && sekarang - waktuGerakTerakhir >= PIR_TAHAN) {
        aturRelay(false, sekarang, "tidak ada gerakan");
      }
      break;

    case MODE_KUNCI:
      if (tekan == TEKAN_PENDEK) {
        cetakMode();
        Serial.println("Relay terkunci. Tahan S1 1 detik untuk pindah mode.");
      }
      break;

    default:
      break;
  }

  // 3. Pengaman untuk semua mode: relay aktif terlalu lama → matikan, lalu kunci
  if (relayAktif && sekarang - waktuRelayBerubah >= BATAS_AKTIF) {
    aturRelay(false, sekarang, "batas waktu aktif habis");
    masukMode(MODE_KUNCI, sekarang);
  }
}
