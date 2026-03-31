# BAB 7: Program Pertama — Hello World

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami struktur dasar program Arduino (`setup()` dan `loop()`)
- Menggunakan Serial Monitor untuk komunikasi
- Menulis dan mengupload program pertama ke ESP32
- Memahami fungsi dasar: `pinMode()`, `digitalWrite()`, `delay()`, `Serial.print()`
- Menjalankan LED Blink pada ESP32

---

## 7.1 Struktur Program Arduino

Setiap program Arduino (disebut **sketch**) memiliki dua fungsi wajib:

```cpp
void setup() {
  // Dijalankan SEKALI saat ESP32 dinyalakan atau di-reset
  // Gunakan untuk: inisialisasi pin, Serial, sensor, WiFi
}

void loop() {
  // Dijalankan BERULANG-ULANG tanpa henti setelah setup() selesai
  // Gunakan untuk: logika utama program
}
```

### Alur Eksekusi

```
Power ON / Reset
       │
       ▼
   ┌────────┐
   │ setup()│ ← Dijalankan 1 kali
   └────┬───┘
        │
        ▼
   ┌────────┐
   │ loop() │ ← Dijalankan berulang
   └────┬───┘
        │
        └──→ Kembali ke loop() (selamanya)
```

> **Catatan teknis:** Di balik layar, Arduino framework membungkus `setup()` dan `loop()` dalam fungsi `main()` di Core 1. Core 0 menangani WiFi/BT.

---

## 7.2 Serial Monitor — Komunikasi dengan Komputer

Serial Monitor adalah "jendela" komunikasi antara ESP32 dan komputer melalui kabel USB. Ini adalah alat debugging paling penting.

### Membuka Serial Monitor

- **Arduino IDE:** Tombol kaca pembesar di pojok kanan atas, atau **Tools** → **Serial Monitor** (Ctrl+Shift+M)
- **Baud rate:** Set ke **115200** (harus sama dengan yang di kode)

### Program: Hello World

```cpp
/*
 * BAB 7 - Program 1: Hello World
 * Menampilkan pesan di Serial Monitor
 */

void setup() {
  Serial.begin(115200);  // Inisialisasi Serial dengan baud rate 115200
  delay(1000);           // Tunggu 1 detik agar Serial stabil

  Serial.println("=================================");
  Serial.println("  ESP32 - Hello World!");
  Serial.println("  Kit Bluino IoT Starter v3.2");
  Serial.println("=================================");
  Serial.println();
}

void loop() {
  Serial.println("ESP32 sedang berjalan...");
  delay(2000);  // Cetak setiap 2 detik
}
```

### Penjelasan Kode

| Baris | Fungsi |
|-------|--------|
| `Serial.begin(115200)` | Menginisialisasi komunikasi serial pada kecepatan 115200 baud |
| `delay(1000)` | Menunggu 1000 milidetik (1 detik) |
| `Serial.println("...")` | Mencetak teks ke Serial Monitor + newline |
| `Serial.print("...")` | Mencetak teks tanpa newline (lanjut di baris yang sama) |

### Perbedaan `print()` vs `println()`

```cpp
Serial.print("Suhu: ");     // Output: "Suhu: "
Serial.print(27.5);         // Output: "Suhu: 27.5"
Serial.println(" °C");      // Output: "Suhu: 27.5 °C\n" (dengan newline)
```

---

## 7.3 LED Blink — Program Klasik

LED Blink adalah program "Hello World" di dunia mikrokontroler. Kita akan menyalakan dan mematikan LED secara bergantian.

### Program: Blink LED Built-in

ESP32 DEVKIT V1 memiliki LED built-in yang biasanya terhubung ke **GPIO2**.

```cpp
/*
 * BAB 7 - Program 2: Blink LED Built-in
 * Mengedipkan LED built-in (GPIO2) setiap 1 detik
 */

#define LED_BUILTIN 2  // GPIO2 = LED biru pada board ESP32 DEVKIT V1

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);  // Set GPIO2 sebagai OUTPUT

  Serial.println("LED Blink dimulai!");
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);  // LED ON
  Serial.println("LED: ON");
  delay(1000);                      // Tunggu 1 detik

  digitalWrite(LED_BUILTIN, LOW);   // LED OFF
  Serial.println("LED: OFF");
  delay(1000);                      // Tunggu 1 detik
}
```

### Penjelasan Fungsi

| Fungsi | Parameter | Penjelasan |
|--------|-----------|------------|
| `pinMode(pin, mode)` | `pin`: nomor GPIO, `mode`: INPUT / OUTPUT / INPUT_PULLUP | Mengatur arah pin |
| `digitalWrite(pin, value)` | `pin`: nomor GPIO, `value`: HIGH / LOW | Menulis nilai digital |
| `delay(ms)` | `ms`: milidetik | Menunda eksekusi program (blocking) |

### Apa yang Terjadi di Level Hardware?

```
digitalWrite(2, HIGH)
       │
       ▼
GPIO2 output register = 1
       │
       ▼
Pin GPIO2 → 3.3V
       │
       ▼
Arus mengalir: GPIO2 → LED → Resistor → GND
       │
       ▼
LED ON 💡
```

---

## 7.4 Variasi Blink — Bermain dengan Timing

### Pattern 1: SOS (Morse Code)

```cpp
/*
 * BAB 7 - Program 3: SOS Morse Code
 * LED berkedip pola SOS: ··· −−− ···
 */

#define LED_PIN 2

void setup() {
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
  Serial.println("SOS Morse Code dimulai!");
}

// Fungsi helper
void dot() {
  digitalWrite(LED_PIN, HIGH);
  delay(200);   // Titik = pendek
  digitalWrite(LED_PIN, LOW);
  delay(200);   // Jeda antar sinyal
}

void dash() {
  digitalWrite(LED_PIN, HIGH);
  delay(600);   // Garis = panjang (3x titik)
  digitalWrite(LED_PIN, LOW);
  delay(200);   // Jeda antar sinyal
}

void loop() {
  // S = ···
  dot(); dot(); dot();
  delay(400);  // Jeda antar huruf

  // O = −−−
  dash(); dash(); dash();
  delay(400);

  // S = ···
  dot(); dot(); dot();
  delay(1400);  // Jeda antar kata
}
```

### Pattern 2: Fade (Dimming) — Preview PWM

```cpp
/*
 * BAB 7 - Program 4: LED Fade
 * LED menyala gradual (preview konsep PWM)
 * Menggunakan LEDC (LED Control) peripheral ESP32
 */

#define LED_PIN 2

// Konfigurasi LEDC
const int LEDC_FREQ = 5000;   // Frekuensi 5 KHz
const int LEDC_RESOLUTION = 8; // Resolusi 8-bit (0-255)

void setup() {
  Serial.begin(115200);

  // Setup LEDC peripheral
  ledcAttach(LED_PIN, LEDC_FREQ, LEDC_RESOLUTION);

  Serial.println("LED Fade dimulai!");
}

void loop() {
  // Terang bertahap (fade in)
  for (int duty = 0; duty <= 255; duty++) {
    ledcWrite(LED_PIN, duty);
    delay(5);
  }

  // Redup bertahap (fade out)
  for (int duty = 255; duty >= 0; duty--) {
    ledcWrite(LED_PIN, duty);
    delay(5);
  }
}
```

> **Catatan:** ESP32 menggunakan peripheral **LEDC** untuk PWM, bukan `analogWrite()` seperti Arduino Uno. Detail lengkap akan dibahas di BAB 12.

---

## 7.5 Informasi Chip ESP32

ESP32 menyediakan API untuk membaca informasi chip sendiri. Berguna untuk memastikan board bekerja:

```cpp
/*
 * BAB 7 - Program 5: Info ESP32
 * Membaca dan menampilkan informasi chip ESP32
 */

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=================================");
  Serial.println("    Informasi ESP32 Chip");
  Serial.println("=================================");

  // Model chip
  Serial.print("Chip Model   : ");
  Serial.println(ESP.getChipModel());

  // Revisi chip
  Serial.print("Chip Revision : ");
  Serial.println(ESP.getChipRevision());

  // Jumlah core
  Serial.print("CPU Cores     : ");
  Serial.println(ESP.getChipCores());

  // Frekuensi CPU
  Serial.print("CPU Frequency : ");
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println(" MHz");

  // Flash size
  Serial.print("Flash Size    : ");
  Serial.print(ESP.getFlashChipSize() / 1024 / 1024);
  Serial.println(" MB");

  // Free heap memory
  Serial.print("Free Heap     : ");
  Serial.print(ESP.getFreeHeap() / 1024);
  Serial.println(" KB");

  // SDK version
  Serial.print("SDK Version   : ");
  Serial.println(ESP.getSdkVersion());

  Serial.println("=================================");
}

void loop() {
  // Tidak ada yang perlu dilakukan
  delay(10000);
}
```

**Output yang diharapkan:**

```
=================================
    Informasi ESP32 Chip
=================================
Chip Model   : ESP32-D0WDQ6
Chip Revision : 1
CPU Cores     : 2
CPU Frequency : 240 MHz
Flash Size    : 4 MB
Free Heap     : 305 KB
SDK Version   : v4.4.x
=================================
```

---

## 7.6 Panduan Upload Program

### Langkah Upload

1. Hubungkan ESP32 via kabel Micro USB
2. Pilih **Board:** ESP32 Dev Module
3. Pilih **Port:** COMx (lihat Device Manager jika ragu)
4. Klik tombol **Upload** (ikon panah →)
5. Tunggu kompilasi dan upload selesai
6. Buka **Serial Monitor** (Ctrl+Shift+M)
7. Set baud rate ke **115200**

### Proses Upload

```
Kompilasi ──→ Connecting... ──→ Uploading ──→ Done!
  (10-30s)      (1-5s)          (5-15s)      
```

### Jika Upload Gagal

| Masalah | Solusi |
|---------|--------|
| "Failed to connect" | Tahan tombol **BOOT** pada ESP32, lalu klik Upload. Lepaskan BOOT setelah muncul "Connecting..." |
| Port tidak muncul | 1. Cek kabel (beberapa kabel micro USB hanya charging). 2. Instal driver (BAB 5). 3. Coba port USB lain |
| "No serial data received" | Pastikan baud rate Serial Monitor = 115200 |
| LED tidak berkedip | Cek pin LED — GPIO2 mungkin berbeda pada board tertentu |

---

## 7.7 Praktik Baik: Komentar & Dokumentasi Kode

Dari awal, biasakan menulis kode yang **terdokumentasi dengan baik**:

```cpp
/*
 * Nama Program : LED Blink dengan Status Serial
 * BAB          : 7
 * Deskripsi    : Mengedipkan LED dan menampilkan status di Serial Monitor
 * Hardware     : ESP32 DEVKIT V1 + Kit Bluino v3.2
 * Pin          : GPIO2 (LED built-in)
 * Author       : (nama kamu)
 * Tanggal      : (tanggal)
 */

// ============================================================
// KONFIGURASI PIN
// ============================================================
#define LED_PIN 2        // LED built-in ESP32 DEVKIT V1

// ============================================================
// KONSTANTA
// ============================================================
const int BLINK_DELAY = 1000;  // Delay dalam milidetik

// ============================================================
// SETUP
// ============================================================
void setup() {
  Serial.begin(115200);       // Inisialisasi Serial
  pinMode(LED_PIN, OUTPUT);   // Set LED sebagai output

  Serial.println("Program siap!");
}

// ============================================================
// LOOP UTAMA
// ============================================================
void loop() {
  // Nyalakan LED
  digitalWrite(LED_PIN, HIGH);
  Serial.println("LED: ON");
  delay(BLINK_DELAY);

  // Matikan LED
  digitalWrite(LED_PIN, LOW);
  Serial.println("LED: OFF");
  delay(BLINK_DELAY);
}
```

### Tips Dokumentasi

- ✅ Gunakan `#define` atau `const` untuk pin dan konstanta — mudah diubah
- ✅ Tulis header di awal file — deskripsi, pin, author
- ✅ Komentar pada bagian yang tidak jelas — bukan pada yang sudah jelas
- ❌ Jangan: `digitalWrite(2, HIGH); // menulis HIGH ke pin 2` (terlalu obvious)
- ✅ Lakukan: `digitalWrite(LED_PIN, HIGH); // Nyalakan LED indikator status`

---

## 📝 Latihan

1. **Hello Serial:** Buat program yang menampilkan nama kamu, tanggal, dan pesan "Saya sedang belajar ESP32!" di Serial Monitor saat pertama kali dinyalakan.

2. **Blink kustom:** Modifikasi program Blink agar LED menyala selama 200ms dan mati selama 800ms. Apa efek visualnya?

3. **Counter:** Buat program yang menampilkan angka berurutan dari 1 sampai 100 di Serial Monitor, dengan jeda 500ms antar angka. Setelah mencapai 100, mulai lagi dari 1.

4. **Info chip:** Jalankan Program 5 (info ESP32). Catat hasilnya:
   - Berapa CPU core pada board kamu?
   - Berapa free heap memory?
   - Bandingkan dengan spesifikasi di BAB 2.

5. **Morse Code:** Buat pola Morse Code nama depan kamu menggunakan LED. (Cari tabel Morse Code di internet)

---

## 📚 Referensi

- [Arduino Reference — Language](https://www.arduino.cc/reference/en/)
- [Serial.begin() — Arduino](https://www.arduino.cc/reference/en/language/functions/communication/serial/begin/)
- [digitalWrite() — Arduino](https://www.arduino.cc/reference/en/language/functions/digital-io/digitalwrite/)
- [ESP32 Arduino Core API](https://docs.espressif.com/projects/arduino-esp32/en/latest/api.html)

---

[⬅️ BAB 6: Pin Mapping](../BAB-06/README.md) | [Kembali ke Daftar Isi](../README.md) | [Selanjutnya: BAB 8 — Digital Output ➡️](../BAB-08/README.md)

---

## ☕ Dukung Proyek Ini

Bab ini disusun dengan dedikasi waktu dan keahlian yang tidak sedikit. Jika materi *engineering-grade* ini bermanfaat bagi riset atau proses belajarmu, bentuk apresiasi terbaik yang bisa kamu berikan adalah secangkir kopi untuk penulis:

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
Punya kritik, saran, atau butuh mentoring khusus ESP32 & IoT industri? Silakan sapa saya di:
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

