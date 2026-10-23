/*
 * BAB 36 - Program 5: Lampu Suasana dengan State Machine
 * Tombol S1 mengganti mode: MATI → PELANGI → BERNAPAS → LILIN → SIRENE → MATI ...
 * Mode SIRENE kembali ke MATI sendiri setelah 10 detik.
 * Potensiometer mengatur kecerahan global.
 *
 * Rangkaian: D-IN  (header LED)    → IO12
 *            SW1   (header kuning) → IO15
 *            VR10K (header kuning) → IO34
 * Library  : FastLED (Daniel Garcia)
 */

#include <FastLED.h>

#define PIN_WS2812 12   // D-IN WS2812 → IO12
#define JUMLAH_LED 1
#define TOMBOL_S1  15   // ditekan = HIGH (pull-down 10 kΩ di PCB)
#define PIN_POT    34   // potensiometer VR10K → IO34 (ADC1)

// ---- Tipe data ditulis paling atas supaya dikenal oleh semua fungsi ----
enum Mode { MATI, PELANGI, BERNAPAS, LILIN, SIRENE };

struct LangkahSirene {
  CRGB warna;
  unsigned long lamaMs;
};

const LangkahSirene POLA_SIRENE[] = {
  {CRGB(255, 0, 0),  60}, {CRGB(0, 0, 0),  60},
  {CRGB(255, 0, 0),  60}, {CRGB(0, 0, 0), 300},
  {CRGB(0, 0, 255),  60}, {CRGB(0, 0, 0),  60},
  {CRGB(0, 0, 255),  60}, {CRGB(0, 0, 0), 300},
};
const int JUMLAH_LANGKAH = sizeof(POLA_SIRENE) / sizeof(POLA_SIRENE[0]);

const unsigned long INTERVAL_FRAME_MS = 20;     // perbarui LED 50 kali per detik
const unsigned long PERIODE_NAPAS_MS  = 4000;   // satu kali napas = 4 detik
const unsigned long BATAS_SIRENE_MS   = 10000;  // sirene berhenti sendiri setelah 10 detik
const int KECERAHAN_MIN  = 5;                   // kecerahan saat potensiometer di ujung 0
const int KECERAHAN_MAKS = 200;                 // kecerahan saat potensiometer di ujung lain

CRGB leds[JUMLAH_LED];

Mode mode = MATI;
unsigned long waktuMasukMode = 0;  // kapan mode sekarang dimulai
unsigned long waktuFrame = 0;
int kecerahan = KECERAHAN_MIN;
int kecerahanTercetak = -100;      // supaya kecerahan pertama pasti dicetak

int langkahSirene = 0;             // untuk mode SIRENE
unsigned long waktuLangkah = 0;
unsigned long waktuLilin = 0;      // untuk mode LILIN
unsigned long jedaLilin = 0;

// ---- Debounce tombol S1 (sama seperti di BAB 9, BAB 16, dan BAB 17) ----
const unsigned long WAKTU_DEBOUNCE = 50;
int bacaanSebelumnya = LOW;
int statusStabil = LOW;
unsigned long waktuBerubah = 0;

// Mengembalikan true hanya sekali setiap S1 baru ditekan (sudah di-debounce)
bool s1BaruDitekan() {
  int bacaan = digitalRead(TOMBOL_S1);
  if (bacaan != bacaanSebelumnya) {
    waktuBerubah = millis();
    bacaanSebelumnya = bacaan;
  }
  if ((millis() - waktuBerubah) > WAKTU_DEBOUNCE && bacaan != statusStabil) {
    statusStabil = bacaan;
    return statusStabil == HIGH;  // true hanya pada tepi naik (LOW → HIGH)
  }
  return false;
}

// ---- Bagian state machine ----
const char *namaMode(Mode m) {
  switch (m) {
    case MATI:     return "MATI";
    case PELANGI:  return "PELANGI";
    case BERNAPAS: return "BERNAPAS";
    case LILIN:    return "LILIN";
    case SIRENE:   return "SIRENE";
  }
  return "?";
}

Mode modeBerikutnya(Mode m) {
  switch (m) {
    case MATI:     return PELANGI;
    case PELANGI:  return BERNAPAS;
    case BERNAPAS: return LILIN;
    case LILIN:    return SIRENE;
    case SIRENE:   return MATI;
  }
  return MATI;
}

void pindahMode(Mode modeBaru, const char *alasan) {
  mode = modeBaru;
  waktuMasukMode = millis();

  // Aksi masuk: siapkan efek supaya selalu mulai dari awal
  langkahSirene = 0;
  waktuLangkah = waktuMasukMode;
  jedaLilin = 0;  // lilin langsung menggambar nyala pertama

  Serial.printf("Mode -> %s (%s)\n", namaMode(mode), alasan);
}

// ---- Efek-efek: hanya menghitung warna untuk "waktu sekarang" ----
void gambarPelangi(unsigned long sekarang) {
  leds[0] = CHSV((uint8_t)(sekarang / 20), 255, 255);
}

void gambarBernapas(unsigned long sekarang) {
  uint8_t posisi = (sekarang % PERIODE_NAPAS_MS) * 256 / PERIODE_NAPAS_MS;
  leds[0] = CHSV(160, 255, sin8(posisi));
}

void gambarLilin(unsigned long sekarang) {
  if (sekarang - waktuLilin >= jedaLilin) {
    waktuLilin = sekarang;
    jedaLilin = random(40, 150);        // lama nyala berikutnya: 40–149 ms, tidak teratur
    uint8_t hue = random(18, 34);       // jingga
    uint8_t terang = random(140, 256);  // kadang redup, kadang terang
    leds[0] = CHSV(hue, 255, terang);
  }
}

void gambarSirene(unsigned long sekarang) {
  if (sekarang - waktuLangkah >= POLA_SIRENE[langkahSirene].lamaMs) {
    waktuLangkah = sekarang;
    langkahSirene = (langkahSirene + 1) % JUMLAH_LANGKAH;
  }
  leds[0] = POLA_SIRENE[langkahSirene].warna;
}

void setup() {
  Serial.begin(115200);
  pinMode(TOMBOL_S1, INPUT);  // pull-down eksternal sudah ada di PCB

  FastLED.addLeds<WS2812B, PIN_WS2812, GRB>(leds, JUMLAH_LED);
  FastLED.setBrightness(KECERAHAN_MIN);
  FastLED.clear(true);

  Serial.println("BAB 36 - Program 5: Lampu Suasana dengan State Machine");
  Serial.println("Tekan S1 untuk ganti mode, putar potensiometer untuk kecerahan.");
  pindahMode(MATI, "awal");
}

void loop() {
  unsigned long sekarang = millis();
  unsigned long lamaDiMode = sekarang - waktuMasukMode;

  // 1) Transisi state: karena tombol, atau karena waktu
  if (s1BaruDitekan()) {
    pindahMode(modeBerikutnya(mode), "S1 ditekan");
  } else if (mode == SIRENE && lamaDiMode >= BATAS_SIRENE_MS) {
    pindahMode(MATI, "sirene sudah 10 detik");
  }

  // 2) Gambar satu frame setiap 20 ms
  if (sekarang - waktuFrame >= INTERVAL_FRAME_MS) {
    waktuFrame = sekarang;

    // Kecerahan global dari potensiometer
    kecerahan = map(analogRead(PIN_POT), 0, 4095, KECERAHAN_MIN, KECERAHAN_MAKS);
    if (abs(kecerahan - kecerahanTercetak) >= 10) {
      kecerahanTercetak = kecerahan;
      Serial.printf("Kecerahan: %d/255\n", kecerahan);
    }

    // Aksi selama di state: gambar efek sesuai mode
    switch (mode) {
      case MATI:     leds[0] = CRGB::Black;    break;
      case PELANGI:  gambarPelangi(sekarang);  break;
      case BERNAPAS: gambarBernapas(sekarang); break;
      case LILIN:    gambarLilin(sekarang);    break;
      case SIRENE:   gambarSirene(sekarang);   break;
    }

    FastLED.setBrightness(kecerahan);  // satu-satunya pembatas kecerahan
    FastLED.show();
  }
}
