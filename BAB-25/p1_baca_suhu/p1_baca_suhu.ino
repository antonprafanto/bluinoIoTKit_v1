/*
 * BAB 25 - Program 1: Membaca Suhu Pertama Kali
 * Membaca suhu DS18B20 setiap 2 detik dengan cara sederhana (menunggu
 * sampai sensor selesai mengukur), lalu menampilkan suhu dan lama menunggunya.
 *
 * Rangkaian: kabel jumper dari pin DS (header hijau) ke IO14.
 * Resistor pull-up 4,7 kOhm sudah ada di PCB kit.
 * Library: OneWire (Paul Stoffregen dkk.) dan DallasTemperature (Miles Burton dkk.)
 */

#include <OneWire.h>
#include <DallasTemperature.h>

#define PIN_ONEWIRE 14   // jalur data DS18B20 (pin DS -> IO14)

OneWire oneWire(PIN_ONEWIRE);         // bus 1-Wire di IO14
DallasTemperature sensor(&oneWire);   // pengurus sensor suhu yang memakai bus tadi

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung

  Serial.println("BAB 25 - Program 1: Membaca Suhu Pertama Kali");

  sensor.begin();  // cari semua perangkat di bus 1-Wire
  Serial.printf("Perangkat 1-Wire ditemukan: %u\n", sensor.getDeviceCount());

  if (sensor.getDeviceCount() == 0) {
    Serial.println("Sensor tidak ditemukan! Cabut USB, periksa kabel DS -> 14,");
    Serial.println("lalu sambungkan USB lagi.");
  }
}

void loop() {
  unsigned long mulai = millis();
  sensor.requestTemperatures();               // suruh sensor mengukur, lalu TUNGGU sampai selesai
  unsigned long lamaTunggu = millis() - mulai;

  float suhu = sensor.getTempCByIndex(0);     // ambil hasil sensor pertama (indeks 0)

  if (suhu == DEVICE_DISCONNECTED_C) {        // -127 berarti pembacaan gagal
    Serial.println("Gagal membaca sensor (-127). Periksa kabel DS -> 14.");
  } else {
    Serial.printf("Suhu: %.2f °C  (menunggu %lu ms)\n", suhu, lamaTunggu);
  }

  delay(2000);  // jeda 2 detik sebelum mengukur lagi
}
