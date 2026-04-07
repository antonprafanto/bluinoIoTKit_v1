# BAB 44: WiFiManager & Captive Portal — Konfigurasi WiFi via Browser

> ✅ **Prasyarat:** Selesaikan **BAB 42 (WiFi Station Mode)** dan **BAB 43 (WiFi Access Point)**. Pemahaman `WiFi.softAP()`, `WiFi.begin()`, dan mode `WIFI_AP_STA` wajib dikuasai.

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami **mengapa _hardcoding_ kredensial WiFi adalah anti-pattern** dalam produk IoT profesional.
- Menjelaskan mekanisme teknis di balik **Captive Portal (DNS Redirect Trick)**.
- Menggunakan **library WiFiManager (tzapu)** untuk konfigurasi WiFi otomatis tanpa menyentuh kode.
- Membangun **DIY Captive Portal** dari nol menggunakan `DNSServer`, `WebServer`, dan `Preferences` (NVS).
- Menambahkan **parameter kustom** (MQTT broker, API key, dsb.) ke dalam portal konfigurasi.

---

## 44.1 Masalah: Mengapa JANGAN Hardcode Kredensial WiFi?

```
SKENARIO NYAMAN (TAPI BERBAHAYA):

  Versi 1 Produk IoT Anda:
  ┌──────────────────────────────────────────────────────────────┐
  │  const char* SSID = "WiFi_Rumah_Anton";                      │
  │  const char* PASS = "passwordRahasia123";                    │
  └──────────────────────────────────────────────────────────────┘

  Apa yang terjadi jika:
  ❌ Pelanggan Anda pindah rumah → Router baru         → Produk MATI PERMANEN
  ❌ Password router diganti    → Beli router baru     → Produk MATI PERMANEN
  ❌ Dijual 100 unit ke 100 orang berbeda              → Harus Flash 100 kali
  ❌ SSID/Password bocor di source code GitHub         → KEAMANAN JEBOL

SOLUSI PROFESIONAL: WiFiManager & Captive Portal

  Alur Kerja Produk IoT yang Benar:
  
  ┌────────────────────────────────────────────────────────────────┐
  │  BOOT ESP32                                                    │
  │     │                                                          │
  │     ▼                                                          │
  │  Ada kredensial WiFi di NVS?                                   │
  │     │                                                          │
  │  YA ─────────────► Coba koneksi ke WiFi                       │
  │                         │                                      │
  │                    Berhasil? ──YA──► Mode Normal (IoT)        │
  │                         │                                      │
  │                    TIDAK/Timeout                               │
  │                         │                                      │
  │  TIDAK ──────────────────┘                                     │
  │     │                                                          │
  │     ▼                                                          │
  │  Buka AP + Captive Portal                                     │
  │     │                                                          │
  │     ▼                                                          │
  │  User konek ke AP di HP → browser OTOMATIS terbuka            │
  │     │                                                          │
  │     ▼                                                          │
  │  User pilih SSID + ketik password → Submit                    │
  │     │                                                          │
  │     ▼                                                          │
  │  Simpan ke NVS → Restart → Koneksi berhasil                   │
  └────────────────────────────────────────────────────────────────┘
```

---

## 44.2 Cara Kerja Captive Portal: "Trik DNS Redirect"

```
MENGAPA BROWSER BISA TERBUKA OTOMATIS?

  Saat HP terhubung ke WiFi baru, sistem operasi (iOS/Android/Windows)
  secara otomatis mengirimkan permintaan HTTP ke URL "Captive Portal Check":
  
  iOS    : http://captive.apple.com/hotspot-detect.html
  Android: http://connectivitycheck.gstatic.com/generate_204
  Windows: http://www.msftconnecttest.com/connecttest.txt
  
  Jika respons TIDAK sesuai (mis. ESP32 merespons apapun selain isi aslinya),
  OS akan menganggap ada "Captive Portal" dan OTOMATIS membuka browser!

  MEKANISME 3 LAPIS:
  
  Lapisan 1 — DNS Server (Port 53):
  ┌─────────────────────────────────────────────────────────────────┐
  │  HP → "Berapa IP dari google.com?"                              │
  │  DNSServer → "IP-nya adalah 10.10.10.1" (selalu!)              │
  │                                                                 │
  │  DNS Server menangkap SEMUA permintaan domain apapun           │
  │  dan menjawabnya dengan IP AP milik ESP32 kita.                 │
  │  Inilah "Wildcard DNS" atau "DNS Poison for Good".             │
  └─────────────────────────────────────────────────────────────────┘

  Lapisan 2 — Web Server (Port 80):
  ┌─────────────────────────────────────────────────────────────────┐
  │  Browser → HTTP GET ke "10.10.10.1" (hasil DNS redirect)       │
  │  WebServer → Kirim halaman HTML Form konfigurasi WiFi          │
  └─────────────────────────────────────────────────────────────────┘

  Lapisan 3 — Respons 302 Redirect (Portabilitas):
  ┌─────────────────────────────────────────────────────────────────┐
  │  Setiap request ke path apapun (mis. /favicon.ico, /style.css) │
  │  diarahkan ulang (302 Found) ke halaman utama portal.           │
  │  Ini memastikan browser apapun berakhir di halaman form kita.  │
  └─────────────────────────────────────────────────────────────────┘

  ARSITEKTUR KOMPONEN:
  
  ┌───────────────────────────────┐
  │  ESP32 (Mode WIFI_AP_STA)     │
  │  ┌─────────────────────────┐ │
  │  │  DNSServer (Port 53)    │ │ ← Menangkap semua DNS → IP ESP32
  │  │  WebServer (Port 80)    │ │ ← Menyajikan form HTML
  │  │  Preferences (NVS)      │ │ ← Menyimpan SSID + Password
  │  │  WiFi.begin() [STA]     │ │ ← Mencoba koneksi ke router
  │  └─────────────────────────┘ │
  └───────────────────────────────┘
```

---

## 44.3 Instalasi Library

### Metode 1: WiFiManager by tzapu (Program 1 & 3)
`Arduino IDE → Tools → Manage Libraries → Cari "WiFiManager" → Install by tzapu`

> ⚠️ **Pastikan versi ≥ 2.0.16-rc.2**. Versi lama memiliki bug kompatibilitas ESP32 Arduino Core 3.x

### Metode 2: Library Bawaan (Program 2 — DIY)
Program 2 hanya membutuhkan library yang sudah **terinstal bawaan** di ESP32 Arduino Core:
```
WiFi.h         → Bawaan ESP32 Arduino Core
WebServer.h    → Bawaan ESP32 Arduino Core  
DNSServer.h    → Bawaan ESP32 Arduino Core
Preferences.h  → Bawaan ESP32 Arduino Core
```

---

## 44.4 Program 1: WiFiManager Sederhana

```cpp
/*
 * BAB 44 - Program 1: WiFiManager Sederhana (Library tzapu)
 *
 * Alur kerja:
 *   ▶ Saat boot, coba terhubung ke WiFi tersimpan secara otomatis
 *   ▶ Jika gagal / belum ada, buka Captive Portal AP di "Bluino-Config"
 *   ▶ Hubungkan HP ke AP → Browser OTOMATIS membuka portal konfigurasi
 *   ▶ Pilih SSID + masukkan password → Simpan & Restart
 *   ▶ Setelah tersambung, masuk ke mode normal dan cetak IP
 *
 * Library: WiFiManager by tzapu (versi >= 2.0.16-rc.2)
 *
 * Cara Reset (hapus WiFi tersimpan):
 *   → Tekan tombol BOOT (GPIO 0) selama 3 detik saat startup
 *
 * ⚠️ ESP32 hanya mendukung WiFi 2.4 GHz!
 */

#include <WiFiManager.h>

// ── Pin & Konstanta ──────────────────────────────────────────────
#define LED_PIN     2    // LED builtin ESP32 DevKit V1
#define RESET_PIN   0    // Tombol BOOT ESP32 DevKit V1 (active-LOW)

const char* PORTAL_SSID = "Bluino-Config";
const char* PORTAL_PASS = "bluino123";   // Min 8 karakter
const uint32_t CONFIG_PORTAL_TIMEOUT = 180; // Detik sampai portal tutup

// ── State ────────────────────────────────────────────────────────
WiFiManager wm;
volatile bool wifiConnected = false;

// ─────────────────────────────────────────────────────────────────
// Callback: Dipanggil saat Portal konfigurasi dibuka
// ─────────────────────────────────────────────────────────────────
void onPortalStarted(WiFiManager* myWiFiManager) {
  Serial.println("\n[PORTAL] ▶ Portal Konfigurasi dibuka!");
  Serial.printf ("[PORTAL] Hubungkan HP ke SSID : %s\n", PORTAL_SSID);
  Serial.printf ("[PORTAL] Password             : %s\n", PORTAL_PASS);
  Serial.printf ("[PORTAL] Buka browser         : http://10.10.10.1\n");
  Serial.printf ("[PORTAL] Portal tutup otomatis: %u detik\n\n", CONFIG_PORTAL_TIMEOUT);

  // LED blink cepat = portal aktif
  for (int i = 0; i < 6; i++) {
    digitalWrite(LED_PIN, i % 2 == 0 ? HIGH : LOW);
    delay(150);
  }
  digitalWrite(LED_PIN, HIGH); // LED solid saat portal aktif
}

// ─────────────────────────────────────────────────────────────────
// Callback: Dipanggil saat user menekan tombol "Save" di portal
// ─────────────────────────────────────────────────────────────────
void onSaveConfig() {
  Serial.println("\n[PORTAL] Kredensial WiFi disimpan.");
  Serial.println("[PORTAL] Mencoba menghubungkan ke router...");
}

// ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  pinMode(RESET_PIN, INPUT_PULLUP); // Tombol BOOT: LOW saat ditekan
  digitalWrite(LED_PIN, HIGH); // LED nyala saat proses awal
  delay(500);

  Serial.println("\n=== BAB 44 Program 1: WiFiManager Sederhana ===");

  // ── Jendela Keamanan Reset (Aman dari Download Mode) ──────────
  // GPIO 0 tidak boleh ditahan saat boot fisik karena bisa tersangkut mode ROM.
  // Oleh karena itu, kita membuat jendela aksi 3 detik setelah ESP32 hidup.
  Serial.println("[INFO] Tekan & Tahan tombol BOOT sekarang untuk Factory Reset (Tunggu 3s)...");
  uint32_t waitStart = millis();
  while (millis() - waitStart < 3000) {
    if (digitalRead(RESET_PIN) == LOW) {
      Serial.println("\n[RESET] Menghapus data WiFi tersimpan...");
      wm.resetSettings();
      Serial.println("[RESET] Selesai. Rebooting...");
      delay(1000);
      ESP.restart();
    }
    delay(50);
  }

  // ── Konfigurasi WiFiManager ─────────────────────────────────────
  wm.setAPCallback(onPortalStarted);
  wm.setSaveConfigCallback(onSaveConfig);

  // IP kustom AP Portal: kelas-A untuk menghindari konflik subnet
  wm.setAPStaticIPConfig(
    IPAddress(10, 10, 10, 1),  // IP AP
    IPAddress(10, 10, 10, 1),  // Gateway
    IPAddress(255, 255, 255, 0) // Subnet
  );

  wm.setConfigPortalTimeout(CONFIG_PORTAL_TIMEOUT);
  wm.setTitle("Bluino IoT Kit — Konfigurasi WiFi");

  // Tampilkan tombol "Info" di portal agar siswa bisa melihat status
  wm.setShowInfoErase(true);

  // ── Upaya Koneksi Utama ─────────────────────────────────────────
  Serial.println("[WiFiManager] Mencoba koneksi ke WiFi tersimpan...");
  bool res = wm.autoConnect(PORTAL_SSID, PORTAL_PASS);

  if (!res) {
    // Portal timeout tanpa konfigurasi apapun — restart dan coba lagi
    Serial.println("[ERR] Konfigurasi portal timeout. Restart...");
    delay(3000);
    ESP.restart();
  }

  // Sampai sini = WiFi berhasil tersambung
  wifiConnected = true;
  Serial.println("\n[INFO] WiFiManager: Mode normal aktif!");
  Serial.printf("[WiFi] ✅ Terhubung! IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("[WiFi] SSID          : %s\n", WiFi.SSID().c_str());
  Serial.printf("[WiFi] RSSI (Sinyal) : %d dBm\n", WiFi.RSSI());
  
  digitalWrite(LED_PIN, LOW); // LED Mati = Mode Operasi Normal
}

void loop() {
  static unsigned long tHb = 0;
  if (wifiConnected && millis() - tHb >= 10000UL) {
    tHb = millis();
    Serial.printf("[Heartbeat] IP: %s | RSSI: %d dBm | Uptime: %lu s\n",
      WiFi.localIP().toString().c_str(), WiFi.RSSI(), millis() / 1000UL);
  }
}
```

**Tampilan Portal di Browser (otomatis terbuka di HP):**
```
╔══════════════════════════════════════════╗
║  Bluino IoT Kit — Konfigurasi WiFi       ║
╠══════════════════════════════════════════╣
║  [Configure WiFi]                        ║
║  [Configure WiFi (No Scan)]              ║
║  [Info]                                  ║
║  [Erase WiFi Config]                     ║
║  [Restart]                               ║
╚══════════════════════════════════════════╝
```

---

## 44.5 Program 2: DIY Captive Portal dari Nol

Program ini membangun Captive Portal **tanpa library eksternal apapun**, hanya menggunakan library bawaan ESP32. Ini adalah cara terbaik untuk memahami cara kerja WiFiManager secara mendalam.

```cpp
/*
 * BAB 44 - Program 2: DIY Captive Portal Manual
 *
 * Library yang Digunakan (SEMUANYA bawaan ESP32 Arduino Core):
 *   - WiFi.h      : Manajemen koneksi WiFi
 *   - WebServer.h : HTTP Server untuk sajikan HTML
 *   - DNSServer.h : DNS Server palsu (redirect semua domain ke ESP32)
 *   - Preferences.h : NVS untuk simpan/baca kredensial
 *
 * Alur Kerja:
 *   Boot → Cek NVS → Coba konek STA → Gagal → Buka AP + DNS + WebServer
 *   → User submit form → Simpan ke NVS → Restart → Konek berhasil
 *
 * Cara Reset: Tekan GPIO 0 (tombol BOOT) selama > 3 detik saat boot.
 *
 * ⚠️ ESP32 hanya mendukung WiFi 2.4 GHz!
 */

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <Preferences.h>

// ── Pin ──────────────────────────────────────────────────────────
#define LED_PIN     2
#define RESET_PIN   0

// ── AP Captive Portal ─────────────────────────────────────────────
const char*     PORTAL_SSID    = "Bluino-Setup";
const char*     PORTAL_PASS    = "bluino123";
// IP Kelas-A: hindari konflik dengan router rumah tangga (192.168.x.x)
const IPAddress PORTAL_IP      (10, 10, 10, 1);
const IPAddress PORTAL_GATEWAY (10, 10, 10, 1);
const IPAddress PORTAL_SUBNET  (255, 255, 255, 0);
const uint32_t  STA_TIMEOUT_MS = 15000UL;
const uint8_t   DNS_PORT       = 53;

// ── Objek Utama & Global ──────────────────────────────────────────
WebServer   webServer(80);
DNSServer   dnsServer;
Preferences prefs;
String      portalPage; // Cache HTML agar tidak blocking saat diakses HP

// ── State (volatile: diakses oleh WiFi Event Task) ────────────────
volatile bool staConnected   = false;
volatile bool configMode     = false;
volatile bool restartPending = false;

// ─────────────────────────────────────────────────────────────────
// HTML: Halaman Konfigurasi Portal
// ─────────────────────────────────────────────────────────────────
// Fungsi untuk menghasilkan HTML form dengan daftar jaringan WiFi
String buildPortalHTML(int scanCount) {
  String html = R"rawhtml(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>Bluino IoT — Konfigurasi WiFi</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body { font-family: Arial, sans-serif; background: #1a1a2e;
           color: #eee; min-height: 100vh; display: flex;
           align-items: center; justify-content: center; padding: 20px; }
    .card { background: #16213e; border-radius: 12px; padding: 28px;
            width: 100%; max-width: 400px; box-shadow: 0 8px 32px rgba(0,0,0,0.4); }
    h2 { color: #4fc3f7; margin-bottom: 6px; font-size: 1.3rem; }
    p  { color: #aaa; font-size: 0.85rem; margin-bottom: 20px; }
    label { display: block; margin-bottom: 5px; color: #ccc; font-size: 0.9rem; }
    select, input[type=password], input[type=text] {
      width: 100%; padding: 10px 12px; border-radius: 8px;
      border: 1px solid #0f3460; background: #0f3460; color: #eee;
      font-size: 0.95rem; margin-bottom: 16px; outline: none; }
    select:focus, input:focus { border-color: #4fc3f7; }
    .toggle { cursor: pointer; background: none; border: none;
              color: #4fc3f7; font-size: 0.8rem; float: right; margin-top: -46px; }
    button[type=submit] {
      width: 100%; padding: 12px; border-radius: 8px; border: none;
      background: linear-gradient(135deg, #4fc3f7, #0288d1);
      color: #fff; font-size: 1rem; font-weight: bold; cursor: pointer; }
    button[type=submit]:hover { opacity: 0.9; }
    .info { font-size: 0.78rem; color: #888; text-align: center; margin-top: 14px; }
    .badge { display: inline-block; padding: 2px 8px; border-radius: 4px;
             font-size: 0.7rem; margin-left: 4px; vertical-align: middle; }
    .wpa2 { background: #1b5e20; color: #a5d6a7; }
    .open { background: #4a1000; color: #ffab91; }
    .rssi-good   { color: #81c784; }
    .rssi-fair   { color: #ffb74d; }
    .rssi-weak   { color: #ef9a9a; }
  </style>
</head>
<body>
<div class="card">
  <h2>📶 Bluino IoT Kit</h2>
  <p>Hubungkan perangkat ke jaringan WiFi rumahmu</p>
  <form method="POST" action="/save">
    <label for="ssid">Pilih Jaringan WiFi</label>
    <select name="ssid" id="ssid" required>
)rawhtml";

  // Pre-allocation Memori C++:
  // Pesan alokasi SRAM di awal (Ukuran Dasar HTML + (Perkiraan Ukuran Tiap WiFi * Jumlah Scan)).
  // Ini 100% membinasakan potensi Crash/Memory Fragmentation akibat pengocokan memori Looping String!!
  html.reserve(html.length() + (scanCount * 120) + 1000);

  // Render opsi jaringan dari hasil Scan
  if (scanCount == 0) {
    html += "      <option value=\"\">-- SSID tidak ditemukan --</option>\n";
  } else {
    html += "      <option value=\"\">-- Pilih Jaringan (" + String(scanCount) + " Ditemukan) --</option>\n";
  }

  for (int i = 0; i < scanCount; i++) {
    String enc   = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "open" : "wpa2";
    String encLbl = (enc == "open") ? "OPEN" : "WPA2";
    int    rssi  = WiFi.RSSI(i);
    String icon  = (rssi >= -65) ? "🟢" : (rssi >= -80) ? "🟡" : "🔴";
    
    // HTML-escape mutlak untuk SSID yang mengandung karakter khusus
    String ssidEsc = WiFi.SSID(i);
    ssidEsc.replace("&", "&amp;");
    ssidEsc.replace("\"", "&quot;");
    ssidEsc.replace("<", "&lt;");
    ssidEsc.replace(">", "&gt;");

    html += "      <option value=\"" + ssidEsc + "\">";
    html += icon + " " + ssidEsc + " [" + encLbl + ", " + String(rssi) + " dBm]";
    html += "</option>\n";
  }

  html += R"rawhtml(
    </select>
    <label for="pass">Password WiFi</label>
    <input type="password" name="pass" id="pass" placeholder="Minimal 8 karakter (kosong = OPEN)" maxlength="63">
    <button type="button" class="toggle" onclick="
      var p=document.getElementById('pass');
      p.type=(p.type==='password')?'text':'password';">Tampilkan</button>
    <br>
    <button type="submit">Sambungkan &amp; Simpan</button>
  </form>
  <div class="info">
    ESP32 Bluino IoT Kit &bull; Hanya mendukung WiFi 2.4 GHz
  </div>
</div>
</body>
</html>
)rawhtml";

  return html;
}

// ─────────────────────────────────────────────────────────────────
// HTML: Halaman Sukses setelah submit
// ─────────────────────────────────────────────────────────────────
const char* HTML_SUCCESS = R"rawhtml(
<!DOCTYPE html><html lang="id"><head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>Tersimpan!</title>
<style>
  body { background:#1a1a2e; color:#eee; font-family:Arial,sans-serif;
         display:flex; align-items:center; justify-content:center; height:100vh; }
  .card { background:#16213e; border-radius:12px; padding:32px;
          text-align:center; max-width:340px; }
  h2 { color:#81c784; font-size:1.4rem; margin-bottom:12px; }
  p  { color:#aaa; font-size:0.9rem; }
</style>
</head>
<body><div class="card">
  <h2>✅ Konfigurasi Disimpan!</h2>
  <p>ESP32 akan restart dan mencoba terhubung.<br><br>
  Jika berhasil, hotspot <strong>Bluino-Setup</strong> akan hilang.<br><br>
  Jika gagal (SSID/password salah), hotspot akan muncul kembali.</p>
</div></body></html>
)rawhtml";

// ─────────────────────────────────────────────────────────────────
// Fungsi: Muat Kredensial dari NVS
// ─────────────────────────────────────────────────────────────────
bool loadCredentials(String& ssid, String& pass) {
  prefs.begin("wifi-cfg", true);
  ssid = prefs.getString("ssid", "");
  pass = prefs.getString("pass", "");
  prefs.end();
  return ssid.length() > 0;
}

// ─────────────────────────────────────────────────────────────────
// Fungsi: Simpan Kredensial ke NVS
// ─────────────────────────────────────────────────────────────────
void saveCredentials(const String& ssid, const String& pass) {
  prefs.begin("wifi-cfg", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
  Serial.printf("[NVS] Disimpan — SSID: '%s'\n", ssid.c_str());
}

// ─────────────────────────────────────────────────────────────────
// Fungsi: Hapus Kredensial dari NVS
// ─────────────────────────────────────────────────────────────────
void clearCredentials() {
  prefs.begin("wifi-cfg", false);
  prefs.clear();
  prefs.end();
  Serial.println("[NVS] Semua data WiFi dihapus.");
}

// ─────────────────────────────────────────────────────────────────
// WiFi Event Handler
// ─────────────────────────────────────────────────────────────────
void onWiFiEvent(WiFiEvent_t e, WiFiEventInfo_t info) {
  if (e == ARDUINO_EVENT_WIFI_STA_GOT_IP) {
    staConnected = true;
    Serial.printf("[STA] ✅ Terhubung! IP: %s\n",
      WiFi.localIP().toString().c_str());
  } else if (e == ARDUINO_EVENT_WIFI_STA_DISCONNECTED && !configMode) {
    staConnected = false;
    Serial.println("[STA] ⚠️  Terputus dari WiFi. Auto-reconnect...");
  }
}

// ─────────────────────────────────────────────────────────────────
// Handler HTTP: GET / — Halaman Portal Utama
// ─────────────────────────────────────────────────────────────────
void handleRoot() {
  Serial.printf("[HTTP] GET %s dari %s\n",
    webServer.uri().c_str(), webServer.client().remoteIP().toString().c_str());

  // Kirim halaman yang sudah di-render secara statis
  webServer.send(200, "text/html", portalPage);
}

// ─────────────────────────────────────────────────────────────────
// Handler HTTP: POST /save — Simpan Kredensial & Restart
// ─────────────────────────────────────────────────────────────────
void handleSave() {
  String ssid = webServer.arg("ssid");
  String pass = webServer.arg("pass");

  // JANGAN di-trim()! SSID dan Password WPA2 secara sah boleh
  // memiliki karakter Spasi di awal atau di akhir string.
  
  Serial.printf("[HTTP] POST /save — SSID: '%s'\n", ssid.c_str());

  // Validasi Input
  if (ssid.length() == 0 || ssid.length() > 32) {
    webServer.send(400, "text/html", "<script>alert('Error: SSID tidak valid (1-32 karakter).');history.back();</script>");
    return;
  }
  if (pass.length() > 0 && pass.length() < 8) {
    webServer.send(400, "text/html", "<script>alert('Error: Password minimal 8 karakter!');history.back();</script>");
    return;
  }
  if (pass.length() > 63) {
    webServer.send(400, "text/html", "<script>alert('Error: Password WPA2 maksimal 63 karakter ASCII!');history.back();</script>");
    return;
  }

  // Simpan ke NVS
  saveCredentials(ssid, pass);

  // Tampilkan halaman sukses
  webServer.send(200, "text/html", HTML_SUCCESS);

  // Tandai untuk restart setelah respons sukses terkirim
  restartPending = true;
}

// Handler catch-all: redirect semua path ke halaman utama
void handleNotFound() {
  webServer.sendHeader("Location", "http://10.10.10.1/", true);
  webServer.send(302, "text/plain", "Redirect ke portal...");
}

// ─────────────────────────────────────────────────────────────────
// Fungsi: Buka Mode Config (AP + DNS + WebServer)
// ─────────────────────────────────────────────────────────────────
void startConfigPortal() {
  configMode = true;
  Serial.println("\n[PORTAL] Membuka Captive Portal...");
  
  // WAJIB: Hentikan 'channel hopping' dari STA yang mencari sinyal.
  // Jika STA memaksa mencari sinyal di background, antarmuka Radio
  // akan lompat-lompat channel dan memutuskan koneksi HP ke Portal!
  WiFi.setAutoReconnect(false);
  WiFi.disconnect();
  
  // WAJIB menggunakan WIFI_AP_STA jika kita ingin melakukan WiFi.scanNetworks() di bawah!
  // Karena proses pemindaian gelombang radio (Scanning) membutuhkan interface Station beroperasi.
  WiFi.mode(WIFI_AP_STA);

  // softAPsetHostname WAJIB sebelum softAPConfig
  WiFi.softAPsetHostname("bluino-setup");

  // softAPConfig WAJIB sebelum softAP
  if (!WiFi.softAPConfig(PORTAL_IP, PORTAL_GATEWAY, PORTAL_SUBNET)) {
    Serial.println("[ERR] Konfigurasi IP AP gagal!");
    return;
  }

  // Validasi password saat buat AP Portal
  const char* apPass = (strlen(PORTAL_PASS) >= 8) ? PORTAL_PASS : nullptr;
  if (!WiFi.softAP(PORTAL_SSID, apPass)) {
    Serial.println("[ERR] Gagal membuat AP Portal!");
    return;
  }

  Serial.printf("[AP] SSID   : %s\n", PORTAL_SSID);
  Serial.printf("[AP] IP     : %s\n", WiFi.softAPIP().toString().c_str());
  Serial.printf("[AP] Pass   : %s\n", PORTAL_PASS);

  // PRE-RENDER HTML: Scan WiFi 1x saja sebelum Web Server naik!
  // Krusial agar HP pendeteksi Captive Portal tidak Timeout (Lag)
  Serial.println("[PORTAL] Melakukan scan WiFi...");
  int n = WiFi.scanNetworks();
  portalPage = buildPortalHTML(n >= 0 ? n : 0);
  WiFi.scanDelete();

  // DNS Server: tangkap SEMUA domain dan arahkan ke IP kita
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);
  dnsServer.start(DNS_PORT, "*", PORTAL_IP); // "*" = wildcard domain

  // Web Server: Daftarkan route & Shortcut Probe OS Mobile
  // Menghapus '302 Redirection Hop' untuk Apple/Android Probe agar login page muncul secara instan!
  webServer.on("/",      HTTP_GET,  handleRoot);
  webServer.on("/generate_204", HTTP_GET, handleRoot);       // By-pass latensi Probe Android
  webServer.on("/hotspot-detect.html", HTTP_GET, handleRoot);// By-pass latensi Probe Apple iOS
  webServer.on("/save",  HTTP_POST, handleSave);
  webServer.onNotFound(handleNotFound);
  webServer.begin();

  Serial.println("[PORTAL] Siap! Hubungkan HP ke WiFi \"" +
    String(PORTAL_SSID) + "\" maka browser akan terbuka otomatis.");

  // LED blink cepat = portal aktif
  digitalWrite(LED_PIN, HIGH);
}

// ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN,   OUTPUT);
  pinMode(RESET_PIN, INPUT_PULLUP);
  digitalWrite(LED_PIN, LOW);
  delay(500);

  Serial.println("\n=== BAB 44 Program 2: DIY Captive Portal ===");

  // Daftarkan Event Handler SEDINI mungkin & SEKALI saja agar tidak duplikat
  WiFi.onEvent(onWiFiEvent);

  // ── Jendela Keamanan Reset (3 Detik Awal) ─────────────────────
  Serial.println("[INFO] Tekan tombol BOOT sekarang memperbarui/reset (Tunggu 3s)...");
  uint32_t bootWait = millis();
  while (millis() - bootWait < 3000) {
    if (digitalRead(RESET_PIN) == LOW) {
      Serial.println("\n[RESET] Menghapus memori...");
      clearCredentials();
      Serial.println("[RESET] Selesai. Rebooting...");
      delay(1000);
      ESP.restart();
    }
    delay(50);
  }

  // ── Muat Kredensial dari NVS ──────────────────────────────────
  String savedSSID, savedPass;
  bool hasCreds = loadCredentials(savedSSID, savedPass);

  if (!hasCreds) {
    Serial.println("[NVS] Tidak ada kredensial. Buka portal.");
    startConfigPortal();
    return;
  }

  // ── Coba Koneksi STA ──────────────────────────────────────────
  Serial.printf("[STA] Mencoba koneksi ke '%s'...\n", savedSSID.c_str());

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setHostname("bluino-iot");
  WiFi.begin(savedSSID.c_str(), savedPass.length() > 0 ? savedPass.c_str() : nullptr);

  uint32_t tStart = millis();
  while (!staConnected) {
    if (millis() - tStart >= STA_TIMEOUT_MS) {
      Serial.println("[STA] ⚠️  Timeout koneksi. Buka portal...");
      // Pindah arus kerja ke Portal (akan menonaktifkan auto-reconnect di dalam fungsi)
      startConfigPortal();
      return;
    }
    Serial.print(".");
    delay(500);
    yield();
  }
  Serial.println();

  Serial.println("[INFO] Mode Normal aktif! Siap untuk proyek IoT.");
  Serial.printf ("[INFO] Ketik alamat http://%s di browser!\n",
    WiFi.localIP().toString().c_str());
}

void loop() {
  // ── Pelayan DNS & Web (aktif hanya di Config Mode) ────────────
  if (configMode) {
    dnsServer.processNextRequest();
    webServer.handleClient();

    // LED blink = portal sedang aktif
    static unsigned long tBlink = 0;
    static bool ledState = false;
    if (millis() - tBlink >= 300UL) {
      tBlink = millis();
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState ? HIGH : LOW);
    }

    // Restart setelah respons sukses HTTP terkirim
    if (restartPending) {
      Serial.println("[PORTAL] Restart dalam 2 detik...");
      delay(2000);
      ESP.restart();
    }

    // WAJIB: Timeout Portal (Autonomous Recovery Loop)
    // Jika Router rumah mati akibat pemadaman listrik, ESP32 akan menyala lebih cepat
    // dari Router dan masuk ke sini. Jika kita biarkan selamanya di mode AP, 
    // ESP32 takkan pernah nyambung lagi ke WiFi!. Kita paksa Restart tiap 3 Menit!
    static uint32_t portalStartTime = 0;
    if (portalStartTime == 0) portalStartTime = millis();
    
    if (millis() - portalStartTime >= 180000UL) { // 180 Detik (3 Menit)
      Serial.println("[PORTAL] ⚠️ Waktu habis (3 Menit)! Restart mencari Router...");
      delay(1000);
      ESP.restart();
    }
  }

  // ── Mode Normal: Heartbeat ────────────────────────────────────
  static unsigned long tHb = 0;
  if (!configMode && staConnected && millis() - tHb >= 10000UL) {
    tHb = millis();
    Serial.printf("[Heartbeat] IP: %s | RSSI: %d dBm | Uptime: %lu s\n",
      WiFi.localIP().toString().c_str(), WiFi.RSSI(), millis() / 1000UL);
  }
}
```

---

## 44.6 Program 3: WiFiManager Lanjutan — Parameter Kustom

Program ini mendemonstrasikan kemampuan WiFiManager untuk menyavariabel konfigurasi selain SSID/Password, seperti **alamat server MQTT**, **nama perangkat**, atau **API Key Cloud**.

```cpp
/*
 * BAB 44 - Program 3: WiFiManager Lanjutan dengan Parameter Kustom
 *
 * Fitur Tambahan vs Program 1:
 *   ▶ Parameter kustom: Nama Perangkat, Server MQTT, Port MQTT
 *   ▶ Parameter disimpan di NVS (Preferences) secara terpisah
 *   ▶ Callback update otomatis saat user simpan konfigurasi baru
 *   ▶ Tombol "Buka Portal" manual (tanpa perlu reset seluruh data)
 *
 * Library: WiFiManager by tzapu (versi >= 2.0.16-rc.2)
 */

#include <WiFiManager.h>
#include <Preferences.h>

// ── Pin ──────────────────────────────────────────────────────────
#define LED_PIN       2
#define RESET_PIN     0    // Tahan 3 detik: hapus semua config (Tombol BOOT On-Board)
#define PORTAL_PIN    4    // Tekan: buka portal manual (Gunakan Push Button Kit Bluino)

// ── Konstanta ─────────────────────────────────────────────────────
const char* PORTAL_SSID = "Bluino-Config";
const char* PORTAL_PASS = "bluino123";
const uint32_t PORTAL_TIMEOUT = 180;

// ── Konfigurasi Kustom (disimpan di NVS) ─────────────────────────
struct DeviceConfig {
  String deviceName = "bluino-device-01";
  String mqttServer = "broker.hivemq.com";
  String mqttPort   = "1883";
} devCfg;

Preferences prefs;

// ── WiFiManager dan Parameter Kustom ─────────────────────────────
WiFiManager wm;
// Deklarasi parameter: id, label, default value, panjang max
WiFiManagerParameter wmDeviceName("dname",  "Nama Perangkat",  "", 32);
WiFiManagerParameter wmMqttServer("mserver", "Server MQTT",    "", 40);
WiFiManagerParameter wmMqttPort  ("mport",   "Port MQTT",      "", 6);

// ─────────────────────────────────────────────────────────────────
// Fungsi: Muat konfigurasi kustom dari NVS
// ─────────────────────────────────────────────────────────────────
void loadDeviceConfig() {
  prefs.begin("dev-cfg", true);
  devCfg.deviceName = prefs.getString("dname",   devCfg.deviceName);
  devCfg.mqttServer = prefs.getString("mserver", devCfg.mqttServer);
  devCfg.mqttPort   = prefs.getString("mport",   devCfg.mqttPort);
  prefs.end();

  // Validasi batas nilai (Fallback perlindungan)
  if (devCfg.deviceName.length() < 1 || devCfg.deviceName.length() > 32)
    devCfg.deviceName = "bluino-device-01";
    
  if (devCfg.mqttServer.length() < 3)
    devCfg.mqttServer = "broker.hivemq.com";
    
  if (devCfg.mqttPort.toInt() < 1 || devCfg.mqttPort.toInt() > 65535)
    devCfg.mqttPort = "1883";

  Serial.printf("[NVS] Config: nama='%s' mqtt='%s:%s'\n",
    devCfg.deviceName.c_str(), devCfg.mqttServer.c_str(), devCfg.mqttPort.c_str());
}

// ─────────────────────────────────────────────────────────────────
// Fungsi: Simpan konfigurasi kustom dari parameter WiFiManager ke NVS
// ─────────────────────────────────────────────────────────────────
void saveDeviceConfig() {
  // Ambil nilai dari parameter WiFiManager
  String newName   = String(wmDeviceName.getValue()); newName.trim();
  String newServer = String(wmMqttServer.getValue()); newServer.trim();
  String newPort   = String(wmMqttPort.getValue());   newPort.trim();

  // Auto-Sanitasi Domain Hostname (Spasi dilarang di aturan IETF RFC-952)
  if (newName.length()   > 0 && newName.length()   <= 32) {
    newName.replace(" ", "-"); // Spasi wajib diubah jadi Strip!
    devCfg.deviceName = newName;
  }
  
  if (newServer.length() > 0 && newServer.length() <= 40) devCfg.mqttServer = newServer;
  if (newPort.length()   > 0 && newPort.toInt() > 0)      devCfg.mqttPort   = newPort;

  prefs.begin("dev-cfg", false);
  prefs.putString("dname",   devCfg.deviceName);
  prefs.putString("mserver", devCfg.mqttServer);
  prefs.putString("mport",   devCfg.mqttPort);
  prefs.end();

  Serial.printf("[NVS] Config disimpan: '%s' | mqtt: %s:%s\n",
    devCfg.deviceName.c_str(), devCfg.mqttServer.c_str(), devCfg.mqttPort.c_str());
}

// ─────────────────────────────────────────────────────────────────
// Callback: Dipanggil SEBELUM portal ditutup (data disubmit user)
// ─────────────────────────────────────────────────────────────────
void onSaveConfig() {
  Serial.println("\n[PORTAL] Tombol 'Save' ditekan! Menyimpan parameter...");
  saveDeviceConfig();
}

// Callback: Portal terbuka
void onPortalOpened(WiFiManager* myWm) {
  Serial.println("\n[PORTAL] Portal Konfigurasi dibuka!");
  Serial.printf ("[PORTAL] SSID: %s | Pass: %s\n", PORTAL_SSID, PORTAL_PASS);
  Serial.printf ("[PORTAL] IP  : http://10.10.10.1\n\n");
  digitalWrite(LED_PIN, HIGH);
}

// ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN,    OUTPUT);
  pinMode(RESET_PIN,  INPUT_PULLUP);
  pinMode(PORTAL_PIN, INPUT);        // Sesuaikan hardware: Push Button Bluino Kit menggunakan Pull-Down (Active-HIGH)
  digitalWrite(LED_PIN, LOW);
  delay(500);

  Serial.println("\n=== BAB 44 Program 3: WiFiManager + Parameter Kustom ===");

  // ── Jendela Keamanan Reset (3 Detik Awal) ─────────────────────
  Serial.println("[INFO] Tekan tombol BOOT sekarang untuk Factory Reset (Tunggu 3s)...");
  uint32_t bootWait = millis();
  while (millis() - bootWait < 3000) {
    if (digitalRead(RESET_PIN) == LOW) {
      Serial.println("\n[RESET] Menghapus data WiFi + Konfigurasi Alat...");
      wm.resetSettings();
      prefs.begin("dev-cfg", false); prefs.clear(); prefs.end();
      Serial.println("[RESET] Selesai. Rebooting...");
      delay(1000);
      ESP.restart();
    }
    delay(50);
  }

  // ── Muat Konfigurasi Kustom dari NVS ─────────────────────────
  loadDeviceConfig();

  // WAJIB: Suntikkan nilai NVS ke Objek UI WiFiManager agar Field Web 
  // Portal tidak selalu "kosong" saat diakses lewat HP siswa!
  wmDeviceName.setValue(devCfg.deviceName.c_str(), 32);
  wmMqttServer.setValue(devCfg.mqttServer.c_str(), 40);
  wmMqttPort.setValue(devCfg.mqttPort.c_str(), 6);

  // ── Konfigurasi WiFiManager ── ────────────────────────────────
  wm.addParameter(&wmDeviceName);
  wm.addParameter(&wmMqttServer);
  wm.addParameter(&wmMqttPort);

  wm.setSaveConfigCallback(onSaveConfig);
  wm.setAPCallback(onPortalOpened);

  // IP AP Portal: kelas-A (anti-konflik subnet)
  wm.setAPStaticIPConfig(
    IPAddress(10, 10, 10, 1),
    IPAddress(10, 10, 10, 1),
    IPAddress(255, 255, 255, 0)
  );

  wm.setConfigPortalTimeout(PORTAL_TIMEOUT);
  wm.setTitle("Bluino IoT — Setup Perangkat");
  wm.setShowInfoErase(true);

  // WAJIB panggil setHostname SEBELUM autoConnect agar Router DHCP 
  // merekam nama perangkat dengan benar (bukan 'espressif' bawaan)
  WiFi.setHostname(devCfg.deviceName.c_str());

  // ── Upaya Koneksi / Portal ────────────────────────────────────
  bool connected = wm.autoConnect(PORTAL_SSID, PORTAL_PASS);

  if (!connected) {
    Serial.println("[ERR] Timeout portal. Restart...");
    delay(3000);
    ESP.restart();
  }

  // ── Mode Normal ───────────────────────────────────────────────
  Serial.println("\n[INFO] Koneksi berhasil! Konfigurasi aktif:");
  Serial.printf ("[INFO]   Nama Perangkat : %s\n",   devCfg.deviceName.c_str());
  Serial.printf ("[INFO]   IP ESP32       : %s\n",   WiFi.localIP().toString().c_str());
  Serial.printf ("[INFO]   Server MQTT    : %s:%s\n", devCfg.mqttServer.c_str(), devCfg.mqttPort.c_str());
  Serial.printf ("[INFO]   RSSI           : %d dBm\n", WiFi.RSSI());
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  // ── Buka Portal Manual lewat GPIO 4 (Push Button external) ───
  static bool portalOpened = false;
  if (!portalOpened && digitalRead(PORTAL_PIN) == HIGH) { // Active-HIGH pada Bluino Kit
    delay(50); // Debounce
    if (digitalRead(PORTAL_PIN) == HIGH) {
      portalOpened = true;
      Serial.println("[PORTAL] Membuka portal konfigurasi manual...");
      if (wm.startConfigPortal(PORTAL_SSID, PORTAL_PASS)) {
        // Reboot memaksa parameter & Hostname baru diterapkan penuh
        Serial.println("[PORTAL] Konfigurasi disubmit. Rebooting...");
        delay(1000);
        ESP.restart();
      }
      portalOpened = false;
    }
  }

  static unsigned long tHb = 0;
  if (millis() - tHb >= 10000UL) {
    tHb = millis();
    Serial.printf("[Heartbeat] %s | IP: %s | RSSI: %d dBm | Uptime: %lu s\n",
      devCfg.deviceName.c_str(), WiFi.localIP().toString().c_str(),
      WiFi.RSSI(), millis() / 1000UL);
  }
}
```

---

## 44.7 Troubleshooting Captive Portal

```
MASALAH UMUM DAN SOLUSINYA:

┌──────────────────────────────────────────────────────────────────────┐
│ MASALAH 1: Browser TIDAK terbuka otomatis setelah konek ke AP       │
├──────────────────────────────────────────────────────────────────────┤
│ Penyebab: DNS Server belum terdaftar atau OS tidak mendeteksi portal │
│ Solusi:                                                              │
│  ✓ Buka browser MANUAL dan ketik: http://10.10.10.1                 │
│  ✓ Nonaktifkan data seluler HP (koneksi ganda bisa memblokir)       │
│  ✓ Pastikan dnsServer.start() dipanggil SEBELUM webServer.begin()   │
│  ✓ Gunakan Mode Pesawat dulu, lalu sambung WiFi saja                │
└──────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────┐
│ MASALAH 2: Form HTML muncul, tapi submit gagal / redirect error     │
├──────────────────────────────────────────────────────────────────────┤
│ Penyebab: restartPending langsung memicu ESP.restart() sebelum      │
│ respons HTTP 200 sempat dikirimkan ke browser pengirim              │
│ Solusi:                                                              │
│  ✓ Set restartPending = true SETELAH webServer.send()               │
│  ✓ Beri delay 2000ms sebelum ESP.restart() di dalam loop()          │
└──────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────┐
│ MASALAH 3: WiFi tersimpan tapi ESP terus-terusan buka portal        │
├──────────────────────────────────────────────────────────────────────┤
│ Penyebab: Timeout STA terlalu pendek, atau SSID/password di NVS     │
│ sudah tidak valid (router diganti, password berubah)                │
│ Solusi:                                                              │
│  ✓ Naikkan STA_TIMEOUT_MS (default 15 detik, coba 30 detik)        │
│  ✓ Tekan tombol RESET (GPIO 0) 3 detik → hapus NVS → isi ulang     │
└──────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────┐
│ MASALAH 4: Portal terbuka di iOS tapi TIDAK di Android              │
├──────────────────────────────────────────────────────────────────────┤
│ Penyebab: Android menggunakan URL probe berbeda untuk deteksi portal │
│ (connectivitycheck.gstatic.com) dan lebih sensitif terhadap respons │
│ Solusi:                                                              │
│  ✓ Pastikan handler "/" merespons dengan HTTP 200 (bukan redirect)  │
│  ✓ Tambahkan handler khusus: server.on("/generate_204", handleRoot) │
│  ✓ Library WiFiManager (Program 1) sudah menangani ini otomatis     │
└──────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────┐
│ MASALAH 5: Heap memory rendah / ESP32 crash saat portal aktif       │
├──────────────────────────────────────────────────────────────────────┤
│ Penyebab: HTML string dan scan result di-buffer di SRAM sekaligus   │
│ Solusi:                                                              │
│  ✓ Panggil WiFi.scanDelete() segera setelah buildPortalHTML()       │
│  ✓ Batasi scan: WiFi.scanNetworks(false, false, false, 300, ch, 8)  │
│    → Argumen terakhir "8" = maksimal 8 jaringan ditampilkan         │
│  ✓ Gunakan F() macro: Serial.print(F("teks")); → simpan di Flash    │
└──────────────────────────────────────────────────────────────────────┘
```

---

## 44.8 Ringkasan & Best Practices

```
PRINSIP WIFI MANAGER YANG TANGGUH:

  1. URUTAN INISIALISASI WAJIB (Program 2 — DIY):
     ┌──────────────────────────────────────────────────────────┐
     │  1. WiFi.onEvent(handler)            ← Event dulu       │
     │  2. WiFi.mode(WIFI_AP)               ← Mode AP          │
     │  3. WiFi.softAPsetHostname("nama")   ← Hostname AP      │
     │  4. WiFi.softAPConfig(ip, gw, sub)   ← IP kustom        │
     │  5. WiFi.softAP(ssid, pass)          ← Hidupkan AP      │
     │  6. dnsServer.start(53, "*", ip)     ← DNS Wildcard     │
     │  7. webServer.begin()                ← Web Server       │
     └──────────────────────────────────────────────────────────┘

  2. RESTART HARUS DELAY:
     ┌──────────────────────────────────────────────────────────┐
     │  // ❌ SALAH — restart sebelum HTTP response terkirim    │
     │  webServer.send(200, "text/html", HTML_SUCCESS);         │
     │  ESP.restart(); // Browser dapat status "Connection Reset"│
     │                                                          │
     │  // ✅ BENAR — restart via flag dari loop()             │
     │  webServer.send(200, "text/html", HTML_SUCCESS);         │
     │  restartPending = true; // restart di loop() + delay     │
     └──────────────────────────────────────────────────────────┘

  3. SIMPAN CONFIG DI setup(), BUKAN DI CALLBACK:
     ┌──────────────────────────────────────────────────────────┐
     │  // ❌ SALAH — NVS prefs.begin() dalam callback WiFiMgr  │
     │  void onSaveConfig() { prefs.begin(...); ... }           │
     │                                                          │
     │  // ✅ BENAR — hanya set flag di callback               │
     │  void onSaveConfig() { shouldSaveConfig = true; }        │
     │  ... → if (shouldSaveConfig) { saveDeviceConfig(); }    │
     └──────────────────────────────────────────────────────────┘

  4. SELALU WiFi.scanDelete() PASCA SCAN:
     ┌──────────────────────────────────────────────────────────┐
     │  int n = WiFi.scanNetworks();                            │
     │  buildPortalHTML(n);                                     │
     │  WiFi.scanDelete(); // ← WAJIB! Cegah heap fragmentation │
     └──────────────────────────────────────────────────────────┘

ANTIPATTERN KLASIK WiFiManager:
  ❌ ESP.restart() langsung setelah webServer.send() — respons belum terkirim
  ❌ Operasi NVS di dalam callback WiFiManager — potensi deadlock
  ❌ DNS Server tidak di-process di loop() — portal tidak responsif
  ❌ WiFi.scanNetworks() tanpa WiFi.scanDelete() — heap leak
  ❌ password < 8 karakter pada PORTAL_PASS — AP gagal diam-diam
```

---

## 44.9 Latihan

**Latihan 1 — Dasar:** Modifikasi Program 1: tambahkan tombol (GPIO 35) yang ketika ditekan selama 1 detik, ESP32 langsung memanggil `wm.startConfigPortal()` tanpa perlu restart.

**Latihan 2 — Menengah:** Pada DIY Portal (Program 2), tambahkan halaman **`/status`** yang menampilkan IP, SSID terhubung, RSSI, dan uptime dalam format HTML sederhana. Akses http://10.10.10.1/status dari browser saat portal aktif.

**Latihan 3 — Menengah:** Tambahkan **indikator OLED** (IO21 SDA, IO22 SCL) pada Program 2: tampilkan teks `"Config Mode"` dan `"SSID: Bluino-Setup"` saat portal aktif, dan `"WiFi: [SSID]"` + IP setelah berhasil konek.

**Latihan 4 — Lanjutan:** Ganti dropdown SSID di Program 2 dengan sistem **scan AJAX** (tanpa reload halaman): tambahkan endpoint `GET /scan` yang mengembalikan daftar SSID dalam format JSON, lalu gunakan JavaScript `fetch()` untuk memperbarui dropdown secara dinamis.

**Latihan 5 — Mahir (Proyek):** Gabungkan Program 3 (WiFiManager + Parameter Kustom) dengan BAB 42 Program 3 (WiFi Scanner): saat konfigurasi SSID via portal, tampilkan juga **kekuatan sinyal (RSSI)** untuk setiap SSID dalam bentuk ikon bar (≥ -65dBm: 🔵🔵🔵, ≥ -80dBm: 🔵🔵⚪, lainnya: 🔵⚪⚪).

---

## 44.10 Apa Selanjutnya?

| BAB | Topik | Hubungan dengan BAB ini |
|-----|-------|-------------------------|
| **BAB 45** | mDNS | Akses ESP32 via `bluino.local` — tanpa ingat IP |
| **BAB 46** | Web Server | Bangun dashboard IoT via browser di atas fondasi ini |
| **BAB 47** | WebSocket | Data real-time di halaman web yang sudah kita bangun |
| **BAB 48** | HTTP Client & REST API | ESP32 komunikasi ke cloud setelah tersambung WiFi |
