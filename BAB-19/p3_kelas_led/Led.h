// Led.h — deklarasi class Led: satu objek = satu LED (atau output aktif-HIGH lain)
#pragma once  // cegah header ini terbaca dua kali (lihat 19.7)

#include <Arduino.h>

class Led {
public:                                // boleh dipakai dari luar class
  Led(int pin);                        // constructor: dijalankan saat objek dibuat
  void begin();                        // siapkan pin (panggil di setup())
  void nyala();
  void mati();
  void toggle();                       // balik keadaan
  void kedip(unsigned long interval);  // kedip non-blocking, panggil terus di loop()
  bool sedangNyala();

private:                               // hanya untuk fungsi milik class Led
  int _pin;
  bool _nyala = false;
  unsigned long _waktuKedip = 0;
  void tulisKePin();                   // fungsi bantu internal
};
