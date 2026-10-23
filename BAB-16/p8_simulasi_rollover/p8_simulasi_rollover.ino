/*
 * BAB 16 - Program 8: Simulasi Rollover millis()
 * Membandingkan dua rumus pemeriksa waktu memakai angka buatan
 * di sekitar titik millis() kembali ke 0. Tidak perlu menunggu 49,7 hari!
 *
 * Rangkaian: tidak perlu merangkai.
 */

const unsigned long INTERVAL   = 1000;          // ms
const unsigned long WAKTU_LALU = 4294966796UL;  // 500 ms sebelum millis() kembali ke 0

// Lima contoh nilai millis() "sekarang" dan lama yang sebenarnya sudah berlalu
const unsigned long CONTOH_SEKARANG[] = {4294967000UL, 4294967295UL, 0, 300, 500};
const unsigned long SUDAH_BERLALU[]   = {204, 499, 500, 800, 1000};
const int JUMLAH_CONTOH = 5;

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung; delay() di setup() tidak masalah

  Serial.println("BAB 16 - Program 8: Simulasi Rollover millis()");
  Serial.print("waktuLalu = ");
  Serial.print(WAKTU_LALU);
  Serial.print(", INTERVAL = ");
  Serial.print(INTERVAL);
  Serial.println(" ms");

  for (int i = 0; i < JUMLAH_CONTOH; i++) {
    unsigned long sekarang = CONTOH_SEKARANG[i];
    bool seharusnya = (SUDAH_BERLALU[i] >= INTERVAL);  // jawaban yang benar

    // Rumus 1 (dipakai di BAB ini): hitung selisihnya
    unsigned long selisih = sekarang - WAKTU_LALU;
    bool hasilSelisih = (selisih >= INTERVAL);

    // Rumus 2 (keliru): bandingkan dengan hasil penjumlahan
    bool hasilJumlah = (sekarang >= WAKTU_LALU + INTERVAL);

    Serial.println();
    Serial.print("millis() = ");
    Serial.print(sekarang);
    Serial.print(" (sebenarnya sudah berlalu ");
    Serial.print(SUDAH_BERLALU[i]);
    Serial.println(" ms)");

    Serial.print("  Rumus selisih    : selisih = ");
    Serial.print(selisih);
    Serial.print(" -> ");
    Serial.print(hasilSelisih ? "SUDAH waktunya" : "belum waktunya");
    Serial.println(hasilSelisih == seharusnya ? "  (benar)" : "  <- KELIRU!");

    Serial.print("  Rumus penjumlahan: ");
    Serial.print(hasilJumlah ? "SUDAH waktunya" : "belum waktunya");
    Serial.println(hasilJumlah == seharusnya ? "  (benar)" : "  <- KELIRU!");
  }
}

void loop() {
  // Tidak ada yang perlu diulang. Tekan tombol EN untuk menjalankan ulang.
}
