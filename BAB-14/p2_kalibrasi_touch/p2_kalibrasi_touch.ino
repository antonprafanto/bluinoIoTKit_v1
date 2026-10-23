/*
 * BAB 14 - Program 2: Kalibrasi Dua Langkah
 * 1) Ukur nilai saat pad TIDAK disentuh, 2) ukur nilai saat pad DISENTUH,
 * 3) hitung ambang di tengah-tengahnya, lalu pakai untuk menyalakan LED.
 *
 * Rangkaian:
 *   TOUCH (touch pad, header kuning) → IO33 (T8)
 *   LED1 (LED merah, header LED)     → IO4
 */

#define TOUCH_PIN T8  // T8 = IO33
#define LED_MERAH 4

const int JUMLAH_SAMPEL = 50;  // 50 bacaan x 20 ms = sekitar 1 detik

uint32_t ambang = 0;           // hasil kalibrasi
bool statusTerakhir = false;   // false = dilepas, true = disentuh

// Rata-rata beberapa bacaan supaya derau teredam
uint32_t bacaRataRata(int jumlah) {
  uint32_t total = 0;
  for (int i = 0; i < jumlah; i++) {
    total += touchRead(TOUCH_PIN);
    delay(20);  // driver memperbarui nilai sekitar setiap 10 ms
  }
  return total / jumlah;
}

// Menampilkan hitung mundur "3... 2... 1..." di Serial Monitor
void hitungMundur(int detik) {
  for (int i = detik; i > 0; i--) {
    Serial.print(i);
    Serial.print("... ");
    delay(1000);
  }
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);
  digitalWrite(LED_MERAH, LOW);
  delay(1000);  // beri waktu Serial Monitor tersambung

  Serial.println("BAB 14 - Program 2: Kalibrasi Dua Langkah");

  // Langkah 1: pad tidak disentuh
  Serial.println("Langkah 1: JANGAN sentuh pad. Mengukur dalam");
  hitungMundur(3);
  uint32_t nilaiLepas = bacaRataRata(JUMLAH_SAMPEL);
  Serial.print("Nilai tanpa sentuhan : ");
  Serial.println(nilaiLepas);

  // Langkah 2: pad disentuh
  Serial.println("Langkah 2: SENTUH dan TAHAN pad. Mengukur dalam");
  hitungMundur(3);
  uint32_t nilaiSentuh = bacaRataRata(JUMLAH_SAMPEL);
  Serial.print("Nilai saat disentuh  : ");
  Serial.println(nilaiSentuh);
  Serial.println("Silakan lepaskan jari.");

  // Langkah 3: ambang di tengah-tengah kedua nilai
  ambang = (nilaiLepas + nilaiSentuh) / 2;
  Serial.print("Ambang (tengah-tengah): ");
  Serial.println(ambang);

  if (nilaiLepas > 0) {
    Serial.print("Persentase sentuh    : ");
    Serial.print(nilaiSentuh * 100 / nilaiLepas);
    Serial.println("% dari nilai tanpa sentuhan");
  }

  // Penurunan kurang dari 20% dianggap terlalu kecil untuk deteksi yang andal
  if (nilaiSentuh * 10 > nilaiLepas * 8) {
    Serial.println("PERINGATAN: perbedaannya terlalu kecil!");
    Serial.println("Cek kabel touch pad -> IO33, lalu tekan EN untuk mengulang.");
  }

  Serial.println("Kalibrasi selesai. Sentuh pad untuk menyalakan LED merah.");
}

void loop() {
  uint32_t nilai = touchRead(TOUCH_PIN);
  bool disentuh = (nilai < ambang);  // ESP32 klasik: disentuh = nilai di bawah ambang

  digitalWrite(LED_MERAH, disentuh ? HIGH : LOW);

  // Cetak hanya saat status berubah, supaya Serial Monitor tidak banjir
  if (disentuh != statusTerakhir) {
    Serial.print(disentuh ? "Disentuh" : "Dilepas");
    Serial.print(" (nilai ");
    Serial.print(nilai);
    Serial.println(")");
    statusTerakhir = disentuh;
  }

  delay(20);
}
