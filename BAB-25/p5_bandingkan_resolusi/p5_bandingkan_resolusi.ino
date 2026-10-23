/*
 * BAB 25 - Program 5: Membandingkan Resolusi
 * Mengukur suhu pada resolusi 9, 10, 11, dan 12 bit, lalu menampilkan
 * hasilnya, langkah terkecilnya, dan lama konversi yang benar-benar terjadi
 * dibandingkan dengan waktu maksimum dari datasheet.
 *
 * Rangkaian: kabel jumper dari pin DS (header hijau) ke IO14.
 */

#include <OneWire.h>
#include <DallasTemperature.h>

#define PIN_ONEWIRE 14   // jalur data DS18B20 (pin DS -> IO14)

OneWire oneWire(PIN_ONEWIRE);
DallasTemperature sensor(&oneWire);

void ukurPadaResolusi(int resolusi) {
  sensor.setResolution(resolusi);               // ubah resolusi semua sensor

  unsigned long mulai = millis();
  sensor.requestTemperatures();                 // menunggu sampai sensor selesai mengukur
  unsigned long lamaNyata = millis() - mulai;

  float suhu = sensor.getTempCByIndex(0);
  float langkah = 0.5 / (1 << (resolusi - 9));  // 9 bit: 0.5; 10 bit: 0.25; dst.

  if (suhu == DEVICE_DISCONNECTED_C) {
    Serial.printf("%2d bit: gagal membaca sensor (-127)\n", resolusi);
  } else {
    Serial.printf("%2d bit: suhu %.4f °C, langkah %.4f °C, konversi %lu ms (maks. %u ms)\n",
                  resolusi, suhu, langkah, lamaNyata,
                  sensor.millisToWaitForConversion(resolusi));
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung

  Serial.println("BAB 25 - Program 5: Membandingkan Resolusi");

  sensor.begin();
  if (sensor.getDeviceCount() == 0) {
    Serial.println("Sensor tidak ditemukan! Periksa kabel DS -> 14, lalu tekan EN.");
    return;
  }

  // Resolusi cukup diubah di scratchpad (sementara), tidak disimpan ke EEPROM
  // sensor, supaya EEPROM tidak cepat aus kalau program ini sering diulang.
  sensor.setAutoSaveScratchPad(false);

  for (int resolusi = 9; resolusi <= 12; resolusi++) {
    ukurPadaResolusi(resolusi);
  }
  Serial.println("Selesai. Tekan tombol EN untuk mengulang.");
}

void loop() {
  // Perbandingan hanya dijalankan sekali di setup().
}
