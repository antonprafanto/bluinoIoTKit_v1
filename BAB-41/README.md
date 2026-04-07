# BAB 41: MicroSD Card — Data Logging Eksternal ESP32

> ✅ **Prasyarat:** Selesaikan **BAB 24 (SPI — Serial Peripheral Interface)**, **BAB 39 (NVS)**, dan **BAB 40 (LittleFS)**. Modul **Micro SD Adapter** pada kit Bluino terhubung permanen ke bus SPI di pin IO5/IO18/IO19/IO23.

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami **kapan memilih MicroSD vs LittleFS vs NVS** untuk penyimpanan data IoT.
- Melakukan **inisialisasi SD Card** via bus SPI menggunakan `SD.h` bawaan Arduino ESP32.
- Membuat, membaca, menulis, dan menghapus **file di SD Card** dengan API yang akrab.
- Membangun **sistem data logger multi-sensor profesional** ke format CSV.
- Menerapkan strategi **rolling log** untuk mencegah file membengkak tanpa batas.
- Membaca dan menulis **file konfigurasi JSON** menggunakan library ArduinoJson.
- Memahami **batasan teknis**: kecepatan SPI, kompatibilitas FAT, dan tip troubleshooting.

---

## 41.1 Hierarki Penyimpanan — Kapan Pakai MicroSD?

```
PANDUAN MEMILIH JENIS PENYIMPANAN (Update Lengkap):

  Jenis Data          │ Ukuran     │ Rekomendasi
  ────────────────────┼────────────┼─────────────────────────────────────
  Setting/konfigurasi │ < 4 KB     │ NVS (Preferences.h)  ← BAB 39
  File teks/JSON      │ 4KB – 1MB  │ LittleFS (Flash int) ← BAB 40
  Log data IoT        │ 1MB – 4MB  │ LittleFS (jika muat)
  Log jangka panjang  │ > 4MB      │ MicroSD Card ← BAB INI ✅
  Data multimedia     │ MB – GB    │ MicroSD Card ← BAB INI ✅
  ────────────────────┴────────────┴─────────────────────────────────────

KAPASITAS PERBANDINGAN:
  ┌──────────────┬────────────┬──────────────────────────────────────┐
  │ Media        │ Kapasitas  │ Catatan                              │
  ├──────────────┼────────────┼──────────────────────────────────────┤
  │ NVS Flash    │ ~20 KB     │ Key-value, cepat, tahan reboot       │
  │ LittleFS     │ ~1.5 MB    │ File system, manusiawi, terformat    │
  │ MicroSD 8GB  │ 8.000 MB   │ 5.333× lebih besar dari LittleFS!   │
  │ MicroSD 32GB │ 32.000 MB  │ Untuk logging bertahun-tahun         │
  └──────────────┴────────────┴──────────────────────────────────────┘

KEPUTUSAN DESAIN:
  Gunakan MicroSD bila:
  ✅ Data log > 1 MB (sensor multi-channel, logging cepat)
  ✅ Perlu diambil fisik (card dicabut, dibaca di PC)
  ✅ Kapasitas harus mudah diperluas (ganti card)
  ✅ Data perlu dibuka langsung di Excel/PC

  Tetap di LittleFS bila:
  ✅ Data < 500 KB dan perangkat di lokasi sulit dijangkau
  ✅ Ingin zero-dependency (tidak butuh komponen tambahan)
```

---

## 41.2 Koneksi Hardware MicroSD pada Kit Bluino

Kit Bluino telah menyediakan koneksi **hardwired** antara Micro SD Adapter dan ESP32 via bus **SPI2 (HSPI)**:

```
KONEKSI SPI MICROSD PADA KIT BLUINO (PERMANEN — TIDAK PERLU KABEL):

  MicroSD Adapter Pin │ ESP32 Pin │ Fungsi SPI
  ────────────────────┼───────────┼────────────────────────────────────
  CS  (Chip Select)   │ IO5       │ Memilih slave SPI yang aktif
  MOSI (Master→Slave) │ IO23      │ Data dari ESP32 ke SD Card
  CLK  (Clock)        │ IO18      │ Sinyal clock SPI
  MISO (Slave→Master) │ IO19      │ Data dari SD Card ke ESP32
  VCC                 │ 5V        │ Tegangan operasi adapter
  GND                 │ GND       │ Ground bersama
  ────────────────────┴───────────┴────────────────────────────────────

⚠️ CATATAN PENTING:
  - Pin IO5 adalah strapping pin. Pastikan SD Card sudah tersambung
    SEBELUM boot / upload, atau beri pull-up 10KΩ pada CS agar
    IO5 bernilai HIGH saat boot (SPI slave tidak aktif).
  - Library SD.h menggunakan SPI default (VSPI: CLK=18, MISO=19, MOSI=23)
    sehingga konfigurasi kit Bluino sudah sesuai langsung.
  - Gunakan SD Card format FAT32 (kapasitas ≤ 32GB direkomendasikan).
    SD Card > 32GB umumnya berformat exFAT yang tidak didukung library SD.h dasar.
```

### Format SD Card yang Kompatibel

| Format | Kapasitas | Dukungan `SD.h` | Rekomendasi |
|--------|-----------|-----------------|-------------|
| FAT16  | ≤ 2 GB    | ✅ Ya            | Tidak direkomendasikan (kapasitas kecil) |
| FAT32  | 4 GB – 32 GB | ✅ Ya (optimal) | ✅ **Gunakan ini** |
| exFAT  | > 32 GB   | ❌ Tidak         | ⚠️ Perlu library `SD_MMC` atau format ulang ke FAT32 |
| NTFS   | Semua     | ❌ Tidak         | Tidak kompatibel |

> 💡 **Tips Format SD Card:** Di Windows, buka File Explorer → klik kanan SD Card → Format → pilih **FAT32** → centang "Quick Format" → OK.

---

## 41.3 Referensi API `SD.h`

```cpp
#include <SD.h>
#include <SPI.h>

// ── Inisialisasi ──────────────────────────────────────────────────
SD.begin(CS_PIN);               // Mount SD dengan pin CS yang ditentukan
SD.begin(CS_PIN, SPI, 4000000); // Mount dengan clock SPI 4MHz (default 25MHz)
SD.end();                       // Unmount (jarang diperlukan)

// ── Info Kapasitas ────────────────────────────────────────────────
uint64_t total = SD.cardSize();         // Total kapasitas (bytes)
uint64_t used  = SD.usedBytes();        // Digunakan (bytes)
uint8_t  type  = SD.cardType();         // CARD_NONE/MMC/SD/SDHC/UNKNOWN
String   typeStr = (type == CARD_SD) ? "SD" : "SDHC";

// ── Operasi File ──────────────────────────────────────────────────
File f = SD.open("/data.txt", FILE_READ);    // Buka baca
File f = SD.open("/data.txt", FILE_WRITE);   // Buka tulis (overwrite dari awal!)
File f = SD.open("/data.txt", FILE_APPEND);  // Buka append (tambah di akhir)
bool ok = SD.exists("/data.txt");            // Cek keberadaan file/folder
bool ok = SD.remove("/data.txt");            // Hapus file
bool ok = SD.rename("/a.txt", "/b.txt");     // Rename / pindah file

// ── Operasi File (setelah open) ───────────────────────────────────
f.print("teks");              // Tulis teks
f.println("baris");           // Tulis teks + newline
f.printf("%.2f", nilai);      // Tulis terformat (float dll)
f.write(buf, len);            // Tulis raw bytes
String s = f.readStringUntil('\n');  // Baca satu baris
String s = f.readString();           // Baca seluruh isi (hati-hati RAM!)
bool  ok = f.available();            // Ada data lagi untuk dibaca?
size_t sz = f.size();                // Ukuran file (bytes)
f.close();                           // WAJIB tutup setelah selesai!

// ── Operasi Direktori ─────────────────────────────────────────────
SD.mkdir("/logs");             // Buat direktori
SD.rmdir("/logs");             // Hapus direktori (harus kosong)
File dir = SD.open("/logs");   // Buka direktori
File entry = dir.openNextFile(); // Iterasi isi direktori
while (entry) {
  Serial.printf("  %s (%u bytes)\n", entry.name(), entry.size());
  entry = dir.openNextFile();
}
dir.close(); // Tutup juga handle direktori!
```

> ⚠️ **Perbedaan Kritis `FILE_WRITE` vs `FILE_APPEND`:** Di library `SD.h`, `FILE_WRITE` akan **menulis dari posisi awal file** — artinya file yang sudah ada akan ditimpa dari byte 0, bukan dihapus total seperti mode `"w"` di LittleFS. Gunakan `FILE_APPEND` untuk logging data yang benar!

---

## 41.4 Program 1: Hello MicroSD — Inisialisasi & Operasi Dasar

```cpp
/*
 * BAB 41 - Program 1: Hello MicroSD
 *
 * Tujuan:
 *   Membuktikan SD Card bekerja: inisialisasi, tulis, baca,
 *   tampilkan info kapasitas. Setiap boot menambah satu baris ke file.
 *
 * Hardware:
 *   MicroSD Adapter — sudah hardwired di kit Bluino:
 *   CS=IO5, MOSI=IO23, CLK=IO18, MISO=IO19
 *
 * Cara uji:
 *   1. Pastikan SD Card FAT32 terpasang di slot
 *   2. Upload dan buka Serial Monitor (115200 baud)
 *   3. Reset ESP32 beberapa kali — baris file akan bertambah
 *   4. Cabut SD Card, buka di PC → buka /hello.txt
 */

#include <SD.h>
#include <SPI.h>

#define SD_CS_PIN  5       // Chip Select — hardwired di kit Bluino
#define LOG_FILE   "/hello.txt"

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("=== BAB 41 Program 1: Hello MicroSD ===");

  // ── Inisialisasi SD Card ─────────────────────────────────────
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("[ERR] SD Card gagal dimount!");
    Serial.println("      Periksa: SD Card terpasang? Format FAT32?");
    return;
  }
  Serial.println("[OK] SD Card berhasil dimount.");

  // ── Info kapasitas ────────────────────────────────────────────
  uint64_t cardSize = SD.cardSize() / (1024ULL * 1024ULL); // Bytes → MB
  uint64_t usedSize = SD.usedBytes() / (1024ULL * 1024ULL);
  uint8_t  cardType = SD.cardType();
  const char* typeStr = (cardType == CARD_SDHC) ? "SDHC" :
                        (cardType == CARD_SD)   ? "SD"   : "MMC/Lainnya";
  Serial.printf("[INFO] Tipe: %s | Kapasitas: %llu MB | Terpakai: %llu MB\n",
    typeStr, cardSize, usedSize);

  // ── Tulis baris baru ke file (append) ────────────────────────
  File f = SD.open(LOG_FILE, FILE_APPEND);
  if (!f) {
    Serial.println("[ERR] Gagal membuka file untuk ditulis!");
    return;
  }
  f.printf("Boot — uptime: %lu ms\n", millis());
  f.close();
  Serial.println("[OK] Baris baru ditulis ke " LOG_FILE);

  // ── Baca dan tampilkan seluruh isi file ──────────────────────
  Serial.println("\n── Isi " LOG_FILE " ──");
  f = SD.open(LOG_FILE, FILE_READ);
  if (!f) {
    Serial.println("[ERR] Gagal membuka file untuk dibaca!");
    return;
  }
  int lineNum = 1;
  while (f.available()) {
    String baris = f.readStringUntil('\n');
    baris.trim();
    if (baris.length() > 0) {
      Serial.printf("  %2d: %s\n", lineNum++, baris.c_str());
    }
    yield(); // Perlindungan Watchdog Timer
  }
  f.close();
  Serial.println("────────────────────────────────────");
}

void loop() {}
```

**Output contoh (setelah 3x reset):**
```text
=== BAB 41 Program 1: Hello MicroSD ===
[OK] SD Card berhasil dimount.
[INFO] Tipe: SDHC | Kapasitas: 7580 MB | Terpakai: 4 MB
[OK] Baris baru ditulis ke /hello.txt

── Isi /hello.txt ──
   1: Boot — uptime: 512 ms
   2: Boot — uptime: 508 ms
   3: Boot — uptime: 514 ms
────────────────────────────────────
```

> 💡 **Kenapa ukuran `usedBytes` bisa 4 MB untuk file kecil?** FAT32 mengalokasikan ruang dalam cluster (biasanya 32 KB per cluster). File satu baris pun akan mengkonsumsi minimal satu cluster penuh di level filesystem, meskipun tidak terlihat dari ukuran file.

---

## 41.5 Program 2: CSV Data Logger Multi-Sensor Profesional

Program ini adalah implementasi lengkap sistem **data logger berbasis SD Card** yang dalam praktik nyata digunakan pada stasiun pemantauan lingkungan, perangkat penelitian lapangan, dan sistem SCADA sederhana.

```cpp
/*
 * BAB 41 - Program 2: CSV Data Logger Multi-Sensor ke SD Card
 *
 * Fitur:
 *   ▶ Baca DHT11 (IO27) — suhu & kelembaban
 *   ▶ Baca sensor internal ESP32 — Hall sensor simulasi
 *   ▶ Tulis data ke SD Card dalam format CSV bertanggal
 *   ▶ Nama file otomatis berdasarkan sesi boot: /logs/log_NNNN.csv
 *   ▶ Logging setiap 5 detik (non-blocking)
 *   ▶ Guard: cek kapasitas SD sebelum setiap penulisan
 *   ▶ CLI Serial: dump, list, info, clear
 *   ▶ LED indikator aktivitas logging (IO bawaan)
 *
 * Hardware:
 *   DHT11  → IO27 (pull-up 4K7Ω bawaan kit)
 *   MicroSD → CS=IO5, MOSI=IO23, CLK=IO18, MISO=IO19 (hardwired)
 *
 * Format CSV:
 *   uptime_ms,suhu_C,kelembaban_pct,status
 *   5012,28.5,65,OK
 *   10013,ERR,ERR,SENSOR_FAIL
 */

#include <SD.h>
#include <SPI.h>
#include <DHT.h>

// ── Pin Definitions ────────────────────────────────────────────────
#define SD_CS_PIN    5
#define DHT_PIN      27
#define DHT_TYPE     DHT11

// ── Timing ────────────────────────────────────────────────────────
const unsigned long LOG_INTERVAL_MS  = 5000UL;   // Interval logging
const uint64_t      MIN_FREE_MB      = 10ULL;     // Min free space sebelum berhenti log
unsigned long tLog = 0;

// ── State ─────────────────────────────────────────────────────────
uint32_t logCount  = 0;   // Jumlah baris log sesi ini
bool     sdOk      = false;
String   logFile   = "";  // Path file log aktif sesi ini

// ── Objek Sensor ──────────────────────────────────────────────────
DHT dht(DHT_PIN, DHT_TYPE);

// ── Serial CLI buffer ─────────────────────────────────────────────
String serialBuf = "";

// ─────────────────────────────────────────────────────────────────
// Fungsi: Pilih nama file log unik untuk sesi ini
// Strategi: /logs/log_0001.csv, /logs/log_0002.csv, dst.
// ─────────────────────────────────────────────────────────────────
String chooseLogFile() {
  if (!SD.exists("/logs")) {
    SD.mkdir("/logs");
    Serial.println("[SD] Direktori /logs dibuat.");
  }
  for (uint16_t n = 1; n <= 9999; n++) {
    char path[30];
    snprintf(path, sizeof(path), "/logs/log_%04u.csv", n);
    if (!SD.exists(path)) {
      Serial.printf("[SD] File log sesi: %s\n", path);
      return String(path);
    }
  }
  // Fallback: timpa file terakhir jika sudah penuh (edge case)
  return "/logs/log_9999.csv";
}

// ─────────────────────────────────────────────────────────────────
// Fungsi: Tulis header CSV ke file baru
// ─────────────────────────────────────────────────────────────────
bool writeHeader(const String& path) {
  File f = SD.open(path, FILE_WRITE);
  if (!f) {
    Serial.printf("[ERR] Gagal buat file: %s\n", path.c_str());
    return false;
  }
  f.println("uptime_ms,suhu_C,kelembaban_pct,status");
  f.close();
  Serial.println("[CSV] Header ditulis.");
  return true;
}

// ─────────────────────────────────────────────────────────────────
// Fungsi: Tulis satu baris data log
// ─────────────────────────────────────────────────────────────────
void logData(unsigned long now, float temp, float hum) {
  // Guard: Cek kapasitas sebelum tulis
  uint64_t freeMB = (SD.cardSize() - SD.usedBytes()) / (1024ULL * 1024ULL);
  if (freeMB < MIN_FREE_MB) {
    Serial.printf("[WARN] SD hampir penuh! Sisa: %llu MB. Logging dihentikan.\n", freeMB);
    sdOk = false; // Hentikan logging untuk protect data yang ada
    return;
  }

  File f = SD.open(logFile, FILE_APPEND);
  if (!f) {
    Serial.println("[ERR] Gagal append ke log file!");
    return;
  }
  if (isnan(temp) || isnan(hum)) {
    f.printf("%lu,ERR,ERR,SENSOR_FAIL\n", now);
  } else {
    f.printf("%lu,%.1f,%.0f,OK\n", now, temp, hum);
  }
  f.close();
  logCount++;
}

// ─────────────────────────────────────────────────────────────────
// Fungsi: List semua file di /logs/
// ─────────────────────────────────────────────────────────────────
void listLogFiles() {
  Serial.println("\n── File di /logs/ ──");
  File dir = SD.open("/logs");
  if (!dir || !dir.isDirectory()) {
    Serial.println("  (direktori tidak ada atau kosong)");
    return;
  }
  File entry = dir.openNextFile();
  int count = 0;
  while (entry) {
    if (!entry.isDirectory()) {
      Serial.printf("  %-25s %8u bytes\n", entry.name(), entry.size());
      count++;
    }
    entry = dir.openNextFile();
    yield(); // Jaga kestabilan sistem jika file ribuan
  }
  dir.close();
  if (count == 0) Serial.println("  (kosong)");

  uint64_t total = SD.cardSize() / (1024ULL * 1024ULL);
  uint64_t used  = SD.usedBytes() / (1024ULL * 1024ULL);
  Serial.printf("\nTotal: %llu MB / %llu MB (%.1f%% penuh)\n\n",
    used, total, 100.0f * (float)used / (float)total);
}

// ─────────────────────────────────────────────────────────────────
// Fungsi: Tampilkan isi file log aktif ke Serial
// ─────────────────────────────────────────────────────────────────
void dumpLog() {
  Serial.printf("\n── Isi %s ──\n", logFile.c_str());
  File f = SD.open(logFile, FILE_READ);
  if (!f) { Serial.println("  (file tidak ada)"); return; }

  uint32_t totalBaris = 0;
  Serial.println("─────────────────────────────────────────────────");
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) {
      Serial.println(line);
      if (!line.startsWith("uptime_ms")) totalBaris++;
    }
    yield(); // Perlindungan Watchdog: Cegah crash jika log mencapai ribuan baris!
  }
  f.close();
  Serial.printf("\nTotal log historis: %u baris data (sesi ini: %u)\n\n",
    totalBaris, logCount);
}

// ─────────────────────────────────────────────────────────────────
// Fungsi: Info SD Card
// ─────────────────────────────────────────────────────────────────
void printSDInfo() {
  uint8_t  t = SD.cardType();
  const char* ts = (t == CARD_SDHC) ? "SDHC" : (t == CARD_SD) ? "SD" : "MMC";
  uint64_t total = SD.cardSize() / (1024ULL * 1024ULL);
  uint64_t used  = SD.usedBytes() / (1024ULL * 1024ULL);
  Serial.printf("\n[SD INFO]\n  Tipe   : %s\n  Total  : %llu MB\n  Terpakai: %llu MB\n  Sisa   : %llu MB\n  File log: %s\n  Log sesi: %u baris\n\n",
    ts, total, used, total - used, logFile.c_str(), logCount);
}

// ─────────────────────────────────────────────────────────────────
// Fungsi: Hapus semua file log lama (bukan log aktif)
// ─────────────────────────────────────────────────────────────────
void clearOldLogs() {
  File dir = SD.open("/logs");
  if (!dir || !dir.isDirectory()) {
    Serial.println("[ERR] Direktori /logs tidak ada.");
    return;
  }
  int deleted = 0;
  while (true) {
    File entry = dir.openNextFile();
    if (!entry) break; // Tidak ada file lagi, selesai.
    
    String name = String(entry.name());
    bool isDir = entry.isDirectory();
    entry.close(); // Tutup sebelum di-remove
    
    if (isDir) continue; // Jangan hapus direktori
    
    // Pastikan path absolut
    String fullPath = name;
    if (!fullPath.startsWith("/")) {
      if (!fullPath.startsWith("logs/")) fullPath = "/logs/" + fullPath;
      else fullPath = "/" + fullPath;
    }
    
    if (fullPath != logFile) {  // Jangan hapus file log aktif!
      if (SD.remove(fullPath.c_str())) {
        deleted++;
        dir.rewindDirectory(); // WAJIB! Reset index karena isi direktori (FAT) berubah
      }
    }
    yield(); // SANGAT KRUSIAL: Mencegah trigger WDT akibat efek O(N^2) proses rewind
  }
  dir.close();
  Serial.printf("[OK] %d file log lama dihapus. File aktif %s dipertahankan.\n",
    deleted, logFile.c_str());
}

// ─────────────────────────────────────────────────────────────────
// Serial CLI Handler
// ─────────────────────────────────────────────────────────────────
void handleCmd(const String& cmd) {
  if      (cmd == "dump")  dumpLog();
  else if (cmd == "list")  listLogFiles();
  else if (cmd == "info")  printSDInfo();
  else if (cmd == "clear") clearOldLogs();
  else if (cmd == "help") {
    Serial.println("Perintah: dump | list | info | clear | help");
    Serial.println("  dump  — Tampilkan isi file log aktif");
    Serial.println("  list  — List semua file di /logs/");
    Serial.println("  info  — Info kapasitas SD Card");
    Serial.println("  clear — Hapus file log lama (kecuali sesi ini)");
  } else if (cmd.length() > 0) {
    Serial.printf("[ERR] Tidak dikenal: '%s'. Ketik 'help'.\n", cmd.c_str());
  }
}

// ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  dht.begin();
  delay(1000); // DHT11 butuh stabilisasi ~1 detik

  Serial.println("=== BAB 41 Program 2: CSV Data Logger (SD Card) ===");
  Serial.println("Perintah: 'dump' | 'list' | 'info' | 'clear' | 'help'");

  // Inisialisasi SD Card
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("[ERR] SD Card gagal dimount! Periksa koneksi & format.");
    return;
  }
  sdOk = true;
  Serial.println("[OK] SD Card siap.");

  // Pilih file log & tulis header
  logFile = chooseLogFile();
  if (!writeHeader(logFile)) {
    sdOk = false;
    return;
  }
  listLogFiles();
}

void loop() {
  unsigned long now = millis();

  // ── Serial CLI ────────────────────────────────────────────────
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      String cmd = serialBuf;
      cmd.trim();
      serialBuf = "";
      handleCmd(cmd);
    } else {
      if (serialBuf.length() < 64) serialBuf += c;
    }
  }

  // ── Logging setiap LOG_INTERVAL_MS ───────────────────────────
  if (sdOk && (now - tLog >= LOG_INTERVAL_MS)) {
    tLog = now;
    float t = dht.readTemperature();
    float h = dht.readHumidity();
    logData(now, t, h);

    if (!isnan(t) && !isnan(h)) {
      Serial.printf("[LOG #%u] %lu ms | %.1f°C | %.0f%%\n",
        logCount, now, t, h);
    } else {
      Serial.printf("[LOG #%u] %lu ms | SENSOR ERROR\n", logCount, now);
    }
  }
}
```

**Output contoh sesi pertama:**
```text
=== BAB 41 Program 2: CSV Data Logger (SD Card) ===
Perintah: 'dump' | 'list' | 'info' | 'clear' | 'help'
[OK] SD Card siap.
[SD] Direktori /logs dibuat.
[SD] File log sesi: /logs/log_0001.csv
[CSV] Header ditulis.

── File di /logs/ ──
  log_0001.csv              44 bytes

Total: 4 MB / 7580 MB (0.1% penuh)

[LOG #1]  5012 ms | 28.5°C | 65%
[LOG #2] 10013 ms | 28.6°C | 64%
[LOG #3] 15015 ms | 29.0°C | 63%
```

> 💡 **Fitur `chooseLogFile()`:** Setiap kali ESP32 di-reset, sistem memilih nama file baru (`log_0001.csv`, `log_0002.csv`, dst.). Ini memastikan data dari setiap sesi tersimpan terpisah — sangat berguna untuk eksperimen yang membutuhkan pemisahan log per session.

---

## 41.6 Program 3: File Konfigurasi JSON di SD Card

Program ini mendemonstrasikan cara menyimpan konfigurasi dalam format **JSON** di SD Card menggunakan library **ArduinoJson**. JSON lebih fleksibel dan lebih mudah diparse dibandingkan format INI untuk konfigurasi yang kompleks.

> 📦 **Library yang Dibutuhkan:** Install `ArduinoJson` by Benoit Blanchon via Library Manager Arduino IDE. Versi yang direkomendasikan: **ArduinoJson 7.x**.

```cpp
/*
 * BAB 41 - Program 3: JSON Config Manager via SD Card
 *
 * Format file /config.json:
 *   {
 *     "device_name": "Bluino-Station-01",
 *     "log_interval_s": 10,
 *     "temp_alarm": 35.5,
 *     "alarm_enabled": true,
 *     "brightness": 75
 *   }
 *
 * Fitur:
 *   ▶ Baca & parse config JSON saat boot
 *   ▶ Buat config default jika file belum ada
 *   ▶ Update via Serial: SET key value
 *   ▶ Simpan kembali ke SD sebagai JSON terformat
 *   ▶ RELOAD: muat ulang dari SD
 *
 * Hardware: MicroSD hardwired, Hanya ESP32
 */

#include <SD.h>
#include <SPI.h>
#include <ArduinoJson.h>

#define SD_CS_PIN  5
#define CFG_FILE   "/config.json"

// ── Konfigurasi aktif ──────────────────────────────────────────────
struct Config {
  String  deviceName    = "Bluino-Station-01";
  uint32_t logIntervalS = 10;
  float   tempAlarm     = 35.0f;
  bool    alarmEnabled  = true;
  uint8_t brightness    = 75;
} cfg;

// ─────────────────────────────────────────────────────────────────
// Fungsi: Simpan konfigurasi ke SD sebagai JSON
// ─────────────────────────────────────────────────────────────────
bool saveConfig() {
  // Buat JSON document (kapasitas 512 byte mencukupi untuk config ini)
  JsonDocument doc;
  doc["device_name"]    = cfg.deviceName;
  doc["log_interval_s"] = cfg.logIntervalS;
  doc["temp_alarm"]     = cfg.tempAlarm;
  doc["alarm_enabled"]  = cfg.alarmEnabled;
  doc["brightness"]     = cfg.brightness;

  File f = SD.open(CFG_FILE, FILE_WRITE);
  if (!f) {
    Serial.println("[ERR] Gagal buka config.json untuk ditulis!");
    return false;
  }
  // serializeJsonPretty → JSON terformat cantik (mudah dibaca manusia)
  serializeJsonPretty(doc, f);
  f.close();
  Serial.println("[CFG] Config disimpan ke " CFG_FILE);
  return true;
}

// ─────────────────────────────────────────────────────────────────
// Fungsi: Muat konfigurasi dari SD
// ─────────────────────────────────────────────────────────────────
bool loadConfig() {
  if (!SD.exists(CFG_FILE)) {
    Serial.println("[CFG] File config belum ada, membuat default...");
    return saveConfig(); // Simpan nilai default struct cfg
  }

  File f = SD.open(CFG_FILE, FILE_READ);
  if (!f) {
    Serial.println("[ERR] Gagal buka config.json untuk dibaca!");
    return false;
  }

  // Guard: Batasi ukuran file maksimal 4KB agar tidak memicu memory leak (OOM Crash)
  if (f.size() > 4096) {
    Serial.println("[ERR] Ukuran file config.json abnormal (>4 KB)!");
    Serial.println("[CFG] Aborting load, menggunakan nilai default.");
    f.close();
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();

  if (err) {
    Serial.printf("[ERR] Gagal parse JSON: %s\n", err.c_str());
    Serial.println("[CFG] Menggunakan nilai default.");
    return false;
  }

  // Ambil nilai dengan fallback ke nilai default struct jika key tidak ada
  cfg.deviceName    = doc["device_name"]    | cfg.deviceName;
  cfg.logIntervalS  = doc["log_interval_s"] | cfg.logIntervalS;
  cfg.tempAlarm     = doc["temp_alarm"]     | cfg.tempAlarm;
  cfg.alarmEnabled  = doc["alarm_enabled"]  | cfg.alarmEnabled;
  cfg.brightness    = doc["brightness"]     | cfg.brightness;

  // Validasi batas nilai setelah load
  if (cfg.brightness > 100) cfg.brightness = 100;
  if (cfg.tempAlarm < -40.0f || cfg.tempAlarm > 125.0f) cfg.tempAlarm = 35.0f;
  if (cfg.logIntervalS < 1 || cfg.logIntervalS > 3600) cfg.logIntervalS = 10;
  if (cfg.deviceName.length() == 0 || cfg.deviceName.length() > 32)
    cfg.deviceName = "Bluino-Station";

  Serial.println("[CFG] Config dimuat dari " CFG_FILE);
  return true;
}

// ─────────────────────────────────────────────────────────────────
// Fungsi: Tampilkan config ke Serial
// ─────────────────────────────────────────────────────────────────
void printConfig() {
  Serial.println("\n┌──────────────────────────────────────────┐");
  Serial.println("│           Konfigurasi Aktif               │");
  Serial.println("├──────────────────────────────────────────┤");
  Serial.printf ("│  device_name    : %-24s│\n", cfg.deviceName.c_str());
  Serial.printf ("│  log_interval_s : %-24u│\n", cfg.logIntervalS);
  Serial.printf ("│  temp_alarm     : %-24.1f│\n", cfg.tempAlarm);
  Serial.printf ("│  alarm_enabled  : %-24s│\n", cfg.alarmEnabled ? "true" : "false");
  Serial.printf ("│  brightness     : %-24d│\n", cfg.brightness);
  Serial.println("└──────────────────────────────────────────┘\n");
}

// ─────────────────────────────────────────────────────────────────
// Fungsi: Tampilkan raw JSON file
// ─────────────────────────────────────────────────────────────────
void dumpConfigFile() {
  Serial.println("\n── Raw " CFG_FILE " ──");
  File f = SD.open(CFG_FILE, FILE_READ);
  if (!f) { Serial.println("  (file tidak ada)"); return; }
  while (f.available()) {
    Serial.write(f.read()); // Tulis byte per byte agar presisi
    yield(); // Perlindungan Watchdog: Mencegah CPU tersendat/reboot
  }
  f.close();
  Serial.println("\n────────────────────────────\n");
}

// ─────────────────────────────────────────────────────────────────
// Serial CLI
// ─────────────────────────────────────────────────────────────────
String serialBuf = "";

void handleCmd(const String& cmd) {
  if (cmd == "get") {
    printConfig();
  } else if (cmd == "dump") {
    dumpConfigFile();
  } else if (cmd == "save") {
    saveConfig();
  } else if (cmd == "reload") {
    loadConfig();
    printConfig();
  } else if (cmd == "help") {
    Serial.println("Perintah:");
    Serial.println("  get               - Tampilkan config aktif");
    Serial.println("  dump              - Tampilkan raw JSON file");
    Serial.println("  save              - Simpan config ke SD");
    Serial.println("  reload            - Muat ulang dari SD");
    Serial.println("  set <key> <value> - Ubah nilai config");
    Serial.println("  help              - Panduan ini");
  } else if (cmd.startsWith("set ")) {
    String rest = cmd.substring(4);
    int sp = rest.indexOf(' ');
    if (sp < 0) { Serial.println("[ERR] Format: set <key> <value>"); return; }
    String key = rest.substring(0, sp);
    String val = rest.substring(sp + 1);
    key.trim(); val.trim();

    if (key == "device_name") {
      if (val.length() > 0 && val.length() <= 32) {
        cfg.deviceName = val;
        Serial.printf("[OK] device_name = %s (belum disimpan, ketik 'save')\n", val.c_str());
      } else { Serial.println("[ERR] Nama harus 1–32 karakter."); }
    } else if (key == "log_interval_s") {
      uint32_t v = (uint32_t)val.toInt();
      if (v >= 1 && v <= 3600) {
        cfg.logIntervalS = v;
        Serial.printf("[OK] log_interval_s = %u detik\n", v);
      } else { Serial.println("[ERR] Interval harus 1–3600 detik."); }
    } else if (key == "temp_alarm") {
      float v = val.toFloat();
      if (v >= -40.0f && v <= 125.0f) {
        cfg.tempAlarm = v;
        Serial.printf("[OK] temp_alarm = %.1f °C\n", v);
      } else { Serial.println("[ERR] Suhu harus -40 s/d 125 °C."); }
    } else if (key == "alarm_enabled") {
      cfg.alarmEnabled = (val == "true" || val == "1");
      Serial.printf("[OK] alarm_enabled = %s\n", cfg.alarmEnabled ? "true" : "false");
    } else if (key == "brightness") {
      int v = val.toInt();
      if (v >= 0 && v <= 100) {
        cfg.brightness = (uint8_t)v;
        Serial.printf("[OK] brightness = %d%%\n", v);
      } else { Serial.println("[ERR] Brightness harus 0–100."); }
    } else {
      Serial.printf("[ERR] Key tidak dikenal: '%s'\n", key.c_str());
    }
  } else if (cmd.length() > 0) {
    Serial.printf("[ERR] Tidak dikenal: '%s'. Ketik 'help'.\n", cmd.c_str());
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("[ERR] SD Card gagal!"); return;
  }
  Serial.println("=== BAB 41 Program 3: JSON Config Manager ===");
  loadConfig();
  printConfig();
  Serial.println("Ketik 'help' untuk panduan perintah.");
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      String cmd = serialBuf;
      cmd.trim();
      serialBuf = "";
      handleCmd(cmd);
    } else {
      if (serialBuf.length() < 64) serialBuf += c;
    }
  }
}
```

**Contoh file `/config.json` di SD Card (bisa dibuka di Notepad/VSCode):**
```json
{
  "device_name": "Bluino-Station-01",
  "log_interval_s": 10,
  "temp_alarm": 35.5,
  "alarm_enabled": true,
  "brightness": 75
}
```

> 💡 **Keunggulan JSON vs Format INI:** File JSON bisa dibuka, diedit, dan divalidasi di browser (`F12 → Console → JSON.parse(...)`) atau editor apapun. Selain itu, ArduinoJson mendukung tipe data yang benar — float tetap float, bool tetap bool, tanpa perlu parsing manual seperti pada INI parser.

---

## 41.7 Tips & Panduan Troubleshooting

### 1. `SD.begin()` selalu mengembalikan `false`?
```text
Solusi bertingkat (lakukan dari A):

A. SD Card tidak terpasang
   → Pastikan kartu terpasang dengan benar di slot
   → Coba cabut dan pasang kembali

B. SD Card format tidak kompatibel
   → Format ke FAT32 (bukan exFAT, bukan NTFS)
   → Di Windows: klik kanan → Format → FAT32 → Quick Format
   → Di Linux/Mac: sudo mkfs.fat -F32 /dev/sdX

C. Kapasitas SD Card terlalu besar
   → SD Card > 32GB sering berformat exFAT
   → Gunakan tools seperti SDFormatter (SDCard.org) untuk format
     SD > 32GB ke FAT32

D. Kecepatan SPI terlalu tinggi
   → Coba: SD.begin(SD_CS_PIN, SPI, 4000000) untuk turunkan ke 4MHz
   → Kabel jumper panjang atau soldering buruk bisa menyebabkan ini

E. Konflik pin SPI
   → Pastikan tidak ada library/device lain yang mengklaim
     pin IO18, IO19, IO23 secara bersamaan
   → Semua SPI device berbagi CLK/MOSI/MISO, hanya CS yang berbeda
```

### 2. File berhasil ditulis tetapi data tidak lengkap?
```text
Penyebab: f.close() tidak dipanggil atau dipanggil prematur.

  File f = SD.open("/test.csv", FILE_APPEND);
  f.println("data"); // Data masih di buffer internal
  // ❌ Tidak ada f.close() → data belum pasti tersimpan!
  f.close();         // ✅ Flush buffer → data tersimpan ke SD Card

PERBEDAAN DENGAN RAM:
  SD Card menulis melalui antrian buffer. f.close() mengosongkan
  (flush) buffer ke media fisik. Tanpanya, data terakhir ada risiko
  hilang jika listrik mati atau SD dicabut.
```

### 3. SD Card Error setelah dicabut dan dipasang kembali (tanpa reboot)?
```text
Library SD.h tidak mendukung hot-swap (cabut-pasang saat running).
Setelah SD dicabut dan dipasang kembali, perlu reinisialisasi:

  SD.end();                  // Bersihkan state lama
  delay(100);                // Beri waktu untuk stabilisasi
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("[ERR] SD reinit gagal!");
  }

Alternatif terbaik: Rancang sistem agar SD Card tidak perlu dicabut
saat ESP32 menyala (praktik umum di IoT produksi).
```

### 4. SD dan SPI device lain (mis. tampilan TFT) konflik?
```text
Solusi: Setiap SPI slave memiliki pin CS (Chip Select) sendiri.

  // Matikan semua slave sebelum berkomunikasi dengan salah satu
  #define SD_CS   5
  #define TFT_CS  15

  digitalWrite(TFT_CS, HIGH); // Matikan TFT
  SD.begin(SD_CS);             // Initialis SD
  // Operasi file...

  // Ketika giliran TFT:
  SD.end();                    // (opsional, atau gunakan SPI bus sharing manual)
  // Inisialisasi TFT...

Atau gunakan library SPI.h langsung dengan beginTransaction/endTransaction
untuk bus sharing yang lebih rapi.
```

### 5. Performa lambat — tulis hanya 10-20 KB/s?
```text
Faktor yang mempengaruhi kecepatan tulis SD via SPI:
  • Clock SPI (default 25MHz, bisa diturunkan ke 4-8MHz untuk stabilitas)
  • Kualitas SD Card (Class 4 vs Class 10 vs V30)
  • Format buffer: tulis dalam blok besar lebih efisien dari byte per byte

Optimasi:
  // Kumpulkan data dulu dalam buffer, tulis sekaligus:
  String buffer = "";
  buffer += String(uptime) + "," + String(temp) + "," + String(hum);
  buffer += "\n";
  File f = SD.open(path, FILE_APPEND);
  f.print(buffer); // Tulis sekaligus, lebih efisien dari f.printf berkali-kali
  f.close();

Untuk aplikasi kecepatan tinggi (>100KB/s), pertimbangkan menggunakan
SD_MMC (SDIO 4-bit) yang jauh lebih cepat dari SPI.
```

---

## 41.8 Perbandingan Lengkap: NVS vs LittleFS vs MicroSD

```
┌─────────────────────┬─────────────┬─────────────┬─────────────────────┐
│ Kriteria            │ NVS (Flash) │ LittleFS    │ MicroSD (SPI)       │
├─────────────────────┼─────────────┼─────────────┼─────────────────────┤
│ Kapasitas           │ ~20 KB      │ ~1.5 MB     │ 4 GB – 2 TB         │
│ Kecepatan tulis     │ Cepat       │ Sedang       │ Lambat (SPI)        │
│ Format data         │ Key-Value   │ File system │ File system (FAT32) │
│ Baca di PC          │ ❌ Tidak    │ ❌ Tidak    │ ✅ Langsung di PC   │
│ Tahan power-loss    │ ✅ Ya       │ ✅ Ya       │ ⚠️ Risiko korupsi  │
│ Komponen tambahan   │ ❌ Tidak    │ ❌ Tidak    │ ✅ SD Adapter       │
│ Ekspansi kapasitas  │ ❌ Tidak    │ ❌ Tidak    │ ✅ Ganti card       │
│ Wear leveling       │ ✅ Built-in │ ✅ Built-in │ ⚠️ Tergantung card  │
├─────────────────────┼─────────────┼─────────────┼─────────────────────┤
│ GUNAKAN UNTUK       │ Config kecil│ Log < 1MB   │ Log jangka panjang  │
│                     │ State FSM   │ Web assets  │ Data multimedia     │
│                     │ Boot count  │ Config file │ Dataset penelitian  │
└─────────────────────┴─────────────┴─────────────┴─────────────────────┘
```

---

## 41.9 Ringkasan

```
┌─────────────────────────────────────────────────────────────────────┐
│               RINGKASAN BAB 41 — MicroSD Card ESP32                  │
├─────────────────────────────────────────────────────────────────────┤
│ KONEKSI KIT BLUINO (HARDWIRED):                                       │
│   CS=IO5  MOSI=IO23  CLK=IO18  MISO=IO19  (SPI2/HSPI)              │
│                                                                       │
│ SYARAT SD CARD:                                                       │
│   ✅ Format FAT32 (bukan exFAT/NTFS)                                 │
│   ✅ Kapasitas ≤ 32GB (optimal)                                      │
│   ✅ Kelas ≥ Class 6 untuk logging cepat                            │
│                                                                       │
│ API ESENSIAL:                                                         │
│   SD.begin(CS_PIN)                 → Init & mount                   │
│   SD.open(path, FILE_APPEND)       → Buka untuk append log          │
│   SD.open(path, FILE_READ)         → Buka untuk baca                │
│   SD.open(path, FILE_WRITE)        → Buka untuk tulis (overwrite!)  │
│   f.printf() / f.println()         → Tulis data                     │
│   f.readStringUntil('\n')          → Baca per baris                  │
│   f.close()                        → WAJIB! Flush buffer ke SD      │
│   SD.exists() / SD.remove()        → Cek / Hapus file               │
│   SD.cardSize() / SD.usedBytes()   → Info kapasitas                 │
│                                                                       │
│ BEST PRACTICES:                                                       │
│   ✅ Selalu f.close() — tidak boleh skip!                            │
│   ✅ Guard kapasitas sebelum append (cegah file korup)              │
│   ✅ Gunakan FILE_APPEND (bukan FILE_WRITE) untuk data log          │
│   ✅ Buat semua direktori sebelum membuat file di dalamnya          │
│   ✅ Satu file per sesi (log_0001.csv, log_0002.csv, dst.)          │
│                                                                       │
│ ANTIPATTERN:                                                          │
│   ❌ Buka file dalam loop() tanpa close() → buffer leak             │
│   ❌ Pakai FILE_WRITE untuk logging → data lama tertimpa!           │
│   ❌ Tidak cek SD Card tersedia sebelum operasi → crash             │
│   ❌ SD Card exFAT > 32GB tanpa format ulang → gagal mount          │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 41.10 Latihan

1. **Rolling Log Otomatis (Max 500 Baris):** Modifikasi Program 2 agar satu file log tidak pernah melebihi 500 baris. Ketika batas tercapai, tutup file lama dan buat file baru secara otomatis (log_0001.csv → log_0002.csv). Ini mensimulasikan perilaku data logger profesional di dunia nyata.

2. **Logger Timestamp dengan RTC DS3231:** Tambahkan modul RTC (Real Time Clock) DS3231 ke jaringan I2C (SDA=IO21, SCL=IO22). Ganti kolom `uptime_ms` dengan timestamp asli (`2025-04-01 14:32:05`). File log kini memiliki waktu yang bisa dikorelasi dengan kejadian nyata.

3. **Web File Manager:** Kombinasikan SD Card dengan WiFi dan Web Server ESP32. Buat antarmuka web sederhana di `index.html` (tersimpan di SD) yang menampilkan daftar file log dan memungkinkan pengguna men-download file CSV langsung dari browser tanpa perlu mencabut SD Card secara fisik.

4. **Dual Storage Fault-Tolerant:** Rancang sistem yang secara normal menulis ke SD Card, tetapi secara otomatis beralih ke LittleFS jika SD Card dicabut atau gagal mount. Tampilkan informasi status media aktif via OLED (IO21/IO22). Setelah SD Card dipasang kembali, salin log dari LittleFS ke SD Card.

5. **CSV ke JSON Converter via Serial:** Buat program yang membaca file `data.csv` dari SD Card, mengkonversikannya ke format JSON (`[{"uptime":5012,"suhu":28.5},...]`), dan menyimpan hasilnya sebagai `data.json` di SD Card yang sama, menggunakan ArduinoJson. Verifikasi hasilnya dengan membuka `data.json` di browser.

---

## ☕ Dukung Proyek Ini

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

---

*Sebelumnya → [BAB 40: LittleFS — Sistem File di Flash ESP32](../BAB-40/README.md)*

*Selanjutnya → [BAB 42: WiFi Station Mode](../BAB-42/README.md)*
