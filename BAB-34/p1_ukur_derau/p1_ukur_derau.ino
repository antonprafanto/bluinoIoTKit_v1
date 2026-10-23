/*
 * BAB 34 - Program 1: Mengukur Derau Sensor
 * Membaca potensiometer dan LDR setiap 5 ms tanpa delay(), lalu setiap
 * 200 sampel (1 detik) mencetak ringkasan derau: rata-rata, minimum,
 * maksimum, rentang, dan simpangan baku.
 *
 * Rangkaian: VR10K (potensiometer) → IO34, LDR → IO35
 */

#define PIN_POT 34  // potensiometer (VR10K) → IO34, ADC1
#define PIN_LDR 35  // LDR → IO35, ADC1

const int JUMLAH_SAMPEL = 200;  // 200 sampel × 5 ms = satu ringkasan per detik

// Timer serbaguna dari BAB 16
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

// Penampung statistik untuk satu sensor
struct Statistik {
  uint32_t jumlah;         // total nilai: paling besar 200 × 4095 = 819.000
  uint64_t jumlahKuadrat;  // total kuadrat nilai: bisa lebih dari 3 miliar
  int minimum;
  int maksimum;

  void mulaiUlang() {
    jumlah = 0;
    jumlahKuadrat = 0;
    minimum = 4095;  // sengaja diisi nilai terbesar supaya sampel pertama pasti lebih kecil
    maksimum = 0;    // sengaja diisi nilai terkecil supaya sampel pertama pasti lebih besar
  }

  void tambah(int nilai) {
    jumlah = jumlah + nilai;
    jumlahKuadrat = jumlahKuadrat + (uint64_t)nilai * nilai;
    if (nilai < minimum) minimum = nilai;
    if (nilai > maksimum) maksimum = nilai;
  }

  void cetak(const char *nama, int n) {
    double rataRata = (double)jumlah / n;
    double varian = (double)jumlahKuadrat / n - rataRata * rataRata;
    if (varian < 0) varian = 0;  // jaga-jaga galat pembulatan
    Serial.printf("%s: rata-rata %7.1f | min %4d | maks %4d | rentang %4d | simpangan baku %5.1f\n",
                  nama, rataRata, minimum, maksimum, maksimum - minimum, sqrt(varian));
  }
};

Timer timerSampel = {5, 0};  // ambil satu sampel setiap 5 ms
Statistik statPot;
Statistik statLdr;
int banyakSampel = 0;

void setup() {
  Serial.begin(115200);
  statPot.mulaiUlang();
  statLdr.mulaiUlang();
  Serial.println("BAB 34 - Program 1: Mengukur Derau Sensor");
  Serial.println("Biarkan kenop dan LDR diam. Ringkasan muncul setiap 1 detik.");
}

void loop() {
  unsigned long sekarang = millis();

  if (timerSampel.sudahWaktunya(sekarang)) {
    statPot.tambah(analogRead(PIN_POT));
    statLdr.tambah(analogRead(PIN_LDR));
    banyakSampel = banyakSampel + 1;

    if (banyakSampel == JUMLAH_SAMPEL) {
      statPot.cetak("Pot", banyakSampel);
      statLdr.cetak("LDR", banyakSampel);
      Serial.println();
      statPot.mulaiUlang();  // mulai menghitung ringkasan berikutnya dari nol
      statLdr.mulaiUlang();
      banyakSampel = 0;
    }
  }
}
