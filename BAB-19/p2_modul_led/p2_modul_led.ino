/*
 * BAB 19 - Program 2: Modul LED (.h + .cpp)
 * Semua urusan LED dipindahkan ke modul terpisah: led.h dan led.cpp.
 * File utama cukup #include "led.h" lalu memanggil fungsinya.
 *
 * File: p2_modul_led.ino (utama), led.h, led.cpp
 * Rangkaian: LED1 (merah) → IO4
 */

#include "led.h"  // tanda kutip: cari di folder sketch ini

#define LED_MERAH 4

bool keadaanTerakhir = false;  // untuk mendeteksi perubahan keadaan LED

void setup() {
  Serial.begin(115200);
  ledSetup(LED_MERAH);
  Serial.println("BAB 19 - Program 2: Modul LED (.h + .cpp)");
}

void loop() {
  ledKedip(500);  // dari led.cpp: LED berganti keadaan setiap 500 ms tanpa delay()

  // Laporkan setiap kali keadaan LED berubah
  if (ledSedangNyala() != keadaanTerakhir) {
    keadaanTerakhir = ledSedangNyala();
    Serial.print("LED ");
    Serial.print(keadaanTerakhir ? "NYALA" : "MATI");
    Serial.print(" | ganti ke-");
    Serial.println(jumlahGantiLed);  // milik led.cpp, dikenal lewat extern di led.h
  }
}
