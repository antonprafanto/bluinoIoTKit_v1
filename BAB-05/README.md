# BAB 5: Setup Lingkungan Pengembangan

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Menginstal Arduino IDE dan mengkonfigurasi board ESP32
- Mengenal PlatformIO sebagai alternatif IDE profesional
- Menginstal driver USB-to-Serial untuk ESP32
- Menginstal library yang dibutuhkan
- Menggunakan Wokwi Simulator untuk simulasi tanpa hardware

---

## 5.1 Arduino IDE

### Langkah 1: Download & Install Arduino IDE

1. Buka [https://www.arduino.cc/en/software](https://www.arduino.cc/en/software)
2. Download **Arduino IDE 2.x** (versi terbaru) untuk sistem operasi kamu
3. Install seperti biasa

### Langkah 2: Tambahkan Board ESP32

ESP32 **tidak tersedia secara default** di Arduino IDE. Perlu ditambahkan:

1. Buka **Arduino IDE** → **File** → **Preferences** (atau **Ctrl+,**)
2. Pada field **"Additional Board Manager URLs"**, tambahkan URL berikut:

```
https://espressif.github.io/arduino-esp32/package_esp32_index.json
```

> **Tip:** Jika sudah ada URL lain, pisahkan dengan koma.

3. Klik **OK**
4. Buka **Tools** → **Board** → **Boards Manager...**
5. Cari **"esp32"** di kolom pencarian
6. Instal **"esp32 by Espressif Systems"** (versi terbaru)
7. Tunggu proses instalasi selesai (bisa memakan waktu beberapa menit)

### Langkah 3: Pilih Board

1. **Tools** → **Board** → **esp32** → **ESP32 Dev Module**
2. Pengaturan yang direkomendasikan:

| Setting | Nilai |
|---------|-------|
| Board | ESP32 Dev Module |
| Upload Speed | 921600 |
| CPU Frequency | 240MHz (WiFi/BT) |
| Flash Frequency | 80MHz |
| Flash Mode | QIO |
| Flash Size | 4MB (32Mb) |
| Partition Scheme | Default 4MB with spiffs |
| Port | (pilih port COM yang sesuai) |

### Langkah 4: Pilih Port

1. Hubungkan ESP32 ke komputer via **kabel Micro USB**
2. Buka **Tools** → **Port**
3. Pilih port COM yang muncul (biasanya **COM3**, **COM4**, atau lebih tinggi)

> **Windows:** Jika port tidak muncul, perlu install driver (lihat bagian 5.2)

---

## 5.2 Instalasi Driver USB-to-Serial

ESP32 DEVKIT V1 menggunakan chip USB-to-Serial untuk komunikasi dengan komputer. Ada 2 jenis chip yang umum:

### Identifikasi Chip

Lihat pada board ESP32 DEVKIT V1, cari IC kecil dekat konektor USB:
- **CP2102** (Silicon Labs) — chip persegi kecil bertuliskan "CP2102"
- **CH340** (WCH) — chip persegi bertuliskan "CH340"

### Download Driver

| Chip | Link Download | OS |
|------|---------------|----|
| CP2102 | [Silicon Labs CP2102 Driver](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers) | Windows, macOS, Linux |
| CH340 | [CH340 Driver (WCH)](http://www.wch.cn/download/CH341SER_EXE.html) | Windows |
| CH340 | [CH340 macOS Driver](https://github.com/adrianmihalko/ch340g-ch34g-ch34x-mac-os-x-driver) | macOS |

### Verifikasi Driver

Setelah install driver dan hubungkan ESP32:

**Windows:**
1. Buka **Device Manager** (Win+X → Device Manager)
2. Expand **Ports (COM & LPT)**
3. Harus terlihat: **"Silicon Labs CP210x"** atau **"USB-SERIAL CH340"** dengan nomor COM

**macOS/Linux:**
```bash
ls /dev/tty.* # macOS
ls /dev/ttyUSB* # Linux
```

---

## 5.3 Instalasi Library

Beberapa BAB memerlukan library tambahan. Berikut cara install dan daftar library yang akan digunakan:

### Cara Install Library

**Melalui Library Manager:**
1. **Sketch** → **Include Library** → **Manage Libraries...**
2. Cari nama library
3. Klik **Install**

**Melalui ZIP:**
1. Download file `.zip` library
2. **Sketch** → **Include Library** → **Add .ZIP Library...**
3. Pilih file ZIP yang didownload

### Library yang Dibutuhkan

Pastikan library berikut terinstal (tidak perlu sekaligus — instal saat dibutuhkan pada BAB masing-masing):

| Library | Fungsi | Digunakan di BAB |
|---------|--------|-------------------|
| DHT sensor library (Adafruit) | Baca DHT11 | BAB 26 |
| Adafruit Unified Sensor | Dependency DHT | BAB 26 |
| OneWire | Protokol OneWire | BAB 25, 27 |
| DallasTemperature | Baca DS18B20 | BAB 27 |
| Adafruit BMP085 Library | Baca BMP180 | BAB 28 |
| Adafruit MPU6050 | Baca MPU-6050 | BAB 29 |
| Adafruit SSD1306 | OLED display driver | BAB 35 |
| Adafruit GFX Library | Grafik untuk OLED | BAB 35 |
| Adafruit NeoPixel | WS2812 RGB LED | BAB 36 |
| ESP32Servo | Servo motor | BAB 38 |
| PubSubClient | MQTT client | BAB 50 |
| ArduinoJson | JSON parsing | BAB 48 |
| ESPAsyncWebServer | Web server async | BAB 46 |
| AsyncTCP | Dependency ESPAsyncWebServer | BAB 46 |
| WiFiManager (tzapu) | WiFi config portal | BAB 44 |
| UniversalTelegramBot | Telegram bot | BAB 61 |

> **Tip:** Jangan instal semua sekarang. Instal sesuai kebutuhan saat mengerjakan BAB terkait.

---

## 5.4 PlatformIO (Alternatif Profesional)

**PlatformIO** adalah IDE berbasis Visual Studio Code yang populer di kalangan developer embedded profesional.

### Kelebihan dibanding Arduino IDE

| Fitur | Arduino IDE | PlatformIO |
|-------|-------------|------------|
| Editor | Sederhana | VS Code (autocomplete, IntelliSense) |
| Build system | Sederhana | Profesional (CMake-based) |
| Library management | Manual | Otomatis via `platformio.ini` |
| Multi-project | Satu sketch | Multiple project support |
| Debugging | Serial saja | Serial + JTAG |
| Version control | Manual | Git terintegrasi |

### Cara Install

1. Install [Visual Studio Code](https://code.visualstudio.com/)
2. Buka VS Code → Extensions (Ctrl+Shift+X)
3. Cari **"PlatformIO IDE"** → Install
4. Tunggu instalasi selesai (butuh waktu)
5. Restart VS Code

### Membuat Project ESP32

1. Klik ikon PlatformIO (di sidebar kiri)
2. **New Project**
3. Board: **"Espressif ESP32 Dev Module"**
4. Framework: **Arduino**
5. Klik **Finish**

File `platformio.ini` akan terbuat otomatis:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
```

> **Di materi ini**, kita menggunakan **Arduino IDE** sebagai IDE utama. PlatformIO bisa digunakan sebagai alternatif tanpa mengubah kode program.

---

## 5.5 Wokwi Simulator

**Wokwi** adalah simulator online yang memungkinkan kamu menjalankan kode ESP32 **tanpa hardware fisik**.

### Cara Menggunakan

1. Buka [https://wokwi.com](https://wokwi.com)
2. Klik **"Start a new project"**
3. Pilih **ESP32**
4. Tulis kode, tambahkan komponen virtual
5. Klik **▶ Run** untuk menjalankan simulasi

### Komponen yang Tersedia di Wokwi

| Komponen | Tersedia? | Catatan |
|----------|-----------|---------|
| LED | ✅ | Warna bisa dipilih |
| Push Button | ✅ | — |
| Potensiometer | ✅ | — |
| DHT22 | ✅ | DHT11 juga tersedia |
| OLED SSD1306 | ✅ | I2C |
| Servo | ✅ | — |
| NeoPixel | ✅ | WS2812/SK6812 |
| Relay | ✅ | — |
| HC-SR04 | ✅ | — |
| DS18B20 | ✅ | — |
| BMP180/BME280 | ✅ | — |
| MPU-6050 | ✅ | — |
| MicroSD | ❌ | Tidak tersedia |
| PIR | ⚠️ | Terbatas |

### Keterbatasan Wokwi

- **WiFi/Bluetooth tidak bisa disimulasikan** (BAB 42+ perlu hardware asli)
- Timing mungkin sedikit berbeda dari hardware asli
- Tidak semua library tersedia
- Cocok untuk belajar dasar, **bukan** pengganti hardware

---

## 5.6 Test Koneksi — Upload Program Pertama

Sebelum lanjut ke BAB berikutnya, pastikan semuanya bekerja:

### Langkah-langkah

1. Hubungkan ESP32 ke komputer via USB
2. Buka Arduino IDE
3. Pilih Board: **ESP32 Dev Module**
4. Pilih Port yang sesuai
5. Buka **File** → **Examples** → **01.Basics** → **Blink**
6. Ubah `LED_BUILTIN` menjadi `2` (GPIO2 built-in LED):

```cpp
void setup() {
  pinMode(2, OUTPUT);  // GPIO2 = built-in LED
}

void loop() {
  digitalWrite(2, HIGH);
  delay(1000);         // LED ON selama 1 detik
  digitalWrite(2, LOW);
  delay(1000);         // LED OFF selama 1 detik
}
```

7. Klik **Upload** (→)
8. Tunggu sampai muncul **"Done uploading"**
9. LED biru kecil di board ESP32 harus berkedip

### Troubleshooting Upload Gagal

| Problem | Solusi |
|---------|--------|
| "A fatal error occurred: Failed to connect" | Tahan tombol **BOOT** saat upload, lepaskan setelah "Connecting..." muncul |
| Port tidak terdeteksi | Install driver USB-to-Serial (lihat 5.2). Coba kabel USB lain (beberapa kabel hanya charging) |
| "Permission denied" (Linux) | Jalankan: `sudo chmod a+rw /dev/ttyUSB0` |
| Upload sangat lambat | Turunkan Upload Speed ke **115200** |
| "Brownout detector was triggered" | Power supply kurang — gunakan kabel USB yang lebih pendek, atau USB port langsung (bukan hub) |

---

## 📝 Latihan

1. **Setup lengkap:** Install Arduino IDE, tambahkan board ESP32, upload program Blink. Pastikan LED berkedip.

2. **Serial Monitor:** Modifikasi program Blink agar juga menampilkan pesan "LED ON" dan "LED OFF" di Serial Monitor (baud rate 115200).

3. **Wokwi:** Buka [wokwi.com](https://wokwi.com), buat project ESP32 baru, dan jalankan simulasi Blink.

4. **Library:** Install library **Adafruit SSD1306** dan **Adafruit GFX** melalui Library Manager. Pastikan tidak ada error saat install.

---

## 📚 Referensi

- [Arduino IDE Setup for ESP32 — Random Nerd Tutorials](https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/)
- [PlatformIO Quick Start](https://docs.platformio.org/en/latest/integration/ide/vscode.html)
- [Wokwi ESP32 Simulator](https://wokwi.com)
- [CP2102 Driver — Silicon Labs](https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers)

---

[⬅️ BAB 4: Membaca Schematic](../BAB-04/README.md) | [Kembali ke Daftar Isi](../README.md) | [Selanjutnya: BAB 6 — Pin Mapping ➡️](../BAB-06/README.md)

---

## ☕ Dukung Proyek Ini

Bab ini disusun dengan dedikasi waktu dan keahlian yang tidak sedikit. Jika materi *engineering-grade* ini bermanfaat bagi riset atau proses belajarmu, bentuk apresiasi terbaik yang bisa kamu berikan adalah secangkir kopi untuk penulis:

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
Punya kritik, saran, atau butuh mentoring khusus ESP32 & IoT industri? Silakan sapa saya di:
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

