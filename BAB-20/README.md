# BAB 20: Memory Management

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami arsitektur memori ESP32 (Flash, SRAM, PSRAM)
- Membedakan Stack dan Heap
- Memonitor penggunaan memori secara real-time
- Menghindari memory leak dan fragmentasi
- Memilih antara `String` dan `char[]` dengan tepat
- Menggunakan `PROGMEM` dan `F()` untuk menghemat RAM

---

## 20.1 Arsitektur Memori ESP32

```
┌──────────────────────────────────────────────────┐
│                   FLASH (4 MB)                    │
│  ┌──────────┬──────────┬───────────┬───────────┐ │
│  │Bootloader│ Partisi  │  Program  │ SPIFFS/   │ │
│  │  (64KB)  │  Table   │  (.bin)   │ LittleFS  │ │
│  └──────────┴──────────┴───────────┴───────────┘ │
│  → Non-volatile (tetap saat mati)                 │
│  → Read-only saat runtime (kecuali NVS/SPIFFS)    │
├──────────────────────────────────────────────────┤
│                   SRAM (520 KB)                   │
│  ┌─────────────────┬────────────────────────────┐│
│  │   IRAM (128 KB) │     DRAM (320+ KB)         ││
│  │   Instruksi     │  ┌───────┐  ┌───────────┐ ││
│  │   cepat, ISR    │  │ Stack │  │   Heap    │ ││
│  │   (IRAM_ATTR)   │  │(8KB/  │  │ (dinamis) │ ││
│  │                 │  │ task) │  │ ~160-300KB│ ││
│  │                 │  └───────┘  └───────────┘ ││
│  └─────────────────┴────────────────────────────┘│
│  → Volatile (hilang saat mati)                    │
├──────────────────────────────────────────────────┤
│                RTC SRAM (16 KB)                   │
│  → Bertahan saat deep sleep                       │
│  → Digunakan untuk: RTC_DATA_ATTR variabel       │
└──────────────────────────────────────────────────┘
```

### Ringkasan

| Memori | Ukuran | Sifat | Digunakan Untuk |
|--------|--------|-------|-----------------|
| Flash | 4 MB | Non-volatile, read-only | Program, file system, konstanta |
| DRAM | ~320 KB | Volatile | Variabel global, heap, stack |
| IRAM | 128 KB | Volatile, cepat | ISR, kode kritis kecepatan |
| RTC SRAM | 16 KB | Bertahan saat deep sleep | Variabel yang perlu survive deep sleep |

---

## 20.2 Stack vs Heap

### Stack (Otomatis)

```cpp
void fungsi() {
  int x = 10;           // Stack — dialokasi otomatis
  char buffer[100];     // Stack — dialokasi otomatis
  float arr[50];        // Stack — dialokasi otomatis
}                       // Semua dibebaskan otomatis saat fungsi selesai
```

**Karakteristik Stack:**
- Cepat (alokasi = geser pointer)
- Ukuran tetap per task (~8 KB default di FreeRTOS)
- Otomatis dibebaskan saat fungsi keluar
- ⚠️ **Stack overflow** jika terlalu banyak variabel lokal atau rekursi dalam

### Heap (Manual/Dinamis)

```cpp
void fungsi() {
  int* arr = (int*)malloc(100 * sizeof(int));  // Heap — alokasi manual
  // Gunakan arr...
  free(arr);                                    // WAJIB dibebaskan manual!

  // Atau C++ style:
  int* arr2 = new int[100];                    // Heap
  delete[] arr2;                               // WAJIB
}
```

**Karakteristik Heap:**
- Fleksibel (ukuran bisa runtime)
- Lebih lambat dari stack
- ⚠️ **Memory leak** jika lupa `free()`/`delete`
- ⚠️ **Fragmentasi** setelah banyak alloc/free

---

## 20.3 Program: Memory Monitor

```cpp
/*
 * BAB 20 - Program 1: Memory Monitor
 * Menampilkan penggunaan memori ESP32 secara real-time
 */

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("=== ESP32 Memory Report ===");
  Serial.println();

  // Total heap (semua DRAM yang tersedia untuk program)
  Serial.print("Total Heap        : ");
  Serial.print(ESP.getHeapSize() / 1024);
  Serial.println(" KB");

  // Free heap saat ini
  Serial.print("Free Heap         : ");
  Serial.print(ESP.getFreeHeap() / 1024);
  Serial.println(" KB");

  // Free heap minimum sejak boot (watermark)
  Serial.print("Min Free Heap     : ");
  Serial.print(ESP.getMinFreeHeap() / 1024);
  Serial.println(" KB");

  // Blok terbesar yang bisa dialokasi sekaligus
  Serial.print("Max Alloc Block   : ");
  Serial.print(ESP.getMaxAllocHeap() / 1024);
  Serial.println(" KB");

  // Flash
  Serial.print("Flash Size        : ");
  Serial.print(ESP.getFlashChipSize() / 1024 / 1024);
  Serial.println(" MB");

  // Sketch size
  Serial.print("Sketch Size       : ");
  Serial.print(ESP.getSketchSize() / 1024);
  Serial.println(" KB");

  Serial.print("Free Sketch Space : ");
  Serial.print(ESP.getFreeSketchSpace() / 1024);
  Serial.println(" KB");

  Serial.println();
  Serial.println("=== Real-time monitoring ===");
}

void loop() {
  Serial.print("Free Heap: ");
  Serial.print(ESP.getFreeHeap());
  Serial.print(" bytes | Min: ");
  Serial.print(ESP.getMinFreeHeap());
  Serial.println(" bytes");

  delay(5000);
}
```

**Output contoh:**
```
=== ESP32 Memory Report ===

Total Heap        : 328 KB
Free Heap         : 305 KB
Min Free Heap     : 301 KB
Max Alloc Block   : 113 KB
Flash Size        : 4 MB
Sketch Size       : 236 KB
Free Sketch Space : 1536 KB
```

---

## 20.4 String vs char[] — Pertarungan Memori

### `String` (Arduino String Class)

```cpp
String nama = "ESP32";           // Heap-allocated
String pesan = "Suhu: " + String(27.5) + " °C";  // Heap — multiple allocation!
```

**Kelebihan:** Mudah digunakan, operator `+`, konversi otomatis  
**Kekurangan:** Fragmentasi heap, memory leak potensial, tidak deterministic

### `char[]` (C-String)

```cpp
char nama[] = "ESP32";           // Stack — efisien
char pesan[50];
snprintf(pesan, sizeof(pesan), "Suhu: %.1f °C", 27.5);  // Aman, terkontrol
```

**Kelebihan:** Deterministik, tidak fragmentasi, efisien  
**Kekurangan:** Perlu atur ukuran manual, operasi string lebih verbose

### Perbandingan

| Aspek | `String` | `char[]` |
|-------|---------|---------|
| Kemudahan | ✅ Sangat mudah | ⚠️ Perlu belajar |
| Efisiensi memori | ❌ Fragmentasi | ✅ Efisien |
| Cocok untuk | Prototyping cepat | Produksi / long-running |
| Runtime stabil | ❌ Bisa crash setelah jam | ✅ Stabil |
| Program pendek | ✅ OK | Berlebihan |
| Program yang berjalan lama | ❌ Hindari | ✅ Wajib |

### Contoh Konversi

```cpp
// ❌ String (heap fragmentation)
String buildMessage(float suhu, int hum) {
  String msg = "Suhu: ";
  msg += String(suhu, 1);
  msg += " °C, Kelembaban: ";
  msg += String(hum);
  msg += "%";
  return msg;  // Banyak alokasi heap!
}

// ✅ char[] (stack, efisien)
void buildMessage(char* buffer, size_t bufLen, float suhu, int hum) {
  snprintf(buffer, bufLen, "Suhu: %.1f °C, Kelembaban: %d%%", suhu, hum);
}

// Penggunaan:
char msg[64];
buildMessage(msg, sizeof(msg), 27.5, 65);
Serial.println(msg);
```

---

## 20.5 Program: Demonstrasi Memory Leak

```cpp
/*
 * BAB 20 - Program 2: Memory Leak Demo
 * Menunjukkan bagaimana String bisa menghabiskan memori
 *
 * ⚠️ Program ini SENGAJA dibuat cacat untuk demonstrasi!
 */

int counter = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("BAB 20: Memory Leak Demo");
  Serial.print("Free heap awal: ");
  Serial.println(ESP.getFreeHeap());
}

void loop() {
  counter++;

  // ❌ BURUK: String concat di loop — fragmentasi heap
  String pesan = "Counter: " + String(counter);
  pesan += " | Uptime: " + String(millis() / 1000) + "s";
  pesan += " | Free: " + String(ESP.getFreeHeap());

  Serial.println(pesan);

  // ✅ BANDINGKAN: cara yang benar
  // char buf[80];
  // snprintf(buf, sizeof(buf), "Counter: %d | Uptime: %lus | Free: %u",
  //          counter, millis() / 1000, ESP.getFreeHeap());
  // Serial.println(buf);

  delay(100);  // Cepat agar leak terlihat
}
```

> Jalankan program ini dan perhatikan nilai "Free" yang perlahan **menurun**. Kemudian uncomment versi `snprintf` dan comment versi `String` — free heap akan **stabil**.

---

## 20.6 `F()` Macro dan PROGMEM

### `F()` — Simpan String di Flash

Tanpa `F()`, string literal disalin ke RAM:

```cpp
Serial.println("Pesan panjang ini menggunakan RAM");         // ❌ Pakai RAM
Serial.println(F("Pesan panjang ini tetap di Flash"));       // ✅ Hemat RAM
```

### PROGMEM — Simpan Data Besar di Flash

```cpp
// Lookup table sine wave — terlalu besar untuk RAM
const uint8_t PROGMEM sineTable[256] = {
  128, 131, 134, 137, ...
};

// Baca dari PROGMEM
uint8_t value = pgm_read_byte(&sineTable[i]);
```

> **Tip:** Gunakan `F()` untuk **semua** `Serial.println()` yang berisi string konstan, terutama di program besar.

---

## 20.7 Tips Manajemen Memori ESP32

### Checklist

```
┌──────────────────────────────────────────────────────────┐
│                TIPS MEMORI ESP32                           │
│                                                           │
│  ✅ Monitor free heap secara periodik                     │
│  ✅ Gunakan char[] daripada String untuk long-running     │
│  ✅ snprintf() daripada concat String                     │
│  ✅ Alokasi buffer di awal (setup), bukan berulang        │
│  ✅ Gunakan F() untuk Serial.print string konstan         │
│  ✅ Array lokal besar? Pertimbangkan static atau global   │
│  ✅ Cek getMinFreeHeap() — alert jika < 10 KB             │
│                                                           │
│  ❌ Hindari malloc/new di loop()                           │
│  ❌ Hindari String concat berulang                         │
│  ❌ Hindari rekursi dalam (stack overflow)                 │
│  ❌ Hindari char buffer[10000] di fungsi (stack overflow)  │
└──────────────────────────────────────────────────────────┘
```

### Batas Aman

| Kondisi | Nilai | Tindakan |
|---------|-------|----------|
| Free heap > 50 KB | ✅ Aman | Normal |
| Free heap 20–50 KB | ⚠️ Waspada | Optimasi diperlukan |
| Free heap < 20 KB | 🔴 Bahaya | WiFi bisa gagal, crash |
| Free heap < 10 KB | 💀 Kritis | ESP32 hampir pasti crash |

---

## 20.8 RTC Memory — Bertahan Saat Deep Sleep

```cpp
// Variabel ini bertahan saat deep sleep
RTC_DATA_ATTR int bootCount = 0;

void setup() {
  Serial.begin(115200);
  bootCount++;

  Serial.print("Boot ke-");
  Serial.println(bootCount);

  // Deep sleep 10 detik
  esp_sleep_enable_timer_wakeup(10 * 1000000);  // µs
  esp_deep_sleep_start();
}

void loop() {
  // Tidak pernah sampai sini karena deep sleep
}
```

> **RTC SRAM** (16 KB) tersedia melalui atribut `RTC_DATA_ATTR`. Data akan bertahan saat deep sleep tapi hilang saat power cycle penuh.

---

## 📝 Latihan

1. **Memory benchmark:** Buat program yang mengalokasi String dengan ukuran bertambah (10 char, 100 char, 1000 char, ...) dan catat free heap setelah setiap alokasi. Di ukuran berapa ESP32 mulai kehabisan memori?

2. **String vs char[] race:** Buat dua versi program yang mengirim pesan Serial 1000 kali — satu pakai String concat, satu pakai snprintf. Bandingkan free heap sebelum dan sesudah.

3. **Stack overflow test:** Buat fungsi rekursif (memanggil dirinya sendiri). Berapa kedalaman rekursi sebelum ESP32 crash? Tambahkan pengecekan `uxTaskGetStackHighWaterMark()` untuk mendeteksi.

4. **Memory-safe sensor logger:** Buat program yang membaca potensiometer (ADC) setiap 1 detik dan menyimpan 100 pembacaan terakhir di circular buffer (array tetap, bukan dynamic). Tampilkan rata-rata dan min/max saat diminta via Serial.

---

## 📚 Referensi

- [ESP32 Memory Types — Espressif](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/memory-types.html)
- [Arduino String vs char — Majenko](https://majenko.co.uk/blog/evils-arduino-strings)
- [ESP32 Heap Memory — Random Nerd Tutorials](https://randomnerdtutorials.com/esp32-useful-wi-fi-functions-arduino/)
- [PROGMEM — Arduino Reference](https://www.arduino.cc/reference/en/language/variables/utilities/progmem/)

---

[⬅️ BAB 19: Kode Terstruktur](../BAB-19/README.md) | [Kembali ke Daftar Isi](../README.md) | [Selanjutnya: BAB 21 — Debugging ➡️](../BAB-21/README.md)

---

## ☕ Dukung Proyek Ini

Bab ini disusun dengan dedikasi waktu dan keahlian yang tidak sedikit. Jika materi *engineering-grade* ini bermanfaat bagi riset atau proses belajarmu, bentuk apresiasi terbaik yang bisa kamu berikan adalah secangkir kopi untuk penulis:

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
Punya kritik, saran, atau butuh mentoring khusus ESP32 & IoT industri? Silakan sapa saya di:
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

