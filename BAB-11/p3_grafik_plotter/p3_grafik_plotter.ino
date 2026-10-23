/*
 * BAB 11 - Program 3: Grafik Nilai ADC di Serial Plotter
 * Menggambar dua garis: nilai mentah potensiometer (sekali baca)
 * dan rata-rata 32 kali baca, untuk melihat efek perata-rataan terhadap derau.
 * Buka Tools → Serial Plotter, lalu pilih baud 115200.
 *
 * Rangkaian: VR10K (potensiometer, header kuning pin ke-5) → IO34
 */

#define POT_PIN 34  // potensiometer → IO34 (ADC1_CH6)

const int JUMLAH_SAMPEL = 32;  // banyaknya pembacaan yang dirata-rata

// Baca pin beberapa kali, lalu kembalikan rata-ratanya
int bacaRataRata(int pin, int jumlahSampel) {
  long total = 0;  // penampung jumlah semua pembacaan
  for (int i = 0; i < jumlahSampel; i++) {
    total += analogRead(pin);
  }
  return total / jumlahSampel;
}

void setup() {
  Serial.begin(115200);
  // Sengaja tidak mencetak judul: Serial Plotter menganggap setiap baris sebagai data grafik.
}

void loop() {
  int mentah = analogRead(POT_PIN);                     // sekali baca
  int rataRata = bacaRataRata(POT_PIN, JUMLAH_SAMPEL);  // rata-rata 32 kali baca

  // Format Serial Plotter: label:nilai, dipisah koma, satu baris = satu titik grafik
  Serial.print("Mentah:");
  Serial.print(mentah);
  Serial.print(",RataRata:");
  Serial.println(rataRata);

  delay(50);  // sekitar 20 titik per detik
}
