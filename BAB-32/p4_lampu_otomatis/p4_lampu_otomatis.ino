/*
 * BAB 32 - Program 4: Lampu Otomatis dengan Histeresis dan Durasi Minimum
 * LED merah (dan relay, opsional) menyala saat gelap dan mati saat terang.
 *   - Histeresis     : nyala kalau cahaya di bawah 30 %, baru mati di atas 40 %
 *   - Durasi minimum : keadaan baru harus bertahan 3 detik sebelum lampu berubah
 *   - Rata-rata bergerak 20 sampel × 5 ms; semuanya tanpa delay()
 *
 * Rangkaian:
 *   LDR (header kuning, pin ke-1)   → IO35
 *   LED1 (merah, header LED)        → IO4
 *   RELAY (header biru, kanan BUZZ) → IO26 (opsional)
 */

#define LDR_PIN   35  // LDR → IO35 (ADC1_CH7)
#define LED_MERAH 4   // LED1 merah → IO4 (aktif HIGH)
#define RELAY_PIN 26  // RELAY → IO26 (aktif HIGH, lewat transistor BC547)

// Hasil kalibrasi dari Program 3 — ganti dengan angkamu
const int NILAI_GELAP = 3800;
const int NILAI_TERANG = 1000;

const int BATAS_NYALA = 30;                  // % cahaya: di bawah ini dianggap gelap
const int BATAS_MATI = 40;                   // % cahaya: di atas ini dianggap terang
const unsigned long LAMA_KONFIRMASI = 3000;  // ms: keadaan baru harus bertahan selama ini
const int JUMLAH_SAMPEL = 20;                // 20 sampel × 5 ms = rata-rata 100 ms terakhir

// Pemeriksaan saat compile: kalau angka di atas tidak masuk akal, compile gagal dengan pesan ini
static_assert(NILAI_GELAP - NILAI_TERANG >= 300, "NILAI_GELAP harus lebih besar dari NILAI_TERANG minimal 300");
static_assert(BATAS_NYALA < BATAS_MATI, "BATAS_NYALA harus lebih kecil dari BATAS_MATI");

enum StateLampu { TERANG, CALON_GELAP, GELAP, CALON_TERANG };

// Timer serbaguna dari BAB 16
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

// Rata-rata bergerak dengan kotak melingkar (lihat 32.7.2)
struct RataRataBergerak {
  int sampel[JUMLAH_SAMPEL];
  int posisi;
  long jumlah;

  void mulai(int nilaiAwal) {
    for (int i = 0; i < JUMLAH_SAMPEL; i++) {
      sampel[i] = nilaiAwal;
    }
    jumlah = (long)nilaiAwal * JUMLAH_SAMPEL;
    posisi = 0;
  }

  void tambah(int nilaiBaru) {
    jumlah = jumlah - sampel[posisi] + nilaiBaru;
    sampel[posisi] = nilaiBaru;
    posisi = (posisi + 1) % JUMLAH_SAMPEL;
  }

  int nilai() {
    return jumlah / JUMLAH_SAMPEL;
  }
};

Timer timerSampel = {5, 0};      // ambil sampel setiap 5 ms
Timer timerLaporan = {1000, 0};  // laporan setiap 1 detik
RataRataBergerak filterLdr;

StateLampu state = TERANG;
unsigned long waktuMasukState = 0;  // kapan state sekarang dimulai

const char *namaState(StateLampu s) {
  switch (s) {
    case TERANG:       return "TERANG";
    case CALON_GELAP:  return "CALON_GELAP";
    case GELAP:        return "GELAP";
    case CALON_TERANG: return "CALON_TERANG";
  }
  return "?";
}

// Persentase cahaya: 0 % = titik gelap, 100 % = titik terang
int hitungPersen(int nilai) {
  long persen = (long)(NILAI_GELAP - nilai) * 100 / (NILAI_GELAP - NILAI_TERANG);
  if (persen < 0) persen = 0;
  if (persen > 100) persen = 100;
  return persen;
}

void aturLampu(bool nyala) {
  digitalWrite(LED_MERAH, nyala ? HIGH : LOW);
  digitalWrite(RELAY_PIN, nyala ? HIGH : LOW);
}

// Lampu menyala di state GELAP dan selama menunggu konfirmasi terang
bool lampuNyala() {
  return state == GELAP || state == CALON_TERANG;
}

void pindahState(StateLampu stateBaru, int persen, const char *alasan) {
  Serial.printf("[%6.1f s] %s -> %s (cahaya %d %%: %s)\n",
                millis() / 1000.0, namaState(state), namaState(stateBaru), persen, alasan);
  state = stateBaru;
  waktuMasukState = millis();

  // Aksi masuk: lampu hanya berubah saat masuk GELAP atau TERANG
  if (state == GELAP) aturLampu(true);
  if (state == TERANG) aturLampu(false);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(RELAY_PIN, OUTPUT);

  filterLdr.mulai(analogRead(LDR_PIN));  // isi semua kotak dengan bacaan pertama
  int persen = hitungPersen(filterLdr.nilai());

  Serial.println("BAB 32 - Program 4: Lampu Otomatis dengan Histeresis dan Durasi Minimum");
  Serial.printf("Nyala di bawah %d %% | Mati di atas %d %% | Konfirmasi %.1f detik\n",
                BATAS_NYALA, BATAS_MATI, LAMA_KONFIRMASI / 1000.0);

  // State awal langsung mengikuti cahaya saat ini, tanpa menunggu konfirmasi
  state = (persen < BATAS_NYALA) ? GELAP : TERANG;
  waktuMasukState = millis();
  aturLampu(lampuNyala());
  Serial.printf("[Mulai] Cahaya %d %% -> state %s, lampu %s\n",
                persen, namaState(state), lampuNyala() ? "NYALA" : "MATI");
}

void loop() {
  unsigned long sekarang = millis();

  // Tugas 1: ambil satu sampel LDR setiap 5 ms
  if (timerSampel.sudahWaktunya(sekarang)) {
    filterLdr.tambah(analogRead(LDR_PIN));
  }

  // Tugas 2: state machine
  int persen = hitungPersen(filterLdr.nilai());
  unsigned long lamaDiState = sekarang - waktuMasukState;

  switch (state) {
    case TERANG:  // lampu mati
      if (persen < BATAS_NYALA) {
        pindahState(CALON_GELAP, persen, "mulai gelap, tunggu konfirmasi");
      }
      break;

    case CALON_GELAP:  // lampu masih mati, menunggu konfirmasi
      if (persen >= BATAS_NYALA) {
        pindahState(TERANG, persen, "batal, sudah terang lagi");
      } else if (lamaDiState >= LAMA_KONFIRMASI) {
        pindahState(GELAP, persen, "gelap bertahan, lampu NYALA");
      }
      break;

    case GELAP:  // lampu nyala
      if (persen > BATAS_MATI) {
        pindahState(CALON_TERANG, persen, "mulai terang, tunggu konfirmasi");
      }
      break;

    case CALON_TERANG:  // lampu masih nyala, menunggu konfirmasi
      if (persen <= BATAS_MATI) {
        pindahState(GELAP, persen, "batal, sudah gelap lagi");
      } else if (lamaDiState >= LAMA_KONFIRMASI) {
        pindahState(TERANG, persen, "terang bertahan, lampu MATI");
      }
      break;
  }

  // Tugas 3: laporan setiap 1 detik
  if (timerLaporan.sudahWaktunya(sekarang)) {
    Serial.printf("Cahaya: %3d %% | State: %-12s | Lampu: %s\n",
                  persen, namaState(state), lampuNyala() ? "NYALA" : "MATI");
  }
}
