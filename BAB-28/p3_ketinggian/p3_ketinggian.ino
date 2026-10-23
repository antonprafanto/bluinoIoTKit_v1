/*
 * BAB 28 - Program 3: Menghitung Ketinggian
 * Membaca tekanan udara setiap 2 detik, lalu menghitung ketinggian
 * di atas permukaan laut dengan rumus barometrik dari datasheet BMP180.
 *
 * Rangkaian: tidak perlu kabel. BMP180 tersambung ke
 * IO21 (SDA) dan IO22 (SCL) lewat jumper cap di kit.
 * Library: Adafruit BMP085 Library (by Adafruit)
 */

#include <Wire.h>
#include <Adafruit_BMP085.h>

#define PIN_SDA 21
#define PIN_SCL 22

// Tekanan permukaan laut (p0) dalam hPa. 1013.25 = atmosfer standar.
// Ganti dengan QNH bandara terdekat hari ini, misalnya 1010.0 untuk Q1010.
const float TEKANAN_LAUT_HPA = 1013.25;

const unsigned long JEDA_BACA = 2000;  // baca sensor setiap 2000 ms
unsigned long waktuBacaTerakhir = 0;

Adafruit_BMP085 bmp;

// Rumus barometrik: ketinggian (meter) dari tekanan p dan tekanan permukaan laut p0.
// p dan p0 harus bersatuan sama (di program ini keduanya dalam pascal).
float hitungKetinggian(float p, float p0) {
  return 44330.0 * (1.0 - pow(p / p0, 0.1903));
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  Wire.begin(PIN_SDA, PIN_SCL);

  Serial.println("BAB 28 - Program 3: Menghitung Ketinggian");
  if (!bmp.begin()) {
    Serial.println("BMP180 tidak ditemukan! Jalankan Program 1 untuk memeriksa.");
    while (true) {
      delay(1000);  // berhenti di sini
    }
  }
  Serial.printf("Tekanan permukaan laut (p0): %.2f hPa\n", TEKANAN_LAUT_HPA);
}

void loop() {
  if (millis() - waktuBacaTerakhir < JEDA_BACA) {
    return;  // belum 2 detik
  }
  waktuBacaTerakhir = millis();

  int32_t tekananPa = bmp.readPressure();  // tekanan dibaca sekali saja (Pa)
  float ketinggian = hitungKetinggian(tekananPa, TEKANAN_LAUT_HPA * 100.0);  // p0: hPa -> Pa

  Serial.printf("Tekanan: %.2f hPa | Ketinggian: %.1f m dpl\n",
                tekananPa / 100.0, ketinggian);
}
