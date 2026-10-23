/*
 * BAB 28 - Program 4: Menghitung Tekanan Permukaan Laut
 * Kalau ketinggian lokasi sudah diketahui, program ini menghitung tekanan
 * permukaan laut (p0) dari tekanan yang terukur, setiap 2 detik.
 * Hasilnya bisa dibandingkan dengan QNH dari BMKG atau dipakai di Program 3.
 *
 * Rangkaian: tidak perlu kabel. BMP180 tersambung ke
 * IO21 (SDA) dan IO22 (SCL) lewat jumper cap di kit.
 * Library: Adafruit BMP085 Library (by Adafruit)
 */

#include <Wire.h>
#include <Adafruit_BMP085.h>

#define PIN_SDA 21
#define PIN_SCL 22

// Ketinggian tempat kit berada, dalam meter di atas permukaan laut.
// Ganti dengan ketinggian lokasimu.
const float KETINGGIAN_LOKASI_M = 10.0;

const unsigned long JEDA_BACA = 2000;  // baca sensor setiap 2000 ms
unsigned long waktuBacaTerakhir = 0;

Adafruit_BMP085 bmp;

// Kebalikan rumus barometrik: tekanan permukaan laut dari tekanan p yang
// terukur di ketinggian tertentu (meter). Satuan hasilnya sama dengan satuan p.
float hitungTekananLaut(float p, float ketinggian) {
  return p / pow(1.0 - ketinggian / 44330.0, 5.255);
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  Wire.begin(PIN_SDA, PIN_SCL);

  Serial.println("BAB 28 - Program 4: Menghitung Tekanan Permukaan Laut");
  if (!bmp.begin()) {
    Serial.println("BMP180 tidak ditemukan! Jalankan Program 1 untuk memeriksa.");
    while (true) {
      delay(1000);  // berhenti di sini
    }
  }
  Serial.printf("Ketinggian lokasi: %.1f m dpl\n", KETINGGIAN_LOKASI_M);
}

void loop() {
  if (millis() - waktuBacaTerakhir < JEDA_BACA) {
    return;  // belum 2 detik
  }
  waktuBacaTerakhir = millis();

  int32_t tekananPa = bmp.readPressure();  // tekanan di lokasi ini (Pa)
  float tekananLautPa = hitungTekananLaut(tekananPa, KETINGGIAN_LOKASI_M);

  Serial.printf("Tekanan terukur: %.2f hPa | Tekanan permukaan laut: %.2f hPa\n",
                tekananPa / 100.0, tekananLautPa / 100.0);
}
