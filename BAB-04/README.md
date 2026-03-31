# BAB 4: Membaca Schematic & Datasheet

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Membaca dan memahami simbol-simbol pada schematic elektronika
- Mengidentifikasi koneksi antar komponen dari schematic
- Memahami cara membaca datasheet komponen
- Menganalisis schematic Kit Bluino ESP32 Starter Shield v3.2

---

## 4.1 Apa itu Schematic?

**Schematic** (skematik) adalah diagram yang menggambarkan rangkaian elektronika menggunakan **simbol standar**. Schematic menunjukkan:
- Komponen apa saja yang digunakan
- Bagaimana komponen terhubung satu sama lain
- Nilai komponen (resistor, kapasitor, dll)
- Label pin dan jalur sinyal

> **Analogi:** Schematic ibarat "peta" rangkaian. Seperti peta jalan yang tidak menunjukkan bentuk jalan sebenarnya tapi menunjukkan koneksi antar lokasi, schematic tidak menunjukkan posisi fisik komponen tapi menunjukkan koneksi listriknya.

---

## 4.2 Simbol-Simbol Dasar

### Komponen Pasif

```
Resistor          Kapasitor          Kapasitor         Induktor
                  (Non-polar)        (Elektrolit)
  ─┤├─              ─┤ ├─              ─┤ ┣─            ─(((─
                                         +
```

### Semikonduktor

```
LED                 Dioda              NPN Transistor
                                       
  ──►|──            ──►|──               C
                                         │
                                       B─┤
                                         │
                                         E
```

### Sumber & Koneksi

```
GND (Ground)       VCC / VDD          Koneksi           Tidak Terhubung
                   (Sumber +)
    ┴               ───●               ─── ┼ ───          ── ╳ ──
   ─┴─                                 (titik = terhubung) (tanpa titik)
   ━┻━
```

---

## 4.3 Praktik: Membaca Schematic Kit Bluino

Buka file `Schematic.jpg` pada repository ini. Kita akan menganalisis setiap bagian:

### Bagian 1: ESP32 DEVKIT V1

Di pojok kiri atas, terlihat simbol ESP32 DEVKIT V1 dengan seluruh pinout:

```
Sisi Kiri:             Sisi Kanan:
EN, VP, VN,            IO23, IO22, TX0(IO1),
IO34, IO35, IO32,      RX0(IO3), IO21, IO19,
IO33, IO25, IO26,      IO18, IO5, IO17,
IO27, IO14, IO12,      IO16, IO4, IO2,
IO13, GND, VIN         IO15, GND, 3V3
```

### Bagian 2: Header Ekspansi (di tengah)

Header 2×15 pin berfungsi sebagai **jembatan** antara ESP32 dan komponen kit. Pin-pin ESP32 disalurkan ke header ini, dan dari header inilah komponen-komponen terhubung.

### Bagian 3: I2C Device (Kanan Atas)

Perhatikan 3 komponen yang **berbagi bus I2C** yang sama (IO21=SDA, IO22=SCL):

```
IO21 (SDA) ───┬─── OLED 128x64 (alamat: 0x3C)
              ├─── MPU-6050     (alamat: 0x68)
              └─── BMP180       (alamat: 0x77)

IO22 (SCL) ───┬─── OLED 128x64
              ├─── MPU-6050
              └─── BMP180
```

> **Catatan:** Setiap device I2C memiliki **alamat unik**, sehingga tidak ada konflik meskipun berbagi 2 kabel yang sama.

### Bagian 4: SPI Bus (Kiri Tengah)

MicroSD Adapter menggunakan 4 jalur SPI:

| Pin ESP32 | Fungsi SPI | Pin MicroSD |
|-----------|-----------|-------------|
| IO5 | CS (Chip Select) | Pin 2 |
| IO23 | MOSI (Data Out) | Pin 3 |
| IO18 | CLK (Clock) | Pin 5 |
| IO19 | MISO (Data In) | Pin 7 |

### Bagian 5: Sensor dengan Pin Tetap (Kanan)

| Komponen | Pin | Catatan |
|----------|-----|---------|
| DHT11 | IO27 | Resistor pull-up 4.7KΩ ke VDD sudah terpasang |
| DS18B20 | IO14 | Resistor pull-up 4.7KΩ ke VDD sudah terpasang |
| WS2812 | IO12 | Terhubung ke pin DIN (Data In) |

### Bagian 6: Komponen "Custom" (Berlabel Custom)

Komponen berikut **tidak terhubung langsung** ke GPIO tertentu. Kamu harus menghubungkannya sendiri menggunakan kabel jumper:

| Komponen | Label di Schematic | Catatan Koneksi |
|----------|--------------------|-----------------|
| 4× LED 5mm | Custom | Butuh pin digital output. Resistor 330Ω sudah terpasang |
| 2× Push Button | Custom | Butuh pin digital input. Pull-down 10KΩ sudah terpasang |
| Active Buzzer | Custom | Butuh pin digital output |
| Potensiometer | Custom | Butuh pin **ADC** (analog input) |
| LDR | Custom | Butuh pin **ADC**. Voltage divider 10KΩ sudah terpasang |
| PIR AM312 | Custom | Butuh pin digital input |
| Relay | Custom | Butuh pin digital output. Driver BC547 sudah terpasang |
| HC-SR04 | Custom | Butuh 2 pin: Trig (output) + Echo (input) |
| Servo | Custom | Butuh pin **PWM** output |
| Touch Pad | Custom | Butuh pin **touch** (T0–T9, contoh: IO4, IO13, IO15, IO32, IO33) |

### Bagian 7: Power Regulator (Kiri Bawah)

```
    +12V ── 1N4001 ──┬── IN │78M05│ OUT ──┬── +5V
                      │                    │
                   C1 ═ 100µF/6V        C2 ═ keramik
                      │                    │
                     GND                  GND
```

- **1N4001:** Proteksi polaritas terbalik
- **78M05:** Regulator 12V → 5V
- **C1 (100µF/6V):** Filter ripple input
- **C2 (keramik):** Filter noise frekuensi tinggi

### Bagian 8: Relay Driver (Kanan Bawah)

```
    5V ──── Relay Coil ──┬── Collector (BC547)
             ┃           │
        1N4148 (flyback)  │
             ┃           │
            ─┘     Base ─┤── Resistor ── (pin Custom)
                         │
                   Emitter ── GND
```

- **BC547 (NPN):** Transistor saklar — menguatkan sinyal 3.3V ESP32 untuk menggerakkan relay 5V
- **1N4148:** Dioda flyback — melindungi dari spike tegangan saat relay OFF
- **LED + 1KΩ:** Indikator visual relay aktif

---

## 4.4 Cara Membaca Datasheet

**Datasheet** adalah dokumen teknis resmi dari produsen komponen yang berisi semua spesifikasi.

### Bagian Penting dari Datasheet

| Bagian | Isi | Contoh |
|--------|-----|--------|
| **Absolute Maximum Ratings** | Batas yang TIDAK BOLEH dilampaui | VDD max = 3.6V untuk ESP32 |
| **Electrical Characteristics** | Spesifikasi normal operasi | Arus GPIO = 12mA recommended |
| **Pin Description** | Fungsi setiap pin | IO6–IO11 = Flash SPI |
| **Typical Application** | Contoh rangkaian | Rangkaian dasar ESP32 |
| **Timing Diagram** | Timing sinyal | Protocol DHT11 |
| **Package Information** | Dimensi fisik | Ukuran chip ESP32-WROOM-32 |

### Tips Membaca Datasheet

1. **Selalu cek Absolute Maximum Ratings dulu** — ini yang menentukan batas aman
2. **Fokus pada bagian yang relevan** — tidak perlu baca seluruh dokumen
3. **Perhatikan conditions** — spesifikasi berlaku pada kondisi tertentu (suhu, tegangan)
4. **Cek application notes** — sering berisi tips implementasi praktis

### Contoh: Membaca Datasheet ESP32

Dari [ESP32 Datasheet](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf):

| Parameter | Min | Typical | Max | Satuan |
|-----------|-----|---------|-----|--------|
| VDD | 2.3 | 3.3 | 3.6 | V |
| Arus GPIO (satu pin) | — | — | 40 | mA |
| Arus GPIO (recommended) | — | — | 12 | mA |
| Arus total semua GPIO | — | — | 1200 | mA |
| Deep sleep current | — | 10 | — | µA |
| WiFi TX current | — | 240 | — | mA |

> **Catatan penting:** Walaupun max per pin = 40mA, **disarankan tidak melebihi 12mA** per pin untuk operasi yang aman dan stabil.

---

## 4.5 Datasheet Komponen Kit Bluino

Berikut link datasheet untuk setiap komponen utama pada kit:

| Komponen | Datasheet / Referensi |
|----------|----------------------|
| ESP32 | [ESP32 Datasheet (Espressif)](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf) |
| DHT11 | [DHT11 Datasheet](https://www.mouser.com/datasheet/2/758/DHT11-Technical-Data-Sheet-Translated-Version-1143054.pdf) |
| DS18B20 | [DS18B20 Datasheet (Maxim)](https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf) |
| BMP180 | [BMP180 Datasheet (Bosch)](https://cdn-shop.adafruit.com/datasheets/BST-BMP180-DS000-09.pdf) |
| MPU-6050 | [MPU-6050 Datasheet (InvenSense)](https://invensense.tdk.com/wp-content/uploads/2015/02/MPU-6000-Datasheet1.pdf) |
| SSD1306 (OLED) | [SSD1306 Datasheet](https://cdn-shop.adafruit.com/datasheets/SSD1306.pdf) |
| WS2812 | [WS2812B Datasheet](https://cdn-shop.adafruit.com/datasheets/WS2812B.pdf) |
| HC-SR04 | [HC-SR04 Datasheet](https://cdn.sparkfun.com/datasheets/Sensors/Proximity/HCSR04.pdf) |
| PIR AM312 | [AM312 Datasheet](https://pdf1.alldatasheet.com/datasheet-pdf/view/1328584/EONE/AM312.html) |
| BC547 | [BC547 Datasheet (ON Semi)](https://www.onsemi.com/pdf/datasheet/bc546-d.pdf) |

---

## 📝 Latihan

1. **Baca schematic:** Buka `Schematic.jpg` dan identifikasi:
   - Berapa komponen yang menggunakan I2C? Sebutkan alamatnya.
   - Pin GPIO mana saja yang sudah terpakai (hardwired)?
   - Berapa komponen yang berlabel "Custom"?

2. **Datasheet ESP32:** Buka datasheet ESP32 dan jawab:
   - Berapa tegangan kerja minimum ESP32?
   - Berapa arus saat deep sleep?
   - Mengapa GPIO 34–39 hanya bisa digunakan sebagai input?

3. **Analisis relay driver:** Dari schematic bagian relay, jelaskan:
   - Mengapa dioda 1N4148 dipasang terbalik (reverse bias) paralel dengan koil relay?
   - Apa yang terjadi jika dioda ini tidak dipasang?

4. **Trace koneksi:** Dari schematic, trace jalur sinyal berikut:
   - Dari pin IO27 ESP32, melalui apa saja sampai ke pin data DHT11?
   - Dari pin IO5 ESP32, melalui apa saja sampai ke pin CS MicroSD?

---

## 📚 Referensi

- [How to Read a Schematic — SparkFun](https://learn.sparkfun.com/tutorials/how-to-read-a-schematic)
- [How to Read a Datasheet — SparkFun](https://learn.sparkfun.com/tutorials/how-to-read-a-datasheet)
- [ESP32 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)

---

[⬅️ BAB 3: Dasar Elektronika](../BAB-03/README.md) | [Kembali ke Daftar Isi](../README.md) | [Selanjutnya: BAB 5 — Setup Lingkungan ➡️](../BAB-05/README.md)

---

## ☕ Dukung Proyek Ini

Bab ini disusun dengan dedikasi waktu dan keahlian yang tidak sedikit. Jika materi *engineering-grade* ini bermanfaat bagi riset atau proses belajarmu, bentuk apresiasi terbaik yang bisa kamu berikan adalah secangkir kopi untuk penulis:

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
Punya kritik, saran, atau butuh mentoring khusus ESP32 & IoT industri? Silakan sapa saya di:
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

