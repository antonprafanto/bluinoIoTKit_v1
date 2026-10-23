/*
 * BAB 34 - Program 2: Membandingkan SMA, WMA, dan EMA
 * Membaca potensiometer setiap 20 ms tanpa delay(), lalu mengirim
 * nilai mentah dan hasil tiga filter ke Serial Plotter.
 * Ketiga filter disetel "setara 16 sampel" supaya adil dibandingkan.
 * Buka Tools → Serial Plotter, lalu pilih baud 115200.
 *
 * Rangkaian: VR10K (potensiometer) → IO34
 */

#define PIN_POT 34  // potensiometer (VR10K) → IO34

const int  N_SMA = 16;  // ukuran jendela SMA
const int  N_WMA = 16;  // ukuran jendela WMA
const long TOTAL_BOBOT = (long)N_WMA * (N_WMA + 1) / 2;  // 1 + 2 + ... + 16 = 136

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

// SMA: rata-rata N sampel terakhir dengan buffer melingkar
struct FilterSMA {
  int  sampel[N_SMA];
  int  posisi;  // kursi yang akan ditimpa berikutnya
  long jumlah;  // jumlah isi semua kursi

  void mulai(int nilaiAwal) {
    for (int i = 0; i < N_SMA; i++) {
      sampel[i] = nilaiAwal;
    }
    jumlah = (long)nilaiAwal * N_SMA;
    posisi = 0;
  }

  float tambah(int nilaiBaru) {
    jumlah = jumlah - sampel[posisi] + nilaiBaru;
    sampel[posisi] = nilaiBaru;
    posisi = (posisi + 1) % N_SMA;
    return (float)jumlah / N_SMA;
  }
};

// WMA: sampel terbaru berbobot N, sampel paling lama berbobot 1
struct FilterWMA {
  int sampel[N_WMA];
  int posisi;

  void mulai(int nilaiAwal) {
    for (int i = 0; i < N_WMA; i++) {
      sampel[i] = nilaiAwal;
    }
    posisi = 0;
  }

  float tambah(int nilaiBaru) {
    sampel[posisi] = nilaiBaru;
    posisi = (posisi + 1) % N_WMA;

    long jumlahBerbobot = 0;
    for (int i = 0; i < N_WMA; i++) {
      int indeks = (posisi - 1 - i + N_WMA) % N_WMA;  // mundur dari sampel terbaru
      int bobot = N_WMA - i;
      jumlahBerbobot = jumlahBerbobot + (long)sampel[indeks] * bobot;
    }
    return (float)jumlahBerbobot / TOTAL_BOBOT;
  }
};

// EMA: cukup satu variabel
struct FilterEMA {
  float alpha;
  float nilai;
  bool  siap;

  float tambah(float sampelBaru) {
    if (!siap) {
      nilai = sampelBaru;  // sampel pertama langsung menjadi nilai awal
      siap = true;
    } else {
      nilai = nilai + alpha * (sampelBaru - nilai);
    }
    return nilai;
  }
};

Timer timerSampel = {20, 0};  // 50 sampel per detik
FilterSMA filterSma;
FilterWMA filterWma;
FilterEMA filterEma = {2.0f / (N_SMA + 1), 0.0f, false};  // alpha ≈ 0.12, setara SMA 16

void setup() {
  Serial.begin(115200);
  int nilaiAwal = analogRead(PIN_POT);
  filterSma.mulai(nilaiAwal);
  filterWma.mulai(nilaiAwal);
  filterEma.tambah(nilaiAwal);
  // Sengaja tidak mencetak judul: Serial Plotter menganggap setiap baris sebagai data grafik.
}

void loop() {
  unsigned long sekarang = millis();

  if (timerSampel.sudahWaktunya(sekarang)) {
    int mentah = analogRead(PIN_POT);
    float sma = filterSma.tambah(mentah);
    float wma = filterWma.tambah(mentah);
    float ema = filterEma.tambah(mentah);

    // Format Serial Plotter: label:nilai, dipisah koma, satu baris = satu titik grafik
    Serial.printf("Mentah:%d,SMA:%.1f,WMA:%.1f,EMA:%.1f\n", mentah, sma, wma, ema);
  }
}
