# BAB 24: SPI — Serial Peripheral Interface

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami prinsip kerja protokol SPI (bus, MOSI/MISO, clock polarity/phase)
- Membedakan SPI dari I2C dan UART berdasarkan karakteristik teknisnya
- Menggunakan library `SPI.h` dan `SD.h` pada ESP32
- Menginisialisasi dan memverifikasi modul Micro SD yang terpasang di Kit Bluino
- Membuat, membaca, dan menghapus file di kartu SD untuk keperluan *data logging*
- Membangun sistem *data logger* suhu dan waktu yang robustik

---

## 24.1 Apa itu SPI?

**SPI** (Serial Peripheral Interface) adalah protokol komunikasi serial **sinkron** yang dikembangkan oleh Motorola pada tahun 1980-an. Keunggulan utama SPI: **kecepatan transfer sangat tinggi** karena jalur data kirim dan terima berjalan **secara bersamaan** (full-duplex) dengan clock yang dikontrol penuh oleh Master.

### Perbandingan SPI vs I2C vs UART

```
┌──────────────────┬─────────────────┬─────────────────┬─────────────────┐
│ Kriteria         │ UART            │ I2C             │ SPI             │
├──────────────────┼─────────────────┼─────────────────┼─────────────────┤
│ Kabel minimum    │ 2 (TX, RX)      │ 2 (SDA, SCL)    │ 4 (MOSI, MISO,  │
│                  │                 │                 │  CLK, CS)       │
│ Clock            │ Asinkron        │ Sinkron         │ Sinkron         │
│ Full-duplex      │ Ya              │ Tidak           │ Ya              │
│ Jumlah perangkat │ 2               │ Hingga 127      │ Banyak (1 CS    │
│                  │                 │                 │  per perangkat) │
│ Kecepatan        │ Hingga ~1 Mbps  │ 100K–3.4 Mbps   │ Hingga 80 Mbps  │
│ Jarak            │ Cukup jauh      │ <50 cm          │ <30 cm          │
│ Kompleksitas H/W │ Rendah          │ Rendah          │ Sedang          │
└──────────────────┴─────────────────┴─────────────────┴─────────────────┘
```

> 💡 **Pilih SPI bila:** Kamu membutuhkan kecepatan tinggi — misalnya kartu SD, layar TFT, DAC audio, atau ADC presisi tinggi.

---

## 24.2 Cara Kerja SPI

### Empat Jalur SPI

| Pin | Nama Lengkap | Arah | Fungsi |
|-----|-------------|------|--------|
| **MOSI** | Master Out Slave In | Master → Slave | Jalur data dari ESP32 ke perangkat |
| **MISO** | Master In Slave Out | Slave → Master | Jalur data dari perangkat ke ESP32 |
| **CLK / SCK** | Clock | Master → Slave | Sinyal detak yang disediakan Master |
| **CS / SS** | Chip Select / Slave Select | Master → Slave | Pilih perangkat (aktif LOW) |

### Arsitektur Multi-Slave SPI

Tidak seperti I2C yang menggunakan sistem alamat, SPI menggunakan pin **CS (Chip Select)** terpisah untuk setiap perangkat:

```
                           ┌──────────────────────────────────────┐
              ESP32        │           SPI Bus bersama             │
          ┌──────────┐     │                                       │
MOSI IO23 │──────────┼─────┼────── MOSI ──[SD Card]  ──[TFT LCD] │
MISO IO19 │──────────┼─────┼────── MISO ──[SD Card]  ──[TFT LCD] │
CLK  IO18 │──────────┼─────┼────── CLK  ──[SD Card]  ──[TFT LCD] │
CS   IO5  │──────────┼─────┼────── CS   ──[SD Card]              │
CS2  IO27 │──────────┼─────┼────── CS   ──────────────[TFT LCD]  │
          └──────────┘     └──────────────────────────────────────┘

Hanya SATU perangkat yang aktif pada satu waktu (CS LOW = aktif).
```

> ⚠️ **Pin CS adalah ACTIVE LOW:** Perangkat aktif saat CS ditarik ke LOW, dan tidak aktif saat CS HIGH. Ini kebalikan dari cara kerja kebanyakan pin digital!

### Mode SPI (CPOL dan CPHA)

SPI memiliki 4 mode yang didefinisikan oleh dua parameter:
- **CPOL** (Clock Polarity): Kondisi *idle* CLK — `0` = LOW, `1` = HIGH
- **CPHA** (Clock Phase): Kapan data dibaca — `0` = rising edge, `1` = falling edge

| Mode | CPOL | CPHA | Perangkat Umum |
|------|------|------|----------------|
| **Mode 0** | 0 | 0 | **Kartu SD** ✅, SPI Flash |
| Mode 1 | 0 | 1 | Jarang dipakai |
| Mode 2 | 1 | 0 | Jarang dipakai |
| Mode 3 | 1 | 1 | Beberapa ADC, sensor suhu |

> 📝 **Kartu SD menggunakan SPI Mode 0.** Library `SD.h` sudah menangani ini secara otomatis.

---

## 24.3 SPI di ESP32

### Port SPI Hardware ESP32

ESP32 secara internal memiliki 4 *controller* SPI. Dua di antaranya digunakan untuk Flash SPI internal dan tidak boleh disentuh. Dua yang tersedia untuk pengguna adalah:

```
┌────────────────────────────────────────────────────────────────┐
│                  SPI Controller Tersedia di ESP32              │
├──────────┬─────────────────────────────────────────────────────┤
│ Nama     │ Pin Default                                          │
├──────────┼──────────────────────────────────────────────────────┤
│ VSPI     │ CLK=IO18, MISO=IO19, MOSI=IO23, CS=IO5              │
│ (SPI3)   │ ← Dipakai Kit Bluino untuk Micro SD!               │
├──────────┼──────────────────────────────────────────────────────┤
│ HSPI     │ CLK=IO14, MISO=IO12, MOSI=IO13, CS=IO15             │
│ (SPI2)   │ ⚠️ IO12/IO15 adalah strapping pins — hati-hati!   │
└──────────┴──────────────────────────────────────────────────────┘
```

### Pin SPI Micro SD di Kit Bluino

```
┌────────────────────────────────────────────────────────────────┐
│             Micro SD Adapter — Kit Bluino ESP32               │
├─────────────────────────────┬──────────────────────────────────┤
│ Fungsi SPI                  │ Pin ESP32                         │
├─────────────────────────────┼──────────────────────────────────┤
│ CS  (Chip Select)           │ IO5                               │
│ MOSI (Data ke SD)           │ IO23                              │
│ CLK  (Clock)                │ IO18                              │
│ MISO (Data dari SD)         │ IO19                              │
└─────────────────────────────┴──────────────────────────────────┘
```

> ✅ Pin-pin ini adalah **VSPI default ESP32** — tidak perlu konfigurasi remapping!

### Inisialisasi SPI dan SD

```cpp
#include <SPI.h>
#include <SD.h>

#define SD_CS   5    // Chip Select Micro SD di Kit Bluino
#define SD_MOSI 23
#define SD_MISO 19
#define SD_CLK  18

void setup() {
  // Inisialisasi SPI (CLK, MISO, MOSI). 
  // PENTING: Jangan masukkan pin CS di sini agar tidak konflik dengan SD library!
  SPI.begin(SD_CLK, SD_MISO, SD_MOSI);

  // Mulai SD dengan kecepatan 4 MHz (aman untuk semua kartu SD)
  if (!SD.begin(SD_CS, SPI, 4000000UL)) {
    // Gagal inisialisasi — tangani error
  }
}
```

> ⚠️ **Urutan parameter `SPI.begin()`:** `SPI.begin(CLK, MISO, MOSI)`. Perhatikan MISO sebelum MOSI — ini kebalikan dari sebutan "MOSI MISO"! Selain itu, **jangan masukkan pin CS** pada parameter `SPI.begin()` jika kamu menggunakan `SD.begin(CS)`, karena fungsi SD akan mengatur CS-nya secara manual (Software CS).

---

## 24.4 Program 1: Verifikasi Micro SD Card

Program pertama ini memverifikasi bahwa kartu SD terpasang dan berfungsi, serta menampilkan informasi kapasitas dan ruang kosong.

```cpp
/*
 * BAB 24 - Program 1: Verifikasi Micro SD Card
 * Menampilkan info kapasitas dan status SD Card.
 *
 * Wiring (Kit Bluino — sudah terhubung di PCB):
 *   CS   → IO5
 *   MOSI → IO23
 *   CLK  → IO18
 *   MISO → IO19
 *
 * PENTING: Gunakan kartu SD berformat FAT32,
 *          ukuran maksimum 32GB untuk kompatibilitas terbaik.
 */

#include <SPI.h>
#include <SD.h>

// ── Definisi Pin SPI Micro SD (Kit Bluino) ─────────────────────
#define SD_CS   5
#define SD_MOSI 23
#define SD_MISO 19
#define SD_CLK  18

// ── Kecepatan SPI yang aman untuk SD Card ──────────────────────
#define SD_SPI_FREQ 4000000UL  // 4 MHz — stabil di semua suhu

void setup() {
  Serial.begin(115200);

  Serial.println("══════════════════════════════════════════");
  Serial.println("   BAB 24: Verifikasi Micro SD Card");
  Serial.println("══════════════════════════════════════════");

  // Pre-kondisi CS: Pastikan HIGH (tidak aktif) sebelum init SPI
  // Ini menghindari 'glitch' sinyal reset pada perangkat SPI
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  // Parameter SPI.begin(): (CLK, MISO, MOSI) -> Hindari memasukkan CS
  SPI.begin(SD_CLK, SD_MISO, SD_MOSI);

  if (!SD.begin(SD_CS, SPI, SD_SPI_FREQ)) {
    Serial.println("❌ SD Card gagal diinisialisasi!");
    Serial.println("   Kemungkinan penyebab:");
    Serial.println("   1. Tidak ada kartu SD yang terpasang");
    Serial.println("   2. Format bukan FAT32");
    Serial.println("   3. Kartu SD rusak atau tidak kompatibel");
    return;
  }

  Serial.println("✅ SD Card berhasil diinisialisasi!");
  Serial.println();

  // ── Tampilkan tipe kartu SD ───────────────────────────────────
  uint8_t cardType = SD.cardType();
  Serial.print("Tipe kartu   : ");
  switch (cardType) {
    case CARD_MMC:  Serial.println("MMC");   break;
    case CARD_SD:   Serial.println("SDSC");  break;
    case CARD_SDHC: Serial.println("SDHC/SDXC"); break;
    default:        Serial.println("Tidak dikenal"); break;
  }

  // ── Tampilkan kapasitas ───────────────────────────────────────
  uint64_t kapasitas  = SD.cardSize();
  uint64_t terpakai   = SD.usedBytes();
  uint64_t tersedia   = kapasitas - terpakai;

  // Konversi ke MB (1 MB = 1048576 byte)
  Serial.printf("Kapasitas    : %llu MB\n", kapasitas  / (1024ULL * 1024ULL));
  Serial.printf("Terpakai     : %llu MB\n", terpakai   / (1024ULL * 1024ULL));
  Serial.printf("Tersedia     : %llu MB\n", tersedia   / (1024ULL * 1024ULL));

  Serial.println("══════════════════════════════════════════");
}

void loop() {
  // Verifikasi hanya sekali di setup()
}
```

**Contoh output yang diharapkan:**
```
══════════════════════════════════════════
   BAB 24: Verifikasi Micro SD Card
══════════════════════════════════════════
✅ SD Card berhasil diinisialisasi!

Tipe kartu   : SDHC/SDXC
Kapasitas    : 7580 MB
Terpakai     : 12 MB
Tersedia     : 7568 MB
══════════════════════════════════════════
```

---

## 24.5 Operasi File di SD Card

### Konsep Penting: File System FAT32

SD library Arduino menggunakan sistem file **FAT32**. Beberapa batasan penting:

```
┌────────────────────────────────────────────────────────────┐
│ Batasan FAT32 yang harus dipahami                          │
├────────────────────────────────────────────────────────────┤
│ ✅ Nama file/folder: Maksimum 255 karakter (LFN)           │
│ ✅ Ukuran file maksimum: 4 GB per file                     │
│ ✅ Ukuran SD maksimum: Hingga 32 GB (FAT32 standar)        │
│                                                            │
│ 📁 Path dimulai dari root: "/namafile.txt"                 │
│ 📁 Separator folder: "/"  → "/folder/subfolder/file.txt"  │
└────────────────────────────────────────────────────────────┘
```

### Fungsi File Operations SD Library

| Fungsi | Keterangan |
|--------|------------|
| `SD.exists("/file.txt")` | Cek apakah file/folder ada |
| `SD.remove("/file.txt")` | Hapus file |
| `SD.mkdir("/folder")` | Buat folder baru |
| `SD.rmdir("/folder")` | Hapus folder (harus kosong) |
| `SD.open("/file.txt", FILE_WRITE)` | Buka untuk ditulis (append) |
| `SD.open("/file.txt", FILE_READ)` | Buka untuk dibaca |
| `SD.open("/file.txt", FILE_APPEND)` | Buka untuk ditambah di akhir |
| `file.print("teks")` | Tulis teks ke file |
| `file.println("teks")` | Tulis teks + newline ke file |
| `file.printf("format", ...)` | Tulis format seperti printf |
| `file.available()` | Jumlah byte tersisa untuk dibaca |
| `file.read()` | Baca 1 byte dari file |
| `file.readStringUntil('\n')` | Baca satu baris |
| `file.close()` | **WAJIB ditutup setelah selesai!** |
| `file.size()` | Ukuran file dalam byte |

> ⚠️ **WAJIB: Selalu panggil `file.close()`!** Tanpa menutup file, data yang baru ditulis bisa hilang karena tersimpan di *write buffer* dan belum *flush* ke kartu fisik.

---

## 24.6 Program 2: Tulis dan Baca File

```cpp
/*
 * BAB 24 - Program 2: Operasi Tulis dan Baca File SD
 * Menulis beberapa baris data ke file, lalu membacanya kembali.
 *
 * Wiring (Kit Bluino — sudah terhubung di PCB):
 *   CS → IO5, MOSI → IO23, CLK → IO18, MISO → IO19
 */

#include <SPI.h>
#include <SD.h>

#define SD_CS      5
#define SD_MOSI    23
#define SD_MISO    19
#define SD_CLK     18
#define SD_SPI_FREQ 4000000UL

// ── Path file yang akan digunakan ──────────────────────────────
#define FILE_PATH  "/catatan.txt"

bool sdReady = false;

// ── Inisialisasi SD ─────────────────────────────────────────────
bool initSD() {
  // Pre-kondisi CS HIGH demi stabilitas bus saat boot
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  SPI.begin(SD_CLK, SD_MISO, SD_MOSI);
  if (!SD.begin(SD_CS, SPI, SD_SPI_FREQ)) {
    Serial.println("❌ SD Card gagal diinisialisasi!");
    return false;
  }
  Serial.println("✅ SD Card siap.");
  return true;
}

// ── Tulis baris ke file ─────────────────────────────────────────
bool tulisFile(const char* path, const char* isi) {
  // FILE_APPEND: isi lama tidak dihapus, teks baru ditambah di akhir
  File f = SD.open(path, FILE_APPEND);
  if (!f) {
    Serial.printf("❌ Gagal membuka '%s' untuk ditulis\n", path);
    return false;
  }
  f.println(isi);
  f.close();  // WAJIB close agar data benar-benar tersimpan
  return true;
}

// ── Baca seluruh isi file ───────────────────────────────────────
void bacaFile(const char* path) {
  File f = SD.open(path, FILE_READ);
  if (!f) {
    Serial.printf("❌ Gagal membuka '%s' untuk dibaca\n", path);
    return;
  }

  Serial.printf("── Isi file '%s' (%u byte) ──\n", path,
                (unsigned)f.size());

  while (f.available()) {
    // Baca per baris menggunakan readStringUntil (blocking, tapi untuk
    // membaca file lokal yang sudah selesai ditulis ini aman dilakukan)
    String baris = f.readStringUntil('\n');
    baris.trim();  // Hapus whitespace/newline sisa
    if (baris.length() > 0) {
      Serial.println(baris);
    }
  }
  f.close();
}

void setup() {
  Serial.begin(115200);

  Serial.println("══════════════════════════════════════════");
  Serial.println("   BAB 24: Tulis & Baca File SD Card");
  Serial.println("══════════════════════════════════════════");

  sdReady = initSD();
  if (!sdReady) return;

  // Hapus file lama jika ada, agar contoh selalu bersih
  if (SD.exists(FILE_PATH)) {
    SD.remove(FILE_PATH);
    Serial.printf("🗑️  File lama '%s' dihapus.\n", FILE_PATH);
  }

  // ── Tulis beberapa baris ─────────────────────────────────────
  Serial.println("\n📝 Menulis ke file...");
  tulisFile(FILE_PATH, "Bab 24: SPI dan Micro SD Card");
  tulisFile(FILE_PATH, "Baris kedua: Hello from ESP32!");
  tulisFile(FILE_PATH, "Baris ketiga: Ini adalah data logging.");

  char baris4[48];
  snprintf(baris4, sizeof(baris4), "Waktu sejak boot: %lu ms", millis());
  tulisFile(FILE_PATH, baris4);

  Serial.println("✅ Penulisan selesai.");

  // ── Baca kembali ─────────────────────────────────────────────
  Serial.println();
  bacaFile(FILE_PATH);

  Serial.println("══════════════════════════════════════════");
}

void loop() {
  // Program ini hanya demonstrasi sekali di setup()
}
```

**Contoh output:**
```
══════════════════════════════════════════
   BAB 24: Tulis & Baca File SD Card
══════════════════════════════════════════
✅ SD Card siap.
🗑️  File lama '/catatan.txt' dihapus.

📝 Menulis ke file...
✅ Penulisan selesai.

── Isi file '/catatan.txt' (110 byte) ──
Bab 24: SPI dan Micro SD Card
Baris kedua: Hello from ESP32!
Baris ketiga: Ini adalah data logging.
Waktu sejak boot: 342 ms
══════════════════════════════════════════
```

---

## 24.7 Program 3: Data Logger Sensor (Non-Blocking)

Inilah aplikasi paling berguna SPI di dunia IoT: merekam data sensor secara otomatis ke kartu SD. Program ini menggabungkan semua prinsip yang telah kita pelajari (non-blocking dengan `millis()`, output terformat, dan *error handling* yang solid).

```cpp
/*
 * BAB 24 - Program 3: Data Logger Sensor ke SD Card
 * Mencatat data secara otomatis ke kartu SD setiap 5 detik.
 * Format log: CSV (mudah dibuka di Excel/spreadsheet).
 *
 * Wiring (Kit Bluino — sudah terhubung di PCB):
 *   CS → IO5, MOSI → IO23, CLK → IO18, MISO → IO19
 *
 * Format file log:
 *   /datalog.csv
 *   uptime_ms,loop_count,simulasi_suhu
 *   1234,1,28.50
 *   6234,2,28.51
 *   ...
 */

#include <SPI.h>
#include <SD.h>

// ── Definisi Pin ───────────────────────────────────────────────
#define SD_CS        5
#define SD_MOSI      23
#define SD_MISO      19
#define SD_CLK       18
#define SD_SPI_FREQ  4000000UL

// ── Konfigurasi Logger ─────────────────────────────────────────
#define LOG_FILE     "/datalog.csv"
#define LOG_INTERVAL 5000UL   // Interval logging (ms)
#define MAX_LOG_SIZE (1024UL * 1024UL * 10UL)  // Batas ukuran 10 MB

bool sdReady         = false;
unsigned long prevLog = 0;
uint32_t loopCount    = 0;

// ── Inisialisasi SD ─────────────────────────────────────────────
bool initSD() {
  // Pre-kondisi CS HIGH demi stabilitas bus saat boot
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  SPI.begin(SD_CLK, SD_MISO, SD_MOSI);
  if (!SD.begin(SD_CS, SPI, SD_SPI_FREQ)) {
    return false;
  }
  return true;
}

// ── Tulis header CSV jika file baru ────────────────────────────
void tulisHeaderCSV() {
  if (!SD.exists(LOG_FILE)) {
    File f = SD.open(LOG_FILE, FILE_WRITE);
    if (f) {
      f.println("uptime_ms,loop_count,suhu_C");
      f.close();
      Serial.printf("📄 File baru dibuat: %s\n", LOG_FILE);
    }
  } else {
    Serial.printf("📂 File log sudah ada, data akan ditambahkan.\n");
  }
}

// ── Tulis satu baris data ke log ────────────────────────────────
bool tulisLog(unsigned long uptime, uint32_t count, float suhu) {
  // Cek ukuran file — hindari SD penuh/file terlalu besar
  if (SD.exists(LOG_FILE)) {
    File fCheck = SD.open(LOG_FILE, FILE_READ);
    if (fCheck) {
      uint32_t ukuran = fCheck.size();
      fCheck.close();
      if (ukuran >= MAX_LOG_SIZE) {
        Serial.println("⚠️  File log melebihi 10MB, logging dihentikan.");
        return false;
      }
    }
  }

  File f = SD.open(LOG_FILE, FILE_APPEND);
  if (!f) {
    Serial.println("❌ Gagal membuka file log untuk ditulis!");
    return false;
  }

  // Format CSV: uptime_ms,loop_count,suhu
  // Gunakan snprintf untuk keamanan buffer
  char baris[64];
  snprintf(baris, sizeof(baris), "%lu,%lu,%.2f",
           uptime, (unsigned long)count, suhu);

  f.println(baris);
  f.close();  // WAJIB tutup setiap kali selesai menulis
  return true;
}

// ── Simulasi baca suhu (ganti dengan sensor nyata di BAB 26) ───
float bacaSuhuSimulasi() {
  // Tambahkan noise kecil untuk simulasi realistis
  return 28.0f + ((float)(millis() % 100) / 100.0f);
}

void setup() {
  Serial.begin(115200);

  Serial.println("══════════════════════════════════════════");
  Serial.println("   BAB 24: SD Card Data Logger");
  Serial.println("══════════════════════════════════════════");

  sdReady = initSD();

  if (!sdReady) {
    Serial.println("❌ SD Card tidak tersedia — logging dinonaktifkan.");
    Serial.println("   Program tetap berjalan tanpa logging.");
  } else {
    Serial.printf("✅ SD Card siap. Logging setiap %lu detik.\n",
                  LOG_INTERVAL / 1000UL);
    tulisHeaderCSV();
  }

  Serial.println("──────────────────────────────────────────");
}

void loop() {
  unsigned long sekarang = millis();

  // ── Logging non-blocking setiap LOG_INTERVAL ─────────────────
  if (sekarang - prevLog >= LOG_INTERVAL) {
    prevLog = sekarang;
    loopCount++;

    float suhu = bacaSuhuSimulasi();

    Serial.printf("[%8lu ms] Loop #%lu | Suhu: %.2f °C",
                  sekarang, (unsigned long)loopCount, suhu);

    if (sdReady) {
      bool berhasil = tulisLog(sekarang, loopCount, suhu);
      Serial.print(berhasil ? " → ✅ Tersimpan" : " → ❌ Gagal simpan");
    } else {
      Serial.print(" → ⚠️  [SD tidak tersedia]");
    }

    Serial.println();
  }

  // Kerjaan lain bisa ditambahkan di sini tanpa terganggu logging
}
```

**Contoh output di Serial Monitor:**
```
══════════════════════════════════════════
   BAB 24: SD Card Data Logger
══════════════════════════════════════════
✅ SD Card siap. Logging setiap 5 detik.
📄 File baru dibuat: /datalog.csv
──────────────────────────────────────────
[    5001 ms] Loop #1 | Suhu: 28.01 °C → ✅ Tersimpan
[   10002 ms] Loop #2 | Suhu: 28.02 °C → ✅ Tersimpan
[   15003 ms] Loop #3 | Suhu: 28.03 °C → ✅ Tersimpan
```

**Isi file `/datalog.csv` (bisa dibuka di Excel):**
```
uptime_ms,loop_count,suhu_C
5001,1,28.01
10002,2,28.02
15003,3,28.03
```

---

## 24.8 Membuat Struktur Folder di SD Card

SD library mendukung pembuatan dan navigasi folder:

```cpp
/*
 * BAB 24 - Program 4: Manajemen Folder SD Card (Konsep)
 * Menciptakan struktur folder terorganisir di kartu SD.
 */

#include <SPI.h>
#include <SD.h>

#define SD_CS      5
#define SD_MOSI    23
#define SD_MISO    19
#define SD_CLK     18
#define SD_SPI_FREQ 4000000UL

void buatStrukturFolder() {
  // Buat folder berdasarkan kategori data
  const char* folders[] = {
    "/logs",
    "/sensor",
    "/config",
    nullptr  // Sentinel: penanda akhir array
  };

  for (int i = 0; folders[i] != nullptr; i++) {
    if (!SD.exists(folders[i])) {
      if (SD.mkdir(folders[i])) {
        Serial.printf("📁 Folder dibuat: %s\n", folders[i]);
      } else {
        Serial.printf("❌ Gagal buat folder: %s\n", folders[i]);
      }
    } else {
      Serial.printf("📂 Folder sudah ada: %s\n", folders[i]);
    }
  }
}

void listDirRoot() {
  File root = SD.open("/");
  if (!root) {
    Serial.println("❌ Gagal membuka root directory");
    return;
  }

  Serial.println("── Isi root SD Card ──");
  File entry = root.openNextFile();
  while (entry) {
    if (entry.isDirectory()) {
      Serial.printf("  [DIR]  %s\n", entry.name());
    } else {
      Serial.printf("  [FILE] %-20s  %u byte\n",
                    entry.name(), (unsigned)entry.size());
    }
    entry.close();
    entry = root.openNextFile();
  }
  root.close();
}

void setup() {
  Serial.begin(115200);
  
  // Stabilitas Hardware: CS harus HIGH sebelum SPI.begin
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  SPI.begin(SD_CLK, SD_MISO, SD_MOSI);

  if (!SD.begin(SD_CS, SPI, SD_SPI_FREQ)) {
    Serial.println("❌ SD Card gagal!");
    return;
  }
  Serial.println("✅ SD Card siap.");

  buatStrukturFolder();
  Serial.println();
  listDirRoot();
}

void loop() {}
```

---

## 24.9 Tips & Panduan Troubleshooting SPI / SD Card

### 1. SD Card Tidak Terdeteksi?

```
✅ Cek 1: Pastikan kartu SD diformat FAT32 (bukan exFAT atau NTFS)
✅ Cek 2: Gunakan kartu SD ≤32GB untuk kompatibilitas FAT32 terbaik
✅ Cek 3: Gunakan SD.begin(CS, SPI, 4000000UL) — kecepatan 4MHz lebih stabil
✅ Cek 4: Hubungkan GND perangkat dan ESP32 dengan kuat
✅ Cek 5: Coba kartu SD lain — kartu SD lama atau murah sering tidak kompatibel
```

### 2. Data Hilang Setelah Reset?

```
❌ Penyebab umum: Lupa memanggil file.close() setelah menulis
✅ Solusi: SELALU close file setelah setiap operasi tulis
✅ Alternatif: Panggil file.flush() jika ingin tetap membuka file dalam waktu lama
```

### 3. Kecepatan SPI vs Stabilitas

| Frekuensi | Jarak Kabel | Status |
|-----------|-------------|--------|
| 400 kHz   | Semua       | ✅ Paling stabil (debugging) |
| 4 MHz     | <10 cm      | ✅ **Rekomendasi Kit Bluino** |
| 20 MHz    | <5 cm       | ⚠️ Hanya kabel sangat pendek |
| 40 MHz    | Sangat pendek | ❌ Sering tidak stabil |

### 4. SPI Tidak Boleh Dipakai Bersamaan Tanpa CS yang Benar

```cpp
// ❌ SALAH — kedua perangkat di-SELECT sekaligus
SPI.begin();
SD.begin(SD_CS);
display.begin();   // Jika display juga pakai SPI tanpa CS dikelola

// ✅ BENAR — hanya 1 CS aktif pada satu waktu
// Library yang baik (SD.h, TFT_eSPI, dll) mengelola CS internal
// Pastikan setiap perangkat punya pin CS TERSENDIRI
```

### 5. Kode Return `SD.begin()`

```cpp
if (!SD.begin(SD_CS, SPI, SD_SPI_FREQ)) {
  // Kemungkinan penyebab (urutan paling umum):
  // 1. Tidak ada kartu SD
  // 2. Format kartu salah (bukan FAT32)
  // 3. Pin CS salah
  // 4. Kecepatan SPI terlalu tinggi
  // 5. Kartu SD butuh waktu lebih lama untuk "warm up"
  //    → Coba tambah delay(500) sebelum SD.begin()
}
```

---

## 24.10 Ringkasan

```
┌─────────────────────────────────────────────────────────┐
│            RINGKASAN BAB 24 — SPI & SD CARD              │
├─────────────────────────────────────────────────────────┤
│ SPI: Sinkron, full-duplex, 4 kabel, clock dari Master   │
│                                                         │
│ Pin Micro SD di Kit Bluino (VSPI Default):              │
│   CS   = IO5   MOSI = IO23                              │
│   CLK  = IO18  MISO = IO19                              │
│                                                         │
│ Inisialisasi:                                           │
│   SPI.begin(CLK, MISO, MOSI)  ← Tanpa CS!              │
│   SD.begin(CS, SPI, 4000000UL)  ← CS diurus ke sini     │
│                                                         │
│ Kunci keberhasilan:                                     │
│   ✅ Format kartu: FAT32                                │
│   ✅ Kecepatan: 4 MHz (default)                         │
│   ✅ SELALU file.close() setelah menulis!               │
│   ✅ SELALU cek return value SD.begin() dan SD.open()   │
│                                                         │
│ Pantangan:                                              │
│   ❌ Lupa file.close() → data hilang saat mati lampu   │
│   ❌ Tulis ke SD tanpa cek ukuran → SD penuh & crash    │
│   ❌ Kecepatan SPI >4MHz dengan kabel panjang           │
│   ❌ Format exFAT/NTFS → SD tidak terdeteksi            │
└─────────────────────────────────────────────────────────┘
```

---

## 24.11 Latihan

1. **Logger Berformat Tanggal**: Modifikasi Program 3 agar nama file menggunakan format `/log_YYYYMMDD.csv` (gunakan counter sebagai pengganti tanggal nyata, atau kombinasikan dengan BAB WiFi untuk mendapatkan waktu NTP).

2. **Cek Ukuran Sebelum Tulis**: Tambahkan fungsi yang memeriksa sisa ruang kosong SD (`SD.cardSize() - SD.usedBytes()`) dan mencetak peringatan ke Serial jika sisa kurang dari 100 MB.

3. **Baca File Baris per Baris**: Buat program yang membaca file `/datalog.csv` kemudian menghitung **rata-rata suhu** dari semua nilai di kolom ketiga. Gunakan `strtok()` atau `split` untuk memisahkan nilai CSV.

4. **Manager File Sederhana**: Buat program interaktif. Saat pengguna mengetik perintah lewat Serial Monitor:
   - `LIST` → tampilkan semua file di root
   - `DEL:<namafile>` → hapus file tersebut
   - `NEW:<namafile>:<isi>` → buat file baru dengan isi yang diberikan

---

*Selanjutnya → [BAB 25: OneWire Protocol](../BAB-25/README.md)*
