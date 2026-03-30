# BAB 33: Sensor Internal ESP32 — Temperature & Hall Effect

> ✅ **Prasyarat:** Selesaikan **BAB 16 (Non-Blocking Programming)** dan **BAB 11 (Analog Input & ADC)**. Bab ini tidak memerlukan komponen hardware tambahan apapun — sensor sudah tertanam di dalam chip ESP32!

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami bahwa ESP32 memiliki **sensor suhu dan sensor medan magnet** yang sudah terintegrasi di dalam chip (*on-chip sensors*).
- Membaca **suhu internal die (chip) ESP32** menggunakan API bawaan.
- Memahami **keterbatasan dan konteks penggunaan** sensor internal ini secara jujur dan kritis.
- Membangun sistem **monitoring kesehatan ESP32** (thermal health monitor) yang berjalan secara non-blocking.
- Memahami status **deprecated** Hall Effect Sensor dan implikasinya terhadap portabilitas kode.

---

## 33.1 Sensor Internal ESP32 — Apa dan Mengapa?

### Konsep: Sensor yang Bersembunyi di Dalam Chip

Kebanyakan tutorial ESP32 hanya mengajarkan cara membaca sensor *eksternal* (DHT11, DS18B20, dll). Padahal, chip ESP32 itu sendiri sudah membawa beberapa sensor mikro yang tertanam di dalam silicon-nya:

```
KOMPONEN INTERNAL ESP32 (Yang Jarang Dibahas):

┌─────────────────────────────────────────────────────────┐
│                  CHIP ESP32 (Xtensa LX6)                 │
│                                                          │
│  ┌──────────────┐   ┌──────────────┐   ┌─────────────┐  │
│  │   CPU Core 0  │   │  Temperature  │   │  Hall Effect │  │
│  │   CPU Core 1  │   │   Sensor     │   │   Sensor    │  │
│  │               │   │  (on-die)    │   │  (DEPRECATED)│  │
│  │  Running your │   │              │   │             │  │
│  │     code!     │   │ Baca suhu    │   │ Baca medan  │  │
│  │               │   │ chip itu     │   │ magnet di   │  │
│  │               │   │ sendiri      │   │ sekitar chip│  │
│  └──────────────┘   └──────────────┘   └─────────────┘  │
│                                                          │
│  ADC12 | DAC | RTC | WiFi/BT Radio | Hall | Temp | ...  │
└─────────────────────────────────────────────────────────┘
```

### Kenapa Ini Penting Dipelajari?

```
KASUS PENGGUNAAN NYATA SENSOR INTERNAL:

1. THERMAL PROTECTION SYSTEM
   → Monitor suhu chip → alarm/throttle jika terlalu panas
   → Produk IoT yang dipasang dalam enclosure tertutup
     rentan overheating! Ini menyelamatkan hardware!

2. SELF-DIAGNOSTICS
   → Sistem bisa melaporkan "kesehatan dirinya sendiri"
     ke dashboard monitoring cloud
   → Digunakan di sistem produksi untuk preventive maintenance

3. RISET & BENCHMARKING
   → Mengukur dampak thermal dari algoritma yang berat
     (enkripsi, FFT, dll) terhadap chip

4. ZERO HARDWARE OVERHEAD
   → Tidak butuh sensor + kabel + soldering
   → Sangat berguna untuk quick prototype dan demo
```

> 💡 **Perspektif Industri:** Pada produk IoT profesional, monitoring suhu chip adalah **syarat wajib** dalam *quality assurance*. Sistem yang berjalan melebihi 85°C (Tj max ESP32) berisiko mengalami *thermal shutdown* atau degradasi performa permanen.

---

## 33.2 Internal Temperature Sensor — Teori & Keterbatasan

### Cara Kerja

Sensor suhu internal ESP32 adalah sebuah **rangkaian bandgap reference** yang menghasilkan tegangan proporsional terhadap suhu **die (inti) chip** itu sendiri. Tegangan ini kemudian dibaca oleh ADC internal dan dikonversi ke nilai suhu dalam format Celsius.

```
ARSITEKTUR SENSOR SUHU INTERNAL:

  Suhu Die (°C)
       │
       ▼
  Bandgap Circuit ──→ Tegangan referensi (sangat sensitif terhadap suhu)
       │
       ▼
  ADC Internal ──→ Nilai digital
       │
       ▼
  temperatureRead() ──→ float (°C)
```

### ⚠️ Keterbatasan WAJIB Dipahami

```
KETERBATASAN KRITIS SENSOR SUHU INTERNAL ESP32:

1. BUKAN SUHU LINGKUNGAN!
   → Ini adalah suhu DIE CHIP, bukan suhu udara di sekitarnya.
   → Saat CPU idle: suhu chip ≈ suhu ruangan + 5-15°C
   → Saat CPU 100%: suhu chip bisa 50-70°C meski ruangan 27°C!

2. AKURASI RENDAH: ±5°C
   → Tidak bisa digunakan untuk pengukuran presisi lingkungan.
   → Gunakan DHT11/DS18B20 untuk suhu ruangan!

3. DIPENGARUHI BEBAN CPU DAN CLOCK
   Contoh nyata:
     CPU idle @ 80MHz  → chip ~ 36°C
     CPU busy @ 240MHz → chip ~ 52°C  (kondisi ruangan sama!)
   → Nilai suhu AKAN NAIK saat menjalankan WiFi, enkripsi, dll.

4. SATURASI DI SUHU TINGGI
   → Sensor mulai tidak akurat di atas ~70-80°C.

KESIMPULAN: Gunakan untuk TREND dan THRESHOLD (panas/tidak panas),
            BUKAN untuk pengukuran absolut yang presisi!
```

### API yang Digunakan

```cpp
// Fungsi utama — sudah tersedia built-in, tidak perlu library!
float suhu = temperatureRead(); // Mengembalikan suhu dalam °C

// Catatan versi:
//   Arduino ESP32 Core v1.x - v2.x : temperatureRead() ✅ Tersedia
//   Arduino ESP32 Core v3.x        : API BERUBAH (lihat catatan di bawah)
```

> ⚠️ **Catatan Kompatibilitas ESP32 Core v3.x:** Di Arduino ESP32 Core v3.0 ke atas, fungsi `temperatureRead()` masih tersedia namun perilakunya mungkin sedikit berbeda tergantung board variant. Jika terjadi error kompilasi, cek versi Board Manager di Arduino IDE: `Tools → Board → Boards Manager → esp32 by Espressif Systems`.

---

## 33.3 Program 1: Baca Temperatur Internal — Dasar

Program pertama yang sangat sederhana ini membuktikan bahwa sensor benar-benar bekerja dan membantu kamu memahami hubungan antara beban CPU dan suhu chip.

```cpp
/*
 * BAB 33 - Program 1: ESP32 Internal Temperature Sensor
 *
 * Tujuan: Membaca dan menampilkan suhu internal chip ESP32.
 *
 * Hardware: Tidak ada! Murni menggunakan sensor on-chip.
 *
 * Eksperimen yang disarankan:
 *   1. Upload dan amati suhu saat CPU idle
 *   2. Tambahkan busy loop → amati suhu naik
 *   3. Letakkan jari di atas chip ESP32 → amati apakah berpengaruh
 *      (Jawaban: TIDAK. Ini bukan sensor suhu udara/sentuhan!)
 */

void setup() {
  Serial.begin(115200);
  Serial.println("═══════════════════════════════════════════════");
  Serial.println("  BAB 33: ESP32 Internal Temperature Sensor");
  Serial.println("═══════════════════════════════════════════════");
  Serial.println("  Waktu (ms)  │  Suhu (°C)  │  Suhu (°F)  │ Status");
  Serial.println("──────────────┼─────────────┼─────────────┼───────");
}

void loop() {
  float suhuC = temperatureRead();         // Baca suhu in °C
  float suhuF = (suhuC * 9.0f / 5.0f) + 32.0f; // Konversi ke °F

  // Klasifikasi status thermal
  const char* status;
  if      (suhuC < 40.0f) status = "DINGIN ";
  else if (suhuC < 55.0f) status = "NORMAL ";
  else if (suhuC < 70.0f) status = "HANGAT ⚠️";
  else                     status = "PANAS! 🔥";

  Serial.printf("  [%8lu ms] │  %6.1f °C  │  %6.1f °F  │ %s\n",
                millis(), suhuC, suhuF, status);

  delay(1000); // Program 1 boleh pakai delay — ini hanya alat observasi
}
```

**Contoh output yang diharapkan:**
```text
═══════════════════════════════════════════════
  BAB 33: ESP32 Internal Temperature Sensor
═══════════════════════════════════════════════
  Waktu (ms)  │  Suhu (°C)  │  Suhu (°F)  │ Status
──────────────┼─────────────┼─────────────┼───────
  [    1001 ms] │    42.8 °C  │   109.0 °F  │ NORMAL
  [    2001 ms] │    43.1 °C  │   109.6 °F  │ NORMAL
  [    3001 ms] │    42.8 °C  │   109.0 °F  │ NORMAL

  ← (WiFi diaktifkan di kode lain yang berjalan paralel)

  [    4001 ms] │    51.3 °C  │   124.3 °F  │ NORMAL
  [    5001 ms] │    56.7 °C  │   134.1 °F  │ HANGAT ⚠️
```

> 💡 **Pengamatan Penting:** Catat suhu saat idle, lalu tambahkan `for(int i=0; i<1000000; i++);` sebelum `delay()` untuk membuat CPU sibuk. Lihat bagaimana suhu naik! Ini adalah **demonstrasi langsung pengaruh beban komputasi terhadap thermal chip**.

---

## 33.4 Program 2: Monitoring Thermal CPU — Non-Blocking

Program ini adalah versi *production-ready*: non-blocking, dengan alarm suhu dan logging periodik yang terstruktur.

```cpp
/*
 * BAB 33 - Program 2: ESP32 Thermal Monitor — Non-Blocking
 *
 * Fitur:
 *   ✅ Baca suhu setiap 1 detik (non-blocking)
 *   ✅ Moving Average filter 8 sampel untuk stabilitas
 *   ✅ Sistem alarm suhu (PIN_BUZZER berbunyi jika terlalu panas)
 *   ✅ Log periodik ke Serial Monitor
 *   ✅ Tidak ada delay() apapun!
 *
 * Pin:
 *   PIN_BUZZER → Custom (hubungkan buzzer via jumper, atau set -1 jika tidak pakai)
 */

// ── Konfigurasi ──────────────────────────────────────────────────────
const int   PIN_BUZZER       = -1;   // Set ke pin buzzer, atau -1 jika tidak pakai
const float SUHU_WARNING     = 60.0f; // °C — mulai beri peringatan
const float SUHU_ALARM       = 75.0f; // °C — alarm darurat!

// ── Moving Average ───────────────────────────────────────────────────
const int   N_SAMPLES        = 8;
float       bufferSuhu[N_SAMPLES];
int         idxBuf           = 0;
float       sumBuf           = 0.0f;
float       suhuSmooth        = 0.0f;

void tambahSampelSuhu(float s) {
  sumBuf      -= bufferSuhu[idxBuf];
  bufferSuhu[idxBuf] = s;
  sumBuf      += s;
  idxBuf       = (idxBuf + 1) % N_SAMPLES;
  suhuSmooth   = sumBuf / N_SAMPLES;
}

// ── Timing ───────────────────────────────────────────────────────────
const unsigned long INTERVAL_BACA_MS = 1000UL;
const unsigned long INTERVAL_LOG_MS  = 5000UL;
unsigned long tBacaTerakhir = 0;
unsigned long tLogTerakhir  = 0;

// ── State ─────────────────────────────────────────────────────────────
enum StatusThermal { DINGIN, NORMAL, WARNING, ALARM };
StatusThermal statusSekarang = DINGIN;
StatusThermal statusLama     = DINGIN;
float suhuMax = -999.0f;

StatusThermal tentukanStatus(float suhu) {
  if      (suhu >= SUHU_ALARM)   return ALARM;
  else if (suhu >= SUHU_WARNING) return WARNING;
  else if (suhu >= 40.0f)        return NORMAL;
  else                            return DINGIN;
}

const char* namaStatus[] = { "DINGIN", "NORMAL", "WARNING ⚠️", "ALARM 🔥" };

void setup() {
  Serial.begin(115200);

  if (PIN_BUZZER >= 0) {
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);
  }

  // Pre-fill buffer dengan pembacaan pertama
  float suhuAwal = temperatureRead();
  for (int i = 0; i < N_SAMPLES; i++) bufferSuhu[i] = suhuAwal;
  sumBuf     = suhuAwal * N_SAMPLES;
  suhuSmooth = suhuAwal;
  suhuMax    = suhuAwal;

  Serial.println("══════════════════════════════════════════════════════");
  Serial.println("  BAB 33: ESP32 Thermal Health Monitor");
  Serial.println("══════════════════════════════════════════════════════");
  Serial.printf ("  Batas Warning : %.1f °C\n", SUHU_WARNING);
  Serial.printf ("  Batas Alarm   : %.1f °C\n", SUHU_ALARM);
  Serial.printf ("  Suhu Awal     : %.1f °C\n", suhuAwal);
  Serial.println("──────────────────────────────────────────────────────");
}

void loop() {
  unsigned long sekarang = millis();

  // ── Baca suhu setiap 1 detik ─────────────────────────────────────
  if (sekarang - tBacaTerakhir >= INTERVAL_BACA_MS) {
    tBacaTerakhir = sekarang;
    float suhuBaru = temperatureRead();
    tambahSampelSuhu(suhuBaru);

    // Update suhu tertinggi sepanjang session
    if (suhuSmooth > suhuMax) suhuMax = suhuSmooth;

    // Deteksi perubahan status
    statusSekarang = tentukanStatus(suhuSmooth);
    if (statusSekarang != statusLama) {
      Serial.printf("[%8lu ms] ⚡ STATUS: %s → %s │ Suhu=%.1f°C\n",
                    sekarang, namaStatus[statusLama],
                    namaStatus[statusSekarang], suhuSmooth);
      statusLama = statusSekarang;

      // Aktifkan buzzer jika alarm
      if (PIN_BUZZER >= 0) {
        digitalWrite(PIN_BUZZER, statusSekarang == ALARM ? HIGH : LOW);
      }
    }
  }

  // ── Log periodik setiap 5 detik ──────────────────────────────────
  if (sekarang - tLogTerakhir >= INTERVAL_LOG_MS) {
    tLogTerakhir = sekarang;
    unsigned long uptime = sekarang / 1000;
    Serial.printf("  [%5lu s] │ Suhu=%.1f°C │ Max=%.1f°C │ Status:%s\n",
                  uptime, suhuSmooth, suhuMax, namaStatus[statusSekarang]);
  }
}
```

**Contoh output:**
```text
══════════════════════════════════════════════════════
  BAB 33: ESP32 Thermal Health Monitor
══════════════════════════════════════════════════════
  Batas Warning : 60.0 °C
  Batas Alarm   : 75.0 °C
  Suhu Awal     : 43.2 °C
──────────────────────────────────────────────────────
  [    5 s] │ Suhu=43.6°C │ Max=43.8°C │ Status:NORMAL
  [   10 s] │ Suhu=44.1°C │ Max=44.1°C │ Status:NORMAL

  ← (WiFi scan diaktifkan oleh bagian kode lain)

[   12450 ms] ⚡ STATUS: NORMAL → WARNING ⚠️ │ Suhu=61.4°C
  [   15 s] │ Suhu=63.2°C │ Max=63.2°C │ Status:WARNING ⚠️
```

---

## 33.5 Hall Effect Sensor — Sejarah & Status Deprecation

### Apa itu Hall Effect?

```
PRINSIP HALL EFFECT:

  Magnet (kutub utara)         Sensor Hall Effect
       │                             │
       ▼                             ▼
  Medan magnet tegak lurus ──→ Arus listrik berbelok ──→ Tegangan terukur!

  TANPA MEDAN: Tidak ada magnet → Tegangan Vhall ≈ 0
  ADA MEDAN:   Ada magnet dekat → Tegangan Vhall ≠ 0 (proporsional kekuatan magnet)

Satuan: Gauss (G) atau Tesla (T)
1 Tesla = 10,000 Gauss
Medan magnet bumi: ~0.25 – 0.65 Gauss (sangat lemah!)
Magnet kulkas biasa: ~50 – 200 Gauss → Mudah dideteksi!
```

### ⚠️ Status Deprecated — Transparent & Jujur

```
TIMELINE DUKUNGAN HALL SENSOR ESP32 DI ARDUINO:

  Arduino ESP32 Core v1.x  →  hallRead() ✅ BERFUNGSI
  Arduino ESP32 Core v2.x  →  hallRead() ✅ BERFUNGSI (dengan warning)
  Arduino ESP32 Core v3.0+ →  hallRead() ❌ DIHAPUS! Tidak tersedia lagi!

ALASAN PENGHAPUSAN:
  Espressif menghapus Hall Sensor dari ESP-IDF v5.x dengan alasan:
  1. Akurasi sangat rendah — mudah terganggu interferensi internal chip
  2. Mengorbankan ADC1 channel 0 dan 3 (IO36 dan IO39) secara eksklusif
  3. Sensor eksternal seperti AH3503 / SS49E jauh lebih akurat dan murah

CEK VERSI ESP32 CORE KAMU:
  Arduino IDE → Tools → Board → Boards Manager
  Cari: "esp32 by Espressif Systems"
  Versi 2.x → hallRead() masih ada
  Versi 3.x → hallRead() sudah tidak ada
```

> ⚠️ **Panduan Instruktur:** Bagian ini tetap diajarkan karena nilai edukasi historis dan konseptualnya sangat tinggi. Siswa yang memahami *mengapa* suatu API didepresiasi akan menjadi engineer yang lebih baik — mereka belajar menimbang *trade-off* hardware, bukan hanya mengkopi kode!

---

## 33.6 Program 3: Demo Hall Effect Sensor (ESP32 Core v2.x)

> ⚠️ **Prasyarat Program Ini:** Pastikan kamu menggunakan **Arduino ESP32 Core versi 2.x** (bukan 3.x). Cek di: `Tools → Board → Boards Manager → esp32 by Espressif`.

```cpp
/*
 * BAB 33 - Program 3: ESP32 Hall Effect Sensor Demo
 *
 * PERINGATAN KOMPATIBILITAS:
 *   Hanya berfungsi di Arduino ESP32 Core v1.x dan v2.x!
 *   Di Core v3.x, fungsi hallRead() sudah DIHAPUS.
 *
 * Hardware yang dibutuhkan:
 *   - Magnet permanen (magnet kulkas, magnet neodymium, dll)
 *   - Tidak ada komponen lain yang dibutuhkan!
 *
 * Cara eksperimen:
 *   1. Upload program → amati nilai baseline (tanpa magnet)
 *   2. Dekatkan magnet ke bagian BAWAH chip ESP32 → nilai akan berubah!
 *   3. Balik kutub magnet → nilai berubah berlawanan arah!
 *   4. Jauhkan magnet → nilai kembali ke baseline!
 *
 * Mengapa di bagian BAWAH chip?
 *   Sensor Hall tertanam di die silicon, paling sensitif dari arah
 *   yang tegak lurus terhadap permukaan chip. Posisi terbaik:
 *   magnet di bawah PCB, tepat di bawah lokasi chip ESP32.
 */

// ── Kalibrasi Baseline ────────────────────────────────────────────
// Nilai Hall Effect bervariasi antar chip karena offset internal!
// Kita perlu kalibrasi zero-point di setiap unit.
int   baselineHall = 0;
const int THRESHOLD_DETEKSI = 20; // Perubahan minimal untuk dianggap "ada magnet"

// ── Moving Average ─────────────────────────────────────────────────
const int N_SAMPLES = 16;
int   bufferHall[N_SAMPLES];
int   idxBuf     = 0;
long  sumBuf     = 0;
int   hallSmooth = 0;
int   hallRaw    = 0; // Simpan nilai raw terakhir agar tidak double-call hallRead()

void tambahSampelHall(int s) {
  sumBuf       -= bufferHall[idxBuf];
  bufferHall[idxBuf] = s;
  sumBuf       += s;
  idxBuf        = (idxBuf + 1) % N_SAMPLES;
  hallSmooth    = (int)(sumBuf / N_SAMPLES);
}

// ── Timing ─────────────────────────────────────────────────────────
const unsigned long INTERVAL_BACA_MS  = 50UL;   // 20Hz sampling
const unsigned long INTERVAL_PRINT_MS = 500UL;
unsigned long tBacaTerakhir  = 0;
unsigned long tPrintTerakhir = 0;

void setup() {
  Serial.begin(115200);

  // Kalibrasi: ambil rata-rata 32 sampel sebagai baseline
  Serial.println("Kalibrasi baseline... (jangan dekatkan magnet!)");
  long totalKalibrasi = 0;
  for (int i = 0; i < 32; i++) {
    totalKalibrasi += hallRead();
    delay(10);
  }
  baselineHall = (int)(totalKalibrasi / 32);

  // Pre-fill buffer
  for (int i = 0; i < N_SAMPLES; i++) bufferHall[i] = baselineHall;
  sumBuf      = (long)baselineHall * N_SAMPLES;
  hallSmooth  = baselineHall;
  hallRaw     = baselineHall;

  Serial.println("═══════════════════════════════════════════════════════");
  Serial.println("  BAB 33: Hall Effect Sensor Demo (Core v2.x only!)");
  Serial.println("═══════════════════════════════════════════════════════");
  Serial.printf ("  Baseline (zero-point): %d\n", baselineHall);
  Serial.printf ("  Threshold deteksi    : ±%d\n", THRESHOLD_DETEKSI);
  Serial.println("  Dekatkan magnet ke bagian bawah chip ESP32!");
  Serial.println("───────────────────────────────────────────────────────");
}

void loop() {
  unsigned long sekarang = millis();

  // ── Sampling Hall setiap 50ms ─────────────────────────────────
  if (sekarang - tBacaTerakhir >= INTERVAL_BACA_MS) {
    tBacaTerakhir = sekarang;
    hallRaw = hallRead();         // Simpan satu kali, pakai dua tempat
    tambahSampelHall(hallRaw);
  }

  // ── Print setiap 500ms ────────────────────────────────────────
  if (sekarang - tPrintTerakhir >= INTERVAL_PRINT_MS) {
    tPrintTerakhir = sekarang;

    int delta = hallSmooth - baselineHall;
    const char* deteksi;
    if      (delta >  THRESHOLD_DETEKSI) deteksi = "🔵 KUTUB UTARA";
    else if (delta < -THRESHOLD_DETEKSI) deteksi = "🔴 KUTUB SELATAN";
    else                                  deteksi = "⬜ Tidak ada magnet";

    Serial.printf("  [%8lu ms] Raw=%4d │ Smooth=%4d │ Delta=%+4d │ %s\n",
                  sekarang, hallRaw, hallSmooth, delta, deteksi);
  }
}
```

**Contoh output:**
```text
Kalibrasi baseline... (jangan dekatkan magnet!)
═══════════════════════════════════════════════════════
  BAB 33: Hall Effect Sensor Demo (Core v2.x only!)
═══════════════════════════════════════════════════════
  Baseline (zero-point): 48
  Threshold deteksi    : ±20
  Dekatkan magnet ke bagian bawah chip ESP32!
───────────────────────────────────────────────────────
  [     501 ms] Raw=  50 │ Smooth=  49 │ Delta=  +1 │ ⬜ Tidak ada magnet
  [    1001 ms] Raw=  47 │ Smooth=  48 │ Delta=   0 │ ⬜ Tidak ada magnet

  ← (Magnet kulkas didekatkan dari bawah PCB)

  [    1501 ms] Raw=  82 │ Smooth=  68 │ Delta= +20 │ 🔵 KUTUB UTARA
  [    2001 ms] Raw=  98 │ Smooth=  91 │ Delta= +43 │ 🔵 KUTUB UTARA

  ← (Magnet dibalik, kutub selatan didekatkan)

  [    3001 ms] Raw=  15 │ Smooth=  22 │ Delta= -26 │ 🔴 KUTUB SELATAN
```

---

## 33.7 Program 4: ESP32 System Health Monitor

Program terakhir ini menggabungkan semua sensor internal dan informasi sistem ESP32 menjadi **dashboard kesehatan sistem** yang lengkap.

```cpp
/*
 * BAB 33 - Program 4: ESP32 System Health Monitor
 *
 * Dashboard komprehensif yang menampilkan:
 *   - Suhu internal chip (dengan moving average)
 *   - Uptime sistem (jam:menit:detik)
 *   - Free Heap Memory (deteksi memory leak!)
 *   - CPU Frequency
 *   - Status thermal real-time
 *
 * Arsitektur: 100% non-blocking menggunakan millis()
 * Tidak ada delay() apapun!
 */

// ── Konfigurasi ──────────────────────────────────────────────────────
const float SUHU_WARNING = 60.0f;
const float SUHU_ALARM   = 75.0f;

// ── Moving Average Suhu ──────────────────────────────────────────────
const int  N_SUHU = 8;
float      bufSuhu[N_SUHU];
int        idxSuhu  = 0;
float      sumSuhu  = 0.0f;
float      suhuSmooth = 0.0f;

void tambahSuhu(float s) {
  sumSuhu      -= bufSuhu[idxSuhu];
  bufSuhu[idxSuhu] = s;
  sumSuhu      += s;
  idxSuhu       = (idxSuhu + 1) % N_SUHU;
  suhuSmooth    = sumSuhu / N_SUHU;
}

// ── Timing ───────────────────────────────────────────────────────────
const unsigned long INTERVAL_SUHU_MS      = 1000UL;
const unsigned long INTERVAL_DASHBOARD_MS = 5000UL;
unsigned long tSuhuTerakhir      = 0;
unsigned long tDashboardTerakhir = 0;

// ── State ─────────────────────────────────────────────────────────────
float suhuMin =  999.0f;
float suhuMax = -999.0f;

// ── Utility: Format uptime ke HH:MM:SS ────────────────────────────────
void printUptime(unsigned long ms) {
  unsigned long detik = ms / 1000;
  unsigned long jam   = detik / 3600;
  unsigned long menit = (detik % 3600) / 60;
  unsigned long dtk   = detik % 60;
  Serial.printf("%02lu:%02lu:%02lu", jam, menit, dtk);
}

void setup() {
  Serial.begin(115200);

  float suhuAwal = temperatureRead();
  for (int i = 0; i < N_SUHU; i++) bufSuhu[i] = suhuAwal;
  sumSuhu    = suhuAwal * N_SUHU;
  suhuSmooth = suhuAwal;
  suhuMin    = suhuAwal;
  suhuMax    = suhuAwal;

  Serial.println();
  Serial.println("╔════════════════════════════════════════════════════╗");
  Serial.println("║    BAB 33: ESP32 System Health Monitor v1.0        ║");
  Serial.println("╠════════════════════════════════════════════════════╣");
  Serial.printf ("║  CPU Freq   : %-6u MHz                           ║\n",
                  getCpuFrequencyMhz());
  Serial.printf ("║  Flash Size : %-6u KB                            ║\n",
                  ESP.getFlashChipSize() / 1024);
  Serial.printf ("║  Heap Total : %-8u bytes                       ║\n",
                  ESP.getHeapSize());
  Serial.printf ("║  Suhu Awal  : %-6.1f °C                          ║\n",
                  suhuAwal);
  Serial.println("╚════════════════════════════════════════════════════╝");
  Serial.println();
}

void loop() {
  unsigned long sekarang = millis();

  // ── Update suhu setiap 1 detik ──────────────────────────────────
  if (sekarang - tSuhuTerakhir >= INTERVAL_SUHU_MS) {
    tSuhuTerakhir = sekarang;
    float suhuBaru = temperatureRead();
    tambahSuhu(suhuBaru);
    if (suhuSmooth < suhuMin) suhuMin = suhuSmooth;
    if (suhuSmooth > suhuMax) suhuMax = suhuSmooth;
  }

  // ── Tampilkan dashboard setiap 5 detik ──────────────────────────
  if (sekarang - tDashboardTerakhir >= INTERVAL_DASHBOARD_MS) {
    tDashboardTerakhir = sekarang;

    const char* statusThermal;
    if      (suhuSmooth >= SUHU_ALARM)   statusThermal = "🔥 ALARM!  ";
    else if (suhuSmooth >= SUHU_WARNING) statusThermal = "⚠️ WARNING  ";
    else if (suhuSmooth >= 40.0f)        statusThermal = "✅ NORMAL   ";
    else                                  statusThermal = "❄️ DINGIN   ";

    Serial.println("┌────────────────────────────────────────────────────┐");
    Serial.print  ("│ Uptime    : ");
    printUptime(sekarang);
    Serial.println("                               │");
    Serial.printf ("│ Suhu      : %5.1f °C  [Min:%-4.1f Max:%-4.1f]          │\n",
                   suhuSmooth, suhuMin, suhuMax);
    Serial.printf ("│ Status    : %s                            │\n",
                   statusThermal);
    Serial.printf ("│ Free Heap : %-6u bytes  (dari %-6u total)      │\n",
                   ESP.getFreeHeap(), ESP.getHeapSize());
    Serial.printf ("│ CPU Freq  : %-3u MHz                                │\n",
                   getCpuFrequencyMhz());
    Serial.println("└────────────────────────────────────────────────────┘");
  }
}
```

**Contoh output dashboard:**
```text
╔════════════════════════════════════════════════════╗
║    BAB 33: ESP32 System Health Monitor v1.0        ║
╠════════════════════════════════════════════════════╣
║  CPU Freq   : 240    MHz                           ║
║  Flash Size : 4096   KB                            ║
║  Heap Total : 327680 bytes                         ║
║  Suhu Awal  : 43.2   °C                          ║
╚════════════════════════════════════════════════════╝

┌────────────────────────────────────────────────────┐
│ Uptime    : 00:00:05                               │
│ Suhu      :  43.6 °C  [Min:43.2 Max:43.8]          │
│ Status    : ✅ NORMAL                              │
│ Free Heap : 320456 bytes  (dari 327680 total)      │
│ CPU Freq  : 240 MHz                                │
└────────────────────────────────────────────────────┘
┌────────────────────────────────────────────────────┐
│ Uptime    : 00:00:10                               │
│ Suhu      :  44.1 °C  [Min:43.2 Max:44.3]          │
│ Status    : ✅ NORMAL                              │
│ Free Heap : 320456 bytes  (dari 327680 total)      │
│ CPU Freq  : 240 MHz                                │
└────────────────────────────────────────────────────┘
```

> 💡 **Gunakan Tips Ini:** Amati kolom `Free Heap` selama beberapa menit. Jika angkanya terus **turun** tanpa pernah naik lagi, itu adalah gejala **memory leak** — *bug* yang sangat berbahaya di sistem embedded yang berjalan berhari-hari tanpa restart!

---

## 33.8 Tips & Panduan Troubleshooting

### 1. `temperatureRead()` Tidak Ada / Error Kompilasi?
```text
Penyebab: Arduino ESP32 Core v3.x mengubah beberapa API.

Solusi A — Downgrade ke Core v2.x:
  Tools → Board → Boards Manager → "esp32 by Espressif Systems"
  Pilih versi 2.0.17 (versi 2.x terakhir yang stabil)

Solusi B — Gunakan API baru (Core v3.x):
  #include "driver/temperature_sensor.h"
  temperature_sensor_handle_t temp_handle = NULL;
  temperature_sensor_config_t temp_config = TEMPERATURE_SENSOR_CONFIG_DEFAULT(10, 80);
  temperature_sensor_install(&temp_config, &temp_handle);
  temperature_sensor_enable(temp_handle);
  // Kemudian baca dengan:
  float suhu;
  temperature_sensor_get_celsius(temp_handle, &suhu);
```

### 2. Nilai Suhu Terlalu Tinggi (> 60°C) padahal Ruangan Normal?
```text
Ini NORMAL jika:
  A. Kamu baru saja mengaktifkan WiFi atau Bluetooth
     → Radio WiFi/BT menghasilkan panas internal yang signifikan
     → Solusi: Tunggu 30 detik sampai suhu stabil di titik baru.

  B. CPU sedang menjalankan loop berat (enkripsi, FFT, kompresi)
     → Suhu akan naik proporsional dengan beban

  C. Chip di dalam enclosure tertutup tanpa ventilasi
     → Pertimbangkan heat sink mini atau ventilasi lubang angin
```

### 3. `hallRead()` Selalu Bernilai 0 atau Error?
```text
Kemungkinan penyebab:
  A. Kamu menggunakan Arduino ESP32 Core v3.x
     → hallRead() sudah DIHAPUS. Downgrade ke v2.x.

  B. ADC1 Channel 0/3 (IO36/IO39) dipakai untuk tujuan lain
     → Hall sensor menggunakan ADC1 Ch0 dan Ch3 secara internal
     → Jangan gunakan IO36 dan IO39 saat memakai hallRead()!
```

### 4. Nilai Hall Effect Sangat Berisik (±30-50 poin tanpa magnet)?
```text
Ini adalah karakteristik bawaan. Solusi:
  A. Naikkan N_SAMPLES moving average dari 16 ke 32
  B. Naikkan THRESHOLD_DETEKSI dari 20 ke 40
  C. Gunakan sensor Hall eksternal (AH3503, SS49E) untuk akurasi lebih baik
     — Sensor eksternal ini jauh lebih stabil dan bisa dikalibrasi!
```

---

## 33.9 Ringkasan

```
┌─────────────────────────────────────────────────────────────────────┐
│              RINGKASAN BAB 33 — ESP32 INTERNAL SENSORS               │
├─────────────────────────────────────────────────────────────────────┤
│ TEMPERATURE SENSOR INTERNAL:                                        │
│   API         = temperatureRead()  (Core v2.x)                     │
│   Output      = float (°C), suhu die chip                           │
│   Akurasi     = ±5°C (bukan untuk ukur suhu lingkungan!)           │
│   Dipengaruhi = Beban CPU, WiFi/BT aktif, clock speed              │
│   Gunakan untuk: Thermal protection, health monitoring              │
│                                                                     │
│ HALL EFFECT SENSOR:                                                  │
│   API         = hallRead()  (Core v1.x / v2.x SAJA!)              │
│   Status      = ⚠️ DEPRECATED — Dihapus di Core v3.x!             │
│   Output      = int (tidak bersatuan — relatif terhadap baseline)  │
│   Gunakan ADC1 Ch0/3 (IO36, IO39) secara internal                  │
│   Gangguan     = Sangat tinggi — butuh moving average besar         │
│                                                                     │
│ BEST PRACTICES:                                                     │
│   ✅ Selalu pre-fill buffer moving average di setup()               │
│   ✅ Gunakan threshold + hysteresis untuk deteksi event             │
│   ✅ Monitor Free Heap bersamaan untuk deteksi memory leak          │
│   ✅ Dokumentasikan versi Core yang dibutuhkan di komentar kode     │
│   ⚠️ Jangan pakai IO36/IO39 untuk sensor lain saat hallRead() aktif │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 33.10 Latihan

1. **Thermal Stress Test:** Buat program yang menjalankan CPU busy-loop selama 10 detik (`for(long i=0; i<50000000; i++);`), lalu idle selama 10 detik, bergantian. Plot suhu chip yang naik dan turun! Amati seberapa cepat chip "mendingin" setelah beban berkurang.

2. **Overheating Watchdog:** Modifikasi Program 2 agar jika suhu mencapai `SUHU_ALARM` selama lebih dari 30 detik berturut-turut, sistem melakukan `ESP.restart()` (reboot otomatis). Tambahkan counter di NVS (Non-Volatile Storage) untuk menghitung berapa kali sistem reboot akibat overheating.

3. **Thermal Logger ke Serial Plotter:** Modifikasi Program 1 agar outputnya berformat `suhu_raw,suhu_smooth` (dua angka dipisah koma, tanpa teks lain) sehingga bisa divisualisasikan di **Serial Plotter** Arduino IDE secara real-time. Amati efek moving average dibanding nilai mentah!

4. **Hall Effect Proximity Switch (Core v2.x):** Gunakan Program 3 sebagai basis. Tambahkan sebuah LED. LED menyala jika magnet terdeteksi (delta > threshold) dan mati jika tidak. Ukur jarak deteksi maksimum dengan magnet neodymium yang berbeda ukuran!

5. **Integrasi Sensor Fusion:** Gabungkan data dari BAB 26 (DHT11 — suhu lingkungan) dan BAB 33 (suhu internal chip). Hitung dan tampilkan **perbedaan suhu** antara suhu chip dan suhu ruangan (Δt). Nilai Δt yang besar menunjukkan chip sedang bekerja keras. Ini adalah cara profesional untuk mengestimasi beban kerja processor!

---

*Selanjutnya → [BAB 34: Pemrosesan Data Sensor](../BAB-34/README.md)*
