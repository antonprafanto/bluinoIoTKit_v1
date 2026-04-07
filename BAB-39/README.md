# BAB 39: NVS — Memori Permanen ESP32

> ✅ **Prasyarat:** Selesaikan **BAB 05 (Serial Monitor)**, **BAB 09 (Digital Input — Push Button)**, dan **BAB 16 (Non-Blocking Programming)**. Tidak ada komponen tambahan yang diperlukan untuk BAB ini — semua program berjalan hanya dengan ESP32 dan Serial Monitor.

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami **mengapa data hilang saat ESP32 di-reset** dan bagaimana NVS menyelesaikannya.
- Menjelaskan **perbedaan fundamental antara EEPROM, Flash NVS, dan RAM**.
- Menggunakan library **`Preferences.h`** (bawaan Arduino ESP32) untuk membaca dan menulis data permanen.
- Menyimpan berbagai tipe data: integer, float, String, dan boolean ke Flash.
- Menerapkan **pola Write-on-Change** (hanya tulis saat data berubah) untuk melindungi umur Flash.
- Membangun **sistem manajemen pengaturan (Settings Manager)** yang persisten lintas reboot.
- Merancang **FSM dengan memori** yang mengingat mode terakhir setelah daya diputus.

---

## 39.1 Mengapa Data Hilang Setelah Reset?

### Peta Memori ESP32

```
JENIS MEMORI ESP32:
┌─────────────────────────────────────────────────────────────────┐
│  RAM (SRAM) — 520 KB                                            │
│  ├─ Semua variabel C++ (int, float, String...)                  │
│  ├─ Stack program, heap malloc                                   │
│  └─ ⚠️ HILANG TOTAL saat reset atau mati listrik!              │
├─────────────────────────────────────────────────────────────────┤
│  Flash — 4 MB (umumnya)                                         │
│  ├─ [0x000000] Bootloader                                        │
│  ├─ [0x001000] Partition Table                                   │
│  ├─ [0x008000] Program kode (.ino / .bin) yang kamu upload      │
│  ├─ [0x...   ] NVS Partition (~20 KB default)  ← KITA PAKAI    │
│  └─ [0x...   ] SPIFFS / LittleFS (opsional)                    │
│  ✅ TIDAK HILANG saat reset — tersimpan permanen               │
└─────────────────────────────────────────────────────────────────┘
```

### EEPROM vs NVS — Mengapa Pilih NVS?

| Aspek | EEPROM.h (Arduino AVR) | NVS / Preferences.h (ESP32) |
|-------|------------------------|------------------------------|
| Media | Chip EEPROM terpisah | Flash internal ESP32 |
| Akses | Per-byte (tulis manual per alamat) | Key-Value (nama sebagai kunci) |
| Wear Leveling | ❌ Tidak ada | ✅ Otomatis (memperpanjang umur Flash) |
| Tipe Data | Byte manual, casting manual | Native: int, float, String, bool, blob |
| Namespace | ❌ Tidak ada (flat) | ✅ Ada (isolasi antar fitur) |
| Endurance | ~100.000 siklus tulis | ~10.000 siklus per halaman (wear-leveled) |
| Keamanan | Tidak ada | Checksum CRC bawaan |

> 💡 **Wear Leveling** = NVS tidak menulis ulang ke halaman Flash yang sama setiap kali. Ia mendistribusikan tulisan ke seluruh area NVS secara rotasi, sehingga umur efektif penyimpanan jauh lebih panjang.

---

## 39.2 Arsitektur NVS — Namespace dan Key-Value

NVS terorganisir dalam **namespace** (seperti folder) yang berisi pasangan **key → value**.

```
STRUKTUR NVS (Ilustrasi):

  NVS Flash Partition
  ├── Namespace: "settings"
  │     ├── "brightness"  : uint8  = 75
  │     ├── "threshold"   : float  = 32.5
  │     └── "deviceName"  : String = "Bluino-001"
  │
  ├── Namespace: "fsm"
  │     ├── "lastMode"    : int32  = 2
  │     └── "servoPos"    : int32  = 135
  │
  └── Namespace: "boot"
        └── "bootCount"   : uint32 = 47

Aturan:
  ▶ Nama namespace : maks 15 karakter
  ▶ Nama key       : maks 15 karakter
  ▶ Satu value per key (overwrite jika key sudah ada)
  ▶ Namespace dibuat otomatis saat pertama kali digunakan
```

---

## 39.3 API Library Preferences.h — Referensi Lengkap

```cpp
#include <Preferences.h>
Preferences prefs;

// ── Membuka / Menutup ────────────────────────────────────────────
prefs.begin("namespace", false);  // false = read-write mode
prefs.begin("namespace", true);   // true  = read-only mode (lebih aman)
prefs.end();                      // WAJIB dipanggil setelah selesai!

// ── Menulis Data ─────────────────────────────────────────────────
prefs.putUChar  ("key", value);   // uint8_t  (0–255)
prefs.putChar   ("key", value);   // int8_t   (-128 – 127)
prefs.putUShort ("key", value);   // uint16_t
prefs.putShort  ("key", value);   // int16_t
prefs.putUInt   ("key", value);   // uint32_t
prefs.putInt    ("key", value);   // int32_t
prefs.putULong  ("key", value);   // uint32_t (alias)
prefs.putFloat  ("key", value);   // float
prefs.putDouble ("key", value);   // double
prefs.putBool   ("key", value);   // bool
prefs.putString ("key", value);   // String / char*
prefs.putBytes  ("key", buf, len);// byte array (struct, dll)

// ── Membaca Data (dengan default value jika key belum ada) ───────
uint8_t  v = prefs.getUChar  ("key", 0);
int32_t  v = prefs.getInt    ("key", 0);
float    v = prefs.getFloat  ("key", 0.0f);
bool     v = prefs.getBool   ("key", false);
String   v = prefs.getString ("key", "default");
size_t   n = prefs.getBytes  ("key", buf, maxLen);

// ── Utilitas ──────────────────────────────────────────────────────
bool exists = prefs.isKey("key");          // Cek apakah key ada
prefs.remove("key");                       // Hapus satu key
prefs.clear();                             // Hapus SEMUA key di namespace
size_t free = prefs.freeEntries();         // Sisa entri yang tersedia
```

> ⚠️ **WAJIB:** Selalu panggil `prefs.end()` setelah selesai read/write. Lupa memanggil `end()` dapat menyebabkan NVS handle tidak dilepas dan program berikutnya gagal membuka namespace yang sama!

---

## 39.4 Program 1: Boot Counter — Hello World NVS

Program paling sederhana untuk membuktikan NVS bekerja: sebuah penghitung yang terus bertambah setiap kali ESP32 di-reset, dan nilainya **tidak hilang** meskipun daya diputus.

```cpp
/*
 * BAB 39 - Program 1: Boot Counter (Hello World NVS)
 *
 * Tujuan:
 *   Membuktikan bahwa nilai yang disimpan di NVS TIDAK hilang
 *   setelah ESP32 di-reset atau dimatikan. Setiap boot, counter
 *   bertambah 1 dan nilainya ditampilkan di Serial Monitor.
 *
 * Cara uji:
 *   1. Upload dan buka Serial Monitor (115200 baud)
 *   2. Tekan tombol EN/RST di ESP32 beberapa kali
 *   3. Perhatikan angka boot terus bertambah dan tidak reset ke 0!
 *
 * Hardware: Hanya ESP32 (tidak ada komponen tambahan)
 */

#include <Preferences.h>

Preferences prefs;

void setup() {
  Serial.begin(115200);
  delay(500); // Beri waktu Serial Monitor terhubung

  // Buka namespace "boot" dalam mode read-write
  prefs.begin("boot", false);

  // Baca nilai saat ini (default 0 jika belum pernah disimpan)
  uint32_t bootCount = prefs.getUInt("count", 0);

  // Tambahkan 1 dan simpan kembali
  bootCount++;
  prefs.putUInt("count", bootCount);

  // Tutup — WAJIB!
  prefs.end();

  // Tampilkan hasil
  Serial.println("╔══════════════════════════════════════╗");
  Serial.println("║   BAB 39 - Program 1: Boot Counter   ║");
  Serial.println("╚══════════════════════════════════════╝");
  Serial.printf("  Boot ke: %u\n", bootCount);
  Serial.println();
  Serial.println("  Coba tekan RST/EN button...");
  Serial.println("  Angka akan terus bertambah, tidak reset ke 0!");
}

void loop() {
  // Tidak ada yang perlu dilakukan di loop()
}
```

**Contoh output Serial Monitor setelah beberapa kali reset:**
```text
╔══════════════════════════════════════╗
║   BAB 39 - Program 1: Boot Counter   ║
╚══════════════════════════════════════╝
  Boot ke: 7

  Coba tekan RST/EN button...
  Angka akan terus bertambah, tidak reset ke 0!
```

> 💡 **Reset NVS:** Jika kamu ingin mengosongkan NVS dan memulai dari nol, panggil `prefs.begin("boot", false)` lalu `prefs.clear()` sebelum `prefs.end()`. Ini menghapus semua key dalam namespace "boot".

---

## 39.5 Program 2: Multi-Type Settings Storage

Program ini menyimpan dan memuat lima jenis data berbeda secara simultan — simulasi nyata *Settings/Configuration Manager* yang digunakan di IoT device produksi.

```cpp
/*
 * BAB 39 - Program 2: Multi-Type Settings Manager
 *
 * Fitur:
 *   ▶ Menyimpan 5 tipe data berbeda ke NVS
 *   ▶ Memuat data saat startup (default jika pertama kali boot)
 *   ▶ Menampilkan status "BARU" vs "DIMUAT DARI NVS"
 *   ▶ Mendemonstrasikan pattern Write-on-Change yang benar
 *
 * Skenario:
 *   Boot pertama   → Nilai default disimpan ke NVS
 *   Boot berikutnya → Nilai dimuat dari NVS (bukan default)
 *
 * Hardware: Hanya ESP32
 */

#include <Preferences.h>

Preferences prefs;

// ── Struktur pengaturan ───────────────────────────────────────────
struct AppSettings {
  uint8_t  brightness;    // LED brightness 0–100%
  float    tempThreshold; // Alarm suhu °C
  bool     alarmEnabled;  // Alarm aktif/nonaktif
  int32_t  servoHome;     // Posisi home servo (derajat)
  String   deviceName;    // Nama perangkat
};

AppSettings cfg;

// Nilai default (digunakan hanya jika belum pernah disimpan)
const uint8_t  DEFAULT_BRIGHT    = 75;
const float    DEFAULT_TEMP      = 35.0f;
const bool     DEFAULT_ALARM     = true;
const int32_t  DEFAULT_SERVO     = 90;
const char*    DEFAULT_NAME      = "Bluino-IoT";

// ── Namespace NVS ─────────────────────────────────────────────────
#define NS_SETTINGS "app_cfg"

void loadSettings() {
  prefs.begin(NS_SETTINGS, true); // read-only

  bool firstBoot = !prefs.isKey("bright"); // Cek apakah data sudah ada

  cfg.brightness    = prefs.getUChar ("bright",   DEFAULT_BRIGHT);
  cfg.tempThreshold = prefs.getFloat ("tempThr",  DEFAULT_TEMP);
  cfg.alarmEnabled  = prefs.getBool  ("alarm",    DEFAULT_ALARM);
  cfg.servoHome     = prefs.getInt   ("servoHom", DEFAULT_SERVO);
  cfg.deviceName    = prefs.getString("devName",  DEFAULT_NAME);

  prefs.end();

  Serial.println(firstBoot ? "[NVS] Pertama boot — memuat nilai DEFAULT"
                           : "[NVS] Data dimuat dari Flash NVS");
}

void saveSettings() {
  prefs.begin(NS_SETTINGS, false); // read-write

  prefs.putUChar ("bright",   cfg.brightness);
  prefs.putFloat ("tempThr",  cfg.tempThreshold);
  prefs.putBool  ("alarm",    cfg.alarmEnabled);
  prefs.putInt   ("servoHom", cfg.servoHome);
  prefs.putString("devName",  cfg.deviceName);

  prefs.end();

  Serial.println("[NVS] Pengaturan disimpan ke Flash!");
}

void printSettings() {
  Serial.println();
  Serial.println("┌─────────────────────────────────────────┐");
  Serial.println("│         Pengaturan Perangkat Saat Ini    │");
  Serial.println("├─────────────────────────────────────────┤");
  Serial.printf ("│  Nama perangkat : %-22s│\n", cfg.deviceName.c_str());
  Serial.printf ("│  Brightness     : %d%%%-20s│\n", cfg.brightness, "");
  Serial.printf ("│  Suhu alarm     : %.1f °C%-17s│\n", cfg.tempThreshold, "");
  Serial.printf ("│  Alarm aktif    : %-22s│\n", cfg.alarmEnabled ? "YA" : "TIDAK");
  Serial.printf ("│  Servo home     : %d°%-21s│\n", cfg.servoHome, "");
  Serial.println("└─────────────────────────────────────────┘");
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(500);

  Serial.println("=== BAB 39 Program 2: Multi-Type Settings ===");

  // Muat pengaturan dari NVS (atau default jika pertama kali)
  loadSettings();
  printSettings();

  // Simulasi: ubah beberapa pengaturan dan simpan ulang
  // (Dalam aplikasi nyata, ini dipicu oleh input user)
  Serial.println("[SIM] Mengubah pengaturan...");
  cfg.brightness    = 50;
  cfg.tempThreshold = 40.0f;
  cfg.alarmEnabled  = false;
  cfg.servoHome     = 45;
  cfg.deviceName    = "Bluino-Lab";

  saveSettings();
  printSettings();

  Serial.println("Restart ESP32 untuk melihat nilai ini dimuat dari NVS!");
}

void loop() {}
```

**Contoh output Serial Monitor:**
```text
=== BAB 39 Program 2: Multi-Type Settings ===
[NVS] Pertama boot — memuat nilai DEFAULT

┌─────────────────────────────────────────┐
│         Pengaturan Perangkat Saat Ini    │
├─────────────────────────────────────────┤
│  Nama perangkat : Bluino-IoT            │
│  Brightness     : 75%                   │
│  Suhu alarm     : 35.0 °C              │
│  Alarm aktif    : YA                    │
│  Servo home     : 90°                   │
└─────────────────────────────────────────┘

[SIM] Mengubah pengaturan...
[NVS] Pengaturan disimpan ke Flash!
...
Restart ESP32 untuk melihat nilai ini dimuat dari NVS!
```

---

## 39.6 Program 3: Settings Manager via Serial Commands

Program ini membangun CLI (Command Line Interface) sederhana lewat Serial Monitor agar pengguna dapat membaca dan mengubah pengaturan secara interaktif. Ini adalah pola yang digunakan di firmware IoT produksi nyata.

```cpp
/*
 * BAB 39 - Program 3: Interactive Settings Manager via Serial CLI
 *
 * Perintah yang tersedia:
 *   get         → Tampilkan semua pengaturan saat ini
 *   set bright <0-100> → Ubah brightness
 *   set temp <nilai>   → Ubah threshold suhu
 *   set alarm <on/off> → Aktif/nonaktifkan alarm
 *   set name <teks>    → Ubah nama perangkat
 *   reset       → Hapus NVS dan kembali ke default
 *   help        → Tampilkan daftar perintah
 *
 * Hardware: Hanya ESP32
 * Buka Serial Monitor dengan Line Ending: "Newline" atau "Both NL & CR"
 */

#include <Preferences.h>

Preferences prefs;

#define NS "cfg"

// ── Struktur pengaturan ───────────────────────────────────────────
uint8_t  cfgBright   = 75;
float    cfgTemp     = 35.0f;
bool     cfgAlarm    = true;
String   cfgName     = "Bluino-IoT";

// ── Write-on-Change tracker ───────────────────────────────────────
bool settingsChanged = false;
unsigned long tLastChange = 0;
const unsigned long SAVE_DEBOUNCE_MS = 2000UL; // Simpan 2 detik setelah perubahan terakhir

void loadSettings() {
  prefs.begin(NS, true);
  cfgBright = prefs.getUChar ("bright",  75);
  cfgTemp   = prefs.getFloat ("temp",    35.0f);
  cfgAlarm  = prefs.getBool  ("alarm",   true);
  cfgName   = prefs.getString("name",    "Bluino-IoT");
  prefs.end();
}

void saveSettings() {
  prefs.begin(NS, false);
  prefs.putUChar ("bright", cfgBright);
  prefs.putFloat ("temp",   cfgTemp);
  prefs.putBool  ("alarm",  cfgAlarm);
  prefs.putString("name",   cfgName);
  prefs.end();
  settingsChanged = false;
  Serial.println("[NVS] ✅ Pengaturan disimpan ke Flash.");
}

void resetSettings() {
  prefs.begin(NS, false);
  prefs.clear();
  prefs.end();
  cfgBright = 75;  cfgTemp = 35.0f;
  cfgAlarm  = true; cfgName = "Bluino-IoT";
  Serial.println("[NVS] ⚠️  NVS dihapus. Nilai kembali ke default.");
}

void printSettings() {
  Serial.println();
  Serial.println("┌──────────────────────────────────────┐");
  Serial.println("│         Pengaturan Saat Ini           │");
  Serial.println("├──────────────────────────────────────┤");
  Serial.printf ("│  brightness : %d%%\n", cfgBright);
  Serial.printf ("│  temp_alarm : %.1f °C\n", cfgTemp);
  Serial.printf ("│  alarm      : %s\n", cfgAlarm ? "ON" : "OFF");
  Serial.printf ("│  name       : %s\n", cfgName.c_str());
  Serial.println("└──────────────────────────────────────┘");
  Serial.println();
}

void printHelp() {
  Serial.println();
  Serial.println("=== Perintah CLI NVS ===");
  Serial.println("  get                → Tampilkan semua pengaturan");
  Serial.println("  set bright <0-100> → Ubah brightness");
  Serial.println("  set temp <nilai>   → Ubah threshold suhu (°C)");
  Serial.println("  set alarm <on/off> → Aktif/nonaktifkan alarm");
  Serial.println("  set name <teks>    → Ubah nama (maks 15 karakter)");
  Serial.println("  reset              → Hapus NVS dan kembali ke default");
  Serial.println("  help               → Tampilkan panduan ini");
  Serial.println();
}

void handleCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  Serial.printf("> %s\n", cmd.c_str());

  if (cmd == "get") {
    printSettings();

  } else if (cmd.startsWith("set bright ")) {
    int val = cmd.substring(11).toInt();
    if (val < 0 || val > 100) {
      Serial.println("[ERR] Brightness harus antara 0–100");
      return;
    }
    cfgBright = (uint8_t)val;
    settingsChanged = true;
    tLastChange = millis();
    Serial.printf("[OK] brightness = %d%%\n", cfgBright);

  } else if (cmd.startsWith("set temp ")) {
    float val = cmd.substring(9).toFloat();
    if (val < -40.0f || val > 125.0f) {
      Serial.println("[ERR] Suhu harus antara -40 s/d 125 °C");
      return;
    }
    cfgTemp = val;
    settingsChanged = true;
    tLastChange = millis();
    Serial.printf("[OK] temp_alarm = %.1f °C\n", cfgTemp);

  } else if (cmd.startsWith("set alarm ")) {
    String val = cmd.substring(10);
    val.trim(); val.toLowerCase();
    if (val == "on")       { cfgAlarm = true;  }
    else if (val == "off") { cfgAlarm = false; }
    else { Serial.println("[ERR] Gunakan 'on' atau 'off'"); return; }
    settingsChanged = true;
    tLastChange = millis();
    Serial.printf("[OK] alarm = %s\n", cfgAlarm ? "ON" : "OFF");

  } else if (cmd.startsWith("set name ")) {
    String val = cmd.substring(9);
    val.trim();
    if (val.length() == 0 || val.length() > 15) {
      Serial.println("[ERR] Nama harus 1–15 karakter");
      return;
    }
    cfgName = val;
    settingsChanged = true;
    tLastChange = millis();
    Serial.printf("[OK] name = %s\n", cfgName.c_str());

  } else if (cmd == "reset") {
    resetSettings();

  } else if (cmd == "help") {
    printHelp();

  } else {
    Serial.printf("[ERR] Perintah tidak dikenal: '%s'. Ketik 'help'.\n", cmd.c_str());
  }
}

String serialBuf = "";

void setup() {
  Serial.begin(115200);
  delay(500);
  loadSettings();

  Serial.println("=== BAB 39 Program 3: NVS Serial CLI ===");
  Serial.println("Ketik 'help' untuk melihat daftar perintah.");
  Serial.println("Pengaturan disimpan otomatis 2 detik setelah perubahan.");
  printSettings();
}

void loop() {
  unsigned long now = millis();

  // ── Baca input Serial ─────────────────────────────────────────
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialBuf.length() > 0) {
        handleCommand(serialBuf);
        serialBuf = "";
      }
    } else {
      if (serialBuf.length() < 64) serialBuf += c;
    }
  }

  // ── Auto-save dengan debounce (Write-on-Change pattern) ──────
  // Hanya menyimpan ke Flash jika ada perubahan DAN sudah 2 detik berlalu
  // Ini PENTING: jangan simpan ke Flash di setiap loop() iterasi!
  if (settingsChanged && (now - tLastChange >= SAVE_DEBOUNCE_MS)) {
    saveSettings();
  }
}
```

> 💡 **Mengapa Write-on-Change + Debounce 2 Detik?**
> Halaman Flash NVS memiliki endurance terbatas (~10.000 siklus tulis). Jika kamu menulis ke NVS di setiap iterasi `loop()` (yang bisa berjalan 10.000+ kali per detik), kamu dapat **merusak Flash dalam hitungan menit**! Pattern yang benar:
> 1. Perubahan hanya ditandai (`settingsChanged = true`)
> 2. Penulisan ke Flash terjadi sekali saja, setelah 2 detik tidak ada perubahan baru

---

## 39.7 Program 4: FSM dengan Persistent State

Program puncak BAB 39: sebuah FSM yang **mengingat mode dan konfigurasi terakhirnya** setelah ESP32 mati dan menyala kembali. Ini adalah dasar dari semua IoT device yang "cerdas" — seperti smart thermostat yang mengingat setpoint terakhirnya.

```cpp
/*
 * BAB 39 - Program 4: FSM dengan Persistent State (Memori Lintas Reboot)
 *
 * Fitur:
 *   ▶ 3 mode FSM: IDLE, MONITOR, ALERT
 *   ▶ Mode terakhir disimpan ke NVS dan dimuat saat boot
 *   ▶ Threshold suhu tersimpan di NVS, dikonfigurasi via Serial
 *   ▶ Counter notifikasi persisten (berapa kali alert dipicu total)
 *   ▶ Timestamp boot terakhir (simulasi, pakai bootCount)
 *   ▶ Tombol IO32: short-press = ganti mode, long-press = reset counter
 *
 * Hardware:
 *   BTN_MODE → IO32 (pull-down 10KΩ bawaan kit, aktif HIGH)
 */

#include <Preferences.h>

Preferences prefs;
#define NS_FSM "fsm_state"

// ── Definisi FSM ─────────────────────────────────────────────────
enum AppMode { MODE_IDLE = 0, MODE_MONITOR = 1, MODE_ALERT = 2 };
const char* modeName[] = { "IDLE", "MONITOR", "ALERT" };
AppMode currentMode;

// ── Persistent State ─────────────────────────────────────────────
float    pTempThreshold;
uint32_t pAlertCount;
uint32_t pBootCount;

// ── Hardware ──────────────────────────────────────────────────────
#define BTN_PIN 32
const unsigned long DEBOUNCE_MS  = 50UL;
const unsigned long LONGPRESS_MS = 1500UL;

struct Button {
  bool physical, stable, shortPress, longPress, longFired;
  unsigned long tEdge, tDown;
};
Button btn = { false, false, false, false, false, 0, 0 };

void updateBtn(unsigned long now) {
  btn.shortPress = btn.longPress = false;
  bool rd = (digitalRead(BTN_PIN) == HIGH);
  if (rd != btn.physical) btn.tEdge = now;
  if ((now - btn.tEdge) >= DEBOUNCE_MS) {
    if (rd != btn.stable) {
      btn.stable = rd;
      if (btn.stable) { btn.tDown = now; btn.longFired = false; }
      else {
        unsigned long dur = now - btn.tDown;
        if (!btn.longFired && dur >= DEBOUNCE_MS && dur < LONGPRESS_MS)
          btn.shortPress = true;
      }
    }
  }
  if (btn.stable && !btn.longFired && (now - btn.tDown) >= LONGPRESS_MS) {
    btn.longPress = btn.longFired = true;
  }
  btn.physical = rd;
}

// ── NVS Helpers ───────────────────────────────────────────────────
void loadPersistentState() {
  prefs.begin(NS_FSM, true);
  currentMode    = (AppMode)prefs.getInt  ("mode",      MODE_IDLE);
  pTempThreshold = prefs.getFloat("tempThr",  35.0f);
  pAlertCount    = prefs.getUInt ("alertCnt", 0);
  pBootCount     = prefs.getUInt ("bootCnt",  0);
  bool firstBoot = !prefs.isKey("mode");
  prefs.end();

  if (firstBoot) {
    Serial.println("[NVS] Pertama kali boot — menggunakan default state.");
  } else {
    Serial.printf("[NVS] State dimuat: mode=%s, threshold=%.1f°C, alert=%u kali\n",
      modeName[currentMode], pTempThreshold, pAlertCount);
  }
}

void savePersistentState() {
  prefs.begin(NS_FSM, false);
  prefs.putInt  ("mode",     (int)currentMode);
  prefs.putFloat("tempThr",  pTempThreshold);
  prefs.putUInt ("alertCnt", pAlertCount);
  prefs.putUInt ("bootCnt",  pBootCount);
  prefs.end();
}

// ── Transisi Mode ─────────────────────────────────────────────────
void switchMode(AppMode newMode) {
  currentMode = newMode;
  savePersistentState(); // Simpan segera saat mode berubah
  Serial.printf("\n[FSM] Mode → %s (tersimpan ke NVS)\n", modeName[currentMode]);
}

// ── Simulasi Sensor Suhu (tanpa sensor fisik) ─────────────────────
float simulateTemp(unsigned long now) {
  // Suhu berfluktuasi antara 30–42°C dalam siklus sinusoidal simulasi
  return 36.0f + 6.0f * sinf(now / 8000.0f);
}

// ── Ticker Display ────────────────────────────────────────────────
unsigned long tDisplay = 0;

void printStatus(float temp) {
  Serial.printf("[%s] Suhu: %.1f°C | Threshold: %.1f°C | Alert: %u kali | Boot: %u\n",
    modeName[currentMode], temp, pTempThreshold, pAlertCount, pBootCount);
}

// ── Serial Command Handler ─────────────────────────────────────────
String serialBuf = "";

void handleCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  if (cmd.startsWith("thr ")) {
    float v = cmd.substring(4).toFloat();
    if (v >= 20.0f && v <= 60.0f) {
      pTempThreshold = v;
      savePersistentState();
      Serial.printf("[CFG] Threshold diubah → %.1f°C (disimpan)\n", v);
    } else {
      Serial.println("[ERR] Threshold harus 20–60°C");
    }
  } else if (cmd == "info") {
    Serial.printf("[INFO] Mode: %s | Thr: %.1f | Alert: %u | Boot: %u\n",
      modeName[currentMode], pTempThreshold, pAlertCount, pBootCount);
  } else {
    Serial.println("[ERR] Perintah: 'thr <nilai>' atau 'info'");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(BTN_PIN, INPUT);
  delay(500);

  Serial.println("=== BAB 39 Program 4: FSM Persistent State ===");
  Serial.println("Short-press IO32: Ganti mode FSM");
  Serial.println("Long-press  IO32: Reset alert counter");
  Serial.println("Ketik 'thr <nilai>' untuk ubah threshold suhu");
  Serial.println("----------------------------------------------");

  loadPersistentState();
  pBootCount++;
  savePersistentState();

  Serial.printf("[BOOT] Boot ke-%u | Mode dimuat: %s\n", pBootCount, modeName[currentMode]);
}

void loop() {
  unsigned long now = millis();

  updateBtn(now);
  float temp = simulateTemp(now);

  // ── Input: Short-press = ganti mode ──────────────────────────
  if (btn.shortPress) {
    switchMode((AppMode)((currentMode + 1) % 3));
  }

  // ── Input: Long-press = reset alert counter ───────────────────
  if (btn.longPress) {
    pAlertCount = 0;
    savePersistentState();
    Serial.println("[RST] Alert counter di-reset ke 0 (disimpan)");
  }

  // ── Serial CLI ────────────────────────────────────────────────
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (serialBuf.length() > 0) {
        handleCommand(serialBuf);
        serialBuf = "";
      }
    } else {
      if (serialBuf.length() < 64) serialBuf += c;
    }
  }

  // ── Logika FSM ────────────────────────────────────────────────
  switch (currentMode) {
    case MODE_IDLE:
      // Di mode IDLE: tidak ada pemantauan, hanya menunggu tombol
      break;

    case MODE_MONITOR:
      // Pantau suhu, picu ALERT jika melebihi threshold
      if (temp > pTempThreshold) {
        pAlertCount++;
        savePersistentState(); // Simpan counter yang diperbarui
        switchMode(MODE_ALERT);
      }
      break;

    case MODE_ALERT:
      // Di mode ALERT: tampilkan warning visual
      // Kembali ke MONITOR jika suhu sudah turun
      if (temp <= pTempThreshold - 2.0f) { // Hysteresis 2°C
        switchMode(MODE_MONITOR);
      }
      break;
  }

  // ── Display setiap 1 detik ────────────────────────────────────
  if (now - tDisplay >= 1000UL) {
    tDisplay = now;
    if (currentMode != MODE_IDLE) {
      printStatus(temp);
    }
    if (currentMode == MODE_ALERT) {
      Serial.println("  ⚠️  SUHU MELEBIHI THRESHOLD!");
    }
  }
}
```

**Contoh output Serial Monitor:**
```text
=== BAB 39 Program 4: FSM Persistent State ===
Short-press IO32: Ganti mode FSM
Long-press  IO32: Reset alert counter
Ketik 'thr <nilai>' untuk ubah threshold suhu
----------------------------------------------
[NVS] State dimuat: mode=MONITOR, threshold=35.0°C, alert=3 kali
[BOOT] Boot ke-8 | Mode dimuat: MONITOR

[MONITOR] Suhu: 33.2°C | Threshold: 35.0°C | Alert: 3 kali | Boot: 8
[MONITOR] Suhu: 36.7°C | Threshold: 35.0°C | Alert: 3 kali | Boot: 8

[FSM] Mode → ALERT (tersimpan ke NVS)
  ⚠️  SUHU MELEBIHI THRESHOLD!
[ALERT] Suhu: 38.1°C | Threshold: 35.0°C | Alert: 4 kali | Boot: 8
```

> 💡 **Hysteresis pada threshold:** Perhatikan kondisi `temp <= pTempThreshold - 2.0f` saat keluar dari `MODE_ALERT`. Ini adalah teknik **hysteresis** — mode ALERT baru berakhir jika suhu turun 2°C di bawah threshold, bukan tepat di threshold. Tanpa hysteresis, FSM akan bolak-balik antara MONITOR dan ALERT dengan cepat (*oscillating*) saat suhu berada tepat di batas, menyebabkan penulisan NVS yang berlebihan.

---

## 39.8 Tips & Panduan Troubleshooting

### 1. `prefs.begin()` mengembalikan `false` atau gagal?
```text
Penyebab paling umum:

A. Namespace sudah dibuka oleh kode lain dan belum di-end()
   → Pastikan setiap prefs.begin() diikuti prefs.end()
   → Gunakan satu instance Preferences per namespace secara bergiliran

B. Partisi NVS rusak (jarang terjadi)
   → Gunakan alat "Erase Flash" di Arduino IDE saat upload:
     Tools → Erase All Flash Before Sketch Upload → Enable

C. Namespace atau key melebihi 15 karakter
   → Hitung karakter! "my_long_namespace" = 18 karakter → TIDAK VALID
```

### 2. Nilai yang dibaca tidak sesuai yang disimpan?
```text
Penyebab:
A. Tipe data tidak cocok saat baca vs tulis
   → putInt() HARUS dibaca dengan getInt(), bukan getUInt() atau getFloat()
   → Tipe yang tidak cocok mengembalikan nilai default, bukan error

B. Namespace yang salah digunakan
   → Pastikan string namespace identik saat write dan read

C. Key berubah (typo atau versi berbeda)
   → prefs.isKey("keyname") untuk verifikasi key ada sebelum membaca
```

### 3. ESP32 semakin lambat / NVS penuh?
```text
Cek sisa kapasitas:
  prefs.begin("namespace", false);
  Serial.println(prefs.freeEntries());
  prefs.end();

NVS default menyimpan ~97 entri untuk partisi 4KB.
Jika mendekati penuh:
  → Gunakan putBytes() untuk menyimpan struct sebagai satu entri
  → Hapus key yang tidak diperlukan dengan prefs.remove("key")
  → Pertimbangkan SPIFFS/LittleFS untuk data besar (file konfigurasi JSON)
```

### 4. Kapan sebaiknya NOT menggunakan NVS?
```text
❌ JANGAN gunakan NVS untuk:
  → Data yang berubah sangat sering (> ratusan kali per jam per key)
    karena Flash memiliki endurance terbatas
  → Data besar (> beberapa KB) → gunakan SPIFFS/LittleFS
  → Log data sensorik berkecepatan tinggi → gunakan SD Card
  → Data sementara antar loop() → gunakan variabel RAM biasa

✅ GUNAKAN NVS untuk:
  → Konfigurasi device (threshold, nama, interval)
  → Mode atau state FSM terakhir
  → Credential WiFi (SSID/Password)
  → Counter/statistik yang perlu persisten
  → Kalibrasi sensor yang disimpan di lapangan
```

---

## 39.9 Ringkasan

```
┌─────────────────────────────────────────────────────────────────────┐
│                   RINGKASAN BAB 39 — NVS ESP32                      │
├─────────────────────────────────────────────────────────────────────┤
│ KONSEP KUNCI:                                                         │
│   NVS = Non-Volatile Storage di Flash internal ESP32                │
│   Key-Value store terorganisir dalam Namespace (folder logika)       │
│   Wear-leveling otomatis → distribusi tulis merata di Flash         │
│   Data bertahan selama power-off, reset, dan update firmware*       │
│   (*tergantung konfigurasi OTA / partition scheme)                  │
│                                                                       │
│ API PREFERENCES.H:                                                   │
│   prefs.begin("ns", rdOnly)  → Buka namespace                      │
│   prefs.put<Type>("key", v)  → Tulis nilai                         │
│   prefs.get<Type>("key", def)→ Baca nilai (+ default)              │
│   prefs.isKey("key")         → Cek keberadaan key                  │
│   prefs.remove("key")        → Hapus satu key                      │
│   prefs.clear()              → Hapus semua key di namespace         │
│   prefs.end()                → WAJIB dipanggil setelah selesai!    │
│                                                                       │
│ BEST PRACTICES:                                                       │
│   ✅ Write-on-Change: Hanya tulis saat nilai benar-benar berubah    │
│   ✅ Debounce Save: Tunda 1-2 detik setelah perubahan terakhir      │
│   ✅ Selalu panggil prefs.end() — jangan lupa!                      │
│   ✅ Gunakan prefs.isKey() untuk deteksi first-boot vs reload       │
│   ✅ Batasi nama namespace & key ≤ 15 karakter                      │
│                                                                       │
│ ANTIPATTERN (JANGAN):                                                 │
│   ❌ Tulis ke NVS di dalam loop() tanpa kondisi / debounce          │
│   ❌ Baca NVS dengan tipe berbeda dari tipe saat menulis            │
│   ❌ Gunakan NVS untuk data besar (> 4KB) — pakai LittleFS/SD      │
└─────────────────────────────────────────────────────────────────────┘
```

---

## 39.10 Latihan

1. **WiFi Credential Manager:** Simpan SSID dan password WiFi ke NVS. Saat boot, coba koneksi dengan credential tersimpan. Jika gagal dalam 10 detik, masuk ke mode AP (Access Point) untuk konfigurasi ulang. Simpan credential baru yang diterima via AP ke NVS. (Petunjuk: Integrasi dengan BAB WiFi).

2. **Odometer Servo:** Integrasikan NVS dengan Program 3 BAB 38 (Preset Servo). Setiap kali servo bergerak, hitung total derajat pergerakan kumulatif dan simpan ke NVS setiap 100 derajat. Tampilkan "odometer" ini saat boot. Reset via long-press.

3. **Struct Serialization:** Buat `struct SensorCalibration { float offset; float scale; uint8_t channel; }` dan simpan seluruh struct ke NVS menggunakan `prefs.putBytes()` sebagai satu entri tunggal. Muat kembali saat boot menggunakan `prefs.getBytes()`. Verifikasi integritas data dengan mencetak semua field.

4. **NVS Version Migration:** Buat program yang menyimpan versi skema NVS sebagai integer (misalnya `cfgVersion = 1`). Saat boot, baca versi tersimpan. Jika berbeda dari versi program aktif, jalankan fungsi migrasi data (misalnya menambahkan key baru dengan nilai default). Ini adalah teknik firmware upgrade yang digunakan di device IoT produksi.

5. **Multi-Namespace Logger:** Buat sistem logging sederhana dengan 3 namespace berbeda: `"daily"` (reset setiap 24 jam simulasi), `"weekly"` (reset setiap 7 hari simulasi), dan `"total"` (tidak pernah direset). Setiap namespace menyimpan nilai `maxTemp`, `minTemp`, dan `avgTemp` yang diperbarui menggunakan sensor DHT11 (IO27). Tampilkan semua laporan saat Serial `"report"` dikirim.

---

## ☕ Dukung Proyek Ini

Bab ini disusun dengan dedikasi waktu dan keahlian yang tidak sedikit. Jika materi *engineering-grade* ini bermanfaat, bentuk apresiasi terbaik adalah secangkir kopi untuk penulis:

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

---

*Sebelumnya → [BAB 38: Servo Motor](../BAB-38/README.md)*

*Selanjutnya → [BAB 40: SPIFFS / LittleFS — Sistem File di Flash](../BAB-40/README.md)*
