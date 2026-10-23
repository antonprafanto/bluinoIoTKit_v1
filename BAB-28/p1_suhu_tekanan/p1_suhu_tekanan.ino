/*
 * BAB 28 - Program 1: Membaca Suhu dan Tekanan Udara
 * Membaca suhu (°C) dan tekanan udara (Pa dan hPa) dari BMP180 setiap 2 detik.
 * Kalau BMP180 tidak ditemukan, program memeriksa alamat 0x77 dan 0x76,
 * lalu menampilkan chip apa yang menjawab di sana.
 *
 * Rangkaian: tidak perlu kabel. BMP180 tersambung ke
 * IO21 (SDA) dan IO22 (SCL) lewat jumper cap di kit.
 * Library: Adafruit BMP085 Library (by Adafruit)
 */

#include <Wire.h>
#include <Adafruit_BMP085.h>

#define PIN_SDA 21
#define PIN_SCL 22

const unsigned long JEDA_BACA = 2000;  // baca sensor setiap 2000 ms
unsigned long waktuBacaTerakhir = 0;

Adafruit_BMP085 bmp;  // objek sensor BMP180

// Membaca 1 byte dari sebuah register (pola dari BAB 23).
// Hasil: 0-255 kalau berhasil, -1 kalau perangkat tidak menjawab.
int bacaRegister(uint8_t alamat, uint8_t reg) {
  Wire.beginTransmission(alamat);
  Wire.write(reg);
  Wire.endTransmission(false);
  if (Wire.requestFrom(alamat, 1) != 1) {
    return -1;
  }
  return Wire.read();
}

// Dipanggil kalau bmp.begin() gagal: periksa apa yang ada di 0x77 dan 0x76
void periksaSensorTekanan() {
  const uint8_t daftarAlamat[] = {0x77, 0x76};
  bool adaYangMenjawab = false;

  for (int i = 0; i < 2; i++) {
    uint8_t alamat = daftarAlamat[i];
    Wire.beginTransmission(alamat);
    if (Wire.endTransmission() != 0) {
      continue;  // tidak ada perangkat di alamat ini, lanjut ke alamat berikutnya
    }
    adaYangMenjawab = true;

    int id = bacaRegister(alamat, 0xD0);  // register chip-id
    Serial.printf("  Ada perangkat di 0x%02X, chip-id ", alamat);
    if (id == 0x55) {
      Serial.println("0x55 = BMP180. Cabut USB beberapa detik, lalu coba lagi.");
    } else if (id == 0x58) {
      Serial.println("0x58 = BMP280. Library BMP085 tidak bisa membacanya (lihat 28.11).");
    } else if (id == 0x60) {
      Serial.println("0x60 = BME280. Library BMP085 tidak bisa membacanya (lihat 28.11).");
    } else if (id < 0) {
      Serial.println("tidak terbaca.");
    } else {
      Serial.printf("0x%02X = chip lain yang tidak dikenal.\n", id);
    }
  }

  if (!adaYangMenjawab) {
    Serial.println("  Tidak ada perangkat di 0x77 maupun 0x76.");
    Serial.println("  Cabut USB, lalu periksa jumper cap SCL/SDA milik BMP180.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);                   // beri waktu Serial Monitor tersambung
  Wire.begin(PIN_SDA, PIN_SCL);  // bus I2C kit: SDA = IO21, SCL = IO22

  Serial.println("BAB 28 - Program 1: Membaca Suhu dan Tekanan Udara");

  if (!bmp.begin()) {  // mode bawaan: ultra high resolution
    Serial.println("BMP180 tidak ditemukan! Hasil pemeriksaan:");
    periksaSensorTekanan();
    Serial.println("Program berhenti. Perbaiki dulu, lalu tekan tombol EN.");
    while (true) {
      delay(1000);  // berhenti di sini
    }
  }
  Serial.println("BMP180 siap.");
}

void loop() {
  if (millis() - waktuBacaTerakhir < JEDA_BACA) {
    return;  // belum 2 detik, keluar dulu dari loop()
  }
  waktuBacaTerakhir = millis();

  float suhu = bmp.readTemperature();      // °C
  int32_t tekananPa = bmp.readPressure();  // pascal (Pa)
  float tekananHPa = tekananPa / 100.0;    // 1 hPa = 100 Pa

  Serial.printf("Suhu: %.1f °C | Tekanan: %ld Pa = %.2f hPa\n",
                suhu, (long)tekananPa, tekananHPa);
}
