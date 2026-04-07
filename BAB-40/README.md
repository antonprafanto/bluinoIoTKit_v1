# BAB 40: LittleFS — Sistem File di Flash ESP32

> ✅ **Prasyarat:** Selesaikan **BAB 39 (NVS — Memori Permanen)** dan **BAB 05 (Serial Monitor)**. Tidak ada komponen hardware tambahan yang diperlukan untuk materi inti bab ini.

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami **kapan harus memilih LittleFS vs NVS vs SD Card** untuk penyimpanan data.
- Mem-format dan menginisialisasi **partisi LittleFS** di Flash internal ESP32.
- Membuat, membaca, menulis, dan menghapus **file teks** di LittleFS.
- Melakukan **navigasi direktori** (list file, cek ukuran, free space).
- Menyimpan **data log sensor** dalam format CSV ke file secara non-blocking.
- Membangun sistem **konfigurasi berbasis file JSON-like** yang terbaca manusia.
- Memahami **batasan dan risiko** LittleFS dibandingkan kartu SD eksternal.

---

## 40.1 Hierarki Penyimpanan ESP32 — Kapan Pakai Apa?

```
PANDUAN MEMILIH JENIS PENYIMPANAN:

  Jenis Data        │ Ukuran    │ Rekomendasi
  ──────────────────┼───────────┼────────────────────────────────
  Konfigurasi kecil │ < 4 KB    │ NVS (Preferences.h) ← BAB 39
  File teks/JSON    │ 4KB–1MB   │ LittleFS ← BAB INI
  Log data harian   │ 1MB–4MB   │ LittleFS (jika muat)
  Log data jangka   │ > 4MB     │ SD Card (MicroSD) ← BAB 41
  panjang / besar   │           │
  ──────────────────┴───────────┴────────────────────────────────

  FLASH 4MB TYPICAL PADA ESP32:
  ┌──────────────┬──────────────┬──────────────┬──────────────┐
  │  Bootloader  │  Program     │     NVS      │  LittleFS   │
  │   ~28 KB     │  ~1.5 MB     │  ~20 KB      │  ~1.5 MB    │
  └──────────────┴──────────────┴──────────────┴──────────────┘
  (Ukuran tergantung Partition Scheme yang dipilih di Arduino IDE)
```

### LittleFS vs SPIFFS — Mana yang Dipakai?

| Aspek | SPIFFS (lama) | LittleFS (baru, direkomendasikan) |
|-------|---------------|-----------------------------------|
| Status | Deprecated (tidak dikembangkan lagi) | ✅ Aktif & direkomendasikan |
| Direktori | ❌ Flat (tidak ada folder) | ✅ Hierarki folder penuh |
| Wear-leveling | Basic | ✅ Lebih canggih |
| Power-loss safety | Rentan korupsi | ✅ Lebih tahan gangguan daya |
| Arduino ESP32 | Tersedia | ✅ Tersedia (default sejak core 2.x) |

> 💡 **Selalu gunakan LittleFS** untuk proyek baru. SPIFFS masih disebutkan di sini hanya untuk referensi kode legacy yang mungkin kamu temui.

---

## 40.2 Setup Partition Scheme

Sebelum menggunakan LittleFS, pastikan Partition Scheme sudah dikonfigurasi untuk menyediakan ruang untuk filesystem:

```
MENGATUR PARTITION SCHEME DI ARDUINO IDE:
  1. Buka: Tools → Board → ESP32 Dev Module
  2. Buka: Tools → Partition Scheme
  3. Pilih: "Default 4MB with spiffs (1.2MB APP / 1.5MB SPIFFS)"
             atau
             "Huge APP (3MB No OTA / 1MB SPIFFS)"
             (LittleFS menggunakan partisi yang sama dengan SPIFFS)
  4. Re-upload sketch kamu

CATATAN PENTING:
  Jika kamu mengubah Partition Scheme, perlu erase flash terlebih dahulu:
  Tools → Erase All Flash Before Sketch Upload → Enabled (saat upload pertama)
  Setelah upload pertama, kembalikan ke "Disabled" agar LittleFS tidak terhapus.
```

---

## 40.3 Referensi API LittleFS

```cpp
#include <LittleFS.h>

// ── Inisialisasi ──────────────────────────────────────────────────
LittleFS.begin();              // Mount filesystem (FORMAT jika gagal = false)
LittleFS.begin(true);          // Mount + format otomatis jika belum ada
LittleFS.end();                // Unmount (jarang diperlukan)
LittleFS.format();             // Format ulang — hapus SEMUA file!

// ── Info Kapasitas ────────────────────────────────────────────────
size_t total = LittleFS.totalBytes();   // Total kapasitas partisi
size_t used  = LittleFS.usedBytes();    // Digunakan
size_t free  = total - used;            // Sisa

// ── Operasi File ──────────────────────────────────────────────────
File f = LittleFS.open("/data.txt", "r");   // Buka baca
File f = LittleFS.open("/data.txt", "w");   // Buka tulis (hapus isi lama)
File f = LittleFS.open("/data.txt", "a");   // Buka append (tambah di akhir)
bool ok = LittleFS.exists("/data.txt");     // Cek keberadaan file
bool ok = LittleFS.remove("/data.txt");     // Hapus file
bool ok = LittleFS.rename("/a.txt", "/b.txt"); // Rename/pindah

// ── Operasi File (setelah open) ───────────────────────────────────
f.print("teks");            // Tulis teks (seperti Serial.print)
f.println("baris");         // Tulis teks + newline
f.printf("%.2f", nilai);    // Tulis terformat
f.write(buf, len);          // Tulis raw bytes
String baris = f.readStringUntil('\n');  // Baca satu baris
String semua = f.readString();           // Baca seluruh isi
size_t n = f.read(buf, len);             // Baca raw bytes
bool  ok = f.available();               // Ada data lagi?
size_t sz = f.size();                   // Ukuran file (bytes)
f.close();                              // WAJIB tutup setelah selesai!

// ── Operasi Direktori ─────────────────────────────────────────────
LittleFS.mkdir("/logs");                // Buat direktori
LittleFS.rmdir("/logs");                // Hapus direktori (harus kosong)
File dir = LittleFS.open("/");          // Buka direktori root
File entry = dir.openNextFile();        // Iterasi isi direktori
while (entry) {
  Serial.println(entry.name());
  entry = dir.openNextFile();
}
```

---

## 40.4 Program 1: Hello LittleFS — Tulis dan Baca File

```cpp
/*
 * BAB 40 - Program 1: Hello LittleFS
 *
 * Tujuan:
 *   Operasi file dasar: inisialisasi, tulis, baca, cek info.
 *   Setiap boot menambahkan satu baris ke file yang sama (append),
 *   lalu membaca dan menampilkan seluruh isi file.
 *
 * Hardware: Hanya ESP32
 *
 * Cara uji:
 *   1. Upload dan buka Serial Monitor
 *   2. Reset ESP32 beberapa kali
 *   3. Perhatikan baris di file terus bertambah!
 */

#include <LittleFS.h>

#define LOG_FILE "/hello.txt"

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("=== BAB 40 Program 1: Hello LittleFS ===");

  // ── Mount LittleFS ────────────────────────────────────────────
  if (!LittleFS.begin(true)) { // true = format otomatis jika belum ada
    Serial.println("[ERR] LittleFS gagal dimount!");
    return;
  }
  Serial.println("[OK] LittleFS berhasil dimount.");

  // ── Info partisi ──────────────────────────────────────────────
  size_t total = LittleFS.totalBytes();
  size_t used  = LittleFS.usedBytes();
  Serial.printf("[INFO] Storage: %u bytes digunakan dari %u bytes (%.1f%%)\n",
    used, total, 100.0f * used / total);

  // ── Append satu baris ke file ─────────────────────────────────
  File f = LittleFS.open(LOG_FILE, "a"); // Mode append
  if (!f) {
    Serial.println("[ERR] Gagal membuka file untuk ditulis!");
    return;
  }
  f.printf("Boot #%lu — millis: %lu\n", 
    (unsigned long)LittleFS.usedBytes(), // Trik: pakai usedBytes sbg proxy unik
    millis());
  f.close();
  Serial.println("[OK] Baris baru ditambahkan ke " LOG_FILE);

  // ── Baca seluruh isi file ─────────────────────────────────────
  Serial.println("\n── Isi file " LOG_FILE " ──");
  f = LittleFS.open(LOG_FILE, "r");
  if (!f) {
    Serial.println("[ERR] Gagal membuka file untuk dibaca!");
    return;
  }
  int lineNum = 1;
  while (f.available()) {
    String baris = f.readStringUntil('\n');
    Serial.printf("  %2d: %s\n", lineNum++, baris.c_str());
  }
  f.close();
  Serial.println("────────────────────────────────────");
}

void loop() {}
```

**Output contoh (setelah 3x reset):**
```text
=== BAB 40 Program 1: Hello LittleFS ===
[OK] LittleFS berhasil dimount.
[INFO] Storage: 8192 bytes digunakan dari 1441792 bytes (0.6%)
[OK] Baris baru ditambahkan ke /hello.txt

── Isi file /hello.txt ──
   1: Boot #8100 — millis: 512
   2: Boot #8130 — millis: 508
   3: Boot #8160 — millis: 511
────────────────────────────────────
```

---

## 40.5 Program 2: CSV Data Logger Sensor

Program ini membangun sistem **pencatatan data sensor ke file CSV** yang bisa dibuka di Excel atau Google Sheets. Ini adalah fitur inti dari hampir semua IoT data-acquisition device.

```cpp
/*
 * BAB 40 - Program 2: CSV Data Logger (DHT11 → LittleFS)
 *
 * Fitur:
 *   ▶ Baca suhu & kelembaban dari DHT11 (IO27)
 *   ▶ Tulis data ke /logs/data.csv dalam format standar
 *   ▶ Header CSV otomatis jika file baru
 *   ▶ Logging setiap 5 detik (non-blocking)
 *   ▶ List semua file di direktori /logs/
 *   ▶ Hapus log via perintah Serial "clear"
 *   ▶ Tampilkan isi log via perintah Serial "dump"
 *
 * Hardware:
 *   DHT11 → IO27 (hardwired di kit, pull-up 4K7Ω bawaan)
 *
 * Format CSV:
 *   uptime_ms,suhu_C,kelembaban_%
 *   1000,28.5,65
 *   6000,28.6,64
 */

#include <LittleFS.h>
#include <DHT.h>

#define DHT_PIN  27
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

#define LOG_DIR  "/logs"
#define LOG_FILE "/logs/data.csv"

// ── Non-blocking timing ───────────────────────────────────────────
const unsigned long LOG_INTERVAL_MS = 5000UL;  // Log setiap 5 detik
unsigned long tLog = 0;
uint32_t logCount = 0;

bool fsOk = false;

// ── Inisialisasi LittleFS + direktori ────────────────────────────
bool initFS() {
  if (!LittleFS.begin(true)) {
    Serial.println("[ERR] LittleFS gagal mount!");
    return false;
  }
  // Buat direktori /logs jika belum ada
  if (!LittleFS.exists(LOG_DIR)) {
    LittleFS.mkdir(LOG_DIR);
    Serial.println("[FS] Direktori /logs dibuat.");
  }
  return true;
}

// ── Tulis header CSV jika file baru ──────────────────────────────
void ensureHeader() {
  if (!LittleFS.exists(LOG_FILE)) {
    File f = LittleFS.open(LOG_FILE, "w");
    if (f) {
      f.println("uptime_ms,suhu_C,kelembaban_pct,status");
      f.close();
      Serial.println("[CSV] Header ditulis ke file baru.");
    }
  }
}

// ── Tulis satu baris data log ─────────────────────────────────────
void logData(unsigned long now, float temp, float hum) {
  File f = LittleFS.open(LOG_FILE, "a");
  if (!f) {
    Serial.println("[ERR] Gagal membuka log untuk append!");
    return;
  }
  // Format: uptime_ms,suhu,kelembaban,status
  if (isnan(temp) || isnan(hum)) {
    f.printf("%lu,ERR,ERR,SENSOR_FAIL\n", now);
  } else {
    f.printf("%lu,%.1f,%.0f,OK\n", now, temp, hum);
  }
  f.close();
  logCount++;
}

// ── List semua file di direktori /logs ────────────────────────────
void listFiles() {
  Serial.println("\n── File di /logs/ ──");
  File dir = LittleFS.open(LOG_DIR);
  if (!dir || !dir.isDirectory()) {
    Serial.println("  (direktori kosong atau tidak ada)");
    return;
  }
  File entry = dir.openNextFile();
  int count = 0;
  while (entry) {
    Serial.printf("  %-30s %7u bytes\n", entry.name(), entry.size());
    entry = dir.openNextFile();
    count++;
  }
  if (count == 0) Serial.println("  (kosong)");
  size_t tot = LittleFS.totalBytes();
  size_t use = LittleFS.usedBytes();
  Serial.printf("\nTotal: %u / %u bytes (%.1f%% penuh)\n\n",
    use, tot, 100.0f * use / tot);
}

// ── Dump isi CSV ke Serial ────────────────────────────────────────
void dumpLog() {
  Serial.println("\n── Isi " LOG_FILE " ──");
  File f = LittleFS.open(LOG_FILE, "r");
  if (!f) { Serial.println("  (file tidak ada)"); return; }
  
  uint32_t totalBaris = 0;
  Serial.println("─────────────────────────────────────────");
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() > 0) {
      Serial.println(line);
      // Jangan hitung baris header sebagai row data
      if (!line.startsWith("uptime_ms")) {
        totalBaris++;
      }
    }
  }
  f.close();
  Serial.printf("\nTotal log historis: %u baris (Sesi aktif: %u)\n\n", totalBaris, logCount);
}

// ── Hapus file log ────────────────────────────────────────────────
void clearLog() {
  if (LittleFS.remove(LOG_FILE)) {
    ensureHeader();
    logCount = 0;
    Serial.println("[OK] Log dihapus. Header baru disiapkan.");
  } else {
    Serial.println("[ERR] Gagal menghapus log.");
  }
}

// ── Serial CLI ────────────────────────────────────────────────────
String serialBuf = "";

void handleCmd(String cmd) {
  cmd.trim();
  if      (cmd == "dump")  dumpLog();
  else if (cmd == "list")  listFiles();
  else if (cmd == "clear") clearLog();
  else if (cmd == "help") {
    Serial.println("Perintah: dump | list | clear | help");
  } else if (cmd.length() > 0) {
    Serial.printf("[ERR] Tidak dikenal: '%s'\n", cmd.c_str());
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  delay(1000); // Tunggu DHT11 stabilisasi

  Serial.println("=== BAB 40 Program 2: CSV Data Logger ===");
  Serial.println("Perintah: 'dump' | 'list' | 'clear' | 'help'");

  fsOk = initFS();
  if (fsOk) {
    ensureHeader();
    listFiles();
  }
}

void loop() {
  unsigned long now = millis();

  // ── Baca Serial CLI ───────────────────────────────────────────
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      handleCmd(serialBuf);
      serialBuf = "";
    } else {
      if (serialBuf.length() < 64) serialBuf += c; // Mencegah buffer overflow OOM!
    }
  }

  // ── Log data setiap LOG_INTERVAL_MS ──────────────────────────
  if (fsOk && (now - tLog >= LOG_INTERVAL_MS)) {
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

**Output contoh:**
```text
=== BAB 40 Program 2: CSV Data Logger ===
Perintah: 'dump' | 'list' | 'clear' | 'help'
[FS] Direktori /logs dibuat.
[CSV] Header ditulis ke file baru.

── File di /logs/ ──
  data.csv                            48 bytes

Total: 8240 / 1441792 bytes (0.6% penuh)

[LOG #1]  5012 ms | 28.5°C | 65%
[LOG #2] 10013 ms | 28.6°C | 64%
[LOG #3] 15014 ms | 29.0°C | 63%
```

> 💡 **Kenapa `delay(1000)` di `setup()`?** DHT11 memerlukan waktu stabilisasi ~1-2 detik setelah power-on sebelum pembacaan pertama valid. Ini adalah satu-satunya `delay()` yang diperbolehkan — semua logging berikutnya menggunakan `millis()`.

---

## 40.6 Program 3: Konfigurasi Berbasis File (INI Parser sederhana)

Program ini menyimpan konfigurasi dalam format file teks yang **bisa dibaca dan diedit manusia** — mirip dengan file `.ini` atau `.cfg` di sistem komputer. Sangat berguna untuk device yang dikonfigurasi di lapangan tanpa harus upload firmware ulang.

```cpp
/*
 * BAB 40 - Program 3: File-Based Configuration Manager
 *
 * Format file /config.ini:
 *   brightness=75
 *   temp_alarm=35.5
 *   device_name=Bluino-Lab
 *   alarm_enabled=1
 *
 * Fitur:
 *   ▶ Parse file config "key=value" saat boot
 *   ▶ Buat config default jika file belum ada
 *   ▶ Update via Serial: SET key value
 *   ▶ Simpan perubahan ke file
 *   ▶ RELOAD: baca ulang config dari file
 *
 * Hardware: Hanya ESP32
 */

#include <LittleFS.h>

#define CFG_FILE "/config.ini"

// ── Konfigurasi ───────────────────────────────────────────────────
uint8_t cfgBright  = 75;
float   cfgTempAlm = 35.0f;
String  cfgName    = "Bluino-IoT";
bool    cfgAlarm   = true;

// ── Tulis config default ──────────────────────────────────────────
void writeDefaultConfig() {
  File f = LittleFS.open(CFG_FILE, "w");
  if (!f) { Serial.println("[ERR] Gagal tulis config!"); return; }
  f.printf("brightness=%d\n",       cfgBright);
  f.printf("temp_alarm=%.1f\n",     cfgTempAlm);
  f.printf("device_name=%s\n",      cfgName.c_str());
  f.printf("alarm_enabled=%d\n",    cfgAlarm ? 1 : 0);
  f.close();
  Serial.println("[CFG] Config default ditulis ke " CFG_FILE);
}

// ── Parse satu baris "key=value" ──────────────────────────────────
void parseLine(const String &line) {
  int sep = line.indexOf('=');
  if (sep < 0) return;
  String key = line.substring(0, sep);
  String val = line.substring(sep + 1);
  key.trim(); val.trim();

  if (key == "brightness") {
    int b = val.toInt();
    cfgBright = (uint8_t)(b < 0 ? 0 : (b > 100 ? 100 : b)); // Constrain 0-100
  }
  else if (key == "temp_alarm") {
    float t = val.toFloat();
    if (t >= -40.0f && t <= 125.0f) cfgTempAlm = t; // Batas wajar sensor
  }
  else if (key == "device_name") {
    if (val.length() > 0 && val.length() <= 15) cfgName = val;
  }
  else if (key == "alarm_enabled") {
    cfgAlarm = (val == "1" || val == "true");
  }
}

// ── Muat config dari file ─────────────────────────────────────────
bool loadConfig() {
  if (!LittleFS.exists(CFG_FILE)) {
    Serial.println("[CFG] File config belum ada, membuat default...");
    writeDefaultConfig();
    return true;
  }
  File f = LittleFS.open(CFG_FILE, "r");
  if (!f) { Serial.println("[ERR] Gagal buka config!"); return false; }
  while (f.available()) {
    String line = f.readStringUntil('\n');
    line.trim();
    if (line.length() > 0 && line[0] != '#') // Abaikan komentar (#)
      parseLine(line);
  }
  f.close();
  Serial.println("[CFG] Config dimuat dari " CFG_FILE);
  return true;
}

// ── Simpan config ke file ─────────────────────────────────────────
void saveConfig() {
  File f = LittleFS.open(CFG_FILE, "w");
  if (!f) { Serial.println("[ERR] Gagal simpan config!"); return; }
  f.println("# Bluino IoT Config File");
  f.println("# Format: key=value (satu per baris)");
  f.printf("brightness=%d\n",    cfgBright);
  f.printf("temp_alarm=%.1f\n",  cfgTempAlm);
  f.printf("device_name=%s\n",   cfgName.c_str());
  f.printf("alarm_enabled=%d\n", cfgAlarm ? 1 : 0);
  f.close();
  Serial.println("[CFG] Config disimpan ke " CFG_FILE);
}

// ── Tampilkan config ──────────────────────────────────────────────
void printConfig() {
  Serial.println("\n┌─────────────────────────────────┐");
  Serial.println("│        Konfigurasi Aktif         │");
  Serial.println("├─────────────────────────────────┤");
  Serial.printf ("│  brightness    = %d%%\n",        cfgBright);
  Serial.printf ("│  temp_alarm    = %.1f °C\n",     cfgTempAlm);
  Serial.printf ("│  device_name   = %s\n",          cfgName.c_str());
  Serial.printf ("│  alarm_enabled = %s\n",          cfgAlarm ? "true" : "false");
  Serial.println("└─────────────────────────────────┘\n");
}

// ── Dump isi file config raw ──────────────────────────────────────
void dumpConfigFile() {
  Serial.println("\n── Raw " CFG_FILE " ──");
  File f = LittleFS.open(CFG_FILE, "r");
  if (!f) { Serial.println("  (tidak ada)"); return; }
  while (f.available()) Serial.println(f.readStringUntil('\n'));
  f.close();
  Serial.println("────────────────────────────\n");
}

// ── Serial CLI ────────────────────────────────────────────────────
String serialBuf = "";

void handleCmd(String cmd) {
  cmd.trim();
  if (cmd == "get")          printConfig();
  else if (cmd == "dump")    dumpConfigFile();
  else if (cmd == "reload")  { loadConfig(); printConfig(); }
  else if (cmd == "save")    saveConfig();
  else if (cmd == "help") {
    Serial.println("Perintah:");
    Serial.println("  get                     - Tampilkan config aktif");
    Serial.println("  dump                    - Tampilkan raw file");
    Serial.println("  set brightness <0-100>  - Ubah brightness");
    Serial.println("  set temp_alarm <nilai>  - Ubah threshold suhu");
    Serial.println("  set device_name <teks>  - Ubah nama");
    Serial.println("  set alarm_enabled <0/1> - On/off alarm");
    Serial.println("  save                    - Simpan ke file");
    Serial.println("  reload                  - Muat ulang dari file");
  } else if (cmd.startsWith("set ")) {
    // Format: "set key value"
    String rest = cmd.substring(4);
    int sp = rest.indexOf(' ');
    if (sp < 0) { Serial.println("[ERR] Format: set <key> <value>"); return; }
    String key = rest.substring(0, sp);
    String val = rest.substring(sp + 1);
    key.trim(); val.trim();
    parseLine(key + "=" + val);
    Serial.printf("[OK] %s = %s (belum disimpan, ketik 'save')\n",
      key.c_str(), val.c_str());
  } else if (cmd.length() > 0) {
    Serial.printf("[ERR] Tidak dikenal: '%s'. Ketik 'help'.\n", cmd.c_str());
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);

  if (!LittleFS.begin(true)) {
    Serial.println("[ERR] LittleFS gagal!"); return;
  }

  Serial.println("=== BAB 40 Program 3: File-Based Config Manager ===");
  loadConfig();
  printConfig();
  Serial.println("Ketik 'help' untuk panduan perintah.");
}

void loop() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      handleCmd(serialBuf);
      serialBuf = "";
    } else {
      if (serialBuf.length() < 64) serialBuf += c; // Mencegah buffer overflow OOM!
    }
  }
}
```

**Contoh file `/config.ini` yang tersimpan di Flash:**
```ini
# Bluino IoT Config File
# Format: key=value (satu per baris)
brightness=50
temp_alarm=40.0
device_name=Bluino-Lab
alarm_enabled=0
```

> 💡 **Keunggulan file config vs NVS:** File config bisa diedit pada host computer menggunakan fitur upload file LittleFS (tool `arduino-littlefs-upload` untuk Arduino IDE 2.x), tanpa perlu mengunggah ulang firmware. Ini sangat berguna untuk konfigurasi di lapangan!

---

## 40.7 Tips & Panduan Troubleshooting

### 1. `LittleFS.begin()` selalu mengembalikan `false`?
```text
Solusi bertingkat:

A. Partition Scheme belum diatur
   → Tools → Partition Scheme → Pilih yang ada "/spiffs" atau "/littlefs"
   → Upload ulang firmware

B. Flash belum pernah diformat untuk LittleFS
   → Gunakan: LittleFS.begin(true) ← parameter true = format jika perlu
   → Atau: Erase All Flash → Upload ulang

C. Ukuran partisi 0 (Partition Scheme "No OTA, 4MB APP")
   → Tidak ada ruang untuk filesystem!
   → Ganti ke partition scheme yang menyediakan SPIFFS/Filesystem
```

### 2. File yang ditulis tidak muncul saat di-list?
```text
Penyebab: File belum di-close() atau masih di buffer.

  File f = LittleFS.open("/test.txt", "w");
  f.println("data");
  // ❌ Lupa f.close() → data belum pasti tersimpan ke Flash!
  f.close(); // ✅ WAJIB

LittleFS menggunakan buffer internal. Data baru benar-benar
di-flush ke Flash saat f.close() dipanggil.
```

### 3. Ruang Flash cepat habis — padahal data sedikit?
```text
LittleFS mengalokasikan blok minimum 4KB per file.
Bahkan file 10 byte masih memakan 4KB di Flash!

Strategi hemat ruang:
  ✅ Gabungkan data dalam format CSV satu file besar
     (bukan banyak file kecil terpisah)
  ✅ Hapus data lama secara berkala (rolling log)
  ✅ Kompresi data sebelum disimpan (butuh library tambahan)
  ✅ Pertimbangkan SD Card untuk data > 500KB
```

### 4. ESP32 mati saat menulis — apakah data korup?
```text
LittleFS dirancang tahan gangguan daya (power-loss safe):
  ✅ Jika power mati SEBELUM f.close() → baris terakhir mungkin hilang,
     tapi file tidak korup secara struktural
  ✅ Filesystem mount berikutnya akan berhasil
  ✅ Data yang sudah di-close() aman

Untuk data kritikal (seperti keuangan/medis):
  Gunakan write-verify: tulis ke file sementara, verifikasi, rename.
```

### 5. Upload file ke LittleFS dari komputer (tanpa menulis kode)?
```text
Gunakan plugin arduino-littlefs-upload:
  1. Install plugin: github.com/earlephilhower/arduino-littlefs-upload
  2. Buat folder "data/" di direktori sketch kamu
  3. Letakkan file (HTML, config, gambar) di folder data/
  4. Arduino IDE 2.x: Tools → "LittleFS Data Upload"
  5. File akan diupload ke partisi LittleFS Flash ESP32

Ini memungkinkan kamu menyimpan webpage HTML di ESP32 untuk web server!
```

---

## 40.8 Ringkasan

```
┌─────────────────────────────────────────────────────────────────────┐
│               RINGKASAN BAB 40 — LittleFS ESP32                     │
├─────────────────────────────────────────────────────────────────────┤
│ KAPAN PAKAI LittleFS:                                                │
│   ✅ Menyimpan file teks, JSON, HTML, konfigurasi                   │
│   ✅ Data log 4KB–1MB                                               │
│   ✅ File yang perlu diedit/dibaca oleh manusia                     │
│   ❌ Data besar (> LittleFS partition size) → pakai SD Card         │
│   ❌ Key-value kecil (< 4KB total) → lebih efisien pakai NVS       │
│                                                                       │
│ API ESENSIAL:                                                         │
│   LittleFS.begin(true)               → Mount (+ autoformat)         │
│   LittleFS.open("/f.txt", "r/w/a")   → Buka file                   │
│   f.println() / f.printf()           → Tulis data                  │
│   f.readStringUntil('\n')            → Baca per baris               │
│   f.close()                          → WAJIB setelah selesai!      │
│   LittleFS.exists() / remove()       → Cek / Hapus file            │
│   LittleFS.totalBytes() / usedBytes()→ Cek kapasitas               │
│                                                                       │
│ BEST PRACTICES:                                                       │
│   ✅ Selalu f.close() setelah operasi — jangan skip!                │
│   ✅ Gunakan mode "a" (append) untuk data log, bukan "w"            │
│   ✅ Cek LittleFS.begin() return value — tangani jika false         │
│   ✅ Monitor usedBytes() — hindari flash penuh (bisa corrupt)       │
│   ✅ Format file CSV/INI untuk data yang perlu dibaca manusia       │
│                                                                       │
│ ANTIPATTERN:                                                          │
│   ❌ Lupa f.close() → data hilang atau file korup                   │
│   ❌ Buka file dalam loop() tanpa menutupnya                        │
│   ❌ Menulis ribuan file kecil (boros blok 4KB per file)            │
│   ❌ Tidak cek free space sebelum menulis data besar                │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 40.9 Latihan

1. **Rolling Log (200 Baris Maks):** Modifikasi Program 2 agar file CSV tidak pernah melebihi 200 baris. Saat mencapai batas, hapus 50 baris pertama (paling lama) secara otomatis. Implementasikan tanpa membaca seluruh file ke RAM — baca baris ke file sementara `/logs/tmp.csv`, hapus original, rename tmp ke original.

2. **Web Server File Static:** Kombinasikan LittleFS dengan WiFi ESP32. Upload file `index.html` ke folder `data/` menggunakan plugin LittleFS Upload. Jalankan Web Server sederhana yang melayani file tersebut saat browser mengakses IP ESP32. (Petunjuk: Gunakan library `ESPAsyncWebServer` atau `WebServer`).

3. **JSON Config Parser:** Ganti format konfigurasi Program 3 dari INI ke JSON sederhana menggunakan library **ArduinoJson** (`{"brightness":75,"temp_alarm":35.0,"name":"Bluino"}`). Muat, ubah, dan simpan kembali sebagai JSON yang valid. Bandingkan kemudahan developer vs format INI sederhana.

4. **Directory Browser via Serial:** Buat program yang bisa menelusuri hierarki direktori via Serial CLI. Perintah: `ls` (list isi direktori saat ini), `cd <folder>` (masuk folder), `cd ..` (naik ke parent), `cat <file>` (tampilkan isi), `rm <file>` (hapus). Ini mirip shell Unix mini di dalam ESP32!

5. **Dual Storage Strategy:** Buat sistem yang menggabungkan NVS (BAB 39) dan LittleFS secara bersamaan. NVS menyimpan konfigurasi cepat (mode, threshold), LittleFS menyimpan log data CSV. Saat ESP32 boot, muat konfigurasi dari NVS, lalu lanjutkan logging ke file di LittleFS yang ada. Demonstrasikan bahwa keduanya bekerja harmonis di satu program.

---

## ☕ Dukung Proyek Ini

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

---

*Sebelumnya → [BAB 39: NVS — Memori Permanen ESP32](../BAB-39/README.md)*

*Selanjutnya → [BAB 41: WiFi — Koneksi Internet](../BAB-41/README.md)*
