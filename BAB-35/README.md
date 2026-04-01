# BAB 35: OLED Display 128×64

> ✅ **Prasyarat:** Selesaikan **BAB 23 (I2C — Inter-Integrated Circuit)**, **BAB 26 (DHT11)**, dan **BAB 34 (Pemrosesan Data Sensor)**. OLED sudah terpasang permanen di Bluino Kit via I2C (SDA=IO21, SCL=IO22) — tidak perlu konfigurasi hardware tambahan!

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami **prinsip kerja OLED SSD1306** dan perbedaannya dengan LCD biasa.
- Menggunakan library **Adafruit SSD1306** dan **Adafruit GFX** secara profesional.
- Menampilkan **teks, angka, ikon, dan garis** pada layar 128×64 piksel.
- Membangun **dashboard sensor real-time** berbasis OLED yang informatif.
- Menggambar **grafik bar** dinamis untuk visualisasi data ADC.
- Membuat **sistem menu interaktif** dengan navigasi Push Button — tanpa library tambahan.
- Menerapkan teknik **double buffering** dan update selektif untuk performa non-blocking.

---

## 35.1 OLED SSD1306 — Mengapa Beda dari LCD Biasa?

### Perbandingan Teknologi Display

```
PERBANDINGAN LCD 16×2 vs OLED 128×64:

  LCD 16×2 (Character LCD):              OLED 128×64 (Graphic):
  ┌──────────────────────┐               ┌────────────────────────────────┐
  │ Hello, World!        │               │ ░░ SENSOR DASHBOARD ░░░░░░░░░░ │
  │ Temp: 27.5C  Hum:65% │               │ ▸ Suhu : 27.5°C                │
  └──────────────────────┘               │ ▸ Humid: 65%                   │
                                         │ ▸ Press: 1013 hPa              │
  ▶ Hanya teks & karakter               │ ████████████████░░░░  80%      │
  ▶ Resolusi 16 kolom × 2 baris         └────────────────────────────────┘
  ▶ Backlight terpisah (boros daya)
  ▶ Tidak bisa gambar garis/grafik       ▶ Pixel-addressable (128×64=8192 piksel)
  ▶ Antarmuka paralel 8-bit atau I2C     ▶ Gambar apa saja: teks, ikon, grafik
                                         ▶ Self-emissive — TIDAK butuh backlight!
                                         ▶ Kontras sangat tinggi (hitam murni)
                                         ▶ Antarmuka I2C atau SPI
                                         ▶ Konsumsi daya sangat rendah (~4mA)
```

### Cara Kerja OLED (Organic Light-Emitting Diode)

Tidak seperti LCD yang membutuhkan lampu latar belakang (*backlight*), setiap piksel OLED memancarkan cahayanya **sendiri-sendiri** secara organik ketika dialiri arus listrik kecil. Piksel yang "OFF" benar-benar mati (hitam total), sehingga kontras rasionya jauh melampaui LCD konvensional.

```
ARSITEKTUR SISTEM OLED SSD1306:

  ESP32                 SSD1306 Controller         Panel OLED
  ┌────────┐            ┌──────────────────┐        ┌──────────────────┐
  │        │  I2C Bus   │                  │ Drive  │ 128 × 64 piksel  │
  │  IO21  │────SDA────▶│  Display Buffer  │───────▶│  self-emitting   │
  │  IO22  │────SCL────▶│  (GDDRAM)        │        │  ~0.96" panel    │
  │        │            │  128 × 64 bits   │        │                  │
  └────────┘            │  = 1024 bytes    │        └──────────────────┘
                        └──────────────────┘
                                 ▲
                         Kita menulis data
                         ke buffer ini lewat
                         library Adafruit!
```

> 💡 **GDDRAM (Graphic Display Data RAM):** Inti dari SSD1306 adalah buffer memori 1024 byte (128×64 bit). Setiap bit mewakili satu piksel. Library Adafruit SSD1306 mengelola buffer ini di sisi ESP32, lalu mengirimkan seluruh isinya via I2C ke chip display setiap kali kamu memanggil `display.display()`.

---

## 35.2 Instalasi Library & Konfigurasi Awal

### Library yang Dibutuhkan

Kamu membutuhkan dua library dari Adafruit:

```
CARA INSTALASI (Arduino IDE 2.x):
  1. Tools → Manage Libraries...
  2. Cari: "Adafruit SSD1306"     → Install
  3. Cari: "Adafruit GFX Library" → Install
     (akan otomatis ter-install sebagai dependency)
  4. Restart Arduino IDE

Versi yang direkomendasikan:
  - Adafruit SSD1306  ≥ 2.5.7
  - Adafruit GFX      ≥ 1.11.5
```

### Konfigurasi Awal Wajib

```cpp
// ── Inisialisasi OLED — Wajib Ada di Setiap Program ──────────────────
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Dimensi layar (sesuai hardware kit Bluino)
#define SCREEN_W  128
#define SCREEN_H   64

// Alamat I2C default SSD1306 — 0x3C (kit Bluino)
// Jika tidak muncul, coba 0x3D atau jalankan I2C scanner
#define OLED_ADDR 0x3C

// RESET_PIN = -1 artinya berbagi pin reset dengan ESP32
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

void setup() {
  Serial.begin(115200);
  
  // Inisialisasi I2C (Wire) — opsional karena Wire sudah auto-init
  // Wire.begin(21, 22); // SDA=IO21, SCL=IO22 (sudah default di ESP32)
  
  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    // Jika gagal — cek kabel I2C atau coba alamat 0x3D
    Serial.println("⚠️ SSD1306 tidak ditemukan! Cek kabel SDA/SCL.");
    while (true); // Hentikan program
  }
  
  display.clearDisplay();   // Hapus buffer (isi dengan hitam semua)
  display.display();        // Kirim buffer ke layar (tampilkan hitam)
}
```

> ⚠️ **Catatan Kritis:** Selalu panggil `display.clearDisplay()` diikuti `display.display()` pada awal `setup()`. Ini memastikan layar bersih dari artefak data lama yang mungkin tersisa di GDDRAM chip.

---

## 35.3 Sistem Koordinat & Konsep Menggambar

Sebelum menulis kode, *pahami sistem koordinat OLED dengan sangat baik* — ini adalah sumber error terbanyak bagi pemula!

```
SISTEM KOORDINAT OLED SSD1306 (128 × 64):

  (0,0) ──────────────────────────── (127,0)
    │  X bertambah ke kanan →              │
    │  Y bertambah ke bawah ↓              │
    │                                      │
    │  display.drawPixel(x, y, WHITE)      │
    │                                      │
    │  display.setCursor(x, y)             │
    │  → Meletakkan "kursor teks"          │
    │    di titik (x,y) — POJOK KIRI ATAS  │
    │    dari karakter teks                │
    │                                      │
  (0,63) ─────────────────────────── (127,63)

PEMBAGIAN AREA  (untuk desain yang rapi):
  Header  → Y: 0–12   (tinggi 13px, cukup untuk font 1×)
  Content → Y: 14–52  (tinggi 39px — area utama)
  Footer  → Y: 54–63  (tinggi 10px — status bar)
```

### Primitif Drawing yang Tersedia (Adafruit GFX)

```cpp
// ── TEKS ───────────────────────────────────────────────────────────────
display.setTextSize(1);            // Ukuran 1 = 6×8 piksel per karakter
display.setTextColor(WHITE);       // Warna: WHITE atau BLACK
display.setCursor(0, 0);           // Posisi awal teks
display.print("Hello!");           // Cetak teks (sama seperti Serial.print)
display.println("World");          // Cetak teks + newline

// ── GARIS & KOTAK ──────────────────────────────────────────────────────
display.drawLine(x0, y0, x1, y1, WHITE);          // Garis lurus
display.drawRect(x, y, w, h, WHITE);              // Kotak kosong
display.fillRect(x, y, w, h, WHITE);              // Kotak padat (solid)
display.drawRoundRect(x, y, w, h, r, WHITE);      // Kotak sudut bulat
display.fillRoundRect(x, y, w, h, r, WHITE);      // Kotak bulat solid

// ── LINGKARAN ──────────────────────────────────────────────────────────
display.drawCircle(cx, cy, r, WHITE);             // Lingkaran kosong
display.fillCircle(cx, cy, r, WHITE);             // Lingkaran padat

// ── PIKSEL TUNGGAL ──────────────────────────────────────────────────────
display.drawPixel(x, y, WHITE);                   // Satu piksel
display.drawPixel(x, y, BLACK);                   // Hapus piksel

// ── INVERT WARNA ───────────────────────────────────────────────────────
display.invertDisplay(true);   // Invert seluruh layar (negatif)
display.invertDisplay(false);  // Kembali normal

// ── MANDATORY: Kirim buffer ke layar ───────────────────────────────────
display.display();  // WAJIB dipanggil agar perubahan terlihat!
```

> 💡 **Konsep Buffer Penting:** Semua fungsi di atas hanya memodifikasi **buffer di RAM ESP32**. Piksel di layar OLED belum berubah sampai kamu memanggil `display.display()`. Ini adalah arsitektur *double-buffering* — sangat efisien untuk animasi dan update berkala.

---

## 35.4 Ukuran Font & Layout Grid

```
PANDUAN UKURAN FONT (Adafruit GFX Built-in):

  setTextSize(1):  6w × 8h piksel per karakter
    → Muat: 128/6 = 21 karakter per baris
    → Muat: 64/8  = 8 baris
    → Gunakan untuk informasi detail & kecil

  setTextSize(2):  12w × 16h piksel per karakter
    → Muat: 128/12 = 10 karakter per baris
    → Muat: 64/16  = 4 baris
    → Gunakan untuk nilai utama / angka besar

  setTextSize(3):  18w × 24h piksel per karakter
    → Muat: 128/18 = 7 karakter per baris
    → Gunakan untuk judul besar / satu nilai kritis

TEMPLATE LAYOUT POPULER:

  ┌──────────HEADER(size=1)──────────┐  Y=0
  │ JUDUL / NAMA APLIKASI            │  Y=0-8
  ├──────────────────────────────────┤  Y=10
  │  Nilai Besar           (size=2)  │  Y=12
  │  Label kecil           (size=1)  │  Y=28
  │  Nilai Kedua           (size=2)  │  Y=36
  ├──────────────────────────────────┤  Y=52
  │ Status / Uptime         (size=1) │  Y=54
  └──────────────────────────────────┘  Y=63
```

---

## 35.5 Program 1: Hello World & Teks Dasar

Program ini memperkenalkan **semua teknik menampilkan teks** yang akan digunakan di program-program selanjutnya: ukuran font berbeda, posisi presisi, mode invert, dan animasi sederhana.

```cpp
/*
 * BAB 35 - Program 1: Hello World & Teks Dasar OLED
 *
 * Tujuan:
 *   Menguasai teknik dasar menampilkan teks, angka, dan efek visual
 *   pada OLED SSD1306 128×64 yang terpasang di Bluino Kit.
 *
 * Hardware (Built-in Kit):
 *   OLED SSD1306 128×64 → SDA=IO21, SCL=IO22, Addr=0x3C
 *
 * Demonstrasi:
 *   1. Hello World dengan animasi blink
 *   2. Layout multi-baris (teks ukuran 1 & 2 bercampur)
 *   3. Menampilkan angka yang terus bertambah (counter)
 *   4. Efek invert seluruh layar (negatif)
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Konfigurasi OLED ───────────────────────────────────────────────────
#define SCREEN_W  128
#define SCREEN_H   64
#define OLED_ADDR  0x3C

Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

// ── Timing Non-Blocking ────────────────────────────────────────────────
const unsigned long INTERVAL_UPDATE_MS = 1000UL;
unsigned long tTerakhir  = 0;
unsigned long counter    = 0;
bool          invertMode = false;

// ── Tampilkan layar info startup ───────────────────────────────────────
void showSplashScreen() {
  display.clearDisplay();

  // Judul besar di tengah
  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setCursor(8, 10);
  display.print("BLUINO");
  display.setCursor(14, 30);
  display.print("IoT Kit");

  // Sub-judul kecil
  display.setTextSize(1);
  display.setCursor(20, 52);
  display.print("BAB 35 - OLED");

  // Garis dekorasi
  display.drawLine(0, 8, 127, 8, WHITE);
  display.drawLine(0, 48, 127, 48, WHITE);

  display.display();
  delay(2500); // Tampilkan splash selama 2.5 detik (hanya di startup!)
}

// ── Tampilkan counter & status ─────────────────────────────────────────
void showCounter() {
  display.clearDisplay();

  // Header
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("BAB35: Teks & Counter");
  display.drawLine(0, 10, 127, 10, WHITE);

  // Nilai counter besar — di tengah layar
  display.setTextSize(3);
  // Hitung posisi X agar counter terpusat
  int digitCount = (counter == 0) ? 1 : (int)log10(counter) + 1;
  int xCenter = (128 - digitCount * 18) / 2;
  if (xCenter < 0) xCenter = 0;
  display.setCursor(xCenter, 18);
  display.print(counter);

  // Label di bawah counter
  display.setTextSize(1);
  display.setCursor(30, 46);
  display.print("detik berjalan");

  // Status invert
  display.setCursor(0, 56);
  display.print(invertMode ? "Mode: INVERT " : "Mode: NORMAL ");

  // Kotak border dekoratif
  display.drawRoundRect(0, 14, 128, 34, 4, WHITE);

  display.display();
}

void setup() {
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    Serial.println("SSD1306 tidak ditemukan! Cek kabel I2C.");
    while (true);
  }

  showSplashScreen();
}

void loop() {
  unsigned long sekarang = millis();

  if (sekarang - tTerakhir >= INTERVAL_UPDATE_MS) {
    tTerakhir = sekarang;
    counter++;

    // Ganti mode invert setiap 5 detik
    if (counter % 5 == 0) {
      invertMode = !invertMode;
      display.invertDisplay(invertMode);
    }

    showCounter();

    Serial.printf("Counter: %lu | Invert: %s\n",
                  counter, invertMode ? "ON" : "OFF");
  }
}
```

**Contoh output OLED (setelah splash):**
```text
┌─────────────────────┐
│BAB35: Teks & Counter│
├─────────────────────┤
│      ┌───────┐      │
│      │  42   │      │ <- size=3
│      └───────┘      │
│   detik berjalan    │
│Mode: NORMAL         │
└─────────────────────┘
```

> 💡 **Mengapa `delay()` boleh di splash screen?** Dalam program yang menggunakan `delay()` pada `setup()` (bukan `loop()`), ini masih diterima karena terjadi sebelum perangkat siap beroperasi — tidak ada event yang bisa terlewat. Namun **JANGAN PERNAH** taruh `delay()` di dalam `loop()`!

---

## 35.6 Program 2: Sensor Dashboard Real-Time

Program ini adalah inti dari BAB 35 — menampilkan data tiga sensor (DHT11, BMP180, Internal) secara bersamaan di layar OLED dengan layout yang informatif dan diperbarui setiap 2 detik secara non-blocking.

```cpp
/*
 * BAB 35 - Program 2: Sensor Dashboard Real-Time di OLED
 *
 * Menampilkan data 3 sensor secara live di OLED 128×64:
 *   1. DHT11    → Suhu Lingkungan + Kelembaban (IO27)
 *   2. BMP180   → Tekanan Udara (I2C: SDA=IO21, SCL=IO22)
 *   3. Internal → Suhu Chip ESP32
 *
 * Fitur:
 *   ✅ Update OLED setiap 2 detik (non-blocking)
 *   ✅ EMA filter pada setiap sensor (dari BAB 34!)
 *   ✅ Tampilan icon status sensor (✔/✘)
 *   ✅ Uptime di footer
 *
 * Library:
 *   DHT sensor library (Adafruit)
 *   Adafruit BMP085/BMP180 (Adafruit)
 *   Adafruit SSD1306 + Adafruit GFX
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>
#include <Adafruit_BMP085.h>

// ── OLED ───────────────────────────────────────────────────────────────
#define SCREEN_W  128
#define SCREEN_H   64
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

// ── Sensor ─────────────────────────────────────────────────────────────
#define DHTPIN  27
#define DHTTYPE DHT11
DHT             dht(DHTPIN, DHTTYPE);
Adafruit_BMP085 bmp;
bool            bmpOK = false;

// ── EMA Filter (dari BAB 34) ───────────────────────────────────────────
const float EMA_A = 0.2f;

struct EMA {
  float val  = 0.0f;
  bool  init = false;
  void update(float newVal) {
    if (!init) { val = newVal; init = true; return; }
    val = EMA_A * newVal + (1.0f - EMA_A) * val;
  }
};

EMA emaSuhu;     // DHT11 suhu
EMA emaHumid;    // DHT11 kelembaban
EMA emaBMP;      // BMP180 tekanan
EMA emaInternal; // ESP32 suhu internal

// ── Timing ─────────────────────────────────────────────────────────────
const unsigned long INTERVAL_SENSOR_MS = 2000UL;
const unsigned long INTERVAL_OLED_MS   = 2000UL;
unsigned long tSensorTerakhir = 0;
unsigned long tOLEDTerakhir   = 0;

// ── Gambar satu baris sensor di OLED ──────────────────────────────────
void drawSensorRow(int y, const char* label, float val,
                   const char* satuan, bool valid) {
  display.setCursor(0, y);
  display.setTextSize(1);
  display.setTextColor(WHITE);

  // Label 3 karakter + spasi
  display.print(label);
  display.print(":");

  if (valid) {
    // Nilai sensor
    display.setCursor(30, y);
    display.print(val, 1); // 1 desimal
    display.print(" ");
    display.print(satuan);

    // Status sukses di ujung kanan
    display.setCursor(114, y);
    display.print("OK"); 
  } else {
    display.setCursor(30, y);
    display.print("-- N/A");
    display.setCursor(114, y);
    display.print("X ");
  }
}

// ── Update layar OLED ──────────────────────────────────────────────────
void updateDisplay() {
  display.clearDisplay();

  // ── Header ──────────────────────────────────────────────────────────
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(14, 0);
  display.print("SENSOR DASHBOARD");
  display.drawLine(0, 9, 127, 9, WHITE);

  // ── Baris Data Sensor ────────────────────────────────────────────────
  drawSensorRow(12, "Tmp", emaSuhu.val,     "\xF8""C",   emaSuhu.init);
  drawSensorRow(23, "Hmd", emaHumid.val,    "%",         emaHumid.init);
  drawSensorRow(34, "BMP", emaBMP.val / 100.0f, "hPa",  bmpOK && emaBMP.init);
  drawSensorRow(45, "Int", emaInternal.val, "\xF8""C",   emaInternal.init);

  // ── Garis Footer ────────────────────────────────────────────────────
  display.drawLine(0, 54, 127, 54, WHITE);

  // ── Uptime di footer ────────────────────────────────────────────────
  unsigned long detik = millis() / 1000;
  unsigned long jam   = detik / 3600;
  unsigned long menit = (detik % 3600) / 60;
  unsigned long dtk   = detik % 60;

  display.setCursor(0, 56);
  display.printf("Up: %02lu:%02lu:%02lu", jam, menit, dtk);

  // ── Status BMP di footer kanan ───────────────────────────────────────
  display.setCursor(82, 56);
  display.print(bmpOK ? " BMP:Ok" : " BMP:--");

  display.display();
}

void setup() {
  Serial.begin(115200);

  // Inisialisasi OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED gagal!");
    while (true);
  }

  // Tampilan loading sementara sensor siap
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(20, 20);
  display.print("Menginisialisasi...");
  display.setCursor(28, 35);
  display.print("Harap tunggu!");
  display.display();

  // Inisialisasi sensor
  dht.begin();
  bmpOK = bmp.begin();
  if (!bmpOK) Serial.println("BMP180 tidak ditemukan!");

  // Pre-init EMA
  float s = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(s)) emaSuhu.update(s);
  if (!isnan(h)) emaHumid.update(h);
  if (bmpOK)     emaBMP.update(bmp.readPressure());
  emaInternal.update(temperatureRead());

  delay(1500); // Delay pendek di setup saja
}

void loop() {
  unsigned long sekarang = millis();

  // ── Baca sensor setiap 2 detik ───────────────────────────────────────
  if (sekarang - tSensorTerakhir >= INTERVAL_SENSOR_MS) {
    tSensorTerakhir = sekarang;

    float suhu  = dht.readTemperature();
    float humid = dht.readHumidity();
    if (!isnan(suhu))  emaSuhu.update(suhu);
    if (!isnan(humid)) emaHumid.update(humid);

    if (bmpOK) emaBMP.update(bmp.readPressure());
    emaInternal.update(temperatureRead());
  }

  // ── Update OLED setiap 2 detik ───────────────────────────────────────
  if (sekarang - tOLEDTerakhir >= INTERVAL_OLED_MS) {
    tOLEDTerakhir = sekarang;
    updateDisplay();
  }
}
```

**Tampilan layar OLED (Realistik 21 Kolom):**
```text
┌───────────────────┐
│ SENSOR  DASHBOARD │
├───────────────────┤
│Tmp: 27.5 °C     OK│
│Hmd: 64.8 %      OK│
│BMP: 1013.2 hPa  OK│
│Int: 44.2 °C     OK│
├───────────────────┤
│Up:00:02:34  BMP:Ok│
└───────────────────┘
```

---

## 35.7 Program 3: Grafik Bar Real-Time

Program ini menampilkan nilai ADC dari **Potensiometer** sebagai grafik batang (*bar chart*) yang bergerak secara real-time. Teknik ini dapat diadaptasi untuk menampilkan riwayat suhu, kelembaban, atau sinyal sensor apapun.

```cpp
/*
 * BAB 35 - Program 3: Grafik Bar Real-Time (ADC Visualizer)
 *
 * Membaca ADC Potensiometer dan menampilkan:
 *   1. Nilai ADC raw saat ini (teks besar)
 *   2. Riwayat 32 sampel terakhir sebagai bar chart bergerak
 *   3. Garis rata-rata horisontal (SMA dari 32 sampel)
 *
 * Hardware:
 *   Potensiometer 10K → pin ADC (rekomendasi: IO34 atau IO35)
 *
 * Konsep utama:
 *   Circular buffer digunakan untuk menyimpan 32 sampel terakhir
 *   secara efisien O(1) — teknik dari BAB 34!
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── OLED ───────────────────────────────────────────────────────────────
#define SCREEN_W 128
#define SCREEN_H  64
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

// ── Hardware ───────────────────────────────────────────────────────────
const int PIN_POT = 34;

// ── Bar Chart Config ───────────────────────────────────────────────────
const int  BAR_COUNT  = 32;  // Jumlah bar (kolom) dalam chart
const int  BAR_W      = 3;   // Lebar tiap bar dalam piksel
const int  BAR_GAP    = 1;   // Jarak antar bar
const int  CHART_X    = 0;   // Posisi awal chart (X)
const int  CHART_Y    = 18;  // Posisi atas chart (Y)
const int  CHART_H    = 40;  // Tinggi area chart dalam piksel
// Total lebar = BAR_COUNT * (BAR_W + BAR_GAP) = 32 * 4 = 128 piksel ✅

// ── Circular Buffer ────────────────────────────────────────────────────
int  histBuf[BAR_COUNT];
int  histIdx   = 0;
long histSum   = 0;
bool histFull  = false;
int  histCount = 0;

void histPush(int val) {
  histSum -= histBuf[histIdx]; // Kurangi nilai lama dari sum
  histBuf[histIdx] = val;
  histSum += val;              // Tambahkan nilai baru ke sum
  histIdx = (histIdx + 1) % BAR_COUNT;
  if (!histFull) {
    histCount++;
    if (histCount >= BAR_COUNT) histFull = true;
  }
}

float histAvg() {
  int n = histFull ? BAR_COUNT : histCount;
  return (n > 0) ? (float)histSum / n : 0.0f;
}

// ── Gambar Bar Chart ───────────────────────────────────────────────────
void drawBarChart(int currentVal) {
  display.clearDisplay();

  // ── Header: nilai saat ini ─────────────────────────────────────────
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.print("ADC:");

  display.setTextSize(1);
  display.setCursor(28, 0);
  display.printf("%4d", currentVal);

  // Garis desibel (persentase)
  display.setCursor(70, 0);
  display.printf("%3d%%", currentVal * 100 / 4095);

  // Rata-rata
  display.setCursor(100, 0);
  display.printf("A:%d", (int)histAvg());

  // ── Garis pembatas ─────────────────────────────────────────────────
  display.drawLine(0, 10, 127, 10, WHITE);

  // ── Bar Chart ──────────────────────────────────────────────────────
  int n = histFull ? BAR_COUNT : histCount;
  for (int i = 0; i < n; i++) {
    // Urutan: bar paling kiri = data paling lama
    int dataIdx = (histIdx - n + i + BAR_COUNT) % BAR_COUNT;
    int rawVal  = histBuf[dataIdx];

    // Peta ADC (0-4095) ke tinggi bar (0-CHART_H piksel)
    int barH = map(rawVal, 0, 4095, 0, CHART_H);
    if (barH < 1) barH = 1; // Minimal 1 piksel

    int bx = CHART_X + i * (BAR_W + BAR_GAP);
    int by = CHART_Y + CHART_H - barH;

    display.fillRect(bx, by, BAR_W, barH, WHITE);
  }

  // ── Garis rata-rata horisontal ─────────────────────────────────────
  int avgH   = map((int)histAvg(), 0, 4095, 0, CHART_H);
  int avgY   = CHART_Y + CHART_H - avgH;
  // Gambar garis putus-putus setiap 4 piksel
  for (int x = 0; x < 128; x += 4) {
    display.drawPixel(x, avgY, WHITE);
    display.drawPixel(x + 1, avgY, WHITE);
  }

  // ── Label sumbu Y ──────────────────────────────────────────────────
  display.setTextSize(1);
  display.setCursor(0, CHART_Y);
  display.print("^");
  display.setCursor(0, CHART_Y + CHART_H - 8);
  display.print("v");

  display.display();
}

// ── Timing ─────────────────────────────────────────────────────────────
const unsigned long INTERVAL_MS = 100UL; // 10Hz sampling
unsigned long tTerakhir = 0;

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED gagal!");
    while (true);
  }

  // Pre-fill buffer dengan nilai pertama
  int firstRead = analogRead(PIN_POT);
  for (int i = 0; i < BAR_COUNT; i++) {
    histBuf[i] = firstRead;
  }
  histSum = (long)firstRead * BAR_COUNT;
  histFull = true;         // PENTING: Buffer sudah penuh!
  histCount = BAR_COUNT;   // PENTING: Cegah math error pembagi 0


  display.clearDisplay();
  display.display();
}

void loop() {
  unsigned long sekarang = millis();

  if (sekarang - tTerakhir >= INTERVAL_MS) {
    tTerakhir = sekarang;

    int adcVal = analogRead(PIN_POT);
    histPush(adcVal);
    drawBarChart(adcVal);

    Serial.printf("ADC: %d | Avg: %.0f\n", adcVal, histAvg());
  }
}
```

> 💡 **Eksperimen:** Coba putar potensiometer perlahan dari MIN ke MAX sambil mengamati bar chart bergerak dari kiri ke kanan. Lalu putar cepat dan perhatikan bentuk gelombang yang tercipta. Grafik rata-rata (garis putus-putus) juga akan bergeser dengan smooth!

---

## 35.8 Program 4: Menu Interaktif dengan Push Button

Program paling kompleks di BAB 35 ini membangun **sistem menu 3-item** yang dapat dinavigasi menggunakan dua Push Button — satu untuk scroll, satu untuk pilih. Ini adalah fondasi dari hampir semua perangkat IoT dengan UI berbasis OLED!

```cpp
/*
 * BAB 35 - Program 4: Menu Interaktif OLED + Push Button
 *
 * Fitur:
 *   ▶ Menu 3 item: [INFO SENSOR], [GRAFIK ADC], [PENGATURAN]
 *   ▶ Tombol UP/DN: Scroll cursor menu
 *   ▶ Tombol SELECT: Masuk ke sub-menu / halaman
 *   ▶ Animasi cursor ► yang bergerak mulus
 *   ▶ Anti-bounce hardware (state machine) — TANPA delay()!
 *
 * Hardware (via kabel jumper dari header Custom):
 *   BTN_SCROLL → Tombol scroll (pull-down, aktif HIGH)
 *   BTN_SELECT → Tombol pilih (pull-down, aktif HIGH)
 *
 * Catatan: Sesuaikan PIN_BTN_* dengan kabel jumper yang kamu pasang.
 *          Lihat BAB 09 (Push Button) untuk referensi konfigurasi.
 */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

// ── OLED ───────────────────────────────────────────────────────────────
#define SCREEN_W 128
#define SCREEN_H  64
Adafruit_SSD1306 display(SCREEN_W, SCREEN_H, &Wire, -1);

// ── Push Button Config ─────────────────────────────────────────────────
const int PIN_BTN_SCROLL = 32;  // Ganti sesuai jumper kamu
const int PIN_BTN_SELECT = 33;  // Ganti sesuai jumper kamu
const unsigned long DEBOUNCE_MS = 50UL;

// ── Sensor ─────────────────────────────────────────────────────────────
#define DHTPIN  27
DHT dht(DHTPIN, DHT11);

// ── State Machine Menu ─────────────────────────────────────────────────
enum AppState { STATE_MENU, STATE_INFO, STATE_GRAFIK, STATE_SETTING };
AppState currentState = STATE_MENU;

const int  MENU_ITEM_COUNT = 3;
const char* menuItems[] = {
  "INFO SENSOR",
  "GRAFIK ADC",
  "PENGATURAN"
};
int menuCursor = 0; // Item yang sedang dipilih (0..2)

// ── ADC Grafik (mini, 32 sample) ────────────────────────────────────────
const int POT_PIN   = 34;
const int MINI_BARS = 32;
int miniBuf[MINI_BARS] = {};
int miniIdx = 0;

// ── Tombol: state machine anti-bounce ─────────────────────────────────
struct Button {
  int  pin;
  bool lastReading;
  bool buttonState; // State stabil (debounced)
  unsigned long lastChange;
  bool pressed; // True selama satu frame ketika tombol baru ditekan

  void init(int p) {
    pin = p;
    lastReading = false;
    buttonState = false;
    lastChange = 0;
    pressed = false;
    pinMode(pin, INPUT);
  }

  void update(unsigned long now) {
    pressed = false;
    bool reading = (digitalRead(pin) == HIGH);
    
    // Reset timer jika ada perubahan sinyal kotor (bouncing)
    if (reading != lastReading) {
      lastChange = now;
    }
    
    // Jika sinyal stabil lebih lama dari DEBOUNCE_MS
    if ((now - lastChange) >= DEBOUNCE_MS) {
      if (reading != buttonState) {
        buttonState = reading;
        if (buttonState == true) { // Rising edge (ditekan)
          pressed = true;
        }
      }
    }
    lastReading = reading;
  }
};

Button btnScroll;
Button btnSelect;

// ── Gambar layar Menu Utama ─────────────────────────────────────────────
void drawMenu() {
  display.clearDisplay();

  // Header
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(32, 0);
  display.print("MAIN MENU");
  display.drawLine(0, 9, 127, 9, WHITE);

  // Item menu
  for (int i = 0; i < MENU_ITEM_COUNT; i++) {
    int yPos = 14 + i * 14;

    if (i == menuCursor) {
      // Item terpilih: background putih (invert)
      display.fillRect(0, yPos - 1, 128, 13, WHITE);
      display.setTextColor(BLACK);
      display.setCursor(14, yPos);
      display.print(menuItems[i]);
      // Panah kursor
      display.setCursor(3, yPos);
      display.print("\x10"); // ASCII 16 = ► simbol
      display.setTextColor(WHITE);
    } else {
      display.setCursor(14, yPos);
      display.print(menuItems[i]);
    }
  }

  // Footer hint
  display.setTextSize(1);
  display.setCursor(0, 56);
  display.print("[SCROLL]   [SELECT]");

  display.display();
}

// ── Gambar halaman Info Sensor ──────────────────────────────────────────
void drawPageInfo() {
  float suhu  = dht.readTemperature();
  float humid = dht.readHumidity();
  float chipT = temperatureRead();

  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(16, 0);
  display.print("INFO SENSOR");
  display.drawLine(0, 9, 127, 9, WHITE);

  display.setCursor(0, 12);
  if (!isnan(suhu))  display.printf("Suhu   : %.1f C\n", suhu);
  else               display.println("Suhu   : -- (err)");

  display.setCursor(0, 22);
  if (!isnan(humid)) display.printf("Humid  : %.1f %%\n", humid);
  else               display.println("Humid  : -- (err)");

  display.setCursor(0, 32);
  display.printf("Chip T : %.1f C", chipT);

  display.setCursor(0, 44);
  display.printf("Uptime : %lu s", millis() / 1000);

  display.drawLine(0, 54, 127, 54, WHITE);
  display.setCursor(0, 56);
  display.print("[SELECT] = Kembali");

  display.display();
}

// ── Gambar halaman Grafik ADC ─────────────────────────────────────────
void drawPageGrafik() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(14, 0);
  display.print("LIVE ADC CHART");
  display.drawLine(0, 9, 127, 9, WHITE);

  int adcNow = analogRead(POT_PIN);
  miniBuf[miniIdx] = adcNow;
  miniIdx = (miniIdx + 1) % MINI_BARS;

  // Gambar bar chart mini
  for (int i = 0; i < MINI_BARS; i++) {
    int dataIdx = (miniIdx + i) % MINI_BARS;
    int barH = map(miniBuf[dataIdx], 0, 4095, 0, 44);
    if (barH < 1) barH = 1;
    int bx = i * 4;
    int by = 53 - barH;
    display.fillRect(bx, by, 3, barH, WHITE);
  }

  // Nilai sekarang
  display.setTextSize(1);
  display.setCursor(0, 56);
  display.printf("ADC: %4d  %3d%%", adcNow, adcNow * 100 / 4095);

  display.display();
}

// ── Gambar halaman Pengaturan ─────────────────────────────────────────
void drawPageSetting() {
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(16, 0);
  display.print("PENGATURAN");
  display.drawLine(0, 9, 127, 9, WHITE);

  display.setCursor(0, 14);
  display.println("Versi    : BAB35 v1.0");
  display.setCursor(0, 24);
  display.println("I2C Addr : 0x3C");
  display.setCursor(0, 34);
  display.println("DHT Pin  : IO27");
  display.setCursor(0, 44);
  display.println("Resolusi : 128x64");

  display.drawLine(0, 54, 127, 54, WHITE);
  display.setCursor(0, 56);
  display.print("[SELECT] = Kembali");

  display.display();
}

// ── Timing ─────────────────────────────────────────────────────────────
const unsigned long INTERVAL_GRAFIK_MS = 100UL;
unsigned long tGrafikTerakhir = 0;
const unsigned long INTERVAL_INFO_MS = 2000UL;
unsigned long tInfoTerakhir = 0;

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  btnScroll.init(PIN_BTN_SCROLL);
  btnSelect.init(PIN_BTN_SELECT);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED gagal!");
    while (true);
  }

  dht.begin();
  drawMenu();
}

void loop() {
  unsigned long now = millis();

  // Update state tombol
  btnScroll.update(now);
  btnSelect.update(now);

  switch (currentState) {

    case STATE_MENU:
      // Scroll: geser cursor ke bawah (wrap-around otomatis)
      if (btnScroll.pressed) {
        menuCursor = (menuCursor + 1) % MENU_ITEM_COUNT;
        drawMenu();
        Serial.printf("Menu cursor: %d\n", menuCursor);
      }
      // Select: masuk ke halaman yang dipilih
      if (btnSelect.pressed) {
        if (menuCursor == 0) {
          currentState = STATE_INFO;
          tInfoTerakhir = now; // Reset timer auto-update
          drawPageInfo();      // Paksa gambar frame pertama (Anti-Lag UX)
        } else if (menuCursor == 1) {
          currentState = STATE_GRAFIK;
          tGrafikTerakhir = now; // Reset timer animasi
          drawPageGrafik();      // Paksa gambar frame pertama
        } else if (menuCursor == 2) {
          currentState = STATE_SETTING;
          drawPageSetting();     // HANYA dipanggil 1x saat transisi!
        }
        Serial.printf("Masuk state: %d\n", currentState);
      }
      break;

    case STATE_INFO:
      // Auto-update info sensor secara berkala
      if (now - tInfoTerakhir >= INTERVAL_INFO_MS) {
        tInfoTerakhir = now;
        drawPageInfo();
      }
      if (btnSelect.pressed) {
        currentState = STATE_MENU;
        drawMenu(); // Kembali ke menu
      }
      break;

    case STATE_GRAFIK:
      // Auto-update animasi grafik ADC
      if (now - tGrafikTerakhir >= INTERVAL_GRAFIK_MS) {
        tGrafikTerakhir = now;
        drawPageGrafik();
      }
      if (btnSelect.pressed) {
        currentState = STATE_MENU;
        drawMenu(); // Kembali ke menu
      }
      break;

    case STATE_SETTING:
      // HALAMAN STATIS: Tidak butuh fungsi redraw konstan di dalam loop()!
      // (Mencegah layar OLED berkedip parah & traffic packet flood I2C)
      if (btnSelect.pressed) {
        currentState = STATE_MENU;
        drawMenu(); // Kembali ke menu
      }
      break;
  }
}
```

> 💡 **State Machine adalah kuncinya!** Perhatikan bagaimana program ini menggunakan `enum AppState` dan `switch(currentState)` — ini adalah pola *Finite State Machine (FSM)* dari BAB 17 yang diterapkan pada UI. Setiap "halaman" adalah sebuah *state*, transisi antar halaman terjadi ketika tombol ditekan.

---

## 35.9 Tips & Panduan Troubleshooting

### 1. OLED tidak muncul / layar putih semua?
```text
Penyebab paling umum:

A. Alamat I2C salah (0x3C vs 0x3D)
   Solusi: Jalankan I2C Scanner untuk mendeteksi alamat:

#include <Wire.h>
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22); // SDA=IO21, SCL=IO22
  for (byte addr = 1; addr < 127; addr++) {
    Wire.beginTransmission(addr);
    if (Wire.endTransmission() == 0) {
      Serial.printf("I2C Device found: 0x%02X\n", addr);
    }
  }
}
void loop() {}

B. Kabel SDA/SCL terbalik
   → Periksa: IO21 → SDA (kuning), IO22 → SCL (biru) di kit

C. Lupa memanggil display.display() setelah clearDisplay()
   → Selalu pastikan urutan: clearDisplay() → gambar → display()
```

### 2. Teks terpotong atau melebihi batas layar?
```text
Penyebab: Posisi cursor (x,y) terlalu dekat dengan tepi kanan/bawah.

Panduan batas aman per ukuran font:
  setTextSize(1): x maks = 122 (128 - 6 char width)
                  y maks = 56  (64  - 8 char height)
  setTextSize(2): x maks = 116 (128 - 12)
                  y maks = 48  (64  - 16)
  setTextSize(3): x maks = 110 (128 - 18)
                  y maks = 40  (64  - 24)

Tips: Selalu hitung x = (128 - (panjang_teks * lebar_font)) / 2
      untuk penempatan teks yang benar-benar terpusat.
```

### 3. Layar OLED berkedip / flicker?
```text
Penyebab: display.clearDisplay() + display.display() dipanggil
          TERLALU sering (setiap loop), menyebabkan "flash hitam"
          sebelum konten baru muncul.

Solusi non-blocking yang benar:
  if (sekarang - tUpdate >= INTERVAL_OLED_MS) {
    tUpdate = sekarang;
    display.clearDisplay();
    // ... gambar semua elemen ...
    display.display(); // Hanya panggil satu kali, di sini!
  }

Gunakan interval update OLED yang lebih lambat dari sensor
(misalnya update sensor 500ms, update OLED 1000ms).
```

### 4. RAM ESP32 hampir penuh setelah include Adafruit SSD1306?
```text
Library Adafruit SSD1306 mengalokasikan buffer di heap:
  128 × 64 / 8 = 1024 bytes (1KB) → Ini PLUS overhead library

Tips penghematan RAM:
  A. Hindari String objek — gunakan char[] dan snprintf()
  B. Hindari float yang tidak perlu — kalikan ×10 simpan sebagai int
  C. Gunakan PROGMEM untuk data konstan (icon bitmap, dll)
  D. Monitor penggunaan RAM: Serial.printf("Free heap: %d\n",
     ESP.getFreeHeap());
```

### 5. Karakter khusus (°, %, ✓) tidak tampil dengan benar?
```text
Font bawaan Adafruit GFX menggunakan ASCII 0-127 + subset Latin-1.

Untuk simbol derajat (°): gunakan '\xF8' atau print("\xB0")
Untuk simbol centang:     Gunakan teks standar "OK" (CP437 tak memiliki ✓)
Untuk simbol panah:       gunakan '\x10' (▶, ASCII 16)

Jika membutuhkan lebih banyak simbol atau font custom,
gunakan library "GFXFonts" yang tersedia di GitHub Adafruit.
Kamu bisa mengimpor font.h tambahan ke project kamu!
```

---

## 35.10 Ringkasan

```
┌──────────────────────────────────────────────────────────────────────┐
│              RINGKASAN BAB 35 — OLED DISPLAY 128×64                  │
├──────────────────────────────────────────────────────────────────────┤
│ HARDWARE & PROTOKOL:                                                  │
│   SSD1306 Controller → Dikendalikan via I2C (SDA=IO21, SCL=IO22)    │
│   GDDRAM Buffer      → 1024 bytes di dalam chip untuk menyimpan      │
│                         seluruh 128×64 = 8192 piksel                 │
│   Alamat I2C Default → 0x3C (kit Bluino)                             │
│                                                                       │
│ SISTEM KOORDINAT:                                                     │
│   Titik (0,0) = pojok kiri atas                                       │
│   X → kanan (0 hingga 127)                                            │
│   Y → bawah (0 hingga 63)                                             │
│                                                                       │
│ ALUR RENDERING (WAJIB diikuti):                                       │
│   1. display.clearDisplay()   → Bersihkan buffer di RAM ESP32         │
│   2. Semua fungsi draw*()     → Tulis ke buffer (belum ke layar!)     │
│   3. display.display()        → Kirim seluruh buffer ke SSD1306       │
│                                 (inilah yang membuat layar berubah)   │
│                                                                       │
│ UKURAN FONT (Built-in Adafruit GFX):                                  │
│   Size 1 → 6×8px,  muat 21 karakter/baris × 8 baris                 │
│   Size 2 → 12×16px, muat 10 karakter/baris × 4 baris                │
│   Size 3 → 18×24px, muat 7 karakter/baris × 2 baris                 │
│                                                                       │
│ POLA DESAIN UI YANG DIREKOMENDASIKAN:                                 │
│   ✅ Update OLED via millis() timer — JANGAN dalam setiap loop()     │
│   ✅ Satu fungsi draw*() per halaman — mudah di-maintain             │
│   ✅ State Machine (FSM) untuk navigasi antar halaman                │
│   ✅ Pisahkan logika sensor dan logika tampilan                      │
│   ⚠️ JANGAN panggil display.display() lebih dari perlu —            │
│      setiap call mengirim 1KB data via I2C (butuh ~5ms!)             │
│                                                                       │
│ FRAMEWORK DARI PROGRAM 4 (GUNAKAN ULANG!):                           │
│   enum AppState + switch(currentState) + Button struct              │
│   = Pola universal yang berlaku untuk menu dengan N item             │
└──────────────────────────────────────────────────────────────────────┘
```

---

## 35.11 Latihan

1. **Splash Screen Animasi:** Modifikasi Program 1 agar saat startup muncul animasi: teks "BLUINO" muncul dari kiri perlahan (geser 2 piksel per frame, 30ms per frame, menggunakan `millis()` tanpa `delay()` di `loop()`). Setelah teks sampai di tengah, tampilkan subtitle "IoT Kit v3.2" dari atas ke bawah. Ini berlatih teknik animasi pixel-level!

2. **Dashboard 2 Halaman Otomatis:** Kembangkan Program 2 agar secara otomatis bergantian menampilkan dua halaman setiap 5 detik: Halaman 1 = Suhu & Kelembaban (font besar, size=2), Halaman 2 = Tekanan Udara & Suhu Chip. Tambahkan indikator halaman di pojok kanan atas (`1/2` atau `2/2`).

3. **Grafik Suhu Historis:** Modifikasi Program 3 agar yang ditampilkan bukan ADC, melainkan riwayat **suhu DHT11** dalam 32 sampel terakhir (sampling setiap 5 detik). Skala Y harus otomatis menyesuaikan antara nilai min dan max yang ada dalam buffer grafik. Tantangan: Bagaimana cara menampilkan label nilai min/max di sumbu Y?

4. **Menu 5 Item dengan Scroll Terpaginate:** Kembangkan Program 4 agar menu memiliki 5 item, namun layar hanya menampilkan 3 item sekaligus (halaman). Ketika cursor mencapai item ke-3 yang terlihat, tampilan harus "scroll" ke bawah untuk menampilkan item berikutnya. Ini mensimulasikan *paginated menu navigation* yang lazim di perangkat embedded.

5. **Alarm Visual:** Tambahkan fitur ke Program 2: jika suhu DHT11 melebihi 35°C, lakukan `display.invertDisplay(true)` selama 0.5 detik kemudian `invertDisplay(false)` — menciptakan efek *flash warning*. Lakukan ini menggunakan `millis()` sebagai timer tanpa `delay()`. Pastikan alarm tidak mengganggu siklus update sensor.

---

## ☕ Dukung Proyek Ini

Bab ini disusun dengan dedikasi waktu dan keahlian yang tidak sedikit. Jika materi *engineering-grade* ini bermanfaat bagi riset atau proses belajarmu, bentuk apresiasi terbaik yang bisa kamu berikan adalah secangkir kopi untuk penulis:

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
Punya kritik, saran, atau butuh mentoring khusus ESP32 & IoT industri? Silakan sapa saya di:
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

---

*Sebelumnya → [BAB 34: Pemrosesan Data Sensor](../BAB-34/README.md)*

*Selanjutnya → [BAB 36: RGB LED WS2812 (NeoPixel)](../BAB-36/README.md)*
