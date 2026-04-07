# BAB 42: WiFi Station Mode — Koneksi Internet ESP32

> ✅ **Prasyarat:** Selesaikan **BAB 16 (Non-Blocking Programming)**, **BAB 39 (NVS)**, dan **BAB 41 (MicroSD)**. ESP32 DEVKIT V1 pada kit Bluino dilengkapi **WiFi 802.11 b/g/n** bawaan — tidak perlu modul tambahan.

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami **arsitektur WiFi ESP32**: peran radio, protokol stack, dan driver.
- Menghubungkan ESP32 ke Access Point (router) menggunakan **mode Station (STA)**.
- Menerapkan strategi **koneksi yang tangguh** (robust): retry, timeout, dan reconnect otomatis.
- Menyimpan credentials WiFi secara aman menggunakan **NVS (Preferences)**.
- Memantau **kualitas sinyal (RSSI)** dan status jaringan secara real-time.
- Melakukan **scanning jaringan WiFi** di sekitar untuk discovery.
- Memahami **event-driven WiFi** menggunakan callback `WiFiEvent`.

---

## 42.1 Arsitektur WiFi pada ESP32

```
LAPISAN STACK WIFI ESP32:
                                        ┌───────────────────────────────────┐
 LAPISAN APLIKASI                       │  Kode Arduino kamu (setup/loop)   │
 (Yang kita tulis)                      │  WiFiClient, HTTPClient, MQTT, dll │
                                        └───────────────┬───────────────────┘
                                                        │ API
                                        ┌───────────────▼───────────────────┐
 LAPISAN LIBRARY                        │      WiFi.h (Arduino ESP32)        │
 (Abstraksi C++ Arduino)                │  WiFi.begin(), WiFi.status(), dll  │
                                        └───────────────┬───────────────────┘
                                                        │ Panggil
                                        ┌───────────────▼───────────────────┐
 LAPISAN SDK (ESP-IDF)                  │     esp_wifi.h / LwIP Stack        │
 (Dikelola Espressif)                   │  TCP/IP, DHCP, DNS, ARP            │
                                        └───────────────┬───────────────────┘
                                                        │ Interrupt
                                        ┌───────────────▼───────────────────┐
 LAPISAN HARDWARE                       │  Radio WiFi ESP32 (inti ULP Co-   │
 (Silikon)                              │  processor) — 2.4 GHz 802.11b/g/n │
                                        └───────────────────────────────────┘

FAKTA TEKNIS PENTING:
  ▶ WiFi ESP32 dijalankan di Core 0, kode Arduino di Core 1 (dual-core)
  ▶ Stack TCP/IP (LwIP) dikelola oleh FreeRTOS task terpisah
  ▶ Jarak: ~100m outdoor (line-of-sight), ~30m indoor (kondisi ideal)
  ▶ Kecepatan: Max 20 Mbps (UDP), ~10 Mbps (TCP) dalam kondisi laboratorium
  ▶ Konsumsi daya: ~160 mA (aktif Tx), ~5 mA (modem-sleep), <1 mA (light-sleep)
```

### Mode Operasi WiFi ESP32

| Mode | Fungsi | Penggunaan |
|------|--------|------------|
| **STA (Station)** | Klien — terhubung ke router/AP | BAB ini ← |
| **AP (Access Point)** | Hotspot — ESP32 jadi router sementara | BAB 43 |
| **AP+STA** | Dual mode bersamaan | BAB 44 |
| **WiFi Off (`WIFI_OFF`)** | Matikan WiFi, hemat daya | BAB 68 |

---

## 42.2 Konsep Dasar: Siklus Hidup Koneksi WiFi

```
SIKLUS KONEKSI WIFI (STATE MACHINE):

  ┌──────────┐   WiFi.begin()   ┌──────────────┐   Autentikasi    ┌──────────────┐
  │   IDLE   ├─────────────────►│  CONNECTING  ├─────────────────►│ WAITING DHCP │
  └──────────┘                  └──────────────┘   Berhasil        └──────┬───────┘
                                                                          │ DHCP Assign IP
                                                                   ┌──────▼───────┐
                                                              ┌────┤  CONNECTED   ├────┐
                                                              │    └──────────────┘    │
                                                    Disconnect│              Signal Drop│
                                                              │    ┌──────────────┐    │
                                                              └───►│ DISCONNECTED ├────┘
                                                                   └──────┬───────┘
                                                            WiFi.reconnect()│
                                                                   └────────┘

STATUS WiFi (WiFi.status()):
  WL_IDLE_STATUS     (0) — Awal, belum ada perintah
  WL_NO_SSID_AVAIL   (1) — SSID tidak ditemukan di scan
  WL_SCAN_COMPLETED  (2) — Scan selesai
  WL_CONNECTED       (3) — ✅ Terhubung, IP tersedia
  WL_CONNECT_FAILED  (4) — Password salah (4 kali gagal)
  WL_CONNECTION_LOST (5) — Koneksi terputus (AP hilang)
  WL_DISCONNECTED    (6) — Sengaja disconnect
```

---

## 42.3 Referensi API `WiFi.h`

```cpp
#include <WiFi.h>

// ── Koneksi & Kontrol ─────────────────────────────────────────────
WiFi.begin(ssid, password);          // Mulai koneksi ke AP
WiFi.disconnect();                   // Putuskan koneksi
WiFi.reconnect();                    // Coba sambung ulang
WiFi.setAutoReconnect(true);         // Aktifkan reconnect otomatis (DEFAULT: true)
WiFi.setAutoConnect(true);           // Sambung otomatis saat boot

// ── Status ───────────────────────────────────────────────────────
wl_status_t s = WiFi.status();       // Status koneksi saat ini
bool ok = WiFi.isConnected();        // true jika status == WL_CONNECTED

// ── Info Jaringan ─────────────────────────────────────────────────
IPAddress ip   = WiFi.localIP();     // IP yang diterima dari DHCP
IPAddress gw   = WiFi.gatewayIP();   // Gateway (IP router)
IPAddress mask = WiFi.subnetMask();  // Subnet mask
IPAddress dns  = WiFi.dnsIP();       // DNS server
String    mac  = WiFi.macAddress();  // MAC address ESP32 (format XX:XX:XX:XX:XX:XX)
int32_t rssi   = WiFi.RSSI();        // Kekuatan sinyal dBm (semakin ke 0, semakin kuat)
String  ssid   = WiFi.SSID();        // Nama AP yang terhubung
String  bssid  = WiFi.BSSIDstr();    // MAC address AP

// ── Konfigurasi ───────────────────────────────────────────────────
WiFi.setHostname("bluino-iot-kit");  // Nama device di jaringan (mDNS)
WiFi.config(staticIP, gateway, subnet); // Set IP statis (opsional)
WiFi.mode(WIFI_STA);                 // Set mode: WIFI_STA / WIFI_AP / WIFI_AP_STA / WIFI_OFF

// ── Scanning ─────────────────────────────────────────────────────
int n = WiFi.scanNetworks();         // Pindai (blocking) — return jumlah AP ditemukan
WiFi.scanNetworks(true);             // Pindai async (non-blocking)
int result = WiFi.scanComplete();    // Cek hasil scan (-1=sedang jalan, -2=tidak dijalankan)
for (int i = 0; i < n; i++) {
  Serial.printf("  [%d] SSID: %-25s  RSSI: %3d dBm  Ch: %2d  Enc: %s\n",
    i, WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i),
    (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "OPEN" : "WPA2");
}
WiFi.scanDelete(); // Bebaskan memori hasil scan!

// ── Event Handler ─────────────────────────────────────────────────
WiFi.onEvent(callback);              // Daftarkan handler event WiFi
// Event: ARDUINO_EVENT_WIFI_STA_GOT_IP, ARDUINO_EVENT_WIFI_STA_DISCONNECTED, dll
```

### Panduan Membaca RSSI (Kekuatan Sinyal)

| RSSI (dBm) | Kualitas | Keterangan |
|------------|----------|------------|
| > -50 dBm | ⭐⭐⭐⭐⭐ Luar biasa | Sangat dekat AP |
| -50 s/d -60 | ⭐⭐⭐⭐ Sangat Baik | Koneksi stabil, kecepatan optimal |
| -60 s/d -70 | ⭐⭐⭐ Baik | Kondisi normal dalam ruangan |
| -70 s/d -80 | ⭐⭐ Lemah | Mulai tidak stabil, latency naik |
| < -80 dBm | ⭐ Sangat Lemah | Sering putus, tidak direkomendasikan |

---

## 42.4 Program 1: Hello WiFi — Koneksi Dasar & Info Jaringan

```cpp
/*
 * BAB 42 - Program 1: Hello WiFi
 *
 * Tujuan:
 *   Koneksi ke WiFi dengan mekanisme retry+timeout yang tangguh,
 *   tampilkan info jaringan lengkap, dan monitor sinyal setiap 5 detik.
 *
 * ⚠️  GANTI ssid dan password sesuai jaringan kamu!
 */

#include <WiFi.h>

// ── Kredensial WiFi ───────────────────────────────────────────────
// PENTING: Jangan commit file ini ke repository publik jika SSID/password asli!
const char* WIFI_SSID = "NamaWiFiKamu";
const char* WIFI_PASS = "PasswordWiFiKamu";

// ── Konfigurasi ───────────────────────────────────────────────────
const uint32_t WIFI_TIMEOUT_MS  = 15000UL; // Batas waktu koneksi: 15 detik
const uint8_t  WIFI_MAX_RETRY   = 3;       // Maksimal percobaan ulang
const uint32_t RSSI_INTERVAL_MS = 5000UL;  // Tampilkan RSSI setiap 5 detik

unsigned long tRssi = 0;

// ─────────────────────────────────────────────────────────────────
// Fungsi: Koneksi ke WiFi dengan retry dan timeout
// Return: true jika berhasil, false jika semua retry gagal
// ─────────────────────────────────────────────────────────────────
bool connectWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);      // Aktifkan auto-reconnect bawaan
  WiFi.setHostname("bluino-42");    // Nama device di jaringan lokal

  for (uint8_t attempt = 1; attempt <= WIFI_MAX_RETRY; attempt++) {
    Serial.printf("[WiFi] Percobaan %u/%u — Menghubungkan ke '%s'...\n",
      attempt, WIFI_MAX_RETRY, WIFI_SSID);

    WiFi.begin(WIFI_SSID, WIFI_PASS);

    // Tunggu terhubung dengan batas waktu
    unsigned long tStart = millis();
    while (WiFi.status() != WL_CONNECTED) {
      if (millis() - tStart >= WIFI_TIMEOUT_MS) {
        Serial.println("[WiFi] TIMEOUT! Koneksi tidak berhasil dalam 15 detik.");
        WiFi.disconnect(true); // Bersihkan state internal driver
        delay(1000);
        break;
      }
      Serial.print(".");
      delay(500);
    }
    Serial.println(); // Newline setelah titik-titik

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("[WiFi] ✅ TERHUBUNG!");
      return true;
    }

    // Diagnosa kegagalan berdasarkan status
    wl_status_t st = WiFi.status();
    if (st == WL_NO_SSID_AVAIL) {
      Serial.printf("[WiFi] ❌ SSID '%s' tidak ditemukan. Cek nama WiFi.\n", WIFI_SSID);
    } else if (st == WL_CONNECT_FAILED) {
      Serial.println("[WiFi] ❌ Password salah atau autentikasi gagal.");
    } else {
      Serial.printf("[WiFi] ❌ Gagal (status: %d)\n", (int)st);
    }

    if (attempt < WIFI_MAX_RETRY) {
      Serial.println("[WiFi] Menunggu 3 detik sebelum percobaan ulang...");
      delay(3000);
    }
  }

  Serial.println("[WiFi] ❌ SEMUA PERCOBAAN GAGAL.");
  return false;
}

// ─────────────────────────────────────────────────────────────────
// Fungsi: Tampilkan info jaringan lengkap
// ─────────────────────────────────────────────────────────────────
void printNetworkInfo() {
  Serial.println("\n╔══════════════════════════════════════════════╗");
  Serial.println("║           Informasi Jaringan WiFi            ║");
  Serial.println("╠══════════════════════════════════════════════╣");
  Serial.printf ("║  SSID        : %-29s║\n", WiFi.SSID().c_str());
  Serial.printf ("║  IP Address  : %-29s║\n", WiFi.localIP().toString().c_str());
  Serial.printf ("║  Gateway     : %-29s║\n", WiFi.gatewayIP().toString().c_str());
  Serial.printf ("║  Subnet Mask : %-29s║\n", WiFi.subnetMask().toString().c_str());
  Serial.printf ("║  DNS Server  : %-29s║\n", WiFi.dnsIP().toString().c_str());
  Serial.printf ("║  MAC Address : %-29s║\n", WiFi.macAddress().c_str());
  Serial.printf ("║  RSSI        : %-3d dBm                       ║\n", WiFi.RSSI());
  Serial.printf ("║  Channel     : %-29d║\n", WiFi.channel());
  Serial.printf ("║  BSSID (AP)  : %-29s║\n", WiFi.BSSIDstr().c_str());
  Serial.println("╚══════════════════════════════════════════════╝\n");
}

// ─────────────────────────────────────────────────────────────────
// Fungsi: Konversi RSSI ke string kualitas sinyal
// ─────────────────────────────────────────────────────────────────
const char* rssiToQuality(int32_t rssi) {
  if (rssi > -50)  return "Luar Biasa 🌟";
  if (rssi > -60)  return "Sangat Baik ✅";
  if (rssi > -70)  return "Baik 👍";
  if (rssi > -80)  return "Lemah ⚠️";
  return "Sangat Lemah ❌";
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== BAB 42 Program 1: Hello WiFi ===");
  
  // WAJIB: Nyalakan WiFi terlebih dahulu sebelum menarik MAC Address
  // Jika tidak, MAC Address akan mengembalikan nilai dummy (00:00:00:00)
  WiFi.mode(WIFI_STA); 
  
  Serial.printf("[SYS] MAC Address ESP32: %s\n", WiFi.macAddress().c_str());

  if (connectWiFi()) {
    printNetworkInfo();
  } else {
    Serial.println("[SYS] Sistem berjalan tanpa koneksi internet.");
  }
}

void loop() {
  unsigned long now = millis();

  // Cek status koneksi & tampilkan RSSI berkala
  if (now - tRssi >= RSSI_INTERVAL_MS) {
    tRssi = now;
    if (WiFi.isConnected()) {
      int32_t rssi = WiFi.RSSI();
      Serial.printf("[WiFi] Terhubung | IP: %s | RSSI: %d dBm (%s)\n",
        WiFi.localIP().toString().c_str(), rssi, rssiToQuality(rssi));
    } else {
      Serial.println("[WiFi] ⚠️  Koneksi terputus! Menunggu auto-reconnect...");
    }
  }
}
```

**Output contoh:**
```text
=== BAB 42 Program 1: Hello WiFi ===
[SYS] MAC Address ESP32: A4:CF:12:78:3B:0E
[WiFi] Percobaan 1/3 — Menghubungkan ke 'RumahKu-5G'...
..........
[WiFi] ✅ TERHUBUNG!

╔══════════════════════════════════════════════╗
║           Informasi Jaringan WiFi            ║
╠══════════════════════════════════════════════╣
║  SSID        : RumahKu-5G                   ║
║  IP Address  : 192.168.1.105                ║
║  Gateway     : 192.168.1.1                  ║
║  Subnet Mask : 255.255.255.0                ║
║  DNS Server  : 192.168.1.1                  ║
║  MAC Address : A4:CF:12:78:3B:0E            ║
║  RSSI        : -58 dBm                      ║
║  Channel     : 6                            ║
║  BSSID (AP)  : 14:91:82:40:AA:C3            ║
╚══════════════════════════════════════════════╝

[WiFi] Terhubung | IP: 192.168.1.105 | RSSI: -58 dBm (Baik 👍)
[WiFi] Terhubung | IP: 192.168.1.105 | RSSI: -57 dBm (Sangat Baik ✅)
```

> 💡 **Mengapa `WiFi.disconnect(true)` bukan hanya `disconnect()`?** Parameter `true` pada `disconnect(true)` memaksa driver internal ESP32 membersihkan state WiFi (clear SSID & password yang tersimpan di RAM driver). Ini mencegah percobaan ulang menggunakan kredensial sesi sebelumnya yang sudah *stale*.

---

## 42.5 Program 2: WiFi Manager dengan NVS — Kredensial Persisten

Program ini adalah implementasi **WiFi Manager sederhana** yang menyimpan SSID dan password ke NVS. Pengguna bisa mengubah kredensial via Serial tanpa harus upload ulang kode.

```cpp
/*
 * BAB 42 - Program 2: WiFi + NVS Credentials Manager
 *
 * Fitur:
 *   ▶ Simpan SSID & Password ke NVS (tetap tersimpan saat mati)
 *   ▶ Baca kredensial dari NVS saat boot
 *   ▶ CLI Serial: SET ssid / SET pass / CONNECT / STATUS / FORGET / HELP
 *   ▶ Reconnect otomatis jika koneksi putus (event-driven)
 *   ▶ LED indikator status (IO2 = LED builtin ESP32)
 *
 * Cara pertama kali:
 *   1. Upload program
 *   2. Buka Serial Monitor 115200
 *   3. Ketik: SET ssid NamaWiFiKamu
 *   4. Ketik: SET pass PasswordWiFiKamu
 *   5. Ketik: CONNECT
 */

#include <WiFi.h>
#include <Preferences.h>

// ── Pin ──────────────────────────────────────────────────────────
#define LED_PIN     2      // LED builtin ESP32 DEVKIT V1

// ── Konstanta ─────────────────────────────────────────────────────
const uint32_t WIFI_TIMEOUT_MS  = 20000UL;
const uint32_t STATUS_INTERVAL  = 10000UL;

// ── State ─────────────────────────────────────────────────────────
volatile bool wifiConnected    = false; // Harus volatile karena diakses oleh WiFi Event task
volatile bool manualDisconnect = false; // Penanda apakah WiFi diputus manual oleh user
unsigned long tStatus  = 0;

// ── NVS & Credentials ─────────────────────────────────────────────
Preferences prefs;
String      savedSSID = "";
String      savedPass = "";

// ── Serial CLI ───────────────────────────────────────────────────
String   serialBuf = "";

// ─────────────────────────────────────────────────────────────────
// Fungsi: Muat kredensial dari NVS
// ─────────────────────────────────────────────────────────────────
void loadCredentials() {
  prefs.begin("wifi-cfg", true); // Buka namespace "wifi-cfg" mode READ-ONLY
  savedSSID = prefs.getString("ssid", "");
  savedPass = prefs.getString("pass", "");
  prefs.end();

  if (savedSSID.length() > 0) {
    Serial.printf("[NVS] Kredensial ditemukan: SSID='%s', Pass='%s'\n",
      savedSSID.c_str(),
      String(savedPass).replace(savedPass, String(savedPass.length(), '*')).c_str());
  } else {
    Serial.println("[NVS] Belum ada kredensial tersimpan.");
    Serial.println("[NVS] Ketik 'SET ssid <nama>' dan 'SET pass <password>'");
  }
}

// ─────────────────────────────────────────────────────────────────
// Fungsi: Simpan kredensial ke NVS
// ─────────────────────────────────────────────────────────────────
void saveCredentials(const String& ssid, const String& pass) {
  prefs.begin("wifi-cfg", false); // Buka mode READ-WRITE
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
  Serial.printf("[NVS] Tersimpan: SSID='%s'\n", ssid.c_str());
}

// ─────────────────────────────────────────────────────────────────
// Fungsi: Hapus kredensial dari NVS
// ─────────────────────────────────────────────────────────────────
void forgetCredentials() {
  prefs.begin("wifi-cfg", false);
  prefs.clear();
  prefs.end();
  savedSSID = "";
  savedPass = "";
  manualDisconnect = true; // Mencegah pesan log palsu di event handler
  WiFi.disconnect(true);
  wifiConnected = false;
  digitalWrite(LED_PIN, LOW);
  Serial.println("[NVS] Kredensial dihapus. Ketik 'SET ssid' dan 'SET pass' untuk set ulang.");
}

// ─────────────────────────────────────────────────────────────────
// Fungsi: Koneksi WiFi menggunakan kredensial tersimpan
// ─────────────────────────────────────────────────────────────────
bool connectWiFi() {
  if (savedSSID.length() == 0) {
    Serial.println("[WiFi] ❌ Belum ada SSID tersimpan. Ketik: SET ssid <nama>");
    return false;
  }

  Serial.printf("[WiFi] Menghubungkan ke '%s'...\n", savedSSID.c_str());
  manualDisconnect = false; // Reset status manual
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setHostname("bluino-42");
  WiFi.begin(savedSSID.c_str(), savedPass.c_str());

  unsigned long tStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - tStart >= WIFI_TIMEOUT_MS) {
      Serial.println("\n[WiFi] ❌ Timeout koneksi awal (20 detik).");

      wl_status_t st = WiFi.status();
      if      (st == WL_NO_SSID_AVAIL)  Serial.println("[WiFi] SSID tidak ditemukan di sekitar.");
      else if (st == WL_CONNECT_FAILED) Serial.println("[WiFi] Password salah.");
      else Serial.printf("[WiFi] Status error: %d\n", (int)st);

      Serial.println("[WiFi] Driver akan terus mencoba menyambung ulang di latar belakang (Auto-Reconnect)...");
      // JANGAN panggil WiFi.disconnect() di sini! 
      // Memanggilnya akan mematikan fitur Auto-Reconnect dan menghapus kredensial driver.
      return false;
    }
    Serial.print(".");
    delay(500);
    yield();
  }

  Serial.println();
  Serial.printf("[WiFi] ✅ Terhubung! IP: %s | RSSI: %d dBm\n",
    WiFi.localIP().toString().c_str(), WiFi.RSSI());
  wifiConnected = true;
  digitalWrite(LED_PIN, HIGH);
  return true;
}

// ─────────────────────────────────────────────────────────────────
// WiFi Event Handler (Event-Driven — non-blocking!)
// Dipanggil otomatis oleh driver WiFi di background
// ─────────────────────────────────────────────────────────────────
void onWiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_STA_GOT_IP:
      wifiConnected = true;
      digitalWrite(LED_PIN, HIGH);
      Serial.printf("\n[EVENT] ✅ IP Diterima: %s\n", WiFi.localIP().toString().c_str());
      break;

    case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
      wifiConnected = false;
      digitalWrite(LED_PIN, LOW);
      if (manualDisconnect) {
        Serial.println("\n[EVENT] ⚠️  WiFi dihentikan sesuai perintah user.");
      } else {
        Serial.println("\n[EVENT] ⚠️  Terputus dari WiFi (Sinyal/Router). Auto-reconnect aktif...");
      }
      break;

    case ARDUINO_EVENT_WIFI_STA_CONNECTED:
      manualDisconnect = false; // Reset proteksi log jika berhasil konek
      Serial.println("\n[EVENT] Terhubung ke AP, menunggu IP dari DHCP...");
      break;

    default:
      break;
  }
}

// ─────────────────────────────────────────────────────────────────
// Serial CLI: Tampilkan status lengkap
// ─────────────────────────────────────────────────────────────────
void printStatus() {
  Serial.println("\n┌─────────────────────────────────────────────┐");
  Serial.println("│              Status WiFi Manager             │");
  Serial.println("├─────────────────────────────────────────────┤");
  Serial.printf ("│  SSID (NVS)  : %-28s│\n", savedSSID.length() > 0 ? savedSSID.c_str() : "(kosong)");
  if (WiFi.isConnected()) {
    Serial.printf("│  Status      : %-28s│\n", "TERHUBUNG ✅");
    Serial.printf("│  IP          : %-28s│\n", WiFi.localIP().toString().c_str());
    Serial.printf("│  RSSI        : %-3d dBm                      │\n", WiFi.RSSI());
    Serial.printf("│  Channel     : %-28d│\n", WiFi.channel());
    Serial.printf("│  Gateway     : %-28s│\n", WiFi.gatewayIP().toString().c_str());
  } else {
    Serial.printf("│  Status      : %-28s│\n", "TERPUTUS ❌");
  }
  Serial.printf ("│  MAC Address : %-28s│\n", WiFi.macAddress().c_str());
  Serial.println("└─────────────────────────────────────────────┘\n");
}

// ─────────────────────────────────────────────────────────────────
// Serial CLI Handler
// ─────────────────────────────────────────────────────────────────
void handleCmd(const String& cmd) {
  if (cmd == "STATUS" || cmd == "status") {
    printStatus();

  } else if (cmd == "CONNECT" || cmd == "connect") {
    connectWiFi();

  } else if (cmd == "DISCONNECT" || cmd == "disconnect") {
    manualDisconnect = true; // Tandai agar event handler tahu ini disengaja
    WiFi.disconnect(true);
    wifiConnected = false;
    digitalWrite(LED_PIN, LOW);
    Serial.println("[WiFi] Koneksi diputus secara manual.");

  } else if (cmd == "FORGET" || cmd == "forget") {
    forgetCredentials();

  } else if (cmd.startsWith("SET ") || cmd.startsWith("set ")) {
    String rest = cmd.substring(4);
    int sp = rest.indexOf(' ');
    if (sp < 0) {
      Serial.println("[ERR] Format: SET ssid <nama> atau SET pass <password>");
      return;
    }
    String key = rest.substring(0, sp);
    String val = rest.substring(sp + 1);
    key.trim(); val.trim();

    if (key.equalsIgnoreCase("ssid")) {
      if (val.length() < 1 || val.length() > 32) {
        Serial.println("[ERR] SSID harus 1-32 karakter."); return;
      }
      savedSSID = val;
      prefs.begin("wifi-cfg", false);
      prefs.putString("ssid", savedSSID);
      prefs.end();
      Serial.printf("[OK] SSID disimpan: '%s'. Ketik CONNECT untuk menghubungkan.\n", savedSSID.c_str());

    } else if (key.equalsIgnoreCase("pass")) {
      if (val.length() > 64) {
        Serial.println("[ERR] Password maksimal 64 karakter."); return;
      }
      savedPass = val;
      prefs.begin("wifi-cfg", false);
      prefs.putString("pass", savedPass);
      prefs.end();
      Serial.printf("[OK] Password disimpan (%u karakter).\n", val.length());

    } else {
      Serial.printf("[ERR] Key tidak dikenal: '%s'. Gunakan 'ssid' atau 'pass'.\n", key.c_str());
    }

  } else if (cmd == "HELP" || cmd == "help") {
    Serial.println("\n══ Perintah WiFi Manager ══════════════════════");
    Serial.println("  SET ssid <nama>   — Simpan nama WiFi ke NVS");
    Serial.println("  SET pass <pass>   — Simpan password ke NVS");
    Serial.println("  CONNECT           — Hubungkan menggunakan kredensial tersimpan");
    Serial.println("  DISCONNECT        — Putuskan koneksi WiFi");
    Serial.println("  STATUS            — Tampilkan status & info jaringan");
    Serial.println("  FORGET            — Hapus kredensial dari NVS");
    Serial.println("  HELP              — Tampilkan panduan ini");
    Serial.println("══════════════════════════════════════════════\n");

  } else if (cmd.length() > 0) {
    Serial.printf("[ERR] Perintah tidak dikenal: '%s'. Ketik HELP.\n", cmd.c_str());
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  delay(500);

  Serial.println("\n=== BAB 42 Program 2: WiFi NVS Manager ===");
  Serial.println("Ketik HELP untuk panduan perintah.\n");

  // Inisialisasi awal Driver WiFi agar pembacaan seperti macAddress() tidak eror
  WiFi.mode(WIFI_STA);

  // Daftarkan event handler SEBELUM WiFi.begin()
  WiFi.onEvent(onWiFiEvent);

  // Muat kredensial tersimpan
  loadCredentials();

  // Otomatis coba konek jika ada kredensial
  if (savedSSID.length() > 0) {
    connectWiFi();
  }
}

void loop() {
  // ── Serial CLI ──────────────────────────────────────────
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      String cmd = serialBuf;
      cmd.trim();
      serialBuf = "";
      handleCmd(cmd);
    } else {
      if (serialBuf.length() < 64) serialBuf += c; // Buffer OOM Protection
    }
  }

  // ── Tampilkan status periodik ───────────────────────────
  unsigned long now = millis();
  if (now - tStatus >= STATUS_INTERVAL) {
    tStatus = now;
    if (wifiConnected) {
      Serial.printf("[Heartbeat] IP: %s | RSSI: %d dBm | Uptime: %lu s\n",
        WiFi.localIP().toString().c_str(), WiFi.RSSI(), millis() / 1000UL);
    }
  }
}
```

> ⚠️ **Keamanan Penyimpanan Kredensial:** NVS menggunakan proteksi *namespace* dan flash yang terenkripsi secara partisi. Password yang tersimpan tidak tampil di `Serial.print` — artinya **cukup aman untuk kit edukasi**. Untuk produksi, gunakan **Flash Encryption ESP32** (BAB 71).

---

## 42.6 Program 3: WiFi Scanner — Deteksi Jaringan Sekitar

```cpp
/*
 * BAB 42 - Program 3: WiFi Network Scanner
 *
 * Fitur:
 *   ▶ Scan semua jaringan WiFi 2.4 GHz di sekitar
 *   ▶ Tampilkan SSID, RSSI, Channel, dan Enkripsi
 *   ▶ Scan ulang otomatis setiap 30 detik
 *   ▶ Pindai secara asinkron (non-blocking) agar loop tetap responsif
 *   ▶ CLI: SCAN (paksa pindai sekarang)
 */

#include <WiFi.h>

// ── Konstanta ─────────────────────────────────────────────────────
const uint32_t SCAN_INTERVAL_MS = 30000UL; // Auto-scan setiap 30 detik

// ── State ─────────────────────────────────────────────────────────
unsigned long tScan      = 0;
bool          scanActive = false;
String        serialBuf  = "";

// ─────────────────────────────────────────────────────────────────
// Fungsi: Mulai scan asinkron (non-blocking)
// ─────────────────────────────────────────────────────────────────
void startScan() {
  Serial.println("\n[SCAN] Memulai pindai jaringan WiFi...");
  WiFi.scanNetworks(true); // true = asinkron, tidak memblokir loop
  scanActive = true;
  tScan = millis();
}

// ─────────────────────────────────────────────────────────────────
// Konversi tipe enkripsi ke string
// ─────────────────────────────────────────────────────────────────
const char* encryptionStr(wifi_auth_mode_t enc) {
  switch (enc) {
    case WIFI_AUTH_OPEN:         return "OPEN    ";
    case WIFI_AUTH_WEP:          return "WEP     ";
    case WIFI_AUTH_WPA_PSK:      return "WPA     ";
    case WIFI_AUTH_WPA2_PSK:     return "WPA2    ";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2";
    case WIFI_AUTH_WPA3_PSK:     return "WPA3    ";
    default:                     return "UNKNOWN ";
  }
}

// ─────────────────────────────────────────────────────────────────
// Fungsi: Tampilkan hasil scan
// ─────────────────────────────────────────────────────────────────
void printScanResult() {
  int n = WiFi.scanComplete();
  if (n == WIFI_SCAN_RUNNING) return;  // Masih berjalan
  if (n == WIFI_SCAN_FAILED || n < 0) {
    Serial.println("[SCAN] Gagal pindai. Coba ulang nanti.");
    scanActive = false;
    WiFi.scanDelete();
    return;
  }
  
  // Guard krusial keamanan memori: Mencegah deklarasi Array Panjang 0 (int idx[0])
  if (n == 0) {
    Serial.println("[SCAN] Ditemukan 0 jaringan di sekitar (Gelap sinyal).");
    scanActive = false;
    WiFi.scanDelete();
    return;
  }

  Serial.printf("[SCAN] Ditemukan %d jaringan:\n\n", n);
  Serial.println("  No  SSID                       RSSI   Ch  Enkripsi");
  Serial.println("  ──  ─────────────────────────  ─────  ──  ────────");

  // Buat array index terurut (Insertion Sort) berdasarkan RSSI
  // Kita menggunakan array index agar tidak perlu memodifikasi objek scan asli
  int idx[n];
  for (int i = 0; i < n; i++) idx[i] = i;
  for (int i = 1; i < n; i++) {
    int key = idx[i];
    int j = i - 1;
    while (j >= 0 && WiFi.RSSI(idx[j]) < WiFi.RSSI(key)) {
      idx[j + 1] = idx[j];
      j--;
    }
    idx[j + 1] = key;
  }

  for (int i = 0; i < n; i++) {
    int id = idx[i];
    bool isHidden = (WiFi.SSID(id).length() == 0);
    Serial.printf("  %-3d %-27s  %4d dBm  %2d  %s%s\n",
      i + 1,
      isHidden ? "(tersembunyi)" : WiFi.SSID(id).c_str(),
      WiFi.RSSI(id),
      WiFi.channel(id),
      encryptionStr(WiFi.encryptionType(id)),
      WiFi.encryptionType(id) == WIFI_AUTH_OPEN ? " ⚠️ Terbuka!" : "");
    yield(); // Yield agar WDT tidak trip saat banyak AP
  }

  Serial.println();
  WiFi.scanDelete(); // WAJIB: Bebaskan memori heap hasil scan
  scanActive = false;
  Serial.printf("[SCAN] Scan berikutnya dalam %lu detik.\n", SCAN_INTERVAL_MS / 1000UL);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== BAB 42 Program 3: WiFi Scanner ===");
  Serial.println("Perintah: SCAN (paksa pindai sekarang)\n");

  WiFi.mode(WIFI_STA);  // Mode STA agar bisa scan tanpa AP aktif
  WiFi.disconnect();    // Pastikan tidak ada koneksi sebelumnya
  delay(100);

  startScan(); // Langsung scan saat pertama kali
}

void loop() {
  // ── Serial CLI ──────────────────────────────────────────
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      String cmd = serialBuf;
      cmd.trim();
      serialBuf = "";
      if (cmd.equalsIgnoreCase("SCAN") || cmd.equalsIgnoreCase("scan")) {
        if (!scanActive) startScan();
        else Serial.println("[SCAN] Pindai sedang berjalan...");
      } else if (cmd.length() > 0) {
        Serial.println("[ERR] Perintah: SCAN");
      }
    } else {
      if (serialBuf.length() < 64) serialBuf += c;
    }
  }

  // ── Cek hasil scan asinkron ─────────────────────────────
  if (scanActive) {
    printScanResult(); // Akan return cepat jika belum selesai
  }

  // ── Auto-scan berkala ───────────────────────────────────
  if (!scanActive && (millis() - tScan >= SCAN_INTERVAL_MS)) {
    startScan();
  }
}
```

**Output contoh:**
```text
=== BAB 42 Program 3: WiFi Scanner ===
Perintah: SCAN (paksa pindai sekarang)

[SCAN] Memulai pindai jaringan WiFi...
[SCAN] Ditemukan 8 jaringan:

  No  SSID                       RSSI   Ch  Enkripsi
  ──  ─────────────────────────  ─────  ──  ────────
  1   RumahKu-5G                 -47 dBm   6  WPA2
  2   Indihome_2G                -54 dBm   1  WPA/WPA2
  3   ICONNET_PLAZA              -61 dBm  11  WPA2
  4   Wifi-Tetangga              -68 dBm   6  WPA2
  5   MyRepublic_Office          -71 dBm   3  WPA2
  6   (tersembunyi)              -75 dBm   8  WPA2
  7   FREE-WIFI                  -78 dBm  13  OPEN     ⚠️ Terbuka!
  8   AndroidAP-XM               -82 dBm   7  WPA2

[SCAN] Scan berikutnya dalam 30 detik.
```

> ⚠️ **Keamanan Jaringan:** Jika kamu melihat jaringan berlabel `OPEN` (tanpa enkripsi), hindari penggunaannya untuk komunikasi yang mengandung data sensitif. Semua data yang dikirim/diterima dari jaringan terbuka dapat disadap!

---

## 42.7 Troubleshooting WiFi

```
MASALAH UMUM DAN SOLUSINYA:

┌──────────────────────────────────────────────────────────────────────┐
│ MASALAH 1: WiFi.status() terus WL_NO_SSID_AVAIL                     │
├──────────────────────────────────────────────────────────────────────┤
│ Penyebab: SSID salah, atau sinyal SSID terlalu lemah untuk terdeteksi│
│ Solusi:                                                              │
│  ✓ Pastikan SSID persis sama (case-sensitive: "WiFi" ≠ "wifi")      │
│  ✓ ESP32 hanya support WiFi 2.4 GHz — jika router 5 GHz only, buat  │
│    SSID terpisah untuk 2.4 GHz atau aktifkan dual-band               │
│  ✓ Jalankan Program 3 (Scanner) untuk memverifikasi SSID terdeteksi  │
└──────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────┐
│ MASALAH 2: WiFi.status() terus WL_CONNECT_FAILED                    │
├──────────────────────────────────────────────────────────────────────┤
│ Penyebab: Password salah, atau MAC address diblokir router           │
│ Solusi:                                                              │
│  ✓ Cek ulang password — perhatikan huruf besar/kecil dan spasi      │
│  ✓ Coba akses router (192.168.1.1), pastikan MAC ESP32 tidak diblokir│
│  ✓ Sementara, coba nonaktifkan MAC Filtering di router               │
└──────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────┐
│ MASALAH 3: Terhubung tapi IP tidak valid / 0.0.0.0                  │
├──────────────────────────────────────────────────────────────────────┤
│ Penyebab: DHCP server router lambat atau penuh (batas client)        │
│ Solusi:                                                              │
│  ✓ Tunggu 3-5 detik setelah WL_CONNECTED sebelum baca localIP()     │
│  ✓ Restart router jika terlalu banyak device terhubung              │
│  ✓ Gunakan IP statis: WiFi.config(ip, gateway, subnet)              │
└──────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────┐
│ MASALAH 4: WiFi sering putus sendiri                                │
├──────────────────────────────────────────────────────────────────────┤
│ Penyebab: Sinyal lemah, interferensi, atau power supply tidak stabil │
│ Solusi:                                                              │
│  ✓ Dekatkan ESP32 ke router, cek RSSI harus > -70 dBm              │
│  ✓ Pastikan catu daya ESP32 stabil (gunakan adaptor 5V/2A min)      │
│  ✓ Hindari channel yang sama dengan WiFi tetangga (gunakan scanner) │
│  ✓ Aktifkan setAutoReconnect(true) dan tangani event DISCONNECTED   │
└──────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────┐
│ MASALAH 5: ESP32 tidak konek ke WiFi 5 GHz                         │
├──────────────────────────────────────────────────────────────────────┤
│ Penyebab: Hardware WiFi ESP32 hanya mendukung 2.4 GHz              │
│ Solusi:                                                              │
│  ✓ Aktifkan SSID 2.4 GHz terpisah di router (contoh: "RumahKu_2G") │
│  ✓ Atau aktifkan fitur "Smart Connect" dimatikan agar bisa pilih    │
│    band secara manual                                                │
└──────────────────────────────────────────────────────────────────────┘
```

---

## 42.8 Ringkasan & Best Practices

```
PRINSIP CONNECTED DEVICE YANG TANGGUH:

  1. SELALU Tentukan Timeout Koneksi
     ┌──────────────────────────────────────────────────────────┐
     │  ❌ BURUK: while(WiFi.status() != WL_CONNECTED) delay(500);│
     │  ✅ BAIK:  Tambahkan batas waktu (15-30 detik)            │
     └──────────────────────────────────────────────────────────┘

  2. SELALU Tangani Disconnect
     ┌──────────────────────────────────────────────────────────┐
     │  Gunakan WiFiEvent atau cek WiFi.isConnected() di loop  │
     │  Jangan asumsi WiFi selalu terhubung setelah boot       │
     └──────────────────────────────────────────────────────────┘

  3. JANGAN Simpan Credentials di Kode (Hardcode)
     ┌──────────────────────────────────────────────────────────┐
     │  ❌ BURUK: const char* pass = "PasswordAsli123";         │
     │  ✅ BAIK:  Simpan di NVS via Serial CLI (Program 2)      │
     └──────────────────────────────────────────────────────────┘

  4. SELALU Bebaskan Memori Scan
     ┌──────────────────────────────────────────────────────────┐
     │  WiFi.scanDelete() WAJIB dipanggil setelah scan selesai │
     │  Tanpa ini, heap akan bocor setiap siklus scan          │
     └──────────────────────────────────────────────────────────┘

  5. Desain untuk Offline-First
     ┌──────────────────────────────────────────────────────────┐
     │  Kode harus tetap berjalan (baca sensor, simpan log)    │
     │  meskipun WiFi tidak tersedia atau sedang reconnect     │
     └──────────────────────────────────────────────────────────┘

ANTIPATTERN KLASIK:
  ❌ Menggunakan delay() panjang saat menunggu koneksi (blocking loop)
  ❌ Tidak memeriksa WiFi.status() sebelum mengirim data
  ❌ Lupa WiFi.scanDelete() → heap leak meledak saat loop scan
  ❌ Menyimpan SSID/Pass di flash dengan FILE_WRITE tanpa enkripsi
  ❌ Hardcode IP statis tanpa fallback DHCP
```

---

## 42.9 Latihan

**Latihan 1 — Dasar:** Modifikasi Program 1 untuk menampilkan notifikasi berbeda di Serial berdasarkan kualitas RSSI: "Sinyal optimal", "Sinyal baik", "Sinyal lemah", "Ganti posisi!".

**Latihan 2 — Menengah:** Tambahkan fitur **IP Statis** ke Program 2. Buat perintah CLI `SET ip <192.168.1.x>` yang menyimpan IP statis ke NVS dan menggunakannya saat `CONNECT` (gunakan `WiFi.config()`).

**Latihan 3 — Menengah:** Modifikasi Program 3 untuk menampilkan grafik kekuatan sinyal menggunakan karakter ASCII (`█`) berdasarkan nilai RSSI. Semakin kuat sinyal, semakin panjang batang grafik.

**Latihan 4 — Lanjutan:** Gabungkan Program 2 (WiFi Manager) dengan BAB 40 (LittleFS). Simpan log koneksi ke file `/wifi_log.txt`: setiap koneksi dan disconnect dicatat dengan timestamp `millis()`. Tampilkan log via perintah CLI `LOG`.

**Latihan 5 — Mahir (Proyek):** Bangun sistem **WiFi Watchdog**. Catat waktu koneksi (`millis()`), hitung durasi setiap sesi koneksi, dan simpan statistik (total koneksi, rata-rata durasi, jumlah disconnect) ke NVS. Tampilkan statistik via perintah `STATS`.

---

## 42.10 Apa Selanjutnya?

| BAB | Topik | Keterkaitan |
|-----|-------|-------------|
| **BAB 43** | WiFi Access Point | ESP32 sebagai hotspot mandiri |
| **BAB 44** | WiFiManager & Captive Portal | Konfigurasi WiFi tanpa kode hardcode |
| **BAB 46** | Web Server | Sajikan halaman web dari ESP32 |
| **BAB 48** | HTTP Client & REST API | Kirim data ke server cloud |
| **BAB 50** | MQTT | Protokol IoT untuk real-time messaging |
