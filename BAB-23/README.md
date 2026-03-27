# BAB 23: I2C — Inter-Integrated Circuit

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami prinsip kerja protokol I2C secara mendalam (bus, address, ACK/NACK)
- Menggunakan library `Wire.h` pada ESP32 untuk membaca dan menulis ke perangkat I2C
- Menemukan alamat I2C perangkat yang terhubung menggunakan I2C Scanner
- Membaca data dari OLED, MPU6050, dan BMP180 yang terpasang di Kit Bluino
- Membangun sistem multi-sensor di atas satu bus I2C bersama

---

## 23.1 Apa itu I2C?

**I2C** (Inter-Integrated Circuit, dibaca "I squared C" atau "I two C") adalah protokol komunikasi serial bus yang dirancang oleh **Philips Semiconductor** pada tahun 1982. Keunggulan utamanya: menghubungkan **banyak perangkat hanya dengan 2 kabel**.

### Perbandingan Protokol Komunikasi

```
┌────────────────┬──────────┬────────────┬───────────────┐
│ Protokol       │ Kabel    │ Jml Device │ Kecepatan     │
├────────────────┼──────────┼────────────┼───────────────┤
│ UART           │ 2 (TX,RX)│ 2 saja     │ hingga 1 Mbps │
│ I2C            │ 2 (SDA,  │ Hingga 127 │ 100K–3.4Mbps  │
│                │  SCL)    │ perangkat  │               │
│ SPI            │ 4+ (MOSI,│ Banyak     │ Hingga 80Mbps │
│                │MISO,CLK, │ (CS/device)│               │
│                │ CS)      │            │               │
│ 1-Wire         │ 1        │ Banyak     │ ~16 Kbps      │
└────────────────┴──────────┴────────────┴───────────────┘
```

### Dua Jalur I2C

| Pin | Nama | Fungsi |
|-----|------|--------|
| **SDA** | Serial Data | Jalur data dua arah (bidirectional) |
| **SCL** | Serial Clock | Jalur clock — selalu dikendalikan oleh Master |

> 💡 Kedua jalur **WAJIB** diberi resistor **pull-up** ke VCC (biasanya 4.7kΩ). Tanpa pull-up, komunikasi tidak akan berfungsi! Pada Kit Bluino, resistor pull-up sudah **terpasang di PCB** — tidak perlu menambah komponen apapun.

---

## 23.2 Cara Kerja I2C

### Arsitektur Master-Slave

Dalam I2C, ada satu atau lebih **Master** yang **menginisiasi komunikasi**, dan satu atau lebih **Slave** yang **merespons**.

```
                    VCC (3.3V)
                    │    │
                   4.7K 4.7K   ← Pull-up resistors (sudah di PCB Bluino)
                    │    │
Master (ESP32) ─────┼────┼──── SDA ──── [OLED 0x3C] ─── [MPU6050 0x68] ─── [BMP180 0x77]
               ─────┼────┘──── SCL ──── [OLED 0x3C] ─── [MPU6050 0x68] ─── [BMP180 0x77]
                    
Semua perangkat berbagi 2 kawat yang sama!
```

### Alamat I2C (7-bit Address)

Setiap perangkat I2C punya **alamat unik 7-bit** (0x00–0x7F = 128 kemungkinan). ESP32 mengirim alamat ini di awal setiap transaksi untuk "memanggil" perangkat yang dituju.

```
Perangkat di Kit Bluino:
┌─────────────────────────────┬──────────────┬──────────┐
│ Perangkat                   │ Alamat Hex   │ Alamat   │
│                             │              │ Desimal  │
├─────────────────────────────┼──────────────┼──────────┤
│ OLED 0.96" SSD1306          │ 0x3C         │ 60       │
│ MPU6050 (Accel/Gyro)        │ 0x68         │ 104      │
│ BMP180 (Tekanan/Suhu)       │ 0x77         │ 119      │
└─────────────────────────────┴──────────────┴──────────┘
```

> 📝 **Catatan:** Beberapa perangkat I2C memiliki pin `ADDR` atau `AD0` yang memungkinkan pergantian alamat. Misalnya, MPU6050 bisa diubah ke `0x69` dengan menghubungkan pin `AD0` ke VCC.

### Struktur Transaksi I2C

Setiap kali ESP32 berkomunikasi dengan perangkat I2C, urutan sinyal mengikuti pola berikut:

```
START → [7-bit Address] + [R/W bit] → ACK → [Data bytes] → ACK → ... → STOP

• START  : Sinyal awal (SDA turun saat SCL HIGH)
• Address: 7-bit ID perangkat yang dipanggil
• R/W    : Bit ke-8 — 0=Write (kirim), 1=Read (terima)
• ACK    : Slave menjawab "siap" dengan menarik SDA ke LOW
• NACK   : Slave tidak merespons, kemungkinan alamat salah atau perangkat error
• STOP   : Sinyal akhir (SDA naik saat SCL HIGH)
```

### Mode Kecepatan I2C

| Mode | Kecepatan | Penggunaan |
|------|-----------|------------|
| Standard Mode | 100 kHz | Sensor umum, OLED |
| Fast Mode | 400 kHz | Sensor presisi |
| Fast Mode Plus | 1 MHz | Aplikasi khusus |
| High Speed Mode | 3.4 MHz | Jarang dipakai di embedded |

> 🔧 **Kit Bluino:** Semua sensor I2C bawaan kit mendukung hingga **400 kHz** (Fast Mode). Default `Wire.begin()` berjalan di **100 kHz** — sudah aman untuk semua perangkat.

---

## 23.3 I2C di ESP32 (Library Wire)

### Pin I2C Default ESP32

```
┌─────────────────────────────────────────────────────────┐
│              I2C di Kit Bluino ESP32                     │
├──────────────────────────────────┬──────────────────────┤
│ Fungsi                           │ Pin ESP32             │
├──────────────────────────────────┼──────────────────────┤
│ SDA (Serial Data)                │ IO21                  │
│ SCL (Serial Clock)               │ IO22                  │
└──────────────────────────────────┴──────────────────────┘
```

### Inisialisasi Library Wire

```cpp
#include <Wire.h>

void setup() {
  // Inisialisasi default (SDA=IO21, SCL=IO22, 100kHz)
  Wire.begin();

  // Atau dengan parameter eksplisit:
  Wire.begin(21, 22);          // Wire.begin(SDA, SCL)

  // Ganti kecepatan ke Fast Mode (400kHz):
  Wire.setClock(400000);
}
```

### Fungsi-Fungsi Library Wire

| Fungsi | Keterangan |
|--------|-----------|
| `Wire.begin()` | Inisialisasi I2C sebagai Master |
| `Wire.beginTransmission(addr)` | Mulai pengiriman ke alamat `addr` |
| `Wire.write(byte)` | Tambahkan byte ke antrian kirim |
| `Wire.endTransmission()` | Kirim data & akhiri transaksi |
| `Wire.requestFrom(addr, n)` | Minta `n` byte dari alamat `addr` |
| `Wire.available()` | Jumlah byte yang sudah diterima |
| `Wire.read()` | Baca 1 byte dari buffer terima |
| `Wire.setClock(freq)` | Atur kecepatan bus (Hz) |

---

## 23.4 Program 1: I2C Scanner

Program paling penting saat bekerja dengan I2C adalah **Scanner** — alat untuk menemukan alamat semua perangkat yang terhubung di bus.

```cpp
/*
 * BAB 23 - Program 1: I2C Scanner
 * Memindai semua alamat I2C yang mungkin (0x01 – 0x7F)
 * dan melaporkan perangkat yang ditemukan.
 *
 * Wiring (Kit Bluino — sudah terhubung di PCB):
 *   SDA → IO21
 *   SCL → IO22
 *
 * Jalankan program ini setiap kali kamu menyambung
 * perangkat I2C baru untuk memverifikasi alamatnya!
 */

#include <Wire.h>

#define SDA_PIN 21
#define SCL_PIN 22

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.println("══════════════════════════════════");
  Serial.println("    BAB 23: I2C Bus Scanner");
  Serial.println("══════════════════════════════════");
  Serial.println("Memindai alamat 0x01 hingga 0x7F...");
  Serial.println();

  uint8_t deviceCount = 0;

  for (uint8_t addr = 1; addr < 128; addr++) {
    Wire.beginTransmission(addr);
    uint8_t error = Wire.endTransmission();

    if (error == 0) {
      // Perangkat merespons dengan ACK
      Serial.printf("  ✅ Ditemukan perangkat di alamat 0x%02X (%d)\n",
                    addr, addr);
      deviceCount++;
    } else if (error == 4) {
      // Error tidak dikenal (kemungkinan masalah hardware)
      Serial.printf("  ⚠️  Error tidak dikenal di alamat 0x%02X\n", addr);
    }
    // error == 2: NACK (tidak ada perangkat) — abaikan
  }

  Serial.println();
  if (deviceCount == 0) {
    Serial.println("❌ Tidak ada perangkat I2C yang ditemukan.");
    Serial.println("   Cek koneksi kabel SDA/SCL dan resistor pull-up.");
  } else {
    Serial.printf("✅ Pemindaian selesai. %d perangkat ditemukan.\n",
                  deviceCount);
  }
  Serial.println("══════════════════════════════════");
}

void loop() {
  // Scanner hanya dijalankan sekali di setup()
  // Tekan tombol RESET untuk scan ulang
}
```

**Contoh output yang diharapkan dari Kit Bluino:**
```
══════════════════════════════════
    BAB 23: I2C Bus Scanner
══════════════════════════════════
Memindai alamat 0x01 hingga 0x7F...

  ✅ Ditemukan perangkat di alamat 0x3C (60)   ← OLED SSD1306
  ✅ Ditemukan perangkat di alamat 0x68 (104)  ← MPU6050
  ✅ Ditemukan perangkat di alamat 0x77 (119)  ← BMP180

✅ Pemindaian selesai. 3 perangkat ditemukan.
══════════════════════════════════
```

---

## 23.5 Membaca Register I2C Secara Langsung (Raw Read)

Sebelum menggunakan library sensor yang sudah jadi, penting untuk memahami cara membaca register I2C secara **langsung** — ini adalah fondasi semua library sensor.

### Konsep Register

Perangkat I2C menyimpan data di **register** internal beralamat spesifik:

```
MPU6050 — Contoh Map Register (sebagian):
┌──────────┬──────────────────────────────────────┐
│ Alamat   │ Isi Register                          │
│ Register │                                       │
├──────────┼──────────────────────────────────────┤
│ 0x75     │ WHO_AM_I — ID chip (selalu = 0x68)   │
│ 0x3B     │ ACCEL_XOUT_H — Akselerasi X (High)   │
│ 0x3C     │ ACCEL_XOUT_L — Akselerasi X (Low)    │
│ 0x41     │ TEMP_OUT_H   — Suhu (High byte)       │
│ 0x42     │ TEMP_OUT_L   — Suhu (Low byte)        │
│ 0x6B     │ PWR_MGMT_1   — Power management       │
└──────────┴──────────────────────────────────────┘
```

### Pola Baca Register I2C

```cpp
// Pola universal untuk membaca N byte dari sebuah register:
// 1. Kirim alamat register yang ingin dibaca
// 2. Minta data sejumlah N byte
// 3. Baca byte demi byte dari buffer

uint8_t bacaRegister(uint8_t deviceAddr, uint8_t regAddr) {
  Wire.beginTransmission(deviceAddr);
  Wire.write(regAddr);             // Tunjuk register yang ingin dibaca
  Wire.endTransmission(false);     // false = repeated start (jangan STOP dulu)

  Wire.requestFrom(deviceAddr, (uint8_t)1);  // Minta 1 byte
  if (Wire.available()) {
    return Wire.read();
  }
  return 0xFF;  // Nilai error
}
```

> ⚠️ **Perhatian `endTransmission(false)`:** Parameter `false` sangat penting! Ini mengirimkan **Repeated START** — bus tidak dilepas setelah write sehingga slave tahu bahwa transaksi berikutnya (requestFrom) masih berasal dari master yang sama. Menggunakan `true` atau tanpa parameter BISA menyebabkan beberapa perangkat gagal merespons.

### Program 2: Verifikasi MPU6050 dengan WHO_AM_I

```cpp
/*
 * BAB 23 - Program 2: Verifikasi Perangkat via Register
 * Membaca register WHO_AM_I dari MPU6050 untuk
 * memastikan perangkat terhubung dan berfungsi.
 *
 * Wiring (Kit Bluino — sudah terhubung di PCB):
 *   SDA → IO21
 *   SCL → IO22
 */

#include <Wire.h>

#define SDA_PIN      21
#define SCL_PIN      22

#define MPU6050_ADDR 0x68   // Alamat I2C MPU6050
#define REG_WHO_AM_I 0x75   // Register ID perangkat
#define REG_PWR_MGMT 0x6B   // Register power management

// ── Tulis 1 byte ke register ────────────────────────────────
void tulisRegister(uint8_t devAddr, uint8_t regAddr, uint8_t value) {
  Wire.beginTransmission(devAddr);
  Wire.write(regAddr);
  Wire.write(value);
  Wire.endTransmission();
}

// ── Baca 1 byte dari register ───────────────────────────────
uint8_t bacaRegister(uint8_t devAddr, uint8_t regAddr) {
  Wire.beginTransmission(devAddr);
  Wire.write(regAddr);
  Wire.endTransmission(false);  // Repeated START

  Wire.requestFrom(devAddr, (uint8_t)1);
  if (Wire.available()) {
    return Wire.read();
  }
  return 0xFF;
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.println("BAB 23 - Program 2: Verifikasi MPU6050");
  Serial.println("────────────────────────────────────────");

  // Baca WHO_AM_I — nilai yang benar adalah 0x68
  uint8_t whoAmI = bacaRegister(MPU6050_ADDR, REG_WHO_AM_I);

  Serial.printf("WHO_AM_I register: 0x%02X\n", whoAmI);

  if (whoAmI == 0x68) {
    Serial.println("✅ MPU6050 terdeteksi dengan benar!");
  } else {
    Serial.println("❌ MPU6050 tidak ditemukan atau alamat salah.");
    return;
  }

  // Bangunkan MPU6050 dan atur Clock ke PLL X-Gyro untuk stabilitas (0x01 lebih baik dari 0x00)
  tulisRegister(MPU6050_ADDR, REG_PWR_MGMT, 0x01);
  Serial.println("✅ MPU6050 dibangunkan (Clock = PLL X-Gyro).");
  Serial.println("────────────────────────────────────────");
}

void loop() {
  // Program ini hanya verifikasi koneksi — tidak ada loop
}
```

---

## 23.6 Program 3: Membaca Data Suhu dari MPU6050

MPU6050 memiliki sensor suhu internal (bukan untuk mengukur suhu udara, melainkan suhu chip). Ini latihan yang baik untuk memahami cara menggabungkan dua byte (High + Low) menjadi nilai 16-bit.

```cpp
/*
 * BAB 23 - Program 3: Baca Suhu Internal MPU6050
 * Menampilkan suhu chip MPU6050 setiap 1 detik.
 *
 * Formula: Suhu (°C) = (raw_value / 340.0) + 36.53
 *
 * Wiring (Kit Bluino — sudah terhubung di PCB):
 *   SDA → IO21
 *   SCL → IO22
 */

#include <Wire.h>

#define SDA_PIN       21
#define SCL_PIN       22

#define MPU6050_ADDR  0x68
#define REG_TEMP_H    0x41   // Temperature High byte
#define REG_TEMP_L    0x42   // Temperature Low byte
#define REG_PWR_MGMT  0x6B

// ── Tulis 1 byte ke register ────────────────────────────────
void tulisRegister(uint8_t devAddr, uint8_t regAddr, uint8_t value) {
  Wire.beginTransmission(devAddr);
  Wire.write(regAddr);
  Wire.write(value);
  Wire.endTransmission();
}

// ── Baca N byte berurutan mulai dari regAddr ────────────────
void bacaRegisterMulti(uint8_t devAddr, uint8_t regAddr,
                       uint8_t* buf, uint8_t len) {
  Wire.beginTransmission(devAddr);
  Wire.write(regAddr);
  Wire.endTransmission(false);  // Repeated START

  Wire.requestFrom(devAddr, len);
  for (uint8_t i = 0; i < len && Wire.available(); i++) {
    buf[i] = Wire.read();
  }
}

// ── Baca suhu dari MPU6050 ──────────────────────────────────
float bacaSuhuMPU() {
  uint8_t data[2] = {0, 0};
  bacaRegisterMulti(MPU6050_ADDR, REG_TEMP_H, data, 2);

  // Gabungkan byte (Gunakan tipe UNSIGNED saat SHIFT untuk menghindari C++ Undefined Behavior!)
  uint16_t unsignedRaw = ((uint16_t)data[0] << 8) | data[1];
  int16_t rawTemp = (int16_t)unsignedRaw;

  // Konversi menggunakan formula dari datasheet MPU6050
  return (rawTemp / 340.0f) + 36.53f;
}

unsigned long prevMillis = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  // Bangunkan MPU6050 & gunakan PLL X-Gyro clock sumber (lebih stabil)
  tulisRegister(MPU6050_ADDR, REG_PWR_MGMT, 0x01);

  Serial.println("══════════════════════════════════");
  Serial.println("  BAB 23: Suhu Internal MPU6050");
  Serial.println("══════════════════════════════════");
}

void loop() {
  // Non-blocking: update setiap 1 detik
  if (millis() - prevMillis >= 1000) {
    prevMillis = millis();

    float suhu = bacaSuhuMPU();
    Serial.printf("Suhu chip MPU6050: %.2f °C\n", suhu);
  }
}
```

> 💡 **Catatan Penting:** Suhu yang dibaca adalah **suhu chip MPU6050** bukan suhu udara sekitar. Nilai ini biasanya 5–10°C lebih tinggi dari suhu ruangan karena panas yang dihasilkan chip sendiri. Untuk mengukur suhu udara, gunakan **DHT11** (BAB 26) atau **BMP180** (BAB 28).

---

## 23.7 Program 4: Multi-Device I2C — MPU6050 + BMP180

Inilah keistimewaan I2C: **lebih dari satu sensor bisa dibaca dari bus yang sama**. Kit Bluino secara default sudah menghubungkan OLED, MPU6050, dan BMP180 ke satu bus I2C `IO21/IO22`.

```cpp
/*
 * BAB 23 - Program 4: Multi-Sensor I2C
 * Membaca data dari MPU6050 dan BMP180 secara bersamaan
 * pada satu bus I2C IO21 (SDA) dan IO22 (SCL).
 *
 * ⚠️  Program ini membaca register BMP180 secara LOW-LEVEL
 *     (tanpa library) sebagai demonstrasi konsep.
 *     Untuk proyek nyata, gunakan library Adafruit_BMP085.
 *
 * Wiring (Kit Bluino — sudah terhubung di PCB):
 *   SDA → IO21
 *   SCL → IO22
 */

#include <Wire.h>

#define SDA_PIN       21
#define SCL_PIN       22

// === MPU6050 ===================================================
#define MPU_ADDR      0x68
#define MPU_PWR_MGMT  0x6B
#define MPU_TEMP_H    0x41
#define MPU_WHO_AM_I  0x75

// === BMP180 ====================================================
#define BMP_ADDR      0x77
#define BMP_REG_ID    0xD0   // Chip ID register — harus berisi 0x55

// ── Tulis register ─────────────────────────────────────────────
void tulisReg(uint8_t dev, uint8_t reg, uint8_t val) {
  Wire.beginTransmission(dev);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

// ── Baca 1 register ────────────────────────────────────────────
uint8_t bacaReg(uint8_t dev, uint8_t reg) {
  Wire.beginTransmission(dev);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(dev, (uint8_t)1);
  return Wire.available() ? Wire.read() : 0xFF;
}

// ── Baca N register berurutan ───────────────────────────────────
void bacaRegMulti(uint8_t dev, uint8_t reg, uint8_t* buf, uint8_t len) {
  Wire.beginTransmission(dev);
  Wire.write(reg);
  Wire.endTransmission(false);
  Wire.requestFrom(dev, len);
  for (uint8_t i = 0; i < len && Wire.available(); i++) {
    buf[i] = Wire.read();
  }
}

// ── Baca suhu MPU6050 (raw → °C) ───────────────────────────────
float bacaSuhuMPU() {
  uint8_t d[2] = {0, 0};
  bacaRegMulti(MPU_ADDR, MPU_TEMP_H, d, 2);
  uint16_t u_raw = ((uint16_t)d[0] << 8) | d[1];
  int16_t raw = (int16_t)u_raw;
  return (raw / 340.0f) + 36.53f;
}

bool mpuOK  = false;
bool bmpOK  = false;

unsigned long prevMillis = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);

  Serial.println("══════════════════════════════════════════");
  Serial.println("  BAB 23: Multi-Sensor I2C");
  Serial.println("══════════════════════════════════════════");

  // ── Verifikasi MPU6050 ────────────────────────────────────────
  uint8_t mpuId = bacaReg(MPU_ADDR, MPU_WHO_AM_I);
  if (mpuId == 0x68) {
    tulisReg(MPU_ADDR, MPU_PWR_MGMT, 0x01);  // Wake up & PLL X-Gyro
    mpuOK = true;
    Serial.println("✅ MPU6050 terdeteksi (WHO_AM_I = 0x68)");
  } else {
    Serial.printf("❌ MPU6050 gagal (WHO_AM_I = 0x%02X, expected 0x68)\n",
                  mpuId);
  }

  // ── Verifikasi BMP180 ────────────────────────────────────────
  uint8_t bmpId = bacaReg(BMP_ADDR, BMP_REG_ID);
  if (bmpId == 0x55) {
    bmpOK = true;
    Serial.println("✅ BMP180 terdeteksi (Chip ID = 0x55)");
  } else {
    Serial.printf("❌ BMP180 gagal (Chip ID = 0x%02X, expected 0x55)\n",
                  bmpId);
  }

  Serial.println("══════════════════════════════════════════");
  Serial.println();
}

void loop() {
  if (millis() - prevMillis < 2000) return;
  prevMillis = millis();

  Serial.println("──────────────────────────────────────────");

  if (mpuOK) {
    float suhuMPU = bacaSuhuMPU();
    Serial.printf("MPU6050 — Suhu chip  : %.2f °C\n", suhuMPU);
  } else {
    Serial.println("MPU6050 — [tidak tersedia]");
  }

  if (bmpOK) {
    // Untuk pembacaan penuh BMP180 (tekanan + suhu akurat),
    // diperlukan proses kalibrasi 11 koefisien yang kompleks.
    // Akan dibahas secara mendalam di BAB 28 (Sensor BMP180).
    Serial.println("BMP180  — Terdeteksi ✅ (Data lengkap → lihat BAB 28)");
  } else {
    Serial.println("BMP180  — [tidak tersedia]");
  }
}
```

---

## 23.8 Memahami Konflik Alamat I2C

Setiap proyek I2C yang semakin besar akan menghadapi kemungkinan **konflik alamat** — dua perangkat berbeda yang punya alamat sama.

### Solusi Konflik Alamat

```
Masalah: Dua sensor BMP180 dihubungkan ke satu bus.
         Keduanya beralamat 0x77 → KONFLIK!

Solusi 1: Hardware Address Pin
  BMP180 tidak punya pin ADDR, jadi tidak bisa diubah.
  Gunakan sensor yang berbeda untuk data redundan.

Solusi 2: TCA9548A — I2C Multiplexer
  Memungkinkan 8 bus I2C terpisah dari satu bus master:

  ESP32 ─── SDA/SCL ──► TCA9548A (0x70)
                           ├── Channel 0: [BMP180 #1 - 0x77]
                           ├── Channel 1: [BMP180 #2 - 0x77]
                           └── Channel 2: [BMP180 #3 - 0x77]

  Tidak ada konflik karena setiap channel adalah bus terpisah!

Solusi 3: Software I2C (Second Wire instance)
  Gunakan pin GPIO lain untuk bus I2C kedua:
```

```cpp
// Membuat 2 bus I2C terpisah menggunakan 2 instance Wire
#include <Wire.h>

TwoWire I2C_A = TwoWire(0);  // Bus I2C pertama (hardware I2C0)
TwoWire I2C_B = TwoWire(1);  // Bus I2C kedua  (hardware I2C1)

void setup() {
  // Bus A: Pin default IO21/IO22
  I2C_A.begin(21, 22);

  // Bus B: Pin alternatif — HINDARI strapping pins (IO0, IO2, IO12, IO15)
  I2C_B.begin(25, 26);

  // Gunakan seperti Wire biasa, tapi dengan nama instance:
  I2C_A.beginTransmission(0x68);  // MPU6050 di bus A
  I2C_A.endTransmission();

  I2C_B.beginTransmission(0x77);  // BMP180 kedua di bus B
  I2C_B.endTransmission();
}
```

---

## 23.9 Tips & Panduan Troubleshooting I2C

### 1. Tidak ada perangkat yang terdeteksi?

```
✅ Cek 1: Pastikan SDA=IO21 dan SCL=IO22 (sesuai Kit Bluino)
✅ Cek 2: Jalankan I2C Scanner (Program 1) terlebih dahulu
✅ Cek 3: Periksa tegangan supply sensor (3.3V atau 5V?)
✅ Cek 4: Pastikan Ground ESP32 dan sensor terhubung
✅ Cek 5: Resistor pull-up sudah ada? (Di Kit Bluino = sudah ada)
```

### 2. Data Intermittent (Kadang Berhasil, Kadang Tidak)?

```
❌ Kabel terlalu panjang → I2C bekerja baik di bawah 50cm
❌ Kecepatan clock terlalu tinggi → Coba turunkan ke 100kHz
❌ Tidak menggunakan repeated START → Ganti endTransmission(true)
   menjadi endTransmission(false) sebelum requestFrom()
```

### 3. Kode Return `endTransmission()`

| Return Value | Arti |
|---|---|
| `0` | Sukses |
| `1` | Data terlalu panjang untuk buffer transmit |
| `2` | NACK saat mengirim alamat — **perangkat tidak ada atau alamat salah** |
| `3` | NACK saat mengirim data |
| `4` | Error lain (bus busy, SDA/SCL stuck) |
| `5` | Timeout |

### 4. Gunakan `Wire.setTimeout()`

```cpp
// Default timeout Wire adalah 50ms — bisa disesuaikan
// Perbesar jika sensor lambat merespons:
Wire.setTimeout(100);   // 100ms timeout
```

---

## 23.10 Ringkasan

```
┌─────────────────────────────────────────────────────────┐
│              RINGKASAN BAB 23 — I2C                      │
├─────────────────────────────────────────────────────────┤
│ I2C: Master-Slave, 2 kabel (SDA + SCL), alamat 7-bit    │
│                                                         │
│ Pin di Kit Bluino:                                       │
│   SDA = IO21   SCL = IO22                               │
│   Pull-up resistor sudah terpasang di PCB!              │
│                                                         │
│ Perangkat I2C di kit:                                    │
│   • OLED SSD1306  → 0x3C                                │
│   • MPU6050       → 0x68                                │
│   • BMP180        → 0x77                                │
│                                                         │
│ Pola baca register:                                      │
│   beginTransmission(addr)                               │
│   write(regAddr)                                        │
│   endTransmission(false)  ← Repeated START!             │
│   requestFrom(addr, n)                                  │
│   read() × n                                            │
│                                                         │
│ Pantangan:                                              │
│   ❌ endTransmission(true) sebelum requestFrom()         │
│   ❌ Dua perangkat dengan alamat sama di satu bus        │
│   ❌ Kabel I2C terlalu panjang (>50cm tanpa terminasi)   │
└─────────────────────────────────────────────────────────┘
```

---

## 23.11 Latihan

1. **Scanner Plus**: Modifikasi Program 1 agar selain alamat, juga mencetak nama perangkat yang dikenal (misal: jika `0x3C` → tampilkan "OLED SSD1306", `0x68` → "MPU6050", `0x77` → "BMP180").

2. **Loop Suhu**: Modifikasi Program 3 agar menampilkan grafik batang sederhana di Serial Monitor sesuai nilai suhu. Misal: `25°C = [=====     ]` dan `40°C = [==========]`.

3. **Status Message**: Pada Program 4, tambahkan logika: jika `mpuOK` atau `bmpOK` adalah `false`, cetak pesan peringatan **setiap 5 detik** menggunakan `millis()` (non-blocking) agar pengguna tahu ada sensor yang tidak terhubung.

4. **Multi-Bus**: Buat program yang menginisialisasi dua bus Wire (`I2C_A` dan `I2C_B`) dan memindai masing-masing secara terpisah, lalu melaporkan perangkat yang ada di setiap bus.

---

*Selanjutnya → [BAB 24: SPI — Serial Peripheral Interface](../BAB-24/README.md)*
