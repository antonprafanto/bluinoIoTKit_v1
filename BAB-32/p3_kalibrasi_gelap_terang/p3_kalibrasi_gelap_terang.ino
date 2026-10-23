/*
 * BAB 32 - Program 3: Kalibrasi Titik Gelap dan Titik Terang
 * Nilai LDR dirata-rata tanpa delay() (20 sampel, satu setiap 5 ms),
 * lalu diubah menjadi persentase cahaya 0-100 % berdasarkan dua titik kalibrasi.
 * Ketik di Serial Monitor (line ending: New Line), lalu tekan Enter:
 *   g → simpan titik GELAP (matikan lampu atau tutup LDR dulu)
 *   t → simpan titik TERANG (nyalakan lampu atau buka LDR dulu)
 *   r → kembali ke nilai bawaan
 *
 * Rangkaian: LDR (header kuning, pin ke-1) → IO35
 */

#define LDR_PIN 35  // LDR → IO35 (ADC1_CH7)

const int GELAP_BAWAAN = 3800;         // titik gelap bawaan (ADC besar = gelap)
const int TERANG_BAWAAN = 1000;        // titik terang bawaan
const int SELISIH_MINIMUM = 300;       // titik gelap harus lebih besar dari titik terang minimal sebesar ini
const int JUMLAH_SAMPEL = 20;          // 20 sampel × 5 ms = rata-rata 100 ms terakhir
const unsigned int PANJANG_MAKS = 20;  // batas panjang satu perintah

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

Timer timerSampel = {5, 0};     // ambil sampel setiap 5 ms
Timer timerLaporan = {500, 0};  // laporan setiap 500 ms
RataRataBergerak filterLdr;

int titikGelap = GELAP_BAWAAN;    // hasil kalibrasi disimpan di variabel (hilang saat reset)
int titikTerang = TERANG_BAWAAN;
String penampung = "";            // karakter perintah yang sudah tiba

// Persentase cahaya: 0 % = titik gelap, 100 % = titik terang.
// Hasil -1 berarti kalibrasinya belum masuk akal.
int hitungPersen(int nilai) {
  if (titikGelap - titikTerang < SELISIH_MINIMUM) {
    return -1;
  }
  long persen = (long)(titikGelap - nilai) * 100 / (titikGelap - titikTerang);
  if (persen < 0) persen = 0;      // lebih gelap daripada titik gelap
  if (persen > 100) persen = 100;  // lebih terang daripada titik terang
  return persen;
}

void prosesPerintah(String perintah) {
  perintah.trim();         // buang spasi dan karakter akhir baris
  perintah.toLowerCase();  // "G" dianggap sama dengan "g"
  if (perintah.length() == 0) return;

  char huruf = perintah.charAt(0);  // cukup huruf pertamanya
  int nilaiSaatIni = filterLdr.nilai();

  if (huruf == 'g') {
    titikGelap = nilaiSaatIni;
    Serial.printf(">>> Titik GELAP disimpan: %d\n", titikGelap);
  } else if (huruf == 't') {
    titikTerang = nilaiSaatIni;
    Serial.printf(">>> Titik TERANG disimpan: %d\n", titikTerang);
  } else if (huruf == 'r') {
    titikGelap = GELAP_BAWAAN;
    titikTerang = TERANG_BAWAAN;
    Serial.println(">>> Kembali ke nilai bawaan");
  } else {
    Serial.println(">>> Perintah tidak dikenal. Pilihan: g, t, r");
    return;
  }

  if (titikGelap - titikTerang < SELISIH_MINIMUM) {
    Serial.println(">>> PERINGATAN: titik gelap harus lebih besar dari titik terang minimal 300.");
    Serial.println(">>> Ulangi: simpan g saat gelap, lalu t saat terang.");
  }
  Serial.printf("Salin ke Program 4: const int NILAI_GELAP = %d; const int NILAI_TERANG = %d;\n",
                titikGelap, titikTerang);
}

void setup() {
  Serial.begin(115200);
  penampung.reserve(PANJANG_MAKS);
  filterLdr.mulai(analogRead(LDR_PIN));  // isi semua kotak dengan bacaan pertama

  Serial.println("BAB 32 - Program 3: Kalibrasi Titik Gelap dan Titik Terang");
  Serial.println("Ketik g (simpan gelap), t (simpan terang), atau r (nilai bawaan), lalu Enter");
}

void loop() {
  unsigned long sekarang = millis();

  // Tugas 1: ambil satu sampel LDR setiap 5 ms
  if (timerSampel.sudahWaktunya(sekarang)) {
    filterLdr.tambah(analogRead(LDR_PIN));
  }

  // Tugas 2: ambil karakter perintah yang SUDAH tiba, tanpa menunggu
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {  // akhir baris → perintah lengkap
      prosesPerintah(penampung);
      penampung = "";
    } else if (penampung.length() < PANJANG_MAKS) {
      penampung += c;
    }
  }

  // Tugas 3: laporan setiap 500 ms
  if (timerLaporan.sudahWaktunya(sekarang)) {
    int nilai = filterLdr.nilai();
    int persen = hitungPersen(nilai);
    Serial.printf("ADC rata-rata: %4d | ", nilai);
    if (persen < 0) {
      Serial.println("Cahaya: ? (kalibrasi belum benar)");
    } else {
      Serial.printf("Cahaya: %3d %% | Gelap: %d | Terang: %d\n", persen, titikGelap, titikTerang);
    }
  }
}
