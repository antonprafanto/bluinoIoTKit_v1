/*
 * BAB 10 - Program 1: Tombol Tetap Tercatat Saat loop() Sibuk
 * Tombol S1 memicu interupsi. ISR hanya menghitung, loop() yang mencetak.
 * loop() sengaja dibuat sibuk 3 detik untuk menunjukkan bahwa tekanan
 * tombol tetap tercatat walaupun program sedang mengerjakan hal lain.
 *
 * Rangkaian: SW1 (tombol S1, header kuning) → IO15
 */

#define TOMBOL_S1 15  // tombol S1 → IO15 (pull-down 10 kΩ sudah ada di kit)

volatile unsigned long jumlahInterupsi = 0;  // diubah oleh ISR → wajib volatile
unsigned long jumlahSebelumnya = 0;          // hanya dipakai loop()

// ISR: dijalankan otomatis setiap ada tepi naik (LOW → HIGH) di IO15
void IRAM_ATTR isrTombolS1() {
  jumlahInterupsi = jumlahInterupsi + 1;  // cukup menghitung, lalu selesai secepatnya
}

void setup() {
  Serial.begin(115200);
  pinMode(TOMBOL_S1, INPUT);  // INPUT biasa: pull-down sudah ada di kit

  // Daftarkan ISR: setiap tepi naik di IO15 → jalankan isrTombolS1()
  attachInterrupt(digitalPinToInterrupt(TOMBOL_S1), isrTombolS1, RISING);

  Serial.println("BAB 10 - Program 1: Tombol Tetap Tercatat Saat loop() Sibuk");
  Serial.println("Tekan S1 satu kali saat loop() sedang sibuk...");
}

void loop() {
  Serial.println("loop(): sibuk 3 detik...");
  delay(3000);  // pura-pura mengerjakan tugas yang lama; tombol TIDAK diperiksa

  // Salin nilai dari ISR satu kali, lalu pakai salinannya
  unsigned long jumlahSekarang = jumlahInterupsi;

  if (jumlahSekarang != jumlahSebelumnya) {
    Serial.print("  -> Selama sibuk, S1 memicu ");
    Serial.print(jumlahSekarang - jumlahSebelumnya);
    Serial.print(" interupsi (total: ");
    Serial.print(jumlahSekarang);
    Serial.println(")");
    jumlahSebelumnya = jumlahSekarang;
  }
}
