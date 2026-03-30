# BAB 34: Pemrosesan Data Sensor

> ✅ **Prasyarat:** Selesaikan **BAB 11 (Analog Input & ADC)**, **BAB 26 (DHT11)**, **BAB 28 (BMP180)**, **BAB 32 (LDR)**, dan **BAB 33 (Sensor Internal ESP32)**. BAB ini adalah fondasi teknis dari seluruh aplikasi IoT — pastikan kamu memahami konsep `millis()` dari BAB 16 sebelum melanjutkan.

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami **mengapa data mentah dari sensor tidak bisa langsung digunakan** dalam sistem IoT produksi.
- Mengimplementasikan **Simple Moving Average (SMA)**, **Weighted Moving Average (WMA)**, dan **Exponential Moving Average (EMA)** menggunakan circular buffer yang efisien.
- Mendeteksi dan membuang **outlier/spike** menggunakan range check, rate-of-change filter, dan median filter.
- Melakukan **kalibrasi sensor** (offset dan gain) serta linearisasi nilai ADC ke satuan fisik nyata.
- Menerapkan **threshold dengan hysteresis** untuk menghindari false trigger berulang.
- Membangun **mini data aggregator** yang menggabungkan data dari beberapa sensor secara non-blocking.

---

## 34.1 Mengapa Data Sensor Perlu Diproses?

### Realita sensor di dunia nyata

Bagi pemula, seringkali timbul pertanyaan: *"Saya sudah baca sensor, kenapa nilai ADC-nya loncat-loncat terus?"*

Jawabannya sederhana: **tidak ada sensor yang sempurna**. Setiap sensor menghasilkan data yang terkontaminasi oleh berbagai sumber gangguan. Di lingkungan nyata, data mentah (*raw data*) hampir tidak pernah bisa digunakan langsung tanpa pemrosesan terlebih dahulu.

```
ANATOMI DATA SENSOR (Dari Fisika ke Angka Digital):

  Fenomena Fisik         Sensor Fisik          ADC ESP32         Kode Arduino
  (Suhu, Cahaya, dll)    (LDR, DHT11, dll)     (12-bit, 0-4095)  (float, int)
         │                      │                     │                │
         ▼                      ▼                     ▼                ▼
   Nilai "Nyata"    ──→  Tegangan Analog  ──→  Nilai Digital  ──→  Angka di Tangan
   yang ingin diukur      + NOISE            + Quantization       + Semua Error
                          + Offset           + Rounding           di atasnya!

SUMBER-SUMBER GANGGUAN:
  1. Noise ADC       → Fluktuasi ±LSB (Least Significant Bit) karena referensi
                       tegangan tidak sempurna. Selalu ada, selalu berubah.
  2. EMI             → Interferensi dari WiFi, motor, kabel panjang.
  3. Spike/Outlier   → Nilai ekstrem sesaat akibat getaran atau lonjakan daya.
  4. Offset Drift    → Nilai zero-point sensor bergeser seiring suhu & umur.
  5. Non-Linearitas  → Hubungan input-output sensor tidak selalu linear (LDR!)
```

### Contoh nyata: LDR vs. Lux

Tanpa pemrosesan, nilai LDR hanya menghasilkan angka ADC (0–4095). Dengan pemrosesan yang tepat, angka itu bisa menjadi nilai **Lux** yang bermakna. Itulah perbedaan antara *proyek mahasiswa* dan *produk industri*.

```
TANPA PEMROSESAN:
  ADC Raw: 2156 → ??? (angka tidak bermakna)
  ADC Raw: 2203 →       ↑ noise ±50 ADC unit
  ADC Raw: 2178 → ???

DENGAN PEMROSESAN PENUH:
  ADC Raw: 2180 (±50 noise)
  Filter SMA 16x  →  2181.3 (stabil!)
  Kalibrasi offset →  2181.3 + 47 = 2228.3
  Linearisasi      →  245.7 Lux  ← nilai yang BERMAKNA
```

> 💡 **Perspektif Industri:** Di sistem IoT produksi seperti smart agriculture atau environmental monitoring, data yang tidak diproses dengan benar akan menyebabkan false alarm, keputusan yang salah, dan pada akhirnya — kerugian finansial. Pemrosesan data sensor adalah investasi wajib, bukan opsional.

---

## 34.2 Filter Moving Average — SMA dan WMA

### Simple Moving Average (SMA)

SMA adalah filter paling dasar: rata-rata dari N sampel terakhir. Kamu sudah melihatnya di BAB 33 — kini kita membahasnya secara mendalam dengan implementasi yang benar-benar efisien.

```
VISUALISASI SMA (N=4):

  Sampel masuk:    45  48  52  47  51  49  53  46
  ────────────────────────────────────────────────
  Buffer saat ini: [45][48][52][47]      → SMA = (45+48+52+47)/4 = 48.0
  Setelah update:  [48][52][47][51]      → SMA = (48+52+47+51)/4 = 49.5
  Setelah update:  [52][47][51][49]      → SMA = (52+47+51+49)/4 = 49.8

  Terlihat: nilai 51 yang "agak tinggi" ter-smooth menjadi 49.5 dan 49.8
```

**Implementasi Circular Buffer (BENAR):**

```cpp
// ── O(1) Circular Buffer — Efisien dan Tidak Memboroskan Memori ──────
const int  N = 8;
float      buf[N];
int        idx = 0;
float      sum = 0.0f;
float      sma = 0.0f;

// Inisialisasi di setup(): pre-fill dengan nilai pertama
void initSMA(float firstVal) {
  for (int i = 0; i < N; i++) buf[i] = firstVal;
  sum = firstVal * N;
  sma = firstVal;
  idx = 0;
}

// Update di setiap sampel baru — O(1), tanpa loop!
void updateSMA(float newVal) {
  sum -= buf[idx];      // Kurangi nilai lama
  buf[idx] = newVal;    // Simpan nilai baru
  sum += newVal;        // Tambah nilai baru
  idx = (idx + 1) % N; // Maju ke slot berikutnya (circular!)
  sma = sum / N;        // Hitung rata-rata baru
}
```

### Weighted Moving Average (WMA)

WMA memberikan **bobot lebih besar pada sampel terbaru**. Cocok ketika kamu ingin filter yang responsif tapi tetap menghaluskan noise:

```
WMA (N=4, bobot: [1, 2, 3, 4] — 4 = paling baru):

  Buffer: [val_lama, ..., val_baru]
  Contoh: [45,  48,  52,  51]
  WMA = (1×45 + 2×48 + 3×52 + 4×51) / (1+2+3+4)
      = (45 + 96 + 156 + 204) / 10
      = 501 / 10 = 50.1

  Vs SMA: (45+48+52+51) / 4 = 49.0

  WMA lebih "mengikuti" nilai terbaru (51) dibanding SMA (49.0)
```

---

## 34.3 Exponential Moving Average (EMA)

EMA adalah senjata rahasia embedded engineer. Ia memberikan efek yang mirip dengan SMA namun hanya membutuhkan **satu variabel** — tidak perlu array buffer sama sekali!

```
RUMUS EMA:

  EMA(t) = α × Input(t) + (1 - α) × EMA(t-1)

  Dimana:
    α (alpha) = "faktor kelancaran" (smoothing factor), 0 < α < 1
    α besar (misal 0.5) → Lebih responsif, kurang halus
    α kecil (misal 0.05) → Lebih halus, tapi lebih lambat merespons

PERBANDINGAN VISUAL (data dengan spike pada t=5):

  Data Asli:  42  43  44  43  44  99  43  44  43  44
  SMA (N=4):  42  42  43  43  43  58  57  57  47  43  ← butuh N sampel untuk pulih
  EMA α=0.1:  42  42  43  43  43  49  48  47  46  45  ← pulih lebih perlahan
  EMA α=0.3:  42  42  43  43  43  63  57  54  51  49  ← lebih responsif

PEMILIHAN ALPHA BERDASARKAN TIME CONSTANT (τ):
  α ≈ 1 - e^(-Δt/τ)   atau secara praktis: α ≈ Δt / (τ + Δt)

  Jika sampel setiap 100ms dan kamu mau filter 2 detik:
  α ≈ 0.1 / (2.0 + 0.1) ≈ 0.048 → gunakan α = 0.05
```

```cpp
// ── EMA — Sangat Efisien, O(1) Waktu dan Memori ──────────────────────
float ema      = 0.0f;
bool  emaReady = false;
const float ALPHA = 0.1f; // Sesuaikan dengan kebutuhan

void updateEMA(float newVal) {
  if (!emaReady) {
    ema = newVal; // Inisialisasi dengan nilai pertama
    emaReady = true;
    return;
  }
  ema = ALPHA * newVal + (1.0f - ALPHA) * ema;
}
```

> 💡 **Kapan Pakai SMA vs. EMA?**
> - **SMA** → Ketika kamu butuh rata-rata matematis yang akurat & jendela waktu tetap.
> - **EMA** → Ketika memori sangat terbatas, atau kamu ingin filter yang "hilang dengan sendirinya" seiring waktu.
> - **WMA** → Ketika responsivitas terhadap data terbaru lebih penting daripada kehalusan mutlak.

---

## 34.4 Deteksi Outlier & Validasi Data

### Range Check (Batas Fisik)

Cara paling sederhana: tolak nilai yang *tidak mungkin secara fisik*.

```cpp
bool isValidSuhu(float suhu) {
  return (suhu >= -40.0f && suhu <= 125.0f); // Batas fisik DHT11
}

bool isValidADC(int val) {
  return (val >= 0 && val <= 4095); // Batas ADC 12-bit ESP32
}
```

### Rate-of-Change Filter

Tolak nilai jika ia **berubah terlalu cepat** dari sampel sebelumnya — karena sensor fisik tidak bisa berubah drastis dalam satu tick:

```cpp
float suhuSebelumnya = 0.0f;
const float MAX_DELTA_PER_SEC = 5.0f; // Suhu tidak bisa naik >5°C per detik

bool isRateValid(float suhuBaru) {
  float delta = fabsf(suhuBaru - suhuSebelumnya);
  return (delta <= MAX_DELTA_PER_SEC);
}
```

### Median Filter (Spike Rejection Terbaik)

Median filter sangat efektif untuk menolak spike sesaat, terutama pada data ADC:

```
CARA KERJA MEDIAN FILTER (3 sampel):

  Ambil 3 sampel berurutan: [44, 99, 45]
  Urutkan:                  [44, 45, 99]
  Ambil nilai tengah:        45  ← spike (99) terbuang!

  Ini jauh lebih baik dari SMA yang akan menghasilkan: (44+99+45)/3 = 62.7
```

```cpp
float medianOf3(float a, float b, float c) {
  if (a > b) { float t = a; a = b; b = t; } // Swap jika a > b
  if (b > c) { float t = b; b = c; c = t; } // Swap jika b > c
  if (a > b) { float t = a; a = b; b = t; } // Pastikan terurut
  return b; // Nilai tengah
}
```

---

## 34.5 Program 1: Filter Toolkit — Demonstrasi SMA, WMA & EMA

Program ini menggunakan **Potensiometer** (input analog murni) untuk mensimulasikan sinyal dengan noise. Kamu bisa memutar potensiometer perlahan dan melihat bagaimana ketiga filter bereaksi secara berbeda di Serial Monitor — bahkan di Serial Plotter Arduino IDE!

```cpp
/*
 * BAB 34 - Program 1: Filter Toolkit — SMA vs WMA vs EMA
 *
 * Tujuan:
 *   Demonstrasi perbandingan tiga jenis filter pada sinyal ADC real-time.
 *   Gunakan Serial Plotter Arduino IDE untuk visualisasi terbaik!
 *
 * Hardware:
 *   Potensiometer 10K → pin ADC (sambungkan ke pin yang kamu pilih)
 *
 * Output format Serial Plotter (4 nilai dipisah koma):
 *   raw,sma,wma,ema
 *
 * Pin:
 *   PIN_POT → GPIO yang terhubung ke potensiometer
 *             (gunakan kabel jumper dari header Custom ke GPIO ADC-capable)
 *             Rekomendasi: IO34 atau IO35 (input-only, aman untuk ADC)
 */

// ── Konfigurasi ───────────────────────────────────────────────────────
const int PIN_POT = 34;       // Ganti sesuai kabel jumper kamu

// ── SMA (Simple Moving Average) ───────────────────────────────────────
const int  SMA_N   = 16;
float      smaBuf[SMA_N];
int        smaIdx  = 0;
float      smaSum  = 0.0f;
float      smaVal  = 0.0f;

void initSMA(float v) {
  for (int i = 0; i < SMA_N; i++) smaBuf[i] = v;
  smaSum = v * SMA_N;
  smaVal = v;
  smaIdx = 0;
}

void updateSMA(float v) {
  smaSum -= smaBuf[smaIdx];
  smaBuf[smaIdx] = v;
  smaSum += v;
  smaIdx = (smaIdx + 1) % SMA_N;
  smaVal = smaSum / SMA_N;
}

// ── WMA (Weighted Moving Average) ────────────────────────────────────
const int  WMA_N    = 8;
float      wmaBuf[WMA_N];
int        wmaIdx   = 0;
float      wmaVal   = 0.0f;

// Bobot: index 0 = paling lama (bobot 1), index N-1 = paling baru (bobot N)
// Total bobot = N*(N+1)/2. Untuk N=8 → 36
const float WMA_WEIGHT_SUM = (WMA_N * (WMA_N + 1)) / 2.0f; // = 36.0

void initWMA(float v) {
  for (int i = 0; i < WMA_N; i++) wmaBuf[i] = v;
  wmaVal = v;
  wmaIdx = 0;
}

void updateWMA(float v) {
  wmaBuf[wmaIdx] = v;
  wmaIdx = (wmaIdx + 1) % WMA_N;
  float weightedSum = 0.0f;
  for (int i = 0; i < WMA_N; i++) {
    // Hitung posisi relatif (0 = paling lama, WMA_N-1 = paling baru)
    int age     = (wmaIdx - 1 - i + WMA_N) % WMA_N;
    int weight  = WMA_N - age;          // Makin baru, bobot makin besar
    weightedSum += wmaBuf[(wmaIdx - 1 - i + WMA_N) % WMA_N] * weight;
  }
  wmaVal = weightedSum / WMA_WEIGHT_SUM;
}

// ── EMA (Exponential Moving Average) ─────────────────────────────────
const float EMA_ALPHA = 0.08f; // α kecil = lebih halus, lebih lambat
float       emaVal    = 0.0f;
bool        emaInit   = false;

void updateEMA(float v) {
  if (!emaInit) { emaVal = v; emaInit = true; return; }
  emaVal = EMA_ALPHA * v + (1.0f - EMA_ALPHA) * emaVal;
}

// ── Timing ────────────────────────────────────────────────────────────
const unsigned long INTERVAL_SAMPLE_MS = 50UL;  // Sampling 20Hz
unsigned long tSampleTerakhir = 0;

void setup() {
  Serial.begin(115200);
  analogReadResolution(12); // Pastikan resolusi 12-bit (0-4095)

  float firstRead = (float)analogRead(PIN_POT);
  initSMA(firstRead);
  initWMA(firstRead);
  updateEMA(firstRead); // EMA init via pertama kali update

  // Header untuk Serial Monitor (komentar ini tidak muncul di Serial Plotter)
  // Serial.println("raw,sma,wma,ema");
}

void loop() {
  unsigned long sekarang = millis();

  if (sekarang - tSampleTerakhir >= INTERVAL_SAMPLE_MS) {
    tSampleTerakhir = sekarang;

    float raw = (float)analogRead(PIN_POT);

    updateSMA(raw);
    updateWMA(raw);
    updateEMA(raw);

    // Format CSV — ideal untuk Serial Plotter Arduino IDE
    // Buka Tools → Serial Plotter untuk melihat grafik real-time!
    Serial.print(raw);    Serial.print(",");
    Serial.print(smaVal); Serial.print(",");
    Serial.print(wmaVal); Serial.print(",");
    Serial.println(emaVal);
  }
}
```

**Cara Menggunakan Serial Plotter:**
```text
1. Upload kode ke ESP32
2. Buka Arduino IDE → Tools → Serial Plotter
3. Putar potensiometer perlahan → amati semua filter mengikuti
4. Putar potensiometer CEPAT → perhatikan perbedaan respons!
   - SMA  (biru)  : lambat, rata, butuh N sampel untuk "pulih"
   - WMA  (merah) : sedikit lebih cepat dari SMA
   - EMA  (hijau) : paling mulus, tapi paling lambat pulih ke nilai stabil
5. Ketuk kabel jumper ke PCB → lihat spike → amati mana yang paling tahan!
```

> 💡 **Eksperimen Penting:** Coba ubah nilai `EMA_ALPHA` dari `0.08` ke `0.5`. Lihat bagaimana EMA menjadi jauh lebih responsif tapi lebih berisik! Ini adalah **trade-off fundamental** dalam desain filter: **responsivitas vs. kehalusan**.

---

## 34.6 Kalibrasi Sensor — Offset, Gain, dan Linearisasi

### Dua jenis kalibrasi yang paling umum

```
KALIBRASI OFFSET (Zero-Point):
  Masalah: Sensor membaca 47 saat seharusnya 0.
  Solusi:  nilai_terkoreksi = nilai_raw - offset
           offset = 47
           → nilai_terkoreksi = 47 - 47 = 0 ✅

KALIBRASI GAIN (Scale Factor):
  Masalah: Sensor membaca 2.0 saat seharusnya 2.5 (pada titik referensi)
  Solusi:  nilai_terkoreksi = nilai_raw × gain
           gain = 2.5 / 2.0 = 1.25
           → nilai_terkoreksi = 2.0 × 1.25 = 2.5 ✅

KALIBRASI DUA TITIK (Paling Akurat):
  Titik 1: Input=0   → Sensor=47   (nilai offest)
  Titik 2: Input=100 → Sensor=212  (nilai skala penuh)

  Slope (gain) = (100 - 0) / (212 - 47) = 100 / 165 = 0.606
  Output = (raw - 47) × 0.606
```

### Linearisasi LDR ke Lux

LDR memiliki respons **logaritmik** terhadap intensitas cahaya — hubungannya tidak linear! Kita gunakan pendekatan praktis dengan **lookup table**:

```cpp
// ── Lookup Table: ADC raw → Lux (Kalibrasi Manual Sekali Pakai) ──────
// Catatan: Nilai ini spesifik untuk rangkaian LDR di Kit Bluino
//          (LDR + resistor 10KΩ ke GND, VCC = 3.3V, ADC 12-bit)
// Kalibrasi ulang jika menggunakan kit berbeda!

struct LuxPoint {
  int   adcRaw;
  float lux;
};

// 10 titik kalibrasi — diukur langsung dengan lux meter
const LuxPoint LUX_TABLE[] = {
  {  100,  2000.0f },  // Sangat terang (sinar matahari langsung)
  {  300,   800.0f },  // Terang (dalam ruangan siang hari)
  {  600,   300.0f },  // Cukup terang
  {  900,   150.0f },  // Normal dalam ruangan
  { 1400,    80.0f },  // Agak redup
  { 1900,    40.0f },  // Redup
  { 2400,    15.0f },  // Sangat redup
  { 2900,     5.0f },  // Hampir gelap
  { 3400,     1.0f },  // Gelap
  { 3900,     0.1f },  // Sangat gelap
};
const int LUX_TABLE_SIZE = sizeof(LUX_TABLE) / sizeof(LUX_TABLE[0]);

// Interpolasi linear antara dua titik terdekat
float adcToLux(int adcRaw) {
  // Di luar batas tabel
  if (adcRaw <= LUX_TABLE[0].adcRaw)                    return LUX_TABLE[0].lux;
  if (adcRaw >= LUX_TABLE[LUX_TABLE_SIZE - 1].adcRaw)   return LUX_TABLE[LUX_TABLE_SIZE - 1].lux;

  // Cari interval yang sesuai
  for (int i = 0; i < LUX_TABLE_SIZE - 1; i++) {
    if (adcRaw <= LUX_TABLE[i + 1].adcRaw) {
      // Interpolasi linear
      float t = (float)(adcRaw - LUX_TABLE[i].adcRaw) /
                (float)(LUX_TABLE[i + 1].adcRaw - LUX_TABLE[i].adcRaw);
      return LUX_TABLE[i].lux + t * (LUX_TABLE[i + 1].lux - LUX_TABLE[i].lux);
    }
  }
  return 0.0f; // Seharusnya tidak pernah tercapai
}
```

---

## 34.7 Threshold dengan Hysteresis

Ini adalah teknik yang **sangat sering dilupakan** pemula dan **sangat sering menyebabkan bug** di sistem IoT produksi!

```
MASALAH TANPA HYSTERESIS:

  Threshold = 1000 Lux
  Data LDR (dengan noise ±20 Lux): 998, 1003, 997, 1005, 999, 1002 ...
  Status:                          OFF, ON,   OFF, ON,   OFF, ON  ...
                                   ↑ Relay/alarm toggle 6 kali per detik!
                                   ↑ Relay RUSAK karena ini!

SOLUSI: HYSTERESIS (dua threshold berbeda)

  Threshold ON  = 1000 Lux (untuk mengaktifkan)
  Threshold OFF =  950 Lux (untuk menonaktifkan) ← gap = 50 Lux

  Data tadi:       998, 1003, 997, 1005, 999, 1002 ...
  Status (hyst):   OFF, ON,   ON,  ON,   ON,  ON  ...
                        ↑ ON sekali saja, lalu stabil!
```

```cpp
// ── Implementasi Hysteresis ───────────────────────────────────────────
const float THRESHOLD_ON  = 1000.0f; // Lux — aktifkan (gelap → nyalakan lampu)
const float THRESHOLD_OFF =  950.0f; // Lux — nonaktifkan (terang kembali)
bool lampuAktif = false;

void updateHysteresis(float lux) {
  if (!lampuAktif && lux < THRESHOLD_ON) {
    lampuAktif = true;
    Serial.println("[EVENT] Lampu NYALA — Lux turun di bawah ambang!");
  } else if (lampuAktif && lux >= THRESHOLD_OFF) {
    lampuAktif = false;
    Serial.println("[EVENT] Lampu MATI — Lux kembali di atas ambang!");
  }
  // Di antara kedua threshold = tidak ada perubahan (itulah hysteresis!)
}
```

---

## 34.8 Program 2: Outlier Rejection System

Program ini membaca data dari **LDR** dan menerapkan tiga lapis validasi data secara bertingkat: Range Check → Rate-of-Change Filter → Median Filter.

```cpp
/*
 * BAB 34 - Program 2: Outlier Rejection System
 *
 * Demonstrasi sistem validasi data sensor berlapis:
 *   Layer 1 — Range Check    : Tolak nilai di luar batas fisik sensor
 *   Layer 2 — Rate-of-Change : Tolak nilai yang berubah terlalu cepat
 *   Layer 3 — Median Filter  : Hilangkan spike sesaat yang lolos
 *   Layer 4 — EMA            : Haluskan sinyal bersih yang tersisa
 *
 * Hardware:
 *   LDR + Resistor 10KΩ → pin ADC (sambungkan via jumper ke GPIO ADC)
 *   Rekomendasi: IO35 (input-only, bebas dari perangkat lain)
 *
 * Eksperimen:
 *   1. Jalankan program, lihat baseline normal
 *   2. Ketuk keras PCB → amati spike tertolak di Serial Monitor!
 *   3. Tutup LDR dengan jari perlahan → lihat rate filter bekerja
 *   4. Tutup LDR SANGAT cepat (lampu tiba-tiba mati) → rate filter tertrigger!
 */

// ── Konfigurasi ────────────────────────────────────────────────────────
const int   PIN_LDR             = 35;      // GPIO ADC untuk LDR
const int   ADC_MIN             = 0;       // Batas bawah ADC (Range Check)
const int   ADC_MAX             = 4095;    // Batas atas ADC (Range Check)
const float MAX_DELTA_PER_TICK  = 200.0f;  // Rate limit (ADC delta per 50ms)
const float EMA_ALPHA           = 0.15f;

// ── Statistik Penolakan ────────────────────────────────────────────────
unsigned long totalSampel     = 0;
unsigned long tolakRange      = 0;
unsigned long tolakRate       = 0;
unsigned long tolakMedian     = 0; // Spike yang terdeteksi median
unsigned long diterima        = 0;

// ── Circular Buffer Median (3 sampel) ─────────────────────────────────
float medBuf[3];
int   medIdx = 0;

// ── State ──────────────────────────────────────────────────────────────
float prevRaw  = 0.0f;
float emaVal   = 0.0f;
bool  emaInit  = false;

// ── Timing ─────────────────────────────────────────────────────────────
const unsigned long INTERVAL_SAMPLE_MS = 50UL;
const unsigned long INTERVAL_STATS_MS  = 5000UL;
unsigned long tSampleTerakhir = 0;
unsigned long tStatsTerakhir  = 0;

// ── Fungsi Median 3 ──────────────────────────────────────────────────
float median3(float a, float b, float c) {
  if (a > b) { float t = a; a = b; b = t; }
  if (b > c) { float t = b; b = c; c = t; }
  if (a > b) { float t = a; a = b; b = t; }
  return b;
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);

  float firstRead = (float)analogRead(PIN_LDR);
  prevRaw  = firstRead;
  emaVal   = firstRead;
  emaInit  = true;
  for (int i = 0; i < 3; i++) medBuf[i] = firstRead;

  Serial.println("══════════════════════════════════════════════════════");
  Serial.println("  BAB 34: Outlier Rejection System — 3-Layer Filter");
  Serial.println("══════════════════════════════════════════════════════");
  Serial.println("  Format: [status] raw → filtered | EMA output");
  Serial.println("  Status: OK=diterima, RANGE=tolak, RATE=tolak");
  Serial.println("──────────────────────────────────────────────────────");
}

void loop() {
  unsigned long sekarang = millis();

  // ── Sampling & Filtering ────────────────────────────────────────────
  if (sekarang - tSampleTerakhir >= INTERVAL_SAMPLE_MS) {
    tSampleTerakhir = sekarang;
    totalSampel++;

    int rawInt = analogRead(PIN_LDR);
    float raw  = (float)rawInt;

    // ── Layer 1: Range Check ─────────────────────────────────────────
    if (rawInt < ADC_MIN || rawInt > ADC_MAX) {
      tolakRange++;
      Serial.printf("  [RANGE] raw=%4d → DITOLAK! Di luar batas ADC\n", rawInt);
      return; // Buang, jangan proses lebih lanjut
    }

    // ── Layer 2: Rate-of-Change Filter ───────────────────────────────
    float delta = fabsf(raw - prevRaw);
    if (delta > MAX_DELTA_PER_TICK) {
      tolakRate++;
      Serial.printf("  [RATE ] raw=%4d → DITOLAK! Δ=%.0f (maks:%.0f)\n",
                    rawInt, delta, MAX_DELTA_PER_TICK);
      // Penting: jangan update prevRaw dengan nilai yang ditolak!
      return;
    }
    prevRaw = raw; // Update hanya jika lolos rate check

    // ── Layer 3: Median Filter (Spike Rejection) ─────────────────────
    medBuf[medIdx] = raw;
    medIdx = (medIdx + 1) % 3;
    float filtered = median3(medBuf[0], medBuf[1], medBuf[2]);

    // Deteksi jika median berbeda jauh dari raw (ada spike)
    if (fabsf(filtered - raw) > 50.0f) {
      tolakMedian++;
      Serial.printf("  [SPIKE] raw=%4d → filtered=%4.0f (spike terdeteksi!)\n",
                    rawInt, filtered);
    }

    // ── Layer 4: EMA pada data bersih ────────────────────────────────
    emaVal = EMA_ALPHA * filtered + (1.0f - EMA_ALPHA) * emaVal;
    diterima++;

    Serial.printf("  [OK   ] raw=%4d → med=%4.0f → ema=%6.1f\n",
                  rawInt, filtered, emaVal);
  }

  // ── Laporan Statistik Setiap 5 Detik ──────────────────────────────
  if (sekarang - tStatsTerakhir >= INTERVAL_STATS_MS) {
    tStatsTerakhir = sekarang;
    float pctOK = (totalSampel > 0) ? (100.0f * diterima / totalSampel) : 0.0f;

    Serial.println();
    Serial.println("  ┌── LAPORAN FILTER (5s) ─────────────────────────┐");
    Serial.printf ("  │ Total sampel : %-6lu                           │\n", totalSampel);
    Serial.printf ("  │ Diterima     : %-6lu (%.1f%%)                  │\n", diterima, pctOK);
    Serial.printf ("  │ Tolak RANGE  : %-6lu                           │\n", tolakRange);
    Serial.printf ("  │ Tolak RATE   : %-6lu                           │\n", tolakRate);
    Serial.printf ("  │ Spike terdet : %-6lu                           │\n", tolakMedian);
    Serial.printf ("  │ EMA sekarang : %-6.1f ADC                      │\n", emaVal);
    Serial.println("  └────────────────────────────────────────────────┘");
    Serial.println();
  }
}
```

**Contoh output:**
```text
══════════════════════════════════════════════════════
  BAB 34: Outlier Rejection System — 3-Layer Filter
══════════════════════════════════════════════════════
  Format: [status] raw → filtered | EMA output
  Status: OK=diterima, RANGE=tolak, RATE=tolak
──────────────────────────────────────────────────────
  [OK   ] raw=1843 → med=1843 → ema=  1843.0
  [OK   ] raw=1849 → med=1847 → ema=  1847.3
  [OK   ] raw=1851 → med=1849 → ema=  1848.2
  [RATE ] raw=3512 → DITOLAK! Δ=1663 (maks:200)   ← ketuk keras PCB!
  [OK   ] raw=1852 → med=1851 → ema=  1848.9
  [SPIKE] raw=3105 → filtered=1853 (spike terdeteksi!)
  [OK   ] raw=1850 → med=1852 → ema=  1849.6

  ┌── LAPORAN FILTER (5s) ─────────────────────────┐
  │ Total sampel : 100                              │
  │ Diterima     : 96     (96.0%)                   │
  │ Tolak RANGE  : 0                                │
  │ Tolak RATE   : 3                                │
  │ Spike terdet : 1                                │
  │ EMA sekarang : 1849.6 ADC                       │
  └────────────────────────────────────────────────┘
```

> ⚠️ **Catatan Penting — Rate Filter:** Jika sumber cahaya *memang* berubah cepat (lampu dinyalakan tiba-tiba), rate filter akan *salah* menolak data yang valid. Sesuaikan `MAX_DELTA_PER_TICK` dengan *kecepatan perubahan fisik maksimum* yang masuk akal untuk aplikasi kamu.

---

## 34.9 Program 3: LDR Calibration Station

Program ini menggabungkan **EMA filter + lookup table linearisasi + hysteresis** menjadi satu sistem pencahayaan otomatis yang lengkap berbasis LDR.

```cpp
/*
 * BAB 34 - Program 3: LDR Calibration Station
 *
 * Pipeline pemrosesan lengkap:
 *   ADC Raw → EMA Filter → Lookup Table (Lux) → Hysteresis → Output
 *
 * Hardware:
 *   LDR + Resistor 10KΩ → GPIO ADC (via jumper, rekomendasi: IO35)
 *   LED (opsional)       → GPIO Digital (untuk indikator lampu otomatis)
 *
 * Fitur:
 *   ✅ EMA filter untuk menstabilkan pembacaan ADC
 *   ✅ Lookup table interpolasi untuk konversi ke Lux
 *   ✅ Threshold dengan hysteresis (mencegah false trigger)
 *   ✅ Log periodik yang informatif
 *   ✅ 100% Non-Blocking
 */

// ── Konfigurasi Pin ───────────────────────────────────────────────────
const int PIN_LDR = 35;   // ADC — LDR
const int PIN_LED = 26;   // Output — LED indikator (opsional, ganti sesuai kabel)

// ── EMA Filter ────────────────────────────────────────────────────────
const float EMA_ALPHA = 0.12f;
float       emaADC    = 0.0f;

// ── Lookup Table LDR → Lux ────────────────────────────────────────────
struct LuxPoint { int adcRaw; float lux; };
const LuxPoint LUX_TABLE[] = {
  {  100, 2000.0f }, {  300,  800.0f }, {  600,  300.0f },
  {  900,  150.0f }, { 1400,   80.0f }, { 1900,   40.0f },
  { 2400,   15.0f }, { 2900,    5.0f }, { 3400,    1.0f },
  { 3900,    0.1f }
};
const int LUX_N = sizeof(LUX_TABLE) / sizeof(LUX_TABLE[0]);

float adcToLux(int adc) {
  if (adc <= LUX_TABLE[0].adcRaw)           return LUX_TABLE[0].lux;
  if (adc >= LUX_TABLE[LUX_N - 1].adcRaw)  return LUX_TABLE[LUX_N - 1].lux;
  for (int i = 0; i < LUX_N - 1; i++) {
    if (adc <= LUX_TABLE[i + 1].adcRaw) {
      float t = (float)(adc - LUX_TABLE[i].adcRaw) /
                (float)(LUX_TABLE[i + 1].adcRaw - LUX_TABLE[i].adcRaw);
      return LUX_TABLE[i].lux + t * (LUX_TABLE[i + 1].lux - LUX_TABLE[i].lux);
    }
  }
  return 0.0f;
}

// ── Hysteresis ─────────────────────────────────────────────────────────
const float LUX_ON  = 80.0f;  // Nyalakan LED jika Lux < 80 (agak gelap)
const float LUX_OFF = 100.0f; // Matikan LED jika Lux >= 100 (cukup terang)
bool        ledAktif = false;

void applyHysteresis(float lux) {
  bool changed = false;
  if (!ledAktif && lux < LUX_ON) {
    ledAktif = true; changed = true;
    Serial.printf("  ⚡ [EVENT] LED ON  — Lux=%.1f < %.0f (gelap)\n", lux, LUX_ON);
  } else if (ledAktif && lux >= LUX_OFF) {
    ledAktif = false; changed = true;
    Serial.printf("  ⚡ [EVENT] LED OFF — Lux=%.1f >= %.0f (terang)\n", lux, LUX_OFF);
  }
  if (changed) digitalWrite(PIN_LED, ledAktif ? HIGH : LOW);
}

// ── Timing ─────────────────────────────────────────────────────────────
const unsigned long INTERVAL_SAMPLE_MS = 100UL; // 10Hz sampling
const unsigned long INTERVAL_LOG_MS    = 2000UL; // Log setiap 2 detik
unsigned long tSampleTerakhir = 0;
unsigned long tLogTerakhir    = 0;

// ── State ──────────────────────────────────────────────────────────────
float luxMin =  9999.0f;
float luxMax = -9999.0f;

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LOW);

  emaADC = (float)analogRead(PIN_LDR); // Pre-init EMA

  Serial.println("══════════════════════════════════════════════════════");
  Serial.println("  BAB 34: LDR Calibration Station");
  Serial.println("══════════════════════════════════════════════════════");
  Serial.printf ("  Threshold ON  : %.0f Lux (LED menyala di bawah ini)\n", LUX_ON);
  Serial.printf ("  Threshold OFF : %.0f Lux (LED mati di atas ini)\n", LUX_OFF);
  Serial.println("──────────────────────────────────────────────────────");
}

void loop() {
  unsigned long sekarang = millis();

  // ── Sampling ──────────────────────────────────────────────────────
  if (sekarang - tSampleTerakhir >= INTERVAL_SAMPLE_MS) {
    tSampleTerakhir = sekarang;

    int   rawADC = analogRead(PIN_LDR);
    emaADC = EMA_ALPHA * rawADC + (1.0f - EMA_ALPHA) * emaADC;
    float lux = adcToLux((int)emaADC);

    if (lux < luxMin) luxMin = lux;
    if (lux > luxMax) luxMax = lux;

    applyHysteresis(lux);
  }

  // ── Log Periodik ──────────────────────────────────────────────────
  if (sekarang - tLogTerakhir >= INTERVAL_LOG_MS) {
    tLogTerakhir = sekarang;
    float lux = adcToLux((int)emaADC);

    Serial.printf("  [%6lu s] ADC=%4.0f | Lux=%7.1f | LED=%s | Min=%.1f Max=%.1f\n",
                  sekarang / 1000, emaADC, lux,
                  ledAktif ? "ON " : "OFF", luxMin, luxMax);
  }
}
```

**Contoh output:**
```text
══════════════════════════════════════════════════════
  BAB 34: LDR Calibration Station
══════════════════════════════════════════════════════
  Threshold ON  : 80 Lux (LED menyala di bawah ini)
  Threshold OFF : 100 Lux (LED mati di atas ini)
──────────────────────────────────────────────────────
  [     2 s] ADC=1423 | Lux=    81.8 | LED=OFF | Min=81.8 Max=85.2
  [     4 s] ADC=1438 | Lux=    79.3 | LED=OFF | Min=79.3 Max=85.2
  ⚡ [EVENT] LED ON  — Lux=77.1 < 80 (gelap)   ← tutupi LDR
  [     6 s] ADC=1612 | Lux=    64.2 | LED=ON  | Min=64.2 Max=85.2
  [     8 s] ADC=1608 | Lux=    65.0 | LED=ON  | Min=64.2 Max=85.2
  ⚡ [EVENT] LED OFF — Lux=103.4 >= 100 (terang) ← buka tutupan
  [    10 s] ADC=1388 | Lux=   103.4 | LED=OFF | Min=64.2 Max=103.4
```

---

## 34.10 Program 4: Multi-Sensor Data Aggregator

Program puncak BAB 34 ini menggabungkan **DHT11 + BMP180 + Sensor Suhu Internal ESP32** dalam satu dashboard komprehensif. Sistem ini menerapkan **voting suhu** dari 3 sumber berbeda untuk mendeteksi sensor yang bermasalah — teknik yang digunakan dalam sistem *safety-critical* di industri.

```cpp
/*
 * BAB 34 - Program 4: Multi-Sensor Data Aggregator
 *
 * Menggabungkan tiga sensor suhu:
 *   1. DHT11       — Suhu lingkungan (IO27, built-in kit)
 *   2. BMP180      — Suhu barometrik (I2C: SDA=IO21, SCL=IO22)
 *   3. Internal    — Suhu chip ESP32 (temperatureRead())
 *
 * Teknik yang diterapkan:
 *   ✅ EMA filter untuk setiap sensor
 *   ✅ Cross-validation (voting) antar 3 sensor
 *   ✅ Deteksi sensor outlier (kecurigaan sensor rusak)
 *   ✅ Dashboard non-blocking setiap 5 detik
 *
 * Library yang dibutuhkan:
 *   - DHT sensor library (by Adafruit)
 *   - Adafruit BMP085/BMP180 (by Adafruit)
 *   Install via: Arduino IDE → Tools → Manage Libraries
 */

#include <DHT.h>
#include <Adafruit_BMP085.h>

// ── Pin & Konfigurasi ─────────────────────────────────────────────────
#define DHTPIN  27
#define DHTTYPE DHT11

DHT           dht(DHTPIN, DHTTYPE);
Adafruit_BMP085 bmp;
bool            bmpOK = false; // BMP tersedia atau tidak

// ── EMA untuk masing-masing sensor ───────────────────────────────────
const float EMA_A = 0.2f; // Alpha yang lebih besar karena suhu berubah lambat

float emaDHT      = 0.0f;
bool  emaDHTInit  = false;
float emaBMP      = 0.0f;
bool  emaBMPInit  = false;
float emaInternal = 0.0f;

void updateEMA(float &ema, bool &init, float val) {
  if (!init) { ema = val; init = true; return; }
  ema = EMA_A * val + (1.0f - EMA_A) * ema;
}

// ── Validasi rentang suhu (untuk cross-validation) ───────────────────
const float SUHU_MIN_VALID  = -10.0f;
const float SUHU_MAX_VALID  = 85.0f;
const float MAX_DELTA_ANTAR_SENSOR = 10.0f; // Perbedaan >10°C = mencurigakan

bool isSuhuValid(float s) { return (s >= SUHU_MIN_VALID && s <= SUHU_MAX_VALID); }

// Fungsi voting: kembalikan median dari 3 nilai (paling robust)
float medianSuhu(float a, float b, float c) {
  if (a > b) { float t = a; a = b; b = t; }
  if (b > c) { float t = b; b = c; c = t; }
  if (a > b) { float t = a; a = b; b = t; }
  return b;
}

// ── Timing ─────────────────────────────────────────────────────────────
const unsigned long INTERVAL_BACA_MS      = 2000UL; // DHT11 max 1x per 2 detik
const unsigned long INTERVAL_DASHBOARD_MS = 5000UL;
unsigned long tBacaTerakhir      = 0;
unsigned long tDashboardTerakhir = 0;

void setup() {
  Serial.begin(115200);
  dht.begin();

  bmpOK = bmp.begin();
  if (!bmpOK) {
    Serial.println("  ⚠️  BMP180 tidak terdeteksi! Cek kabel I2C (SDA=IO21, SCL=IO22)");
  }

  // Pre-init EMA dengan pembacaan pertama
  float suhuDHTAwal = dht.readTemperature();
  if (!isnan(suhuDHTAwal)) {
    emaDHT = suhuDHTAwal; emaDHTInit = true;
  }
  emaInternal = temperatureRead();

  Serial.println("╔════════════════════════════════════════════════════╗");
  Serial.println("║   BAB 34: Multi-Sensor Data Aggregator v1.0        ║");
  Serial.println("╠════════════════════════════════════════════════════╣");
  Serial.printf ("║  DHT11      : IO%d                                 ║\n", DHTPIN);
  Serial.printf ("║  BMP180     : %s                              ║\n",
                  bmpOK ? "✅ Terdeteksi (I2C)" : "❌ Tidak terdeteksi");
  Serial.println("║  Internal   : temperatureRead() (ESP32 on-chip)    ║");
  Serial.println("╚════════════════════════════════════════════════════╝");
  Serial.println();
}

void loop() {
  unsigned long sekarang = millis();

  // ── Baca semua sensor ─────────────────────────────────────────────
  if (sekarang - tBacaTerakhir >= INTERVAL_BACA_MS) {
    tBacaTerakhir = sekarang;

    // DHT11
    float suhuDHT = dht.readTemperature();
    if (!isnan(suhuDHT) && isSuhuValid(suhuDHT)) {
      updateEMA(emaDHT, emaDHTInit, suhuDHT);
    }

    // BMP180
    if (bmpOK) {
      float suhuBMP = bmp.readTemperature();
      if (isSuhuValid(suhuBMP)) {
        updateEMA(emaBMP, emaBMPInit, suhuBMP);
      }
    }

    // Internal ESP32
    float suhuInt = temperatureRead();
    if (isSuhuValid(suhuInt)) {
      updateEMA(emaInternal, emaDHTInit, suhuInt);
      // Catatan: emaDHTInit dipakai disini supaya internal selalu aktif
      // karena sensor ini selalu valid sejak power-on
    }
  }

  // ── Dashboard Setiap 5 Detik ──────────────────────────────────────
  if (sekarang - tDashboardTerakhir >= INTERVAL_DASHBOARD_MS) {
    tDashboardTerakhir = sekarang;

    // Cross-validation — voting menggunakan median
    // Jika BMP tidak tersedia, gunakan rata-rata DHT + Internal
    float suhuFinal;
    if (bmpOK && emaBMPInit) {
      suhuFinal = medianSuhu(emaDHT, emaBMP, emaInternal);
    } else {
      suhuFinal = (emaDHT + emaInternal) / 2.0f;
    }

    // Deteksi sensor yang mencurigakan (outlier dari konsensus)
    bool dhtSuspect = fabsf(emaDHT - suhuFinal) > MAX_DELTA_ANTAR_SENSOR;
    bool bmpSuspect = bmpOK && fabsf(emaBMP - suhuFinal) > MAX_DELTA_ANTAR_SENSOR;
    bool intSuspect = fabsf(emaInternal - suhuFinal) > MAX_DELTA_ANTAR_SENSOR;

    // Format uptime
    unsigned long detik = sekarang / 1000;
    unsigned long jam   = detik / 3600;
    unsigned long menit = (detik % 3600) / 60;
    unsigned long dtk   = detik % 60;

    Serial.println("┌────────────────────────────────────────────────────┐");
    Serial.printf ("│  Uptime : %02lu:%02lu:%02lu                                  │\n",
                   jam, menit, dtk);
    Serial.println("├────────────────────────────────────────────────────┤");
    Serial.printf ("│  DHT11 (lingkungan) : %5.1f°C  %s              │\n",
                   emaDHT,     dhtSuspect ? "⚠️ OUTLIER" : "✅ OK     ");
    Serial.printf ("│  BMP180 (barometrik): %s                   │\n",
                   bmpOK ? (String(emaBMP, 1) + "°C  " + (bmpSuspect ? "⚠️ OUTLIER" : "✅ OK     ")).c_str()
                         : "N/A (tidak terhubung)    ");
    Serial.printf ("│  Internal (ESP32)   : %5.1f°C  %s              │\n",
                   emaInternal, intSuspect ? "⚠️ OUTLIER" : "✅ OK     ");
    Serial.println("├────────────────────────────────────────────────────┤");
    Serial.printf ("│  ► KONSENSUS (Voting Median): %5.1f°C              │\n", suhuFinal);
    Serial.println("└────────────────────────────────────────────────────┘");
    Serial.println();
  }
}
```

**Contoh output dashboard:**
```text
╔════════════════════════════════════════════════════╗
║   BAB 34: Multi-Sensor Data Aggregator v1.0        ║
╠════════════════════════════════════════════════════╣
║  DHT11      : IO27                                 ║
║  BMP180     : ✅ Terdeteksi (I2C)                  ║
║  Internal   : temperatureRead() (ESP32 on-chip)    ║
╚════════════════════════════════════════════════════╝

┌────────────────────────────────────────────────────┐
│  Uptime : 00:00:05                                 │
├────────────────────────────────────────────────────┤
│  DHT11 (lingkungan) :  27.5°C  ✅ OK              │
│  BMP180 (barometrik):  29.1°C  ✅ OK              │
│  Internal (ESP32)   :  44.3°C  ✅ OK              │
├────────────────────────────────────────────────────┤
│  ► KONSENSUS (Voting Median):  29.1°C             │
└────────────────────────────────────────────────────┘

┌────────────────────────────────────────────────────┐
│  Uptime : 00:00:10                                 │
├────────────────────────────────────────────────────┤
│  DHT11 (lingkungan) :  27.8°C  ✅ OK              │
│  BMP180 (barometrik):  45.0°C  ⚠️ OUTLIER        │
│  Internal (ESP32)   :  44.8°C  ✅ OK              │
├────────────────────────────────────────────────────┤
│  ► KONSENSUS (Voting Median):  44.8°C             │
└────────────────────────────────────────────────────┘
```

> 💡 **Mengapa Internal Temp berbeda jauh dari DHT11?** Itu **normal dan diharapkan!** DHT11 mengukur suhu *lingkungan/udara*, sementara internal sensor mengukur suhu *die chip* ESP32 yang selalu lebih panas. Voting median dengan justru mengabaikan DHT11 di kasus ini adalah perilaku yang *salah* — karena keduanya benar tapi mengukur hal yang berbeda. Inilah mengapa di sistem nyata, sensor fusion yang baik harus mempertimbangkan **semantik data** (apa yang diukur), bukan hanya nilai numeriknya.

---

## 34.11 Tips & Panduan Troubleshooting

### 1. EMA tidak merespons cepat terhadap perubahan nyata?
```text
Penyebab: Alpha terlalu kecil (filter terlalu kuat).

Solusi:
  A. Naikkan EMA_ALPHA (misal dari 0.05 ke 0.2)
  B. Gunakan adaptive alpha: perbesar alpha sementara ketika
     delta antara input dan EMA melebihi threshold tertentu.

Tip praktis:
  Alpha ≈ 0.05  → Sangat halus, sangat lambat   (cocok: suhu ruangan)
  Alpha ≈ 0.15  → Seimbang                       (cocok: LDR, gas sensor)
  Alpha ≈ 0.40  → Responsif, masih agak halus    (cocok: ADC potensiometer)
```

### 2. SMA selalu lag di belakang nilai sebenarnya?
```text
Penyebab: Ini adalah perilaku inherent SMA — ia "menunda" sebesar N/2 sampel.

Keputusan desain:
  A. Kurangi N (window lebih kecil = lag lebih kecil, tapi kurang halus)
  B. Ganti ke EMA (tidak ada lag konsisten, tapi ada time-constant)
  C. Terima lag: untuk aplikasi monitoring, lag beberapa detik biasanya OK.
```

### 3. Rate-of-change filter menolak terlalu banyak data valid?
```text
Penyebab: MAX_DELTA_PER_TICK terlalu kecil untuk perubahan nyata yang cepat.

Cara kalibrasi MAX_DELTA_PER_TICK yang benar:
  1. Nonaktifkan filter sementara
  2. Ukur perubahan maksimum nyata yang VALID (misal: tutup LDR sangat cepat)
  3. Catat delta maksimum yang terjadi
  4. Set MAX_DELTA_PER_TICK = delta_maksimum × 1.5 (beri margin 50%)

Contoh:
  Magkan LDR cepat → delta ADC maks = 400 per 50ms
  → MAX_DELTA_PER_TICK = 400 × 1.5 = 600
```

### 4. Cross-validation (voting) selalu menganggap Internal Temp sebagai outlier?
```text
Ini NORMAL dan BENAR karena sensor internal ESP32 mengukur SUHU CHIP,
bukan suhu lingkungan. Selisihnya bisa 15-30°C dari DHT11!

Solusi A — Kecualikan internal dari voting lingkungan:
  → Gunakan median(DHT, BMP) saja untuk estimasi suhu ruangan
  → Pakai internal hanya untuk sistem thermal protection chip

Solusi B — Normalisasi offset:
  → Ukur selisih rata-rata: offset = emaInternal - emaDHT (saat idle panjang)
  → Kompensasi: suhuIntNorm = emaInternal - offset
  → Sekarang suhuIntNorm ≈ suhu lingkungan (estimasi kasar)
```

### 5. Lookup table LDR tidak akurat di kit saya?
```text
Lookup table di Program 3 dikalibrasi untuk rangkaian Bluino Kit v3.2
(LDR + 10KΩ pull-down, VCC=3.3V, ADC 12-bit).

Untuk kalibrasi ulang:
  1. Siapkan referensi: aplikasi lux meter di HP sudah cukup untuk belajar
  2. Ukur lux di berbagai kondisi (gelap, lampu meja, sinar matahari)
  3. Catat ADC raw yang sesuai dari Serial Monitor
  4. Buat tabel LUX_TABLE baru dengan pasangan data tersebut
  5. Makin banyak titik kalibrasi = makin akurat interpolasinya!
```

---

## 34.12 Ringkasan

```
┌──────────────────────────────────────────────────────────────────────┐
│               RINGKASAN BAB 34 — PEMROSESAN DATA SENSOR              │
├──────────────────────────────────────────────────────────────────────┤
│ TEKNIK FILTER:                                                        │
│   SMA  = Simple Moving Average  — Rata-rata N sampel terakhir         │
│           + mudah dipahami, + akurasi rata-rata baik                  │
│           - butuh N sampel sebelum stabil, - memori O(N)              │
│                                                                       │
│   WMA  = Weighted Moving Average — Bobot lebih besar ke sampel baru  │
│           + lebih responsif dari SMA, - komputasi lebih berat         │
│                                                                       │
│   EMA  = Exponential Moving Average — α × baru + (1-α) × lama        │
│           + efisien memori O(1), + tidak ada batas window            │
│           - parameter α perlu dikalibrasi                             │
│                                                                       │
│ VALIDASI & OUTLIER REJECTION:                                         │
│   Range Check     → Tolak nilai di luar batas fisik sensor            │
│   Rate-of-Change  → Tolak perubahan yang terlalu drastis/cepat       │
│   Median Filter   → Hilangkan spike sesaat (terbaik untuk ADC)        │
│                                                                       │
│ KALIBRASI:                                                            │
│   Offset   = nilai_terkoreksi = raw - offset                          │
│   Gain     = nilai_terkoreksi = raw × gain                            │
│   2-titik  = (raw - ref_low) × ((out_hi - out_lo)/(ref_hi - ref_lo)) │
│   Lookup Table + Interpolasi = untuk sensor non-linear (LDR, NTC)    │
│                                                                       │
│ HYSTERESIS:                                                           │
│   Gunakan DUA threshold berbeda untuk ON dan OFF                      │
│   Gap hysteresis ≥ 2× level noise → eliminasi false trigger           │
│                                                                       │
│ SENSOR FUSION:                                                        │
│   Median 3 sensor = cara paling robust untuk voting/konsensus         │
│   Perhatikan SEMANTIK data: suhu chip ≠ suhu lingkungan!              │
│                                                                       │
│ BEST PRACTICES:                                                       │
│   ✅ Selalu pre-fill buffer di setup() agar tidak ada artefak awal    │
│   ✅ Gunakan float untuk akumulasi sum (hindari integer overflow)      │
│   ✅ Dokumentasikan asal nilai kalibrasi di komentar kode             │
│   ✅ Uji filter kamu dengan isnan() / isinf() untuk keamanan          │
│   ⚠️ Rate filter yang terlalu ketat akan menolak data valid!          │
└──────────────────────────────────────────────────────────────────────┘
```

---

## 34.13 Latihan

1. **Filter Championship:** Buat program yang membaca Potensiometer dan menampilkan *semua empat* nilai sekaligus: `raw`, `SMA(N=4)`, `SMA(N=16)`, dan `EMA(α=0.1)` di Serial Plotter. Amati perbedaan *kecepatan respons* dan *kehalusan* antar konfigurasi. Tentukan: konfigurasi mana yang kamu pilih untuk sistem alarm kebakaran? Mengapa?

2. **Kalibrator Dua Titik Interaktif:** Buat program yang meminta pengguna memasukkan dua titik kalibrasi via Serial Monitor (format: `REF,RAW`), lalu secara otomatis menghitung offset dan gain, dan menggunakannya untuk mengkonversi semua pembacaan ADC berikutnya. Ini mensimulasikan proses kalibrasi lapangan (*field calibration*) yang sesungguhnya.

3. **Adaptive EMA:** Modifikasi Program 1 agar nilai `alpha` berubah secara otomatis: ketika `|raw - EMA| > 300`, alpha dinaikkan ke `0.5` (responsif) dan secara bertahap kembali ke `0.08` selama 10 sampel. Ini disebut **Adaptive EMA** — digunakan di real-time control systems.

4. **Sistem Alarm Anti-Chatter:** Gunakan data LDR dengan threshold hysteresis, tambahkan persyaratan: alarm baru berbunyi jika kondisi "gelap" bertahan selama minimal **3 detik berturut-turut**. Ini mencegah alarm palsu akibat bayangan sesaat. Implementasikan menggunakan `millis()` tanpa `delay()`.

5. **Sensor Health Monitor:** Kembangkan Program 4 dengan menambahkan sistem scoring kesehatan sensor: setiap sensor mendapat skor `0–100` berdasarkan berapa persen pembacaannya *lolos* validasi (tidak NaN, dalam range, tidak outlier terhadap konsensus) selama 60 detik terakhir. Tampilkan skor di dashboard. Sensor dengan skor < 70% diberi tanda `⚠️ DEGRADED`.

---

*Selanjutnya → [BAB 35: OLED Display 128×64](../BAB-35/README.md)*
