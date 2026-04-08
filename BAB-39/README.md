# BAB 39: NVS â€” Memori Permanen ESP32

> âœ… **Prasyarat:** Selesaikan **BAB 05 (Serial Monitor)**, **BAB 09 (Digital Input â€” Push Button)**, dan **BAB 16 (Non-Blocking Programming)**. Tidak ada komponen tambahan yang diperlukan untuk BAB ini â€” semua program berjalan hanya dengan ESP32 dan Serial Monitor.

> **Glosarium Singkatan BAB ini:**
>
> | Singkatan | Kepanjangan |
> |-----------|--------------|
> | **NVS** | *Non-Volatile Storage* — penyimpanan di Flash yang tidak hilang saat mati listrik |
> | **API** | *Application Programming Interface* — antarmuka fungsi/library yang bisa dipanggil programmer |
> | **FSM** | *Finite State Machine* — mesin kondisi berhingga: sistem yang bergerak antar *state* terdefinisi |
> | **CLI** | *Command Line Interface* — antarmuka berbasis teks tempat pengguna mengetik perintah |
> | **SSID** | *Service Set Identifier* — nama jaringan WiFi yang terlihat di daftar WiFi perangkat |
> | **OTA** | *Over-The-Air* — mekanisme update firmware tanpa kabel, melalui jaringan WiFi |
> | **CRC** | *Cyclic Redundancy Check* — algoritma checksum untuk mendeteksi kerusakan data |
> | **EEPROM** | *Electrically Erasable Programmable Read-Only Memory* — chip penyimpanan non-volatile pada Arduino AVR |
> | **AVR** | Keluarga mikrokontroler Atmel/Microchip (Arduino Uno/Nano); berbeda dengan ESP32 yang pakai arsitektur Xtensa LX6 |
> | **SRAM** | *Static Random-Access Memory* — RAM internal chip, cepat namun hilang saat mati listrik |

---

## ðŸ“Œ Tujuan Pembelajaran

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
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚  RAM (SRAM) â€” 520 KB                                            â”‚
â”‚  â”œâ”€ Semua variabel C++ (int, float, String...)                  â”‚
â”‚  â”œâ”€ Stack program, heap malloc                                   â”‚
â”‚  â””â”€ âš ï¸ HILANG TOTAL saat reset atau mati listrik!              â”‚
â”œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¤
â”‚  Flash â€” 4 MB (umumnya)                                         â”‚
â”‚  â”œâ”€ [0x000000] Bootloader                                        â”‚
â”‚  â”œâ”€ [0x001000] Partition Table                                   â”‚
â”‚  â”œâ”€ [0x008000] Program kode (.ino / .bin) yang kamu upload      â”‚
â”‚  â”œâ”€ [0x...   ] NVS Partition (~20 KB default)  â† KITA PAKAI    â”‚
â”‚  â””â”€ [0x...   ] SPIFFS / LittleFS (opsional)                    â”‚
â”‚  âœ… TIDAK HILANG saat reset â€” tersimpan permanen               â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
```

### EEPROM vs NVS â€” Mengapa Pilih NVS?

| Aspek | EEPROM.h (Arduino AVR) | NVS / Preferences.h (ESP32) |
|-------|------------------------|------------------------------|
| Media | Chip EEPROM terpisah | Flash internal ESP32 |
| Akses | Per-byte (tulis manual per alamat) | Key-Value (nama sebagai kunci) |
| Wear Leveling | âŒ Tidak ada | âœ… Otomatis (memperpanjang umur Flash) |
| Tipe Data | Byte manual, casting manual | Native: int, float, String, bool, blob |
| Namespace | âŒ Tidak ada (flat) | âœ… Ada (isolasi antar fitur) |
| Endurance | ~100.000 siklus tulis | ~10.000 siklus per halaman (wear-leveled) |
| Keamanan | Tidak ada | Checksum **CRC** *(Cyclic Redundancy Check)* bawaan |

> **Catatan Singkatan Tabel:**
> - **EEPROM** *(Electrically Erasable Programmable Read-Only Memory)*: chip penyimpanan non-volatile pada papan Arduino berbasis **AVR** (Uno, Nano, Mega). ESP32 tidak menggunakan chip EEPROM terpisah — ia memakai Flash internal.
> - **AVR**: keluarga mikrokontroler Atmel/Microchip. ESP32 bukan AVR — ia menggunakan arsitektur **Xtensa LX6** dual-core 240 MHz.
> - **CRC** *(Cyclic Redundancy Check)*: algoritma checksum matematis yang mendeteksi apakah data tersimpan secara utuh atau mengalami kerusakan bit.
>
> ðŸ’¡ **Wear Leveling** = NVS tidak menulis ulang ke halaman Flash yang sama setiap kali. Ia mendistribusikan tulisan ke seluruh area NVS secara rotasi, sehingga umur efektif penyimpanan jauh lebih panjang.

---

## 39.2 Arsitektur NVS â€” Namespace dan Key-Value

NVS terorganisir dalam **namespace** (seperti folder) yang berisi pasangan **key â†’ value**.

```
STRUKTUR NVS (Ilustrasi):

  NVS Flash Partition
  â”œâ”€â”€ Namespace: "settings"
  â”‚     â”œâ”€â”€ "brightness"  : uint8  = 75
  â”‚     â”œâ”€â”€ "threshold"   : float  = 32.5
  â”‚     â””â”€â”€ "deviceName"  : String = "Bluino-001"
  â”‚
  â”œâ”€â”€ Namespace: "fsm"
  â”‚     â”œâ”€â”€ "lastMode"    : int32  = 2
  â”‚     â””â”€â”€ "servoPos"    : int32  = 135
  â”‚
  â””â”€â”€ Namespace: "boot"
        â””â”€â”€ "bootCount"   : uint32 = 47

Aturan:
  â–¶ Nama namespace : maks 15 karakter
  â–¶ Nama key       : maks 15 karakter
  â–¶ Satu value per key (overwrite jika key sudah ada)
  â–¶ Namespace dibuat otomatis saat pertama kali digunakan
```

---

## 39.3 API Library Preferences.h â€” Referensi Lengkap

> 📌 **API** *(Application Programming Interface)* adalah sekumpulan fungsi dan aturan yang disediakan oleh sebuah library agar dapat digunakan programmer. "API Preferences.h" berarti daftar lengkap fungsi yang bisa kamu panggil untuk berinteraksi dengan NVS — mulai dari membuka namespace, membaca, menulis, hingga menghapus data.

```cpp
#include <Preferences.h>
Preferences prefs;

// â”€â”€ Membuka / Menutup â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
prefs.begin("namespace", false);  // false = read-write mode
prefs.begin("namespace", true);   // true  = read-only mode (lebih aman)
prefs.end();                      // WAJIB dipanggil setelah selesai!

// â”€â”€ Menulis Data â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
prefs.putUChar  ("key", value);   // uint8_t  (0â€“255)
prefs.putChar   ("key", value);   // int8_t   (-128 â€“ 127)
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

// â”€â”€ Membaca Data (dengan default value jika key belum ada) â”€â”€â”€â”€â”€â”€â”€
uint8_t  v = prefs.getUChar  ("key", 0);
int32_t  v = prefs.getInt    ("key", 0);
float    v = prefs.getFloat  ("key", 0.0f);
bool     v = prefs.getBool   ("key", false);
String   v = prefs.getString ("key", "default");
size_t   n = prefs.getBytes  ("key", buf, maxLen);

// â”€â”€ Utilitas â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
bool exists = prefs.isKey("key");          // Cek apakah key ada
prefs.remove("key");                       // Hapus satu key
prefs.clear();                             // Hapus SEMUA key di namespace
size_t free = prefs.freeEntries();         // Sisa entri yang tersedia
```

> âš ï¸ **WAJIB:** Selalu panggil `prefs.end()` setelah selesai read/write. Lupa memanggil `end()` dapat menyebabkan NVS handle tidak dilepas dan program berikutnya gagal membuka namespace yang sama!

---

## 39.4 Program 1: Boot Counter â€” Hello World NVS

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

  // Tutup â€” WAJIB!
  prefs.end();

  // Tampilkan hasil
  Serial.println("â•”â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•—");
  Serial.println("â•‘   BAB 39 - Program 1: Boot Counter   â•‘");
  Serial.println("â•šâ•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•");
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
â•”â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•—
â•‘   BAB 39 - Program 1: Boot Counter   â•‘
â•šâ•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•â•
  Boot ke: 7

  Coba tekan RST/EN button...
  Angka akan terus bertambah, tidak reset ke 0!
```

> ðŸ’¡ **Reset NVS:** Jika kamu ingin mengosongkan NVS dan memulai dari nol, panggil `prefs.begin("boot", false)` lalu `prefs.clear()` sebelum `prefs.end()`. Ini menghapus semua key dalam namespace "boot".

---

## 39.5 Program 2: Multi-Type Settings Storage

Program ini menyimpan dan memuat lima jenis data berbeda secara simultan â€” simulasi nyata *Settings/Configuration Manager* yang digunakan di IoT device produksi.

```cpp
/*
 * BAB 39 - Program 2: Multi-Type Settings Manager
 *
 * Fitur:
 *   â–¶ Menyimpan 5 tipe data berbeda ke NVS
 *   â–¶ Memuat data saat startup (default jika pertama kali boot)
 *   â–¶ Menampilkan status "BARU" vs "DIMUAT DARI NVS"
 *   â–¶ Mendemonstrasikan pattern Write-on-Change yang benar
 *
 * Skenario:
 *   Boot pertama   â†’ Nilai default disimpan ke NVS
 *   Boot berikutnya â†’ Nilai dimuat dari NVS (bukan default)
 *
 * Hardware: Hanya ESP32
 */

#include <Preferences.h>

Preferences prefs;

// â”€â”€ Struktur pengaturan â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
struct AppSettings {
  uint8_t  brightness;    // LED brightness 0â€“100%
  float    tempThreshold; // Alarm suhu Â°C
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

// â”€â”€ Namespace NVS â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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

  Serial.println(firstBoot ? "[NVS] Pertama boot â€” memuat nilai DEFAULT"
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
  Serial.println("â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”");
  Serial.println("â”‚         Pengaturan Perangkat Saat Ini    â”‚");
  Serial.println("â”œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¤");
  Serial.printf ("â”‚  Nama perangkat : %-22sâ”‚\n", cfg.deviceName.c_str());
  Serial.printf ("â”‚  Brightness     : %d%%%-20sâ”‚\n", cfg.brightness, "");
  Serial.printf ("â”‚  Suhu alarm     : %.1f Â°C%-17sâ”‚\n", cfg.tempThreshold, "");
  Serial.printf ("â”‚  Alarm aktif    : %-22sâ”‚\n", cfg.alarmEnabled ? "YA" : "TIDAK");
  Serial.printf ("â”‚  Servo home     : %dÂ°%-21sâ”‚\n", cfg.servoHome, "");
  Serial.println("â”```cpp
/*
 * BAB 39 - Program 3: Interactive Settings Manager via Serial CLI
 *   [CLI = Command Line Interface: antarmuka berbasis teks di mana
 *    pengguna mengetik perintah seperti "set bright 80" atau "reset"]
 *
 * Perintah yang tersedia:
 *   get         â†’ Tampilkan semua pengaturan saat ini
 *   set bright <0-100> â†’ Ubah brightness
 *   set temp <nilai>   â†’ Ubah threshold suhu
 *   set alarm <on/off> â†’ Aktif/nonaktifkan alarm
 *   set name <teks>    â†’ Ubah nama perangkat
 *   reset       â†’ Hapus NVS dan kembali ke default
 *   help        â†’ Tampilkan daftar perintah
 *
 * Hardware: Hanya ESP32
 * Buka Serial Monitor dengan Line Ending: "Newline" atau "Both NL & CR"
 */

#include <Preferences.h>
#include <string.h>
#include <stdlib.h>

Preferences prefs;

#define NS "cfg"

// â”€â”€ Struktur pengaturan â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
uint8_t  cfgBright   = 75;
float    cfgTemp     = 35.0f;
bool     cfgAlarm    = true;
char     cfgName[16] = "Bluino-IoT"; // Maks 15 karakter + null terminator

// â”€â”€ Write-on-Change tracker (Debounce NVS Wear-Leveling) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
bool settingsChanged = false;
unsigned long tLastChange = 0;
const unsigned long SAVE_DEBOUNCE_MS = 2000UL;

void loadSettings() {
  prefs.begin(NS, true);
  cfgBright = prefs.getUChar ("bright",  75);
  cfgTemp   = prefs.getFloat ("temp",    35.0f);
  cfgAlarm  = prefs.getBool  ("alarm",   true);
  
  String nameLoaded = prefs.getString("name", "Bluino-IoT");
  strncpy(cfgName, nameLoaded.c_str(), sizeof(cfgName) - 1);
  cfgName[sizeof(cfgName) - 1] = '\0';
  
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
  Serial.println("[NVS] âœ… Pengaturan disimpan ke Flash.");
}

void resetSettings() {
  prefs.begin(NS, false);
  prefs.clear();
  prefs.end();
  
  cfgBright = 75;
  cfgTemp   = 35.0f;
  cfgAlarm  = true;
  strncpy(cfgName, "Bluino-IoT", sizeof(cfgName));
  
  Serial.println("[NVS] âš ï¸  NVS dihapus. Nilai kembali ke default.");
}

void printSettings() {
  Serial.println();
  Serial.println("â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”");
  Serial.println("â”‚         Pengaturan Saat Ini           â”‚");
  Serial.println("â”œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¤");
  Serial.printf ("â”‚  brightness : %d%%\n",             cfgBright);
  Serial.printf ("â”‚  temp_alarm : %.1f Â°C\n",           cfgTemp);
  Serial.printf ("â”‚  alarm      : %s\n",               cfgAlarm ? "ON" : "OFF");
  Serial.printf ("â”‚  name       : %s\n",               cfgName);
  Serial.println("â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜");
  Serial.println();
}

void printHelp() {
  Serial.println();
  Serial.println("=== Perintah CLI NVS ===");
  Serial.println("  get                â†’ Tampilkan semua pengaturan");
  Serial.println("  set bright <0-100> â†’ Ubah brightness");
  Serial.println("  set temp <nilai>   â†’ Ubah threshold suhu (Â°C)");
  Serial.println("  set alarm <on/off> â†’ Aktif/nonaktifkan alarm");
  Serial.println("  set name <teks>    â†’ Ubah nama (maks 15 karakter)");
  Serial.println("  reset              â†’ Hapus NVS dan kembali ke default");
  Serial.println("  help               â†’ Tampilkan panduan ini");
  Serial.println();
}

// â”€â”€ Parser Zero-Heap (Berbasis pointer C-String murni) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
void handleCommand(char* cmd) {
  // Hapus spasi di akhir jika ada (trim right)
  int len = strlen(cmd);
  while (len > 0 && (cmd[len - 1] == ' ' || cmd[len - 1] == '\r')) {
    cmd[len - 1] = '\0';
    len--;
  }
  
  if (len == 0) return;
  Serial.printf("> %s\n", cmd);

  if (strcmp(cmd, "get") == 0) {
    printSettings();

  } else if (strncmp(cmd, "set bright ", 11) == 0) {
    int val = atoi(cmd + 11);
    if (val < 0 || val > 100) {
      Serial.println("[ERR] Brightness harus antara 0â€“100");
      return;
    }
    cfgBright = (uint8_t)val;
    settingsChanged = true;
    tLastChange = millis();
    Serial.printf("[OK] brightness = %d%%\n", cfgBright);

  } else if (strncmp(cmd, "set temp ", 9) == 0) {
    float val = atof(cmd + 9);
    if (val < -40.0f || val > 125.0f) {
      Serial.println("[ERR] Suhu harus antara -40 s/d 125 Â°C");
      return;
    }
    cfgTemp = val;
    settingsChanged = true;
    tLastChange = millis();
    Serial.printf("[OK] temp_alarm = %.1f Â°C\n", cfgTemp);

  } else if (strncmp(cmd, "set alarm ", 10) == 0) {
    char* val = cmd + 10;
    if (strcasecmp(val, "on") == 0) {
      cfgAlarm = true;
    } else if (strcasecmp(val, "off") == 0) {
      cfgAlarm = false;
    } else {
      Serial.println("[ERR] Gunakan 'on' atau 'off'");
      return;
    }
    settingsChanged = true;
    tLastChange = millis();
    Serial.printf("[OK] alarm = %s\n", cfgAlarm ? "ON" : "OFF");

  } else if (strncmp(cmd, "set name ", 9) == 0) {
    char* val = cmd + 9;
    if (strlen(val) == 0 || strlen(val) > 15) {
      Serial.println("[ERR] Nama harus 1â€“15 karakter");
      return;
    }
    strncpy(cfgName, val, sizeof(cfgName) - 1);
    cfgName[sizeof(cfgName) - 1] = '\0';
    settingsChanged = true;
    tLastChange = millis();
    Serial.printf("[OK] name = %s\n", cfgName);

  } else if (strcmp(cmd, "reset") == 0) {
    resetSettings();

  } else if (strcmp(cmd, "help") == 0) {
    printHelp();

  } else {
    Serial.printf("[ERR] Perintah tidak dikenal: '%s'. Ketik 'help'.\n", cmd);
  }
}

// â”€â”€ Buffer Serial Zero-Heap â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
char serialBuf[64];
size_t bufIdx = 0;

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

  // â”€â”€ Baca input Serial (Non-blocking, anti fragmentasi RAM) â”€â”€â”€â”€â”€â”€
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (bufIdx > 0) {
        serialBuf[bufIdx] = '\0'; // Tutup string
        handleCommand(serialBuf);
        bufIdx = 0; // Reset buffer setelah perintah diproses
      }
    } else {
      if (bufIdx < sizeof(serialBuf) - 1) {
        serialBuf[bufIdx++] = c;
      }
    }
  }

  // â”€â”€ Auto-save dengan debounce â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
  if (settingsChanged && (now - tLastChange >= SAVE_DEBOUNCE_MS)) {
    saveSettings();
  }
}
```antara 0â€“100");
      return;
    }
    cfgBright = (uint8_t)val;
    settingsChanged = true;
    tLastChange = millis();
    Serial.printf("[OK] brightness = %d%%\n", cfgBright);

  } else if (cmd.startsWith("set temp ")) {
    float val = cmd.substring(9).toFloat();
    if (val < -40.0f || val > 125.0f) {
      Serial.println("[ERR] Suhu harus antara -40 s/d 125 Â°C");
      return;
    }
    cfgTemp = val;
    settingsChanged = true;
    tLastChange = millis();
    Serial.printf("[OK] temp_alarm = %.1f Â°C\n", cfgTemp);

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
      Serial.println("[ERR] Nama harus 1â€“15 karakter");
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

  // â”€â”€ Baca input Serial â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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

  // â”€â”€ Auto-save dengan debounce (Write-on-Change pattern) â”€â”€â”€â”€â”€â”€
  // Hanya menyimpan ke Flash jika ada perubahan DAN sudah 2 detik berlalu
  // Ini PENTING: jangan simpan ke Flash di setiap loop() iterasi!
  if (settingsChanged && (now - tLastChange >= SAVE_DEBOUNCE_MS)) {
    saveSettings();
  }
}
```

> ðŸ’¡ **Mengapa Write-on-Change + Debounce 2 Detik?**
> Halaman Flash NVS memiliki endurance terbatas (~10.000 siklus tulis). Jika kamu menulis ke NVS di setiap iterasi `loop()` (yang bisa berjalan 10.000+ kali per detik), kamu dapat **merusak Flash dalam hitungan menit**! Pattern yang benar:
> 1. Perubahan hanya ditandai (`settingsChanged = true`)
> 2. Penulisan ke Flash terjadi sekali saja, setelah 2 detik tidak ada perubahan baru

---

## 39.7 Program 4: FSM dengan Persistent State

> 📌 **FSM** *(Finite State Machine — Mesin Kondisi Berhingga)* adalah pola desain program di mana sistem hanya bisa berada di **satu kondisi (state) pada satu waktu**, dan berpindah antar state berdasarkan kejadian tertentu. Contoh sederhana: lampu lalu lintas bergerak MERAH → HIJAU → KUNING → MERAH. Dalam program ini, FSM mengelola 3 mode: `IDLE`, `MONITOR`, dan `ALERT`.

Program puncak BAB 39: sebuah FSM yang **mengingat mode dan konfigurasi terakhirnya** setelah ESP32 mati dan menyala kembali. Ini adalah dasar dari semua IoT device yang "cerdas" â€” seperti smart thermostat yang mengingat setpoint terakhirnya.

```cpp
/*
 * BAB 39 - Program 4: FSM dengan Persistent State (Memori Lintas Reboot)
 *
 * Fitur:
 *   â–¶ 3 mode FSM: IDLE, MONITOR, ALERT
 *   â–¶ Mode terakhir disimpan ke NVS dan dimuat saat boot
 *   â–¶ Threshold suhu tersimpan di NVS, dikonfigurasi via Serial
 *   â–¶ Counter notifikasi persisten (berapa kali alert dipicu total)
 *   â–¶ Timestamp boot terakhir (simulasi, pakai bootCount)
 *   â–¶ Tombol IO32: short-press = ganti mode, long-press = reset counter
 *
 * Hardware:
 *   BTN_MODE â†’ IO32 (pull-down 10KÎ© bawaan kit, aktif HIGH)
 */

#include <Preferences.h>

Preferences prefs;
#define NS_FSM "fsm_state"

// â”€â”€ Definisi FSM â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
enum AppMode { MODE_IDLE = 0, MODE_MONITOR = 1, MODE_ALERT = 2 };
const char* modeName[] = { "IDLE", "MONITOR", "ALERT" };
AppMode currentMode;

// â”€â”€ Persistent State â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
float    pTempThreshold;
uint32_t pAlertCount;
uint32_t pBootCount;

// â”€â”€ Hardware â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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

// â”€â”€ NVS Helpers â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
void loadPersistentState() {
  prefs.begin(NS_FSM, true);
  currentMode    = (AppMode)prefs.getInt  ("mode",      MODE_IDLE);
  pTempThreshold = prefs.getFloat("tempThr",  35.0f);
  pAlertCount    = prefs.getUInt ("alertCnt", 0);
  pBootCount     = prefs.getUInt ("bootCnt",  0);
  bool firstBoot = !prefs.isKey("mode");
  prefs.end();

  if (firstBoot) {
    Serial.println("[NVS] Pertama kali boot â€” menggunakan default state.");
  } else {
    Serial.printf("[NVS] State dimuat: mode=%s, threshold=%.1fÂ°C, alert=%u kali\n",
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

// â”€â”€ Transisi Mode â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
void switchMode(AppMode newMode) {
  currentMode = newMode;
  savePersistentState(); // Simpan segera saat mode berubah
  Serial.printf("\n[FSM] Mode â†’ %s (tersimpan ke NVS)\n", modeName[currentMode]);
}

// â”€â”€ Simulasi Sensor Suhu (tanpa sensor fisik) â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
float simulateTemp(unsigned long now) {
  // Suhu berfluktuasi antara 30â€“42Â°C dalam siklus sinusoidal simulasi
  return 36.0f + 6.0f * sinf(now / 8000.0f);
}

// â”€â”€ Ticker Display â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
unsigned long tDisplay = 0;

void printStatus(float temp) {
  Serial.printf("[%s] Suhu: %.1fÂ°C | Threshold: %.1fÂ°C | Alert: %u kali | Boot: %u\n",
    modeName[currentMode], temp, pTempThreshold, pAlertCount, pBootCount);
}

// â”€â”€ Serial Command Handler â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
String serialBuf = "";

void handleCommand(String cmd) {
  cmd.trim();
  if (cmd.length() == 0) return;

  if (cmd.startsWith("thr ")) {
    float v = cmd.substring(4).toFloat();
    if (v >= 20.0f && v <= 60.0f) {
      pTempThreshold = v;
      savePersistentState();
      Serial.printf("[CFG] Threshold diubah â†’ %.1fÂ°C (disimpan)\n", v);
    } else {
      Serial.println("[ERR] Threshold harus 20â€“60Â°C");
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

  // â”€â”€ Input: Short-press = ganti mode â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
  if (btn.shortPress) {
    switchMode((AppMode)((currentMode + 1) % 3));
  }

  // â”€â”€ Input: Long-press = reset alert counter â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
  if (btn.longPress) {
    pAlertCount = 0;
    savePersistentState();
    Serial.println("[RST] Alert counter di-reset ke 0 (disimpan)");
  }

  // â”€â”€ Serial CLI â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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

  // â”€â”€ Logika FSM â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
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
      if (temp <= pTempThreshold - 2.0f) { // Hysteresis 2Â°C
        switchMode(MODE_MONITOR);
      }
      break;
  }

  // â”€â”€ Display setiap 1 detik â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€
  if (now - tDisplay >= 1000UL) {
    tDisplay = now;
    if (currentMode != MODE_IDLE) {
      printStatus(temp);
    }
    if (currentMode == MODE_ALERT) {
      Serial.println("  âš ï¸  SUHU MELEBIHI THRESHOLD!");
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
[NVS] State dimuat: mode=MONITOR, threshold=35.0Â°C, alert=3 kali
[BOOT] Boot ke-8 | Mode dimuat: MONITOR

[MONITOR] Suhu: 33.2Â°C | Threshold: 35.0Â°C | Alert: 3 kali | Boot: 8
[MONITOR] Suhu: 36.7Â°C | Threshold: 35.0Â°C | Alert: 3 kali | Boot: 8

[FSM] Mode â†’ ALERT (tersimpan ke NVS)
  âš ï¸  SUHU MELEBIHI THRESHOLD!
[ALERT] Suhu: 38.1Â°C | Threshold: 35.0Â°C | Alert: 4 kali | Boot: 8
```

> ðŸ’¡ **Hysteresis pada threshold:** Perhatikan kondisi `temp <= pTempThreshold - 2.0f` saat keluar dari `MODE_ALERT`. Ini adalah teknik **hysteresis** â€” mode ALERT baru berakhir jika suhu turun 2Â°C di bawah threshold, bukan tepat di threshold. Tanpa hysteresis, FSM akan bolak-balik antara MONITOR dan ALERT dengan cepat (*oscillating*) saat suhu berada tepat di batas, menyebabkan penulisan NVS yang berlebihan.

---

## 39.8 Tips & Panduan Troubleshooting

### 1. `prefs.begin()` mengembalikan `false` atau gagal?
```text
Penyebab paling umum:

A. Namespace sudah dibuka oleh kode lain dan belum di-end()
   â†’ Pastikan setiap prefs.begin() diikuti prefs.end()
   â†’ Gunakan satu instance Preferences per namespace secara bergiliran

B. Partisi NVS rusak (jarang terjadi)
   â†’ Gunakan alat "Erase Flash" di Arduino IDE saat upload:
     Tools â†’ Erase All Flash Before Sketch Upload â†’ Enable

C. Namespace atau key melebihi 15 karakter
   â†’ Hitung karakter! "my_long_namespace" = 18 karakter â†’ TIDAK VALID
```

### 2. Nilai yang dibaca tidak sesuai yang disimpan?
```text
Penyebab:
A. Tipe data tidak cocok saat baca vs tulis
   â†’ putInt() HARUS dibaca dengan getInt(), bukan getUInt() atau getFloat()
   â†’ Tipe yang tidak cocok mengembalikan nilai default, bukan error

B. Namespace yang salah digunakan
   â†’ Pastikan string namespace identik saat write dan read

C. Key berubah (typo atau versi berbeda)
   â†’ prefs.isKey("keyname") untuk verifikasi key ada sebelum membaca
```

### 3. ESP32 semakin lambat / NVS penuh?
```text
Cek sisa kapasitas:
  prefs.begin("namespace", false);
  Serial.println(prefs.freeEntries());
  prefs.end();

NVS default menyimpan ~97 entri untuk partisi 4KB.
Jika mendekati penuh:
  â†’ Gunakan putBytes() untuk menyimpan struct sebagai satu entri
  â†’ Hapus key yang tidak diperlukan dengan prefs.remove("key")
  â†’ Pertimbangkan SPIFFS/LittleFS untuk data besar (file konfigurasi JSON)
```

### 4. Kapan sebaiknya NOT menggunakan NVS?
```text
âŒ JANGAN gunakan NVS untuk:
  â†’ Data yang berubah sangat sering (> ratusan kali per jam per key)
    karena Flash memiliki endurance terbatas
  â†’ Data besar (> beberapa KB) â†’ gunakan SPIFFS/LittleFS
  â†’ Log data sensorik berkecepatan tinggi â†’ gunakan SD Card
  â†’ Data sementara antar loop() â†’ gunakan variabel RAM biasa

âœ… GUNAKAN NVS untuk:
  â†’ Konfigurasi device (threshold, nama, interval)
  â†’ Mode atau state FSM terakhir
  â†’ Credential WiFi (SSID/*Service Set Identifier* — nama jaringan WiFi / Password)
  â†’ Counter/statistik yang perlu persisten
  â†’ Kalibrasi sensor yang disimpan di lapangan
```

---

## 39.9 Ringkasan

```
â”Œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”
â”‚                   RINGKASAN BAB 39 â€” NVS ESP32                      â”‚
â”œâ”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”¤
â”‚ KONSEP KUNCI:                                                         â”‚
â”‚   NVS = Non-Volatile Storage di Flash internal ESP32                â”‚
â”‚   Key-Value store terorganisir dalam Namespace (folder logika)       â”‚
â”‚   Wear-leveling otomatis â†’ distribusi tulis merata di Flash         â”‚
â”‚   Data bertahan selama power-off, reset, dan update firmware*       â”‚
â”‚   (*tergantung konfigurasi OTA/*Over-The-Air* — update firmware via WiFi / partition scheme)                  â”‚
â”‚                                                                       â”‚
â”‚ API PREFERENCES.H:                                                   â”‚
â”‚   prefs.begin("ns", rdOnly)  â†’ Buka namespace                      â”‚
â”‚   prefs.put<Type>("key", v)  â†’ Tulis nilai                         â”‚
â”‚   prefs.get<Type>("key", def)â†’ Baca nilai (+ default)              â”‚
â”‚   prefs.isKey("key")         â†’ Cek keberadaan key                  â”‚
â”‚   prefs.remove("key")        â†’ Hapus satu key                      â”‚
â”‚   prefs.clear()              â†’ Hapus semua key di namespace         â”‚
â”‚   prefs.end()                â†’ WAJIB dipanggil setelah selesai!    â”‚
â”‚                                                                       â”‚
â”‚ BEST PRACTICES:                                                       â”‚
â”‚   âœ… Write-on-Change: Hanya tulis saat nilai benar-benar berubah    â”‚
â”‚   âœ… Debounce Save: Tunda 1-2 detik setelah perubahan terakhir      â”‚
â”‚   âœ… Selalu panggil prefs.end() â€” jangan lupa!                      â”‚
â”‚   âœ… Gunakan prefs.isKey() untuk deteksi first-boot vs reload       â”‚
â”‚   âœ… Batasi nama namespace & key â‰¤ 15 karakter                      â”‚
â”‚                                                                       â”‚
â”‚ ANTIPATTERN (JANGAN):                                                 â”‚
â”‚   âŒ Tulis ke NVS di dalam loop() tanpa kondisi / debounce          â”‚
â”‚   âŒ Baca NVS dengan tipe berbeda dari tipe saat menulis            â”‚
â”‚   âŒ Gunakan NVS untuk data besar (> 4KB) â€” pakai LittleFS/SD      â”‚
â””â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”€â”˜
```

---

## 39.10 Latihan

1. **WiFi Credential Manager:** Simpan SSID dan password WiFi ke NVS. Saat boot, coba koneksi dengan credential tersimpan. Jika gagal dalam 10 detik, masuk ke mode AP (Access Point) untuk konfigurasi ulang. Simpan credential baru yang diterima via AP ke NVS. (Petunjuk: Integrasi dengan BAB WiFi).

2. **Odometer Servo:** Integrasikan NVS dengan Program 3 BAB 38 (Preset Servo). Setiap kali servo bergerak, hitung total derajat pergerakan kumulatif dan simpan ke NVS setiap 100 derajat. Tampilkan "odometer" ini saat boot. Reset via long-press.

3. **Struct Serialization:** Buat `struct SensorCalibration { float offset; float scale; uint8_t channel; }` dan simpan seluruh struct ke NVS menggunakan `prefs.putBytes()` sebagai satu entri tunggal. Muat kembali saat boot menggunakan `prefs.getBytes()`. Verifikasi integritas data dengan mencetak semua field.

4. **NVS Version Migration:** Buat program yang menyimpan versi skema NVS sebagai integer (misalnya `cfgVersion = 1`). Saat boot, baca versi tersimpan. Jika berbeda dari versi program aktif, jalankan fungsi migrasi data (misalnya menambahkan key baru dengan nilai default). Ini adalah teknik firmware upgrade yang digunakan di device IoT produksi.

5. **Multi-Namespace Logger:** Buat sistem logging sederhana dengan 3 namespace berbeda: `"daily"` (reset setiap 24 jam simulasi), `"weekly"` (reset setiap 7 hari simulasi), dan `"total"` (tidak pernah direset). Setiap namespace menyimpan nilai `maxTemp`, `minTemp`, dan `avgTemp` yang diperbarui menggunakan sensor DHT11 (IO27). Tampilkan semua laporan saat Serial `"report"` dikirim.

---

## â˜• Dukung Proyek Ini

Bab ini disusun dengan dedikasi waktu dan keahlian yang tidak sedikit. Jika materi *engineering-grade* ini bermanfaat, bentuk apresiasi terbaik adalah secangkir kopi untuk penulis:

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**ðŸ’¬ Konsultasi & Kolaborasi:**
- âœ‰ï¸ **Email:** antonprafanto@unmul.ac.id
- ðŸ“ž **WhatsApp:** 0811-553-393 / 0852-5069-0673

---

*Sebelumnya â†’ [BAB 38: Servo Motor](../BAB-38/README.md)*

*Selanjutnya â†’ [BAB 40: SPIFFS / LittleFS â€” Sistem File di Flash](../BAB-40/README.md)*

