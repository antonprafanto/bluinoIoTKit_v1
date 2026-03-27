# BAB 28: BMP180 — Sensor Tekanan Udara & Ketinggian (Altitude)

> ✅ **Prasyarat:** Pastikan kamu sudah menyelesaikan **BAB 23 (I2C — Inter-Integrated Circuit)**. BMP180 menggunakan protokol I2C pada bus yang sama dengan OLED dan MPU-6050 di Kit Bluino. Kamu harus paham konsep *shared bus*, I2C address, dan `Wire.begin()`.

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Menjelaskan prinsip fisika di balik pengukuran altitude menggunakan tekanan atmosfer.
- Membaca dua parameter sekaligus dari BMP180: **Tekanan Udara (hPa)** dan **Suhu Ambien (°C)**.
- Menghitung **Ketinggian (Altitude)** secara presisi menggunakan rumus barometrik standar internasional.
- Menangani konfigurasi I2C *shared bus* di mana BMP180 berbagi jalur yang sama dengan OLED dan MPU-6050.
- Mendeteksi dan menangani kegagalan inisialisasi sensor dengan pesan diagnostik yang informatif.
- Menerapkan normalisasi tekanan ke **Mean Sea Level (MSL)** untuk mempersiapkan data bagi sistem cuaca.

---

## 28.1 Mengenal BMP180 — Si Penekan Udara

**BMP180** adalah sensor tekanan barometrik buatan *Bosch Sensortec* yang sangat populer di dunia IoT. Sensor ini mengukur tekanan udara atmosfer di sekitarnya menggunakan elemen *piezoresistive sensing* yang sangat sensitif — sebuah membran silikon tipis yang melentur ketika ada tekanan udara, mengubah resistansi bahan semikonduktornya.

Yang membuat sensor ini semakin hebat: **Di dalam chip yang sama**, terdapat sensor suhu tersembunyi yang digunakan sebagai kompensasi koefisien ekspansi termal. Artinya kamu mendapat dua sensor sekaligus!

### Spesifikasi Kunci BMP180

```
┌─────────────────────────────────────────────────────────────┐
│                  SPESIFIKASI BMP180                         │
├────────────────────────────┬────────────────────────────────┤
│ Parameter                  │ Nilai                          │
├────────────────────────────┼────────────────────────────────┤
│ Rentang Tekanan            │ 300 – 1100 hPa                 │
│ Akurasi Tekanan            │ ±0.12 hPa (pada 25°C)          │
│ Resolusi Tekanan           │ 0.01 hPa                       │
│ Rentang Suhu Internal      │ -40°C – +85°C                  │
│ Akurasi Suhu               │ ±0.5°C                         │
│ Protokol Komunikasi        │ I2C (Alamat: 0x77, tetap)      │
│ Tegangan Operasi           │ 1.62V – 3.6V                   │
│ Arus Konsumsi (Normal)     │ 0.5 µA – 650 µA                │
│ Mode Oversample (OSS)      │ 0 (Ultra Low Power)            │
│                            │ 1 (Standard) — digunakan       │
│                            │ 2 (High Resolution)            │
│                            │ 3 (Ultra High Resolution)      │
└────────────────────────────┴────────────────────────────────┘
```

> ⚠️ **BMP180 vs BMP280/BME280:** Kit Bluino menggunakan **BMP180**. Jika papanmu bertuliskan *BMP280* atau *BME280* (chip yang lebih baru dan murah), library-nya berbeda dan kamu harus menggunakan library Adafruit BMP280/BME280 bukan library Adafruit BMP085/BMP180!

---

## 28.2 Fisika di Balik Altitude — Mengapa Tekanan Mengukur Ketinggian?

Ini adalah konsep paling menawan dari seluruh sistem sensor IoT: mikrokontroler kecil murah ini bisa menghitung ketinggian suatu lokasi tanpa sinyal GPS!

**Prinsipnya sesederhana ini:** Semakin tinggi posisimu di atas permukaan laut, semakin sedikit kolom udara yang menindih tubuhmu dari atas. Lebih sedikit udara = Lebih sedikit berat = Tekanan lebih rendah.

Secara matematika, hubungan ini dimodelkan oleh **Persamaan Barometrik Internasional** (*International Standard Atmosphere*):

```
              ⎡   Tekanan_Ukur  ⎤^0.1903
Altitude(m) = ⎢1 - ────────────⎥          × 44330
              ⎣   Tekanan_MSL   ⎦

Di mana:
  Tekanan_Ukur = Tekanan yang dibaca sensor di lokasi kamu (hPa)
  Tekanan_MSL  = Tekanan di permukaan laut standar = 1013.25 hPa
  44330        = Konstanta ketinggian skala troposfer dalam meter
  0.1903       = Bilangan tetap turunan persamaan gas adiabatik
```

**Contoh Nyata:**
Jika sensor BMP180 di puncak Gunung Merapi membaca tekanan `719.8 hPa`:
```
Altitude = (1 - (719.8 / 1013.25)^0.1903) × 44330
         = (1 - (0.71033)^0.1903) × 44330
         = (1 - 0.93779) × 44330
         = 0.06221 × 44330
         ≈ 2758 meter ← (Ketinggian Sebenarnya Merapi sekitar 2930m, mendekati!)
```

> 💡 **Keterbatasan Kritis:** Persamaan barometrik mengasumsikan bahwa tekanan di permukaan laut selalu tepat `1013.25 hPa`. Kenyataannya, tekanan permukaan laut berubah setiap hari tergantung cuaca (bisa berkisar 950–1050 hPa). Inilah alasan mengapa GPS tetap dibutuhkan untuk navigasi presisi!

---

## 28.3 Konfigurasi Hardware di Kit Bluino

BMP180 di Kit Bluino terhubung langsung ke **bus I2C bersama (Shared I²C Bus)** yang sama dengan OLED dan MPU-6050:

```
           Kit Bluino ESP32 Starter Shield v3.2
                         
  ESP32 IO21 (SDA) ──┬──── OLED 0.96" (Addr: 0x3C)
                     ├──── BMP180       (Addr: 0x77)  ← Bab ini
                     └──── MPU-6050     (Addr: 0x68)
                         
  ESP32 IO22 (SCL) ──┬──── OLED 0.96" (Addr: 0x3C)
                     ├──── BMP180       (Addr: 0x77)
                     └──── MPU-6050     (Addr: 0x68)
```

> ✅ **Kabar Baik:** Tidak ada kabel jumper tambahan yang perlu dipasang! BMP180 sudah terhubung secara permanen ke IO21/IO22 di atas PCB Kit Bluino. Resistor pull-up I2C sudah terpasang di papan.

> ⚠️ **Perhatikan Alamat I2C:** BMP180 menggunakan alamat `0x77` secara permanen (tidak bisa diubah). Pastikan tidak ada sensor lain dengan alamat yang sama pada bus yang sama. Dalam Kit Bluino, ini tidak masalah karena OLED menggunakan `0x3C` dan MPU-6050 menggunakan `0x68`.

---

## 28.4 Instalasi Library

BMP180 menggunakan library Adafruit yang juga mendukung kakak lamanya BMP085:

```
Library Manager (Ctrl+Shift+I) → Ketik "BMP085"

1. Instal: "Adafruit BMP085 Library" by Adafruit
           (Library ini mendukung KEDUA BMP085 dan BMP180!)
2. Jika ditanya dependensi, pilih "Install All"
3. Pastikan "Adafruit Unified Sensor" ikut terinstal.
```

> 💡 Meskipun sensor kamu adalah BMP180, library yang digunakan tetap bernama **BMP085**. Ini karena BMP180 adalah penerus langsung BMP085 dengan protokol komunikasi yang secara internal identik — Bosch tidak mengubah set perintah I2Cnya.

---

## 28.5 Program 1: Pembacaan Tekanan & Suhu Dasar

Program fondasi ini mendemonstrasikan cara membaca dua besaran fisik sekaligus dari BMP180.

```cpp
/*
 * BAB 28 - Program 1: BMP180 Pembacaan Tekanan & Suhu Dasar
 *
 * Target: Membaca Tekanan Udara (hPa) dan Suhu Ambien (°C)
 * Wiring (Topologi Kit Bluino):
 *   BMP180 SDA → IO21  (Shared I2C Bus)
 *   BMP180 SCL → IO22  (Shared I2C Bus)
 *   Alamat I2C : 0x77  (Tetap, tidak bisa di-jumper)
 */

#include <Wire.h>
#include <Adafruit_BMP085.h>

// ── Objek Sensor ────────────────────────────────────────────────
// Library menyediakan class Adafruit_BMP085 yang support BMP180 juga!
Adafruit_BMP085 bmp;

void setup() {
  Serial.begin(115200);

  Serial.println("══════════════════════════════════════════");
  Serial.println("    BAB 28: BMP180 — Tekanan & Suhu");
  Serial.println("══════════════════════════════════════════");

  // Inisialisasi I2C Bus dan komunikasi dengan BMP180
  // Jika sensor tidak terdeteksi di alamat 0x77, begin() mengembalikan false
  if (!bmp.begin()) {
    Serial.println("❌ GAGAL: Sensor BMP180 tidak ditemukan!");
    Serial.println("   Kemungkinan penyebab:");
    Serial.println("   → PCB rusak atau koneksi solder IO21/IO22 buruk");
    Serial.println("   → Konflik alamat I2C di shared bus (cek MPU-6050/OLED)");
    Serial.println("   → Sensor BMP180 rusak (coba ganti modul)");
    // Hentikan eksekusi selamanya — tidak ada gunanya melanjutkan
    while (true) delay(10);
  }

  Serial.println("✅ Sensor BMP180 terdeteksi dan siap!");
  Serial.println("──────────────────────────────────────────");
}

void loop() {
  // ── Ambil nilai dari sensor ────────────────────────────────────
  float suhuC     = bmp.readTemperature();       // °C
  int32_t tekPa   = bmp.readPressure();           // Pascal (bukan hPa!)
  float   takhPa  = tekPa / 100.0f;              // Konversi ke hektoPascal

  // ── Tampilkan hasil ───────────────────────────────────────────
  Serial.printf("🌡️  Suhu         : %5.1f °C\n",   suhuC);
  Serial.printf("🌬️  Tekanan      : %7.2f hPa\n",  takhPa);
  Serial.println("──────────────────────────────────────────");

  delay(2000);
}
```

**Contoh output yang diharapkan (dataran rendah, Jakarta):**
```text
══════════════════════════════════════════
    BAB 28: BMP180 — Tekanan & Suhu
══════════════════════════════════════════
✅ Sensor BMP180 terdeteksi dan siap!
──────────────────────────────────────────
🌡️  Suhu         :  29.4 °C
🌬️  Tekanan      : 1009.87 hPa
──────────────────────────────────────────
🌡️  Suhu         :  29.5 °C
🌬️  Tekanan      : 1009.86 hPa
──────────────────────────────────────────
```

**Tiga poin penting untuk dipahami:**
1. `bmp.readPressure()` mengembalikan nilai dalam satuan **Pascal** (Pa), bukan hektoPascal (hPa). Standar meteorologi menggunakan hPa, jadi kita **wajib** membaginya dengan 100.
2. Nilai tekanan di Jakarta sekitar `1009–1013 hPa`. Di Bandung (740m), sekitar `920–930 hPa`. Di Puncak Jayawijaya (4884m), sekitar `543 hPa`.
3. Suhu BMP180 adalah suhu **di dalam chip**, bukan suhu udara terbuka. Ia cenderung lebih tinggi 1–3°C dari suhu udara aktual karena panas dari komponen PCB di sekitarnya!

---

## 28.6 Program 2: Kalkulator Ketinggian Barometrik (Altitude)

Ini program paling menarik! ESP32 mengubah data tekanan fisik menjadi angka ketinggian yang bisa dipahami manusia.

```cpp
/*
 * BAB 28 - Program 2: Kalkulator Ketinggian Barometrik
 *
 * Menghitung altitude (meter) dari tekanan udara menggunakan
 * Persamaan Barometrik Internasional yang sudah disematkan
 * langsung di dalam library Adafruit BMP085/BMP180.
 *
 * ⚠️ PENTING: Ubah nilai TEKANAN_PERMUKAAN_LAUT sesuai kondisi
 *    cuaca lokalmu hari ini agar hasil altitude akurat!
 */

#include <Wire.h>
#include <Adafruit_BMP085.h>

Adafruit_BMP085 bmp;

// ── Tekanan Referensi Permukaan Laut ────────────────────────────
// Nilai STANDAR: 1013.25 hPa (rata-rata global)
// Untuk akurasi terbaik: ambil nilai tekanan hari ini dari BMKG 
// (bmkg.go.id) dan masukkan ke sini dalam satuan hPa.
const float TEKANAN_PERMUKAAN_LAUT = 1013.25f; // hPa

void setup() {
  Serial.begin(115200);

  Serial.println("═════════════════════════════════════════════════");
  Serial.println(" BAB 28: Kalkulator Altitude Barometrik BMP180");
  Serial.println("═════════════════════════════════════════════════");
  Serial.printf( " Referensi MSL: %.2f hPa\n", TEKANAN_PERMUKAAN_LAUT);
  Serial.println("═════════════════════════════════════════════════");

  if (!bmp.begin()) {
    Serial.println("❌ Sensor BMP180 tidak ditemukan! Cek kabel IO21/IO22.");
    while (true) delay(10);
  }
  Serial.println("✅ BMP180 siap.\n");
}

void loop() {
  float  suhuC   = bmp.readTemperature();
  int32_t tekPa  = bmp.readPressure();
  float  takhPa  = tekPa / 100.0f;

  // ── Kalkulasi Altitude via Library ───────────────────────────
  // Library Adafruit sudah mengimplementasi Persamaan Barometrik Internasional!
  // Parameter: tekanan referensi MSL dalam satuan Pascal (bukan hPa!)
  float altitudeM = bmp.readAltitude(TEKANAN_PERMUKAAN_LAUT * 100.0f);

  // ── Tampilkan Laporan Lengkap ─────────────────────────────────
  Serial.printf("Suhu Ambien   : %5.1f °C\n",       suhuC);
  Serial.printf("Tekanan Udara : %7.2f hPa\n",       takhPa);
  Serial.printf("Ketinggian    : %7.1f m (dpl)\n",   altitudeM);

  // Kategorisasi ketinggian untuk edukasi geografi
  if (altitudeM < 200) {
    Serial.println("Zona: Dataran Rendah (Pantai/Pesisir)");
  } else if (altitudeM < 700) {
    Serial.println("Zona: Dataran Sedang");
  } else if (altitudeM < 1500) {
    Serial.println("Zona: Dataran Tinggi");
  } else {
    Serial.println("Zona: Pegunungan");
  }

  Serial.println("─────────────────────────────────────────────────");
  delay(2000);
}
```

**Contoh output di Bandung (~740 m dpl):**
```text
═════════════════════════════════════════════════
 BAB 28: Kalkulator Altitude Barometrik BMP180
═════════════════════════════════════════════════
 Referensi MSL: 1013.25 hPa
═════════════════════════════════════════════════
✅ BMP180 siap.

Suhu Ambien   :  23.7 °C
Tekanan Udara : 921.43 hPa
Ketinggian    :   762.3 m (dpl)
Zona: Dataran Tinggi
─────────────────────────────────────────────────
```

> 💡 **Mengapa `bmp.readAltitude()` menerima Pascal bukan hPa?** 
> Fungsi `readAltitude()` sendiri mengimplementasikan persamaan ISO 2533:1975 secara internal dengan satuan Pascal. Kita yang menulis konstanta dalam hPa harus mengkonversinya dengan mengalikan 100 sebelum dimasukkan ke fungsi tersebut. Ini jadikan catatan saat debugging!

---

## 28.7 Program 3: Normalisasi Tekanan ke Mean Sea Level (MSL)

Dalam dunia meteorologi dan stasiun cuaca IoT, ada teknik penting bernama **Normalisasi Tekanan ke Mean Sea Level** — mengubah tekanan yang diukur di ketinggian tertentu menjadi nilai yang *seolah-olah* diukur di permukaan laut.

**Mengapa ini penting?** Bayangkan kamu membangun jaringan stasiun cuaca di seluruh Indonesia — satu di Jakarta (5m dpl), satu di Bandung (740m), satu di Dieng (2093m). Jika kamu plot tekanannya dalam satu grafik, Dieng akan *selalu* tampak paling rendah (karena berada paling tinggi), bukan karena cuaca buruk, tapi hanya karena fisika ketinggian! Para meteorolog membutuhkan semua stasiun "berbicara" dalam satuan referensi yang sama.

```cpp
/*
 * BAB 28 - Program 3: Normalisasi Tekanan ke MSL (Mean Sea Level)
 *
 * Teknik wajib untuk stasiun cuaca IoT dalam jaringan multi-lokasi.
 *
 * ✏️ INSTRUKSI KALIBRASI (Sekali saja!):
 *   1. Cari tahu ketinggian lokasi Anda saat ini (dari Google Maps → 
 *      ketuk lama titik lokasi → baca angka "Altitude")
 *   2. Masukkan nilai ke KETINGGIAN_LOKAL_M di bawah
 */

#include <Wire.h>
#include <Adafruit_BMP085.h>

Adafruit_BMP085 bmp;

// ── Kalibrasi Lokal — ISI SESUAI LOKASIMU! ──────────────────────
const float KETINGGIAN_LOKAL_M = 740.0f; // Contoh: Bandung = 740m dpl
                                           // Jakarta = 5m, Dieng = 2093m

void setup() {
  Serial.begin(115200);

  Serial.println("══════════════════════════════════════════════════════");
  Serial.println(" BAB 28: Stasiun Cuaca — Normalisasi Tekanan ke MSL");
  Serial.println("══════════════════════════════════════════════════════");
  Serial.printf( " Ketinggian Lokal Terkonfigurasi: %.0f m dpl\n",
                  KETINGGIAN_LOKAL_M);
  Serial.println("══════════════════════════════════════════════════════");

  if (!bmp.begin()) {
    Serial.println("❌ BMP180 tidak ditemukan! Cek jalur IO21/IO22.");
    while (true) delay(10);
  }
  Serial.println("✅ BMP180 siap.\n");
}

void loop() {
  float  suhuC   = bmp.readTemperature();
  int32_t tekPa  = bmp.readPressure();
  float  tekHPa  = tekPa / 100.0f;

  // ── Kalkulasi Tekanan di Permukaan Laut ──────────────────────
  // readSealevelPressure() menggunakan rumus balik dari persamaan barometrik:
  // Kita tahu tekanan di lokasi ini & ketinggiannya → hitung berapa tekanannya
  // jika kita ada di permukaan laut pada kondisi atmosfer yang sama hari ini!
  // Parameter: ketinggian lokal dalam METER
  float mslHPa = bmp.readSealevelPressure(KETINGGIAN_LOKAL_M) / 100.0f;

  // ── Laporan Stasiun Cuaca ─────────────────────────────────────
  Serial.printf("Suhu Ambien           : %5.1f °C\n",  suhuC);
  Serial.printf("Tekanan Lokasi (%.0fm): %7.2f hPa\n", KETINGGIAN_LOKAL_M, tekHPa);
  Serial.printf("Tekanan MSL (Norm.)   : %7.2f hPa",   mslHPa);

  // Penilaian sederhana kondisi atmosfer
  if (mslHPa > 1020.0f) {
    Serial.println(" [Tekanan Tinggi — Cuaca Cerah] 🌤️");
  } else if (mslHPa >= 1013.0f) {
    Serial.println(" [Tekanan Normal — Stabil] ⛅");
  } else if (mslHPa >= 1000.0f) {
    Serial.println(" [Tekanan Rendah — Potensi Mendung] 🌥️");
  } else {
    Serial.println(" [Tekanan Sangat Rendah — Waspada Hujan Lebat!] 🌩️");
  }

  Serial.println("──────────────────────────────────────────────────────");
  delay(2000);
}
```

**Contoh output:**
```text
══════════════════════════════════════════════════════
 BAB 28: Stasiun Cuaca — Normalisasi Tekanan ke MSL
══════════════════════════════════════════════════════
 Ketinggian Lokal Terkonfigurasi: 740 m dpl
══════════════════════════════════════════════════════
✅ BMP180 siap.

Suhu Ambien           :  23.7 °C
Tekanan Lokasi (740m) : 921.43 hPa
Tekanan MSL (Norm.)   : 1012.85 hPa [Tekanan Normal — Stabil] ⛅
──────────────────────────────────────────────────────
```

---

## 28.8 Program 4: Non-Blocking State Machine (Arsitektur IoT Profesional)

Perlu kamu ketahui: semua fungsi `bmp.read*()` di atas mengakibatkan ESP32 memblokir sementara komunikasi I2C dan menunggu jawaban BMP180. Untuk sistem IoT yang lebih besar (misalnya sambil menjalankan OTA Update atau Web Server), kita harus menerapkan pola non-blocking:

```cpp
/*
 * BAB 28 - Program 4: BMP180 Non-Blocking State Machine
 *
 * Membaca BMP180 di latar belakang tanpa memblokir loop() utama.
 * Arsitektur ini kompatibel dengan Web Server, MQTT, dan FreeRTOS.
 */

#include <Wire.h>
#include <Adafruit_BMP085.h>

Adafruit_BMP085 bmp;

// ── Konfigurasi Waktu ────────────────────────────────────────────
const unsigned long INTERVAL_BACA_MS    = 3000UL; // Baca tiap 3 detik
const float         KETINGGIAN_LOKAL_M  = 740.0f; // Sesuaikan!

// ── Timer State Machine ──────────────────────────────────────────
unsigned long tBacaTerakhir = 0;

// ── Data Register — Global untuk Web Server / OLED ──────────────
struct DataAtmosfer {
  float   suhu;
  float   tekananHPa;
  float   tekananMslHPa;
  float   altitudeM;
  bool    sukses = false;
};

DataAtmosfer atmosferTerakhir;

// ── Fungsi Pembaruan Data ────────────────────────────────────────
void perbaruiDataBMP() {
  unsigned long sekarang = millis();

  // 💡 Trik Free Warm-Up: tBacaTerakhir diawali dari 0, sehingga
  // pembacaan pertama secara alami terjadi setelah INTERVAL_BACA_MS ms!
  if (sekarang - tBacaTerakhir < INTERVAL_BACA_MS) return;
  tBacaTerakhir = sekarang;

  float   suhuC  = bmp.readTemperature();
  int32_t tekPa  = bmp.readPressure();

  // Validasi: jika tekanan di luar rentang fisik, sensor problem!
  // BMP180 hanya bisa membaca 30.000 Pa – 110.000 Pa (300–1100 hPa)
  if (tekPa < 30000L || tekPa > 110000L) {
    atmosferTerakhir.sukses = false;
    Serial.printf("[%6lu ms] ❌ Data BMP180 di luar rentang fisik: %d Pa\n",
                  sekarang, tekPa);
    return;
  }

  float tekHPa = tekPa / 100.0f;
  float mslHPa = bmp.readSealevelPressure(KETINGGIAN_LOKAL_M) / 100.0f;
  float altM   = bmp.readAltitude(1013.25f * 100.0f);

  // Simpan ke register global (Latch-on-Fail: nilai lama dipertahankan jika error)
  atmosferTerakhir.suhu           = suhuC;
  atmosferTerakhir.tekananHPa     = tekHPa;
  atmosferTerakhir.tekananMslHPa  = mslHPa;
  atmosferTerakhir.altitudeM      = altM;
  atmosferTerakhir.sukses         = true;

  Serial.printf("[%6lu ms] 🌬️  T:%.1f°C | P:%.2f hPa | MSL:%.2f hPa | Alt:%.1fm\n",
                sekarang, suhuC, tekHPa, mslHPa, altM);
}

void setup() {
  Serial.begin(115200);

  Serial.println("══════════════════════════════════════════════════");
  Serial.println(" BAB 28: BMP180 Async State Machine");
  Serial.println("══════════════════════════════════════════════════");

  if (!bmp.begin()) {
    Serial.println("❌ BMP180 tidak ditemukan! Cek bus I2C IO21/IO22.");
    while (true) delay(10);
  }

  Serial.println("✅ BMP180 siap — memulai penjadwal asinkron.\n");
}

void loop() {
  // Jalankan pembaca atmosfer di latar belakang
  perbaruiDataBMP();

  // Mesin bergerak bebas di sini!
  // if (webClient.available()) kirimJsonAtmosfer(atmosferTerakhir);
  // if (tombolDitekan) tampilOLED(atmosferTerakhir.altitudeM);
}
```

**Contoh output yang diharapkan:**
```text
══════════════════════════════════════════════════
 BAB 28: BMP180 Async State Machine
══════════════════════════════════════════════════
✅ BMP180 siap — memulai penjadwal asinkron.

[  3001 ms] 🌬️  T:23.8°C | P:921.43 hPa | MSL:1012.85 hPa | Alt:762.3m
[  6001 ms] 🌬️  T:23.9°C | P:921.38 hPa | MSL:1012.79 hPa | Alt:762.7m
[  9001 ms] 🌬️  T:23.9°C | P:921.45 hPa | MSL:1012.87 hPa | Alt:762.1m
```

---

## 28.9 Tips & Panduan Troubleshooting

### 1. `bmp.begin()` Selalu Mengembalikan `false`
```text
Penyebab yang paling umum (urut berdasarkan frekuensi):

A. I2C Bus tidak bisa diinisialisasi:
   → Cek apakah ada pemanggilan Wire.begin() ganda di program yang
     menyebabkan konflik dengan inisialisasi OLED atau MPU-6050.
   → Solusi: Pastikan hanya satu Wire.begin(SDA, SCL) sebelum semua
     inisialisasi sensor dimulai.

B. Konflik Alamat I2C (Address Conflict):
   → Scan bus I2C terlebih dahulu dengan program scanner:
     for(byte addr=1; addr<127; addr++) {
       Wire.beginTransmission(addr);
       if (Wire.endTransmission() == 0)
         Serial.printf("Perangkat ditemukan: 0x%02X\n", addr);
     }
   → BMP180 seharusnya muncul di 0x77

C. Sensor rusak fisik:
   → Tegangan berlebih (>3.6V) dapat membakar chip BMP180.
   → Selalu pastikan sensor terhubung ke jalur 3.3V, bukan 5V!
```

### 2. Nilai Altitude Selalu Jauh Berbeda dari Kenyataan?
```text
Ini adalah masalah Referensi Tekanan MSL, bukan masalah sensor!

Tekanan atmosfer di permukaan laut berubah setiap hari seiring cuaca.
Perbedaan 1 hPa dari nilai referensi MSL akan menggeser altitude ~8.5 meter!

Solusi Akurat:
  1. Buka situs BMKG (bmkg.go.id → Data Online → Cuaca)
  2. Cari stasiun cuaca terdekat
  3. Ambil nilai tekanan MSL yang mereka laporkan hari ini
  4. Masukkan nilai tersebut ke konstanta TEKANAN_PERMUKAAN_LAUT

Solusi Praktis (Kalibrasi Manual):
  1. Cari tahu ketinggian presisi lokasi saat ini (Google Maps)
  2. Baca nilai tekanan dari sensor: tekPa = bmp.readPressure()
  3. Hitung tekanan MSL terbalik secara manual
```

### 3. Suhu yang Dibaca BMP180 Lebih Tinggi dari Suhu Ruangan?
```text
Normal! BMP180 mengukur suhu INTERNAL chip, bukan udara terbuka.

Penyebab:
  - Panas konduksi dari PCB dan komponen di sekitarnya (ESP32 bisa
    menghasilkan panas 2–5°C saat WiFi aktif penuh!)
  - BMP180 menggunakan suhu internalnya untuk kompensasi tekanan,
    bukan sebagai termometer udara.

Jika kamu butuh suhu udara yang akurat: gunakan DHT11 (BAB 26) atau
DS18B20 (BAB 25) yang terisolasi secara termal dari papan utama.
```

### 4. Data `0 Pa` atau `NaN` Sesekali Muncul?
```text
Ini adalah masalah I2C bus collision (tabrakan data).

Penyebab: Jika kamu membaca BMP180 dan OLED/MPU-6050 secara berurutan
sangat cepat tanpa jeda, transaksi I2C dari satu perangkat bisa
"tumpang tindih" dengan perangkat lain di shared bus!

Solusi:
  1. Tambahkan delay minimal 5ms antara setiap pembacaan sensor yang
     berbeda di bus I2C yang sama.
  2. Atau gunakan pola non-blocking (Program 4) di mana setiap sensor
     memiliki timer interval sendiri-sendiri.
```

---

## 28.10 Ringkasan

```
┌─────────────────────────────────────────────────────────────┐
│             RINGKASAN BAB 28 — ESP32 + BMP180               │
├─────────────────────────────────────────────────────────────┤
│ Protokol    = I2C pada IO21 (SDA) dan IO22 (SCL)            │
│ Alamat I2C  = 0x77 (Tetap — tidak ada jumper alternate)     │
│ Library     = "Adafruit BMP085 Library" (support BMP180!)   │
│                                                             │
│ Prosedur Setup Wajib:                                       │
│   Adafruit_BMP085 bmp;                                      │
│   if (!bmp.begin()) { /* tangani error! */ }                │
│                                                             │
│ Pembacaan Data:                                             │
│   float suhu    = bmp.readTemperature();      // °C         │
│   int32_t tekPa = bmp.readPressure();         // Pascal!    │
│   float takhPa  = tekPa / 100.0f;            // → hPa      │
│   float altM    = bmp.readAltitude(MSL_PA);  // meter      │
│   float mslPa   = bmp.readSealevelPressure(ketinggianM);   │
│                                                             │
│ KONVERSI SATUAN (WAJIB DIINGAT!):                           │
│   readPressure()       → Pascal     → bagi 100 → hPa        │
│   readAltitude()       → masukkan MSL dalam Pascal (×100)   │
│   readSealevelPressure → masukkan ketinggian dalam METER    │
│                                                             │
│ Validasi Data Range (gunakan di Non-Blocking!):             │
│   Tekanan valid: 30.000 Pa – 110.000 Pa                     │
│   Jika di luar range → hardware/kabel bermasalah            │
└─────────────────────────────────────────────────────────────┘
```

---

## 28.11 Latihan

1. **Altimeter Pendaki:** Buat program yang pertama-tama mengkalibrasi titik dasar (`alt_base = bmp.readAltitude(...)`) saat reset, lalu terus-menerus menghitung dan menampilkan **selisih ketinggian** (`delta_alt = alt_sekarang - alt_base`) setiap detik. Ini mirip cara kerja komputer mendaki gunung (Altimeter Watch)!

2. **Barometer Cuaca 24 Jam:** Simpan nilai tekanan MSL ke **RTC Memory** (seperti di BAB 27) setiap 30 menit. Setelah 24 siklus, bandingkan nilai tertinggi dan terendah. Trend penurunan tekanan >10 hPa dalam 3 jam? Kirim peringatan via Serial: *"PERHATIAN: Cuaca Buruk Mendekat!"*

3. **OLED Weather Station Lengkap:** Kombinasikan BAB 28 (BMP180) + BAB 26 (DHT11) + BAB 23 (OLED). Tampilkan pada satu layar OLED 128×64: baris atas `T:29.5°C  H:65%`, baris tengah `P:1013 hPa`, baris bawah `Alt: 762m`. Gunakan arsitektur Non-Blocking untuk keduanya!

4. **Survei Ketinggian Gedung:** Letakkan sensor di lantai dasar, catat `alt_lantai_satu`. Naik ke lantai teratas, catat `alt_puncak`. Hitung tinggi gedung berdasarkan selisih altitude! (Catatan: akurasi BMP180 sekitar ±3 meter, cukup untuk gedung 5 lantai ke atas.)

---

*Selanjutnya → [BAB 29: MPU-6050 — Akselerometer & Giroskop (6-DOF IMU)](../BAB-29/README.md)*
