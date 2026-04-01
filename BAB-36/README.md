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

## 36.5 Program 2: Efek Animasi — Breathing & Knight Rider

Program ini memperkenalkan dua teknik animasi fundamental: **Breathing** (napas — memudar masuk-keluar secara sinusoidal) dan **Knight Rider** (efek scanning-light dari serial LED). Keduanya menggunakan `millis()` tanpa `delay()` sama sekali.

```cpp
/*
 * BAB 36 - Program 2: Efek Animasi LED (Breathing + Knight Rider)
 *
 * Fitur:
 *   ▶ Efek Breathing: Kecerahan naik-turun seperti napas (sin wave)
 *   ▶ Efek Knight Rider: Titik cahaya bergerak bolak-balik (scanning)
 *   ▶ Toggle efek setiap 8 detik secara otomatis
 *   ▶ Sepenuhnya non-blocking (tanpa delay())
 *
 * Hardware:
 *   WS2812B → IO12 (built-in Bluino Kit)
 *   Jumlah LED bisa dikonfigurasi (ganti NUM_LEDS)
 */

#include <FastLED.h>

// ── Konfigurasi ─────────────────────────────────────────────────────────
#define LED_PIN     12
#define NUM_LEDS    8      // Ubah sesuai jumlah LED di strip kamu
#define LED_TYPE    WS2812B
#define COLOR_ORDER GRB
#define MAX_BRIGHT  100    // Kecerahan maksimum (jaga dibawah 150 untuk USB)

CRGB leds[NUM_LEDS];

// ── State Efek ───────────────────────────────────────────────────────────
enum EffectState { FX_BREATHING, FX_KNIGHT_RIDER };
EffectState fxState = FX_BREATHING;

// ── Timing ───────────────────────────────────────────────────────────────
const unsigned long INTERVAL_FX_SWITCH = 8000UL; // Ganti efek tiap 8 detik
const unsigned long INTERVAL_BREATH_MS = 20UL;   // Update breathing (50Hz)
const unsigned long INTERVAL_KR_MS     = 60UL;   // Update knight rider (16fps)
unsigned long tFxSwitch = 0;
unsigned long tBreath   = 0;
unsigned long tKR       = 0;

// ── Breathing State ──────────────────────────────────────────────────────
uint16_t breathAngle = 0;   // "Sudut" dalam lingkaran 0-1535 (360 langkah fine)
uint8_t  breathHue   = 160; // Biru tua (bisa diubah warna yang lain)

// Fungsi aproksimasi sin8 FastLED → menghasilkan 0-255
// sin8(angle): angle 0-255 mewakili 0°-360°
uint8_t hitungKecerahan(uint16_t fineAngle) {
  // fineAngle 0-1535, scale ke 0-255
  uint8_t angle8 = (uint8_t)(fineAngle * 255UL / 1535UL);
  // sin8 menghasilkan offset 0-255 (bukan -128 s/d 127)
  // Nilai terkecil ~0, tertinggi ~255
  uint8_t s = sin8(angle8);
  // Skala ke 20-MAX_BRIGHT (jangan sampai mati total di titik terbawah)
  return map(s, 0, 255, 20, MAX_BRIGHT);
}

// ── Knight Rider State ───────────────────────────────────────────────────
int     krPos  = 0;   // Posisi titik cahaya
int     krDir  = 1;   // Arah: +1 = kanan, -1 = kiri
uint8_t krHue  = 0;   // Hue merah (Knight Rider klasik!)

// ── Fungsi Efek: Breathing ───────────────────────────────────────────────
void updateBreathing(unsigned long now) {
  if (now - tBreath < INTERVAL_BREATH_MS) return;
  tBreath = now;

  uint8_t bright = hitungKecerahan(breathAngle);
  // Warnai semua LED dengan warna dan kecerahan yang sama
  fill_solid(leds, NUM_LEDS, CHSV(breathHue, 230, bright));
  FastLED.show();

  breathAngle = (breathAngle + 8) % 1536; // Maju 8 langkah per frame
}

// ── Fungsi Efek: Knight Rider ────────────────────────────────────────────
void updateKnightRider(unsigned long now) {
  if (now - tKR < INTERVAL_KR_MS) return;
  tKR = now;

  // Redup semua LED (fade out) — membuat ekor cahaya berpendar
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].fadeToBlackBy(80); // Kurangi kecerahan 80/256 per frame
  }

  // Tentukan posisi kepala cahaya
  leds[krPos] = CHSV(krHue, 255, MAX_BRIGHT);

  // Refleksi di tepi (bounce back)
  krPos += krDir;
  if (krPos >= NUM_LEDS - 1) { krPos = NUM_LEDS - 1; krDir = -1; }
  if (krPos <= 0)             { krPos = 0;             krDir =  1; }

  FastLED.show();
}

void setup() {
  Serial.begin(115200);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(MAX_BRIGHT);
  FastLED.clear();
  FastLED.show();

  Serial.println("=== BAB 36 Program 2: Efek Animasi ===");
  Serial.println("Mulai: Breathing Effect (biru)");
}

void loop() {
  unsigned long now = millis();

  // ── Toggle efek otomatis setiap 8 detik ──────────────────────────────
  if (now - tFxSwitch >= INTERVAL_FX_SWITCH) {
    tFxSwitch = now;
    if (fxState == FX_BREATHING) {
      fxState = FX_KNIGHT_RIDER;
      FastLED.clear();
      Serial.println("Berganti ke: Knight Rider Effect (merah)");
    } else {
      fxState = FX_BREATHING;
      breathAngle = 0;
      Serial.println("Berganti ke: Breathing Effect (biru)");
    }
  }

  // ── Eksekusi efek aktif ───────────────────────────────────────────────
  if (fxState == FX_BREATHING) {
    updateBreathing(now);
  } else {
    updateKnightRider(now);
  }
}
```

> 💡 **Teknik `fadeToBlackBy()`:** Ini adalah teknik "ekor cahaya" (light trail) paling umum di animasi LED strip. Setiap frame, semua piksel dikurangi kecerahannya dengan nilai konstan. Hasilnya: piksel lama "memudar" secara alami sementara titik baru tetap terang penuh. Semakin besar angkanya (0-255), semakin cepat ekornya menghilang!

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

## 36.7 Program 4: Efek Rainbow & Running Fire (FSM Multi-Efek)

Program tercanggih di BAB 36 ini membangun **sistem manajemen efek LED berbasis Finite State Machine (FSM)** lengkap dengan 4 efek visual yang dapat dipilih secara otomatis maupun via tombol.

```cpp
/*
 * BAB 36 - Program 4: Multi-Efek LED (FSM)
 *
 * Empat efek dalam satu program:
 *   STATE_RAINBOW     → Pelangi berputar (mode default)
 *   STATE_BREATHING   → Napas warna merah (slow pulse)
 *   STATE_FIRE        → Api bergerak dari bawah ke atas (untuk strip ≥8 LED)
 *   STATE_SOLID       → Warna solid tunggal (biru pudar — idle/standby)
 *
 * Navigasi efek:
 *   ▶ Otomatis ganti efek setiap 10 detik
 *   ▶ Tombol BTN (IO32, pull-down) → ganti efek manual
 *
 * Hardware:
 *   WS2812B → IO12 (built-in)
 *   Push Button → IO32 (via jumper header Custom, aktif HIGH)
 */

#include <FastLED.h>

// ── Konfigurasi ─────────────────────────────────────────────────────────
#define LED_PIN        12
#define NUM_LEDS        8      // Sesuaikan dengan strip LED-mu
#define LED_TYPE   WS2812B
#define COLOR_ORDER    GRB
#define MAX_BRIGHT      90

#define BTN_PIN        32      // Tombol scroll (pull-down, aktif HIGH)
#define DEBOUNCE_MS    50UL

CRGB leds[NUM_LEDS];

// ── FSM ─────────────────────────────────────────────────────────────────
enum LedState {
  STATE_RAINBOW,
  STATE_BREATHING,
  STATE_FIRE,
  STATE_SOLID,
  STATE_COUNT    // Penanda jumlah state (untuk modulo wrap-around)
};
LedState currentState = STATE_RAINBOW;

const char* namaState[] = {
  "RAINBOW   ", "BREATHING ", "FIRE      ", "SOLID BLUE"
};

// ── Tombol Debounce ──────────────────────────────────────────────────────
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

// ── Timing Global ────────────────────────────────────────────────────────
unsigned long tAutoSwitch      = 0;
const unsigned long AUTO_MS    = 10000UL; // Ganti efek otomatis tiap 10 detik

// ── State Rainbow ────────────────────────────────────────────────────────
uint8_t rainbowHue = 0;
unsigned long tRainbow = 0;

void updateRainbow(unsigned long now) {
  if (now - tRainbow < 20UL) return; // 50Hz
  tRainbow = now;
  // Isi semua LED dengan hue berbeda-beda, bergeser tiap frame
  fill_rainbow(leds, NUM_LEDS, rainbowHue, 256 / NUM_LEDS);
  FastLED.show();
  rainbowHue++; // overflow uint8_t → 0-255 cyclic
}

// ── State Breathing ──────────────────────────────────────────────────────
uint16_t breathAngle = 0;
unsigned long tBreath = 0;

void updateBreathing(unsigned long now) {
  if (now - tBreath < 18UL) return; // ~55Hz
  tBreath = now;
  uint8_t angle8 = (uint8_t)(breathAngle * 255UL / 1535UL);
  uint8_t bright = map(sin8(angle8), 0, 255, 10, MAX_BRIGHT);
  fill_solid(leds, NUM_LEDS, CHSV(0, 255, bright)); // Merah
  FastLED.show();
  breathAngle = (breathAngle + 10) % 1536;
}

// ── State Fire (Simulasi Api) ─────────────────────────────────────────────
// Algoritma: setiap sel memiliki "suhu" acak, meredam ke atas
// Referensi: Mark Kriegsman "Fire2012" (disesuaikan untuk ESP32)
byte heat[NUM_LEDS]; // Buffer "suhu" setiap LED
unsigned long tFire = 0;

void updateFire(unsigned long now) {
  if (now - tFire < 25UL) return; // 40Hz
  tFire = now;

  // Step 1: Redam panas setiap sel sedikit
  for (int i = 0; i < NUM_LEDS; i++) {
    heat[i] = qsub8(heat[i], random8(0, 55));
  }

  // Step 2: Panaskan "naik" (dari bawah ke atas — difusi)
  for (int k = NUM_LEDS - 1; k >= 2; k--) {
    heat[k] = (heat[k - 1] + heat[k - 2] + heat[k - 2]) / 3;
  }

  // Step 3: Percikan api baru secara acak di pangkal bawah
  if (random8() < 120) {
    int y = random8(3);
    heat[y] = qadd8(heat[y], random8(160, 255));
  }

  // Step 4: Peta "panas" ke warna HeatColor
  for (int j = 0; j < NUM_LEDS; j++) {
    leds[j] = HeatColor(heat[j]);
  }
  FastLED.show();
}

// ── State Solid (Idle) ────────────────────────────────────────────────────
void drawSolid() {
  fill_solid(leds, NUM_LEDS, CHSV(160, 180, 40)); // Biru redup
  FastLED.show();
}

// ── Fungsi Transisi State ─────────────────────────────────────────────────
void switchToState(LedState newState) {
  currentState = newState;
  FastLED.clear(); // Bersihkan layar sebelum efek baru
  FastLED.show();

  // Inisialisasi ulang buffer efek baru
  if (newState == STATE_FIRE) {
    memset(heat, 0, sizeof(heat));
  }
  if (newState == STATE_SOLID) {
    drawSolid(); // Gambar langsung — solid tidak butuh animasi loop
  }

  Serial.printf("[FSM] Efek aktif: %s\n", namaState[newState]);
}

void setup() {
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT);

  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  FastLED.setBrightness(MAX_BRIGHT);
  FastLED.clear();
  FastLED.show();

  Serial.println("=== BAB 36 Program 4: Multi-Efek LED (FSM) ===");
  Serial.printf("[FSM] Efek awal: %s\n", namaState[currentState]);
  Serial.println("Tekan tombol BTN (IO32) untuk ganti efek manual.");
}

void loop() {
  unsigned long now = millis();

  // ── Baca tombol ─────────────────────────────────────────────────────────
  updateButton(now);
  if (btnPressed) {
    LedState next = (LedState)((currentState + 1) % STATE_COUNT);
    switchToState(next);
    tAutoSwitch = now; // Reset timer auto-switch juga
  }

  // ── Auto-switch efek setiap 10 detik ────────────────────────────────────
  if (now - tAutoSwitch >= AUTO_MS) {
    tAutoSwitch = now;
    LedState next = (LedState)((currentState + 1) % STATE_COUNT);
    switchToState(next);
  }

  // ── Jalankan efek aktif ──────────────────────────────────────────────────
  switch (currentState) {
    case STATE_RAINBOW:  updateRainbow(now);  break;
    case STATE_BREATHING: updateBreathing(now); break;
    case STATE_FIRE:     updateFire(now);     break;
    case STATE_SOLID:    /* Statis, tidak perlu update loop */ break;
  }
}
```

> 💡 **Algoritma Fire2012:** Teknik simulasi api ini menggunakan array suhu (`heat[]`) yang setiap frame: (1) didinginkan secara acak, (2) disebar ke atas seperti konduksi panas, lalu (3) dipercik dari bawah. Nilai suhu dipetakan ke warna via `HeatColor()` — fungsi bawaan FastLED yang menghasilkan palet hitam → merah → kuning → putih yang identik dengan api nyata!

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
│   fill_rainbow(leds, N, h, d) → Isi rantai dengan pelangi           │
│   fadeToBlackBy(N)            → Redup per piksel (efek ekor cahaya)  │
│   HeatColor(panas)            → Palet api (untuk efek fire)          │
│                                                                       │
│ BATASAN DAYA (KRITIS!):                                               │
│   1 LED putih penuh = ±60mA                                           │
│   Untuk USB: FastLED.setBrightness(80) → JANGAN lebih dari 100!     │
│   Instalasi banyak LED → Wajib power supply 5V 2A+ terpisah         │
│                                                                       │
│ POLA DESAIN:                                                          │
│   ✅ FSM untuk manajemen multi-efek (enum + switch)                  │
│   ✅ EMA filter untuk transisi warna sensor yang mulus               │
│   ✅ Timer millis() — JANGAN delay() di dalam loop animasi!          │
│   ✅ fadeToBlackBy() untuk efek ekor cahaya alami                    │
│   ✅ sin8() / HeatColor() untuk kurva kecerahan yang natural         │
└──────────────────────────────────────────────────────────────────────┘
```

---

## 36.10 Latihan

1. **Indicator Kipas Angin:** Modifikasi Program 3 agar warna LED merepresentasikan kecepatan kipas virtual: baca ADC potensiometer (0-4095) dan petakan ke: Biru (berhenti) → Cyan (pelan) → Hijau (sedang) → Kuning (cepat) → Merah (turbo). Tambahkan Breathing speed yang juga berubah sesuai "kecepatan" — makin cepat, makin cepat napasnya!

2. **Countdown Timer Visual:** Buat program timer 60 detik dimana semua LED (strip ≥8) mati satu per satu setiap 7.5 detik dari kiri ke kanan. LED yang masih menyala berwarna hijau, LED yang sudah mati berwarna merah redup. Saat hitungan selesai, semua LED berkedip merah 5x lalu mati.

3. **Sensor Alarm Bertingkat:** Kembangkan Program 3 untuk mendeteksi 3 level suhu dengan respons LED yang berbeda: <25°C = biru bernapas lambat, 25-32°C = hijau solid, >32°C = merah berkedip cepat setiap 200ms (alarm). Saat alarm aktif, cetak pesan darurat ke Serial Monitor setiap kedip.

4. **Rainbow dengan Kecepatan Variabel:** Modifikasi efek Rainbow di Program 4 agar kecepatan putarannya dikontrol oleh potensiometer (ADC IO34). Saat potensiometer di MIN → pelangi hampir berhenti. Saat di MAX → pelangi berputar sangat cepat. Hitung dan tampilkan "RPM virtual" pelangi ke Serial Monitor setiap detik.

5. **Dancing Fire:** Modifikasi efek Fire di Program 4 dengan tambahan fitur: nyalakan musik/beat detector virtual via threshold ADC — setiap kali nilai ADC potensiometer melewati 2048 ke atas, perkuat percikan api (naikkan probabilitas `random8()` dan intensitas `heat[y]`) selama 200ms. Efeknya: api "berdenyut" seolah bereaksi terhadap ritme musik!

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
