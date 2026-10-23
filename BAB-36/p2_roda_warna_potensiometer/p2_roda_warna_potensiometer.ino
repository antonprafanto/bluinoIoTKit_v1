/*
 * BAB 36 - Program 2: Roda Warna dengan Potensiometer
 * Posisi potensiometer (ADC 0–4095) diubah menjadi hue 0–255,
 * lalu WS2812 menampilkan CHSV(hue, 255, 255).
 *
 * Rangkaian: D-IN (header LED) → IO12
 *            VR10K (header kuning) → IO34
 * Library  : FastLED (Daniel Garcia)
 */

#include <FastLED.h>

#define PIN_WS2812 12   // D-IN WS2812 → IO12
#define JUMLAH_LED 1
#define KECERAHAN  80   // kecerahan global 0–255 (±31%)
#define PIN_POT    34   // potensiometer VR10K → IO34 (ADC1)

CRGB leds[JUMLAH_LED];

const unsigned long INTERVAL_BACA_MS = 50;  // baca potensiometer 20 kali per detik
unsigned long waktuBaca = 0;
int hueTercetak = -1;  // hue terakhir yang dicetak; -1 = belum pernah mencetak

// Nama perkiraan warna untuk setiap rentang hue FastLED
const char *namaWarna(uint8_t hue) {
  if (hue < 16)  return "merah";
  if (hue < 48)  return "oranye";
  if (hue < 80)  return "kuning";
  if (hue < 112) return "hijau";
  if (hue < 144) return "toska";
  if (hue < 176) return "biru";
  if (hue < 208) return "ungu";
  if (hue < 240) return "merah muda";
  return "merah";  // 240–255: roda warna kembali ke merah
}

void setup() {
  Serial.begin(115200);

  FastLED.addLeds<WS2812B, PIN_WS2812, GRB>(leds, JUMLAH_LED);
  FastLED.setBrightness(KECERAHAN);
  FastLED.clear(true);

  Serial.println("BAB 36 - Program 2: Roda Warna dengan Potensiometer");
  Serial.println("Putar kenop potensiometer untuk memilih warna.");
}

void loop() {
  unsigned long sekarang = millis();

  if (sekarang - waktuBaca >= INTERVAL_BACA_MS) {
    waktuBaca = sekarang;

    int adc = analogRead(PIN_POT);            // 0–4095
    uint8_t hue = map(adc, 0, 4095, 0, 255);  // ubah ke rentang hue 0–255

    leds[0] = CHSV(hue, 255, 255);  // saturasi penuh, value penuh
    FastLED.show();

    // Cetak hanya kalau hue berubah minimal 4 langkah, supaya Serial Monitor tidak banjir
    if (hueTercetak < 0 || abs(hue - hueTercetak) >= 4) {
      hueTercetak = hue;
      Serial.printf("ADC %4d -> hue %3d (%s) -> R=%3d G=%3d B=%3d\n",
                    adc, hue, namaWarna(hue), leds[0].r, leds[0].g, leds[0].b);
    }
  }
}
