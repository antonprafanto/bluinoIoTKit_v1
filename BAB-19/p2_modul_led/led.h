// led.h — HEADER modul LED: daftar fungsi dan variabel yang boleh dipakai file lain.
// Isinya hanya DEKLARASI (janji); isi fungsinya ada di led.cpp.

#ifndef LED_H  // include guard: kalau LED_H belum didefinisikan...
#define LED_H  // ...definisikan sekarang, lalu baca isi di bawah ini

#include <Arduino.h>

void ledSetup(int pin);                 // siapkan pin LED (panggil di setup())
void ledNyala();
void ledMati();
void ledToggle();                       // balik keadaan LED
void ledKedip(unsigned long interval);  // kedip non-blocking, panggil terus di loop()
bool ledSedangNyala();

// Variabel yang dibagi ke file lain: di sini hanya DIDEKLARASIKAN (extern),
// variabelnya sendiri dibuat di led.cpp.
extern unsigned long jumlahGantiLed;

#endif  // akhir bagian yang dilindungi include guard
