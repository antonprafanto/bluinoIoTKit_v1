/*
 * BAB 38 - Program 1: Servo Bergerak Bolak-Balik (Sweep)
 * Servo bergerak 0° → 180° → 0° terus-menerus, 1 derajat setiap 15 ms,
 * tanpa delay() di loop().
 *
 * Rangkaian:
 *   Servo di header servo kit: oranye → S, merah → V, cokelat → G
 *   SRV (header hijau, paling bawah) → IO32
 */

#include <ESP32Servo.h>

#define PIN_SERVO 32  // SRV → IO32

// Lebar pulsa (µs) untuk 0° dan 180°. Cocok untuk banyak SG90;
// ganti dengan hasil kalibrasi Program 2.
const int PULSA_MIN_US = 500;
const int PULSA_MAX_US = 2400;

const unsigned long JEDA_LANGKAH_MS = 15;  // 1° setiap 15 ms → 180° dalam ±2,7 detik

Servo servo;

int sudut = 0;  // sudut yang sedang diperintahkan
int arah = 1;   // +1 = sudut naik, -1 = sudut turun
unsigned long waktuLangkahLalu = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 38 - Program 1: Servo Bergerak Bolak-Balik");

  servo.setPeriodHertz(50);  // sinyal servo standar: 50 Hz (periode 20 ms)
  servo.attach(PIN_SERVO, PULSA_MIN_US, PULSA_MAX_US);
  if (!servo.attached()) {
    Serial.println("Gagal memasang servo di IO32! Periksa nomor pin.");
    while (true) {
      delay(1000);  // program berhenti di sini
    }
  }

  servo.write(sudut);  // mulai dari 0°
  Serial.printf("Servo di IO%d, pulsa %d-%d us, 1 derajat setiap %lu ms\n",
                PIN_SERVO, PULSA_MIN_US, PULSA_MAX_US, JEDA_LANGKAH_MS);
}

void loop() {
  unsigned long sekarang = millis();

  if (sekarang - waktuLangkahLalu >= JEDA_LANGKAH_MS) {
    waktuLangkahLalu = sekarang;

    // Langkah berikutnya, dan balik arah di kedua ujung
    sudut = sudut + arah;
    if (sudut >= 180) {
      sudut = 180;
      arah = -1;
    } else if (sudut <= 0) {
      sudut = 0;
      arah = 1;
    }
    servo.write(sudut);

    // Lapor setiap kelipatan 30° supaya Serial Monitor tidak banjir
    if (sudut % 30 == 0) {
      int pulsa = map(sudut, 0, 180, PULSA_MIN_US, PULSA_MAX_US);
      Serial.printf("Sudut: %3d derajat | pulsa: %4d us | arah berikutnya: %s\n",
                    sudut, pulsa, arah > 0 ? "naik" : "turun");
    }
  }

  // Tugas lain (membaca tombol, sensor, dan sebagainya) bisa ditaruh di sini
}
