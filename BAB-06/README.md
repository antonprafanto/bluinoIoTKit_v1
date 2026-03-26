# BAB 6: Pin Mapping Kit Bluino & Restricsi GPIO

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami seluruh pin mapping ESP32 DEVKIT V1
- Membedakan pin hardwired dan pin custom pada kit Bluino
- Mengetahui restricsi dan batasan setiap GPIO
- Memilih GPIO yang tepat untuk komponen custom
- Menghindari pin-pin yang berbahaya atau terlarang

---

## 6.1 Pinout ESP32 DEVKIT V1

ESP32 DEVKIT V1 memiliki **30 pin** yang tersedia melalui dua baris header. Dari total 34 GPIO pada chip ESP32-WROOM-32, hanya 30 yang di-expose pada board DevKit V1.

```
                      ┌──────────────┐
                      │   ESP32      │
                      │  DEVKIT V1   │
                      │              │
   [1]  3V3 ─────────┤      USB     ├───────── GND  [1]
   [2]   EN ─────────┤              ├───────── IO23 [2]
   [3]   VP (IO36) ──┤              ├───────── IO22 [3]
   [4]   VN (IO39) ──┤              ├───────── TX0 (IO1) [4]
   [5]  IO34 ────────┤              ├───────── RX0 (IO3) [5]
   [6]  IO35 ────────┤              ├───────── IO21 [6]
   [7]  IO32 ────────┤              ├───────── IO19 [7]
   [8]  IO33 ────────┤              ├───────── IO18 [8]
   [9]  IO25 ────────┤              ├───────── IO5  [9]
   [10] IO26 ────────┤              ├───────── IO17 [10]
   [11] IO27 ────────┤              ├───────── IO16 [11]
   [12] IO14 ────────┤              ├───────── IO4  [12]
   [13] IO12 ────────┤              ├───────── IO2  [13]
   [14] IO13 ────────┤              ├───────── IO15 [14]
   [15] GND ─────────┤              ├───────── GND  [15]
   [16] VIN ─────────┤              ├───────── 3V3  [16]
                      └──────────────┘
```

---

## 6.2 Koneksi Tetap (Hardwired) pada Kit Bluino

Komponen-komponen ini sudah terhubung langsung ke GPIO tertentu di PCB. **Tidak bisa diubah.**

### I2C Bus (Shared)

| GPIO | Fungsi | Perangkat |
|------|--------|-----------|
| **IO21** | SDA (Data) | OLED 128×64, BMP180, MPU-6050 |
| **IO22** | SCL (Clock) | OLED 128×64, BMP180, MPU-6050 |

> Ketiga perangkat I2C ini bisa aktif bersamaan karena memiliki **alamat berbeda**:
> - OLED SSD1306: `0x3C`
> - BMP180: `0x77`
> - MPU-6050: `0x68`

### SPI Bus

| GPIO | Fungsi | Perangkat |
|------|--------|-----------|
| **IO5** | CS (Chip Select) | MicroSD Adapter |
| **IO23** | MOSI (Master Out) | MicroSD Adapter |
| **IO18** | CLK (Clock) | MicroSD Adapter |
| **IO19** | MISO (Master In) | MicroSD Adapter |

### Sensor Digital

| GPIO | Perangkat | Catatan |
|------|-----------|---------|
| **IO27** | DHT11 | Pull-up 4.7KΩ terpasang |
| **IO14** | DS18B20 | Pull-up 4.7KΩ terpasang |
| **IO12** | WS2812 (NeoPixel) | Pin data DIN. ⚠️ IO12 juga strapping pin (MTDI) |

### Ringkasan Pin Terpakai

```
┌─────────────────────────────────────────────────────┐
│ PIN TERPAKAI (JANGAN HUBUNGKAN KOMPONEN CUSTOM)     │
├──────┬──────────────────────────────────────────────┤
│ IO5  │ MicroSD CS                                    │
│ IO12 │ WS2812 NeoPixel                               │
│ IO14 │ DS18B20                                       │
│ IO18 │ MicroSD CLK                                   │
│ IO19 │ MicroSD MISO                                  │
│ IO21 │ I2C SDA (OLED, BMP180, MPU-6050)              │
│ IO22 │ I2C SCL (OLED, BMP180, MPU-6050)              │
│ IO23 │ MicroSD MOSI                                  │
│ IO27 │ DHT11                                         │
└──────┴──────────────────────────────────────────────┘
```

**Jumlah: 9 GPIO terpakai** dari 30 yang tersedia di DevKit.

---

## 6.3 Restricsi GPIO ESP32

Tidak semua GPIO bisa digunakan secara bebas. Berikut penjelasan lengkap:

### 🚫 GPIO yang DILARANG Digunakan

| GPIO | Alasan | Risiko |
|------|--------|--------|
| IO6 | Flash SPI (CLK) | ESP32 CRASH / tidak boot |
| IO7 | Flash SPI (D0) | ESP32 CRASH / tidak boot |
| IO8 | Flash SPI (D1) | ESP32 CRASH / tidak boot |
| IO9 | Flash SPI (D2) | ESP32 CRASH / tidak boot |
| IO10 | Flash SPI (D3) | ESP32 CRASH / tidak boot |
| IO11 | Flash SPI (CMD) | ESP32 CRASH / tidak boot |

> **IO6–IO11** terhubung ke flash memory internal. Menyentuh pin ini **akan menyebabkan ESP32 crash dan tidak bisa boot.**

### ⚠️ Strapping Pins (Hati-hati)

Strapping pins menentukan mode boot ESP32. Nilainya dibaca saat power-up atau reset:

| GPIO | Strapping | Kondisi Saat Boot | Dapat Dipakai? |
|------|-----------|-------------------|----------------|
| IO0 | Boot mode | Harus HIGH untuk normal boot | ⚠️ Hindari — terhubung ke tombol BOOT |
| IO2 | Boot mode | Harus LOW saat boot | ✅ Bisa, tapi pastikan LOW saat reset |
| IO12 | MTDI | Mempengaruhi voltase flash (1.8V/3.3V) | ❌ Sudah dipakai WS2812 pada kit ini |
| IO15 | MTDO | Mempengaruhi log output saat boot | ✅ Bisa dipakai, tapi ada boot log output |

### ❌ Pin Serial (Hindari)

| GPIO | Fungsi | Alasan |
|------|--------|--------|
| IO1 | TX0 | Dipakai Serial Monitor output — menggunakan pin ini mengacaukan Serial |
| IO3 | RX0 | Dipakai Serial Monitor input — menggunakan pin ini mengacaukan Serial |

### ℹ️ Input-Only Pins

| GPIO | ADC Channel | Catatan |
|------|-------------|---------|
| IO34 | ADC1_CH6 | Hanya input, **tidak ada internal pull-up/pull-down** |
| IO35 | ADC1_CH7 | Hanya input, **tidak ada internal pull-up/pull-down** |
| IO36 (VP) | ADC1_CH0 | Hanya input |
| IO39 (VN) | ADC1_CH3 | Hanya input |

> Pin ini cocok untuk sensor analog (LDR, Potensiometer) karena **hanya perlu membaca** dan memiliki ADC1 yang tidak konflik dengan WiFi.

### ⚠️ ADC2 vs ADC1

| ADC | GPIO | WiFi Aktif? |
|-----|------|-------------|
| **ADC1** | IO32, IO33, IO34, IO35, IO36, IO39 | ✅ Bisa digunakan bersama WiFi |
| **ADC2** | IO0, IO2, IO4, IO12, IO13, IO14, IO15, IO25, IO26, IO27 | ❌ **TIDAK BISA** digunakan saat WiFi aktif |

> **SANGAT PENTING:** Jika project kamu menggunakan WiFi (BAB 42+), **gunakan hanya ADC1** (IO32–IO39) untuk membaca sensor analog.

---

## 6.4 GPIO yang Tersedia untuk Komponen Custom

Setelah mengurangi pin yang sudah terpakai dan yang memiliki restricsi:

### Tabel GPIO Tersedia

| GPIO | Fungsi yang Bisa | ADC | PWM | Touch | Catatan |
|------|-------------------|-----|-----|-------|---------|
| **IO2** | Digital I/O | ADC2_CH2 | ✅ | T2 | ⚠️ Strapping (harus LOW saat boot), LED built-in |
| **IO4** | Digital I/O | ADC2_CH0 | ✅ | T0 | ✅ Aman dipakai |
| **IO13** | Digital I/O | ADC2_CH4 | ✅ | T4 | ✅ Aman dipakai |
| **IO15** | Digital I/O | ADC2_CH3 | ✅ | T3 | ⚠️ Strapping, ada boot log |
| **IO16** | Digital I/O | — | ✅ | — | ✅ Aman dipakai, tidak ada ADC |
| **IO17** | Digital I/O | — | ✅ | — | ✅ Aman dipakai, tidak ada ADC |
| **IO25** | Digital I/O | ADC2_CH8 | ✅ | — | ✅ Memiliki DAC1 |
| **IO26** | Digital I/O | ADC2_CH9 | ✅ | — | ✅ Memiliki DAC2 |
| **IO32** | Digital I/O | ADC1_CH4 | ✅ | T9 | ✅ **ADC1 — aman dengan WiFi** |
| **IO33** | Digital I/O | ADC1_CH5 | ✅ | T8 | ✅ **ADC1 — aman dengan WiFi** |
| **IO34** | Input Only | ADC1_CH6 | ❌ | — | ✅ **ADC1 — aman dengan WiFi** |
| **IO35** | Input Only | ADC1_CH7 | ❌ | — | ✅ **ADC1 — aman dengan WiFi** |
| **IO36 (VP)** | Input Only | ADC1_CH0 | ❌ | — | ✅ **ADC1 — aman dengan WiFi** |
| **IO39 (VN)** | Input Only | ADC1_CH3 | ❌ | — | ✅ **ADC1 — aman dengan WiFi** |

**Total: 14 GPIO tersedia** untuk komponen custom.

---

## 6.5 Rekomendasi Pin untuk Komponen Custom

Berdasarkan analisis di atas, berikut **rekomendasi** pin untuk setiap komponen:

### Komponen Digital Output

| Komponen | Pin Rekomendasi | Alasan |
|----------|----------------|--------|
| LED 1 (Merah) | **IO4** | GPIO umum, aman, PWM tersedia |
| LED 2 (Kuning) | **IO16** | GPIO umum, aman |
| LED 3 (Hijau) | **IO17** | GPIO umum, aman |
| LED 4 (Biru) | **IO2** | GPIO built-in LED, double-purpose |
| Active Buzzer | **IO13** | GPIO umum, aman |
| Relay (via BC547) | **IO26** | GPIO umum, aman |

### Komponen Digital Input

| Komponen | Pin Rekomendasi | Alasan |
|----------|----------------|--------|
| Push Button 1 | **IO15** | GPIO umum (pull-down sudah di PCB) |
| Push Button 2 | **IO25** | GPIO umum (pull-down sudah di PCB) |
| PIR AM312 | **IO33** | ADC1, aman saat WiFi aktif |

### Komponen Analog Input

| Komponen | Pin Rekomendasi | Alasan |
|----------|----------------|--------|
| Potensiometer 10K | **IO34** | **ADC1_CH6** — aman saat WiFi aktif, input-only cocok untuk sensor |
| LDR | **IO35** | **ADC1_CH7** — aman saat WiFi aktif, input-only cocok untuk sensor |

### Komponen PWM / Khusus

| Komponen | Pin Rekomendasi | Alasan |
|----------|----------------|--------|
| Servo | **IO32** | PWM tersedia, ADC1 (tidak mengganggu WiFi) |
| HC-SR04 Trig | **IO25** | Digital output, GPIO umum (⚠️ shared dengan Button 2 — jangan pakai bersamaan) |
| HC-SR04 Echo | **IO36 (VP)** | Input-only, cocok untuk baca sinyal echo (⚠️ **WAJIB** pakai voltage divider! Echo = 5V) |

> **⚠️ PENTING:** HC-SR04 beroperasi pada 5V. Pin Echo menghasilkan sinyal 5V yang **harus diturunkan** ke 3.3V menggunakan voltage divider sebelum dihubungkan ke GPIO ESP32. Lihat BAB 3, bagian 3.2 untuk penjelasan voltage divider.

> **Catatan:** Rekomendasi di atas tidak mutlak — kamu bebas memilih pin lain selama memperhatikan restricsi. Rekomendasi ini dirancang untuk menghindari konflik antar komponen dan memaksimalkan kompatibilitas dengan WiFi.

---

## 6.6 Peta Visual — Seluruh Alokasi Pin

```
┌────────────────────────────────────────────────────────────────┐
│                  ESP32 DEVKIT V1 — Kit Bluino                  │
│                   Alokasi Pin Lengkap                          │
├────────────────────────────────────────────────────────────────┤
│                                                                │
│  PIN KIRI              STATUS              PIN KANAN           │
│  ─────────             ──────              ─────────           │
│  3V3 ................ Power ............... GND                 │
│  EN  ................ Reset ............... IO23 [MicroSD MOSI] │
│  VP (IO36) ......... ADC1/Input Only ...... IO22 [I2C SCL]     │
│  VN (IO39) ......... ADC1/Input Only ...... IO1  [TX0] ⛔      │
│  IO34 .............. ADC1/Input Only ...... IO3  [RX0] ⛔      │
│  IO35 .............. ADC1/Input Only ...... IO21 [I2C SDA]     │
│  IO32 .............. Tersedia ............. IO19 [MicroSD MISO] │
│  IO33 .............. Tersedia ............. IO18 [MicroSD CLK]  │
│  IO25 .............. Tersedia (DAC1) ...... IO5  [MicroSD CS]   │
│  IO26 .............. Tersedia (DAC2) ...... IO17 [Tersedia]     │
│  IO27 .............. [DHT11] .............. IO16 [Tersedia]     │
│  IO14 .............. [DS18B20] ............ IO4  [Tersedia]     │
│  IO12 .............. [WS2812] ⚠️ ......... IO2  [Tersedia] ⚠️  │
│  IO13 .............. Tersedia ............. IO15 [Tersedia] ⚠️  │
│  GND ................ Power ............... GND                 │
│  VIN ................ Power (5V) .......... 3V3                 │
│                                                                │
│  LEGENDA:                                                      │
│  [Nama]  = Hardwired (sudah terpakai)                          │
│  ⛔      = Dilarang (TX0/RX0 Serial)                           │
│  ⚠️      = Strapping pin (hati-hati)                           │
│  Tersedia = Bisa digunakan untuk komponen custom               │
└────────────────────────────────────────────────────────────────┘
```

---

## 📝 Latihan

1. **Identifikasi:** Dari schematic, sebutkan semua GPIO yang sudah hardwired dan fungsinya. Berapa sisa GPIO yang tersedia untuk custom?

2. **Pilih pin:** Kamu ingin menghubungkan:
   - 2 LED + 1 Buzzer + 1 LDR + 1 Push Button
   - Project akan menggunakan WiFi
   
   Pin mana yang kamu pilih untuk masing-masing komponen? Jelaskan alasannya (terutama untuk LDR yang membutuhkan ADC).

3. **Analisis konflik:** Jelaskan apa yang terjadi jika:
   - Kamu menggunakan IO6 untuk LED
   - Kamu membaca ADC di IO27 saat WiFi aktif
   - Kamu menghubungkan sinyal 5V ke IO34

4. **Quiz cepat:** TRUE atau FALSE:
   - IO34 bisa digunakan sebagai output LED → ?
   - IO25 bisa digunakan untuk analog read saat WiFi aktif → ?
   - IO21 tersedia untuk push button → ?
   - IO4 bisa digunakan untuk PWM → ?

---

## 📚 Referensi

- [ESP32 Pinout Reference — Random Nerd Tutorials](https://randomnerdtutorials.com/esp32-pinout-reference-gpios/)
- [ESP32 GPIO Restrictions](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html)
- [ESP32 ADC Limitations — Last Minute Engineers](https://lastminuteengineers.com/esp32-adc-tutorial/)

---

[⬅️ BAB 5: Setup Lingkungan](../BAB-05/README.md) | [Kembali ke Daftar Isi](../README.md) | [Selanjutnya: BAB 7 — Program Pertama ➡️](../BAB-07/README.md)
