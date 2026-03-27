# BAB 25: OneWire & Sensor Suhu DS18B20

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami cara kerja protokol 1-Wire secara mendalam (timing, ROM code, parasitic power)
- Menjelaskan perbedaan mendasar 1-Wire dengan SPI dan I2C
- Menggunakan library `OneWire` dan `DallasTemperature` untuk pembacaan suhu
- Melakukan operasi **ROM Discovery** untuk mendeteksi dan mengidentifikasi banyak sensor sekaligus di satu kabel
- Membangun sistem pemantauan suhu non-blocking yang akurat dan tangguh
- Memahami konsep *parasitic power mode* dan dampaknya terhadap presisi pengukuran

---

## 25.1 Apa itu Protokol 1-Wire?

**1-Wire** adalah protokol komunikasi serial yang dikembangkan oleh *Dallas Semiconductor* (sekarang bagian dari *Maxim Integrated*). Keistimewaan 1-Wire: **hanya membutuhkan SATU kabel data** untuk komunikasi dua arah — jauh lebih hemat kabel dibanding SPI (4 kabel) maupun I2C (2 kabel).

### Filosofi Desain 1-Wire

| Aspek | UART | I2C | SPI | **1-Wire** |
|-------|------|-----|-----|------------|
| Kabel data minimum | 2 | 2 | 4 | **1** ✅ |
| Clock | Asinkron | Sinkron (Master) | Sinkron (Master) | **Asinkron** (timing ketat) |
| Model komunikasi | Full-duplex | Half-duplex | Full-duplex | **Half-duplex** |
| Identifikasi perangkat | Tidak | Alamat 7-bit | Pin CS terpisah | **ROM Code 64-bit** |
| Jumlah perangkat | 2 | Hingga 127 | Banyak (1 CS/device) | **Hingga ratusan** di 1 pin! |
| Kecepatan | ~1 Mbps | 100K–3.4M bps | Hingga 80 Mbps | **~16 Kbps** |
| Jarak | Hingga 15m | <50 cm | <30 cm | **Hingga 100+ meter** ✅ |

> 💡 **Pilih 1-Wire bila:** Kamu membutuhkan banyak titik sensor suhu yang tersebar dalam satu gedung atau kebun, menggunakan satu kabel panjang!

---

## 25.2 Cara Kerja Protokol 1-Wire

### Anatomi Sinyal 1-Wire

Jalur data 1-Wire adalah **open-drain** — artinya, baik Master (ESP32) maupun Slave (sensor) hanya bisa *menarik* jalur ke LOW (GND). Jalur ke HIGH dilakukan oleh **resistor pull-up eksternal** (disediakan oleh Kit Bluino).

```
           Resistor Pull-up
ESP32 IO14 ──[4K7Ω]── VCC 3.3V
              │
              ├─────────────── DS18B20 sensor #1
              │
              ├─────────────── DS18B20 sensor #2
              │
              └─────────────── DS18B20 sensor #3 (dst.)
```

> ✅ **Kit Bluino sudah menyediakan resistor pull-up 4K7Ω** yang terpasang langsung di PCB pada jalur IO14. Kamu **tidak perlu** menambahkan resistor lagi!

### Tiga Jenis Operasi Sinyal

Protokol 1-Wire bekerja dengan **waktu (timing)** yang sangat ketat. Ada tiga operasi sinyal fundamental:

#### 1. Reset Pulse (Sinkronisasi Awal)
```
Master menahan LOW selama ≥480µs → Lepas → Slave merespons LOW ~60-240µs
───────────┐                           ┌────────────────────────────
           │          RESET            │  PRESENCE PULSE (Sensor ada!)
           └───────────────────────────┘──────│─────────────────────
           ← ≥480µs →                  ← 60µs → ← 60-240µs →
```

#### 2. Write Bit (Master mengirim data ke sensor)
```
Write "1": Master tarik LOW ~1µs, lepas → jalur HIGH 60µs
Write "0": Master tarik LOW 60µs, lepas
```

#### 3. Read Bit (Sensor mengirim data ke Master)
```
Master tarik LOW ~1µs → lepas → baca jalur setelah 15µs
Jika jalur HIGH: data = 1 | Jika jalur LOW: data = 0
```

> 📝 **Kabar baiknya:** Library `OneWire` dan `DallasTemperature` menangani semua timing mikrosecond ini secara otomatis. Kamu tidak perlu menghitung manual!

### ROM Code: Identitas Unik 64-Bit

Setiap sensor DS18B20 memiliki **ROM Code** (Registration Number) yang diprogram secara permanen di pabrik. Format ROM Code adalah 64 bit:

```
┌──────────────────────────────────────────────────────────────┐
│                ROM CODE DS18B20 — 64 bit                      │
├────────────────┬───────────────────────────────┬─────────────┤
│  8-bit Family  │  48-bit Nomor Seri Unik        │  8-bit CRC  │
│    Code        │  (Unik sedunia, tidak berulang)│  (Checksum) │
│    (0x28)      │  contoh: A1 B2 C3 D4 E5 F6     │   (0xXX)    │
└────────────────┴───────────────────────────────┴─────────────┘
```

- **Family Code `0x28`:** Penanda bahwa ini adalah IC DS18B20!
- **48-bit Serial:** Terjamin unik di antara miliaran sensor yang diproduksi
- **8-bit CRC:** Checksum untuk memverifikasi integritas data ROM

---

## 25.3 DS18B20: Sensor Suhu Presisi 1-Wire

### Spesifikasi Teknis DS18B20

```
┌──────────────────────────────────────────────────────────┐
│              Spesifikasi DS18B20 (Maxim Integrated)       │
├──────────────────────────────┬───────────────────────────┤
│ Rentang pengukuran           │ -55°C hingga +125°C        │
│ Akurasi (−10°C hingga +85°C) │ ±0.5°C                    │
│ Resolusi (dapat dikonfigurasi)│ 9, 10, 11, atau 12 bit    │
│ Antarmuka komunikasi         │ 1-Wire (1 kabel data)      │
│ Tegangan operasi             │ 3.0V – 5.5V                │
│ Koneksi kabel data           │ IO14 (Kit Bluino)          │
│ Pull-up resistor             │ 4K7Ω (sudah di PCB!)       │
└──────────────────────────────┴───────────────────────────┘
```

### Resolusi dan Waktu Konversi

Resolusi pengukuran DS18B20 **bisa dikonfigurasi**! Ini adalah fitur unik yang tidak dimiliki sensor DHT11:

| Resolusi | Bit Desimal | Akurasi | Waktu Konversi (maks) |
|----------|-------------|---------|----------------------|
| 9-bit | 0.5°C | ±0.5°C | **94 ms** |
| 10-bit | 0.25°C | ±0.5°C | **188 ms** |
| 11-bit | 0.125°C | ±0.5°C | **375 ms** |
| **12-bit** | **0.0625°C** | **±0.5°C** | **750 ms** (default) |

> ⚠️ **Waktu Konversi adalah Waktu WAJIB!** Setelah `requestTemperatures()` dipanggil, sensor membutuhkan waktu hingga **750 milidetik** (untuk resolusi *default* 12-bit) untuk menyelesaikan proses analog-ke-digital. Membaca suhu sebelum waktu ini habis akan menghasilkan nilai **tidak valid**!

### Mode Catu Daya: External vs Parasitic

DS18B20 mendukung dua mode catu daya:

```
Mode NORMAL (External Power) — Kit Bluino menggunakan ini ✅
  VCC ──── VDD sensor (pin 3)
  GND ──── GND sensor (pin 1)
  IO14 ─── DQ sensor  (pin 2)

Mode PARASITIK (2-wire, tanpa kabel VCC)
  GND ──── GND sensor (pin 1)
  IO14 ─── DQ sensor  (pin 2)
            DQ juga terhubung ke VDD sensor (pin 3)!
```

> ⚠️ **Kit Bluino menggunakan Mode Normal (3-wire).** Kamu tidak perlu khawatir tentang parasitic power mode, tetapi sangat penting memahaminya agar kamu tidak salah saat merakit sensor tambahan di luar Kit.

---

## 25.4 Instalasi Library

Sebelum menulis kode, pastikan dua library ini sudah terpasang di Arduino IDE:

```
Library Manager (Ctrl+Shift+I) → Ketik "DallasTemperature"

Instal: DallasTemperature by Miles Burton (v3.9.x atau lebih baru)
        → Library ini akan otomatis mengunduh "OneWire" sebagai dependensi!
```

> 💡 **Satu instalasi, dua library:** Library `DallasTemperature` membutuhkan `OneWire`, sehingga Arduino IDE akan otomatis menginstal keduanya sekaligus.

```
┌─────────────────────────────────────────────────────────────┐
│ Library yang dibutuhkan:                                     │
│                                                             │
│  #include <OneWire.h>          ← Protokol 1-Wire low-level  │
│  #include <DallasTemperature.h> ← Abstraksi DS18B20         │
│                                                             │
│ DallasTemperature BERGANTUNG pada OneWire.h                 │
│ → Keduanya harus terinstal!                                 │
└─────────────────────────────────────────────────────────────┘
```

---

## 25.5 Program 1: Baca Suhu Pertama Kali (Blocking)

Program paling sederhana untuk memverifikasi sensor DS18B20 berfungsi. Program ini menggunakan metode **blocking** (`requestTemperatures()` berhenti sejenak menunggu konversi) — cocok untuk belajar dasar, tapi tidak disarankan untuk proyek IoT yang membutuhkan respons cepat.

```cpp
/*
 * BAB 25 - Program 1: Baca Suhu DS18B20 (Blocking)
 * Membaca suhu dari sensor DS18B20 setiap 2 detik.
 *
 * Wiring (Kit Bluino — sudah terhubung di PCB):
 *   DQ (Data)    → IO14
 *   VCC          → 3.3V (di PCB)
 *   GND          → GND  (di PCB)
 *   Pull-up 4K7Ω → sudah ada di PCB!
 *
 * Library yang dibutuhkan:
 *   - OneWire (by Paul Stoffregen)
 *   - DallasTemperature (by Miles Burton)
 */

#include <OneWire.h>
#include <DallasTemperature.h>

// ── Definisi Pin ────────────────────────────────────────────────
#define ONE_WIRE_PIN 14   // Jalur data DS18B20 di Kit Bluino

// ── Inisialisasi Objek ──────────────────────────────────────────
OneWire        oneWire(ONE_WIRE_PIN); // Bus 1-Wire di IO14
DallasTemperature sensor(&oneWire);   // Library DS18B20 menggunakan bus itu

void setup() {
  Serial.begin(115200);

  Serial.println("══════════════════════════════════════════");
  Serial.println("   BAB 25: DS18B20 — Baca Suhu Dasar");
  Serial.println("══════════════════════════════════════════");

  // Mulai komunikasi sensor
  sensor.begin();

  // Verifikasi sensor terdeteksi
  uint8_t jumlahSensor = sensor.getDeviceCount();
  Serial.printf("Sensor DS18B20 terdeteksi: %u buah\n", jumlahSensor);

  if (jumlahSensor == 0) {
    Serial.println("❌ Tidak ada sensor! Periksa koneksi fisik.");
    Serial.println("   Kemungkinan penyebab:");
    Serial.println("   1. Sensor DS18B20 tidak menancap sempurna di soket/header");
    Serial.println("   2. Pin ONE_WIRE_PIN tidak sesuai (seharusnya IO14)");
    Serial.println("   3. Sensor rusak atau terbalik memasang VCC/GND");
    return;
  }

  Serial.println("✅ Sensor siap. Mulai membaca suhu...");
  Serial.println("──────────────────────────────────────────");
}

void loop() {
  // Perintahkan SEMUA sensor untuk mulai konversi ADC
  // Fungsi ini akan MENUNGGU (blocking) hingga konversi selesai (~750ms)
  sensor.requestTemperatures();

  // Baca suhu dari sensor pertama (indeks 0) dalam satuan Celsius
  float suhuC = sensor.getTempCByIndex(0);

  // Validasi: DS18B20 mengembalikan -127.0 jika ada error komunikasi
  if (suhuC == DEVICE_DISCONNECTED_C) {
    Serial.println("❌ Error: Sensor terputus atau gagal membaca!");
  } else {
    float suhuF = sensor.toFahrenheit(suhuC);
    Serial.printf("🌡️  Suhu: %.2f °C  (%.2f °F)\n", suhuC, suhuF);
  }

  delay(2000); // Jeda 2 detik antar pembacaan
}
```

**Contoh output yang diharapkan:**
```
══════════════════════════════════════════
   BAB 25: DS18B20 — Baca Suhu Dasar
══════════════════════════════════════════
Sensor DS18B20 terdeteksi: 1 buah
✅ Sensor siap. Mulai membaca suhu..
──────────────────────────────────────────
🌡️  Suhu: 28.31 °C  (82.96 °F)
🌡️  Suhu: 28.44 °C  (83.19 °F)
🌡️  Suhu: 28.56 °C  (83.41 °F)
```

> ⚠️ **Mengapa ada jeda ~750ms setiap pembacaan?** Karena `requestTemperatures()` secara default **menunggu (blocking)** hingga sensor selesai konversi. Ini memakan ~750ms. Di Program 3, kita akan mempelajari cara menghilangkan hambatan ini!

---

## 25.6 Memahami ROM Code Sensor

Setiap sensor DS18B20 memiliki identitas unik 64-bit (ROM Code). Mengetahui ROM Code sangat penting bila kamu memakai **lebih dari satu sensor** di satu bus.

```cpp
/*
 * BAB 25 - Program 2: Deteksi dan Tampilkan ROM Code Sensor
 * Seperti melakukan "WHO_AM_I" request ke semua sensor 1-Wire.
 *
 * Wiring (Kit Bluino): DQ → IO14
 */

#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_PIN 14

OneWire        oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

// ── Cetakan ROM Code dalam format Hex ────────────────────────────
void cetakRomCode(DeviceAddress addr) {
  for (uint8_t i = 0; i < 8; i++) {
    if (addr[i] < 0x10) Serial.print("0"); // Padding nol di depan
    Serial.print(addr[i], HEX);
    if (i < 7) Serial.print(":");
  }
}

void setup() {
  Serial.begin(115200);

  Serial.println("══════════════════════════════════════════");
  Serial.println("   BAB 25: Deteksi ROM Code DS18B20");
  Serial.println("══════════════════════════════════════════");

  sensors.begin();

  uint8_t jumlah = sensors.getDeviceCount();
  Serial.printf("Ditemukan %u sensor di bus 1-Wire (IO%d).\n\n",
                jumlah, ONE_WIRE_PIN);

  DeviceAddress alamat;

  for (uint8_t i = 0; i < jumlah; i++) {
    if (!sensors.getAddress(alamat, i)) {
      Serial.printf("  Sensor #%u: ❌ Gagal baca ROM Code\n", i);
      continue;
    }

    Serial.printf("  Sensor #%u:\n", i);
    Serial.print("    ROM Code : ");
    cetakRomCode(alamat);
    Serial.println();

    // Tampilkan Family Code (byte pertama)
    Serial.printf("    Family   : 0x%02X ", alamat[0]);
    if (alamat[0] == 0x28) {
      Serial.println("→ DS18B20 ✅");
    } else if (alamat[0] == 0x10) {
      Serial.println("→ DS18S20/DS1820");
    } else {
      Serial.println("→ Tidak dikenal");
    }

    // Tampilkan resolusi saat ini
    uint8_t res = sensors.getResolution(alamat);
    Serial.printf("    Resolusi : %u-bit (akurasi %.4f°C)\n",
                  res, (res == 9)  ? 0.5f :
                       (res == 10) ? 0.25f :
                       (res == 11) ? 0.125f : 0.0625f);
    Serial.println();
  }

  Serial.println("══════════════════════════════════════════");
  Serial.println("Tempelkan kode ROM di atas ke array kode");
  Serial.println("Anda untuk pengalamatan sensor langsung!");
  Serial.println("══════════════════════════════════════════");
}

void loop() {
  // ROM Code hanya perlu dibaca sekali
}
```

**Contoh output dengan 1 sensor:**
```
══════════════════════════════════════════
   BAB 25: Deteksi ROM Code DS18B20
══════════════════════════════════════════
Ditemukan 1 sensor di bus 1-Wire (IO14).

  Sensor #0:
    ROM Code : 28:FF:A1:B2:C3:D4:E5:F6
    Family   : 0x28 → DS18B20 ✅
    Resolusi : 12-bit (akurasi 0.0625°C)

══════════════════════════════════════════
Tempelkan kode ROM di atas ke array kode
Anda untuk pengalamatan sensor langsung!
══════════════════════════════════════════
```

---

## 25.7 Program 3: Pembacaan Non-Blocking (Pola yang Benar!)

Inilah inti dari bab ini dari sudut pandang teknik IoT. Fungsi `requestTemperatures()` bisa dikonfigurasi untuk **tidak menunggu** konversi selesai (`setWaitForConversion(false)`). Dengan demikian, kita dapat menggunakan `millis()` untuk memastikan program tidak pernah berhenti beroperasi.

```
Waktu Konversi Berdasarkan Resolusi:
  9-bit  →  94 ms
  10-bit → 188 ms
  11-bit → 375 ms
  12-bit → 750 ms  ← Default DS18B20
```

```cpp
/*
 * BAB 25 - Program 3: Pembacaan Suhu Non-Blocking (Pola Industri)
 *
 * Teknik: setWaitForConversion(false) + millis() timer
 * → Program tidak akan berhenti sama sekali saat menunggu konversi ADC
 *
 * Wiring (Kit Bluino): DQ → IO14
 */

#include <OneWire.h>
#include <DallasTemperature.h>

// ── Konfigurasi Sensor ──────────────────────────────────────────
#define ONE_WIRE_PIN       14
#define RESOLUSI_BIT       12               // 9, 10, 11, atau 12
#define WAKTU_KONVERSI_MS  750UL            // Harus sesuai resolusi!
// Referensi: 9-bit=94ms, 10-bit=188ms, 11-bit=375ms, 12-bit=750ms

OneWire        oneWire(ONE_WIRE_PIN);
DallasTemperature sensor(&oneWire);

// ── State Machine Pembacaan Non-Blocking ────────────────────────
// Tiga kemungkinan state:
//   IDLE       → Menunggu interval baca berikutnya
//   CONVERTING → Permintaan konversi sudah dikirim, menunggu selesai
typedef enum { STATE_IDLE, STATE_CONVERTING } SensorState;

SensorState    state        = STATE_IDLE;
unsigned long  tMulaiKonversi = 0;
unsigned long  tBacaTerakhir  = 0;

#define INTERVAL_BACA_MS  5000UL  // Baca suhu setiap 5 detik

bool           sensorSiap    = false;
float          suhuTerakhir  = NAN; // NAN = belum ada data valid

void setup() {
  Serial.begin(115200);

  Serial.println("══════════════════════════════════════════");
  Serial.println("   BAB 25: DS18B20 Non-Blocking");
  Serial.println("══════════════════════════════════════════");

  sensor.begin();

  // ── Konfigurasi KRITIS: Non-Blocking Mode ──────────────────────
  // Tanpa baris ini, requestTemperatures() akan BLOCKING 750ms!
  sensor.setWaitForConversion(false);

  // Atur resolusi untuk semua sensor di bus
  sensor.setResolution(RESOLUSI_BIT);

  uint8_t jumlah = sensor.getDeviceCount();
  if (jumlah == 0) {
    Serial.println("❌ Tidak ada sensor DS18B20 terdeteksi!");
    return;
  }

  Serial.printf("✅ %u sensor terdeteksi. Resolusi: %u-bit.\n",
                jumlah, RESOLUSI_BIT);
  Serial.printf("   Interval baca: setiap %lu detik.\n",
                INTERVAL_BACA_MS / 1000UL);
  Serial.println("──────────────────────────────────────────");

  sensorSiap = true;
}

void loop() {
  if (!sensorSiap) return;

  unsigned long sekarang = millis();

  switch (state) {
    // ── STATE 1: IDLE — Tunggu interval, lalu kirim permintaan ────
    case STATE_IDLE:
      if (sekarang - tBacaTerakhir >= INTERVAL_BACA_MS) {
        // Kirim sinyal "mulai konversi" ke sensor (TIDAK blocking!)
        sensor.requestTemperatures();
        tMulaiKonversi = sekarang;
        state = STATE_CONVERTING;
      }
      break;

    // ── STATE 2: CONVERTING — Tunggu konversi selesai ─────────────
    case STATE_CONVERTING:
      // Hanya baca setelah waktu konversi cukup
      if (sekarang - tMulaiKonversi >= WAKTU_KONVERSI_MS) {
        float suhuC = sensor.getTempCByIndex(0);

        if (suhuC == DEVICE_DISCONNECTED_C) {
          Serial.printf("[%8lu ms] ❌ Error: Sensor terputus!\n", sekarang);
        } else {
          suhuTerakhir = suhuC;
          Serial.printf("[%8lu ms] 🌡️  Suhu: %.4f °C  (%.4f °F)\n",
                        sekarang, suhuC, sensor.toFahrenheit(suhuC));
        }

        tBacaTerakhir = sekarang;
        state = STATE_IDLE;
      }
      break;
  }

  // ──────────────────────────────────────────────────────────────
  // Kerjaan lain bisa berjalan di sini tanpa pernah terblokir!
  // Contoh: cek tombol, update OLED, kirim data WiFi, dll.
  // ──────────────────────────────────────────────────────────────
}
```

**Contoh output:**
```
══════════════════════════════════════════
   BAB 25: DS18B20 Non-Blocking
══════════════════════════════════════════
✅ 1 sensor terdeteksi. Resolusi: 12-bit.
   Interval baca: setiap 5 detik.
──────────────────────────────────────────
[    5751 ms] 🌡️  Suhu: 28.3125 °C  (82.9625 °F)
[   11501 ms] 🌡️  Suhu: 28.3750 °C  (83.0750 °F)
[   17252 ms] 🌡️  Suhu: 28.4375 °C  (83.1875 °F)
```

> 💡 **Perhatikan: Suhu menampilkan 4 desimal!** Ini adalah akurasi 12-bit sejati (0.0625°C per step). Jika kamu hanya butuh 2 desimal, ubah format `%.4f` menjadi `%.2f`.

---

## 25.8 Program 4: Multi-Sensor (Dua Sensor di Satu Kabel)

Keunggulan 1-Wire paling mencolok: **banyak sensor dalam satu pin**. Program ini mendemonstrasikan cara membaca dua (atau lebih) sensor DS18B20 menggunakan ROM Code unik masing-masing.

```cpp
/*
 * BAB 25 - Program 4: Multi-Sensor DS18B20 dengan ROM Code
 *
 * PERSIAPAN:
 *   1. Jalankan Program 2 terlebih dahulu untuk mendapatkan ROM Code
 *      dari setiap sensor yang akan kamu pasang.
 *   2. Isi array ROM_SENSOR_1 dan ROM_SENSOR_2 dengan nilai tersebut.
 *
 * Wiring (Kit Bluino):
 *   Semua sensor: DQ → IO14 (paralel di satu jalur)
 *   Setiap sensor: VCC → 3V3, GND → GND
 */

#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_PIN 14
#define RESOLUSI_BIT 12

OneWire        oneWire(ONE_WIRE_PIN);
DallasTemperature sensors(&oneWire);

// ── ROM Code: Ganti dengan nilai dari Program 2! ──────────────────
// Format: 8 byte dalam urutan yang ditampilkan Program 2
DeviceAddress ROM_SENSOR_1 = { 0x28, 0xFF, 0xA1, 0xB2, 0xC3, 0xD4, 0xE5, 0xF6 };
DeviceAddress ROM_SENSOR_2 = { 0x28, 0xFF, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66 };

// ── Nama deskriptif untuk setiap sensor ───────────────────────────
const char* NAMA_SENSOR_1 = "Suhu Ruangan";
const char* NAMA_SENSOR_2 = "Suhu Luar";

#define INTERVAL_BACA_MS   5000UL
#define WAKTU_KONVERSI_MS  750UL

typedef enum { STATE_IDLE, STATE_CONVERTING } SensorState;
SensorState   state          = STATE_IDLE;
unsigned long tMulaiKonversi = 0;
unsigned long tBacaTerakhir  = 0;

// ── Helper: Cetak ROM Code ────────────────────────────────────────
void cetakRomCode(DeviceAddress addr) {
  for (uint8_t i = 0; i < 8; i++) {
    if (addr[i] < 0x10) Serial.print("0");
    Serial.print(addr[i], HEX);
    if (i < 7) Serial.print(":");
  }
}

// ── Helper: Baca dan cetak suhu satu sensor ───────────────────────
void tampilSuhu(const char* nama, DeviceAddress addr) {
  float suhuC = sensors.getTempC(addr);  // Baca via ROM Code spesifik!

  if (suhuC == DEVICE_DISCONNECTED_C) {
    Serial.printf("  %-15s : ❌ Terputus / tidak terdeteksi\n", nama);
  } else {
    Serial.printf("  %-15s : %6.2f °C  (%6.2f °F)\n",
                  nama, suhuC, sensors.toFahrenheit(suhuC));
  }
}

void setup() {
  Serial.begin(115200);

  Serial.println("══════════════════════════════════════════");
  Serial.println("   BAB 25: Multi-Sensor DS18B20");
  Serial.println("══════════════════════════════════════════");

  sensors.begin();
  sensors.setWaitForConversion(false); // Non-blocking!
  sensors.setResolution(RESOLUSI_BIT);

  // Verifikasi kedua ROM Code dapat ditemukan di bus
  uint8_t total = sensors.getDeviceCount();
  Serial.printf("Sensor terdeteksi di bus : %u buah\n\n", total);

  Serial.printf("ROM Sensor 1 (%s): ", NAMA_SENSOR_1);
  cetakRomCode(ROM_SENSOR_1);
  if (sensors.isConnected(ROM_SENSOR_1)) {
    Serial.println(" ✅ Terhubung");
  } else {
    Serial.println(" ❌ TIDAK DITEMUKAN! (Lupa ganti ROM Code?)");
  }

  Serial.printf("ROM Sensor 2 (%s)  : ", NAMA_SENSOR_2);
  cetakRomCode(ROM_SENSOR_2);
  if (sensors.isConnected(ROM_SENSOR_2)) {
    Serial.println(" ✅ Terhubung");
  } else {
    Serial.println(" ❌ TIDAK DITEMUKAN! (Lupa ganti ROM Code?)");
  }

  Serial.println("\n──────────────────────────────────────────");
}

void loop() {
  unsigned long sekarang = millis();

  switch (state) {
    case STATE_IDLE:
      if (sekarang - tBacaTerakhir >= INTERVAL_BACA_MS) {
        sensors.requestTemperatures(); // Perintah ke SEMUA sensor sekaligus!
        tMulaiKonversi = sekarang;
        state = STATE_CONVERTING;
      }
      break;

    case STATE_CONVERTING:
      if (sekarang - tMulaiKonversi >= WAKTU_KONVERSI_MS) {
        Serial.printf("[%8lu ms]\n", sekarang);
        tampilSuhu(NAMA_SENSOR_1, ROM_SENSOR_1);
        tampilSuhu(NAMA_SENSOR_2, ROM_SENSOR_2);
        Serial.println();

        tBacaTerakhir = sekarang;
        state = STATE_IDLE;
      }
      break;
  }
}
```

**Contoh output:**
```
══════════════════════════════════════════
   BAB 25: Multi-Sensor DS18B20
══════════════════════════════════════════
Sensor terdeteksi di bus : 2 buah

ROM Sensor 1 (Suhu Ruangan): 28:FF:A1:B2:C3:D4:E5:F6 ✅ Terhubung
ROM Sensor 2 (Suhu Luar)  : 28:FF:11:22:33:44:55:66 ✅ Terhubung

──────────────────────────────────────────
[    5751 ms]
  Suhu Ruangan  :  28.31 °C  ( 82.96 °F)
  Suhu Luar     :  34.56 °C  ( 94.21 °F)

[   11501 ms]
  Suhu Ruangan  :  28.25 °C  ( 82.85 °F)
  Suhu Luar     :  34.62 °C  ( 94.32 °F)
```

---

## 25.9 Mengkonfigurasi Resolusi Sensor

```cpp
/*
 * BAB 25 - Program 5: Konfigurasi Resolusi dan Perbandingan Akurasi
 * Mendemonstrasikan perbedaan nilai suhu pada resolusi berbeda.
 */

#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_PIN 14

OneWire        oneWire(ONE_WIRE_PIN);
DallasTemperature sensor(&oneWire);

// Waktu konversi (ms) berdasarkan resolusi
uint16_t waktuKonversi[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 94, 188, 375, 750 };
// Indeks: [9]=94ms, [10]=188ms, [11]=375ms, [12]=750ms

void bacaPerResolusi(uint8_t resolusi) {
  sensor.setResolution(resolusi);

  // Kirim permintaan konversi (blocking untuk demo ini)
  sensor.setWaitForConversion(true);
  sensor.requestTemperatures();

  float suhuC = sensor.getTempCByIndex(0);

  if (suhuC == DEVICE_DISCONNECTED_C) {
    Serial.printf("  Resolusi %2u-bit: ❌ Error sensor\n", resolusi);
    return;
  }

  Serial.printf("  Resolusi %2u-bit: %8.4f °C  (%5u ms) step=%.4f°C\n",
                resolusi, suhuC,
                waktuKonversi[resolusi],
                (resolusi == 9)  ? 0.5f :
                (resolusi == 10) ? 0.25f :
                (resolusi == 11) ? 0.125f : 0.0625f);
}

void setup() {
  Serial.begin(115200);

  Serial.println("══════════════════════════════════════════════════");
  Serial.println("   BAB 25: Perbandingan Resolusi DS18B20");
  Serial.println("══════════════════════════════════════════════════");

  sensor.begin();

  if (sensor.getDeviceCount() == 0) {
    Serial.println("❌ Tidak ada sensor ditemukan!");
    return;
  }

  Serial.println("Membandingkan hasil pengukuran pada tiap resolusi:\n");

  bacaPerResolusi(9);
  bacaPerResolusi(10);
  bacaPerResolusi(11);
  bacaPerResolusi(12);

  Serial.println("\n──────────────────────────────────────────────────");
  Serial.println("Kesimpulan:");
  Serial.println("  Resolusi lebih tinggi → Nilai lebih presisi");
  Serial.println("  Resolusi lebih rendah → Konversi lebih cepat");
  Serial.println("  Untuk IoT: Gunakan 12-bit kecuali kecepatan kritis!");
  Serial.println("══════════════════════════════════════════════════");
}

void loop() {}
```

**Contoh output:**
```
══════════════════════════════════════════════════
   BAB 25: Perbandingan Resolusi DS18B20
══════════════════════════════════════════════════
Membandingkan hasil pengukuran pada tiap resolusi:

  Resolusi  9-bit:  28.5000 °C  (  94 ms) step=0.5000°C
  Resolusi 10-bit:  28.2500 °C  ( 188 ms) step=0.2500°C
  Resolusi 11-bit:  28.3750 °C  ( 375 ms) step=0.1250°C
  Resolusi 12-bit:  28.3125 °C  ( 750 ms) step=0.0625°C

──────────────────────────────────────────────────
Kesimpulan:
  Resolusi lebih tinggi → Nilai lebih presisi
  Resolusi lebih rendah → Konversi lebih cepat
  Untuk IoT: Gunakan 12-bit kecuali kecepatan kritis!
══════════════════════════════════════════════════
```

---

## 25.10 Tips & Troubleshooting

### 1. Sensor Tidak Terdeteksi (`getDeviceCount()` = 0)?

```
✅ Cek 1: Pastikan library OneWire dan DallasTemperature terinstal
   Tools → Manage Libraries → cari "DallasTemperature"

✅ Cek 2: Verifikasi pin ONE_WIRE_PIN = 14 (bukan pin lain)

✅ Cek 3: Kit Bluino sudah memiliki pull-up 4K7Ω di IO14
   Jika sensor LUAR KIT: gunakan pull-up 4K7Ω ke 3.3V!

✅ Cek 4: VCC sensor harus 3.0V-5.5V (Kit Bluino memberi 3.3V ✅)

✅ Cek 5: Pastikan kabel DQ (data) tidak longgar
```

### 2. Nilai Suhu Selalu -127.0°C?

```
❌ -127°C = DEVICE_DISCONNECTED_C = sensor tidak bisa berkomunikasi

Penyebab paling umum:
  - requestTemperatures() belum dipanggil sebelum getTempCByIndex()
  - Waktu konversi belum cukup (solusi: Non-Blocking atau delay yang benar)
  - Sensor terputus saat sedang membaca

✅ Selalu periksa: if (suhuC == DEVICE_DISCONNECTED_C) { ... }
```

### 3. Nilai Suhu Selalu 85.0°C?

```
⚠️  85°C = Power-On Reset Value DS18B20

Artinya: Kamu membaca nilai SEBELUM konversi pertama selesai!

Solusi untuk Blocking Mode:
  requestTemperatures();    // Minta konversi
  delay(WAKTU_KONVERSI_MS); // TUNGGU konversi selesai!
  float suhu = getTempCByIndex(0);

Solusi untuk Non-Blocking Mode:
  Pastikan WAKTU_KONVERSI_MS = 750 (untuk resolusi 12-bit)
```

### 4. Sensor Nilai Tidak Akurat (Fluktuatif)?

```
✅ Cek 1: Kecepatan Serial.begin() — gunakan 115200 bps
✅ Cek 2: Panjang kabel — 1-Wire bisa hingga 100m, tapi:
   - Kabel panjang → tambahkan kapasitor 100nF antara DQ dan GND
   - Kabel >30m → pertimbangkan pull-up lebih rendah (2.2KΩ)
✅ Cek 3: Mode catu daya — External Power (normal) lebih stabil
          dari Parasitic Power, terutama saat konversi ADC
```

### 5. Perbandingan `getTempCByIndex()` vs `getTempC()`

| Fungsi | Digunakan Untuk | Keuntungan | Risiko |
|--------|----------------|------------|--------|
| `getTempCByIndex(0)` | Sensor tunggal | Kode lebih sederhana | Urutan bergantung bus discovery |
| `getTempC(addr)` | Multi-sensor | Identifikasi pasti via ROM Code | Harus catat ROM Code terlebih dahulu |

> ✅ **Best Practice:** Untuk proyek dengan 1 sensor, `getTempCByIndex(0)` cukup. Untuk 2+ sensor, **selalu** gunakan `getTempC(addr)` dengan ROM Code spesifik!

---

## 25.11 Ringkasan

```
┌──────────────────────────────────────────────────────────┐
│          RINGKASAN BAB 25 — 1-Wire & DS18B20             │
├──────────────────────────────────────────────────────────┤
│ 1-Wire: 1 kabel data, banyak sensor, jarak hingga 100m   │
│                                                          │
│ DS18B20 di Kit Bluino:                                   │
│   Pin Data         = IO14                                │
│   Pull-up 4K7Ω     = sudah ada di PCB!                  │
│   Tegangan         = 3.3V                               │
│   Mode daya        = External Power (Normal)             │
│                                                          │
│ Inisialisasi:                                            │
│   OneWire oneWire(14);                                   │
│   DallasTemperature sensor(&oneWire);                    │
│   sensor.begin();                                        │
│   sensor.setWaitForConversion(false); ← Non-Blocking!   │
│                                                          │
│ Sequence Non-Blocking:                                   │
│   1. sensor.requestTemperatures();                       │
│   2. Tunggu WAKTU_KONVERSI_MS (millis)                   │
│   3. sensor.getTempCByIndex(0);                          │
│                                                          │
│ Kode Error Suhu:                                         │
│   -127°C = DEVICE_DISCONNECTED_C → sensor terputus      │
│    +85°C = Power-On Reset Value  → belum konversi!       │
│                                                          │
│ Pantangan:                                               │
│   ❌ Baca suhu TANPA requestTemperatures() dulu          │
│   ❌ Baca suhu sebelum WAKTU_KONVERSI_MS berlalu         │
│   ❌ Lupa periksa DEVICE_DISCONNECTED_C                  │
│   ❌ Gunakan getTempCByIndex() untuk multi-sensor        │
└──────────────────────────────────────────────────────────┘
```

---

## 25.12 Latihan

1. **Termometer Digital Real-Time**: Modifikasi Program 3 agar suhu juga ditampilkan di **OLED Display** (kombinasikan dengan BAB 12 atau BAB 23). Tampilkan suhu dalam format besar di tengah layar dan update setiap 2 detik.

2. **Alarm Suhu**: Buat program yang menyalakan **Active Buzzer** selama 3 detik ketika suhu melebihi 35°C, dan mematikannya kembali setelah suhu turun di bawah 33°C. Implementasikan dengan pola non-blocking penuh (tanpa `delay()` apapun).

3. **Data Logger Suhu ke SD Card**: Kombinasikan BAB 24 (SPI/SD Card) dengan BAB 25 ini. Catat data `uptime_ms,suhu_C` ke file `/suhu.csv` setiap 30 detik. Ini mensimulasikan sistem monitoring suhu ruang server nyata!

4. **Kalibrasi Sensor**: Sering kali terdapat selisih kecil antara nilai sensor dan termometer referensi. Buat program yang menerima nilai **offset kalibrasi** melalui Serial Monitor (ketik `+0.5` atau `-0.3`) dan menambahkan nilai tersebut ke setiap pembacaan secara otomatis.

5. **Pengujian Multi-Sensor**: Jika kamu memiliki lebih dari satu sensor DS18B20, hubungkan semuanya ke IO14, jalankan Program 2 untuk mendapat ROM Code masing-masing, lalu buat program menggunakan Program 4 dengan nama deskriptif yang berbeda per sensor.

---

*Selanjutnya → [BAB 26: DHT11 / DHT22 — Sensor Suhu dan Kelembaban](../BAB-26/README.md)*
