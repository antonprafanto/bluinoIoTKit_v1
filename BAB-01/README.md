# BAB 1: Apa itu IoT & Embedded Systems

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami konsep dasar Internet of Things (IoT)
- Membedakan mikrokontroler dan mikroprosesor
- Menjelaskan arsitektur sistem IoT
- Mengidentifikasi aplikasi IoT di berbagai bidang

---

## 1.1 Apa itu Internet of Things (IoT)?

**Internet of Things (IoT)** adalah konsep dimana perangkat fisik (benda/things) terhubung ke internet, saling berkomunikasi, mengumpulkan data, dan dapat dikendalikan dari jarak jauh.

### Contoh Sederhana

Bayangkan sebuah sensor suhu di rumahmu:
1. **Sensor** membaca suhu ruangan setiap 5 menit
2. **Mikrokontroler** (ESP32) memproses data suhu tersebut
3. **WiFi** mengirim data ke internet (cloud)
4. **Smartphone** kamu menampilkan suhu ruangan secara real-time
5. Jika suhu > 35°C, ESP32 **menyalakan kipas** lewat relay secara otomatis

Inilah IoT — benda biasa (sensor suhu + kipas) menjadi "pintar" karena terhubung ke internet.

### Definisi Formal

> _"Internet of Things adalah jaringan perangkat fisik yang dilengkapi sensor, aktuator, dan konektivitas, memungkinkan perangkat tersebut mengumpulkan dan bertukar data melalui internet."_

---

## 1.2 Arsitektur Sistem IoT

Setiap sistem IoT terdiri dari **4 lapisan utama**:

```
┌─────────────────────────────────────────────┐
│              4. APLIKASI                     │
│     Dashboard, Mobile App, Notifikasi        │
├─────────────────────────────────────────────┤
│              3. CLOUD / SERVER               │
│     Penyimpanan, Analisis, Machine Learning   │
├─────────────────────────────────────────────┤
│              2. JARINGAN / KONEKTIVITAS       │
│     WiFi, Bluetooth, LoRa, MQTT, HTTP        │
├─────────────────────────────────────────────┤
│              1. PERANGKAT (DEVICE)            │
│     Sensor + Mikrokontroler + Aktuator       │
│     (ESP32 + DHT11 + Relay)                  │
└─────────────────────────────────────────────┘
```

### Lapisan 1 — Perangkat (Device Layer)

Ini adalah **lapisan fisik** — tempat sensor membaca kondisi lingkungan dan aktuator melakukan tindakan.

**Contoh pada kit Bluino:**
| Kategori | Komponen | Fungsi |
|----------|----------|--------|
| Sensor | DHT11 | Membaca suhu & kelembaban |
| Sensor | BMP180 | Membaca tekanan udara |
| Sensor | PIR AM312 | Mendeteksi gerakan manusia |
| Sensor | LDR | Mengukur intensitas cahaya |
| Aktuator | Relay | Menghidupkan/matikan perangkat listrik |
| Aktuator | Buzzer | Menghasilkan bunyi alarm |
| Aktuator | LED | Indikator visual |
| Pemroses | ESP32 | Otak dari seluruh sistem |

### Lapisan 2 — Jaringan (Network Layer)

Bagaimana data dikirim dari perangkat ke cloud:

| Teknologi | Jangkauan | Data Rate | Contoh Penggunaan |
|-----------|-----------|-----------|-------------------|
| WiFi | ~30m indoor / ~100m outdoor | Tinggi (Mbps) | Smart home, dashboard web |
| Bluetooth/BLE | ~10m | Sedang (Kbps–Mbps) | Wearable, kontrol jarak dekat |
| ESP-NOW | ~200m+ | Sedang (Kbps) | Komunikasi antar ESP32 |
| LoRa | ~10km | Rendah (Kbps) | Pertanian, monitoring jarak jauh |

> **Catatan:** MQTT dan HTTP adalah **protokol aplikasi** (berjalan di atas WiFi/internet), bukan media transmisi. Mereka akan dibahas di BAB selanjutnya.

> **Kit Bluino** dilengkapi **WiFi + Bluetooth** bawaan pada ESP32, sehingga langsung bisa digunakan untuk komunikasi nirkabel.

### Lapisan 3 — Cloud / Server

Data yang dikirim perangkat disimpan dan diproses di server:
- **Firebase** — Realtime database dari Google
- **ThingSpeak** — Platform IoT dengan visualisasi MATLAB
- **Blynk** — Mobile IoT platform
- **MQTT Broker** — Server perantara untuk protokol MQTT

### Lapisan 4 — Aplikasi

Tampilan untuk pengguna akhir:
- **Dashboard web** — Grafik dan kontrol di browser
- **Aplikasi mobile** — Kontrol dari smartphone
- **Notifikasi** — Alert via Telegram, email, push notification

---

## 1.3 Mikrokontroler vs Mikroprosesor

### Apa itu Mikrokontroler?

**Mikrokontroler** adalah komputer kecil dalam satu chip (System on Chip / SoC) yang berisi:
- CPU (prosesor)
- RAM (memori kerja)
- Flash (memori penyimpanan program)
- GPIO (pin input/output)
- Peripheral (ADC, DAC, Timer, UART, I2C, SPI)

### Perbandingan

| Aspek | Mikrokontroler | Mikroprosesor |
|-------|---------------|---------------|
| **Contoh** | ESP32, Arduino, STM32 | Intel Core, AMD Ryzen, ARM Cortex-A |
| **RAM** | KB – MB | GB |
| **Kecepatan** | MHz (80–240 MHz) | GHz (1–5 GHz) |
| **Sistem Operasi** | Tidak ada / RTOS | Windows, Linux, macOS |
| **Konsumsi Daya** | µW – mW (sangat rendah) | Watt (tinggi) |
| **GPIO** | ✅ Langsung tersedia | ❌ Butuh tambahan hardware |
| **Harga** | Rp 30.000 – Rp 150.000 | Rp 1.000.000++ |
| **Cocok untuk** | IoT, sensor, kontrol | Desktop, server, AI |

### Mengapa Mikrokontroler untuk IoT?

1. **Konsumsi daya rendah** — Bisa dijalankan dari baterai berminggu-minggu
2. **Murah** — Satu ESP32 hanya ~Rp 50.000
3. **GPIO langsung** — Bisa langsung baca sensor tanpa tambahan hardware
4. **Real-time** — Respon cepat tanpa overhead sistem operasi
5. **Ukuran kecil** — Bisa ditanam di mana saja

---

## 1.4 Aplikasi IoT di Dunia Nyata

### 🏠 Smart Home
- Lampu otomatis berdasarkan deteksi gerakan (PIR → Relay)
- AC/kipas otomatis berdasarkan suhu (DHT11 → Relay)
- Monitoring keamanan rumah (PIR → Notifikasi Telegram)
- Kontrol perangkat dari smartphone

### 🌾 Smart Agriculture
- Monitoring kelembaban tanah
- Irigasi otomatis
- Pemantauan cuaca lokal (suhu, kelembaban, tekanan udara)
- Data logging ke cloud untuk analisis musiman

### 🏭 Industrial IoT (IIoT)
- Monitoring getaran mesin (akselerometer MPU-6050)
- Deteksi overheat pada peralatan (DS18B20)
- Tracking aset dan inventaris
- Predictive maintenance

### 🏥 Kesehatan
- Monitoring suhu tubuh pasien
- Wearable fitness tracker
- Sistem alarm jatuh (akselerometer)
- Telemedicine device

### 🚗 Transportasi
- Fleet tracking (GPS + MQTT)
- Monitoring kondisi kendaraan (CAN Bus)
- Smart parking system (ultrasonik HC-SR04)

### 🌆 Smart City
- Monitoring kualitas udara
- Smart street lighting (LDR → LED)
- Manajemen sampah pintar
- Monitoring banjir (ultrasonik)

---

## 1.5 Posisi Kit Bluino dalam Ekosistem IoT

Kit yang kamu gunakan memiliki **semua komponen dasar** untuk membuat sistem IoT lengkap:

```
┌───────────────────────────────────────────────┐
│              Kit Bluino ESP32                  │
├───────────┬───────────────┬───────────────────┤
│  SENSOR   │   PEMROSES    │ AKTUATOR & OUTPUT │
│           │               │                   │
│  DHT11    │               │    4x LED         │
│  DS18B20  │               │    RGB WS2812     │
│  BMP180   │   ESP32       │    Buzzer         │
│  MPU-6050 │   (Dual Core) │    Relay          │
│  PIR      │   (WiFi)      │    Servo          │
│  LDR      │   (Bluetooth) │                   │
│  HC-SR04  │               │  DISPLAY:         │
│  Pot/Touch│               │    OLED 128x64    │
├───────────┴───────────────┴───────────────────┤
│           PENYIMPANAN: MicroSD + NVS          │
└───────────────────────────────────────────────┘
```

Dengan kit ini, kamu akan belajar **seluruh rantai IoT** — dari membaca sensor, memproses data, menampilkan informasi, hingga mengirim ke internet.

---

## 📝 Latihan

1. **Identifikasi IoT di sekitarmu:** Sebutkan 3 perangkat IoT yang kamu temui sehari-hari dan jelaskan 4 lapisan arsitektur IoT-nya.

2. **Rancang sistem IoT sederhana:** Dengan komponen kit Bluino, rancang sebuah sistem monitoring suhu ruangan yang:
   - Membaca suhu setiap 10 detik
   - Menampilkan di OLED
   - Mengirim notifikasi jika suhu > 35°C
   - Menyalakan kipas (relay) secara otomatis
   
   Gambarkan diagram arsitekturnya (sensor → proses → output → cloud).

3. **Mikrokontroler vs Mikroprosesor:** Mengapa kita menggunakan ESP32 (mikrokontroler) dan bukan Raspberry Pi (mikroprosesor) untuk membaca sensor suhu setiap 5 menit dan mengirim ke cloud? Sebutkan minimal 3 alasan.

---

## 📚 Referensi

- [Espressif — ESP32 Overview](https://www.espressif.com/en/products/socs/esp32)
- [What is IoT? — AWS](https://aws.amazon.com/what-is/iot/)
- [IoT Architecture — IBM](https://www.ibm.com/topics/internet-of-things)

---

[⬅️ Kembali ke Daftar Isi](../README.md) | [Selanjutnya: BAB 2 — Pengenalan ESP32 ➡️](../BAB-02/README.md)
