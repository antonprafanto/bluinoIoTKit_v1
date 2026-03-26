ESP32 IoT Starter Kit merupakan kit yang cocok untuk pengenalan kepada teknologi Internet of Thing (IoT) dengan mudah.
Dilengkapi dengan board ESP32 DEVKIT V1 dan beragam komponen yang ada didalam kit ini kamu dapat membangun berbagai proyek IoT yang dapat terhubung dengan internet web dan handphone melalui WiFi.

Spesifikasi:
4 x LED Merah, Kuning, Hijau, Biru
1 x OLED 0.96" I2C 128x64
1 x DHT11 (Sensor Suhu & Kelembaban)
1 x Micro SD adapter
1 x MPU-6050 sensor Accelerometer Gyroscope
1 x DS18B20 sensor suhu
1 x Mini PIR sensor AM312
1 x BMP180 sensor Barometrik
1 x Active Buzzer
1 x Potensiometer 10K ohm
2 x Push Button
1 x Relay
1 x LDR

Kelengkapan kit:
1 x ESP32 IoT Starter Kit
1 x Board ESP32 DEVKIT V1
1 x Adapter USB OTG
1 x Micro USB Cable
10 x Dupont FF 10cm

https://www.tokopedia.com/bluino/esp32-starter-shield-modul-belajar-iot-kit-wifi-bluetooth-1731230521580291235

### **Buku Catatan Skematik: ESP32 Starter Shield 3.2**

#### **1. Sistem Inti dan Manajemen Daya (Core & Power Management)**

- **Mikrokontroler Utama:** ESP32 DEVKIT V1. Berfungsi sebagai pusat pemrosesan utama dengan kapabilitas WiFi dan Bluetooth bawaan (_System on a Chip_).
- **Regulator Tegangan (Power Regulator):**
  - Menggunakan IC **78M05** (Linear Voltage Regulator) untuk menurunkan tegangan input eksternal (misal: 12V dari adaptor) menjadi 5V DC yang stabil.
  - Dilengkapi dioda **1N4001** pada jalur input untuk proteksi terhadap polaritas terbalik (_reverse polarity protection_).
  - Terdapat kapasitor elektrolit (C1:100uF/6V) dan kapasitor keramik (C2) untuk _filtering_ dan menstabilkan tegangan output 5V.
- **Sirkuit Auto-Reset / Flashing:** Terdapat kapasitor **0.1µF** yang dipasang antara pin **EN** (Enable) ESP32 dan **GND**. Ini membantu proses auto-reset saat mengunggah kode dari IDE.
- **Header Ekspansi Daya:** Tersedia deretan pin header (1-15) yang mengekspos jalur 3V3, 5V, dan GND untuk memudahkan distribusi daya ke sensor eksternal jika diperlukan.

#### **2. Filosofi Desain Pembelajaran: Sistem "Custom" Patching**

- Modul ini tidak dirangkai secara _hardwired_ penuh ke pin IO ESP32. Banyak komponen (LDR, Buzzer, Servo, HC-SR04, Relay, PIR, Push Button, Potensiometer) berakhir pada pin header berlabel **"Custom"**.
- **Nilai Edukasi:** Desain ini memaksa peserta didik untuk menggunakan kabel _jumper_ eksternal untuk menghubungkan pin "Custom" ke IO ESP32 pilihan mereka. Hal ini melatih pemahaman tata letak pin (Pinout), pengenalan fungsi pin (ADC, PWM, Digital I/O), dan kemampuan _wiring_ dasar.

#### **3. Modul Komunikasi & Bus Terintegrasi**

- **I2C Bus (Inter-Integrated Circuit):** Pin IO21 (SDA) dan IO22 (SCL) digunakan secara paralel (shared bus) untuk beberapa modul sekaligus:
  - OLED Display 128x64
  - Sensor MPU6050 (Accelerometer & Gyroscope)
  - Sensor BME280/BMP280 (Suhu & Tekanan Udara)
- **SPI Bus (Serial Peripheral Interface):** Digunakan secara eksklusif untuk **Micro SD Adapter**. Konfigurasi pin: CS (IO5), MOSI (IO23), CLK (IO18), MISO (IO19). Berguna untuk materi _data logging_.
- **1-Wire Bus:** Digunakan untuk sensor suhu DS18B20 pada IO14.

#### **4. Desain Modul Input (Sensor & Saklar)**

Sistem ini telah dilengkapi komponen pendukung (_pull-up/pull-down_) terintegrasi pada PCB untuk memudahkan siswa:

- **Digital Input (Lingkungan):**
  - **DHT11/DHT22** (Suhu & Kelembaban): Terhubung ke IO27, sudah dilengkapi resistor _pull-up_ $4K7\Omega$.
  - **DS18B20** (Suhu Presisi): Terhubung ke IO14, sudah dilengkapi resistor _pull-up_ $4K7\Omega$.
  - **PIR AM312:** Sensor gerak inframerah (Output digital via pin Custom).
  - **HC-SR04:** Sensor jarak ultrasonik (Pin Trigger & Echo via pin Custom).
- **Digital Input (Mekanik & Sentuh):**
  - **2x Push Button:** Menggunakan konfigurasi _pull-down_ resistor $10K\Omega$. Pin akan bernilai LOW saat dilepas, dan HIGH (3V3) saat ditekan.
  - **Touch:** Satu pad sentuh yang langsung terhubung ke sistem sensor kapasitif internal ESP32.
- **Analog Input (ADC):**
  - **Potensiometer $10K\Omega$:** Terhubung sebagai pembagi tegangan (GND ke 3V3) untuk menghasilkan output tegangan analog linier.
  - **LDR (Light Dependent Resistor):** Dirangkai secara seri dengan resistor tetap $10K\Omega$ membentuk sirkuit pembagi tegangan untuk mengukur intensitas cahaya.

#### **5. Desain Modul Output (Aktuator & Indikator)**

- **Indikator Visual Dasar:** 4 buah LED 5mm yang masing-masing dipasangi resistor pembatas arus $330\Omega$ menuju GND. Membutuhkan sinyal HIGH untuk menyala (_active-high_).
- **Indikator Visual Pintar:** **RGB LED WS2812** (Neopixel) yang dikendalikan melalui satu jalur data digital (Pin D12 / IO12). Membutuhkan catu daya 5V dan level logika 3V3 yang dikonversi (atau langsung dari ESP32 tergantung toleransi).
- **Audio:** **Active Buzzer** (Membunyikan nada statis saat diberi tegangan tinggi).
- **Mekanik:** Header **Servo Pin** menyediakan jalur 5V, GND, dan satu pin sinyal (membutuhkan PWM) untuk menggerakkan motor servo.
- **Pengendali Daya Tinggi:** **Relay 5V DC**. Modul ini dilengkapi dengan sirkuit _driver_ yang sangat baik untuk dipelajari:
  - Menggunakan transistor **BC547 (NPN)** sebagai saklar elektronik untuk memicu koil relay.
  - Terdapat dioda **1N4148** yang dipasang paralel dengan koil relay secara _reverse bias_. Ini adalah **dioda flyback** yang berfungsi melindungi transistor dari lonjakan tegangan (_voltage spike_) saat koil relay dimatikan.
  - Dilengkapi LED indikator (dengan resistor $1K\Omega$) yang menyala saat relay aktif.

---
