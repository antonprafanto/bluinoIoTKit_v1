/*
 * BAB 36 - Program 3: Efek Animasi Satu LED
 * Tiga efek bergantian otomatis setiap 8 detik:
 *   BERNAPAS - biru meredup dan menerang perlahan
 *   PELANGI  - hue berputar terus melewati semua warna
 *   SIRENE   - kilat ganda merah, lalu kilat ganda biru
 * Tanpa delay(): LED digambar ulang setiap 20 ms (50 frame per detik).
 *
 * Rangkaian: D-IN (header LED) → IO12
 * Library  : FastLED (Daniel Garcia)
 */

#include <FastLED.h>

#define PIN_WS2812 12   // D-IN WS2812 → IO12
#define JUMLAH_LED 1
#define KECERAHAN  80   // kecerahan global 0–255 (±31%)

CRGB leds[JUMLAH_LED];

// ---- Tipe data ditulis paling atas supaya dikenal oleh semua fungsi ----
enum Efek { BERNAPAS, PELANGI, SIRENE };
const int JUMLAH_EFEK = 3;

// Satu langkah pola sirene: warna yang ditampilkan dan lamanya
struct LangkahSirene {
  CRGB warna;
  unsigned long lamaMs;
};

const LangkahSirene POLA_SIRENE[] = {
  {CRGB(255, 0, 0),  60},  // kilat merah
  {CRGB(0, 0, 0),    60},  // gelap sebentar
  {CRGB(255, 0, 0),  60},  // kilat merah lagi
  {CRGB(0, 0, 0),   300},  // jeda sebelum ganti warna
  {CRGB(0, 0, 255),  60},  // kilat biru
  {CRGB(0, 0, 0),    60},
  {CRGB(0, 0, 255),  60},
  {CRGB(0, 0, 0),   300},
};
const int JUMLAH_LANGKAH = sizeof(POLA_SIRENE) / sizeof(POLA_SIRENE[0]);

const unsigned long LAMA_PER_EFEK_MS  = 8000;  // ganti efek setiap 8 detik
const unsigned long INTERVAL_FRAME_MS = 20;    // gambar ulang LED 50 kali per detik
const unsigned long PERIODE_NAPAS_MS  = 4000;  // satu kali napas = 4 detik

Efek efekAktif = BERNAPAS;
unsigned long waktuGantiEfek = 0;
unsigned long waktuFrame = 0;
int langkahSirene = 0;            // langkah sirene yang sedang tampil
unsigned long waktuLangkah = 0;   // kapan langkah itu dimulai

const char *namaEfek(Efek e) {
  switch (e) {
    case BERNAPAS: return "BERNAPAS (biru, satu napas 4 detik)";
    case PELANGI:  return "PELANGI (satu putaran warna +-5 detik)";
    case SIRENE:   return "SIRENE (kilat ganda merah lalu biru)";
  }
  return "?";
}

// ---- Efek 1: bernapas ----
void gambarBernapas(unsigned long sekarang) {
  // posisi 0–255 di dalam satu kali napas
  uint8_t posisi = (sekarang % PERIODE_NAPAS_MS) * 256 / PERIODE_NAPAS_MS;
  uint8_t terang = sin8(posisi);      // gelombang sinus: naik-turun halus antara ±1 dan 255
  leds[0] = CHSV(160, 255, terang);   // biru; value CHSV sudah dikoreksi kurva redup
}

// ---- Efek 2: pelangi ----
void gambarPelangi(unsigned long sekarang) {
  uint8_t hue = (uint8_t)(sekarang / 20);  // naik 1 setiap 20 ms; 256 langkah ≈ 5,1 detik
  leds[0] = CHSV(hue, 255, 255);
}

// ---- Efek 3: sirene ----
void gambarSirene(unsigned long sekarang) {
  if (sekarang - waktuLangkah >= POLA_SIRENE[langkahSirene].lamaMs) {
    waktuLangkah = sekarang;
    langkahSirene = (langkahSirene + 1) % JUMLAH_LANGKAH;  // setelah langkah terakhir → 0
  }
  leds[0] = POLA_SIRENE[langkahSirene].warna;
}

void gantiEfek(unsigned long sekarang) {
  efekAktif = (Efek)((efekAktif + 1) % JUMLAH_EFEK);
  waktuGantiEfek = sekarang;
  langkahSirene = 0;          // sirene selalu mulai dari langkah pertama
  waktuLangkah = sekarang;

  Serial.print("Efek: ");
  Serial.println(namaEfek(efekAktif));
}

void setup() {
  Serial.begin(115200);

  FastLED.addLeds<WS2812B, PIN_WS2812, GRB>(leds, JUMLAH_LED);
  FastLED.setBrightness(KECERAHAN);
  FastLED.clear(true);

  Serial.println("BAB 36 - Program 3: Efek Animasi Satu LED");
  Serial.print("Efek: ");
  Serial.println(namaEfek(efekAktif));
}

void loop() {
  unsigned long sekarang = millis();

  // Tugas 1: ganti efek setiap 8 detik
  if (sekarang - waktuGantiEfek >= LAMA_PER_EFEK_MS) {
    gantiEfek(sekarang);
  }

  // Tugas 2: gambar satu frame setiap 20 ms
  if (sekarang - waktuFrame >= INTERVAL_FRAME_MS) {
    waktuFrame = sekarang;

    switch (efekAktif) {
      case BERNAPAS: gambarBernapas(sekarang); break;
      case PELANGI:  gambarPelangi(sekarang);  break;
      case SIRENE:   gambarSirene(sekarang);   break;
    }
    FastLED.show();  // kirim hasil frame ini ke LED
  }
}
