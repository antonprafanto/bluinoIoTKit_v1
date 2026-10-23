// Tombol.cpp — definisi fungsi-fungsi class Tombol
#include "Tombol.h"

Tombol::Tombol(int pin, unsigned long waktuDebounce) {
  _pin = pin;
  _waktuDebounce = waktuDebounce;
}

void Tombol::begin() {
  pinMode(_pin, INPUT);  // tombol kit: pull-down 10 kΩ di PCB, ditekan = HIGH
}

void Tombol::saatDitekan(Callback fungsi) {
  _callbackDitekan = fungsi;
}

void Tombol::update() {
  int bacaan = digitalRead(_pin);
  if (bacaan != _bacaanSebelumnya) {  // bacaan mentah berubah: ulang "stopwatch"
    _waktuBerubah = millis();
    _bacaanSebelumnya = bacaan;
  }
  if (millis() - _waktuBerubah > _waktuDebounce && bacaan != _statusStabil) {
    _statusStabil = bacaan;  // perubahan sudah stabil: anggap sungguhan
    if (_statusStabil == HIGH && _callbackDitekan) {
      _callbackDitekan();    // panggil callback yang didaftarkan
    }
  }
}
