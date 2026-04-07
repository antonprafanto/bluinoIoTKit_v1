# BAB 43: WiFi Access Point — ESP32 sebagai Hotspot Mandiri

> ✅ **Prasyarat:** Selesaikan **BAB 42 (WiFi Station Mode)**. Pemahaman tentang `WiFi.h`, event handler, dan prinsip non-blocking wajib dikuasai terlebih dahulu.

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami perbedaan arsitektural antara **mode STA, AP, dan AP+STA**.
- Menghidupkan ESP32 sebagai **Access Point mandiri** menggunakan `WiFi.softAP()`.
- Mengonfigurasi **IP, subnet, dan channel** Access Point secara kustom.
- **Memantau client** yang terhubung ke AP secara real-time.
- Menggunakan ESP32 sebagai **Hotspot Terisolir** untuk komunikasi lokal tanpa internet.
- Memahami **DHCP server bawaan** yang aktif otomatis di mode AP.
- Menerapkan mode **AP+STA (Dual Mode)** secara bersamaan.

---

## 43.1 Perbedaan STA vs AP vs AP+STA

```
TOPOLOGI JARINGAN WIFI ESP32:

  MODE STATION (STA) — BAB 42:
  ┌───────────┐        ┌──────────┐        ┌─────────┐
  │  ESP32    │◄──────►│  Router  │◄──────►│Internet │
  │  (Klien)  │  WiFi  │  (AP)    │        │         │
  └───────────┘        └──────────┘        └─────────┘
  ESP32 adalah tamu di jaringan. Bergantung pada router eksternal.

  ─────────────────────────────────────────────────────────────────

  MODE ACCESS POINT (AP) — BAB INI ✅:
  ┌───────────┐        ┌──────────────────┐
  │  HP/PC    │◄──────►│  ESP32           │
  │  (Klien)  │  WiFi  │  (AP / Hotspot)  │
  └───────────┘        └──────────────────┘
  ESP32 adalah tuan rumah. Tidak butuh router, tidak ada internet.
  Ideal untuk konfigurasi perangkat secara lokal (Captive Portal).

  ─────────────────────────────────────────────────────────────────

  MODE DUAL (AP + STA) — BAB 44:
  ┌────────────┐       ┌──────────────────┐       ┌─────────┐
  │  HP/PC     │◄─────►│  ESP32           │◄─────►│Internet │
  │  (Klien)   │       │  (AP sekaligus   │       │(via STA)│
  └────────────┘       │   STA)           │       └─────────┘
                       └──────────────────┘
  ESP32 meneruskan (bridge) koneksi klien ke internet.
  Digunakan untuk: Web Server + akses internet bersamaan.

KARAKTERISTIK MODE AP ESP32:
  ✅ Tidak butuh router atau infrastruktur jaringan eksternal
  ✅ Bisa diberi password WPA2 atau dibiarkan terbuka
  ✅ DHCP Server otomatis aktif (memberi IP ke klien)
  ✅ IP Default AP: 192.168.4.1
  ✅ Maksimal klien: 4 (default), bisa diset hingga 10
  ⚠️ Tidak ada akses internet di mode AP murni
  ⚠️ Konsumsi daya lebih tinggi dari mode STA
```

---

## 43.2 Referensi API `softAP` (`WiFi.h`)

```cpp
#include <WiFi.h>

// ── Membuat Access Point ───────────────────────────────────────────
WiFi.softAP("NamaHotspot");                       // AP tanpa password (OPEN)
WiFi.softAP("NamaHotspot", "PasswordAP");         // AP dengan WPA2
WiFi.softAP("NamaHotspot", "PasswordAP", ch, hidden, maxConn);
  // ch       = channel WiFi (1–13, default 1)
  // hidden   = true untuk menyembunyikan SSID (default false)
  // maxConn  = maks klien bersamaan (1–10, default 4)

// ── Konfigurasi IP Kustom ─────────────────────────────────────────
// WAJIB dipanggil SEBELUM softAP() agar berlaku!
IPAddress apIP(192, 168, 10, 1);
IPAddress apGateway(192, 168, 10, 1);
IPAddress apSubnet(255, 255, 255, 0);
WiFi.softAPConfig(apIP, apGateway, apSubnet);

// ── Status & Info AP ──────────────────────────────────────────────
IPAddress ip    = WiFi.softAPIP();          // IP Access Point (default 192.168.4.1)
String    mac   = WiFi.softAPmacAddress();  // MAC Address antarmuka AP
uint8_t   count = WiFi.softAPgetStationNum(); // Jumlah klien yang terhubung
bool      ok    = WiFi.softAPdisconnect(); // Matikan AP

// ── Event Spesifik AP ────────────────────────────────────────────
// ARDUINO_EVENT_WIFI_AP_START          — AP berhasil aktif
// ARDUINO_EVENT_WIFI_AP_STOP           — AP dimatikan
// ARDUINO_EVENT_WIFI_AP_STACONNECTED   — Ada klien baru terhubung
// ARDUINO_EVENT_WIFI_AP_STADISCONNECTED — Klien terputus
// ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED  — Klien mendapat IP dari DHCP

// ── Syarat Password AP ────────────────────────────────────────────
// Password HARUS minimal 8 karakter, atau AP akan gagal dibuat!
// Password = "" → AP terbuka tanpa enkripsi (OPEN)
```

### Tabel Pilihan Channel WiFi 2.4 GHz

| Channel | Frekuensi | Rekomendasi |
|---------|-----------|-------------|
| 1 | 2.412 GHz | ✅ Direkomendasikan (tidak overlap dengan Ch. 6 & 11) |
| 6 | 2.437 GHz | ✅ Direkomendasikan |
| 11 | 2.462 GHz | ✅ Direkomendasikan |
| 2-5, 7-10 | Overlap | ⚠️ Hindari — menyebabkan interferensi dengan AP lain |

> 💡 **Tips Channel:** Gunakan WiFi Scanner (BAB 42 Program 3) untuk melihat channel mana yang paling sepi di sekitar kamu, lalu gunakan channel tersebut untuk AP-mu.

---

## 43.3 Program 1: Hello AP — Hotspot Dasar dan Info Klien

```cpp
/*
 * BAB 43 - Program 1: Hello Access Point
 *
 * Tujuan:
 *   Buat ESP32 menjadi hotspot WiFi. Tampilkan info AP dan
 *   pantau klien yang terhubung/terputus via Event Handler.
 *
 * Cara uji:
 *   1. Upload dan buka Serial Monitor (115200 baud)
 *   2. Di HP/PC, buka daftar WiFi → cari "Bluino-AP-43"
 *   3. Masukkan password: bluino123
 *   4. Lihat notifikasi koneksi klien di Serial Monitor
 *
 * ⚠️  Password AP MINIMAL 8 KARAKTER. Lebih pendek = AP GAGAL dibuat!
 */

#include <WiFi.h>

// ── Konfigurasi AP ────────────────────────────────────────────────
const char* AP_SSID     = "Bluino-AP-43";
const char* AP_PASSWORD = "bluino123";    // Min 8 karakter!
const uint8_t AP_CHANNEL  = 1;           // Gunakan channel 1, 6, atau 11
const uint8_t AP_MAX_CONN = 4;           // Maks 4 klien bersamaan
const bool    AP_HIDDEN   = false;       // SSID ditampilkan

// ── IP Kustom AP ─────────────────────────────────────────────────
const IPAddress AP_IP     (10, 10, 10,  1);
const IPAddress AP_GATEWAY(10, 10, 10,  1);
const IPAddress AP_SUBNET (255, 255, 255, 0);

// ── Monitoring ───────────────────────────────────────────────────
const uint32_t MONITOR_INTERVAL_MS = 5000UL;
unsigned long  tMonitor = 0;
volatile uint8_t clientCount = 0; // volatile: diakses oleh Event task

// ─────────────────────────────────────────────────────────────────
// WiFi Event Handler
// ─────────────────────────────────────────────────────────────────
void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_AP_START:
      Serial.println("[AP] ✅ Access Point aktif!");
      Serial.printf ("[AP] SSID     : %s\n", AP_SSID);
      Serial.printf ("[AP] IP       : %s\n", WiFi.softAPIP().toString().c_str());
      Serial.printf ("[AP] MAC (AP) : %s\n", WiFi.softAPmacAddress().c_str());
      break;

    case ARDUINO_EVENT_WIFI_AP_STOP:
      Serial.println("[AP] ⛔ Access Point dimatikan.");
      break;

    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      clientCount++;
      Serial.printf("[AP] 🟢 Klien baru terhubung! (Total: %u)\n", clientCount);
      break;

    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      if (clientCount > 0) clientCount--;
      Serial.printf("[AP] 🔴 Klien terputus! (Tersisa: %u)\n", clientCount);
      break;

    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
      Serial.println("[AP] 📡 IP di-assign ke klien oleh DHCP server.");
      break;

    default:
      break;
  }
}

// ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== BAB 43 Program 1: Hello Access Point ===");

  // Daftarkan Event Handler SEBELUM mengaktifkan AP
  WiFi.onEvent(onWiFiEvent);

  // Set mode AP (WAJIB sebelum softAPConfig / softAP)
  WiFi.mode(WIFI_AP);

  // Konfigurasi IP WAJIB sebelum softAP() agar berlaku!
  if (!WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET)) {
    Serial.println("[ERR] Konfigurasi IP AP gagal!");
    return;
  }

  // Aktifkan AP
  // Validasi password: kosong = OPEN, atau minimal 8 karakter
  const char* apPass = (strlen(AP_PASSWORD) >= 8) ? AP_PASSWORD : nullptr;
  if (strlen(AP_PASSWORD) > 0 && strlen(AP_PASSWORD) < 8) {
    Serial.println("[WARN] Password AP < 8 karakter! AP dibuka tanpa password (OPEN).");
    Serial.println("[WARN] Untuk keamanan, pastikan password minimal 8 karakter.");
  }

  if (!WiFi.softAP(AP_SSID, apPass, AP_CHANNEL, AP_HIDDEN, AP_MAX_CONN)) {
    Serial.println("[ERR] Gagal membuat Access Point!");
    return;
  }

  // Informasi AP
  Serial.println("\n[INFO] Cara terhubung ke AP ini:");
  Serial.printf ("  1. Buka WiFi di HP/PC\n");
  Serial.printf ("  2. Cari SSID  : %s\n", AP_SSID);
  Serial.printf ("  3. Password   : %s\n", strlen(AP_PASSWORD) >= 8 ? AP_PASSWORD : "(terbuka)");
  Serial.printf ("  4. Setelah terhubung, buka browser: http://%s\n",
    AP_IP.toString().c_str());
  Serial.println("\n[MONITOR] Memantau klien setiap 5 detik...\n");
}

void loop() {
  unsigned long now = millis();
  if (now - tMonitor >= MONITOR_INTERVAL_MS) {
    tMonitor = now;
    uint8_t n = WiFi.softAPgetStationNum();
    if (n > 0) {
      Serial.printf("[Monitor] %u klien terhubung ke AP | Uptime: %lu s\n",
        n, millis() / 1000UL);
    } else {
      Serial.printf("[Monitor] Belum ada klien | AP aktif di %s | Uptime: %lu s\n",
        WiFi.softAPIP().toString().c_str(), millis() / 1000UL);
    }
  }
}
```

**Output contoh saat ada klien konek:**
```text
=== BAB 43 Program 1: Hello Access Point ===
[AP] ✅ Access Point aktif!
[AP] SSID     : Bluino-AP-43
[AP] IP       : 192.168.10.1
[AP] MAC (AP) : A4:CF:12:78:3B:0F

[INFO] Cara terhubung ke AP ini:
  1. Buka WiFi di HP/PC
  2. Cari SSID  : Bluino-AP-43
  3. Password   : bluino123
  4. Setelah terhubung, buka browser: http://192.168.10.1

[MONITOR] Memantau klien setiap 5 detik...

[Monitor] Belum ada klien | AP aktif di 10.10.10.1 | Uptime: 5 s
[AP] 🟢 Klien baru terhubung! (Total: 1)
[AP] 📡 IP di-assign ke klien oleh DHCP server.
[Monitor] 1 klien terhubung ke AP | Uptime: 10 s
[AP] 🔴 Klien terputus! (Tersisa: 0)
```

> 💡 **Mengapa IP AP disetel ke `10.10.10.1`?** IP default bawaan library ESP32 adalah `192.168.4.1`. Kita menggantinya dengan kelas IP A (`10.10.10.1`) menggunakan `softAPConfig()` agar lebih aman dan terhindar secara mutlak dari risiko tabrakan subnet dengan router rumah tangga biasa (yang umumnya memakai `192.168.x.x`).

---

## 43.4 Program 2: AP Manager — Kendali Penuh via Serial CLI

Program ini mendemonstrasikan AP dengan fitur manajemen lengkap: konfigurasi dinamis via Serial, pemantauan klien detail, dan penyimpanan konfigurasi AP ke NVS.

```cpp
/*
 * BAB 43 - Program 2: AP Manager dengan NVS & CLI
 *
 * Fitur:
 *   ▶ Buat hotspot WiFi dengan konfigurasi tersimpan di NVS
 *   ▶ CLI Serial: START, STOP, STATUS, SET ssid/pass/ch/maxconn, HELP
 *   ▶ Pantau klien real-time via Event Handler
 *   ▶ LED indikator: ON=AP aktif & ada klien, Blink=AP aktif tanpa klien
 *   ▶ Validasi ketat semua input konfigurasi
 *
 * Hardware:
 *   LED builtin ESP32 DEVKIT V1 → IO2
 */

#include <WiFi.h>
#include <Preferences.h>

// ── Pin ──────────────────────────────────────────────────────────
#define LED_PIN          2

// ── Konstanta ─────────────────────────────────────────────────────
const uint32_t BLINK_INTERVAL_MS  = 500UL;
const uint32_t STATUS_INTERVAL_MS = 10000UL;

// ── State (volatile: diakses lintas task Core 0 / Core 1) ─────────
volatile uint8_t  clientCount = 0;
volatile bool     apActive    = false;
volatile bool     manualStop  = false; // Penanda apakah AP dimatikan manual

// ── Timing ───────────────────────────────────────────────────────
unsigned long tBlink  = 0;
unsigned long tStatus = 0;
bool          ledState = false;

// ── NVS ──────────────────────────────────────────────────────────
Preferences prefs;

// ── Konfigurasi AP ───────────────────────────────────────────────
struct APConfig {
  String  ssid      = "Bluino-MyAP";
  String  password  = "bluino123";
  uint8_t channel   = 1;
  uint8_t maxConn   = 4;
  bool    hidden    = false;
} apCfg;

// ── Serial CLI ───────────────────────────────────────────────────
String serialBuf = "";

// ─────────────────────────────────────────────────────────────────
// Fungsi: Muat konfigurasi AP dari NVS
// ─────────────────────────────────────────────────────────────────
void loadAPConfig() {
  prefs.begin("ap-cfg", true);
  apCfg.ssid    = prefs.getString("ssid",    apCfg.ssid);
  apCfg.password = prefs.getString("pass",   apCfg.password);
  apCfg.channel = prefs.getUChar ("ch",      apCfg.channel);
  apCfg.maxConn = prefs.getUChar ("maxconn", apCfg.maxConn);
  apCfg.hidden  = prefs.getBool  ("hidden",  apCfg.hidden);
  prefs.end();

  // Validasi batas nilai setelah load (Defensive Programming)
  if (apCfg.channel < 1 || apCfg.channel > 13) apCfg.channel = 1;
  if (apCfg.maxConn < 1 || apCfg.maxConn > 10) apCfg.maxConn = 4;
  if (apCfg.ssid.length() < 1 || apCfg.ssid.length() > 32)
    apCfg.ssid = "Bluino-MyAP";

  Serial.println("[NVS] Konfigurasi AP dimuat.");
}

// ─────────────────────────────────────────────────────────────────
// Fungsi: Simpan konfigurasi AP ke NVS
// ─────────────────────────────────────────────────────────────────
void saveAPConfig() {
  prefs.begin("ap-cfg", false);
  prefs.putString("ssid",    apCfg.ssid);
  prefs.putString("pass",    apCfg.password);
  prefs.putUChar ("ch",      apCfg.channel);
  prefs.putUChar ("maxconn", apCfg.maxConn);
  prefs.putBool  ("hidden",  apCfg.hidden);
  prefs.end();
  Serial.println("[NVS] Konfigurasi AP disimpan.");
}

// ─────────────────────────────────────────────────────────────────
// WiFi Event Handler
// ─────────────────────────────────────────────────────────────────
void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_AP_START:
      apActive = true;
      Serial.println("\n[EVENT] ✅ Access Point aktif!");
      break;

    case ARDUINO_EVENT_WIFI_AP_STOP:
      apActive = false;
      clientCount = 0;
      if (manualStop) {
        Serial.println("\n[EVENT] ⛔ AP dihentikan sesuai perintah.");
      } else {
        Serial.println("\n[EVENT] ⚠️  AP berhenti tidak terduga!");
      }
      break;

    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      clientCount++;
      Serial.printf("\n[EVENT] 🟢 Klien #%u terhubung ke AP.\n", clientCount);
      break;

    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      if (clientCount > 0) clientCount--;
      Serial.printf("\n[EVENT] 🔴 Klien terputus. Tersisa: %u\n", clientCount);
      break;

    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
      Serial.println("[EVENT] 📡 IP diberikan oleh DHCP ke klien baru.");
      break;

    default:
      break;
  }
}

// ─────────────────────────────────────────────────────────────────
// Fungsi: Aktifkan AP
// ─────────────────────────────────────────────────────────────────
bool startAP() {
  if (apActive) {
    Serial.println("[AP] AP sudah aktif.");
    return true;
  }

  manualStop = false;

  // IP Kustom (Subnet Anti-Collision) — WAJIB sebelum softAP()
  IPAddress apIP(10, 10, 10, 1);
  IPAddress apGW(10, 10, 10, 1);
  IPAddress apSub(255, 255, 255, 0);

  WiFi.mode(WIFI_AP);

  if (!WiFi.softAPConfig(apIP, apGW, apSub)) {
    Serial.println("[ERR] Konfigurasi IP AP gagal!");
    return false;
  }

  // Validasi password: kosong = OPEN, atau min 8 kar.
  const char* pass = nullptr;
  if (apCfg.password.length() >= 8) {
    pass = apCfg.password.c_str();
  } else if (apCfg.password.length() > 0) {
    Serial.println("[WARN] Password < 8 karakter. AP dibuat sebagai OPEN.");
  }

  if (!WiFi.softAP(apCfg.ssid.c_str(), pass,
                   apCfg.channel, apCfg.hidden ? 1 : 0, apCfg.maxConn)) {
    Serial.println("[ERR] Gagal membuat AP!");
    return false;
  }

  Serial.printf("[AP] SSID    : %s\n", apCfg.ssid.c_str());
  Serial.printf("[AP] IP      : %s\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("[AP] Channel : %u\n", apCfg.channel);
  Serial.printf("[AP] Maks    : %u klien\n", apCfg.maxConn);
  Serial.printf("[AP] Password: %s\n", pass ? "(ada)" : "(TERBUKA)");
  return true;
}

// ─────────────────────────────────────────────────────────────────
// Fungsi: Matikan AP
// ─────────────────────────────────────────────────────────────────
void stopAP() {
  if (!apActive) { Serial.println("[AP] AP sudah tidak aktif."); return; }
  manualStop = true;
  WiFi.softAPdisconnect(true); // true = matikan antarmuka AP sepenuhnya
  digitalWrite(LED_PIN, LOW);
}

// ─────────────────────────────────────────────────────────────────
// Fungsi: Tampilkan status AP lengkap
// ─────────────────────────────────────────────────────────────────
void printStatus() {
  Serial.println("\n┌───────────────────────────────────────────────┐");
  Serial.println("│          Status Access Point Manager          │");
  Serial.println("├───────────────────────────────────────────────┤");
  Serial.printf ("│  AP Status   : %-31s│\n", apActive ? "AKTIF" : "MATI");
  Serial.printf ("│  SSID        : %-31s│\n", apCfg.ssid.c_str());
  Serial.printf ("│  Password    : %-31s│\n",
    apCfg.password.length() >= 8 ? "(tersimpan, WPA2)" : "(TERBUKA/Pendek)");
  Serial.printf ("│  Channel     : %-31u│\n", apCfg.channel);
  Serial.printf ("│  Maks Klien  : %-31u│\n", apCfg.maxConn);
  Serial.printf ("│  Hidden SSID : %-31s│\n", apCfg.hidden ? "Ya" : "Tidak");
  if (apActive) {
    Serial.printf("│  IP AP       : %-31s│\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("│  Klien Aktif : %-31u│\n", WiFi.softAPgetStationNum());
    Serial.printf("│  MAC (AP)    : %-31s│\n", WiFi.softAPmacAddress().c_str());
  }
  Serial.printf ("│  Uptime      : %-29lu s│\n", millis() / 1000UL);
  Serial.println("└───────────────────────────────────────────────┘\n");
}

// ─────────────────────────────────────────────────────────────────
// Serial CLI Handler
// ─────────────────────────────────────────────────────────────────
void handleCmd(const String& cmd) {
  if (cmd.equalsIgnoreCase("START")) {
    startAP();

  } else if (cmd.equalsIgnoreCase("STOP")) {
    stopAP();

  } else if (cmd.equalsIgnoreCase("STATUS")) {
    printStatus();

  } else if (cmd.equalsIgnoreCase("SAVE")) {
    saveAPConfig();

  } else if (cmd.equalsIgnoreCase("HELP")) {
    Serial.println("\n══ Perintah AP Manager ════════════════════════════");
    Serial.println("  START              — Aktifkan Access Point");
    Serial.println("  STOP               — Matikan Access Point");
    Serial.println("  STATUS             — Tampilkan status lengkap");
    Serial.println("  SAVE               — Simpan konfigurasi ke NVS");
    Serial.println("  SET ssid <nama>    — Ubah nama SSID (1-32 karakter)");
    Serial.println("  SET pass <pass>    — Ubah password (min 8 kar, atau kosong utk OPEN)");
    Serial.println("  SET ch <1-13>      — Ubah channel WiFi");
    Serial.println("  SET maxconn <1-10> — Ubah maks klien");
    Serial.println("  SET hidden <0/1>   — Sembunyikan SSID (0=tidak, 1=ya)");
    Serial.println("  HELP               — Panduan ini");
    Serial.println("═══════════════════════════════════════════════════\n");

  } else if (cmd.length() > 4 &&
             (cmd.substring(0,4).equalsIgnoreCase("SET "))) {
    
    // Perbaikan Double-Space Vulneraiblity: Ambil dari indeks 3 ("SET") lalu potong spasi ekstra
    String rest = cmd.substring(3);
    rest.trim();
    int sp = rest.indexOf(' ');

    // Perintah SET tanpa value (boleh untuk "pass" menghapus password)
    String key = (sp < 0) ? rest : rest.substring(0, sp);
    String val = (sp < 0) ? ""   : rest.substring(sp + 1);
    key.trim(); val.trim();

    if (key.equalsIgnoreCase("ssid")) {
      if (val.length() < 1 || val.length() > 32) {
        Serial.println("[ERR] SSID harus 1-32 karakter."); return;
      }
      apCfg.ssid = val;
      Serial.printf("[OK] SSID = '%s' (belum disimpan, ketik SAVE)\n", val.c_str());

    } else if (key.equalsIgnoreCase("pass")) {
      if (val.length() > 0 && val.length() < 8) {
        Serial.println("[ERR] Password minimal 8 karakter, atau kosong untuk AP terbuka.");
        return;
      }
      if (val.length() > 63) {
        Serial.println("[ERR] Password WPA2 maksimal 63 karakter ASCII."); return;
      }
      apCfg.password = val;
      Serial.printf("[OK] Password %s (belum disimpan, ketik SAVE)\n",
        val.length() == 0 ? "dihapus (AP akan TERBUKA)" : "diperbarui.");

    } else if (key.equalsIgnoreCase("ch")) {
      int v = val.toInt();
      if (v < 1 || v > 13) {
        Serial.println("[ERR] Channel harus 1-13."); return;
      }
      apCfg.channel = (uint8_t)v;
      Serial.printf("[OK] Channel = %d (berlaku setelah STOP + START)\n", v);

    } else if (key.equalsIgnoreCase("maxconn")) {
      int v = val.toInt();
      if (v < 1 || v > 10) {
        Serial.println("[ERR] Maks koneksi harus 1-10."); return;
      }
      apCfg.maxConn = (uint8_t)v;
      Serial.printf("[OK] Maks klien = %d (berlaku setelah STOP + START)\n", v);

    } else if (key.equalsIgnoreCase("hidden")) {
      apCfg.hidden = (val == "1" || val.equalsIgnoreCase("true"));
      Serial.printf("[OK] Hidden SSID = %s\n", apCfg.hidden ? "Ya" : "Tidak");

    } else {
      Serial.printf("[ERR] Key tidak dikenal: '%s'. Ketik HELP.\n", key.c_str());
    }

  } else if (cmd.length() > 0) {
    Serial.printf("[ERR] Perintah tidak dikenal: '%s'. Ketik HELP.\n", cmd.c_str());
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  delay(500);

  Serial.println("\n=== BAB 43 Program 2: AP Manager ===");
  Serial.println("Ketik HELP untuk panduan perintah.\n");

  // WAJIB: Init WiFi mode sebelum operasi apapun
  WiFi.mode(WIFI_AP);

  // Daftarkan event handler SEBELUM softAP()
  WiFi.onEvent(onWiFiEvent);

  // Muat konfigurasi dari NVS
  loadAPConfig();

  // Langsung aktifkan AP dengan konfigurasi tersimpan
  startAP();
}

void loop() {
  unsigned long now = millis();

  // ── Serial CLI ─────────────────────────────────────────────────
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      String cmd = serialBuf;
      cmd.trim();
      serialBuf = "";
      handleCmd(cmd);
    } else {
      if (serialBuf.length() < 64) serialBuf += c; // OOM Protection
    }
  }

  // ── LED indikator: blink=AP aktif tanpa klien, solid=ada klien ─
  if (apActive) {
    if (clientCount > 0) {
      // Ada klien → LED menyala solid
      digitalWrite(LED_PIN, HIGH);
    } else {
      // AP aktif tanpa klien → LED berkedip
      if (now - tBlink >= BLINK_INTERVAL_MS) {
        tBlink = now;
        ledState = !ledState;
        digitalWrite(LED_PIN, ledState ? HIGH : LOW);
      }
    }
  } else {
    digitalWrite(LED_PIN, LOW);
  }

  // ── Heartbeat periodik ─────────────────────────────────────────
  if (apActive && (now - tStatus >= STATUS_INTERVAL_MS)) {
    tStatus = now;
    Serial.printf("[Heartbeat] AP: %s | Klien: %u | Uptime: %lu s\n",
      apCfg.ssid.c_str(), WiFi.softAPgetStationNum(), millis() / 1000UL);
  }
}
```

---

## 43.5 Program 3: Dual Mode AP+STA — Bridge Jaringan

Program ini menjalankan ESP32 secara bersamaan sebagai **Access Point** (hotspot) sekaligus **Station** (klien router). Ini adalah pondasi untuk BAB 44 (WiFiManager) dan BAB 46 (Web Server + Internet).

```cpp
/*
 * BAB 43 - Program 3: Dual Mode WiFi (AP + STA bersamaan)
 *
 * Topologi:
 *   HP/Laptop ────WiFi───► ESP32 (AP, 10.10.10.1)
 *                            │
 *                            └── WiFi (STA) ──►  Router ──► Internet
 *
 * Fitur:
 *   ▶ ESP32 membuat hotspot (AP) sekaligus terhubung ke router (STA)
 *   ▶ Status kedua koneksi ditampilkan secara terpisah
 *   ▶ Event handler yang membedakan event AP dan STA
 *
 * ⚠️  GANTI WIFI_STA_SSID dan WIFI_STA_PASS sesuai jaringan kamu!
 */

#include <WiFi.h>

// ── Konfigurasi AP ────────────────────────────────────────────────
const char* AP_SSID     = "Bluino-Bridge";
const char* AP_PASSWORD = "bluino123";
// Menggunakan Subnet 10.10.10.x yang sangat unik untuk menghindari 
// tabrakan rute LwIP (LwIP Routing Collision) dengan router rumah tangga
const IPAddress AP_IP     (10, 10, 10, 1);
const IPAddress AP_GATEWAY(10, 10, 10, 1);
const IPAddress AP_SUBNET (255, 255, 255, 0);

// ── Konfigurasi STA (router/internet) ─────────────────────────────
const char*    STA_SSID     = "NamaWiFiRouter";
const char*    STA_PASSWORD = "PasswordRouter";
const uint32_t STA_TIMEOUT_MS = 20000UL;

// ── State ─────────────────────────────────────────────────────────
volatile bool     staConnected = false;
volatile uint8_t  apClients   = 0;
const uint32_t    STATUS_INTERVAL_MS = 10000UL;
unsigned long     tStatus = 0;

// ─────────────────────────────────────────────────────────────────
// WiFi Event Handler — membedakan event AP dan STA
// ─────────────────────────────────────────────────────────────────
void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
    // ──── AP Events ────────────────────────────────────────────
    case ARDUINO_EVENT_WIFI_AP_START:
      Serial.println("[AP]  ✅ Access Point aktif.");
      break;

    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      apClients++;
      Serial.printf("[AP]  🟢 Klien #%u terhubung ke AP.\n", apClients);
      break;

    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      if (apClients > 0) apClients--;
      Serial.printf("[AP]  🔴 Klien terputus. Tersisa: %u\n", apClients);
      break;

    // ──── STA Events ───────────────────────────────────────────
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      staConnected = true;
      Serial.printf("[STA] ✅ Terhubung ke router! IP: %s\n",
        WiFi.localIP().toString().c_str());
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      staConnected = false;
      Serial.println("[STA] ⚠️  Terputus dari router. Auto-reconnect aktif...");
      break;

    default:
      break;
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== BAB 43 Program 3: Dual Mode WiFi (AP+STA) ===\n");

  // Daftarkan Event SEBELUM mode/AP/STA diaktifkan
  WiFi.onEvent(onWiFiEvent);

  // ── Aktifkan Dual Mode ────────────────────────────────────────
  WiFi.mode(WIFI_AP_STA); // Kunci Utama: Dual mode!

  // ── Setup AP ──────────────────────────────────────────────────
  WiFi.softAPsetHostname("bluino-ap-bridge"); // Nama host untuk sisi Access Point
  WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET);
  
  // Validasi password AP (Wajib >= 8 Karakter)
  const char* apPass = (strlen(AP_PASSWORD) >= 8) ? AP_PASSWORD : nullptr;
  if (!WiFi.softAP(AP_SSID, apPass)) {
    Serial.println("[AP]  ❌ Gagal membuat Access Point!");
    return; // Hentikan operasi jika AP gagal 
  }
  
  Serial.printf("[AP]  SSID: %s | IP: %s\n",
    AP_SSID, WiFi.softAPIP().toString().c_str());

  // ── Setup STA → sambung ke router ─────────────────────────────
  WiFi.setAutoReconnect(true);
  WiFi.setHostname("bluino-sta-client"); // Nama host untuk sisi Router Rumah
  WiFi.begin(STA_SSID, STA_PASSWORD);
  Serial.printf("[STA] Menghubungkan ke '%s'...\n", STA_SSID);

  // Tunggu koneksi STA dengan timeout (non-blocking terhadap AP)
  unsigned long tStart = millis();
  while (!staConnected) {
    if (millis() - tStart >= STA_TIMEOUT_MS) {
      Serial.println("[STA] ⚠️  Timeout koneksi ke router.");
      Serial.println("[STA]     AP tetap berjalan. Auto-reconnect aktif di background.");
      // JANGAN panggil WiFi.disconnect() di sini — akan mematikan auto-reconnect STA!
      break;
    }
    Serial.print(".");
    delay(500);
    yield(); // Watchdog protection
  }
  Serial.println();

  Serial.println("\n[INFO] Dual Mode aktif:");
  Serial.printf ("  AP  (%s) : %s — %s\n",
    WiFi.softAPgetHostname(), AP_SSID, WiFi.softAPIP().toString().c_str());
  Serial.printf ("  STA (%s) : %s\n",
    WiFi.getHostname(), staConnected ? WiFi.localIP().toString().c_str() : "Belum terhubung");
}

void loop() {
  unsigned long now = millis();
  if (now - tStatus >= STATUS_INTERVAL_MS) {
    tStatus = now;
    Serial.printf("[Dual] AP Klien: %u | STA: %s | Uptime: %lu s\n",
      WiFi.softAPgetStationNum(),
      staConnected ? WiFi.localIP().toString().c_str() : "TERPUTUS",
      millis() / 1000UL);
  }
}
```

> 💡 **Kasus Penggunaan Dual Mode:** Pola ini digunakan di **WiFiManager** (BAB 44) di mana ESP32 menyajikan halaman web konfigurasi melalui AP, sementara setelah konfigurasi selesai ia beralih ke mode STA. Di **Web Server** (BAB 46), Dual Mode memungkinkan pengguna lokal (via AP) dan pengguna dari internet (via STA+ Port Forwarding) untuk mengakses antarmuka yang sama.

---

## 43.6 Troubleshooting Access Point

```
MASALAH UMUM DAN SOLUSINYA:

┌──────────────────────────────────────────────────────────────────────┐
│ MASALAH 1: softAP() mengembalikan false (gagal buat AP)             │
├──────────────────────────────────────────────────────────────────────┤
│ Penyebab paling umum: Password kurang dari 8 karakter               │
│ Solusi:                                                              │
│  ✓ Pastikan password ≥ 8 karakter                                   │
│  ✓ Untuk AP terbuka, gunakan nullptr/NULL atau string kosong ""      │
│  ✓ Pastikan WiFi.mode(WIFI_AP) dipanggil SEBELUM softAP()           │
│  ✓ Pastikan softAPConfig() dipanggil SEBELUM softAP()               │
└──────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────┐
│ MASALAH 2: Klien dapat IP tapi tidak bisa ping 192.168.10.1         │
├──────────────────────────────────────────────────────────────────────┤
│ Penyebab: softAPConfig() dipanggil SETELAH softAP()                 │
│ Solusi:                                                              │
│  ✓ Urutan WAJIB: mode() → softAPConfig() → softAP()                 │
│  ✓ Jika sudah terlambat, panggil softAPdisconnect() dulu lalu ulangi│
└──────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────┐
│ MASALAH 3: Hanya bisa konek 1-2 klien padahal maxConn=4            │
├──────────────────────────────────────────────────────────────────────┤
│ Penyebab: Chip ESP32 memiliki batas hardware ~5 klien bersamaan     │
│ Solusi:                                                              │
│  ✓ Set maxConn maksimal 4-5 untuk stabilitas                        │
│  ✓ Klien ke-5+ mungkin bisa konek secara teknis, tapi tidak stabil  │
│  ✓ Untuk IoT Gateway, gunakan MQTT broker, bukan raw AP connection  │
└──────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────┐
│ MASALAH 4: Dual Mode (AP+STA) — STA putus saat klien AP konek      │
├──────────────────────────────────────────────────────────────────────┤
│ Penyebab: AP dan STA *harus* beroperasi di channel yang sama di     │
│ ESP32. Saat ada klien baru konek ke AP, ESP32 mungkin berpindah     │
│ channel untuk mengikuti router STA, menyebabkan klien AP terputus.  │
│ Solusi:                                                              │
│  ✓ Set channel AP sama dengan channel router STA                    │
│  ✓ Gunakan WiFi.channel() setelah STA connect untuk cek channel     │
│  ✓ Ini adalah limitasi hardware ESP32 — tidak bisa sepenuhnya dihindari│
└──────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────┐
│ MASALAH 5: SSID tidak muncul di HP setelah softAP() dipanggil      │
├──────────────────────────────────────────────────────────────────────┤
│ Penyebab: Mode WiFi tidak disetel ke WIFI_AP atau WIFI_AP_STA       │
│ Solusi:                                                              │
│  ✓ Pastikan WiFi.mode(WIFI_AP) dipanggil sebelum softAP()           │
│  ✓ Tunggu minimal 500ms setelah Wifi.mode() sebelum scan di HP      │
│  ✓ Coba restart HP/PC untuk refresh daftar WiFi                     │
└──────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────┐
│ MASALAH 6: Dual Mode (AP+STA) — Klien AP Gagal Loading Halaman Kueri │
├──────────────────────────────────────────────────────────────────────┤
│ Penyebab: Konflik Subnet IP (LwIP Routing Collision). Jika router    │
│ (STA) di rumahmu mengalokasikan IP 192.168.10.x ke ESP32, AP kita   │
│ yang menggunakan subnet statis 192.168.10.1 akan saling bertabrakan! │
│ Solusi:                                                              │
│  ✓ Pastikan segmen ke-3 IP AP (Subnet) berbeda dengan Router STA.   │
│  ✓ Sangat disarankan mengubah IP AP ke alamat unik seperti           │
│    10.10.10.1 atau 172.20.0.1 untuk Mode Dual-Bridge yang aman.      │
└──────────────────────────────────────────────────────────────────────┘
```

---

## 43.7 Ringkasan & Best Practices

```
PRINSIP AP MODE YANG TANGGUH:

  1. URUTAN INISIALISASI WAJIB (Fatal jika salah urutan):
     ┌──────────────────────────────────────────────────────────┐
     │  1. WiFi.onEvent(handler)    ← Event Handler dulu       │
     │  2. WiFi.mode(WIFI_AP)       ← Mode AP                  │
     │  3. WiFi.softAPConfig(...)   ← IP Kustom (jika perlu)   │
     │  4. WiFi.softAP(ssid, pass, ch, hidden, maxConn)        │
     └──────────────────────────────────────────────────────────┘

  2. VALIDASI PASSWORD SELALU:
     ┌──────────────────────────────────────────────────────────┐
     │  Password < 8 karakter → softAP() GAGAL tanpa error msg! │
     │  Selalu validasi: if (pass.length() < 8) pass = nullptr  │
     └──────────────────────────────────────────────────────────┘

  3. VOLATILE untuk SEMUA STATE yang DIAKSES EVENT HANDLER:
     ┌──────────────────────────────────────────────────────────┐
     │  volatile uint8_t clientCount = 0;                       │
     │  volatile bool    apActive    = false;                    │
     └──────────────────────────────────────────────────────────┘

  4. FLAG MANUAL STOP untuk EVENT HANDLER yang JUJUR:
     ┌──────────────────────────────────────────────────────────┐
     │  volatile bool manualStop = false;                        │
     │  // Set true sebelum softAPdisconnect() agar handler     │
     │  // tidak mencetak pesan "berhenti tidak terduga"        │
     └──────────────────────────────────────────────────────────┘

ANTIPATTERN KLASIK AP MODE:
  ❌ softAPConfig() setelah softAP() — IP kustom tidak berlaku
  ❌ Password AP < 8 karakter — AP gagal dibuat diam-diam
  ❌ Tidak memanggil WiFi.mode() sebelum softAP()
  ❌ state bool clientCount tanpa volatile di event-driven code
  ❌ Dual Mode tanpa channel alignment — klien AP sering drop
```

---

## 43.8 Latihan

**Latihan 1 — Dasar:** Modifikasi Program 1 untuk membunyikan Active Buzzer (1 kali) setiap ada klien baru yang terhubung ke AP.

**Latihan 2 — Menengah:** Tambahkan perintah `CLIENTS` di Program 2 yang menampilkan jumlah klien terhubung sekarang menggunakan `WiFi.softAPgetStationNum()`.

**Latihan 3 — Menengah:** Modifikasi Program 2 agar menyimpan "statistik klien": total koneksi sejak boot. Tampilkan di perintah `STATUS`. Simpan statistik ke NVS agar awet setelah restart.

**Latihan 4 — Lanjutan:** Gabungkan Program 2 (AP Manager) dengan BAB 40 (LittleFS): Setiap klien yang konek/putus dicatat ke file `/ap_log.txt` dengan format `[uptime_ms] EVENT CLIENT`. Tambahkan perintah `LOG` di CLI.

**Latihan 5 — Mahir (Proyek):** Implementasikan **Channel Auto-Select**: saat ESP32 boot, jalankan scan WiFi singkat (Program 3 BAB 42), identifikasi channel mana yang paling sedikit AP-nya, lalu aktifkan AP di channel tersebut secara otomatis.

---

## 43.9 Apa Selanjutnya?

| BAB | Topik | Hubungan dengan BAB ini |
|-----|-------|-------------------------|
| **BAB 44** | WiFiManager & Captive Portal | Menggunakan AP untuk konfigurasi WiFi via browser |
| **BAB 45** | mDNS | Akses AP via nama domain (`bluino.local`) |
| **BAB 46** | Web Server | Sajikan halaman HTML melalui AP |
| **BAB 47** | WebSocket | Komunikasi real-time antara klien AP dan ESP32 |
