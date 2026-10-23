/*
 * BAB 26 - Program 1: Membaca Suhu dan Kelembapan
 * Membaca DHT11 setiap 2 detik, lalu menampilkan suhu dan kelembapan
 * di Serial Monitor.
 *
 * Rangkaian: pin DHT (header hijau) → IO27
 *            (resistor pull-up 4,7 kΩ sudah ada di PCB)
 * Library  : DHT sensor library by Adafruit (+ Adafruit Unified Sensor)
 */

#include <DHT.h>

#define DHT_PIN  27     // pin DHT di header hijau → IO27
#define DHT_TYPE DHT11  // jenis sensor (ganti DHT22 kalau memakai DHT22)

DHT dht(DHT_PIN, DHT_TYPE);  // objek sensor bernama "dht"

void setup() {
  Serial.begin(115200);
  dht.begin();  // siapkan pin data DHT11

  Serial.println("BAB 26 - Program 1: Membaca Suhu dan Kelembapan");
  Serial.println("Pembacaan pertama muncul 2 detik lagi...");
}

void loop() {
  // DHT11 paling cepat dibaca sekali setiap 2 detik. Jeda di awal loop()
  // sekaligus memberi DHT11 waktu stabil setelah dinyalakan.
  delay(2000);

  float kelembapan = dht.readHumidity();     // %RH (menghubungi sensor, ±25 ms)
  float suhu       = dht.readTemperature();  // °C (memakai data bacaan yang sama)

  // Kalau gagal, library mengembalikan NaN (Not a Number)
  if (isnan(kelembapan) || isnan(suhu)) {
    Serial.println("Gagal membaca DHT11! Periksa kabel DHT -> IO27.");
    return;  // lewati sisa loop(), coba lagi di putaran berikutnya
  }

  Serial.print("Suhu: ");
  Serial.print(suhu, 1);  // tampilkan 1 angka di belakang koma
  Serial.print(" °C | Kelembapan: ");
  Serial.print(kelembapan, 1);
  Serial.println(" %RH");
}
