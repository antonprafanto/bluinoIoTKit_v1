# BAB 27: DS18B20 Lanjutan — Deep Sleep, Parasitic Power & Data Filter

> ✅ **Prasyarat:** Pastikan kamu sudah menyelesaikan **BAB 25 (OneWire Protocol & DS18B20 Dasar)**. BAB ini membangun langsung di atas fondasi yang sudah ada, jadi kamu harus sudah paham konsep ROM Code, `requestTemperatures()`, dan pola *Non-Blocking* dengan `millis()`.

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Menjelaskan perbedaan **Parasitic Power Mode** vs External Power dan kapan masing-masing digunakan.
- Mengintegrasikan DS18B20 dengan **ESP32 Deep Sleep** untuk sistem baterai yang awet berbulan-bulan.
- Menerapkan algoritma **Moving Average Filter** untuk menghaluskan dan meredam derau (*noise*) data sensor.
- Memahami implikasi koneksi kabel **panjang** (hingga 100m) pada kualitas sinyal 1-Wire.

---

## 27.1 Rekap Cepat — Fondasi BAB 25

Sebelum melompat ke teknik lanjutan, mari kita pastikan pola wajib dari BAB 25 sudah tertancap kuat:

```
DS18B20 di Kit Bluino:
  Pin Data = IO14  (pull-up 4K7Ω sudah ada di PCB)
  
Urutan baca NON-BLOCKING (wajib selalu digunakan!):
  1. sensors.setWaitForConversion(false);   // Mode non-blocking
  2. sensors.requestTemperatures();         // Kirim perintah konversi
  3. delay/millis >= millisToWaitForConversion()  // Tunggu ADC selesai
  4. sensors.getTempCByIndex(0);            // Baca hasil
  5. if (suhu == DEVICE_DISCONNECTED_C)     // Wajib periksa error!
```

> 💡 **Prinsip Abadi:** `+85°C` = sensor belum konversi, `-127°C` = sensor terputus. Kedua nilai ini adalah sinyal *distress*, bukan suhu nyata!

---

## 27.2 Parasitic Power Mode — Koneksi 2-Wire di Lapangan

Di BAB 25, kita hanya menggunakan **Normal Mode** (3 kabel: VCC, GND, DQ). Tapi ada skenario lapangan di mana kabel VCC tidak tersedia atau terlalu mahal untuk direntangkan jauh.

Jawabannya: **Parasitic Power Mode** — sensor mencuri daya dari jalur Data itu sendiri!

### Bagaimana Parasitic Power Bekerja?

```
Mode NORMAL (3-wire) — selalu digunakan di Kit Bluino:
  ESP32 VCC ─── Pin 3 (VDD) Sensor
  ESP32 GND ─── Pin 1 (GND) Sensor
  ESP32 IO14 ── Pin 2 (DQ)  Sensor
  Pull-up 4K7Ω ke VCC

Mode PARASITIK (2-wire) — tanpa kabel VCC terpisah:
  (tidak ada koneksi ke VDD sensor!)
  ESP32 GND ─── Pin 1 (GND) Sensor
  ESP32 GND ─── Pin 3 (VDD) Sensor  ← VDD dihubungkan ke GND!
  ESP32 IO14 ── Pin 2 (DQ)  Sensor
  Pull-up yang LEBIH KUAT (2.2KΩ) ke VCC diperlukan
```

Saat bus DQ dalam kondisi *HIGH*, kapasitor internal 800pF di dalam chip DS18B20 terisi muatan. Energi yang tersimpan inilah yang dipakai sensor saat proses konversi ADC 750ms berlangsung.

### Keterbatasan Parasitic Power Mode

```
┌─────────────────────────────────────────────────────────┐
│                  BATASAN PARASITIC MODE                  │
├─────────────────────────────────────────────────────────┤
│ ❌ Tidak bisa membaca suhu lebih dari satu sensor secara │
│    bersamaan (konversi harus satu-satu!)                 │
│ ❌ Tidak bisa digunakan di suhu > 100°C                  │
│    (arus konversi ADC melebihi kapasitas kapasitor)      │
│ ❌ Membutuhkan pull-up AKTIF (MOSFET) saat konversi      │
│    pada kabel panjang > 10m                              │
│ ✅ Cocok: instalasi kabel tunggal jarak jauh (pipa panas,│
│    tangki, dll.) dengan maksimal 1-2 sensor per segmen   │
└─────────────────────────────────────────────────────────┘
```

> ✅ **Kit Bluino selalu menggunakan Normal Mode.** Bagian ini murni untuk pemahaman lapangan — kamu TIDAK perlu mengubah hardware kit. Ilmu ini krusial saat kamu merancang sistem monitoring suhu pipa industri sendiri.

### Kode Khusus Parasitic Mode

Jika kamu suatu hari membangun sistem 2-wire, ada satu baris krusial yang wajib ditambahkan:

```cpp
/*
 * BAB 27 - Contoh: Inisialisasi DS18B20 untuk Parasitic Power Mode
 * (Referensi saja — Kit Bluino tidak perlu ini!)
 */

#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_PIN 14
OneWire        oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(115200);
  sensors.begin();

  // 🔴 KRITIS untuk Parasitic Power Mode:
  // KITA TIDAK BOLEH MEMAKAI POLA NON-BLOCKING DI SINI!
  // Karena jalur data pada kabel (DQ) sedang diculik / beralih fungsi
  // sebagai penyuplai listrik utama selama 750ms. Jika kita mendesak 
  // "bertanya" apakah sensor sudah siap (polling), ESP32 akan mencabut paksa 
  // tegangan Strong Pull-Up tersebut dan sensor akan mati di tengah konversi!
  sensors.setWaitForConversion(true); 
}

void loop() {
  // Perintah ini akan OTOMATIS memblokir eksekusi selama 750ms (untuk resolusi 12-bit)
  // sambil menahan jalur Data dalam keadaan HIGH konstan (mensuplai listrik parasit)!
  // 💡 MENGAPA KITA PAKAI `ByIndex(0)` DAN BUKAN `requestTemperatures()` SEPERTI DI BAB 25?
  // Jika kamu memakai requestTemperatures() standar, instruksi akan memerintahkan SEMUA sensor di
  // jaringan kabel untuk mengukur suhu BERSAMAAN. Jika kamu pasang 2 sensor saja, keduanya akan rebutan
  // mengambil setrum parasit di kabel yang sama secara serentak. Akibatnya? Tegangan kabel *Brownout* (anjlok)
  // dan sensor seketika mati bertebaran! Di mode parasitik multi-sensor, pengukuran WAJIB digilir satu per satu!
  sensors.requestTemperaturesByIndex(0);
  
  // Begitu fungsi di atas kembali, kita aman membaca hasilnya:
  float suhuC = sensors.getTempCByIndex(0);
  if (suhuC != DEVICE_DISCONNECTED_C) {
    Serial.printf("Suhu (Parasitic): %.2f °C\n", suhuC);
  }

  delay(2000);
}
```

**Output yang diharapkan:**
```text
Suhu (Parasitic): 28.31 °C
Suhu (Parasitic): 28.44 °C
Suhu (Parasitic): 28.56 °C
```

---

## 27.3 DS18B20 + ESP32 Deep Sleep — Sistem Baterai Awet

Ini adalah topik paling *game-changing* di bab ini. Kombinasi DS18B20 + ESP32 Deep Sleep memungkinkanmu membangun **termometer baterai yang bertahan 6–12 bulan** dengan baterai Li-Ion 18650 standar!

### Filosofi Hemat Daya Sistem IoT Baterai

```
Konsumsi daya ESP32 normal (WiFi aktif): ~160–240 mA
Konsumsi daya ESP32 Deep Sleep:          ~10–20 µA (bukan mA!)
DS18B20 saat idle:                       ~1 µA

Perhitungan kasar dengan baterai 2000mAh:
  Mode Normal penuh: 2000mAh / 200mA = 10 JAM saja!
  Mode Deep Sleep (bangun 10 det/15 menit):
    Arus rata-rata ≈ (10s × 80mA + 890s × 0.02mA) / 900s ≈ ~0.9 mA
    Masa pakai: 2000mAh / 0.9mA ≈ 2222 JAM ≈ ~93 HARI! 🔋
```

> ⚠️ **Mitos Baterai vs Realitas Hardware (PENTING!):** Kalkulasi arus mikro-ampere (µA) menakjubkan di atas HANYA bisa dicapai jika kamu mendesain papan sirkuit (PCB) mu sendiri bersama chip ESP32 telanjang, atau menggunakan *board* khusus Ultra-Low Power. Jika kamu menancapkan baterai Li-Ion ke ESP32 tipe *DevKit V1* pasaran (yang ada di dalam Kit-mu), jangan kaget jika daya tahannya merosot. Kenapa? Karena *board DevKit* menyimpan parasit energi seperti Regulator Tegangan murahan (AMS1117) dan chip USB-TTL (CP2102) yang terus menerus meminum 10~20 mA listrik **biarpun** otak ESP32-nya sedang koma tertidur lelap!

### Program 1: Logger Suhu Deep Sleep (Kirim via WiFi Tiap 15 Menit)

Ini adalah arsitektur IoT baterai paling profesional: ESP32 tidur pulas, terbangun setiap 15 menit, baca sensor, kirim data lewat WiFi, lalu tidur lagi!

```cpp
/*
 * BAB 27 - Program 1: DS18B20 + Deep Sleep (15 Menit Interval)
 *
 * Alur Eksekusi:
 *   [BANGUN] → baca sensor → kirim Serial/WiFi → [TIDUR 15 menit] → ulangi
 *
 * Catatan Deep Sleep:
 *   - Semua variabel RAM akan TERHAPUS saat tidur!
 *   - Gunakan RTC Memory untuk menyimpan data antar-siklus tidur.
 *   - GPIO normal (IO0-IO39) tetap mati saat tidur.
 *
 * Wiring (Kit Bluino): DS18B20 sudah di IO14
 */

#include <OneWire.h>
#include <DallasTemperature.h>

// ── Konfigurasi ─────────────────────────────────────────────────
#define ONE_WIRE_PIN           14
#define INTERVAL_TIDUR_DETIK   (15 * 60)  // 15 menit dalam detik
#define MAX_RIWAYAT            10          // Simpan 10 data terakhir di RTC RAM

// ── RTC Memory: Bertahan melewati siklus Deep Sleep! ────────────
// Data di sini tidak terhapus saat ESP32 tidur (selama catu daya ada)
RTC_DATA_ATTR int    siklusDone    = 0;
RTC_DATA_ATTR float  riwayatSuhu[MAX_RIWAYAT];
RTC_DATA_ATTR int    indeksRiwayat = 0;

// ── Objek Sensor ────────────────────────────────────────────────
OneWire           oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

// ── Fungsi Baca Suhu ────────────────────────────────────────────
float bacaSuhuSekali() {
  sensors.begin();
  
  // 💡 TIPS INDUSTRI HEMAT BATERAI (Resolusi vs Waktu Melek):
  // Resolusi 12-bit (0.06°C) butuh 750ms untuk selesai. Resolusi 9-bit (0.5°C) 
  // hanya butuh 94ms! Menggunakan 12-bit berarti CPU ESP32 menyala 650ms lebih 
  // lama di SETIAP SIKLUS, menyedot kapasitas baterai Li-Ion secara masif! 
  // Jika aplikasimu hanya butuh toleransi setengah derajat, wajib gunakan 9-bit.
  sensors.setResolution(12); // Ganti ke 9 (9-bit) untuk Ultra-Hemat!
  
  sensors.setWaitForConversion(false); // Wajib false agar KITA yang mengontrol timeout!

  sensors.requestTemperatures();

  // Tunggu konversi selesai dengan polling timeout maksimum 800ms
  unsigned long mulai = millis();
  while (!sensors.isConversionComplete()) {
    if (millis() - mulai > 800) {
      return DEVICE_DISCONNECTED_C; // Timeout → kembalikan error
    }
  }

  return sensors.getTempCByIndex(0);
}

void setup() {
  Serial.begin(115200);
  siklusDone++;

  Serial.println("\n══════════════════════════════════════════");
  Serial.printf(" BAB 27: Deep Sleep Cycle #%d\n", siklusDone);
  Serial.println("══════════════════════════════════════════");

  // Baca suhu
  float suhuC = bacaSuhuSekali();

  // Filter 85.0°C (Power-On Reset Error) dan -127°C (Terputus)
  if (suhuC == DEVICE_DISCONNECTED_C || suhuC == 85.0) {
    Serial.println("❌ Sensor error/belum siap (85.0)! Lewati siklus ini.");
  } else {
    // Simpan di riwayat RTC (buffer melingkar)
    riwayatSuhu[indeksRiwayat % MAX_RIWAYAT] = suhuC;
    indeksRiwayat++;

    Serial.printf("🌡️  Suhu sekarang : %.2f °C\n", suhuC);
    Serial.printf("📦  Data tersimpan: %d titik\n",
                  min(siklusDone, MAX_RIWAYAT));

    // Tampilkan riwayat ringkas
    Serial.println("\nRiwayat pembacaan:");
    int jumlah = min(siklusDone, MAX_RIWAYAT);
    for (int i = 0; i < jumlah; i++) {
      Serial.printf("  [%2d] %.2f °C\n", i + 1, riwayatSuhu[i]);
    }
  }

  // ── Masuk Deep Sleep ──────────────────────────────────────────
  Serial.println("\n💤 Masuk deep sleep...");
  Serial.printf("   Akan terbangun dalam %d detik (%d menit)\n",
                INTERVAL_TIDUR_DETIK, INTERVAL_TIDUR_DETIK / 60);
  Serial.flush(); // Pastikan semua Serial terkirim sebelum tidur

  // Konversi ke mikrodetik (Deep Sleep menggunakan satuan µs!)
  esp_sleep_enable_timer_wakeup((uint64_t)INTERVAL_TIDUR_DETIK * 1000000ULL);
  esp_deep_sleep_start(); // Titik ini adalah yang TERAKHIR dieksekusi!
  // Setelah ini, program akan mulai dari setup() kembali saat bangun!
}

void loop() {
  // loop() tidak pernah dieksekusi dalam arsitektur Deep Sleep murni!
  // ESP32 selalu mulai ulang dari setup() setiap kali bangun.
}
```

**Contoh output Serial selama 3 siklus bangun:**
```text
══════════════════════════════════════════
 BAB 27: Deep Sleep Cycle #1
══════════════════════════════════════════
🌡️  Suhu sekarang : 28.31 °C
📦  Data tersimpan: 1 titik

Riwayat pembacaan:
  [ 1] 28.31 °C
💤 Masuk deep sleep...
   Akan terbangun dalam 900 detik (15 menit)

══════════════════════════════════════════
 BAB 27: Deep Sleep Cycle #2
══════════════════════════════════════════
🌡️  Suhu sekarang : 28.50 °C
📦  Data tersimpan: 2 titik

Riwayat pembacaan:
  [ 1] 28.31 °C
  [ 2] 28.50 °C
💤 Masuk deep sleep...

══════════════════════════════════════════
 BAB 27: Deep Sleep Cycle #3
══════════════════════════════════════════
🌡️  Suhu sekarang : 28.19 °C
📦  Data tersimpan: 3 titik

Riwayat pembacaan:
  [ 1] 28.31 °C
  [ 2] 28.50 °C
  [ 3] 28.19 °C
💤 Masuk deep sleep...
```

> ⚠️ **Magic `RTC_DATA_ATTR`:** Variabel yang diberi atribut `RTC_DATA_ATTR` disimpan di **128 byte RTC SRAM** yang tetap aktif selama Deep Sleep (selama VCC ada). Tanpa atribut ini, semua variabel akan nol kembali setiap siklus karena RAM utama dimatikan!

---

## 27.4 Moving Average Filter — Meredam Derau Sensor

Di instalasi lapangan nyata (pabrik, gudang, pipa industri), sensor DS18B20 rentan dipengaruhi oleh:
- Interferensi elektromagnetik dari motor/inverter di sekitarnya
- Konduksi panas sementara dari matahari langsung ke kabel
- Noise listrik dari sumber catu daya switching

Akibatnya, bacaan suhu bisa "melompat" sesaat sebelum kembali normal. Untuk mengatasinya, kita gunakan **Moving Average Filter** — sebuah algoritma pemratarataan sederhana namun sangat efektif.

```cpp
/*
 * BAB 27 - Program 2: DS18B20 dengan Moving Average Filter
 *
 * Membandingkan nilai RAW (mentah) vs nilai yang sudah dihaluskan
 * menggunakan Buffer rata-rata bergerak N titik terakhir.
 */

#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_PIN  14
#define FILTER_SIZE   8   // Jumlah sampel untuk dirata-rata

OneWire           oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

// ── Buffer Moving Average ────────────────────────────────────────
float   bufferSuhu[FILTER_SIZE] = {0};
uint8_t indeksBuffer  = 0;
uint8_t jumlahSampel  = 0; // Lacak berapa banyak data yang sudah masuk

// ── Tambahkan sampel baru ke buffer dan kembalikan rata-rata ─────
float tambahSampelDanRataRata(float sampelBaru) {
  bufferSuhu[indeksBuffer] = sampelBaru;
  indeksBuffer = (indeksBuffer + 1) % FILTER_SIZE; // Buffer melingkar
  if (jumlahSampel < FILTER_SIZE) jumlahSampel++;

  float jumlah = 0;
  for (uint8_t i = 0; i < jumlahSampel; i++) {
    jumlah += bufferSuhu[i];
  }
  return jumlah / jumlahSampel;
}

unsigned long  tMulaiKonversi    = 0;
unsigned long  tBacaTerakhir     = 0;
unsigned long  waktu_konversi_ms = 750;

typedef enum { STATE_IDLE, STATE_CONVERTING } SensorState;
SensorState state = STATE_IDLE;

#define INTERVAL_BACA_MS 1000UL

bool sensorSiap = false;

void setup() {
  Serial.begin(115200);

  Serial.println("══════════════════════════════════════════════════════");
  Serial.println(" BAB 27: DS18B20 + Moving Average Filter");
  Serial.println("══════════════════════════════════════════════════════");
  Serial.printf( " Filter: %u titik terakhir dirata-ratakan\n", FILTER_SIZE);
  Serial.println("══════════════════════════════════════════════════════");

  sensors.begin();
  sensors.setWaitForConversion(false);
  sensors.setResolution(12);
  waktu_konversi_ms = sensors.millisToWaitForConversion(12);

  if (sensors.getDeviceCount() == 0) {
    Serial.println("❌ Tidak ada sensor ditemukan!");
    return;
  }

  Serial.println("✅ Sensor siap.\n");
  Serial.println("  Timestamp  │   Suhu RAW   │  Suhu Filter  │ Selisih");
  Serial.println("─────────────┼──────────────┼───────────────┼─────────");

  sensorSiap = true;
}

void loop() {
  if (!sensorSiap) return;

  unsigned long sekarang = millis();

  switch (state) {
    case STATE_IDLE:
      if (sekarang - tBacaTerakhir >= INTERVAL_BACA_MS) {
        sensors.requestTemperatures();
        tMulaiKonversi = sekarang;
        // 💡 Penjadwal Frekuensi Tetap (Fixed Frequency Scheduler):
        // Kita mencatat 'waktu mulai' tepat SEBELUM konversi, BUKAN sesudahnya!
        // Ini menjamin bahwa siklus total murni 1000ms mutlak (dari mulai ke mulai).
        // Jika kode ini diletakkan di dalam STATE_CONVERTING setelah konversi selesai, 
        // interval totalnya akan melar konyol menjadi 1000ms + 750ms konversi = 1750ms!
        tBacaTerakhir = sekarang; 
        state = STATE_CONVERTING;
      }
      break;

    case STATE_CONVERTING:
      if (sekarang - tMulaiKonversi >= waktu_konversi_ms) {
        float suhuRaw = sensors.getTempCByIndex(0);

        // Filter 85.0°C (Power-On Reset Error) dan -127°C (Terputus)
        // Jika 85.0 dibiarkan lolos, ia akan meracuni data Filter Rata-Rata kita!
        if (suhuRaw == DEVICE_DISCONNECTED_C || suhuRaw == 85.0) {
          Serial.printf(" [%8lu ms] ❌ Sensor terputus/Reset (%.2f)!\n", sekarang, suhuRaw);
        } else {
          float suhuFilter = tambahSampelDanRataRata(suhuRaw);
          float selisih    = suhuRaw - suhuFilter;

          Serial.printf(" [%8lu ms] │ %6.4f °C   │ %6.4f °C    │ %+.4f\n",
                        sekarang, suhuRaw, suhuFilter, selisih);
        }

        state = STATE_IDLE;
      }
      break;
  }
}
```

**Contoh output (perhatikan kolom Selisih semakin kecil saat filter "mengisi"):**
```text
══════════════════════════════════════════════════════
 BAB 27: DS18B20 + Moving Average Filter
══════════════════════════════════════════════════════
 Filter: 8 titik terakhir dirata-ratakan
══════════════════════════════════════════════════════
✅ Sensor siap.

  Timestamp  │   Suhu RAW   │  Suhu Filter  │ Selisih
─────────────┼──────────────┼───────────────┼─────────
 [    1751 ms] │ 28.3125 °C   │ 28.3125 °C    │ +0.0000
 [    2751 ms] │ 28.3750 °C   │ 28.3438 °C    │ +0.0313
 [    3751 ms] │ 28.4375 °C   │ 28.3750 °C    │ +0.0625
 [    4751 ms] │ 28.2500 °C   │ 28.3438 °C    │ -0.0938
 [    5751 ms] │ 29.5000 °C   │ 28.5750 °C    │ +0.9250  ← noise spike!
 [    6751 ms] │ 28.3125 °C   │ 28.5396 °C    │ -0.2271  ← filter meredam
 [    7751 ms] │ 28.3125 °C   │ 28.5000 °C    │ -0.1875
 [    8751 ms] │ 28.3125 °C   │ 28.4016 °C    │ -0.0891  ← kembali stabil
```

> 💡 **Perhatikan baris "noise spike":** Pada bacaan ke-5, suhu tiba-tiba melompat ke `29.5°C` (kemungkinan karena sentuhan tangan sebentar atau noise listrik). Kolom `Suhu Filter` hanya naik ke `28.57°C` — jauh lebih tenang! Inilah kekuatan *Moving Average*.

---

## 27.5 Koneksi Kabel Panjang — Panduan Instalasi Lapangan

Salah satu keunggulan DS18B20 yang jarang diajarkan: kabel data bisa sangat panjang (hingga **100 meter** dalam kondisi ideal). Berikut panduan lapangan yang wajib diketahui:

```
┌───────────────────────────────────────────────────────────────┐
│          PANDUAN KABEL PANJANG DS18B20                        │
├────────────────┬──────────────────────────────────────────────┤
│ Panjang Kabel  │ Konfigurasi yang Disarankan                  │
├────────────────┼──────────────────────────────────────────────┤
│ 0 – 10 m      │ Pull-up 4.7KΩ — kondisi standar              │
│ 10 – 30 m     │ Pull-up 2.2KΩ (lebih rendah untuk mengatasi  │
│               │ kapasitansi kabel yang lebih besar)           │
│ 30 – 100 m    │ Pull-up 1KΩ + tambahkan kapasitor 100nF       │
│               │ antara VCC (VDD) dan GND di ujung kabel       │
│               │ (Fungsinya: Stabilisasi tegangan lokal)       │
│ > 100 m       │ Gunakan konverter RS-485 / HART               │
├────────────────┼──────────────────────────────────────────────┤
│ Jenis Kabel   │ CAT5/CAT6 lebih baik dari kabel NYAF biasa   │
│               │ (twisted pair mengurangi interferensi EMI)    │
└────────────────┴──────────────────────────────────────────────┘
```

> ⚠️ **Kabel panjang = kapasitansi tinggi.** Setiap meter kabel menambahkan ~30-100pF kapasitansi parasit. Ini memperlambat tepi naik sinyal 1-Wire dan bisa menyebabkan error CRC. Solusinya adalah memperkuat pull-up (nilai resistor lebih rendah) agar arus pengisian kapasitansi lebih besar.

---

## 27.6 Tips & Troubleshooting Lanjutan

### 1. Deep Sleep Tidak Berjalan / ESP32 Langsung Reset?
```text
✅ Cek 1: Pastikan tidak ada Serial.println() setelah esp_deep_sleep_start()
          Tambahkan Serial.flush() SEBELUM memanggil deep sleep!

✅ Cek 2: Jika menggunakan ESP32 DevKit, pastikan tombol BOOT tidak tertekan
          saat proses booting (bisa memaksa masuk ke mode download).

✅ Cek 3: RTC_DATA_ATTR hanya bertahan selama VCC tidak terputus.
          Jika baterai dicabut, semua data RTC hilang — normal!
```

### 2. Suhu Selalu 85°C Setelah Deep Sleep?
```text
❌ 85°C = Power-On Reset Value = DS18B20 baru saja menyala dan belum selesai mengukur!

Penyebab khusus Baterai/Deep Sleep (Arsitektur GPIO Power):
  Resep rahasia arsitek IoT sejati: Daripada VDD sensor dipasang ke 3.3V konstan,
  mereka memasangnya ke PIN DIGITAL ESP32 (misal IO33). Sebelum tidur, IO33 di-LOW
  (sensor mati total 0 µA). Saat bangun, IO33 di-HIGH. 
  TETAPI, sensor butuh ~50ms untuk "sadar" (booting) setelah strum masuk! Jika kamu
  langsung memanggil sensors.begin(), sensor akan panik dan mengirim angka 85.0°C!

✅ Solusi Wajib Sistem GPIO Power:
  digitalWrite(33, HIGH); // Nyalakan listrik sensor
  delay(50);              // Beri memori internal DS18B20 waktu untuk bernapas
  sensors.begin();        // Baru mulai komunikasi 1-Wire
```

### 3. Nilai Suhu Berbeda dari DS18B20 Sebelum vs Sesudah Deep Sleep?
```text
Ini normal! Ada dua penyebab sah:
  A. Suhu lingkungan memang berubah selama ESP32 tidur (ini yang diharapkan!)
  B. Resolusi sensor reset ke default (9-bit) setelah power cycle.

✅ Solusi B: Selalu panggil sensors.setResolution(12) di setiap setup()
             setelah bangun dari Deep Sleep!
```

---

## 27.7 Ringkasan

```
┌─────────────────────────────────────────────────────────────┐
│           RINGKASAN BAB 27 — DS18B20 LANJUTAN               │
├─────────────────────────────────────────────────────────────┤
│ Topik 1: Parasitic Power (2-wire)                           │
│   → VDD sensor dihubungkan ke GND, bukan VCC               │
│   → Sensor parasit dari jalur DQ                           │
│   → Batasi 1-2 sensor per bus, pull-up lebih kuat           │
│                                                             │
│ Topik 2: Deep Sleep                                         │
│   → Konsumsi turun dari ~200mA ke ~0.02mA!                  │
│   → Gunakan RTC_DATA_ATTR untuk menyimpan data antar-tidur  │
│   → esp_sleep_enable_timer_wakeup() + esp_deep_sleep_start()│
│   → Selalu Serial.flush() sebelum tidur                     │
│   → loop() tidak pernah dijalankan!                         │
│                                                             │
│ Topik 3: Moving Average Filter                              │
│   → Buffer melingkar N titik (FILTER_SIZE = 8 dianjurkan)   │
│   → Meredam spike noise sesaat di lapangan                  │
│   → Tambahkan latency sebesar N × interval_baca             │
│                                                             │
│ Topik 4: Kabel Panjang                                      │
│   → 0-10m: pull-up 4.7KΩ                                    │
│   → 10-30m: pull-up 2.2KΩ                                   │
│   → 30-100m: pull-up 1KΩ + kapasitor 100nF                  │
└─────────────────────────────────────────────────────────────┘
```

---

## 27.8 Latihan

1. **Termometer Portabel Baterai:** Modifikasi Program 1 (Deep Sleep) agar setelah bangun, ESP32 menyambung ke WiFi dan mengirim suhu ke platform **Telegram Bot** (preview BAB 61). Cukup tampilkan pesan sederhana: `"Suhu gudang: 28.3°C — [timestamp]"`.

2. **Filter Perbandingan:** Modifikasi Program 2 untuk mencoba ukuran `FILTER_SIZE` yang berbeda: 4, 8, dan 16. Simpan ketiga nilai terfilter ke buffer terpisah dan tampilkan ketiganya dalam satu baris tabel. Analisalah: ukuran filter mana yang paling responsif vs paling stabil?

3. **Data Logger Baterai + SD Card:** Gabungkan arsitektur Deep Sleep (BAB 27) dengan SD Card logger (BAB 24). Setiap kali ESP32 bangun: Mount SD Card, Buka file di mode *APPEND* (`SD.open`), tuangkan data CSV suhu, lalu **TUTUP SAAT ITU JUGA (`file.close()`)**. BUKAN ditutup setelah 100 siklus! Mengapa? Jika kamu menidurkan ESP32 secara *Deep Sleep* saat file SD Card masih menggantung terbuka di memori RAM, saat itu juga struktur file FAT32-mu akan terkorupsi permanen!

4. **Alarm Suhu Dingin Gudang:** Buat sistem yang membaca suhu setiap 5 menit dengan Deep Sleep. Jika suhu di bawah 10°C atau di atas 35°C, ESP32 *tidak* tidur lagi dan mengaktifkan LED peringatan terus-menerus sampai suhu kembali normal.

---

*Selanjutnya → [BAB 28: BMP180 — Sensor Tekanan Udara & Altitude](../BAB-28/README.md)*
