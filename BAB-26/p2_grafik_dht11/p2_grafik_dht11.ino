/*
 * BAB 26 - Program 2: Grafik Suhu dan Kelembapan di Serial Plotter
 * Mengirim suhu dan kelembapan setiap 2 detik dalam format Serial Plotter.
 * Buka Tools → Serial Plotter, lalu pilih baud 115200.
 *
 * Rangkaian: pin DHT (header hijau) → IO27
 */

#include <DHT.h>

#define DHT_PIN  27
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

void setup() {
  Serial.begin(115200);
  dht.begin();
  // Sengaja tidak mencetak judul: Serial Plotter menganggap setiap baris sebagai data grafik.
}

void loop() {
  delay(2000);  // DHT11 paling cepat dibaca sekali setiap 2 detik

  float kelembapan = dht.readHumidity();
  float suhu = dht.readTemperature();

  if (isnan(kelembapan) || isnan(suhu)) {
    return;  // lewati titik ini; jangan kirim teks lain ke Serial Plotter
  }

  // Format Serial Plotter: label:nilai, dipisah koma, satu baris = satu titik
  Serial.print("Suhu_C:");
  Serial.print(suhu, 1);
  Serial.print(",Kelembapan_RH:");
  Serial.println(kelembapan, 1);
}
