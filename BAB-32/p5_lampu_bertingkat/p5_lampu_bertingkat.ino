/*
 * BAB 32 - Program 5: Lampu Malam Bertingkat
 * Kecerahan LED merah naik bertahap (5 tingkat) saat cahaya sekitar makin gelap.
 * Setiap batas tingkat diberi histeresis (MARGIN) supaya LED tidak
 * bolak-balik di antara dua tingkat. Tanpa delay().
 *
 * Rangkaian:
 *   LDR (header kuning, pin ke-1) → IO35
 *   LED1 (merah, header LED)      → IO4 (PWM)
 */

#define LDR_PIN   35  // LDR → IO35 (ADC1_CH7)
#define LED_MERAH 4   // LED1 merah → IO4

// Hasil kalibrasi dari Program 3 — ganti dengan angkamu
const int NILAI_GELAP = 3800;
const int NILAI_TERANG = 1000;
static_assert(NILAI_GELAP - NILAI_TERANG >= 300, "NILAI_GELAP harus lebih besar dari NILAI_TERANG minimal 300");

const int JUMLAH_TINGKAT = 5;
// Batas cahaya (%) antartingkat: tingkat 0 = cahaya >= 80 %, ..., tingkat 4 = cahaya < 20 %
const int BATAS_CAHAYA[JUMLAH_TINGKAT - 1] = {80, 60, 40, 20};
const int MARGIN = 3;  // histeresis: batas harus terlewati sejauh 3 % supaya tingkat berubah
const int KECERAHAN[JUMLAH_TINGKAT] = {0, 8, 32, 96, 255};  // nilai PWM untuk setiap tingkat
const char *NAMA_TINGKAT[JUMLAH_TINGKAT] = {"terang", "agak redup", "redup", "gelap", "sangat gelap"};

const int FREKUENSI_PWM = 5000;  // Hz
const int RESOLUSI_PWM = 8;      // 8 bit → nilai PWM 0-255
const int JUMLAH_SAMPEL = 20;    // 20 sampel × 5 ms = rata-rata 100 ms terakhir

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
Timer timerLaporan = {2000, 0};  // laporan setiap 2 detik
RataRataBergerak filterLdr;

int tingkat = 0;                      // tingkat yang sedang aktif
unsigned long waktuMasukTingkat = 0;  // kapan tingkat ini dimulai

// Persentase cahaya: 0 % = titik gelap, 100 % = titik terang
int hitungPersen(int nilai) {
  long persen = (long)(NILAI_GELAP - nilai) * 100 / (NILAI_GELAP - NILAI_TERANG);
  if (persen < 0) persen = 0;
  if (persen > 100) persen = 100;
  return persen;
}

// Tingkat tanpa histeresis (dipakai sekali saat program mulai)
int tingkatAwal(int persen) {
  int t = 0;
  while (t < JUMLAH_TINGKAT - 1 && persen < BATAS_CAHAYA[t]) {
    t = t + 1;
  }
  return t;
}

// Tingkat baru dengan histeresis, dihitung dari tingkat sekarang (t)
int tingkatBaru(int persen, int t) {
  // Makin gelap: pindah kalau cahaya di bawah batas bawah tingkat ini dikurangi MARGIN
  while (t < JUMLAH_TINGKAT - 1 && persen < BATAS_CAHAYA[t] - MARGIN) {
    t = t + 1;
  }
  // Makin terang: pindah kalau cahaya di atas batas atas tingkat ini ditambah MARGIN
  while (t > 0 && persen >= BATAS_CAHAYA[t - 1] + MARGIN) {
    t = t - 1;
  }
  return t;
}

void setup() {
  Serial.begin(115200);
  ledcAttach(LED_MERAH, FREKUENSI_PWM, RESOLUSI_PWM);  // siapkan PWM di IO4 (BAB 12)

  filterLdr.mulai(analogRead(LDR_PIN));  // isi semua kotak dengan bacaan pertama
  int persen = hitungPersen(filterLdr.nilai());
  tingkat = tingkatAwal(persen);
  waktuMasukTingkat = millis();
  ledcWrite(LED_MERAH, KECERAHAN[tingkat]);

  Serial.println("BAB 32 - Program 5: Lampu Malam Bertingkat");
  Serial.printf("[Mulai] Cahaya %d %% -> tingkat %d (%s), PWM %d\n",
                persen, tingkat, NAMA_TINGKAT[tingkat], KECERAHAN[tingkat]);
}

void loop() {
  unsigned long sekarang = millis();

  // Tugas 1: ambil satu sampel LDR setiap 5 ms
  if (timerSampel.sudahWaktunya(sekarang)) {
    filterLdr.tambah(analogRead(LDR_PIN));
  }

  // Tugas 2: periksa apakah tingkat cahaya berubah
  int persen = hitungPersen(filterLdr.nilai());
  int t = tingkatBaru(persen, tingkat);
  if (t != tingkat) {
    float lamaDetik = (sekarang - waktuMasukTingkat) / 1000.0;
    ledcWrite(LED_MERAH, KECERAHAN[t]);
    Serial.printf("[%6.1f s] Tingkat %d -> %d (%s) | cahaya %d %% | PWM %d | %.1f s di tingkat %d\n",
                  sekarang / 1000.0, tingkat, t, NAMA_TINGKAT[t], persen, KECERAHAN[t],
                  lamaDetik, tingkat);
    tingkat = t;
    waktuMasukTingkat = sekarang;
  }

  // Tugas 3: laporan setiap 2 detik
  if (timerLaporan.sudahWaktunya(sekarang)) {
    Serial.printf("Cahaya: %3d %% | Tingkat: %d (%s) | PWM LED: %d\n",
                  persen, tingkat, NAMA_TINGKAT[tingkat], KECERAHAN[tingkat]);
  }
}
