/*
 * BAB 36 - Program 1: Warna Dasar RGB
 * WS2812 menampilkan 9 warna bergantian setiap 1 detik (tanpa delay()).
 * Nama warna dan angka RGB-nya dicetak di Serial Monitor.
 *
 * Rangkaian: D-IN (header LED) → IO12
 * Library  : FastLED (Daniel Garcia)
 */

#include <FastLED.h>

#define PIN_WS2812 12   // D-IN WS2812 → IO12
#define JUMLAH_LED 1    // kit hanya punya 1 WS2812
#define KECERAHAN  80   // kecerahan global 0–255 (±31%)

CRGB leds[JUMLAH_LED];

// Satu warna = nama + tiga angka merah, hijau, biru (0–255)
struct Warna {
  const char *nama;
  uint8_t merah;
  uint8_t hijau;
  uint8_t biru;
};

const Warna DAFTAR_WARNA[] = {
  {"Merah",   255,   0,   0},
  {"Hijau",     0, 255,   0},
  {"Biru",      0,   0, 255},
  {"Kuning",  255, 255,   0},
  {"Cyan",      0, 255, 255},
  {"Magenta", 255,   0, 255},
  {"Oranye",  255,  80,   0},
  {"Putih",   255, 255, 255},
  {"Mati",      0,   0,   0},
};
const int JUMLAH_WARNA = sizeof(DAFTAR_WARNA) / sizeof(DAFTAR_WARNA[0]);

const unsigned long INTERVAL_MS = 1000;  // ganti warna setiap 1 detik
unsigned long waktuGanti = 0;
int urutan = 0;  // warna ke berapa yang akan ditampilkan berikutnya

void setup() {
  Serial.begin(115200);

  FastLED.addLeds<WS2812B, PIN_WS2812, GRB>(leds, JUMLAH_LED);
  FastLED.setBrightness(KECERAHAN);
  FastLED.clear(true);  // matikan LED (hapus warna sisa program sebelumnya)

  Serial.println("BAB 36 - Program 1: Warna Dasar RGB");
}

void loop() {
  unsigned long sekarang = millis();

  if (sekarang - waktuGanti >= INTERVAL_MS) {
    waktuGanti = sekarang;

    const Warna &w = DAFTAR_WARNA[urutan];
    leds[0] = CRGB(w.merah, w.hijau, w.biru);  // 1) ubah warna di buffer
    FastLED.show();                            // 2) kirim ke LED

    Serial.printf("Warna %d dari %d: %-8s CRGB(%3d, %3d, %3d)\n",
                  urutan + 1, JUMLAH_WARNA, w.nama, w.merah, w.hijau, w.biru);

    urutan = (urutan + 1) % JUMLAH_WARNA;  // setelah warna terakhir, kembali ke 0
  }

  // loop() tidak pernah tertahan, jadi di sini kamu bebas menambah tugas lain
}
