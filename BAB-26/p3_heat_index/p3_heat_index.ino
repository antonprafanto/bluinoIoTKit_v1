/*
 * BAB 26 - Program 3: Kalkulator Heat Index
 * Menghitung suhu yang terasa (heat index) dari suhu dan kelembapan,
 * lalu menggolongkannya menurut tabel National Weather Service (NWS).
 *
 * Rangkaian: pin DHT (header hijau) → IO27
 */

#include <DHT.h>

#define DHT_PIN  27
#define DHT_TYPE DHT11

DHT dht(DHT_PIN, DHT_TYPE);

// Kategori heat index (°C). Batas dibulatkan dari tabel NWS (80, 90, 103, 125 °F).
const char *kategoriHeatIndex(float heatIndex) {
  if (heatIndex < 27.0) return "di bawah batas waspada";
  if (heatIndex < 32.0) return "Waspada";
  if (heatIndex < 39.0) return "Sangat waspada";
  if (heatIndex < 51.0) return "Bahaya";
  return "Bahaya ekstrem";
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  Serial.println("BAB 26 - Program 3: Kalkulator Heat Index");
}

void loop() {
  delay(2000);  // DHT11 paling cepat dibaca sekali setiap 2 detik

  float kelembapan = dht.readHumidity();
  float suhu = dht.readTemperature();

  if (isnan(kelembapan) || isnan(suhu)) {
    Serial.println("Gagal membaca DHT11! Periksa kabel DHT -> IO27.");
    return;
  }

  // Parameter ketiga false = suhu yang dimasukkan dalam °C, hasilnya juga °C
  float heatIndex = dht.computeHeatIndex(suhu, kelembapan, false);

  Serial.printf("Suhu       : %.1f °C\n", suhu);
  Serial.printf("Kelembapan : %.1f %%RH\n", kelembapan);
  Serial.printf("Heat index : %.1f °C (%s)\n", heatIndex, kategoriHeatIndex(heatIndex));
  Serial.println("----------------------------------------");
}
