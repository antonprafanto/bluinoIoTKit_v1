/*
 * BAB 14 - Program 1: Membaca Nilai Sentuhan
 * Menampilkan nilai touchRead() dari touch pad kit setiap 100 ms.
 * Pada ESP32 "klasik", nilainya TURUN saat pad disentuh.
 *
 * Rangkaian: TOUCH (touch pad, header kuning) → IO33 (T8)
 */

#define TOUCH_PIN T8  // T8 = kanal touch di IO33

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung

  Serial.println("BAB 14 - Program 1: Membaca Nilai Sentuhan");
  Serial.println("Sentuh pad TOUCH dan perhatikan nilainya turun.");
}

void loop() {
  uint32_t nilai = touchRead(TOUCH_PIN);  // baca nilai sentuhan terbaru

  Serial.print("Sentuh:");  // format "label:nilai" juga bisa digambar Serial Plotter
  Serial.println(nilai);

  delay(100);  // 10 kali per detik
}
