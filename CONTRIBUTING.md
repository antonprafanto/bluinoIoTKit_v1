# 🤝 Panduan Kontribusi & Gaya Penulisan

Dokumen ini adalah "aturan main" penulisan materi di repositori ini. Tujuannya satu: **setiap BAB harus bisa diikuti oleh pemula total** — orang yang baru pertama kali memegang ESP32 — tanpa bingung harus membuka aplikasi apa, menyambung kabel ke mana, atau mengetik perintah di mana.

Kalau kamu ingin memperbaiki *typo*, menambah penjelasan, atau mengirim kode, ikuti panduan ini supaya semua BAB tetap seragam.

---

## 1. Prinsip Utama

1. **Ramah awam.** Anggap pembaca belum tahu apa-apa. Setiap istilah teknis dijelaskan saat pertama kali muncul di BAB itu, sebaiknya dengan analogi sehari-hari.
2. **Runut.** Materi dibangun bertahap: konsep → contoh kecil → program lengkap → latihan. Jangan memakai konsep yang belum diajarkan tanpa penjelasan singkat atau tautan ke BAB yang membahasnya.
3. **Bisa langsung dipraktikkan.** Setiap program punya langkah menjalankan yang lengkap: rangkaian, *library*, pengaturan Arduino IDE, cara membuka alat uji, dan contoh hasil yang seharusnya muncul.
4. **Akurat dan teruji.** Semua program lengkap **wajib lolos *compile*** dengan lingkungan acuan di bawah. Klaim teknis (tegangan, arus, kecepatan, batasan) harus benar.
5. **Santai tapi baku.** Bahasa akrab (pakai "kamu"), tetap mengikuti EYD.

---

## 2. Lingkungan Acuan

Semua kode di repositori ini ditulis dan diuji dengan:

| Komponen | Versi / Pengaturan |
|---|---|
| Arduino IDE | 2.x (versi terbaru) |
| *Board package* | **esp32 by Espressif Systems** versi **3.3.x** (diuji dengan 3.3.12) |
| *Board* yang dipilih | **ESP32 Dev Module** (FQBN `esp32:esp32:esp32`) |
| Baud rate Serial | **115200** |
| *Partition scheme* | *Default* — kecuali disebut lain (misalnya **Huge APP** untuk program besar) |

> ⚠️ **Perhatian:** Banyak fungsi berubah antara core 2.x dan 3.x. Jangan menulis kode khusus core 2.x. Lihat [bagian 8](#8-aturan-kode-program) untuk daftar perubahan yang paling sering bikin *error*.

### *Library* yang dipakai di kurikulum

Tuliskan nama *library* **persis seperti yang muncul di Library Manager**, lengkap dengan pembuatnya:

| Nama di Library Manager | Pembuat | Dipakai untuk |
|---|---|---|
| DHT sensor library | Adafruit | DHT11 |
| Adafruit Unified Sensor | Adafruit | Dependensi DHT & MPU6050 |
| OneWire | Paul Stoffregen dkk. | Protokol OneWire |
| DallasTemperature | Miles Burton dkk. | DS18B20 |
| Adafruit BMP085 Library | Adafruit | BMP180 |
| Adafruit MPU6050 | Adafruit | MPU-6050 |
| Adafruit SSD1306 | Adafruit | OLED |
| Adafruit GFX Library | Adafruit | Grafik OLED |
| FastLED | Daniel Garcia | WS2812 (NeoPixel) |
| ESP32Servo | Kevin Harrington, John K. Bennett | Servo |
| ArduinoJson (versi 7) | Benoit Blanchon | JSON |
| WebSockets | Markus Sattler | WebSocket |
| PubSubClient | Nick O'Leary | MQTT |
| WiFiManager | tzapu | Portal konfigurasi WiFi |
| CoAP simple library | Hirotaka Niisato | CoAP |
| painlessMesh | Coopdis dkk. | ESP-MESH |

*Library* bawaan core (tidak perlu diinstal): `WiFi`, `WebServer`, `HTTPClient`, `WiFiClientSecure`, `ESPmDNS`, `Preferences`, `LittleFS`, `SPIFFS`, `SD`, `SPI`, `Wire`, `BluetoothSerial`, `BLE`, `esp_now`, `Matter`.

---

## 3. Isi Kit dan Komponen Tambahan

Kit **Bluino ESP32 IoT Starter Shield v3.2** sudah berisi: ESP32 DEVKIT V1, OLED 0,96", DHT11, DS18B20, BMP180, MPU-6050, RGB LED WS2812, adaptor microSD, *touch pad*, 4 LED, 2 tombol, *buzzer* aktif, potensiometer 10 kΩ, LDR, sensor PIR AM312, *relay* 5 V, konektor HC-SR04, konektor servo, kabel USB, adaptor USB OTG, dan 10 kabel *jumper* *female-female*.

> 💡 **Tips:** Kabel USB harus kabel **data**, bukan kabel yang hanya bisa untuk mengisi daya. Kalau *port* ESP32 tidak muncul di Arduino IDE, coba ganti kabel dulu.

Komponen berikut **tidak termasuk kit** — tulis dengan jelas di bagian "Yang Perlu Disiapkan" kalau sebuah BAB membutuhkannya:

- Sensor ultrasonik **HC-SR04** (kit hanya menyediakan konektornya) dan **2 resistor** untuk pembagi tegangan pin Echo.
- **Motor servo** (misalnya SG90).
- **Kartu microSD** (kit hanya menyediakan adaptornya).
- **ESP32 kedua/ketiga** (untuk ESP-NOW, ESP-MESH, Bluetooth antar-ESP32).
- **ESP32-C6 / ESP32-H2** (untuk Thread).
- Aplikasi di HP, akun layanan *cloud*, dan sebagainya.

---

## 4. Peta Pin Standar

Pakai peta pin ini di **semua** BAB supaya pembaca tidak perlu bongkar-pasang kabel setiap ganti BAB. Kalau sebuah program terpaksa memakai pin lain, jelaskan alasannya.

### Sudah terpasang di PCB (tidak perlu kabel)

| Komponen | Pin ESP32 |
|---|---|
| OLED SSD1306 (alamat I2C `0x3C`) | SDA = IO21, SCL = IO22 |
| BMP180 (alamat I2C `0x77`) | SDA = IO21, SCL = IO22 |
| MPU-6050 (alamat I2C `0x68`) | SDA = IO21, SCL = IO22 |
| DHT11 (*pull-up* 4,7 kΩ di PCB) | IO27 |
| DS18B20 (*pull-up* 4,7 kΩ di PCB) | IO14 |
| RGB LED WS2812 | IO12 |
| microSD (SPI) | CS = IO5, MOSI = IO23, CLK = IO18, MISO = IO19 |

### Pin "Custom" (disambung dengan kabel *jumper*)

| Komponen | Pin ESP32 | Catatan |
|---|---|---|
| LED Merah | IO4 | Aktif HIGH (menyala saat HIGH) |
| LED Kuning | IO16 | Aktif HIGH |
| LED Hijau | IO17 | Aktif HIGH |
| LED Biru | IO2 | Aktif HIGH; IO2 juga terhubung ke LED biru bawaan *board* |
| *Buzzer* aktif | IO13 | Aktif HIGH |
| *Relay* | IO26 | Aktif HIGH (lewat transistor BC547) |
| Tombol S1 (SW1) | IO15 | Ditekan = HIGH (*pull-down* 10 kΩ di PCB) → pakai `INPUT` |
| Tombol S2 (SW2) | IO25 | Ditekan = HIGH → pakai `INPUT` |
| Sensor PIR AM312 | IO33 | HIGH saat ada gerakan |
| *Touch pad* | IO33 (T8) | Bergantian dengan PIR — pasang salah satu saja |
| Potensiometer | IO34 | ADC1, aman dipakai bersama WiFi |
| LDR | IO35 | ADC1; **makin gelap → nilai ADC makin besar** (LDR di sisi GND) |
| Servo (sinyal) | IO32 | Daya servo dari header servo (5 V) |
| HC-SR04 Trig | IO25 | Bergantian dengan tombol S2 |
| HC-SR04 Echo | IO36 (VP) | **Wajib lewat pembagi tegangan** (Echo = 5 V, ESP32 maks. 3,3 V) |

Cara menyambung: kabel *jumper* *female-female* dari pin header bertanda **Custom** ke pin header ESP32 dengan nomor yang sama (misalnya label `D4`/`IO4`).

---

## 5. Struktur Setiap BAB

Setiap `BAB-XX/README.md` mengikuti urutan berikut. Bagian bertanda ★ wajib ada.

```markdown
# BAB N: Judul Bab                                   ★ (format persis: "BAB N: ")

> ✅ **Prasyarat:** Selesaikan **BAB X (Judul)** ...  ★

Paragraf pembuka 2–4 kalimat: apa yang akan dipelajari, kenapa penting,
dan contoh nyata/analogi sehari-hari.                ★

## 📌 Tujuan Pembelajaran                            ★
## 🧰 Yang Perlu Disiapkan                           ★ (tabel: hardware, software, library, alat uji, jaringan)
## 📑 Daftar Isi                                     ★ untuk BAB yang panjang (> 300 baris)
## N.1 ... N.x  (materi dan program)                 ★
## N.y Troubleshooting (masalah → penyebab → solusi)
## 📝 Rangkuman                                      ★
## 💪 Latihan                                        ★ (bertingkat: dasar → menengah → lanjutan)
## 📚 Referensi                                      ★
```

> 📌 **Catatan:** Pakai emoji persis seperti di atas. Emoji yang mengandung *variation selector* (misalnya ⚠️, ⚙️, 🏋️) membuat anchor judul di GitHub berisi karakter tak terlihat sehingga tautan Daftar Isi bisa rusak.

**Footer** (navigasi ⬅️/➡️ dan bagian "☕ Dukung Proyek Ini") dibuat seragam oleh script — jangan menulis atau mengubahnya secara manual dan jangan menaruh konten apa pun setelahnya.

### Penomoran judul

- Judul materi diberi nomor: `## 8.1 Judul`, `### 8.1.1 Subjudul` (opsional). Tanpa emoji.
- Bagian tetap (Tujuan, Yang Perlu Disiapkan, Daftar Isi, Rangkuman, Latihan, Referensi) memakai emoji seperti contoh di atas, tanpa nomor.

### Daftar Isi

Cukup cantumkan judul level `##`. Anchor dibuat mengikuti aturan GitHub: huruf kecil, emoji dan tanda baca (kecuali `-` dan `_`) dibuang, spasi diganti `-`.

| Judul | Anchor |
|---|---|
| `## 8.2 Program 1: LED Berkedip` | `#82-program-1-led-berkedip` |
| `## 📝 Rangkuman` | `#-rangkuman` |

Selalu cek dengan `python tools/check_links.py BAB-08`.

---

## 6. Format Setiap Program

Setiap program lengkap ditulis dengan pola yang sama:

1. **Judul:** `## N.x Program K: Nama Program`
2. **Tujuan:** 1–3 kalimat — apa yang dilakukan program ini.
3. **Rangkaian:** tabel kabel *jumper* (dari pin **Custom** apa ke IO berapa), atau tulis *"Tidak perlu merangkai — komponen sudah terpasang di kit."* Sertakan gambar skematik kit jika membantu (lihat [bagian 10](#10-gambar-dan-diagram)).
4. **Cara Menjalankan:** langkah bernomor, misalnya:
   1. Sambungkan kabel sesuai tabel.
   2. Buka Arduino IDE, salin kode di bawah (atau buka file `BAB-08/p1_led_berkedip/p1_led_berkedip.ino`).
   3. Instal *library* **X** lewat **Library Manager** (kalau perlu).
   4. Pilih *board* **ESP32 Dev Module** dan *port*-nya.
   5. Klik **Upload** dan tunggu sampai muncul tulisan **Done uploading**.
   6. Buka **Serial Monitor**, pilih baud **115200**.
5. **Kode Program:** diawali penanda *sketch* (lihat [bagian 9](#9-penanda-sketch-dan-file-ino)).
6. **Penjelasan Kode:** bagian penting dijelaskan bertahap, bukan baris per baris secara kaku.
7. **Contoh Output:** blok ```` ```text ```` berisi tampilan Serial Monitor / layar OLED / halaman web yang seharusnya muncul.
8. **Kalau Belum Berhasil:** 2–4 penyebab paling umum beserta solusinya.

### Alat uji: selalu sebutkan di mana mengetik dan apa yang dibuka

Pemula sering bingung "perintah ini diketik di mana?". Setiap kali memberi perintah atau sintaks, sebutkan **alatnya**, **cara membukanya**, dan **hasil yang diharapkan**.

| Kebutuhan | Alat yang disarankan | Cara membuka / catatan |
|---|---|---|
| Melihat pesan dari ESP32 | **Serial Monitor** Arduino IDE | **Tools → Serial Monitor** (Ctrl+Shift+M), baud **115200** |
| Mengirim perintah teks ke ESP32 | Kolom pesan di Serial Monitor | Pilih *line ending* **New Line** atau **Both NL & CR** (sebutkan yang mana), ketik perintah, tekan Enter |
| Melihat grafik nilai sensor | **Serial Plotter** | **Tools → Serial Plotter** |
| Membuka halaman web ESP32 | *Browser* di laptop/HP | Laptop/HP harus terhubung ke WiFi yang **sama** dengan ESP32 |
| Mengirim permintaan HTTP | `curl` di **Command Prompt** (Windows) / **Terminal** (macOS/Linux), atau [Hoppscotch](https://hoppscotch.io) | Di Windows buka: tombol Start → ketik `cmd` → Enter. Di PowerShell lama, ketik `curl.exe` (bukan `curl`) |
| MQTT | [MQTT Explorer](https://mqtt-explorer.com) (aplikasi desktop) | Alternatif berbasis web: HiveMQ WebSocket Client |
| Bluetooth Classic | Aplikasi **Serial Bluetooth Terminal** (Android) | iPhone **tidak mendukung** Bluetooth Classic SPP |
| BLE | Aplikasi **nRF Connect for Mobile** (Android/iOS) | Alternatif: LightBlue |
| Perintah Python (`pip`, `python`) | Command Prompt / Terminal | Jelaskan cara cek apakah Python sudah terpasang |

Untuk perintah `curl` yang mengirim JSON, berikan **dua versi**: versi Command Prompt Windows (tanda kutip ganda di-*escape*) dan versi macOS/Linux.

Setiap BAB yang memakai WiFi wajib mengingatkan: **ESP32 hanya mendukung WiFi 2,4 GHz** (bukan 5 GHz), dan nama/kata sandi WiFi peka huruf besar-kecil.

---

## 7. Bahasa dan EYD

Gaya bahasa: **santai, akrab, tetapi baku**. Pakai "kamu" (bukan "Anda" atau "kalian"). Kalimat pendek, satu ide per kalimat, kalimat aktif.

### Kata baku yang sering salah

| ❌ Tidak baku | ✅ Baku |
|---|---|
| silahkan | silakan |
| praktek | praktik |
| analisa | analisis |
| resiko | risiko |
| merubah, rubah | mengubah, ubah |
| ijin | izin |
| aktifitas, efektifitas, sensitifitas | aktivitas, efektivitas, sensitivitas |
| standart | standar |
| tehnik | teknik |
| obyek, subyek | objek, subjek |
| sekedar | sekadar |
| antri | antre |
| jaman | zaman |
| mempengaruhi | memengaruhi |
| mengkonfigurasi, mengkompilasi, mengkoneksikan | mengonfigurasi, mengompilasi, menghubungkan |
| menginstall, terinstall | menginstal, terinstal |
| saklar | sakelar |
| konkrit, ekstrim, metoda | konkret, ekstrem, metode |
| nomer | nomor |
| apapun, siapapun, kapanpun, satupun | apa pun, siapa pun, kapan pun, satu pun |

### Aturan penulisan

- **"di" dan "ke"**: dipisah bila menunjukkan tempat/arah (*di dalam, di atas, di sini, di mana, ke luar*), disambung bila awalan kata kerja (*dijalankan, ditulis*).
- **Partikel "pun"**: dipisah (*apa pun, satu pun*), kecuali bentuk tetap (*meskipun, walaupun, adapun, bagaimanapun, ataupun, maupun*).
- **Istilah asing** ditulis miring di kalimat narasi: *board*, *library*, *upload*, *file*, *browser*, *pull-up*. Pengecualian (ditulis tegak):
  - nama diri/merek/produk: ESP32, Arduino IDE, WiFi, Bluetooth, Google Home;
  - nama menu/tombol aplikasi → **tebal** sesuai label aslinya: **Upload**, **Serial Monitor**, **Tools → Port**;
  - singkatan: LED, GPIO, PWM, I2C, HTTP;
  - kode dan nama fungsi → `backtick`: `digitalWrite()`, `WiFi.begin()`;
  - kata yang sudah baku di KBBI: data, sensor, digital, analog, internet, modul, protokol, kabel, program, kode, memori, monitor, variabel, fungsi, parameter, resistor, transistor, kapasitor, dioda.
- **Kata berimbuhan dari bahasa asing**: utamakan padanan Indonesia yang lazim (*mengunggah*, *mengunduh*, *memasang*, *menyambungkan*). Kalau tetap memakai kata asing, tulis dengan tanda hubung dan miring: di-*upload*, me-*reset*.
- **Singkatan** dijelaskan saat pertama muncul di setiap BAB: "PWM (*Pulse Width Modulation*, modulasi lebar pulsa)".
- **Angka dan satuan** di teks narasi: koma untuk desimal dan spasi sebelum satuan → 3,3 V; 2,4 GHz; 10 kΩ; 100 mA; 25 °C. Di dalam kode, versi (3.3.12), alamat IP, dan nilai yang dicetak program, biarkan apa adanya (pakai titik).
- **Judul** memakai huruf kapital di awal setiap kata, kecuali kata tugas (*dan, di, ke, dari, yang, untuk, dengan*) yang tidak berada di awal.
- Hindari klaim berlebihan ("paling sempurna", "dijamin 100%") dan huruf kapital semua untuk penekanan — pakai **tebal**.

### Kotak catatan (*callout*)

Gunakan format yang seragam:

```markdown
> 💡 **Tips:** ...
> 📌 **Catatan:** ...
> ⚠️ **Perhatian:** ...
> 🚨 **Bahaya:** ...   (khusus risiko keselamatan, misalnya listrik PLN 220 V)
> 🧪 **Coba Sendiri:** ...
```

---

## 8. Aturan Kode Program

- Wajib lolos *compile* dengan esp32 core **3.3.x**. Pakai API core 3.x:

| Core 2.x (jangan dipakai) | Core 3.x (pakai ini) |
|---|---|
| `ledcSetup(ch, f, res)` + `ledcAttachPin(pin, ch)` | `ledcAttach(pin, f, res)` |
| `ledcWrite(channel, duty)` | `ledcWrite(pin, duty)` |
| `ledcWriteTone(channel, f)` | `ledcWriteTone(pin, f)` |
| `timerBegin(0, 80, true)` | `timerBegin(1000000)` (frekuensi dalam Hz) |
| `timerAttachInterrupt(t, fn, true)` | `timerAttachInterrupt(t, fn)` |
| `timerAlarmWrite()` + `timerAlarmEnable()` | `timerAlarm(t, nilai, autoreload, 0)` |
| `hallRead()` | **Dihapus** di core 3.x |
| `esp_task_wdt_init(timeout, panic)` | `esp_task_wdt_init(&config)` |
| Callback ESP-NOW terima `(const uint8_t *mac, ...)` | `(const esp_now_recv_info_t *info, const uint8_t *data, int len)` |
| Callback ESP-NOW kirim `(const uint8_t *mac, status)` | `(const wifi_tx_info_t *info, esp_now_send_status_t status)` (core ≥ 3.3) |
| BLE `std::string v = c->getValue();` | `String v = c->getValue();` |
| ArduinoJson 6 `StaticJsonDocument<N>` / `DynamicJsonDocument` | ArduinoJson 7 `JsonDocument` |

- Komentar kode dalam bahasa Indonesia, singkat dan jelas. Indentasi 2 spasi.
- `Serial.begin(115200);` di awal `setup()`.
- Kredensial selalu contoh: `"NamaWiFiKamu"` dan `"PasswordWiFiKamu"` dengan komentar `// ← ganti dengan WiFi kamu`. Jangan pernah menulis kata sandi atau token asli.
- Pin mengikuti [Peta Pin Standar](#4-peta-pin-standar) dan didefinisikan di bagian atas kode (`#define LED_MERAH 4` atau `const int`).
- Hindari `delay()` panjang di program yang perlu responsif. Kalau `delay()` dipakai demi kesederhanaan di BAB awal, jelaskan keterbatasannya.
- Potongan kode (bukan program lengkap) boleh tanpa penanda *sketch*, tetapi harus tetap benar secara sintaks dan memakai API core 3.x.

---

## 9. Penanda Sketch dan File `.ino`

`README.md` adalah **satu-satunya sumber kode**. File `.ino` di folder `BAB-XX/pK_nama/` dibuat otomatis dari README, jadi jangan mengedit file `.ino` secara langsung.

Tandai setiap program lengkap dengan komentar HTML tepat sebelum blok kodenya:

````markdown
<!-- sketch: p1_led_berkedip -->
```cpp
void setup() { ... }
void loop() { ... }
```
````

- Nama: `p` + nomor program + `_` + nama singkat huruf kecil (hanya huruf, angka, garis bawah).
- Program multi-file: `<!-- sketch: p3_kode_modular/config.h -->` untuk file tambahan.
- Opsi khusus: `<!-- sketch: p2_matter_light fqbn=esp32:esp32:esp32:PartitionScheme=huge_app -->`.
- Kode yang bukan untuk Arduino IDE (misalnya contoh ESP-IDF): `<!-- sketch: nama nocompile -->` — sebaiknya dihindari.

Perintah:

```bash
python tools/sketch_tool.py list BAB-08      # lihat daftar sketch
python tools/sketch_tool.py extract BAB-08   # tulis ulang file .ino dari README
python tools/sketch_tool.py check            # pastikan semua .ino sama dengan README
python tools/sketch_tool.py compile BAB-08   # compile (butuh arduino-cli + core esp32)
```

---

## 10. Gambar dan Diagram

Gambar dipakai untuk **memperjelas**, bukan hiasan. Setiap gambar harus berhubungan langsung dengan paragraf di sekitarnya.

### Sumber gambar yang boleh dipakai

1. **Skematik dan foto kit Bluino.** Potongan skematik per komponen tersedia di folder [`assets/kit/`](assets/kit/) (misalnya `skematik-relay.png`). Sumber: skematik ESP32 Starter Shield 3.2, Bluino (bluino.com).
2. **Wikimedia Commons**, dengan lisensi domain publik, CC0, CC BY, atau CC BY-SA. Catat pembuat, lisensi, dan alamat halaman berkasnya.
3. **Dokumentasi resmi berlisensi terbuka**, misalnya dokumentasi ESP-IDF (Apache-2.0) atau dokumentasi Arduino (CC BY-SA 4.0). Pastikan lisensinya mengizinkan penggunaan ulang.
4. **Diagram buatan sendiri**: Mermaid (dirender otomatis oleh GitHub), diagram ASCII, atau gambar SVG/PNG yang dibuat khusus untuk materi ini.

**Dilarang:** gambar hasil pencarian Google tanpa lisensi jelas, gambar bertanda air (*watermark*), dan menautkan langsung (*hotlink*) gambar dari situs lain — selalu unduh ke repositori.

### Cara menaruh gambar

- Simpan di `BAB-XX/img/` dengan nama deskriptif huruf kecil, misalnya `BAB-08/img/rangkaian-led.png`. Lebar maksimum ±1200 piksel, ukuran berkas sebaiknya < 400 KB.
- Format penulisan (teks alternatif menjelaskan isi gambar, lalu keterangan dan sumber):

```markdown
![Skematik relay pada kit Bluino: pin Custom → resistor 10 kΩ → basis transistor BC547](../assets/kit/skematik-relay.png)

*Gambar 37.1 — Rangkaian driver relay pada kit. Sumber: skematik ESP32 Starter Shield 3.2, [Bluino](http://www.bluino.com).*
```

- Gambar dari pihak lain: `Sumber: [Nama Pembuat](URL halaman berkas), lisensi CC BY-SA 4.0.`
- Gambar/diagram buatan sendiri: `Sumber: dibuat untuk materi ini.`
- Nomori gambar per BAB: Gambar 8.1, Gambar 8.2, dan seterusnya.

### Diagram

- **Diagram ASCII** boleh dipertahankan, tetapi garis kotaknya harus rapi (lurus). Jangan menaruh emoji di dalam kotak ASCII karena lebarnya berbeda dan merusak kerapian.
- **Mermaid** cocok untuk diagram alur, urutan komunikasi (*sequence diagram*), dan *state machine*. Buat sederhana (maksimal ±12 kotak), label berbahasa Indonesia, dan beri tanda kutip pada label yang mengandung tanda baca, misalnya `A["Baca sensor (DHT11)"]`.

---

## 11. Alat Pengecekan

Sebelum mengirim perubahan, jalankan:

```bash
python tools/check_links.py          # tautan, anchor, dan gambar
python tools/sketch_tool.py check    # file .ino sinkron dengan README
python tools/check_style.py          # format BAB dan kata tidak baku
```

GitHub Actions juga menjalankan pengecekan ini dan meng-*compile* semua program secara otomatis setiap ada perubahan.
