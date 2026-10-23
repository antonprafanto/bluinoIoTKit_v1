/*
 * BAB 22 - Program 3: Membaca Data CSV
 * Menerima baris data berformat CSV dari Serial Monitor:
 *   DATA,<suhu>,<kelembapan>      contoh: DATA,27.5,65
 * lalu memeriksa formatnya, menampilkan nilainya, dan
 * menyalakan LED peringatan.
 *
 * Rangkaian (header kit → pin ESP32):
 *   LED1 (merah)  → IO4    menyala kalau suhu > 35 °C
 *   LED4 (kuning) → IO16   menyala kalau kelembapan > 80 %
 * Alat uji: Serial Monitor, baud 115200, line ending New Line
 */

#define LED_MERAH  4
#define LED_KUNING 16

const int PANJANG_MAKS = 64;   // ukuran array baris, termasuk 1 tempat untuk '\0'

char baris[PANJANG_MAKS];      // teks C: array char yang diakhiri '\0'
int panjang = 0;               // jumlah karakter yang sudah terkumpul

// Mengumpulkan karakter yang SUDAH tiba, tanpa menunggu.
// Hasilnya true kalau satu baris lengkap sudah siap di array 'baris'.
bool bacaBaris() {
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\r' || c == '\n') {      // karakter akhir baris
      if (panjang == 0) continue;       // baris kosong (misalnya \n sesudah \r): lewati
      baris[panjang] = '\0';            // tutup teks dengan karakter nol
      panjang = 0;                      // siap untuk baris berikutnya
      return true;
    }
    if (panjang < PANJANG_MAKS - 1) {   // sisakan 1 tempat untuk '\0'
      baris[panjang] = c;
      panjang++;
    }                                   // kalau sudah penuh, karakter dibuang
  }
  return false;                         // baris belum lengkap
}

// Hasilnya true kalau SELURUH teks adalah angka yang sah, misalnya "27.5" atau "-3"
bool apakahAngka(const char *teks) {
  char *sisa;
  strtof(teks, &sisa);                  // sisa = posisi pertama yang bukan bagian angka
  return sisa != teks && *sisa == '\0';
}

void prosesBaris(char *teks) {
  Serial.printf("Diterima: \"%s\"\n", teks);   // cetak dulu, sebelum diubah strtok()

  // Pecah teks di setiap koma. strtok() mengganti setiap koma dengan '\0'.
  char *bagian[4] = {NULL, NULL, NULL, NULL};
  int jumlahBagian = 0;
  char *potongan = strtok(teks, ",");
  while (potongan != NULL && jumlahBagian < 4) {
    bagian[jumlahBagian] = potongan;
    jumlahBagian++;
    potongan = strtok(NULL, ",");
  }

  // Pemeriksaan 1: tepat 3 bagian, dan bagian pertama "DATA"
  if (jumlahBagian != 3 || strcmp(bagian[0], "DATA") != 0) {
    Serial.println("  Format salah. Contoh yang benar: DATA,27.5,65");
    return;
  }
  // Pemeriksaan 2: bagian kedua dan ketiga harus angka
  if (!apakahAngka(bagian[1]) || !apakahAngka(bagian[2])) {
    Serial.println("  Suhu dan kelembapan harus angka (desimal pakai titik).");
    return;
  }
  float suhu = atof(bagian[1]);
  float kelembapan = atof(bagian[2]);

  // Pemeriksaan 3: nilainya masuk akal
  bool suhuWajar = (suhu >= -40 && suhu <= 80);
  bool kelembapanWajar = (kelembapan >= 0 && kelembapan <= 100);
  if (!suhuWajar || !kelembapanWajar) {
    Serial.println("  Nilai di luar jangkauan (suhu -40..80, kelembapan 0..100).");
    return;
  }

  Serial.printf("  Suhu       : %.1f °C\n", suhu);
  Serial.printf("  Kelembapan : %.1f %%\n", kelembapan);

  bool panas = (suhu > 35);
  bool lembap = (kelembapan > 80);
  digitalWrite(LED_MERAH, panas ? HIGH : LOW);
  digitalWrite(LED_KUNING, lembap ? HIGH : LOW);

  if (panas) Serial.println("  Peringatan: suhu tinggi! (LED merah menyala)");
  if (lembap) Serial.println("  Peringatan: udara sangat lembap! (LED kuning menyala)");
  if (!panas && !lembap) Serial.println("  Kondisi normal.");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_KUNING, OUTPUT);
  digitalWrite(LED_MERAH, LOW);
  digitalWrite(LED_KUNING, LOW);

  Serial.println("BAB 22 - Program 3: Membaca Data CSV");
  Serial.println("Format: DATA,<suhu>,<kelembapan>   contoh: DATA,27.5,65");
}

void loop() {
  if (bacaBaris()) {          // tidak pernah menunggu
    prosesBaris(baris);
  }
  // Di sini program bebas mengerjakan tugas lain, misalnya membaca tombol
}
