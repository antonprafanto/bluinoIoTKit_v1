/*
 * BAB 19 - Program 3: Class Led untuk Dua LED
 * Satu class Led, dua objek: LED merah berkedip cepat, LED hijau lambat.
 *
 * File: p3_kelas_led.ino (utama), Led.h, Led.cpp
 * Rangkaian:
 *   LED1 (merah) → IO4
 *   LED2 (hijau) → IO17
 */

#include "Led.h"

#define LED_MERAH 4
#define LED_HIJAU 17

Led ledMerah(LED_MERAH);  // objek pertama dari class Led
Led ledHijau(LED_HIJAU);  // objek kedua: class sama, pin dan datanya sendiri

bool hijauTerakhir = false;  // untuk mendeteksi perubahan LED hijau

void setup() {
  Serial.begin(115200);
  ledMerah.begin();
  ledHijau.begin();
  Serial.println("BAB 19 - Program 3: Class Led untuk Dua LED");
  Serial.println("Merah berkedip setiap 200 ms, hijau setiap 1000 ms");
}

void loop() {
  ledMerah.kedip(200);
  ledHijau.kedip(1000);

  // Laporkan setiap kali LED hijau berubah
  if (ledHijau.sedangNyala() != hijauTerakhir) {
    hijauTerakhir = ledHijau.sedangNyala();
    Serial.println(hijauTerakhir ? "LED hijau: NYALA" : "LED hijau: MATI");
  }
}
