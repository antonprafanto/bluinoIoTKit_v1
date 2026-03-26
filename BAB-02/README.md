# BAB 2: Pengenalan ESP32

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami arsitektur internal ESP32
- Mengenal fitur-fitur utama ESP32
- Membedakan varian keluarga ESP32
- Memahami kelebihan ESP32 dibandingkan mikrokontroler lain

---

## 2.1 Apa itu ESP32?

**ESP32** adalah mikrokontroler buatan **Espressif Systems** (perusahaan asal Shanghai, China) yang menggabungkan prosesor dual-core, WiFi, dan Bluetooth dalam satu chip. ESP32 adalah penerus dari ESP8266 yang sangat populer di komunitas IoT.

### Mengapa ESP32 Begitu Populer?

| Fitur | Keterangan |
|-------|------------|
| 💰 Murah | ~Rp 50.000–80.000 untuk board development |
| 📶 WiFi Built-in | 802.11 b/g/n (2.4 GHz) |
| 📱 Bluetooth Built-in | Classic + BLE (Bluetooth Low Energy) |
| 🧠 Dual-Core | 2 CPU @ 240 MHz |
| ⚡ Low Power | Deep sleep hanya ~10 µA |
| 🔌 Banyak GPIO | 34 pin GPIO programmable |
| 📊 Peripheral Lengkap | ADC, DAC, I2C, SPI, UART, PWM, Touch, dll |
| 🌐 Komunitas Besar | Jutaan pengguna, ribuan tutorial |

---

## 2.2 Arsitektur Internal ESP32

```
┌──────────────────────────────────────────────────────────────┐
│                         ESP32 SoC                            │
│                                                              │
│  ┌────────────┐   ┌────────────┐   ┌─────────────────────┐  │
│  │   Core 0   │   │   Core 1   │   │    WiFi 802.11      │  │
│  │ Xtensa LX6 │   │ Xtensa LX6 │   │    b/g/n            │  │
│  │  240 MHz   │   │  240 MHz   │   ├─────────────────────┤  │
│  └──────┬─────┘   └──────┬─────┘   │    Bluetooth        │  │
│         │                │         │    Classic + BLE     │  │
│         └────────┬───────┘         └─────────────────────┘  │
│                  │                                           │
│  ┌───────────────┴───────────────────────────────────────┐  │
│  │                    Bus Matrix                          │  │
│  └───────────────┬───────────────────────────────────────┘  │
│                  │                                           │
│  ┌───────┬───────┼───────┬───────┬───────┬───────────────┐  │
│  │ SRAM  │ ROM   │  RTC  │ eFuse │  DMA  │ Crypto Accel  │  │
│  │ 520KB │ 448KB │  16KB │       │       │ AES/SHA/RSA   │  │
│  └───────┴───────┴───────┴───────┴───────┴───────────────┘  │
│                                                              │
│  ┌───────────────────────────────────────────────────────┐  │
│  │                   Peripheral                          │  │
│  │  18x ADC │ 2x DAC │ 10x Touch │ 4x Timer │ Hall      │  │
│  │  3x UART │ 2x I2C │ 3x SPI   │ 16x PWM  │ Temp      │  │
│  │  2x I2S  │ 1x TWAI│ RMT (8ch)│ 2x MCPWM │ PCNT      │  │
│  └───────────────────────────────────────────────────────┘  │
│                                                              │
│  ┌───────────────────────────────────────────────────────┐  │
│  │               ULP Co-processor (FSM)                   │  │
│  │          Berjalan saat Deep Sleep (~10 µA)              │  │
│  └───────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### Penjelasan Komponen Utama

#### 🧠 Prosesor Dual-Core Xtensa LX6

ESP32 memiliki **2 CPU** yang berjalan di **240 MHz**:

| Core | Nama Default | Tugas Utama |
|------|-------------|-------------|
| Core 0 | Protocol CPU (PRO) | WiFi, Bluetooth, sistem internal |
| Core 1 | Application CPU (APP) | Kode program pengguna (sketch Arduino) |

> **Catatan:** Saat menggunakan Arduino framework, kode `loop()` berjalan di **Core 1**. Core 0 digunakan oleh sistem WiFi/BT secara otomatis.

#### 📡 WiFi

- Standar: IEEE 802.11 b/g/n
- Frekuensi: 2.4 GHz
- Mode: Station (koneksi ke router), Access Point (jadi hotspot), Station+AP
- Kecepatan: hingga 150 Mbps
- Konsumsi daya: ~120–240 mA saat transmit

#### 📱 Bluetooth

- **Bluetooth Classic:** Untuk streaming audio, serial communication
- **BLE (Bluetooth Low Energy):** Untuk sensor, beacon, wearable — hemat daya

#### 💾 Memori

| Tipe | Ukuran | Fungsi |
|------|--------|--------|
| SRAM | 520 KB | Memori kerja (variabel, stack, heap) |
| ROM | 448 KB | Bootloader dan fungsi inti |
| RTC SRAM | 8 KB fast + 8 KB slow | Memori yang bertahan saat deep sleep |
| Flash Eksternal | 4 MB (tipikal) | Penyimpanan program dan file system |

#### ⚙️ Peripheral

| Peripheral | Jumlah | Fungsi |
|-----------|--------|--------|
| GPIO | 34 pin | Input/output digital |
| ADC | 18 channel (12-bit) | Baca sensor analog |
| DAC | 2 channel (8-bit) | Output tegangan analog |
| Touch | 10 pin | Sensor sentuh kapasitif |
| UART | 3 | Komunikasi serial |
| I2C | 2 bus | Sensor digital (OLED, BMP180, MPU6050) |
| SPI | 3 bus | Komunikasi cepat (MicroSD, display) |
| I2S | 2 | Audio digital |
| PWM (LEDC) | 16 channel | LED dimming, servo, buzzer tone |
| Timer | 4 × 64-bit | Timing presisi, interrupt periodik |
| TWAI/CAN | 1 | Komunikasi otomotif/industri |
| RMT | 8 channel | IR remote, WS2812 LED |
| MCPWM | 2 unit | Kontrol motor presisi |
| PCNT | 8 channel | Counter pulsa hardware |
| Hall Sensor | 1 | Deteksi medan magnet |
| Temp Sensor | 1 | Suhu internal chip |

#### 🔒 Keamanan

- AES, SHA-2, RSA hardware acceleration
- Secure Boot — hanya firmware bertanda tangan yang bisa berjalan
- Flash Encryption — enkripsi isi memori flash
- eFuse — bit keamanan one-time programmable

#### 🔋 ULP Co-processor

**Ultra-Low Power Co-processor** — prosesor mini di dalam ESP32 yang bisa berjalan saat CPU utama tidur (deep sleep). Digunakan untuk:
- Monitoring sensor secara periodik tanpa membangunkan CPU utama
- Konsumsi daya hanya **~10 µA** saat deep sleep dengan ULP aktif

---

## 2.3 ESP32 DEVKIT V1

Board yang digunakan pada **Kit Bluino** adalah **ESP32 DEVKIT V1** — board development paling umum untuk ESP32.

### Spesifikasi Board

| Parameter | Nilai |
|-----------|-------|
| Chip | ESP32-WROOM-32 |
| CPU | Dual-Core Xtensa LX6 @ 240 MHz |
| Flash | 4 MB |
| SRAM | 520 KB |
| WiFi | 802.11 b/g/n |
| Bluetooth | v4.2 BR/EDR + BLE |
| GPIO | 30 pin tersedia pada DevKit (dari 34 GPIO total pada chip) |
| USB | Micro-USB (via CP2102 atau CH340 USB-to-Serial) |
| Tegangan Operasi | 3.3V (logic level) |
| Tegangan Input (VIN) | 5V via USB atau pin VIN |
| Arus GPIO | Max 12 mA (recommended), 40 mA (absolute max) |
| Tombol | EN (Reset), BOOT (GPIO0) |
| LED | 1× LED built-in (biasanya GPIO2, cek board masing-masing) |

### Layout Pin ESP32 DEVKIT V1

```
                    ┌──────────────┐
                    │   ESP32      │
                    │  DEVKIT V1   │
                    │              │
              EN ───┤      USB     ├─── IO23
              VP ───┤              ├─── IO22
              VN ───┤              ├─── TX0 (IO1)
            IO34 ───┤              ├─── RX0 (IO3)
            IO35 ───┤              ├─── IO21
            IO32 ───┤              ├─── IO19
            IO33 ───┤              ├─── IO18
            IO25 ───┤              ├─── IO5
            IO26 ───┤              ├─── IO17
            IO27 ───┤              ├─── IO16
            IO14 ───┤              ├─── IO4
            IO12 ───┤              ├─── IO2
            IO13 ───┤              ├─── IO15
             GND ───┤              ├─── GND
             VIN ───┤              ├─── 3V3
                    └──────────────┘
```

### Tombol pada Board

| Tombol | Fungsi |
|--------|--------|
| **EN** | Reset — menekan akan me-restart ESP32 |
| **BOOT** | GPIO0 — tahan saat upload jika auto-flash gagal |

> **Kit Bluino** memiliki kapasitor 0.1µF antara EN dan GND untuk **auto-reset/auto-flash**, sehingga biasanya tidak perlu menekan tombol BOOT saat upload.

---

## 2.4 Keluarga ESP32 — Varian Lainnya

ESP32 yang kita gunakan adalah **ESP32 (classic)**. Espressif telah merilis beberapa varian:

| Varian | CPU | WiFi | BT | Thread/Zigbee | Keunggulan |
|--------|-----|------|----|---------------|------------|
| **ESP32** ✅ | Dual Xtensa LX6 | ✅ | Classic+BLE | ❌ | Paling mature, komunitas terbesar |
| ESP32-S2 | Single Xtensa LX7 | ✅ | ❌ | ❌ | Native USB, murah |
| ESP32-S3 | Dual Xtensa LX7 | ✅ | BLE 5 | ❌ | AI/ML, USB OTG, lebih cepat |
| ESP32-C3 | Single RISC-V | ✅ | BLE 5 | ❌ | Murah, arsitektur RISC-V |
| ESP32-C6 | Dual RISC-V (HP+LP) | ✅ | BLE 5 | ✅ | Matter & Thread support |
| ESP32-H2 | Single RISC-V | ❌ | BLE 5 | ✅ | Thread/Zigbee dedicated |

> **Kit Bluino** menggunakan **ESP32 classic (Xtensa LX6 Dual-Core)** — pilihan terbaik untuk belajar karena dukungan komunitas dan library paling lengkap.

---

## 2.5 Perbandingan ESP32 vs Arduino

Banyak pemula memulai dengan Arduino. Berikut perbandingan ESP32 dengan Arduino Uno:

| Aspek | ESP32 DEVKIT V1 | Arduino Uno |
|-------|-----------------|-------------|
| CPU | Dual-Core 240 MHz | Single-Core 16 MHz |
| RAM | 520 KB | 2 KB |
| Flash | 4 MB | 32 KB |
| WiFi | ✅ Built-in | ❌ Perlu shield/modul |
| Bluetooth | ✅ Built-in | ❌ Perlu shield/modul |
| ADC | 18 ch × 12-bit | 6 ch × 10-bit |
| DAC | 2 channel | ❌ Tidak ada |
| Touch | 10 pin | ❌ Tidak ada |
| PWM | 16 channel (LEDC) | 6 channel |
| Tegangan Logic | 3.3V | 5V |
| Harga | ~Rp 55.000 | ~Rp 85.000 (clone) / ~Rp 350.000 (original) |
| RTOS | FreeRTOS bawaan | ❌ Tidak ada |

> **Kesimpulan:** ESP32 jauh lebih powerful dengan harga lebih murah. Satu-satunya keunggulan Arduino Uno adalah logic level 5V yang compatible langsung dengan lebih banyak sensor lama.

---

## 2.6 Ekosistem Pengembangan ESP32

ESP32 bisa diprogram menggunakan beberapa framework:

| Framework | Bahasa | Level | Keterangan |
|-----------|--------|-------|------------|
| **Arduino** ✅ | C/C++ | Pemula–Menengah | Yang akan kita gunakan di materi ini |
| PlatformIO | C/C++ | Menengah | IDE profesional, kompatibel Arduino |
| ESP-IDF | C | Lanjutan | Framework resmi Espressif, kontrol penuh |
| MicroPython | Python | Pemula | Scripting, prototyping cepat |
| ESPHome | YAML | Pemula | Deklaratif, untuk Home Assistant |

> **Di materi ini**, kita menggunakan **Arduino framework** karena paling mudah dipelajari dan memiliki library paling banyak. Framework lain akan diperkenalkan di BAB lanjutan.

---

## 📝 Latihan

1. **Arsitektur ESP32:** Sebutkan 5 peripheral ESP32 yang akan kita gunakan pada kit Bluino, dan jelaskan fungsi masing-masing.

2. **Dual-Core:** ESP32 memiliki 2 CPU core. Core mana yang menjalankan kode Arduino `loop()` secara default? Core mana yang menangani WiFi?

3. **Perbandingan:** Kamu diminta membuat sistem monitoring suhu yang mengirim data ke smartphone via Bluetooth. Mengapa ESP32 lebih cocok daripada Arduino Uno? Sebutkan minimal 3 alasan teknis.

4. **Varian ESP32:** Jika kamu ingin membuat perangkat smart home yang kompatibel dengan standar Matter dan Thread, varian ESP32 mana yang harus kamu pilih? Mengapa?

5. **Memori:** ESP32 memiliki 520 KB SRAM. Jelaskan mengapa ini penting dan apa yang terjadi jika program kamu menggunakan terlalu banyak memori.

---

## 📚 Referensi

- [ESP32 Datasheet (Espressif)](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)
- [ESP32 Technical Reference Manual](https://www.espressif.com/sites/default/files/documentation/esp32_technical_reference_manual_en.pdf)
- [Random Nerd Tutorials — Getting Started with ESP32](https://randomnerdtutorials.com/getting-started-with-esp32/)
- [ESP32 Pinout Reference](https://randomnerdtutorials.com/esp32-pinout-reference-gpios/)

---

[⬅️ BAB 1: Apa itu IoT](../BAB-01/README.md) | [Kembali ke Daftar Isi](../README.md) | [Selanjutnya: BAB 3 — Dasar Elektronika ➡️](../BAB-03/README.md)
