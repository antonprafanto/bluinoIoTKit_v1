// Tombol.h — deklarasi class Tombol: debounce + callback saat ditekan (lihat BAB 18)
#pragma once

#include <Arduino.h>
#include <functional>

class Tombol {
public:
  using Callback = std::function<void()>;  // bentuk fungsi callback

  Tombol(int pin, unsigned long waktuDebounce);
  void begin();                       // siapkan pin (panggil di setup())
  void update();                      // panggil terus di loop()
  void saatDitekan(Callback fungsi);  // daftarkan callback

private:
  int _pin;
  unsigned long _waktuDebounce;
  Callback _callbackDitekan;          // kosong sampai didaftarkan
  int _bacaanSebelumnya = LOW;
  int _statusStabil = LOW;
  unsigned long _waktuBerubah = 0;
};
