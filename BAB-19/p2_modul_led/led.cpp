// led.cpp — SOURCE modul LED: isi (definisi) semua fungsi yang dijanjikan di led.h.

#include "led.h"  // selalu include header milik sendiri

// Variabel "static": hanya bisa dipakai di dalam file led.cpp ini,
// jadi file lain tidak bisa mengubahnya secara tidak sengaja.
static int pinLed = -1;  // -1 artinya ledSetup() belum dipanggil
static bool nyala = false;
static unsigned long waktuKedip = 0;

// Variabel global biasa (tanpa static): DEFINISI dari "extern" di led.h.
// Hanya boleh ditulis di SATU file .cpp.
unsigned long jumlahGantiLed = 0;

// Fungsi "static": fungsi bantu yang hanya dipakai di file ini
static void aturLed(bool keadaanBaru) {
  if (pinLed < 0) return;            // belum di-setup: abaikan
  if (keadaanBaru == nyala) return;  // tidak ada perubahan
  nyala = keadaanBaru;
  digitalWrite(pinLed, nyala ? HIGH : LOW);
  jumlahGantiLed++;
}

void ledSetup(int pin) {
  pinLed = pin;
  pinMode(pinLed, OUTPUT);
  nyala = false;
  digitalWrite(pinLed, LOW);
}

void ledNyala() {
  aturLed(true);
}

void ledMati() {
  aturLed(false);
}

void ledToggle() {
  aturLed(!nyala);
}

void ledKedip(unsigned long interval) {
  if (millis() - waktuKedip >= interval) {
    waktuKedip = millis();
    ledToggle();
  }
}

bool ledSedangNyala() {
  return nyala;
}
