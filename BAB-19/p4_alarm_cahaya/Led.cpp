// Led.cpp — definisi fungsi-fungsi class Led
#include "Led.h"

// "Led::" artinya fungsi ini milik class Led
Led::Led(int pin) {
  _pin = pin;  // hanya menyimpan nomor pin; pinMode() baru dipanggil di begin()
}

void Led::begin() {
  pinMode(_pin, OUTPUT);
  tulisKePin();
}

void Led::nyala() {
  _nyala = true;
  tulisKePin();
}

void Led::mati() {
  _nyala = false;
  tulisKePin();
}

void Led::toggle() {
  _nyala = !_nyala;
  tulisKePin();
}

void Led::kedip(unsigned long interval) {
  if (millis() - _waktuKedip >= interval) {
    _waktuKedip = millis();
    toggle();
  }
}

bool Led::sedangNyala() {
  return _nyala;
}

void Led::tulisKePin() {
  digitalWrite(_pin, _nyala ? HIGH : LOW);
}
