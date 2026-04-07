# BAB 45: mDNS — Akses ESP32 dengan Nama Domain Lokal

> ✅ **Prasyarat:** Selesaikan **BAB 42 (WiFi Station Mode)** dan **BAB 44 (WiFiManager & Captive Portal)**. ESP32 harus sudah bisa terhubung ke WiFi router dan mendapatkan alamat IP.

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami **masalah fundamental IP dinamis** pada perangkat IoT dalam jaringan lokal.
- Menjelaskan mekanisme teknis **mDNS (Multicast DNS / Zero-configuration Networking)**.
- Menggunakan library `ESPmDNS` bawaan Arduino Core untuk mengakses ESP32 via nama `bluino.local`.
- Menggabungkan mDNS dengan **Service Discovery** agar jaringan dapat menemukan kapabilitas perangkat secara otomatis.
- Membangun sistem **mDNS + Web Server** yang dapat dibuka tanpa mengetahui IP secara langsung.

---

## 45.1 Masalah: "Tadi IP-nya berapa ya?"

```
SKENARIO NYATA DALAM PROYEK IoT:

  Hari Pertama — Setup:
  ┌──────────────────────────────────────────────────────┐
  │  ESP32 terhubung ke WiFi → Router memberi IP:        │
  │  192.168.1.105                                       │
  │                                                      │
  │  Kamu buka browser: http://192.168.1.105 → ✅ Jalan │
  └──────────────────────────────────────────────────────┘

  Hari Kedua — Masalah:
  ┌──────────────────────────────────────────────────────┐
  │  ESP32 restart/listrik mati → Router memberi IP:     │
  │  192.168.1.117  (IP BEDA!)                           │
  │                                                      │
  │  http://192.168.1.105 → ❌ Tidak ditemukan!         │
  │  Harus buka Serial Monitor dulu untuk cari IP baru.  │
  └──────────────────────────────────────────────────────┘

MENGAPA IP BISA BERUBAH?
  
  Router menggunakan protokol DHCP (Dynamic Host Configuration Protocol).
  Setiap perangkat yang join jaringan diberi IP dari "kolam" (pool) yang tersedia.
  IP yang diberikan sifatnya SEMENTARA (lease time biasanya 24 jam).
  Setelah restart, tidak ada jaminan router akan memberi IP yang sama!

SOLUSI 1: Static IP (Pendekatan Primitif)
  → Tetapkan IP tetap di kode ESP32
  → Bekerja, TAPI: Bisa konflik IP dengan perangkat lain
  → Beda subnet di rumah berbeda = harus ganti kode
  → Bukan cara yang elegan dan portabel

SOLUSI 2: mDNS (Cara Modern & Profesional)
  → ESP32 mengumumkan dirinya dengan nama: "bluino.local"
  → Semua perangkat di jaringan bisa menemukannya via nama
  → IP bisa berubah-ubah, nama tetap SAMA selamanya
  → Tidak perlu server DNS pusat
```

---

## 45.2 Apa itu mDNS? Protokol "Nama Lokal" Tanpa Server Terpusat

```
ARSITEKTUR DNS TRADISIONAL (Yang Biasa Dikenal):

  Kamu              Browser           Server DNS           Server Web
  ───              ───────           ──────────           ──────────
   │                  │                   │                    │
   │ Buka "google.com"│                   │                    │
   │─────────────────►│                   │                    │
   │                  │  "google.com ada  │                    │
   │                  │   di IP berapa?"  │                    │
   │                  │──────────────────►│                    │
   │                  │    "216.58.x.x"   │                    │
   │                  │◄──────────────────│                    │
   │                  │  HTTP GET ke 216.58.x.x               │
   │                  │───────────────────────────────────────►│
   │                  │        Halaman Web                      │
   │                  │◄───────────────────────────────────────│

  MASALAH: Butuh server DNS terpusat yang selalu online!
  Di jaringan LOKAL (rumah/lab), tidak ada server publik.


ARSITEKTUR mDNS (Multicast DNS — RFC 6762):

  Kamu              Browser           Jaringan WiFi         ESP32
  ───              ───────           ─────────────         ─────
   │                  │                   │                    │
   │ Buka "bluino.local"                  │                    │
   │─────────────────►│                   │                    │
   │                  │  MULTICAST:       │                    │
   │                  │  "Siapa yang      │                    │
   │                  │   punya nama      │                    │
   │                  │   bluino.local?" ►│────────────────────│
   │                  │                   │   ESP32: "Saya!    │
   │                  │                   │   IP saya:         │
   │                  │                   │◄── 192.168.1.105"  │
   │                  │◄───────────────────────────────────────│
   │                  │  HTTP GET ke 192.168.1.105             │
   │                  │───────────────────────────────────────►│
   │                  │        Halaman Web                      │
   │                  │◄───────────────────────────────────────│

  KEUNGGULAN mDNS:
  ✅ Tidak butuh server DNS terpusat
  ✅ Perangkat saling "mengumumkan" keberadaannya via Multicast
  ✅ IP bisa berubah-ubah, nama tetap sama
  ✅ Zero-configuration: Plug & Play jaringan lokal
  ✅ Terintegrasi di macOS (Bonjour), Linux (Avahi), Windows 10+


KOMPONEN TEKNIS mDNS:

  1. Hostname (Nama Perangkat):
     bluino.local → Resolusi ke IP perangkat
     Format: [nama].local (akhiran .local = mDNS domain)
  
  2. Service Discovery (DNS-SD, RFC 6763):
     _http._tcp → "Perangkat ini punya web server HTTP!"
     _mqtt._tcp → "Perangkat ini bisa dilayani MQTT!"
     Alat seperti nmap, avahi-browse bisa menemukan layanannya.
  
  3. Multicast Address:
     IPv4: 224.0.0.251 (port 5353)
     IPv6: ff02::fb    (port 5353)
     Bukan unicast! Paket dikirim ke SEMUA perangkat di subnet.
```

---

## 45.3 Instalasi Library

mDNS untuk ESP32 sudah tersedia **sebagai bawaan** dari ESP32 Arduino Core. Tidak perlu instalasi library tambahan apapun:

```
ESPmDNS.h    → Sudah bawaan ESP32 Arduino Core (tidak perlu download)
WiFi.h       → Sudah bawaan ESP32 Arduino Core
WebServer.h  → Sudah bawaan ESP32 Arduino Core (dibutuhkan Program 2 & 3)
```

> ⚠️ **Kompatibilitas OS:**
> - **macOS:** Native support via Bonjour (sejak macOS 10.2)
> - **Linux:** Butuh `avahi-daemon` (`sudo apt install avahi-daemon`)
> - **Windows:** Support native sejak Windows 10 Build 1703 (April 2017)
> - **Android:** Chrome 90+ dan beberapa browser mendukung `.local`
> - **iOS/Safari:** Support penuh

---

## 45.4 Program 1: mDNS Minimal — "Nama Mudah untuk ESP32"

Program paling sederhana: daftarkan nama `bluino.local` agar ESP32 bisa ditemukan tanpa harus ingat nomor IP.

```cpp
/*
 * BAB 45 - Program 1: mDNS Minimal
 *
 * Tujuan: Daftarkan mDNS hostname agar ESP32 bisa diakses via "bluino.local"
 *         tanpa perlu mengetahui nomor IP DHCP yang selalu berubah.
 *
 * Cara Uji:
 *   1. Upload program, buka Serial Monitor (115200 baud)
 *   2. Catat IP yang muncul di Serial
 *   3. Buka Terminal / Command Prompt di PC/Mac:
 *        ping bluino.local        (uji resolusi nama)
 *        ping <nomor IP>          (bandingkan: harusnya sama)
 *   4. Di browser: ketik "bluino.local"
 *      PENTING: Browser akan menampilkan "Koneksi Ditolak / Connection Refused".
 *      MENGAPA? Karena mDNS hanya memberikan NAMA. ESP32 kita belum
 *      diajarkan cara melayani halaman Web (materi ini ada di Program 2).
 *   - Satu nama hanya untuk SATU perangkat di satu jaringan
 *   - Jika ada 2 ESP32 dengan nama sama, gunakan nama berbeda
 *     (misal: bluino-sensor.local, bluino-aktuator.local)
 *
 * ⚠️ ESP32 hanya mendukung WiFi 2.4 GHz!
 * ⚠️ Ganti SSID dan PASSWORD sesuai jaringan WiFi kamu!
 */

#include <WiFi.h>
#include <ESPmDNS.h>

// ── Konfigurasi WiFi ──────────────────────────────────────────────
const char* WIFI_SSID = "NamaWiFiKamu";    // ← Ganti!
const char* WIFI_PASS = "PasswordWiFi";    // ← Ganti!

// ── Konfigurasi mDNS ──────────────────────────────────────────────
const char* MDNS_HOSTNAME = "bluino";      // Akan jadi "bluino.local"

// ── Pin ──────────────────────────────────────────────────────────
#define LED_PIN 2

// ── State ─────────────────────────────────────────────────────────
bool mdnsStarted = false;

// ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  delay(500);

  Serial.println("\n=== BAB 45 Program 1: mDNS Minimal ===");

  // ── Koneksi WiFi ──────────────────────────────────────────────
  Serial.printf("[WiFi] Menghubungkan ke '%s'...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(MDNS_HOSTNAME); // DHCP hostname (berbeda tapi terkait mDNS)
  WiFi.setAutoReconnect(true);     // Opsional tapi penting agar bisa pulih
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  uint32_t tStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - tStart >= 15000UL) {
      Serial.println("\n[ERR] Koneksi WiFi timeout. Restart...");
      delay(2000);
      ESP.restart();
    }
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.printf("[WiFi] ✅ Terhubung! IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("[WiFi] SSID         : %s\n", WiFi.SSID().c_str());
  Serial.printf("[WiFi] RSSI         : %d dBm\n", WiFi.RSSI());

  // ── Inisialisasi mDNS ─────────────────────────────────────────
  // WAJIB: mDNS.begin() dipanggil SETELAH WiFi terhubung dan mendapat IP!
  // Jika dipanggil sebelum WiFi siap, mDNS tidak akan berjalan.
  if (MDNS.begin(MDNS_HOSTNAME)) {
    mdnsStarted = true;
    Serial.println("\n[mDNS] ✅ mDNS aktif!");
    Serial.printf("[mDNS] Nama Lokal   : http://%s.local\n", MDNS_HOSTNAME);
    Serial.printf("[mDNS] Alias ke IP  : %s\n", WiFi.localIP().toString().c_str());
    Serial.println("\n[mDNS] 👇 CARA MENGUJI (LAKUKAN 2 HAL INI):");
    Serial.println("[mDNS] 1. Terminal / CMD : ping bluino.local");
    Serial.println("[mDNS] 2. Di Browser Web : http://bluino.local");
    Serial.println("       (Browser AKAN ERROR 'Connection Refused' karena ESP32");
    Serial.println("        hanya memberikan Nama, namun belum memiliki pelayan WebServer)");
  } else {
    Serial.println("[ERR] Gagal memulai mDNS! Cek koneksi WiFi.");
  }

  digitalWrite(LED_PIN, mdnsStarted ? HIGH : LOW);
}

// ─────────────────────────────────────────────────────────────────
void loop() {
  // mDNS library ESP32 menangani permintaan multicast secara internal.
  // Perubahan IP (saat DHCP reconnect) ditangani otomatis oleh 
  // event listener internal ESPmDNS di background tanpa perlu MDNS.update()
  // atau me-restart MDNS secara manual.

  // ── Heartbeat Log (Tiap 15 Detik) ──────────────────────────────
  static unsigned long tHb = 0;
  if (millis() - tHb >= 15000UL) {
    tHb = millis();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("[Heartbeat] IP: %s | RSSI: %d dBm | Uptime: %lu s\n",
        WiFi.localIP().toString().c_str(), WiFi.RSSI(), millis() / 1000UL);
      digitalWrite(LED_PIN, mdnsStarted ? HIGH : LOW);
    } else {
      Serial.println("[WiFi] ⚠️ Menunggu koneksi...");
      digitalWrite(LED_PIN, LOW);
    }
  }
}
```

**Cara Menguji di PC/Mac/Linux:**
```
# Di Terminal / Command Prompt:

ping bluino.local
# Output: Reply from 192.168.1.105 (IP akan menyesuaikan)

# Di browser — cukup ketik:
http://bluino.local
# (Belum ada web server, tapi nama sudah resolve ke IP)

# Di Linux — daftar semua perangkat mDNS di jaringan:
avahi-browse -a -t
```

---

## 45.5 Program 2: mDNS + Service Discovery + Web Server

Program ini menggabungkan mDNS dengan **DNS-SD (Service Discovery)** dan sebuah Web Server sederhana. Membuka `http://bluino.local` dari browser akan langsung menampilkan halaman status ESP32.

```cpp
/*
 * BAB 45 - Program 2: mDNS + Service Discovery + Web Server
 *
 * Fitur:
 *   ▶ mDNS Hostname: http://bluino.local
 *   ▶ DNS-SD Service Discovery: mengumumkan layanan _http._tcp
 *   ▶ Web Server: halaman status perangkat (IP, RSSI, Uptime)
 *
 * Cara Uji:
 *   1. Upload & buka Serial Monitor
 *   2. Buka browser → http://bluino.local (tanpa perlu tahu IP!)
 *   3. Di Mac: Finder > Go > Network → perangkat "bluino" akan terlihat
 *   4. Di Linux: avahi-browse _http._tcp
 *   5. Di Windows: File Explorer > Network
 *
 * ⚠️ ESP32 hanya mendukung WiFi 2.4 GHz!
 * ⚠️ Ganti SSID dan PASSWORD sesuai jaringan WiFi kamu!
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>

// ── Konfigurasi ───────────────────────────────────────────────────
const char* WIFI_SSID    = "NamaWiFiKamu";   // ← Ganti!
const char* WIFI_PASS    = "PasswordWiFi";   // ← Ganti!
const char* MDNS_NAME    = "bluino";         // Akses via bluino.local
const char* DEVICE_MODEL = "Bluino IoT Kit v3.2";

// ── Pin ──────────────────────────────────────────────────────────
#define LED_PIN 2

// ── Objek Utama ───────────────────────────────────────────────────
WebServer server(80);

// ─────────────────────────────────────────────────────────────────
// HTML: Halaman Status Perangkat
// ─────────────────────────────────────────────────────────────────
String buildStatusPage() {
  String uptime;
  uint32_t s = millis() / 1000;
  uint32_t h = s / 3600; s %= 3600;
  uint32_t m = s / 60;   s %= 60;
  uptime.reserve(20);
  uptime += h; uptime += "j "; uptime += m; uptime += "m "; uptime += s; uptime += "d";

  // Single-Shot Memory Allocation - Praktik terbaik menghindari heap fragmentation 
  // dengan memesan total keseluruhan ukuran buffer di awal secara absolut.
  String html;
  html.reserve(3500); 
  
  // Gunakan += (Append) untuk menggunakan memori yang sudah dialokasikan sepenuhnya
  html += R"rawhtml(
<!DOCTYPE html>
<html lang="id">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <meta http-equiv="refresh" content="10">
  <title>Bluino IoT — Status</title>
  <style>
    * { box-sizing: border-box; margin: 0; padding: 0; }
    body { font-family: Arial, sans-serif; background: #0f0f1a;
           color: #e0e0e0; min-height: 100vh;
           display: flex; align-items: center; justify-content: center; padding: 20px; }
    .card { background: #1a1a2e; border-radius: 16px; padding: 32px;
            width: 100%; max-width: 420px;
            box-shadow: 0 12px 40px rgba(0,0,0,0.5); }
    h2 { color: #4fc3f7; margin-bottom: 4px; display: flex; align-items: center; gap: 10px; }
    .subtitle { color: #666; font-size: 0.82rem; margin-bottom: 24px; }
    .row { display: flex; justify-content: space-between; align-items: center;
           padding: 10px 0; border-bottom: 1px solid #1e2a3a; }
    .row:last-child { border-bottom: none; }
    .label { color: #888; font-size: 0.88rem; }
    .value { color: #e0e0e0; font-size: 0.9rem; font-weight: bold; text-align: right; }
    .online { color: #81c784; }
    .chip   { background: #0f3460; color: #4fc3f7; padding: 2px 8px;
              border-radius: 6px; font-size: 0.75rem; }
    .footer { text-align: center; color: #444; font-size: 0.75rem; margin-top: 20px; }
  </style>
</head>
<body>
<div class="card">
  <h2>⚡ Bluino IoT Kit</h2>
  <p class="subtitle">Status Perangkat Real-Time (Auto-refresh 10s)</p>

  <div class="row">
    <span class="label">Status Jaringan</span>
    <span class="value online">🟢 Online</span>
  </div>
  <div class="row">
    <span class="label">Hostname mDNS</span>
    <span class="value"><span class="chip">bluino.local</span></span>
  </div>
)rawhtml";

  // Sanitasi Khusus XSS DOM Injection untuk SSID
  String safeSSID = WiFi.SSID();
  safeSSID.replace("&", "&amp;");
  safeSSID.replace("<", "&lt;");
  safeSSID.replace(">", "&gt;");

  html += "  <div class=\"row\"><span class=\"label\">Alamat IP</span>";
  html += "<span class=\"value\">";
  html += WiFi.localIP().toString();
  html += "</span></div>\n";

  html += "  <div class=\"row\"><span class=\"label\">SSID WiFi</span>";
  html += "<span class=\"value\">";
  html += safeSSID;
  html += "</span></div>\n";

  int rssi = WiFi.RSSI();
  String rssiIcon = (rssi >= -65) ? "🟢" : (rssi >= -80) ? "🟡" : "🔴";
  html += "  <div class=\"row\"><span class=\"label\">Kekuatan Sinyal</span>";
  html += "<span class=\"value\">";
  html += rssiIcon;
  html += " ";
  html += rssi;
  html += " dBm</span></div>\n";

  html += "  <div class=\"row\"><span class=\"label\">Model Perangkat</span>";
  html += "<span class=\"value\">";
  html += DEVICE_MODEL;
  html += "</span></div>\n";

  html += "  <div class=\"row\"><span class=\"label\">Uptime</span>";
  html += "<span class=\"value\">";
  html += uptime;
  html += "</span></div>\n";

  uint32_t freeHeap = ESP.getFreeHeap();
  html += "  <div class=\"row\"><span class=\"label\">Heap Bebas</span>";
  html += "<span class=\"value\">";
  html += (freeHeap / 1024);
  html += " KB</span></div>\n";

  html += R"rawhtml(
  <p class="footer">Halaman ini otomatis refresh tiap 10 detik.</p>
</div>
</body>
</html>
)rawhtml";

  return html;
}

// ─────────────────────────────────────────────────────────────────
// Handler Web Server
// ─────────────────────────────────────────────────────────────────
void handleRoot() {
  digitalWrite(LED_PIN, LOW); // Indikator fisik (Lampu redup mendadak)
  Serial.printf("[HTTP] GET / dari %s\n",
    server.client().remoteIP().toString().c_str());
  server.send(200, "text/html", buildStatusPage());
  digitalWrite(LED_PIN, HIGH); // Menyala penuh kembali
}

void handleNotFound() {
  server.sendHeader("Location", "http://bluino.local/", true);
  server.send(302, "text/plain", "");
}

// ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  delay(500);

  Serial.println("\n=== BAB 45 Program 2: mDNS + Service Discovery ===");

  // ── Koneksi WiFi ──────────────────────────────────────────────
  Serial.printf("[WiFi] Menghubungkan ke '%s'...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(MDNS_NAME);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  uint32_t tStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - tStart >= 15000UL) {
      Serial.println("\n[ERR] Timeout. Restart...");
      delay(2000);
      ESP.restart();
    }
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.printf("[WiFi] ✅ IP: %s\n", WiFi.localIP().toString().c_str());

  // ── Inisialisasi mDNS ─────────────────────────────────────────
  if (!MDNS.begin(MDNS_NAME)) {
    Serial.println("[ERR] Gagal memulai mDNS!");
    // Tidak fatal — Web Server tetap bisa diakses via IP
  } else {
    Serial.printf("[mDNS] ✅ Hostname: http://%s.local\n", MDNS_NAME);

    // Daftarkan layanan HTTP ke DNS-SD agar bisa di-discover oleh tools
    // Format: MDNS.addService(protokol, transport, port)
    // Ini memberitahu jaringan: "Hei, perangkat ini punya Web Server di port 80!"
    MDNS.addService("http", "tcp", 80);
    Serial.println("[mDNS] Service '_http._tcp' terdaftar di port 80.");

    // Tambahkan TXT record untuk metadata perangkat (opsional tapi berguna)
    // Tools seperti avahi-browse akan menampilkan info ini
    MDNS.addServiceTxt("http", "tcp", "model", DEVICE_MODEL);
    MDNS.addServiceTxt("http", "tcp", "version", "1.0");
    Serial.println("[mDNS] TXT Records: model, version.");
  }

  // ── Web Server ────────────────────────────────────────────────
  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  Serial.println("[HTTP] Web Server aktif di port 80.");

  Serial.println("\n[INFO] Siap! Buka browser dan ketik: http://bluino.local");
  
  digitalWrite(LED_PIN, HIGH);
}

// ─────────────────────────────────────────────────────────────────
void loop() {
  server.handleClient();

  // Heartbeat
  static unsigned long tHb = 0;
  if (millis() - tHb >= 15000UL) {
    tHb = millis();
    Serial.printf("[Heartbeat] IP: %s | RSSI: %d dBm | Heap: %u B | Uptime: %lu s\n",
      WiFi.localIP().toString().c_str(),
      WiFi.RSSI(),
      ESP.getFreeHeap(),
      millis() / 1000UL);
  }
}
```

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 45 Program 2: mDNS + Service Discovery ===
[WiFi] Menghubungkan ke 'WiFiRumah'...
.....
[WiFi] ✅ IP: 192.168.1.105
[mDNS] ✅ Hostname: http://bluino.local
[mDNS] Service '_http._tcp' terdaftar di port 80.
[mDNS] TXT Records: model, version.
[HTTP] Web Server aktif di port 80.

[INFO] Siap! Buka browser dan ketik: http://bluino.local
[HTTP] GET / dari 192.168.1.101
[Heartbeat] IP: 192.168.1.105 | RSSI: -58 dBm | Heap: 218240 B | ...
```

---

## 45.6 Program 3: mDNS Dinamis + Multi-Hostname + Serial CLI

Program tingkat lanjut yang menggabungkan semua konsep mDNS dan menambahkan kontrol via Serial Monitor serta kemampuan mengubah nama mDNS secara dinamis saat runtime menggunakan `Preferences` (NVS).

```cpp
/*
 * BAB 45 - Program 3: mDNS Dinamis + Multi-Hostname + Serial CLI
 *
 * Fitur:
 *   ▶ Hostname mDNS disimpan di NVS (persistent, bertahan restart)
 *   ▶ Serial CLI untuk mengubah hostname tanpa upload ulang
 *   ▶ Daftarkan multiple TXT record sebagai "kartu identitas" perangkat
 *   ▶ Auto-restart mDNS jika WiFi reconnect (robustness production-grade)
 *   ▶ Web Server dengan info lengkap dan CLI reference
 *
 * Serial CLI Commands:
 *   hostname <nama>  → Ganti hostname mDNS (disimpan ke NVS)
 *   status           → Tampilkan status jaringan & mDNS
 *   restart          → Restart ESP32
 *   help             → Tampilkan daftar perintah
 *
 * ⚠️ ESP32 hanya mendukung WiFi 2.4 GHz!
 * ⚠️ Ganti SSID dan PASSWORD sesuai jaringan WiFi kamu!
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>
#include <Preferences.h>

// ── Konfigurasi WiFi (di sini masih hardcode — lihat BAB 44 untuk WiFiManager) ──
const char* WIFI_SSID = "NamaWiFiKamu";   // ← Ganti!
const char* WIFI_PASS = "PasswordWiFi";  // ← Ganti!

// ── Pin ──────────────────────────────────────────────────────────
#define LED_PIN 2

// ── Default Konfigurasi ───────────────────────────────────────────
const char* DEFAULT_HOSTNAME = "bluino";
const char* NVS_NAMESPACE    = "mdns-cfg";
const char* NVS_KEY_HOSTNAME = "hostname";

// ── Objek Global ─────────────────────────────────────────────────
WebServer   server(80);
Preferences prefs;

// ── State ─────────────────────────────────────────────────────────
String  currentHostname = DEFAULT_HOSTNAME;
bool    mdnsActive      = false;
String  inputBuffer;

// ─────────────────────────────────────────────────────────────────
// NVS: Muat hostname tersimpan
// ─────────────────────────────────────────────────────────────────
void loadHostname() {
  prefs.begin(NVS_NAMESPACE, true); // read-only
  currentHostname = prefs.getString(NVS_KEY_HOSTNAME, DEFAULT_HOSTNAME);
  prefs.end();

  // Pengaman Korupsi Memori Fisik NVS: Potong memori garbage yang overbox!
  if (currentHostname.length() > 64) {
      currentHostname = currentHostname.substring(0, 64); 
  }

  // Validasi: hostname mDNS hanya boleh huruf kecil, angka, dan tanda hubung
  // Panjang 1-63 karakter, tidak boleh mulai/akhir dengan tanda hubung
  currentHostname.toLowerCase();
  currentHostname.replace(" ", "-");
  currentHostname.replace("_", "-");

  // Hapus karakter non-valid (hanya izinkan a-z, 0-9, -)
  String sanitized = "";
  sanitized.reserve(currentHostname.length() + 2); // Mencegah Boot-Time Heap Fragmentation!
  for (int i = 0; i < (int)currentHostname.length(); i++) {
    char c = currentHostname[i];
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-') {
      sanitized += c;
    }
  }
  // Hapus tanda hubung di awal/akhir
  while (sanitized.startsWith("-")) sanitized.remove(0, 1);
  while (sanitized.endsWith("-"))   sanitized.remove(sanitized.length() - 1, 1);

  if (sanitized.length() == 0) sanitized = DEFAULT_HOSTNAME;
  if (sanitized.length() > 63) sanitized = sanitized.substring(0, 63);

  currentHostname = sanitized;
}

// ─────────────────────────────────────────────────────────────────
// NVS: Simpan hostname baru
// ─────────────────────────────────────────────────────────────────
void saveHostname(const String& name) {
  prefs.begin(NVS_NAMESPACE, false); // read-write
  size_t bytesWritten = prefs.putString(NVS_KEY_HOSTNAME, name);
  prefs.end();
  
  if (bytesWritten > 0) {
    Serial.printf("[NVS] Hostname disimpan: '%s'\n", name.c_str());
  } else {
    Serial.println("[NVS] ❌ GAGAL! Sektor Flash Memory Penuh/Rusak.");
  }
}

// ─────────────────────────────────────────────────────────────────
// mDNS: Mulai / Restart mDNS dengan hostname saat ini
// ─────────────────────────────────────────────────────────────────
bool startMDNS() {
  // Hentikan instance lama sebelum mulai ulang
  MDNS.end();
  delay(100); // Beri waktu kernel bersih-bersih

  if (!MDNS.begin(currentHostname.c_str())) {
    Serial.println("[mDNS] ❌ Gagal memulai!");
    mdnsActive = false;
    return false;
  }

  // ── Daftarkan Layanan HTTP ────────────────────────────────────
  MDNS.addService("http", "tcp", 80);

  // ── TXT Records: Kartu Identitas Perangkat ────────────────────
  // Semua tools network discovery (Bonjour Browser, avahi-browse)
  // dapat membaca metadata ini tanpa perlu terhubung ke web server.
  MDNS.addServiceTxt("http", "tcp", "board",   "ESP32 DEVKIT V1");
  MDNS.addServiceTxt("http", "tcp", "kit",     "Bluino IoT Kit v3.2");
  MDNS.addServiceTxt("http", "tcp", "bab",     "BAB-45-mDNS");
  MDNS.addServiceTxt("http", "tcp", "sdk",     ESP.getSdkVersion());

  // Kalkulasi Chip ID utuh (48-bit MAC Address) tanpa pemotongan bit
  uint64_t chipId = ESP.getEfuseMac();
  char cidBuf[13];
  snprintf(cidBuf, 13, "%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);
  MDNS.addServiceTxt("http", "tcp", "chip_id", cidBuf);

  mdnsActive = true;
  Serial.printf("[mDNS] ✅ Aktif — http://%s.local\n", currentHostname.c_str());
  return true;
}

// ─────────────────────────────────────────────────────────────────
// HTML: Halaman Status Lengkap
// ─────────────────────────────────────────────────────────────────
String buildStatusPage() {
  uint32_t sec = millis() / 1000;
  uint32_t h = sec/3600; sec%=3600; uint32_t m = sec/60; sec%=60;

  // Single-Shot Memory Allocation: Pesan kapasitas penuh dari awal
  String html;
  html.reserve(3000);

  html += "<!DOCTYPE html><html lang=\"id\"><head>\n";
  html += "<meta charset=\"UTF-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<meta http-equiv=\"refresh\" content=\"10\">";
  html += "<title>";
  html += currentHostname;
  html += ".local — Status</title>\n";
  html += "<style>";
  html += "* { box-sizing: border-box; margin: 0; padding: 0; }";
  html += "body { font-family: Arial, sans-serif; background: #0a0a14;";
  html += "       color: #ddd; min-height:100vh; display:flex;";
  html += "       align-items:center; justify-content:center; padding:20px; }";
  html += ".card { background:#14142a; border-radius:16px; padding:30px;";
  html += "        width:100%; max-width:460px; box-shadow:0 12px 48px rgba(0,0,0,.6); }";
  html += "h2 { color:#4fc3f7; margin-bottom:4px; }";
  html += ".sub { color:#555; font-size:.8rem; margin-bottom:22px; }";
  html += "table { width:100%; border-collapse:collapse; }";
  html += "td { padding:9px 4px; border-bottom:1px solid #1a1a2e; font-size:.88rem; }";
  html += "td:first-child { color:#777; width:44%; }";
  html += "td:last-child { color:#eee; font-weight:bold; text-align:right; }";
  html += ".badge { background:#0f3460; color:#4fc3f7; padding:2px 8px;";
  html += "         border-radius:6px; font-size:.76rem; }";
  html += ".cli { background:#0a0a14; border-radius:10px; padding:14px; margin-top:20px; }";
  html += ".cli code { display:block; color:#81c784; font-size:.8rem; margin:3px 0; }";
  html += ".cli h4 { color:#aaa; font-size:.82rem; margin-bottom:8px; }";
  html += ".footer { text-align:center; color:#333; font-size:.72rem; margin-top:16px; }";
  html += "</style>";
  html += "</head><body><div class=\"card\">";

  html += "<h2>⚡ ";
  html += currentHostname;
  html += ".local</h2>";
  html += "<p class=\"sub\">Bluino IoT Kit — Halaman Status (auto-refresh 10s)</p>";
  html += "<table>";

  // Sanitasi DOM XSS Security
  String safeSSID = WiFi.SSID();
  safeSSID.replace("&", "&amp;");
  safeSSID.replace("<", "&lt;");
  safeSSID.replace(">", "&gt;");

  html += "<tr><td>Status</td><td>🟢 Online</td></tr>";
  html += "<tr><td>Hostname mDNS</td><td><span class=\"badge\">";
  html += currentHostname;
  html += ".local</span></td></tr>";
  html += "<tr><td>Alamat IP</td><td>";
  html += WiFi.localIP().toString();
  html += "</td></tr>";
  html += "<tr><td>SSID WiFi</td><td>";
  html += safeSSID;
  html += "</td></tr>";

  int rssi = WiFi.RSSI();
  String ri = (rssi >= -65) ? "🟢" : (rssi >= -80) ? "🟡" : "🔴";
  html += "<tr><td>Sinyal WiFi</td><td>";
  html += ri;
  html += " ";
  html += rssi;
  html += " dBm</td></tr>";

  html += "<tr><td>Uptime</td><td>";
  html += h;
  html += "j ";
  html += m;
  html += "m ";
  html += sec;
  html += "d</td></tr>";
  html += "<tr><td>Heap Bebas</td><td>";
  html += (ESP.getFreeHeap() / 1024);
  html += " KB</td></tr>";
  html += "<tr><td>CPU Freq.</td><td>";
  html += ESP.getCpuFreqMHz();
  html += " MHz</td></tr>";
  html += "<tr><td>Flash Size</td><td>";
  html += (ESP.getFlashChipSize() / 1024 / 1024);
  html += " MB</td></tr>";
  html += "<tr><td>SDK Version</td><td>";
  html += ESP.getSdkVersion();
  html += "</td></tr>";

  // Chip_ID utuh (48-bit MAC Address Murni)
  uint64_t chipId = ESP.getEfuseMac();
  char cidBuf[13];
  snprintf(cidBuf, 13, "%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);
  html += "<tr><td>Chip ID</td><td>";
  html += cidBuf;
  html += "</td></tr>";

  html += "</table>";

  html += "<div class=\"cli\">"
          "<h4>🖥️  Serial CLI (115200 baud):</h4>"
          "<code>hostname &lt;nama&gt; → Ganti hostname mDNS</code>"
          "<code>status            → Tampilkan status</code>"
          "<code>restart           → Restart ESP32</code>"
          "<code>help              → Bantuan</code>"
          "</div>";

  html += "<p class=\"footer\">Bluino IoT Kit 2026 — BAB 45: mDNS</p>";
  html += "</div></body></html>";

  return html;
}

// ─────────────────────────────────────────────────────────────────
// Serial CLI: Proses Perintah dari Input Pengguna
// ─────────────────────────────────────────────────────────────────
void processSerialCommand(const String& raw) {
  String cmd = raw;
  cmd.trim();

  if (cmd.length() == 0) return;

  Serial.println("[CLI] > " + cmd);

  String lowerCmd = cmd;
  lowerCmd.toLowerCase();

  if (lowerCmd == "help" || lowerCmd == "?") {
    Serial.println("┌──────────────────────────────────────────────┐");
    Serial.println("│           Bluino mDNS — Serial CLI           │");
    Serial.println("├──────────────────────────────────────────────┤");
    Serial.println("│ hostname <nama>  Ganti hostname mDNS         │");
    Serial.println("│                 (disimpan ke NVS permanen)   │");
    Serial.println("│ status           Tampilkan info jaringan      │");
    Serial.println("│ restart          Restart ESP32                │");
    Serial.println("│ help / ?         Tampilkan bantuan ini        │");
    Serial.println("└──────────────────────────────────────────────┘");

  } else if (lowerCmd == "status") {
    Serial.println("── Status Jaringan ──────────────────────────");
    Serial.printf ("  Hostname mDNS : %s.local\n", currentHostname.c_str());
    Serial.printf ("  mDNS Aktif    : %s\n", mdnsActive ? "Ya" : "Tidak");
    Serial.printf ("  WiFi Status   : %s\n", (WiFi.status() == WL_CONNECTED) ? "Terhubung" : "Terputus");
    Serial.printf ("  IP Address    : %s\n", WiFi.localIP().toString().c_str());
    Serial.printf ("  SSID          : %s\n", WiFi.SSID().c_str());
    Serial.printf ("  RSSI          : %d dBm\n", WiFi.RSSI());
    Serial.printf ("  Uptime        : %lu s\n", millis() / 1000UL);
    Serial.printf ("  Free Heap     : %u bytes\n", ESP.getFreeHeap());
    Serial.println("─────────────────────────────────────────────");

  } else if (lowerCmd == "restart") {
    Serial.println("[CLI] Restart dalam 2 detik...");
    MDNS.end(); // Graceful shutdown memutus advertise mDNS secara elegan ke network
    delay(2000);
    ESP.restart();

  } else if (lowerCmd == "hostname") {
    Serial.printf("[CLI] Hostname saat ini: '%s.local'\n", currentHostname.c_str());
    Serial.println("[CLI] Gunakan 'hostname <nama_baru>' untuk mengubah.");

  } else if (lowerCmd.startsWith("hostname ")) {
    String newName = cmd.substring(9);
    newName.trim();
    newName.toLowerCase();
    
    // Mencegah UX fail jika pengguna keliru menginput: "hostname perangkat.local"
    if (newName.endsWith(".local")) {
      newName = newName.substring(0, newName.length() - 6);
    }
    
    newName.replace(" ", "-");
    newName.replace("_", "-");

    // Validasi: hanya karakter valid RFC-1034
    String validated = "";
    validated.reserve(newName.length() + 2); // Hindari 60x Loop Reallocation!
    for (int i = 0; i < (int)newName.length(); i++) {
      char c = newName[i];
      if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '-')
        validated += c;
    }
    while (validated.startsWith("-")) validated.remove(0, 1);
    while (validated.endsWith("-"))   validated.remove(validated.length()-1, 1);

    if (validated.length() < 1 || validated.length() > 63) {
      Serial.println("[CLI] ❌ Nama tidak valid! Gunakan huruf kecil, angka, atau tanda hubung (1-63 karakter).");
      return;
    }

    currentHostname = validated;
    saveHostname(currentHostname);

    // Restart mDNS dengan hostname baru TANPA restart ESP32!
    Serial.printf("[CLI] Mengganti hostname ke '%s.local'...\n", currentHostname.c_str());
    WiFi.setHostname(currentHostname.c_str());
    if (startMDNS()) {
      Serial.printf("[CLI] ✅ Berhasil! Sekarang akses via: http://%s.local\n", currentHostname.c_str());
    } else {
      Serial.println("[CLI] ❌ Gagal restart mDNS. Coba perintah 'restart'.");
    }

  } else {
    Serial.printf("[CLI] ❓ Perintah tidak dikenal: '%s'. Ketik 'help' untuk bantuan.\n", cmd.c_str());
  }
}

// ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  delay(500);

  Serial.println("\n=== BAB 45 Program 3: mDNS Dinamis + Serial CLI ===");
  Serial.println("[INFO] Ketik 'help' di Serial Monitor untuk melihat perintah.");

  // Pre-alokasi CLI buffer global untuk mencegah fragmentasi pada resize input looping
  inputBuffer.reserve(128);

  // ── Muat Hostname dari NVS ────────────────────────────────────
  loadHostname();
  Serial.printf("[NVS] Hostname dimuat: '%s'\n", currentHostname.c_str());

  // ── Koneksi WiFi ──────────────────────────────────────────────
  Serial.printf("[WiFi] Menghubungkan ke '%s'...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(currentHostname.c_str());
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  uint32_t tStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - tStart >= 20000UL) {
      Serial.println("\n[ERR] Timeout WiFi. Restart...");
      delay(2000);
      ESP.restart();
    }
    if (Serial.available()) {
      String input = Serial.readStringUntil('\n');
      input.trim();
      if (input.length() > 0) {
        Serial.println("\n[CLI] ⏳ Harap tunggu hingga WiFi terhubung...");
      }
    }
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  Serial.printf("[WiFi] ✅ Terhubung! IP: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("[WiFi] SSID         : %s\n", WiFi.SSID().c_str());

  // ── Inisialisasi mDNS ─────────────────────────────────────────
  startMDNS();

  // ── Web Server ────────────────────────────────────────────────
  server.on("/", HTTP_GET, []() {
    digitalWrite(LED_PIN, LOW); // Kedip respon penerimaan HTTP Request
    server.send(200, "text/html", buildStatusPage());
    digitalWrite(LED_PIN, HIGH);
  });
  server.on("/status", HTTP_GET, []() {
    digitalWrite(LED_PIN, LOW);
    // 1. Ekstrak identitas murni ESP32
    uint64_t chipId = ESP.getEfuseMac();
    char cidBuf[13];
    snprintf(cidBuf, 13, "%04X%08X", (uint16_t)(chipId >> 32), (uint32_t)chipId);

    // 2. Endpoint JSON High-Performance via alokasi Stack C Native (Mencegah Heap Fragmentation)
    char json[256];
    snprintf(json, sizeof(json),
      "{\"hostname\":\"%s.local\",\"ip\":\"%s\",\"rssi\":%d,\"uptime\":%lu,\"freeHeap\":%u,\"cpuMHz\":%u,\"chipId\":\"%s\"}",
      currentHostname.c_str(),
      WiFi.localIP().toString().c_str(),
      WiFi.RSSI(),
      millis() / 1000UL,
      ESP.getFreeHeap(),
      ESP.getCpuFreqMHz(),
      cidBuf
    );
    // Bypass String() implisit C++ pada `server.send` dengan streaming murni
    server.setContentLength(strlen(json));
    server.send(200, "application/json", "");
    server.sendContent(json);
    digitalWrite(LED_PIN, HIGH);
  });
  server.onNotFound([]() {
    String redirectUrl;
    redirectUrl.reserve(32 + currentHostname.length());
    redirectUrl += "http://";
    redirectUrl += currentHostname;
    redirectUrl += ".local/";
    server.sendHeader("Location", redirectUrl, true);
    server.send(302, "text/plain", "");
  });
  server.begin();
  Serial.println("[HTTP] Web Server aktif.");

  Serial.println("\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");
  Serial.printf (" 🌐 Akses Web: http://%s.local\n", currentHostname.c_str());
  Serial.println(" 📊 Akses API: /status (Berformat JSON)");
  Serial.println("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━");

  digitalWrite(LED_PIN, HIGH);
}

// ─────────────────────────────────────────────────────────────────
void loop() {
  // ── Pelayan Web ───────────────────────────────────────────────
  server.handleClient();

  // ── Serial CLI ────────────────────────────────────────────────
  while (Serial.available()) {
    char c = (char)Serial.read();
    if (c == '\n' || c == '\r') {
      if (inputBuffer.length() > 0) {
        processSerialCommand(inputBuffer);
        inputBuffer = "";
      }
    } else {
      if (inputBuffer.length() < 128) { // Batas buffer input CLI
        inputBuffer += c;
      }
    }
  }

  // ── Deteksi Transisi WiFi ───────────────────────────────────────
  // Perubahan IP akibat reconnect ditangani otomatis oleh ESPmDNS 
  // di dalam kernel (LwIP). Tidak perlu mDNS restart manual.
  static bool prevWiFiState = true;
  bool currWiFiState = (WiFi.status() == WL_CONNECTED);
  
  if (!prevWiFiState && currWiFiState) {
    // Transisi: dari Terputus → Terhubung
    Serial.printf("[WiFi] ✅ Reconnect! IP baru: %s\n",
      WiFi.localIP().toString().c_str());
  }
  if (prevWiFiState && !currWiFiState) {
    Serial.println("[WiFi] ⚠️  Koneksi terputus...");
    digitalWrite(LED_PIN, LOW);
  }
  prevWiFiState = currWiFiState;

  // ── Heartbeat ─────────────────────────────────────────────────
  static unsigned long tHb = 0;
  if (millis() - tHb >= 30000UL) {
    tHb = millis();
    Serial.printf("[Heartbeat] %s.local | IP: %s | RSSI: %d | Heap: %u B\n",
      currentHostname.c_str(),
      WiFi.localIP().toString().c_str(),
      WiFi.RSSI(),
      ESP.getFreeHeap());
  }
}
```

**Contoh Sesi Serial CLI:**
```
=== BAB 45 Program 3: mDNS Dinamis + Serial CLI ===
[INFO] Ketik 'help' di Serial Monitor untuk melihat perintah.
[NVS] Hostname dimuat: 'bluino'
[WiFi] ✅ Terhubung! IP: 192.168.1.105
[mDNS] ✅ Aktif — http://bluino.local
[HTTP] Web Server aktif.

╔══════════════════════════════════════════════╗
║  Akses via: http://bluino.local              ║
║  (Juga tersedia di /status untuk JSON API)   ║
╚══════════════════════════════════════════════╝

--- (ketik "hostname sensor-ruang-tamu" di Serial Monitor) ---

[CLI] > hostname sensor-ruang-tamu
[NVS] Hostname disimpan: 'sensor-ruang-tamu'
[CLI] Mengganti hostname ke 'sensor-ruang-tamu.local'...
[mDNS] ✅ Aktif — http://sensor-ruang-tamu.local
[CLI] ✅ Berhasil! Sekarang akses via: http://sensor-ruang-tamu.local
```

---

## 45.7 Konsep Lanjutan: Mengapa mDNS Esensial untuk IoT?

```
SKENARIO SMART HOME PROFESIONAL:

  Tanpa mDNS:
  ┌───────────────────────────────────────────────────────┐
  │  Sensor Suhu ESP32 #1  → IP: 192.168.1.101 (?)       │
  │  Sensor Suhu ESP32 #2  → IP: 192.168.1.102 (?)       │
  │  Relay Controller      → IP: 192.168.1.115 (?)       │
  │  Dashboard Node-RED    → "Harus cek dulu IP-nya!"     │
  └───────────────────────────────────────────────────────┘
  MASALAH: Setiap reboot bisa berubah. Tidak scalable!

  Dengan mDNS:
  ┌───────────────────────────────────────────────────────┐
  │  Sensor Suhu #1  → sensor-kamar.local      (tetap!)  │
  │  Sensor Suhu #2  → sensor-dapur.local      (tetap!)  │
  │  Relay           → relay-ac-ruang.local    (tetap!)  │
  │  Dashboard       → http://sensor-kamar.local → ✅   │
  └───────────────────────────────────────────────────────┘
  SOLUSI: Nama selalu sama, IP boleh berubah-ubah.


PERBANDINGAN TEKNOLOGI RESOLUSI NAMA LOKAL:

  ┌────────────────┬───────────────┬──────────────┬────────────┐
  │ Teknologi      │ Butuh Server? │ Cross-Platform│ Port       │
  ├────────────────┼───────────────┼──────────────┼────────────┤
  │ mDNS           │ Tidak (P2P)   │ Ya (+ config)│ 5353       │
  │ Static IP      │ Tidak         │ Ya           │ N/A        │
  │ DNS Server     │ Ya (Pi-hole)  │ Ya           │ 53         │
  │ NetBIOS/WINS   │ Optional      │ Windows only │ 137-139    │
  │ Avahi/Bonjour  │ Tidak (daemon)│ Linux/Mac    │ 5353       │
  └────────────────┴───────────────┴──────────────┴────────────┘


KETERBATASAN mDNS (Yang Wajib Dipahami):

  1. HANYA untuk jaringan LOKAL (LAN/WiFi yang sama)
     → Tidak bisa diakses dari internet (WAN)
     → Solusi untuk remote access: lihat BAB 57 (Blynk) atau BAB 58 (Firebase)
  
  2. Bluetooth dan WiFi BERBEDA subnet = tidak bisa
     → mDNS menggunakan Multicast yang dibatasi subnet

  3. Beberapa router mengeblok multicast secara default
     → Cek pengaturan router: izinkan "Multicast" atau "mDNS passthrough"
  
  4. Satu nama = satu perangkat
     → Jika dua ESP32 pakai nama sama, konflik akan terjadi
     → Gunakan MDNS.queryHost("nama.local") untuk cek konflik
```

---

## 45.8 Tabel Ringkasan Fungsi Library `ESPmDNS`

| Fungsi | Keterangan | Contoh |
|--------|------------|--------|
| `MDNS.begin(hostname)` | Mulai mDNS dengan nama tertentu | `MDNS.begin("bluino")` |
| `MDNS.end()` | Hentikan mDNS (sebelum restart) | `MDNS.end()` |
| `MDNS.addService(proto, transport, port)` | Umumkan layanan ke DNS-SD | `MDNS.addService("http","tcp",80)` |
| `MDNS.addServiceTxt(proto, transport, key, val)` | Tambah metadata layanan | `MDNS.addServiceTxt("http","tcp","version","1.0")` |
| `MDNS.queryHost(hostname)` | Cari IP perangkat lain via mDNS | `IPAddress ip = MDNS.queryHost("bluino.local")` |
| `MDNS.queryService(proto, transport)` | Temukan service di jaringan | `int n = MDNS.queryService("http","tcp")` |

---

## 45.9 Latihan & Tantangan

### Latihan Dasar
1. **Upload Program 1**, lakukan `ping bluino.local` dari PC. Verifikasi IP yang muncul sama dengan yang ditampilkan di Serial Monitor.
2. **Upload Program 2**, buka `http://bluino.local` di browser HP kamu. Amati halaman status yang muncul.
3. Coba tutup Serial Monitor, restart ESP32, buka kembali `http://bluino.local`. Apakah bisa diakses? Tanpa melihat IP?

### Latihan Menengah
4. **Modifikasi Program 3**: Tambahkan perintah CLI `scan` yang akan menelusuri semua perangkat `_http._tcp` di jaringan menggunakan `MDNS.queryService("http", "tcp")` dan menampilkan daftar hostname + IP mereka di Serial Monitor.
5. **Uji Konflik Nama**: Sambungkan dua ESP32 berbeda dengan nama mDNS yang sama. Amati apa yang terjadi di browser dan Serial Monitor. Siapa yang "menang"?

### Kata Kunci untuk Penelitian Mandiri
- `DNS-SD (DNS Service Discovery)` — Standar di balik fitur Service Discovery
- `Bonjour` — Implementasi Apple untuk mDNS + DNS-SD (macOS, iOS)
- `Avahi` — Implementasi open-source untuk Linux
- `Zero-configuration Networking (Zeroconf)` — Ekosistem protokol secara keseluruhan

---

## 45.10 Materi Referensi

| Sumber | Link |
|--------|------|
| RFC 6762 — mDNS Standard | https://datatracker.ietf.org/doc/html/rfc6762 |
| RFC 6763 — DNS-SD Standard | https://datatracker.ietf.org/doc/html/rfc6763 |
| ESPmDNS GitHub | https://github.com/espressif/arduino-esp32/tree/master/libraries/ESPmDNS |
| Espressif mDNS Docs | https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/protocols/mdns.html |

---

> ✅ **Selesai!** Sekarang ESP32 Bluino Kit kamu bisa ditemukan di jaringan lokal hanya dengan nama `bluino.local` — tidak perlu tahu nomor IP lagi!
>
> **Langkah Selanjutnya:** [BAB 46 — Web Server](../BAB-46/) untuk membangun dashboard IoT berbasis web yang dapat dikontrol via browser secara penuh.
