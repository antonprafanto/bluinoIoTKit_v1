/*
 * BAB 36 - Program 4: Indikator Suhu Berwarna
 * Suhu DHT11 → warna WS2812 (biru = sejuk, hijau = nyaman, merah = panas)
 * Cahaya LDR → kecerahan WS2812 (ruangan gelap → LED redup)
 *
 * Rangkaian: D-IN (header LED)    → IO12
 *            DHT  (header hijau)  → IO27
 *            LDR  (header kuning) → IO35
 * Library  : FastLED (Daniel Garcia), DHT sensor library (Adafruit)
 */

#include <FastLED.h>
#include <DHT.h>

#define PIN_WS2812 12   // D-IN WS2812 → IO12
#define JUMLAH_LED 1
#define PIN_DHT    27   // DHT11 → IO27
#define TIPE_DHT   DHT11
#define PIN_LDR    35   // LDR → IO35 (ADC1; makin gelap → nilai makin besar)

// Rentang suhu → warna
const float SUHU_SEJUK = 24.0;  // suhu ini atau lebih rendah: biru (hue 160)
const float SUHU_PANAS = 34.0;  // suhu ini atau lebih tinggi: merah (hue 0)

// Rentang LDR → kecerahan (sesuaikan dengan ruanganmu)
const int LDR_TERANG = 500;     // nilai ADC saat ruangan terang
const int LDR_GELAP  = 3500;    // nilai ADC saat ruangan gelap
const int KECERAHAN_MAKS = 150; // kecerahan global saat ruangan terang
const int KECERAHAN_MIN  = 10;  // kecerahan global saat ruangan gelap

const unsigned long INTERVAL_SUHU_MS  = 3000;  // DHT11 lambat; 3 detik sekali sudah cukup
const unsigned long INTERVAL_LDR_MS   = 100;   // baca LDR 10 kali per detik
const unsigned long INTERVAL_FRAME_MS = 20;    // perbarui LED 50 kali per detik

CRGB leds[JUMLAH_LED];
DHT dht(PIN_DHT, TIPE_DHT);

unsigned long waktuSuhu = 0;
unsigned long waktuLdr = 0;
unsigned long waktuFrame = 0;

bool sudahAdaSuhu = false;  // LED baru dinyalakan setelah suhu pertama terbaca
uint8_t hueTarget = 96;     // hue yang dituju (dari suhu terakhir)
uint8_t hueSekarang = 96;   // hue yang sedang tampil (bergeser pelan ke target)
float ldrHalus = 0;         // nilai LDR yang sudah dihaluskan (EMA)
int kecerahan = KECERAHAN_MAKS;

// Suhu → hue: 24 °C = 160 (biru), 28 °C = 96 (hijau), 34 °C = 0 (merah)
uint8_t suhuKeHue(float suhu) {
  if (suhu <= SUHU_SEJUK) return 160;
  if (suhu >= SUHU_PANAS) return 0;
  float rasio = (SUHU_PANAS - suhu) / (SUHU_PANAS - SUHU_SEJUK);  // 1 = sejuk, 0 = panas
  return (uint8_t)(rasio * 160 + 0.5);  // + 0.5 supaya dibulatkan
}

// Nilai LDR → kecerahan global (makin gelap, makin redup)
int ldrKeKecerahan(float ldr) {
  int k = map((long)ldr, LDR_TERANG, LDR_GELAP, KECERAHAN_MAKS, KECERAHAN_MIN);
  return constrain(k, KECERAHAN_MIN, KECERAHAN_MAKS);
}

const char *namaWarna(uint8_t hue) {
  if (hue < 16)  return "merah";
  if (hue < 48)  return "oranye";
  if (hue < 80)  return "kuning";
  if (hue < 112) return "hijau";
  if (hue < 144) return "toska";
  if (hue < 176) return "biru";
  if (hue < 208) return "ungu";
  if (hue < 240) return "merah muda";
  return "merah";
}

void setup() {
  Serial.begin(115200);

  FastLED.addLeds<WS2812B, PIN_WS2812, GRB>(leds, JUMLAH_LED);
  FastLED.setBrightness(KECERAHAN_MAKS);
  FastLED.clear(true);

  dht.begin();
  ldrHalus = analogRead(PIN_LDR);  // EMA dimulai dari bacaan pertama, bukan dari 0

  Serial.println("BAB 36 - Program 4: Indikator Suhu Berwarna");
  Serial.println("Biru = sejuk (<= 24 °C), hijau = 28 °C, merah = panas (>= 34 °C)");
}

void loop() {
  unsigned long sekarang = millis();

  // Tugas 1: baca LDR dan haluskan dengan EMA
  if (sekarang - waktuLdr >= INTERVAL_LDR_MS) {
    waktuLdr = sekarang;
    int adc = analogRead(PIN_LDR);
    ldrHalus = 0.8 * ldrHalus + 0.2 * adc;  // EMA dengan alfa 0,2
    kecerahan = ldrKeKecerahan(ldrHalus);
  }

  // Tugas 2: baca suhu DHT11 (butuh sekitar 25 ms)
  if (sekarang - waktuSuhu >= INTERVAL_SUHU_MS) {
    waktuSuhu = sekarang;
    float suhu = dht.readTemperature();

    if (isnan(suhu)) {
      Serial.println("Gagal membaca DHT11! Periksa kabel DHT -> 27.");
    } else {
      hueTarget = suhuKeHue(suhu);
      if (!sudahAdaSuhu) {        // bacaan pertama: langsung tampilkan, tanpa bergeser
        hueSekarang = hueTarget;
        sudahAdaSuhu = true;
      }
      Serial.printf("Suhu %.1f °C -> hue %3d (%s) | LDR %4d -> kecerahan %3d/255\n",
                    suhu, hueTarget, namaWarna(hueTarget), (int)ldrHalus, kecerahan);
    }
  }

  // Tugas 3: perbarui LED; hue bergeser 1 langkah per frame menuju target
  if (sudahAdaSuhu && sekarang - waktuFrame >= INTERVAL_FRAME_MS) {
    waktuFrame = sekarang;

    if (hueSekarang < hueTarget) hueSekarang++;
    else if (hueSekarang > hueTarget) hueSekarang--;

    leds[0] = CHSV(hueSekarang, 255, 255);
    FastLED.setBrightness(kecerahan);  // satu-satunya pembatas kecerahan
    FastLED.show();
  }
}
