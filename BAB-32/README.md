# BAB 32: LDR — Sensor Cahaya

> ✅ **Prasyarat:** Selesaikan **BAB 11 (Analog Input & ADC)** dan **BAB 16 (Non-Blocking Programming)**. LDR di Kit Bluino menggunakan pin **Custom** — hubungkan dengan kabel jumper ke GPIO pilihan kamu secara manual!

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Menjelaskan cara kerja LDR (*Light-Dependent Resistor*) dan prinsip rangkaian pembagi tegangan (*voltage divider*).
- Memahami kelebihan dan **keterbatasan kritis ADC ESP32** yang berdampak langsung pada hasil pembacaan sensor.
- Melakukan **kalibrasi manual** untuk mengubah nilai ADC mentah menjadi nilai lux perkiraan yang bermakna.
- Membangun sistem kontrol berbasis ambang batas (*threshold*) cahaya menggunakan **State Machine** yang benar-benar non-blocking.
- Menggunakan teknik **penghalusan data (moving average filter)** untuk menghilangkan noise ADC ESP32.
- Merancang sistem otomasi pencahayaan sederhana (lampu nyala otomatis saat gelap) yang siap dipasang di produk nyata.

---

## 32.1 Mengenal LDR — Mata Peka Cahaya

### Apa itu LDR?

**LDR** (*Light-Dependent Resistor*), juga dikenal sebagai **fotoresistor** atau **CdS cell**, adalah komponen pasif yang resistansinya **berubah secara dramatis tergantung intensitas cahaya** yang mengenainya.

```
PRINSIP KERJA LDR:

TERANG (banyak foton):          GELAP (sedikit foton):
  Foton mengenai material         Sedikit/tidak ada foton
  semikonduktor CdS               yang mengenai CdS
       │                               │
       ▼                               ▼
  Elektron tereksitasi ke         Elektron tetap di
  pita konduksi → hambatan        pita valensi → hambatan
  TURUN drastis!                  NAIK drastis!
       │                               │
       ▼                               ▼
  R ≈ 1 KΩ – 10 KΩ              R ≈ 100 KΩ – 10 MΩ
  (sangat konduktif)              (sangat resistif)

Bahan Aktif: Kadmium Sulfida (CdS) — semikonduktor fotokonduktif
Respon spektrum: paling sensitif pada 540nm (cahaya kuning-hijau)
= mendekati kurva sensitivitas mata manusia!
```

LDR adalah sensor **generasi pertama** yang paling sederhana dan terjangkau. Ia tidak mengukur lux secara akurat selayaknya sensor BH1750, tapi untuk proyek yang membutuhkan **deteksi TERANG vs GELAP** atau estimasi kasar intensitas cahaya, LDR lebih dari cukup!

### LDR vs Alternatif Modern

```
┌────────────────────────────────────────────────────────────────────┐
│           PERBANDINGAN SENSOR CAHAYA                               │
├─────────────────────────┬────────────────┬─────────────────────────┤
│ Fitur                   │ LDR (Kit kita) │ BH1750 / TEMT6000       │
├─────────────────────────┼────────────────┼─────────────────────────┤
│ Antarmuka               │ Analog (ADC)   │ I2C / Analog (linear)   │
│ Output                  │ Tegangan ADC   │ Nilai Lux langsung       │
│ Akurasi                 │ ±20–30%        │ ±5–10%                  │
│ Perlu Kalibrasi?        │ ✅ Ya, manual  │ ❌ Tidak (sudah digital) │
│ Linearitas Respons      │ Logaritmik     │ Linear                  │
│ Harga                   │ Sangat murah   │ Lebih mahal             │
│ Cocok untuk             │ On/Off, deteksi│ Pengukuran presisi lux  │
│                         │ siang/malam    │ untuk laporan ilmiah    │
│ Ada di Kit Bluino?      │ ✅ YA          │ ❌ Tidak                │
└─────────────────────────┴────────────────┴─────────────────────────┘
```

> 💡 **Perspektif Industri:** Dalam produk konsumer (lampu jalanan otomatis, kamera HP yang mengatur kecerahan layar, mainan anak sensitif cahaya), LDR masih digunakan secara luas hingga hari ini karena robustness dan harganya yang tidak tertandingi oleh sensor digital.

---

## 32.2 Rangkaian Voltage Divider — Mengubah Hambatan Jadi Tegangan

### Mengapa LDR Butuh Resistor Tetap?

LDR hanya mengubah **hambatan**. Tapi ESP32 tidak bisa membaca hambatan secara langsung — ia membaca **tegangan**. Solusinya adalah rangkaian pembagi tegangan (*voltage divider*):

```
RANGKAIAN VOLTAGE DIVIDER DI KIT BLUINO:

    3.3V
      │
      ├─── R_fix = 10KΩ (resistor tetap — sudah di PCB!)
      │
      ├─────────── → Pin ADC ESP32 (TITIK UKUR)
      │
      ├─── R_LDR (hambatan LDR — berubah sesuai cahaya)
      │
     GND

RUMUS TEGANGAN PADA PIN ADC (Hukum Kirchhoff):

    V_adc = 3.3V × R_LDR / (R_fix + R_LDR)

CONTOH KALKULASI:
  Kondisi Terang: R_LDR ≈ 2KΩ
    V_adc = 3.3 × 2K / (10K + 2K) = 3.3 × 0.167 = 0.55V
    → ADC akan membaca nilai RENDAH (gelap di grafik)

  Kondisi Gelap: R_LDR ≈ 100KΩ
    V_adc = 3.3 × 100K / (10K + 100K) = 3.3 × 0.909 = 3.0V
    → ADC akan membaca nilai TINGGI (terang di grafik)

⚠️ PERHATIKAN POLARITASNYA:
  TERANG → V_adc TURUN (R_LDR kecil → tegangan jatuh di R_fix besar)
  GELAP  → V_adc NAIK  (R_LDR besar → tegangan naik ke titik ukur)
```

> ⚠️ **Jebakan Paling Umum!** Banyak pemula mengasumsikan "ADC tinggi = terang". Di rangkaian Kit Bluino ini, **relasi justru terbalik** — ADC tinggi berarti **GELAP**, ADC rendah berarti **TERANG**. Kita akan kompensasi ini di dalam kode!

---

## 32.3 Perangkap Terbesar: Keterbatasan ADC ESP32

Sebelum menyentuh kode, **WAJIB** memahami cacat bawaan ADC ESP32 — ini adalah jebakan yang menghasilkan data tidak akurat pada ribuan proyek.

```
KURVA IDEAL vs REALITA ADC ESP32:

Tegangan (V)    ADC Ideal (12-bit)     ADC ESP32 (realita)
────────────    ──────────────────     ───────────────────
0.0V       →        0                      0 – 200 (tidak linear!)
0.1V       →      124                      ~0     (dead zone!)
0.3V       →      372                      ~100   (under-read)
1.0V       →     1241                     ~1100   (mendekati)
2.0V       →     2482                     ~2200   (mendekati)
3.1V       →     3847                     ~4095   (saturasi dini!)
3.3V       →     4095                     ~4095   (sudah saturasi)

Masalah utama ADC ESP32:
  A. "Dead Zone" di bawah 0.1V → always reads near 0
  B. "Saturation Zone" di atas 3.1V → always reads 4095
  C. Non-linearity signifikan di tengah rentang
  D. ADC2 tidak bisa digunakan saat WiFi aktif!
     → Selalu gunakan ADC1 (IO32–IO39) untuk proyek IoT

SOLUSI KHUSUS UNTUK ESP32:
  ✅ Gunakan pin ADC1 saja (IO32, IO33, IO34, IO35) — Wajib untuk stabilitas WiFi!
  ✅ Terapkan Moving Average filter untuk meredam fluktuasi/noise
  ✅ Gunakan kalibrasi manual batas terang/gelap (mengompensasi cacat linieritas)
```

> 💡 **Fungsi Kalibrasi Otomatis ESP32:** Framework Arduino ESP32 v2 dibekali fungsi `analogReadMilliVolts()` yang telah terkalibrasi pabrik. Di bab ini, **kita tetap memakai `analogRead()` (Raw ADC)** agar kamu menguasai logika *thresholding* universal yang bisa dipakai di *board* apapun (Arduino Uno, STM32, dll). Pemanggilan `analogReadMilliVolts()` tetap dilampirkan di Program 1 untuk bahan perbandingan eksperimenmu!

---

## 32.4 Spesifikasi Hardware & Wiring Kit Bluino

### Spesifikasi LDR + Rangkaian

```
┌─────────────────────────────────────────────────────────────────┐
│             SPESIFIKASI LDR — KIT BLUINO                        │
├──────────────────────────┬──────────────────────────────────────┤
│ Parameter                │ Nilai                                │
├──────────────────────────┼──────────────────────────────────────┤
│ Koneksi ke Kit           │ Pin CUSTOM (kabel jumper manual)     │
│ Tipe Output              │ Analog (Tegangan ADC)                │
│ Resistor Seri (Tetap)    │ 10KΩ (sudah terintegrasi di PCB)     │
│ Tegangan Referensi       │ 3.3V                                 │
│ Resolusi ADC             │ 12-bit (0 – 4095)                   │
│ Rentang Hambatan LDR     │ ~1KΩ (terang sekali) – ~10MΩ (gelap)│
│ Rentang Tegangan Output  │ ~0.03V (terang) – ~3.27V (gelap)   │
│ Respon Spektrum Puncak   │ ~540nm (cahaya kuning-hijau)        │
│ Waktu Respons            │ ~20ms (terang→gelap), ~5ms (gelap→  │
│                          │  terang) — tidak untuk freq tinggi!  │
└──────────────────────────┴──────────────────────────────────────┘
```

### Diagram Wiring

```
┌──────────────────────────────────────────────────────────┐
│           WIRING LDR ↔ ESP32 KIT BLUINO                  │
├──────────────────────────────────────────────────────────┤
│                                                          │
│   Kit Pin Header    KABEL JUMPER    ESP32 (rekomendasi)  │
│   ─────────────     ────────────    ──────────────────── │
│   LDR_OUT ──────── Kuning ───────── IO35 (ADC1, Input   │
│                                           Only — ideal!) │
│                                                          │
│   (VCC dan GND sudah terintegrasi dalam rangkaian        │
│    pembagi tegangan di PCB — tidak perlu disambung!)     │
│                                                          │
│   💡 Mengapa IO35?                                       │
│   • IO35 adalah bagian ADC1 (IO32–IO39) — aman dari     │
│     konflik WiFi (ADC2 tidak bisa dipakai saat WiFi!)   │
│   • IO35 adalah "Input Only" — tidak bisa output        │
│     sehingga tidak ada risiko konfigurasi salah         │
│   • Tidak ada pin strapping issue                       │
│                                                          │
│   ⚠️  JANGAN gunakan IO0, IO2, IO12, IO15 untuk LDR!   │
│   ⚠️  JANGAN gunakan IO25, IO26 (ADC2) jika WiFi aktif! │
└──────────────────────────────────────────────────────────┘
```

---

## 32.5 Instalasi Library

LDR **tidak memerlukan library eksternal**! Kita murni menggunakan fungsi ADC bawaan framework Arduino ESP32.

```
Tidak perlu menginstal library apapun!

Fungsi yang digunakan:
  √ analogRead()            — Membaca raw ADC 0–4095
  √ analogReadMilliVolts()  — Membaca millivolt dengan koreksi non-lin
  √ analogReadResolution()  — Set resolusi ADC (default 12-bit)
  √ analogSetAttenuation()  — Set atenuasi ADC (default ADC_11db = 0–3.3V)
  √ millis()                — Timing non-blocking
```

---

## 32.6 Program 1: Pembacaan ADC Dasar + Kalibrasi Manual

Program pertama ini adalah **alat kalibrasi** — kita gunakan untuk memahami respons LDR di lingkungan spesifik kamu sebelum membangun logika otomasi. Tidak ada dua ruangan yang sama!

```cpp
/*
 * BAB 32 - Program 1: LDR — Pembacaan ADC & Kalibrasi
 *
 * Program ini tidak melakukan kontrol apapun. Tujuannya SATU:
 * Memahami rentang nilai ADC LDR kamu di LINGKUNGAN SPESIFIK kamu.
 *
 * Cara kalibrasi:
 *   1. Upload program ini.
 *   2. Tutup LDR dengan telapak tangan sepenuhnya → catat nilai ADC "GELAP"
 *   3. Arahkan LDR ke sumber cahaya terang / lampu meja → catat nilai "TERANG"
 *   4. Nilai ini akan dipakai sebagai batas di Program 2, 3, dan 4!
 *
 * PENTING: Kalibrasi HARUS dilakukan di lokasi instalasi sensor sesungguhnya.
 *          Nilai akan berbeda antara sudut ruangan yang satu dengan yang lain!
 *
 * Wiring (Kit Bluino — Koneksi MANUAL via Kabel Jumper):
 *   LDR_OUT → IO35 (ADC1, Input Only — ideal untuk sensor analog pasif)
 */

const int PIN_LDR = 35; // IO35 — ADC1 Channel 7

void setup() {
  Serial.begin(115200);

  // Set resolusi ADC ke 12-bit (0–4095) — default ESP32
  analogReadResolution(12);

  // Set atenuasi ke 11dB → rentang tegangan input 0–3.3V
  // (mode paling umum untuk sensor dengan catu 3.3V)
  analogSetAttenuation(ADC_11db);

  Serial.println("════════════════════════════════════════════════");
  Serial.println("  BAB 32: LDR — Alat Kalibrasi ADC");
  Serial.println("════════════════════════════════════════════════");
  Serial.println("  Instruksi:");
  Serial.println("  1. Tutup LDR dengan telapak tangan (gelap total)");
  Serial.println("     → Catat nilai 'ADC Raw' dan 'mV' saat GELAP");
  Serial.println("  2. Sorot LDR dengan lampu (terang penuh)");
  Serial.println("     → Catat nilai 'ADC Raw' dan 'mV' saat TERANG");
  Serial.println("────────────────────────────────────────────────\n");
  Serial.println("  ADC Raw  │  Tegangan (mV)  │  % Cahaya (est)  │  Kondisi");
  Serial.println("───────────┼─────────────────┼──────────────────┼──────────");
}

void loop() {
  // Baca ADC dengan dua cara untuk perbandingan
  int    nilaiRaw  = analogRead(PIN_LDR);
  int    nilaiMv   = analogReadMilliVolts(PIN_LDR); // Sudah terkoreksi non-linearitas

  // Hitung estimasi persentase cahaya
  // INGAT: Di kit Bluino, ADC TINGGI = GELAP, ADC RENDAH = TERANG
  // Maka kita balik: (4095 - nilaiRaw) / 4095 * 100
  float persen = ((4095.0f - nilaiRaw) / 4095.0f) * 100.0f;

  // Tentukan label kondisi berdasarkan nilai (batas kasar awal)
  const char* kondisi;
  if (persen >= 80.0f)      kondisi = "SANGAT TERANG";
  else if (persen >= 50.0f) kondisi = "TERANG";
  else if (persen >= 25.0f) kondisi = "REDUP";
  else if (persen >= 10.0f) kondisi = "GELAP";
  else                       kondisi = "GELAP TOTAL";

  Serial.printf("  %4d     │  %5d mV       │  %5.1f%%           │  %s\n",
                nilaiRaw, nilaiMv, persen, kondisi);

  delay(500); // Update dua kali per detik saat kalibrasi
}
```

**Contoh output dan cara membacanya:**
```text
════════════════════════════════════════════════
  BAB 32: LDR — Alat Kalibrasi ADC
════════════════════════════════════════════════
  ADC Raw  │  Tegangan (mV)  │  % Cahaya (est)  │  Kondisi
───────────┼─────────────────┼──────────────────┼──────────
   3812     │   3050 mV       │    6.9%           │  GELAP TOTAL    ← tutup dengan tangan
   3756     │   3010 mV       │    8.3%           │  GELAP TOTAL
   3780     │   3025 mV       │    7.7%           │  GELAP TOTAL

   1205     │   970 mV        │   70.6%           │  TERANG         ← lampu meja normal
   1188     │   958 mV        │   71.0%           │  TERANG

    420     │   335 mV        │   89.7%           │  SANGAT TERANG  ← senter langsung
    405     │   325 mV        │   90.1%           │  SANGAT TERANG
```

**Catat hasil kalibrasimu di sini:**
```
┌─────────────────────────────────────────────────┐
│         CATATAN KALIBRASI — ISI SENDIRI         │
├──────────────────────┬──────────────────────────┤
│ Kondisi              │ ADC Raw (nilaiRaw)        │
├──────────────────────┼──────────────────────────┤
│ GELAP TOTAL (tutup)  │ _______________           │
│ Cahaya Ruang Normal  │ _______________           │
│ Terang Penuh (lampu) │ _______________           │
│                      │                          │
│ Batas GELAP > ___    │ Batas TERANG < ___        │
└──────────────────────┴──────────────────────────┘
```

---

## 32.7 Perangkap: Noise ADC & Cara Mengatasinya

Jika kamu membaca output Program 1 dengan seksama, kamu akan melihat nilai ADC berfluktuasi ±20–100 bit meski kondisi cahaya tidak berubah. Ini adalah **noise ADC yang normal** pada ESP32.

```
KONSEKUENSI NOISE TANPA FILTER:

  Nilai ADC  → 3000, 3010, 2995, 3008, 3003, 2998 ← berfluktuasi!

  Jika batas threshold = 3000:
    3000 → "GELAP"
    3010 → "TERANG"   ← berganti-ganti meski kondisi tidak berubah!
    2995 → "GELAP"
    3008 → "TERANG"   ← sistem akan "goyang" (chattering/bouncing)
    → Efek: lampu menyala-mati berkali-kali, alarm palsu, log penuh!

SOLUSI #1 — Moving Average Filter:
  ADC_smooth = (baca[0] + baca[1] + ... + baca[N-1]) / N
  → Dengan N=10 sampel, noise berkurang ~70%!

SOLUSI #2 — Hysteresis Threshold:
  Batas ON  (masuk GELAP): ADC > 3100 → nyalakan lampu
  Batas OFF (keluar GELAP): ADC < 2900 → matikan lampu
  → "Jarak aman" 200 bit mencegah chattering di sekitar batas!
```

Kedua teknik ini akan kita implementasikan bersama di Program 2!

---

## 32.8 Program 2: Pembacaan Stabil dengan Moving Average Filter

```cpp
/*
 * BAB 32 - Program 2: LDR — Moving Average Non-Blocking
 *
 * Program ini memperkenalkan teknik paling mendasar dalam pemrosesan
 * sinyal untuk mikrokontroler: Moving Average Filter (running average).
 *
 * Arsitektur:
 *   - Array buffer melingkar (circular buffer) menyimpan N sampel terakhir
 *   - Rata-rata N sampel dihitung ulang setiap ada sampel baru
 *   - Hasil rata-rata jauh lebih stabil dari pembacaan tunggal
 *   - Tidak ada delay() — menggunakan millis() interval
 */

const int PIN_LDR = 35;

// ── Moving Average Filter ──────────────────────────────────────────
// Sesuaikan N_SAMPLES: semakin besar, semakin halus tapi semakin lambat
// respons terhadap perubahan cahaya yang cepat.
const int N_SAMPLES = 16;

int    bufferADC[N_SAMPLES]; // Circular buffer sampel ADC
int    idxBuffer  = 0;       // Index tulis buffer berikutnya
long   sumBuffer  = 0;       // Jumlah running untuk efisiensi kalkulasi

// ── Timing non-blocking ────────────────────────────────────────────
const unsigned long INTERVAL_BACA_MS   = 20UL;  // Baca ADC setiap 20ms (50Hz)
const unsigned long INTERVAL_CETAK_MS  = 500UL; // Tampilkan ke serial 2x/detik
unsigned long tBacaTerakhir  = 0;
unsigned long tCetakTerakhir = 0;

// ── Variabel hasil ─────────────────────────────────────────────────
int   adcRataRata = 0;
float persenCahaya = 0.0f;

// ── Fungsi tambah sampel ADC ke buffer melingkar ───────────────────
void tambahSampelADC(int sampelBaru) {
  // Kurangi nilai lama yang akan ditimpa dari jumlah running
  sumBuffer -= bufferADC[idxBuffer];
  // Timpa slot dengan sampel baru
  bufferADC[idxBuffer] = sampelBaru;
  // Tambahkan sampel baru ke jumlah running
  sumBuffer += sampelBaru;
  // Maju ke slot berikutnya (melingkar)
  idxBuffer = (idxBuffer + 1) % N_SAMPLES;
  
  // Karena buffer sudah di-pre-fill di awal, selalu bagi dengan N_SAMPLES
  adcRataRata = (int)(sumBuffer / N_SAMPLES);
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  // Inisialisasi buffer dengan nilai pertama agar tidak ada spike awal
  int nilaiAwal = analogRead(PIN_LDR);
  for (int i = 0; i < N_SAMPLES; i++) {
    bufferADC[i] = nilaiAwal;
  }
  sumBuffer = (long)nilaiAwal * N_SAMPLES;
  adcRataRata = nilaiAwal;

  Serial.println("═══════════════════════════════════════════════════════");
  Serial.println("  BAB 32: LDR — Moving Average Filter (Non-Blocking)");
  Serial.println("═══════════════════════════════════════════════════════");
  Serial.println("  [Waktu ms]  │ ADC Raw │ ADC Smooth │ mV Smooth │ Cahaya%");
  Serial.println("──────────────┼─────────┼────────────┼───────────┼────────");
}

void loop() {
  unsigned long sekarang = millis();

  // ── SAMPLING: Baca ADC setiap 20ms ──────────────────────────────
  if (sekarang - tBacaTerakhir >= INTERVAL_BACA_MS) {
    tBacaTerakhir = sekarang;
    int sampelBaru = analogRead(PIN_LDR);
    tambahSampelADC(sampelBaru);

    // Hitung persen dari nilai rata-rata (polaritas dibalik!)
    persenCahaya = ((4095.0f - adcRataRata) / 4095.0f) * 100.0f;
  }

  // ── PRINT: Tampilkan ke Serial setiap 500ms ──────────────────────
  if (sekarang - tCetakTerakhir >= INTERVAL_CETAK_MS) {
    tCetakTerakhir = sekarang;

    int  adcRaw  = analogRead(PIN_LDR);
    int  mvSmooth = (int)((adcRataRata / 4095.0f) * 3300.0f);

    Serial.printf("  [%8lu ms] │  %4d   │    %4d    │  %4d mV   │  %5.1f%%\n",
                  sekarang, adcRaw, adcRataRata, mvSmooth, persenCahaya);
  }
}
```

**Contoh output — perhatikan ADC Smooth lebih stabil dari ADC Raw:**
```text
  [Waktu ms]  │ ADC Raw │ ADC Smooth │ mV Smooth │ Cahaya%
──────────────┼─────────┼────────────┼───────────┼────────
  [   501 ms] │  1205   │    1212    │   976 mV  │  70.4%
  [  1001 ms] │  1198   │    1208    │   973 mV  │  70.5%   ← smooth stabil
  [  1501 ms] │  1215   │    1211    │   976 mV  │  70.4%
  [  2001 ms] │  1187   │    1209    │   974 mV  │  70.5%   ← raw berfluktuasi
  [  2501 ms] │  1220   │    1210    │   975 mV  │  70.5%     smooth tidak!
```

---

## 32.9 Program 3: Sistem Lampu Otomatis dengan State Machine & Hysteresis

Program inilah sistem siap-pakai. Kita menggabungkan semua teknik: **Moving Average**, **Hysteresis**, dan **State Machine** untuk membangun otomasi lampu yang tidak goyang-goyang.

Sistem ini memiliki 3 state:
- `SIANG` — Cahaya cukup, lampu OFF, pantau terus
- `MALAM` — Cahaya kurang, lampu ON, pantau terus
- `TRANSISI` — Jeda waktu saat nilai ADC berada di zona abu-abu (antara dua batas hysteresis)

```cpp
/*
 * BAB 32 - Program 3: Sistem Lampu Otomatis — State Machine + Hysteresis
 *
 * Menggabungkan teknik:
 *   ① Moving Average Filter     → Data ADC yang stabil dan noise-free
 *   ② Hysteresis Threshold      → Mencegah chattering di batas transisi
 *   ③ Finite State Machine      → Logika keadaan yang bersih & terstruktur
 *   ④ Non-Blocking sepenuhnya   → Tidak ada delay() sama sekali!
 *
 * ⚙️  SESUAIKAN KONSTANTA INI DENGAN NILAI KALIBRASI KAMU (dari Program 1)!
 *
 * Konfigurasi Pin:
 *   LDR OUT  → IO35
 *   LED/Lampu → IO2 (LED bawaan kit, atau ganti dengan pin LED eksternal)
 */

// ── Konfigurasi Pin ───────────────────────────────────────────────
const int PIN_LDR  = 35;
const int PIN_LAMP = 2;  // LED bawaan sebagai simulasi lampu

// ── Batas Kalibrasi (SESUAIKAN DENGAN PROGRAM 1!) ─────────────────
// Ingat: Nilai ADC TINGGI = GELAP, ADC RENDAH = TERANG
//
// Batas ON  (nyalakan lampu): ADC melebihi nilai ini → sudah gelap
const int BATAS_NYALA    = 2800; // Sesuaikan! ← ADC raw saat kondisi "mulai gelap"
// Batas OFF (matikan lampu): ADC turun di bawah nilai ini → sudah terang lagi
// Hysteresis gap = BATAS_NYALA - BATAS_MATI = 400 bit (menghindari chattering)
const int BATAS_MATI     = 2400; // Sesuaikan! ← ADC raw saat kondisi "sudah terang"

// ── Moving Average ───────────────────────────────────────────────
const int N_SAMPLES      = 16;
int    bufferADC[N_SAMPLES];
int    idxBuffer         = 0;
long   sumBuffer         = 0;
int    adcSmooth         = 0;

void tambahSampelADC(int sampel) {
  sumBuffer -= bufferADC[idxBuffer];
  bufferADC[idxBuffer] = sampel;
  sumBuffer += sampel;
  idxBuffer = (idxBuffer + 1) % N_SAMPLES;
  adcSmooth = (int)(sumBuffer / N_SAMPLES);
}

// ── State Machine ─────────────────────────────────────────────────
enum StateCahaya { SIANG, MALAM };
StateCahaya stateSekarang = SIANG;

// ── Timing ───────────────────────────────────────────────────────
const unsigned long INTERVAL_BACA_MS  = 20UL;
const unsigned long INTERVAL_LOG_MS   = 2000UL; // Log ke serial tiap 2 detik
unsigned long tBacaTerakhir = 0;
unsigned long tLogTerakhir  = 0;

// ── Statistik ────────────────────────────────────────────────────
unsigned long tMasukMalam = 0; // Waktu masuk ke state MALAM
int    totalTransisiMalam = 0; // Berapa kali lampu dinyalakan

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);

  pinMode(PIN_LAMP, OUTPUT);
  digitalWrite(PIN_LAMP, LOW);

  // Inisialisasi buffer dengan pembacaan awal
  int nilaiAwal = analogRead(PIN_LDR);
  for (int i = 0; i < N_SAMPLES; i++) bufferADC[i] = nilaiAwal;
  sumBuffer   = (long)nilaiAwal * N_SAMPLES;
  adcSmooth   = nilaiAwal;

  // Tentukan state awal berdasarkan kondisi cahaya saat ini
  if (adcSmooth > BATAS_NYALA) {
    stateSekarang = MALAM;
    tMasukMalam   = millis(); // <- Inisialisasi esensial agar perhitungan durasi awal tidak ngawur!
    digitalWrite(PIN_LAMP, HIGH);
    Serial.println("  [Boot] Kondisi gelap terdeteksi! Lampu dinyalakan.");
  } else {
    stateSekarang = SIANG;
    Serial.println("  [Boot] Kondisi terang. Lampu OFF.");
  }

  Serial.println("═══════════════════════════════════════════════════════════");
  Serial.println("  BAB 32: Sistem Lampu Otomatis — State Machine");
  Serial.println("═══════════════════════════════════════════════════════════");
  Serial.printf ("  Batas Nyala : ADC > %d (gelap terdeteksi)\n", BATAS_NYALA);
  Serial.printf ("  Batas Mati  : ADC < %d (terang terdeteksi)\n", BATAS_MATI);
  Serial.printf ("  Hysteresis  : %d bit (zona aman anti-chattering)\n",
                  BATAS_NYALA - BATAS_MATI);
  Serial.println("───────────────────────────────────────────────────────────");
}

void loop() {
  unsigned long sekarang = millis();

  // ── SAMPLING ADC setiap 20ms ──────────────────────────────────
  if (sekarang - tBacaTerakhir >= INTERVAL_BACA_MS) {
    tBacaTerakhir = sekarang;
    tambahSampelADC(analogRead(PIN_LDR));
  }

  // ── STATE MACHINE ─────────────────────────────────────────────
  switch (stateSekarang) {

    case SIANG:
      // Lampu OFF. Pantau apakah sudah gelap
      if (adcSmooth > BATAS_NYALA) {
        // Transisi ke MALAM
        stateSekarang = MALAM;
        tMasukMalam   = sekarang;
        totalTransisiMalam++;
        digitalWrite(PIN_LAMP, HIGH);
        float persen = ((4095.0f - adcSmooth) / 4095.0f) * 100.0f;
        Serial.printf("[%8lu ms] 🌙 MALAM #%d — Lampu ON! ADC=%d (%.1f%% terang)\n",
                      sekarang, totalTransisiMalam, adcSmooth, persen);
      }
      break;

    case MALAM:
      // Lampu ON. Pantau apakah sudah cukup terang
      if (adcSmooth < BATAS_MATI) {
        // Transisi ke SIANG
        unsigned long durasiMalam = sekarang - tMasukMalam;
        stateSekarang = SIANG;
        digitalWrite(PIN_LAMP, LOW);
        float persen = ((4095.0f - adcSmooth) / 4095.0f) * 100.0f;
        Serial.printf("[%8lu ms] ☀️  SIANG    — Lampu OFF. ADC=%d (%.1f%% terang). "
                      "Durasi nyala: %lu detik\n",
                      sekarang, adcSmooth, persen, durasiMalam / 1000);
      }
      break;
  }

  // ── LOG PERIODIK setiap 2 detik ───────────────────────────────
  if (sekarang - tLogTerakhir >= INTERVAL_LOG_MS) {
    tLogTerakhir = sekarang;
    float   persen = ((4095.0f - adcSmooth) / 4095.0f) * 100.0f;
    int     mv     = (int)((adcSmooth / 4095.0f) * 3300.0f);
    const char* state_str = (stateSekarang == SIANG) ? "SIANG/Lampu OFF" : "MALAM/Lampu ON ";
    Serial.printf("  [%8lu ms] Monitoring │ ADC=%4d │ %4dmV │ %5.1f%% │ %s\n",
                  sekarang, adcSmooth, mv, persen, state_str);
  }
}
```

**Contoh output lengkap:**
```text
═══════════════════════════════════════════════════════════
  BAB 32: Sistem Lampu Otomatis — State Machine
═══════════════════════════════════════════════════════════
  Batas Nyala : ADC > 2800 (gelap terdeteksi)
  Batas Mati  : ADC < 2400 (terang terdeteksi)
  Hysteresis  : 400 bit (zona aman anti-chattering)
  [Boot] Kondisi terang. Lampu OFF.
───────────────────────────────────────────────────────────
  [    2001 ms] Monitoring │ ADC=1215 │  979mV │  70.3% │ SIANG/Lampu OFF
  [    4001 ms] Monitoring │ ADC=1208 │  973mV │  70.5% │ SIANG/Lampu OFF

  ← (pengguna menutup LDR dengan tangan atau ruangan jadi gelap)

[   5320 ms] 🌙 MALAM #1 — Lampu ON! ADC=2845 (30.5% terang)
  [    6001 ms] Monitoring │ ADC=3102 │ 2499mV │  24.3% │ MALAM/Lampu ON
  [    8001 ms] Monitoring │ ADC=3789 │ 3053mV │   7.5% │ MALAM/Lampu ON

  ← (pengguna membuka tangan / cahaya kembali)

[  10445 ms] ☀️  SIANG    — Lampu OFF. ADC=2210 (46.0% terang). Durasi nyala: 5 detik
  [   12001 ms] Monitoring │ ADC=1198 │  965mV │  70.7% │ SIANG/Lampu OFF
```

---

## 32.10 Program 4: Monitoring Multi-Level dengan Log Deteksi Perubahan

Program terakhir memperkenalkan sistem yang lebih canggih: **klasifikasi 5 level cahaya** dengan logging event perubahan secara real-time — berguna untuk *data logging* kondisi lingkungan.

```cpp
/*
 * BAB 32 - Program 4: LDR — Klasifikasi Multi-Level & Event Logging
 *
 * Menambahkan dimensi analitis di atas Program 3:
 *   - 5 level klasifikasi cahaya (bukan hanya siang/malam)
 *   - Deteksi PERUBAHAN LEVEL (hanya log saat ada perubahan nyata)
 *   - Histeresis antar-level untuk stabilitas
 *   - Statistik durasi per-level sejak boot
 *
 * Berguna untuk: Audit konsumsi cahaya ruangan, monitoring greenhouse,
 *               sistem smart curtain, penelitian intensitas cahaya.
 */

const int PIN_LDR  = 35;
const int PIN_LAMP = 2;

// ── Definisi 5 Level Cahaya ────────────────────────────────────────
// Batas ditentukan dalam nilai ADC Smooth (ingat: tinggi = gelap)
// SESUAIKAN dengan kalibrasi kamu!
enum LevelCahaya {
  CAHAYA_5_SANGAT_TERANG, // ADC < 600   → > 85% terang
  CAHAYA_4_TERANG,        // ADC 600-1500 → 63–85% terang
  CAHAYA_3_NORMAL,        // ADC 1500-2500 → 39–63% terang
  CAHAYA_2_REDUP,         // ADC 2500-3400 → 17–39% terang
  CAHAYA_1_GELAP          // ADC > 3400  → < 17% terang
};

const char* namaLevel[] = {
  "SANGAT TERANG", "TERANG", "NORMAL", "REDUP", "GELAP"
};

// ── Batas Hysteresis Multi-Level ─────────────────────────────────
// Batas Terang: ADC harus turun di bawah ini untuk CAHAYA LEVEL NAIK
const int BATAS_TERANG_ADC[] = {
  3300, // Dari GELAP (1) ke REDUP (2)
  2400, // Dari REDUP (2) ke NORMAL (3)
  1400, // Dari NORMAL (3) ke TERANG (4)
   500  // Dari TERANG (4) ke SANGAT_TERANG (5)
};

// Batas Gelap: ADC harus naik di atas ini untuk CAHAYA LEVEL TURUN
const int BATAS_GELAP_ADC[] = {
  3500, // Dari REDUP (2) ke GELAP (1)
  2600, // Dari NORMAL (3) ke REDUP (2)
  1600, // Dari TERANG (4) ke NORMAL (3)
   700  // Dari SANGAT_TERANG (5) ke TERANG (4)
};

// ── Moving Average ───────────────────────────────────────────────
const int N_SAMPLES  = 16;
int    bufferADC[N_SAMPLES];
int    idxBuf        = 0;
long   sumBuf        = 0;
int    adcSmooth     = 0;

void tambahSampelADC(int s) {
  sumBuf -= bufferADC[idxBuf];
  bufferADC[idxBuf] = s;
  sumBuf += s;
  idxBuf = (idxBuf + 1) % N_SAMPLES;
  adcSmooth = (int)(sumBuf / N_SAMPLES);
}

// ── State ─────────────────────────────────────────────────────────
LevelCahaya levelSekarang = CAHAYA_3_NORMAL;
unsigned long durasiLevel[5] = {0}; // Akumulasi durasi per level
unsigned long tMasukSekarang = 0;

// ── Timing ───────────────────────────────────────────────────────
const unsigned long INTERVAL_BACA_MS = 20UL;
const unsigned long INTERVAL_LOG_MS  = 3000UL;
unsigned long tBacaTerakhir = 0;
unsigned long tLogTerakhir  = 0;

// ── Fungsi: Tentukan level dari nilai ADC ─────────────────────────
LevelCahaya tentukanLevel(int adc, LevelCahaya levelLama) {
  // Menggunakan hysteresis: batas berbeda untuk naik vs turun
  // Ini mencegah osilasi bolak-balik di dekat batas!
  switch (levelLama) {
    case CAHAYA_1_GELAP:
      if (adc < BATAS_TERANG_ADC[0]) return CAHAYA_2_REDUP;
      return CAHAYA_1_GELAP;
    case CAHAYA_2_REDUP:
      if (adc > BATAS_GELAP_ADC[0])  return CAHAYA_1_GELAP;
      if (adc < BATAS_TERANG_ADC[1]) return CAHAYA_3_NORMAL;
      return CAHAYA_2_REDUP;
    case CAHAYA_3_NORMAL:
      if (adc > BATAS_GELAP_ADC[1])  return CAHAYA_2_REDUP;
      if (adc < BATAS_TERANG_ADC[2]) return CAHAYA_4_TERANG;
      return CAHAYA_3_NORMAL;
    case CAHAYA_4_TERANG:
      if (adc > BATAS_GELAP_ADC[2])  return CAHAYA_3_NORMAL;
      if (adc < BATAS_TERANG_ADC[3]) return CAHAYA_5_SANGAT_TERANG;
      return CAHAYA_4_TERANG;
    case CAHAYA_5_SANGAT_TERANG:
      if (adc > BATAS_GELAP_ADC[3])  return CAHAYA_4_TERANG;
      return CAHAYA_5_SANGAT_TERANG;
  }
  return levelLama;
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);
  analogSetAttenuation(ADC_11db);
  pinMode(PIN_LAMP, OUTPUT);
  digitalWrite(PIN_LAMP, LOW);

  int nilaiAwal = analogRead(PIN_LDR);
  for (int i = 0; i < N_SAMPLES; i++) bufferADC[i] = nilaiAwal;
  sumBuf      = (long)nilaiAwal * N_SAMPLES;
  adcSmooth   = nilaiAwal;

  // Tentukan level awal tanpa hysteresis (boot pertama kali)
  if      (adcSmooth < 500)  levelSekarang = CAHAYA_5_SANGAT_TERANG;
  else if (adcSmooth < 1500) levelSekarang = CAHAYA_4_TERANG;
  else if (adcSmooth < 2500) levelSekarang = CAHAYA_3_NORMAL;
  else if (adcSmooth < 3400) levelSekarang = CAHAYA_2_REDUP;
  else                       levelSekarang = CAHAYA_1_GELAP;

  tMasukSekarang = millis();
  // Lampu ON jika level gelap atau redup
  bool lampuOn = (levelSekarang <= CAHAYA_2_REDUP);
  digitalWrite(PIN_LAMP, lampuOn ? HIGH : LOW);

  Serial.println("════════════════════════════════════════════════════════════");
  Serial.println("  BAB 32: LDR — Multi-Level Monitoring & Event Log");
  Serial.println("════════════════════════════════════════════════════════════");
  Serial.printf ("  [Boot] Level awal: %s (ADC=%d) | Lampu: %s\n",
                  namaLevel[levelSekarang], adcSmooth, lampuOn ? "ON" : "OFF");
  Serial.println("────────────────────────────────────────────────────────────");
}

void loop() {
  unsigned long sekarang = millis();

  // ── ADC Sampling ──────────────────────────────────────────────
  if (sekarang - tBacaTerakhir >= INTERVAL_BACA_MS) {
    tBacaTerakhir = sekarang;
    tambahSampelADC(analogRead(PIN_LDR));
  }

  // ── Deteksi Perubahan Level ───────────────────────────────────
  LevelCahaya levelBaru = tentukanLevel(adcSmooth, levelSekarang);
  if (levelBaru != levelSekarang) {
    // Akumulasi durasi level sebelumnya
    durasiLevel[levelSekarang] += (sekarang - tMasukSekarang);
    tMasukSekarang = sekarang;
    levelSekarang  = levelBaru;

    bool lampuOn = (levelSekarang <= CAHAYA_2_REDUP);
    digitalWrite(PIN_LAMP, lampuOn ? HIGH : LOW);

    float persen = ((4095.0f - adcSmooth) / 4095.0f) * 100.0f;
    Serial.printf("[%8lu ms] ⚡ LEVEL BERUBAH → %-14s │ ADC=%4d │ %5.1f%% │ Lampu:%s\n",
                  sekarang, namaLevel[levelSekarang], adcSmooth, persen,
                  lampuOn ? "ON " : "OFF");
  }

  // ── Log Periodik ──────────────────────────────────────────────
  if (sekarang - tLogTerakhir >= INTERVAL_LOG_MS) {
    tLogTerakhir = sekarang;
    float persen = ((4095.0f - adcSmooth) / 4095.0f) * 100.0f;
    Serial.printf("  [%8lu ms] Status │ %-14s │ ADC=%4d │ %5.1f%%\n",
                  sekarang, namaLevel[levelSekarang], adcSmooth, persen);
  }
}
```

**Contoh output — Event monitoring mencatat momen transisi cahaya secara spesifik:**
```text
════════════════════════════════════════════════════════════
  BAB 32: LDR — Multi-Level Monitoring & Event Log
════════════════════════════════════════════════════════════
  [Boot] Level awal: NORMAL (ADC=1820) | Lampu: OFF
────────────────────────────────────────────────────────────
  [    3001 ms] Status │ NORMAL         │ ADC=1815 │  55.7%
  [    6001 ms] Status │ NORMAL         │ ADC=1822 │  55.5%

  ← (Menjelang sore, ruangan mulai meredup perlahan)

[   8450 ms] ⚡ LEVEL BERUBAH → REDUP          │ ADC=2610 │  36.3% │ Lampu:ON 
  [    9001 ms] Status │ REDUP          │ ADC=2850 │  30.4%

  ← (Ada yang mematikan lampu ruangan utama)

[  10200 ms] ⚡ LEVEL BERUBAH → GELAP          │ ADC=3650 │  10.9% │ Lampu:ON 
  [   12001 ms] Status │ GELAP          │ ADC=3701 │   9.6%
```

---

## 32.11 Tips & Panduan Troubleshooting

### 1. Nilai ADC Selalu 0 atau Selalu 4095?
```text
ADC selalu 0 (atau sangat rendah):
  A. Koneksi LDR_OUT ke GND secara tidak sengaja
     → Cek kabel jumper. Gunakan multimeter untuk cek tegangan pin.
  B. Menggunakan ADC2 pin (IO25–IO27) saat WiFi aktif
     → Ganti ke ADC1 (IO32–IO39)! ADC2 terblokir saat WiFi menyala.
  C. LDR diletakkan di tempat dengan cahaya SANGAT terang sekali
     → Hambatan LDR sangat kecil → tegangan output sangat rendah.

ADC selalu 4095:
  A. Koneksi putus (floating pin) — pin ADC tidak terhubung ke apapun
     → Pin floating ESP32 biasanya terbaca tinggi karena coupling kapasitif.
     → Cek sambungan kabel jumper dengan teliti.
  B. LDR diletakkan di tempat yang sangat gelap total
     → Hambatan LDR sangat besar → tegangan dekat 3.3V → ADC = 4095. Ini NORMAL!
```

### 2. Nilai ADC Berfluktuasi Parah (>200 bit)?
```text
Penyebab dan solusi:

A. Sumber daya 3.3V tidak stabil:
   → ESP32 yang melakukan transmisi WiFi menyebabkan tegangan 3.3V
     turun sebentar → ADC ikut fluktuasi parah!
   → Solusi: tambah kapasitor elektrolit 100µF antara 3.3V dan GND
     dekat rangkaian LDR untuk filtering tegangan.

B. Kabel jumper terlalu panjang:
   → Bertindak seperti antena yang menangkap interferensi 50Hz dari
     jala-jala listrik atau sinyal WiFi ESP32 sendiri!
   → Gunakan kabel jumper pendek (<15cm).

C. ADC1 channel yang sama dipakai oleh kode lain:
   → Pastikan tidak ada kode lain yang menginisialisasi pin yang sama
     dengan konfigurasi berbeda.
```

### 3. Sistem Chattering (Lampu Nyala-Mati Terus)?
```text
Ini terjadi jika nilai ADC berada TEPAT di batas threshold:
  ADC → 2805, 2795, 2801, 2798, 2803 ← terus berbolak-balik sekitar 2800!

Dua solusi (sudah diimplementasikan di Program 3 & 4):
A. Hysteresis:
   → Pisahkan batas NYALA (lebih tinggi) dan batas MATI (lebih rendah)
   → Gap minimal 200–400 bit agar noise tidak menyeberangi keduanya.

B. Moving Average Filter:
   → Dengan N=16 dan interval 20ms, waktu respons total = 320ms.
   → Noise singkat < 300ms TIDAK akan mengubah state.
   → Ini adalah "debouncing" alami via temporal filtering!
```

### 4. Nilai ADC Berbeda Padahal Kondisi yang Sama?
```text
ADC ESP32 dikenal memiliki OFFSET yang berbeda antar chip dan bahkan
antar pin ADC pada chip yang sama. Fenomena ini disebut:
  "ADC offset error" dan "ADC gain error"

Dampak nyata:
  ESP32 unit A → ADC terang = 1200
  ESP32 unit B → ADC terang = 1380 (beda 15% meski kondisi identik!)

Solusi produksi (kalau kamu mau serius):
  → Gunakan analogReadMilliVolts() -- sudah ada koreksi dari eFuse chip
  → Lakukan kalibrasi titik pengukuran per-unit sebelum pemasangan
  → Atau ganti ke sensor cahaya digital (BH1750, MAX44009) yang tidak
    bergantung pada kualitas ADC mikrokontroler.
```

### 5. Bagaimana Mengubah Nilai ADC ke Lux yang Akurat?
```text
Konversi LDR→Lux TIDAK bisa akurat tanpa kalibrasi terhadap luxmeter!

Pendekatan yang realistis untuk proyek edukasi:

A. Pendekatan Relatif (yang kita pakai):
   → Ukur ADC di kondisi referensi yang kamu tahu (gelap, terang lampu,
     terang matahari langsung)
   → Buat peta: "ADC=1200 ≈ cahaya ruang normal (≈300 lux perkiraan)"
   → Akurasi ≈ ±50% — cukup untuk on/off control!

B. Pendekatan Empiris (lebih serius):
   → Ukur bersamaan dengan luxmeter murah (tersedia <100rb di marketplace)
   → Plot kurva ADC vs Lux → fit ke persamaan eksponensial
   → Lux ≈ a × exp(b × ADC) atau Lux ≈ a × pow(ADC, b)
   → Akurasi ≈ ±15% — cukup untuk monitoring semi-serius.
```

---

## 32.12 Ringkasan

```
┌─────────────────────────────────────────────────────────────────────┐
│                RINGKASAN BAB 32 — ESP32 + LDR                       │
├─────────────────────────────────────────────────────────────────────┤
│ Wiring       = LDR_OUT→IO35 (ADC1, Input Only)                      │
│ Library      = Tidak diperlukan (murni analogRead built-in)         │
│ Tipe Output  = Analog — nilai 0 hingga 4095 (12-bit)                │
│                                                                     │
│ INGAT POLARITAS:                                                    │
│   ADC TINGGI (>3000) = GELAP (R_LDR besar, tegangan tinggi)        │
│   ADC RENDAH (<1000) = TERANG (R_LDR kecil, tegangan rendah)       │
│                                                                     │
│ PERINGATAN KRITIS ADC ESP32:                                        │
│   ⚠️  Gunakan ADC1 saja (IO32–IO39) — ADC2 konflik dengan WiFi!    │
│   ⚠️  ADC tidak linear (Gunakan strategi kalibrasi batas manual!)   │
│   ⚠️  Selalu terapkan Moving Average untuk data yang stabil!        │
│   ⚠️  Gunakan Hysteresis — jangan satu batas untuk on DAN off!      │
│                                                                     │
│ Teknik yang Digunakan:                                              │
│   Program 1: Kalibrasi ADC — pahami respons sensor di lokasi kamu   │
│   Program 2: Moving Average Filter — data stabil tanpa noise        │
│   Program 3: State Machine + Hysteresis — otomasi lampu produksi    │
│   Program 4: Multi-Level + Event Log — monitoring lingkungan detail  │
│                                                                     │
│ Kecepatan Sampling Optimal:                                         │
│   ADC LDR: Cukup 20–50ms interval (sensor lambat, ~20ms respons)   │
│   Moving Average: N=16, interval 20ms → smoothing window = 320ms   │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 32.13 Latihan

1. **Dimmer Otomatis LED PWM:** Baca nilai ADC LDR dan gunakan hasilnya untuk mengatur kecerahan LED via PWM (`ledcWrite()`). Saat ruangan gelap → LED terang penuh. Saat ruangan terang → LED redup/mati. Ini adalah *ambient light compensation* yang digunakan pada layar handphone! *(Petunjuk: Gunakan `map()` untuk mengkonversi rentang ADC ke rentang PWM 0–255).*

2. **Data Logger Cahaya ke Serial Plotter:** Modifikasi Program 2 agar outputnya kompatibel dengan **Serial Plotter** Arduino IDE (format: `nilai1,nilai2\n`). Plot bersama ADC Raw dan ADC Smooth pada grafik yang sama. Amati seberapa jauh moving average menghaluskan noise. Coba ubah nilai `N_SAMPLES` dari 4, 8, 16, 32 dan amati perbedaannya!

3. **Penghemat Energi Pintar:** Buat sistem dengan LDR + LED kuning. Bila cahaya ambient >70% (terang), LED mati total. Bila 40–70% (normal), LED menyala 30% (PWM duty cycle rendah). Bila <40% (redup/gelap), LED menyala penuh. Implementasikan dengan State Machine 3-state yang bersih tanpa satu pun `delay()`!

4. **Sensor Fusion LDR + PIR:** Gabungkan BAB 31 (PIR AM312) dan BAB 32 (LDR) dalam satu sistem. Logika: Lampu dinyalakan **hanya jika** kondisi **GELAP** (dari LDR) **DAN** ada **GERAKAN** (dari PIR). Ini adalah cara sistem pencahayaan otomatis gedung profesional bekerja — menghindari lampu menyala di siang hari yang sudah terang!

5. **Alarm Keamanan Berbasis Cahaya:** Deteksi apakah seseorang menutup/menghalangi sensor LDR (bayangan objek). Logika: Jika nilai cahaya turun drastis lebih dari 40% dalam <500ms → **ALARM!** (ada objek yang lewat di depan sensor). Ini adalah prinsip dasar *photoelectric beam sensor* di pintu otomatis toko — versi DIY-nya!

---

*Selanjutnya → [BAB 33: Sensor Internal ESP32](../BAB-33/README.md)*
