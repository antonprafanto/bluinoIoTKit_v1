/*
 * BAB 34 - Program 3: Filter Median Melawan Lonjakan
 * Membaca potensiometer setiap 20 ms tanpa delay(). Setiap kali tombol S1
 * ditekan, satu sampel diganti dengan "lonjakan" buatan (4095 atau 0).
 * Serial Plotter menampilkan nilai mentah, rata-rata 5 sampel terakhir,
 * dan median 5 sampel terakhir.
 * Buka Tools → Serial Plotter, lalu pilih baud 115200.
 *
 * Rangkaian: VR10K (potensiometer) → IO34, SW1 (tombol S1) → IO15
 */

#define PIN_POT 34  // potensiometer (VR10K) → IO34
#define PIN_S1  15  // tombol S1 → IO15; ditekan = HIGH (pull-down 10 kΩ di PCB)

const int N_JENDELA = 5;        // rata-rata dan median dari 5 sampel terakhir
const int JUMLAH_LONJAKAN = 1;  // berapa sampel berturut-turut yang dirusak setiap S1 ditekan

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

Timer timerSampel = {20, 0};  // 50 sampel per detik

int jendela[N_JENDELA];  // 5 sampel terakhir (buffer melingkar)
int posisi = 0;          // kursi yang akan ditimpa berikutnya

int sisaLonjakan = 0;           // berapa sampel lagi yang harus dirusak
bool tombolSebelumnya = false;  // keadaan S1 pada sampel sebelumnya

float hitungRataRata() {
  long jumlah = 0;
  for (int i = 0; i < N_JENDELA; i++) {
    jumlah = jumlah + jendela[i];
  }
  return (float)jumlah / N_JENDELA;
}

int hitungMedian() {
  // Salin dulu ke array lain, supaya isi buffer melingkar tidak ikut teracak
  int urut[N_JENDELA];
  for (int i = 0; i < N_JENDELA; i++) {
    urut[i] = jendela[i];
  }
  // Insertion sort: sisipkan setiap angka ke tempat yang tepat di antara angka sebelumnya
  for (int i = 1; i < N_JENDELA; i++) {
    int kunci = urut[i];
    int j = i - 1;
    while (j >= 0 && urut[j] > kunci) {
      urut[j + 1] = urut[j];  // geser angka yang lebih besar satu tempat ke kanan
      j = j - 1;
    }
    urut[j + 1] = kunci;
  }
  return urut[N_JENDELA / 2];  // nilai tengah: indeks 2 dari 0-4
}

void setup() {
  Serial.begin(115200);
  pinMode(PIN_S1, INPUT);  // INPUT biasa: pull-down sudah ada di PCB kit

  int nilaiAwal = analogRead(PIN_POT);
  for (int i = 0; i < N_JENDELA; i++) {
    jendela[i] = nilaiAwal;  // isi semua kursi supaya awal grafik tidak melonjak dari 0
  }
  // Sengaja tidak mencetak judul: Serial Plotter menganggap setiap baris sebagai data grafik.
}

void loop() {
  unsigned long sekarang = millis();

  if (timerSampel.sudahWaktunya(sekarang)) {
    // 1. Deteksi momen S1 mulai ditekan (dari LOW menjadi HIGH)
    bool tombolSekarang = (digitalRead(PIN_S1) == HIGH);
    if (tombolSekarang && !tombolSebelumnya) {
      sisaLonjakan = JUMLAH_LONJAKAN;
    }
    tombolSebelumnya = tombolSekarang;

    // 2. Baca sensor; rusak sampelnya kalau masih ada jatah lonjakan
    int mentah = analogRead(PIN_POT);
    if (sisaLonjakan > 0) {
      mentah = (mentah < 2048) ? 4095 : 0;  // lompat ke ujung yang berlawanan
      sisaLonjakan = sisaLonjakan - 1;
    }

    // 3. Simpan ke buffer melingkar, lalu hitung kedua filter
    jendela[posisi] = mentah;
    posisi = (posisi + 1) % N_JENDELA;

    Serial.printf("Mentah:%d,RataRata5:%.1f,Median5:%d\n",
                  mentah, hitungRataRata(), hitungMedian());
  }
}
