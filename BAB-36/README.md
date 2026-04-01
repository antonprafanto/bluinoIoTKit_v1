# BAB 36: RGB LED WS2812 (NeoPixel)

> ✅ **Prasyarat:** Selesaikan **BAB 08 (Digital Output — LED & Buzzer)**, **BAB 12 (PWM dengan LEDC)**, dan **BAB 16 (Non-Blocking Programming)**. RGB LED WS2812 sudah terpasang permanen di Bluino Kit pada pin **IO12** — tidak perlu konfigurasi hardware tambahan!

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami **prinsip kerja protokol data one-wire WS2812** dan perbedaannya dengan LED RGB biasa.
- Menggunakan library **FastLED** secara profesional untuk mengontrol warna, kecerahan, dan efek.
- Menerapkan **ruang warna HSV** untuk membuat transisi warna yang natural dan halus.
- Membangun **efek animasi** seperti Rainbow, Knight Rider, Breathing, dan Running Fire.
- Membuat sistem **LED reaktif sensor** yang mengubah warna berdasarkan data DHT11 dan ADC.
- Mengintegrasikan WS2812 dengan **Finite State Machine (FSM)** untuk manajemen efek yang bersih.
- Memahami batasan **performa dan konsumsi daya** LED WS2812 dalam konteks sistem embedded.

---

## 36.1 WS2812 (NeoPixel) — Mengapa Berbeda dari LED RGB Biasa?

### LED RGB Biasa vs WS2812

```
PERBANDINGAN LED RGB BIASA vs WS2812:

  LED RGB Biasa (4-kaki):              WS2812 (Addressable RGB):
  ┌─────────────────────┐              ┌──────────────────────────────────┐
  │  R  G  B  GND       │              │  VCC   GND   DIN   DOUT          │
  │  │  │  │  │         │              │   │     │     │     │            │
  │ PWM PWM PWM 0V      │              │  5V    0V    IO12  → LED 2...   │
  └─────────────────────┘              └──────────────────────────────────┘

  ▶ 3 pin GPIO per LED                 ▶ HANYA 1 pin GPIO untuk N LED!
  ▶ 16×16×16 = 4096 warna             ▶ 256×256×256 = 16 juta warna
  ▶ 8-bit resolusi per channel         ▶ Setiap LED dikontrol INDEPENDEN
  ▶ Perlu 3 timer PWM per LED          ▶ Terdapat IC pengendali di dalam LED
  ▶ Terbatas 3–4 LED (kehabisan GPIO)  ▶ Bisa merangkai 100+ LED dengan 1 pin!
```

### Cara Kerja Protokol Data WS2812

Rahasia WS2812 adalah **IC kontroller kecil yang tertanam di dalam setiap LED bead**. IC ini menerima data serial dengan format bit yang sangat presisi berdasarkan lebar pulsa (*pulse-width encoding*):

```
ARSITEKTUR RANTAI WS2812 (Daisy-Chain):

  ESP32 IO12         LED 1              LED 2              LED N
  ┌───────┐     ┌──────────────┐   ┌──────────────┐   ┌────────────┐
  │       │────▶│ DIN  [IC]    │──▶│ DIN  [IC]    │──▶│ DIN  [IC]  │
  │  RMT  │     │      R G B   │   │      R G B   │   │      R G B │
  │ Engine│     └──────────────┘   └──────────────┘   └────────────┘
  └───────┘     DOUT────────────────▶ Sisa data
                                       diteruskan

FORMAT DATA (per LED):
  24 bit total = 8 bit Hijau + 8 bit Merah + 8 bit Biru (GRB order!)

  Bit "1": HIGH 800ns → LOW 450ns
  Bit "0": HIGH 400ns → LOW 850ns

  Toleransi sangat ketat! Itulah mengapa ESP32 menggunakan mesin RMT
  (Remote Control Transceiver) — hardware khusus untuk menciptakan
  timing yang sempurna tanpa gangguan software.
```

> ⚠️ **IO12 adalah Pin Strapping!** Di Bluino Kit, WS2812 terhubung ke IO12 yang merupakan pin strapping MTDI. Ini **AMAN digunakan saat operasi normal**, namun pin ini harus bertegangan **LOW saat proses upload firmware** (flash). Bluino Kit sudah menangani ini via resistor pull-down pada sirkuit. Jangan tambahkan pull-up eksternal ke IO12!

---

## 36.2 Instalasi Library & Konfigurasi Awal

### Library yang Digunakan: FastLED

Ada dua library populer untuk WS2812: **Adafruit NeoPixel** dan **FastLED**. BAB ini menggunakan **FastLED** karena:
- Mendukung ruang warna **HSV** secara native (sangat penting untuk efek natural)
- Fungsi matematis efek yang kaya (BRIGHTNESS curves, color blending)
- Performa lebih tinggi untuk rantai LED panjang

```
CARA INSTALASI (Arduino IDE 2.x):
  1. Tools → Manage Libraries...
  2. Cari: "FastLED"  → Install (oleh Daniel Garcia)
  3. Versi yang direkomendasikan: ≥ 3.6.0
  4. Restart Arduino IDE jika diminta
```

### Template Minimal — Wajib Dipahami

```cpp
// ── Konfigurasi Dasar FastLED (Wajib di setiap program) ──────────────
#include <FastLED.h>

#define LED_PIN    12      // IO12 — Pin Data WS2812 di Bluino Kit
#define NUM_LEDS   1       // Jumlah LED dalam rantai (sesuaikan kit-mu)
#define LED_TYPE   WS2812B // Tipe chip LED (WS2812B paling umum)
#define COLOR_ORDER GRB    // Urutan warna PENTING! WS2812 = GRB bukan RGB

CRGB leds[NUM_LEDS];       // Array yang merepresentasikan setiap LED

void setup() {
  // Daftarkan LED ke mesin FastLED
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);

  // Set kecerahan global (0-255). SANGAT DISARANKAN max 100 untuk
  // menjaga ESP32 tidak kelebihan beban arus dari USB!
  FastLED.setBrightness(80);

  FastLED.clear(); // Matikan semua LED
  FastLED.show();  // Kirim perintah ke hardware
}

void loop() {
  leds[0] = CRGB::Red;  // Warnai LED 0 dengan merah
  FastLED.show();        // WAJIB dipanggil agar perubahan terlihat
  delay(1000);

  leds[0] = CRGB::Black; // CRGB::Black = LED mati
  FastLED.show();
  delay(1000);
}
```

> 💡 **Filosofi FastLED:** Sama persis seperti Adafruit SSD1306 untuk OLED — semua perubahan pada array `leds[]` hanya tersimpan di buffer RAM. LED fisik baru berubah ketika kamu memanggil `FastLED.show()`. Ini adalah arsitektur **double-buffering** yang efisien!

---

## 36.3 Sistem Warna: RGB vs HSV

Sebelum membuat efek apapun, pahami dua sistem warna yang tersedia di FastLED:

```
SISTEM WARNA RGB (Red-Green-Blue):
  CRGB leds[0] = CRGB(255, 0, 0);    // R=255, G=0, B=0 → Merah murni
  CRGB leds[0] = CRGB(0, 255, 0);    // G=255 → Hijau murni
  CRGB leds[0] = CRGB(128, 0, 128);  // Ungu (campuran R+B)

  Nama warna bawaan tersedia:
  CRGB::Red, CRGB::Green, CRGB::Blue, CRGB::White,
  CRGB::Yellow, CRGB::Cyan, CRGB::Magenta, CRGB::Orange,
  CRGB::Purple, CRGB::Black (= mati), CRGB::White

SISTEM WARNA HSV (Hue-Saturation-Value) ← LEBIH KUAT untuk efek!
  CHSV hsv(hue, saturation, value)
    hue        = Rona warna (0-255, melingkar seperti roda warna)
    saturation = Saturasi (0=putih, 255=warna penuh/jenuh)
    value      = Kecerahan (0=mati/gelap, 255=maksimum)

KEUNGGULAN HSV ─ Contoh efek rainbow sederhana:
  for (int h = 0; h <= 255; h++) {
    leds[0] = CHSV(h, 255, 255); // Cukup ubah Hue → semua spektrum!
    FastLED.show();
    delay(10);
  }
  // Hasilnya: transisi MULUS dari merah → kuning → hijau → biru → ungu → ...
  // Coba lakukan ini dengan RGB — pusing sendiri mengaturnya!
```

```
PETA HUE WS2812 (FastLED, 0-255):

   0     32     64     96    128    160    192    224    255
   │      │      │      │      │      │      │      │      │
  Merah  Org   Kuning  Hijau  Cyan   Biru  Ungu   Pink  Merah
  (▶ 255 = sama dengan 0, roda warnanya melingkar!)
```

---

## 36.4 Program 1: Hello World & Warna Dasar

Program pengenalan yang mendemonstrasikan semua cara mewarnai LED, beralih ke HSV, dan membuat pola warna berulang secara non-blocking.

```cpp
/*
 * BAB 36 - Program 1: Hello World WS2812 — Eksplorasi Warna Dasar
 *
 * Tujuan:
 *   Menguasai cara mewarnai LED via RGB dan HSV,
 *   dan membiasakan diri dengan siklus show() non-blocking.
 *
 * Hardware (Built-in Kit):
 *   WS2812B RGB LED → IO12 (hardwired di Bluino Kit)
 *
 * Demonstrasi:
 *   1. Siklus 6 warna dasar via CRGB (RGB)
 *   2. Transisi hue penuh via CHSV (HSV)
 *   3. Tampilan status di Serial Monitor
 */

#include <FastLED.h>

// ── Konfigurasi ─────────────────────────────────────────────────────────
#define LED_PIN     12
#define NUM_LEDS    1
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define BRIGHTNESS  80      // 0-255, jaga dibawah 100 untuk daya USB

CRGB leds[NUM_LEDS];

// ── Timing Non-Blocking ──────────────────────────────────────────────────
const unsigned long INTERVAL_RGB_MS = 800UL;  // Ganti warna setiap 800ms
const unsigned long INTERVAL_HSV_MS = 15UL;   // Update HSV setiap 15ms
unsigned long tTerakhir = 0;

// ── State Program ────────────────────────────────────────────────────────
enum DemoState { DEMO_RGB, DEMO_HSV };
DemoState demoState = DEMO_RGB;

// ── Warna-warna untuk demonstrasi RGB ────────────────────────────────────
const CRGB warnaRGB[] = {
  CRGB::Red, CRGB::Green, CRGB::Blue,
  CRGB::Yellow, CRGB::Cyan, CRGB::Magenta
};
const char* namaWarna[] = {
  "Merah", "Hijau", "Biru", "Kuning", "Cyan", "Magenta"
};
const int JUMLAH_WARNA = 6;
int indWarna = 0;

// ── State HSV ────────────────────────────────────────────────────────────
uint8_t hue = 0;
int putaranHSV = 0;

void setup() {
  Serial.begin(115200);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(BRIGHTNESS);
  FastLED.clear();
  FastLED.show();

  Serial.println("=== BAB 36 Program 1: Hello World WS2812 ===");
  Serial.println("Fase 1: Siklus 6 warna dasar (RGB)");
}

void loop() {
  unsigned long sekarang = millis();

  if (demoState == DEMO_RGB) {
    // ── Siklus warna RGB, ganti setiap 800ms ──────────────────────────
    if (sekarang - tTerakhir >= INTERVAL_RGB_MS) {
      tTerakhir = sekarang;

      leds[0] = warnaRGB[indWarna];
      FastLED.show();

      Serial.printf("[RGB] Warna: %-8s  (R=%3d G=%3d B=%3d)\n",
        namaWarna[indWarna],
        leds[0].r, leds[0].g, leds[0].b);

      indWarna++;
      // Setelah 6 warna, pindah ke demonstrasi HSV
      if (indWarna >= JUMLAH_WARNA) {
        indWarna = 0;
        putaranHSV++;
        if (putaranHSV >= 2) { // Setelah 2 putaran RGB, masuk HSV
          demoState = DEMO_HSV;
          hue = 0;
          tTerakhir = sekarang;
          Serial.println("\nBeralih ke Fase 2: Rainbow HSV (spektrum penuh)");
        }
      }
    }

  } else { // DEMO_HSV
    // ── Transisi hue HSV, update setiap 15ms ─────────────────────────
    if (sekarang - tTerakhir >= INTERVAL_HSV_MS) {
      tTerakhir = sekarang;

      leds[0] = CHSV(hue, 255, 255);
      FastLED.show();

      if (hue % 32 == 0) { // Print setiap 1/8 putaran
        Serial.printf("[HSV] Hue: %3d → Warna: R=%3d G=%3d B=%3d\n",
          hue, leds[0].r, leds[0].g, leds[0].b);
      }

      hue++; // 0-255, overflow otomatis balik ke 0 (wraparound)
      if (hue == 0) {
        // Setelah 1 putaran penuh, kembali ke RGB
        demoState = DEMO_RGB;
        putaranHSV = 0;
        Serial.println("\nBeralih kembali ke Fase 1: Warna RGB Dasar");
      }
    }
  }
}
```

**Contoh output Serial Monitor:**
```text
=== BAB 36 Program 1: Hello World WS2812 ===
Fase 1: Siklus 6 warna dasar (RGB)
[RGB] Warna: Merah     (R=255 G=  0 B=  0)
[RGB] Warna: Hijau     (R=  0 G=255 B=  0)
[RGB] Warna: Biru      (R=  0 G=  0 B=255)
[RGB] Warna: Kuning    (R=255 G=255 B=  0)
[RGB] Warna: Cyan      (R=  0 G=255 B=255)
[RGB] Warna: Magenta   (R=255 G=  0 B=255)
...
Beralih ke Fase 2: Rainbow HSV (spektrum penuh)
[HSV] Hue:   0 → Warna: R=255 G=  0 B=  0
[HSV] Hue:  32 → Warna: R=255 G=171 B=  0
[HSV] Hue:  64 → Warna: R= 85 G=255 B=  0
[HSV] Hue:  96 → Warna: R=  0 G=255 B=170
...
```

---

## 36.5 Program 2: Efek Animasi — Breathing & Police Strobe

Karena Bluino Kit memiliki **1 buah WS2812 bawaan (Single Pixel)**, program animasi spasial (seperti lampu berjalan) tidak akan relevan. Oleh karena itu, kita akan menggunakan dua teknik animasi temporal fundamental: **Breathing** (napas memudar sinus) dan **Police Strobe** (kedip kilat peringatan bergantian Merah-Biru layaknya sirine). Keduanya dirancang non-blocking.

```cpp
/*
 * BAB 36 - Program 2: Efek Animasi LED (Breathing + Police Strobe)
 *
 * Fitur (Dioptimasi untuk Single LED):
 *   ▶ Efek Breathing: Kecerahan naik-turun halus (sin wave)
 *   ▶ Efek Strobe: Kilatan kilas ganda merah/biru ala polisi
 *   ▶ Toggle efek setiap 8 detik secara otomatis
 *   ▶ Sepenuhnya non-blocking berbasis millis()
 *
 * Hardware:
 *   WS2812B → IO12 (built-in Bluino Kit Single Pixel)
 */

#include <FastLED.h>

// ── Konfigurasi ─────────────────────────────────────────────────────────
#define LED_PIN     12
#define NUM_LEDS    1      // Kit Bluino hanya 1 LED bawaan
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define MAX_BRIGHT  100    // Limit arus USB

CRGB leds[NUM_LEDS];

// ── State Efek ───────────────────────────────────────────────────────────
enum EffectState { FX_BREATHING, FX_STROBE };
EffectState fxState = FX_BREATHING;

// ── Timing ───────────────────────────────────────────────────────────────
const unsigned long INTERVAL_FX_SWITCH = 8000UL; // Ganti efek tiap 8 detik
const unsigned long INTERVAL_BREATH_MS = 20UL;   // Fading halus (50Hz)
unsigned long tFxSwitch = 0;
unsigned long tBreath   = 0;
unsigned long tStrobe   = 0;

// ── Breathing State ──────────────────────────────────────────────────────
uint16_t breathAngle = 0;   // "Sudut" 0-1535
uint8_t  breathHue   = 160; // Biru

// Aproksimasi kurva sinus natural untuk fading
uint8_t hitungKecerahan(uint16_t fineAngle) {
  uint8_t angle8 = (uint8_t)(fineAngle * 255UL / 1535UL);
  uint8_t s = sin8(angle8);
  return map(s, 0, 255, 10, MAX_BRIGHT);
}

void updateBreathing(unsigned long now) {
  if (now - tBreath < INTERVAL_BREATH_MS) return;
  tBreath = now;

  uint8_t bright = hitungKecerahan(breathAngle);
  fill_solid(leds, NUM_LEDS, CHSV(breathHue, 230, bright));
  FastLED.show();

  breathAngle = (breathAngle + 8) % 1536; // Laju pernapasan
}

// ── Police Strobe State ──────────────────────────────────────────────────
// Urutan pola strobe: Nyala(Merah) → Mati → Nyala(Merah) → Jeda 
//                   → Nyala(Biru) → Mati → Nyala(Biru) → Jeda
uint8_t strobeStep = 0;
uint8_t strobeHue  = 0; // 0=Merah, 160=Biru

void updatePoliceStrobe(unsigned long now) {
  // Atur interval dinamis: nyala sangat cepat, mati jeda sejenak
  unsigned long interval = 0;
  
  if (strobeStep % 2 == 1) {
    interval = 60; // Durasi "Mati" kilat
  } else if (strobeStep == 4 || strobeStep >= 8) {
    interval = 250; // Jeda antara ganti warna
  } else {
    interval = 50;  // Durasi "Nyala" kilat
  }

  if (now - tStrobe < interval) return;
  tStrobe = now;

  if (strobeStep == 0) strobeHue = 0;      // Mulai Merah
  if (strobeStep == 5) strobeHue = 160;    // Mulai Biru (jangan pakai CHSV BIRU karena R=0, ganti Hue)

  if (strobeStep < 4 || (strobeStep > 4 && strobeStep < 9)) {
    if (strobeStep % 2 == 0) {
      // Nyala terang penuh
      fill_solid(leds, NUM_LEDS, CHSV(strobeHue, 255, MAX_BRIGHT));
    } else {
      fill_solid(leds, NUM_LEDS, CRGB::Black);
    }
    FastLED.show();
  }

  strobeStep++;
  if (strobeStep >= 10) strobeStep = 0; // Reset siklus
}

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();

  Serial.println("=== BAB 36 Program 2: Animasi Single LED ===");
  Serial.println("Mulai: Breathing Effect (biru)");
}

void loop() {
  unsigned long now = millis();

  // ── Toggle otomatis setiap 8 detik ──────────────────────────────
  if (now - tFxSwitch >= INTERVAL_FX_SWITCH) {
    tFxSwitch = now;
    if (fxState == FX_BREATHING) {
      fxState = FX_STROBE;
      FastLED.clear();
      strobeStep = 0;
      Serial.println("Berganti ke: Police Strobe (Sirine Merah/Biru)");
    } else {
      fxState = FX_BREATHING;
      breathAngle = 0;
      Serial.println("Berganti ke: Breathing Effect (Biru Lambat)");
    }
  }

  // ── Eksekusi state aktif ───────────────────────────────────────────────
  if (fxState == FX_BREATHING) {
    updateBreathing(now);
  } else {
    updatePoliceStrobe(now);
  }
}
```

> 💡 **Teknik Timer Dinamis (`interval` variabel):** Menarik untuk diamati pada `updatePoliceStrobe()`! Jika fungsi biasanya menggunakan jeda tetap seperti `INTERVAL_BREATH_MS = 20UL`, sirine *Strobe* terasa menyentak karena waktu *Tunda Nyala* dibedakan dengan *Tunda Mati*. Perilaku asimetris (interval tidak konstan) ini menciptakan ilusi optik mirip lampu kilat asli pada mobil polisi.

---

## 36.6 Program 3: Reaktif Sensor — LED Termometer & Cahaya

Program ini adalah aplikasi IoT sesungguhnya — warna dan kecerahan LED berubah **otomatis berdasarkan data sensor real-time**: suhu dari DHT11 dan tingkat cahaya dari LDR (ADC). Ini membangun sensor-to-actuator feedback loop yang fundamental dalam sistem embedded.

```cpp
/*
 * BAB 36 - Program 3: LED Reaktif Sensor (Termometer + LDR)
 *
 * Fitur:
 *   ▶ Suhu DHT11  → Warna LED (Biru=dingin, Hijau=normal, Merah=panas)
 *   ▶ ADC LDR     → Kecerahan LED (ruangan gelap = LED redup)
 *   ▶ Transisi warna halus via interpolasi HSV
 *   ▶ Dashboard Serial Monitor lengkap
 *
 * Hardware:
 *   WS2812B    → IO12 (built-in Bluino Kit)
 *   DHT11      → IO27 (built-in Bluino Kit)
 *   LDR        → IO35 atau IO34 (via jumper header Custom)
 *
 * Library: FastLED, DHT sensor library (Adafruit)
 */

#include <FastLED.h>
#include <DHT.h>

// ── Konfigurasi Hardware ─────────────────────────────────────────────────
#define LED_PIN     12
#define NUM_LEDS    1
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB

#define DHTPIN  27
#define DHTTYPE DHT11
#define LDR_PIN 35        // Pin ADC untuk LDR (IO34/IO35 = input-only, aman)

// ── Suhu → Warna (Termometer Warna) ────────────────────────────────────
// Rentang suhu: < 25°C = Biru (dingin), 25-30°C = Hijau (normal), >30°C = Merah (panas)
#define SUHU_DINGIN  25.0f  // Di bawah ini → Biru (Hue ~160)
#define SUHU_PANAS   35.0f  // Di atas ini  → Merah (Hue ~0)
// Hue: 160 (biru tua) → 96 (hijau) → 0 (merah)

// ── Kecerahan dari LDR ───────────────────────────────────────────────────
// ADC LDR 12-bit: gelap = ~3800, terang = ~200 (tergantung rangkaian)
#define LDR_GELAP    3500   // Nilai ADC saat ruangan gelap
#define LDR_TERANG    300   // Nilai ADC saat ruangan terang
#define BRIGHT_MIN    15    // Minimum kecerahan (tidak pernah mati total)
#define BRIGHT_MAX   120    // Maximum kecerahan untuk USB

CRGB leds[NUM_LEDS];
DHT  dht(DHTPIN, DHTTYPE);

// ── EMA Filter untuk stabilisasi ─────────────────────────────────────────
const float EMA_A = 0.15f;
struct EMA {
  float val  = 0.0f;
  bool  init = false;
  void update(float newVal) {
    if (!init) { val = newVal; init = true; return; }
    val = EMA_A * newVal + (1.0f - EMA_A) * val;
  }
};

EMA emaSuhu;
EMA emaLDR;
EMA emaHue;     // Filter hue agar transisi warna halus
EMA emaBright;  // Filter kecerahan agar tidak berkedip

// ── Fungsi Peta Suhu → Hue ───────────────────────────────────────────────
float suhuKeHue(float suhu) {
  // Petakan suhu ke rentang hue
  // Biru (160) ──Hijau(96)── Merah (0)
  if (suhu <= SUHU_DINGIN) return 160.0f;
  if (suhu >= SUHU_PANAS)  return 0.0f;
  // Interpolasi linear antara 160 dan 0
  float rasio = (suhu - SUHU_DINGIN) / (SUHU_PANAS - SUHU_DINGIN);
  return 160.0f - rasio * 160.0f;
}

// ── Label warna untuk Serial Monitor ─────────────────────────────────────
const char* labelWarna(float suhu) {
  if (suhu < SUHU_DINGIN)          return "BIRU (Dingin)  ";
  if (suhu >= SUHU_PANAS)          return "MERAH (Panas!) ";
  if (suhu < (SUHU_DINGIN + 2.5f)) return "CYAN  (Sejuk)  ";
  return                                   "HIJAU (Normal) ";
}

// ── Timing ───────────────────────────────────────────────────────────────
const unsigned long INTERVAL_SENSOR_MS = 2000UL;
const unsigned long INTERVAL_LED_MS    =   50UL; // Update LED 20Hz
unsigned long tSensor = 0;
unsigned long tLED    = 0;

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();

  dht.begin();

  // Pre-inisialisasi EMA dengan bacaan pertama
  float s = dht.readTemperature();
  if (!isnan(s)) {
    emaSuhu.update(s);
    emaHue.update(suhuKeHue(s));
  }
  float ldrRaw = analogRead(LDR_PIN);
  emaLDR.update(ldrRaw);
  emaBright.update(map(ldrRaw, LDR_GELAP, LDR_TERANG, BRIGHT_MIN, BRIGHT_MAX));

  Serial.println("=== BAB 36 Program 3: LED Reaktif Sensor ===");
  Serial.println("Suhu DHT11 → Warna LED | LDR → Kecerahan LED");
  Serial.println("---------------------------------------------");
}

void loop() {
  unsigned long now = millis();

  // ── Baca sensor setiap 2 detik ────────────────────────────────────────
  if (now - tSensor >= INTERVAL_SENSOR_MS) {
    tSensor = now;

    float suhu = dht.readTemperature();
    if (!isnan(suhu)) {
      emaSuhu.update(suhu);
      emaHue.update(suhuKeHue(suhu));

      Serial.printf("[Sensor] Suhu: %5.1f°C  Warna: %s",
        emaSuhu.val, labelWarna(emaSuhu.val));
    } else {
      Serial.printf("[Sensor] Suhu: -- (Error DHT11)         ");
    }

    float ldrRaw  = analogRead(LDR_PIN);
    emaLDR.update(ldrRaw);
    int bright = (int)map((long)emaLDR.val, LDR_GELAP, LDR_TERANG,
                          BRIGHT_MIN, BRIGHT_MAX);
    bright = constrain(bright, BRIGHT_MIN, BRIGHT_MAX);
    emaBright.update(bright);

    Serial.printf("| LDR ADC: %4d  Bright: %3d/255\n",
      (int)emaLDR.val, (int)emaBright.val);
  }

  // ── Update LED 20×/detik untuk transisi mulus ─────────────────────────
  if (now - tLED >= INTERVAL_LED_MS) {
    tLED = now;

    uint8_t h = (uint8_t)constrain((int)emaHue.val, 0, 255);
    uint8_t b = (uint8_t)constrain((int)emaBright.val, BRIGHT_MIN, BRIGHT_MAX);

    leds[0] = CHSV(h, 240, b);
    FastLED.show();
  }
}
```

**Contoh output Serial Monitor:**
```text
=== BAB 36 Program 3: LED Reaktif Sensor ===
Suhu DHT11 → Warna LED | LDR → Kecerahan LED
---------------------------------------------
[Sensor] Suhu:  24.0°C  Warna: BIRU (Dingin)   | LDR ADC: 2847  Bright:  45/255
[Sensor] Suhu:  27.3°C  Warna: HIJAU (Normal)   | LDR ADC: 1203  Bright:  78/255
[Sensor] Suhu:  31.5°C  Warna: MERAH (Panas!)   | LDR ADC:  480  Bright: 108/255
```

---

## 36.7 Program 4: FSM Multi-Efek LED Interaktif

Program puncak di BAB 36 ini membangun **Finite State Machine (FSM) yang aman memori** untuk menavigasi empat jenis *mood* LED Single Pixel via kontrol otomatis (Timer) & manual (Tombol).

```cpp
/*
 * BAB 36 - Program 4: Multi-Efek LED Interaktif (FSM)
 *
 * Empat efek Single Pixel 100% Memori-Safe:
 *   STATE_RAINBOW     → Siklus rona pelangi memutar (default)
 *   STATE_BREATHING   → Napas merah darah tanda waspada (pulse)
 *   STATE_POLICE      → Sirine peringatan berkedip asimetris 
 *   STATE_SOLID       → Hijau siaga redup statis (idle)
 *
 * Navigasi efek:
 *   ▶ Otomatis ganti efek setiap 10 detik
 *   ▶ Tombol BTN (IO32, pull-down) → Ganti paksa (Manual Override)
 *
 * Hardware:
 *   WS2812B → IO12 (built-in 1 Pixel aman)
 *   Push Button → IO32 (via jumper header Custom, aktif HIGH)
 */

#include <FastLED.h>

// ── Konfigurasi Utama ────────────────────────────────────────────────────
#define LED_PIN        12
#define NUM_LEDS        1      // Kit = 1. (Jangan naikkan jika fisik tak ada)
#define LED_TYPE   WS2812B
#define COLOR_ORDER    GRB
#define MAX_BRIGHT      90

#define BTN_PIN        32      // Tombol navigasi efek
#define DEBOUNCE_MS    50UL

CRGB leds[NUM_LEDS];

// ── Definisi FSM State ───────────────────────────────────────────────────
enum LedState {
  STATE_RAINBOW,
  STATE_BREATHING,
  STATE_POLICE,
  STATE_SOLID,
  STATE_COUNT    // Total efek = 4
};
LedState currentState = STATE_RAINBOW;

const char* namaState[] = {
  "RAINBOW PIXEL", "RED BREATHE  ", "POLICE STROBE", "GREEN SOLID  "
};

// ── Variabel Tombol (Non-Blocking) ───────────────────────────────────────
bool    btnLastReading = false;
bool    btnState       = false;
bool    btnPressed     = false;
unsigned long tBtnChange = 0;

void updateButton(unsigned long now) {
  btnPressed = false;
  bool reading = (digitalRead(BTN_PIN) == HIGH);
  if (reading != btnLastReading) tBtnChange = now;
  if ((now - tBtnChange) >= DEBOUNCE_MS) {
    if (reading != btnState) {
      btnState = reading;
      if (btnState) btnPressed = true;
    }
  }
  btnLastReading = reading;
}

// ── Timer Global ─────────────────────────────────────────────────────────
unsigned long tAutoSwitch      = 0;
const unsigned long AUTO_MS    = 10000UL; // Timer 10 detik

// ── Modul Efek: Rainbow ──────────────────────────────────────────────────
uint8_t rainbowHue = 0;
unsigned long tRainbow = 0;
void updateRainbow(unsigned long now) {
  if (now - tRainbow < 30UL) return; // Kecepatan putar lambat
  tRainbow = now;
  // Karena Single Pixel, cukup set warna tunggal memutar
  fill_solid(leds, NUM_LEDS, CHSV(rainbowHue, 255, MAX_BRIGHT));
  FastLED.show();
  rainbowHue += 2; // Makin besar jedanya, makin drastis pergerakannya
}

// ── Modul Efek: Breathing Merah ──────────────────────────────────────────
uint16_t breathAngle = 0;
unsigned long tBreath = 0;
void updateBreathing(unsigned long now) {
  if (now - tBreath < 15UL) return; 
  tBreath = now;
  // Skala ke-255
  uint8_t angle8 = (uint8_t)(breathAngle * 255UL / 1535UL);
  uint8_t bright = map(sin8(angle8), 0, 255, 5, MAX_BRIGHT);
  fill_solid(leds, NUM_LEDS, CHSV(0, 255, bright)); // Hue 0 = Merah
  FastLED.show();
  breathAngle = (breathAngle + 12) % 1536;
}

// ── Modul Efek: Police Strobe (Sirine) ───────────────────────────────────
uint8_t pStep = 0;
unsigned long tStrobe = 0;
void updatePolice(unsigned long now) {
  unsigned long jeda = (pStep % 2 == 1) ? 60 : (pStep >= 4 && pStep <= 5) ? 200 : 40;
  if (now - tStrobe < jeda) return;
  tStrobe = now;

  if (pStep < 4) { // Siklus kilat merah ganda
    fill_solid(leds, NUM_LEDS, (pStep % 2 == 0) ? CHSV(0, 255, MAX_BRIGHT) : CRGB::Black);
  } else if (pStep > 5 && pStep < 10) { // Siklus kilat biru cerah ganda
    fill_solid(leds, NUM_LEDS, (pStep % 2 == 0) ? CHSV(160, 255, MAX_BRIGHT) : CRGB::Black);
  } else {
    fill_solid(leds, NUM_LEDS, CRGB::Black);
  }
  FastLED.show();

  pStep++;
  if (pStep >= 11) pStep = 0;
}

// ── Modul Efek: Solid Standby ────────────────────────────────────────────
void drawSolid() {
  fill_solid(leds, NUM_LEDS, CHSV(96, 255, 30)); // Hijau gelap penanda idle
  FastLED.show();
}

// ── Sistem Transisi ──────────────────────────────────────────────────────
void switchToState(LedState newState) {
  currentState = newState;
  FastLED.clear(); // Black-out sepersekian detik antar scene
  FastLED.show();

  if (newState == STATE_SOLID) drawSolid(); 

  Serial.printf("[FSM] Mengalihkan Mood ke: %s\n", namaState[newState]);
}

void setup() {
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(MAX_BRIGHT);
  FastLED.clear(); FastLED.show();

  Serial.println("=== BAB 36 Program 4: FSM LED ===");
  Serial.println("Tekan IO32 untuk melompat navigasi secara manual!");
}

void loop() {
  unsigned long now = millis();

  updateButton(now);

  // Memicu paksa transisi ketika tombol diklik penuh (Override Mode)
  if (btnPressed) {
    switchToState((LedState)((currentState + 1) % STATE_COUNT));
    tAutoSwitch = now; // Cegah lompatan dobel jika timer baru saja mau habis
  }

  // Auto-cruise mode
  if (now - tAutoSwitch >= AUTO_MS) {
    tAutoSwitch = now;
    switchToState((LedState)((currentState + 1) % STATE_COUNT));
  }

  // Merender mesin status yg berkuasa (State Director)
  switch (currentState) {
    case STATE_RAINBOW:   updateRainbow(now); break;
    case STATE_BREATHING: updateBreathing(now); break;
    case STATE_POLICE:    updatePolice(now); break;
    case STATE_SOLID:     /* Idle mode */ break;
  }
}
```

> 💡 **Bug Memori Pada String Cahaya Besar (Peringatan Khusus):** Banyak tutorial online menggunakan efek memori berat seperti *Fire2012 Array*. Ingat, kode buffer array statis seperti `byte heat[NUM_LEDS]` **pasti menabrak luar batas** (`OutOfBound memory fault`) dan me-restart *panic* ESP32 Anda jika Anda mencoba membajalannya dengan `NUM_LEDS` bernilai `1`! FSM Single-Pixel di atas adalah model yang 100% aman untuk hardware Bluino Anda.

---

## 36.8 Tips & Panduan Troubleshooting

### 1. LED tidak menyala / warna salah?
```text
Penyebab paling umum:

A. COLOR_ORDER salah (WS2812 vs WS2812B berbeda!)
   WS2812  = RGB order → COLOR_ORDER GRB (berlaku keduanya umumnya)
   Jika warna tampak keliru (merah muncul sebagai hijau):
   Coba ganti COLOR_ORDER dari GRB ke RGB atau BGR

B. Library belum terpasang atau versi lama
   → Pastikan FastLED ≥ 3.6.0 ter-install

C. FastLED.show() lupa dipanggil
   → Modifikasi leds[] tidak tampil tanpa show()!

D. Tegangan tidak stabil
   → WS2812 butuh 5V stabil. Jika banyak LED, tambahkan
     kapasitor elco 1000µF 16V di jalur 5V supply!
```

### 2. ESP32 hang / restart saat LED dinyalakan?
```text
Penyebab: Kelebihan konsumsi arus saat semua LED menyala putih penuh.

1 LED WS2812 putih 100%   = ~60 mA
8 LED WS2812 putih 100%   = ~480 mA
USB PC hanya mampu        = ~500 mA total (termasuk ESP32 ~100mA!)

SOLUSI WAJIB:
  A. Batasi kecerahan: FastLED.setBrightness(80)  // Max 80 untuk USB
  B. Hindari fill_solid(leds, N, CRGB::White) tanpa batas kecerahan
  C. Untuk instalasi permanen, gunakan adaptor 5V 3A atau lebih
  D. Tambahkan resistor 330-500Ω di jalur DIN untuk proteksi sinyal
```

### 3. Animasi terlihat patah-patah / stuttering?
```text
Penyebab: FastLED.show() memblokir CPU selama pengiriman data I2C.
Durasi blokir: ~30µs per LED (untuk 8 LED = ~240µs)

Solusi:
  A. Jangan panggil show() lebih dari 50-100 kali/detik untuk 8 LED
  B. Atur interval update minimum 10ms:
     if (now - tLast >= 10UL) { tLast = now; ... show(); }
  C. Hindari Serial.print() di dalam loop update LED yang cepat
     (Serial lambat, bisa menyebabkan lag)
```

### 4. IO12 dan strapping pin — apakah aman?
```text
IO12 = MTDI strapping pin pada ESP32.

Saat proses upload (Flash mode), IO12 harus LOW.
Bluino Kit sudah memiliki rangkaian pull-down di IO12 untuk ini.

AMAN untuk digunakan selama operasi normal setelah boot.
JANGAN tambahkan pull-up resistor eksternal ke IO12 — ini
akan menyebabkan ESP32 gagal masuk mode flash!

Jika mengalami gagal upload, coba:
  1. Tahan tombol BOOT (IO0)
  2. Tekan dan lepas tombol RESET
  3. Lepas tombol BOOT → ESP32 siap menerima firmware baru
```

### 5. `HeatColor()` menghasilkan warna aneh?
```text
HeatColor(value) memetakan 0-255 ke:
  0–85  → Hitam ke Merah
  85–170 → Merah ke Kuning
  170–255 → Kuning ke Putih

Jika api terlihat ungu/biru, pastikan:
  COLOR_ORDER = GRB (bukan RGB)
  heat[] hanya diisi nilai 0-255
  Tidak ada nilai negatif (gunakan byte, bukan int untuk heat[])
```

---

## 36.9 Ringkasan

```
┌──────────────────────────────────────────────────────────────────────┐
│              RINGKASAN BAB 36 — RGB LED WS2812 (NeoPixel)            │
├──────────────────────────────────────────────────────────────────────┤
│ HARDWARE & PROTOKOL:                                                  │
│   WS2812B Controller → Dikontrol via protokol 1-wire timing ketat    │
│   IC tertanam di tiap LED → daisy-chain N LED dengan 1 pin saja      │
│   IO12 di Bluino Kit     → Pin DATA WS2812 (aman setelah boot)       │
│   24-bit per LED         → 8b Green + 8b Red + 8b Blue (GRB order)  │
│   ESP32 RMT Engine       → Hardware timer untuk timing akurat 100%  │
│                                                                       │
│ SISTEM WARNA:                                                         │
│   RGB: CRGB(r, g, b) atau CRGB::Red, Green, ...                     │
│   HSV: CHSV(hue, sat, val) → lebih intuitif untuk efek & animasi    │
│   Hue 0-255 = roda warna melingkar (merah→kuning→hijau→biru→...)    │
│                                                                       │
│ FUNGSI UTAMA FastLED:                                                 │
│   FastLED.setBrightness(n) → Skala kecerahan global (0-255)          │
│   FastLED.show()           → Kirim buffer ke LED (WAJIB dipanggil!)  │
│   FastLED.clear()          → Set semua LED ke hitam (off)            │
│   fill_solid(leds, N, warna)  → Warnai N LED dengan warna sama       │
│   fadeToBlackBy(N)            → Redup per piksel (efek ekor lembut)  │
│                                                                       │
│ BATASAN DAYA (KRITIS!):                                               │
│   1 LED putih penuh = ±60mA                                           │
│   Untuk USB: FastLED.setBrightness(80) → JANGAN lebih dari 100!     │
│   Instalasi banyak LED → Wajib power supply 5V 2A+ terpisah         │
│                                                                       │
│ POLA DESAIN (SINGLE LED):                                             │
│   ✅ FSM untuk manajemen multi-efek LED (enum + switch)              │
│   ✅ EMA filter untuk transisi warna sensor yang mulus               │
│   ✅ Timer asimetris — Durasi jeda nyala & mati tidak konstan!       │
│   ✅ Amankan index memori array jika mem-port kode LED-Strip panjang │
└──────────────────────────────────────────────────────────────────────┘
```

---

## 36.10 Latihan

1. **Indicator Kipas Angin:** Modifikasi Program 3 agar warna LED merepresentasikan kecepatan kipas virtual: baca ADC potensiometer (0-4095) dan petakan ke: Biru (berhenti) → Cyan (pelan) → Hijau (sedang) → Kuning (cepat) → Merah (turbo). Tambahkan Breathing speed yang juga berubah sesuai "kecepatan" — makin cepat, makin cepat napasnya!

2. **Morse Code Beeper:** Buat program translasi SOS dimana Single LED WS2812 memancarkan sinyal S-O-S dengan warna solid oranye yang tajam. `Dit` = 200ms, `Dah` = 600ms, dan jeda karakter = 200ms, tanpa menggunakan fungsi `delay()`.

3. **Sensor Alarm Bertingkat (Police Trigger):** Kembangkan Program 3 untuk mendeteksi 3 level suhu dengan respons LED yang berbeda: <25°C = biru pernapasan lambat, 25-32°C = hijau solid, >32°C = FSM pindah ke mode `updatePoliceStrobe(now)` seketika sebagai alarm tanda bahaya kebakaran.

4. **Rainbow dengan Kecepatan Variabel:** Modifikasi efek Rainbow di Program 4 agar kecepatan putaran warnanya (*Hue cycle*) dikontrol oleh potensiometer (ADC IO34). Saat potensiometer di MIN → pelangi hampir berhenti berpendar. Saat di MAX → pelangi memutar siklusnya sangat cepat.

5. **RGB Dimmer Mixer:** Jika kamu memiliki 3 komponen Potensiometer, bacalah masing-masing ADC-nya dan jadikan sebagai input manual `CHSV(pot1, pot2, pot3)` atau `CRGB(pot1, pot2, pot3)`. Petakan nilai 0-4095 ke kisaran 0-255 sehingga Bluino berubah fungsi menjadi konsol *Color Tuner*!

---

## ☕ Dukung Proyek Ini

Bab ini disusun dengan dedikasi waktu dan keahlian yang tidak sedikit. Jika materi *engineering-grade* ini bermanfaat bagi riset atau proses belajarmu, bentuk apresiasi terbaik yang bisa kamu berikan adalah secangkir kopi untuk penulis:

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
Punya kritik, saran, atau butuh mentoring khusus ESP32 & IoT industri? Silakan sapa saya di:
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

---

*Sebelumnya → [BAB 35: OLED Display 128×64](../BAB-35/README.md)*

*Selanjutnya → [BAB 37: Relay — Kontrol AC/DC](../BAB-37/README.md)*
