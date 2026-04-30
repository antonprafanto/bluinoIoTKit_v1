# 📡 BAB 52 — Bluetooth Classic: Komunikasi Nirkabel Tanpa Internet

> ✅ **Prasyarat:** Selesaikan **BAB 51 (CoAP)**. Kamu harus sudah memahami konsep komunikasi nirkabel IoT dan pola non-blocking dengan `millis()`.

> 📖 **Glosarium Singkatan BAB ini:**
> | Singkatan | Kepanjangan |
> |-----------|-------------|
> | **BT** | *Bluetooth* — standar komunikasi nirkabel jarak pendek |
> | **SPP** | *Serial Port Profile* — profil Bluetooth untuk komunikasi serial (seperti UART nirkabel) |
> | **MAC** | *Media Access Control* — alamat unik perangkat Bluetooth (6 byte) |
> | **Pairing** | Proses pencocokan dua perangkat Bluetooth sebelum dapat berkomunikasi |
> | **SDP** | *Service Discovery Protocol* — protokol untuk menemukan layanan Bluetooth |
> | **BLE** | *Bluetooth Low Energy* — versi Bluetooth hemat daya (dipelajari di BAB 53) |

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami perbedaan Bluetooth Classic vs BLE dan kapan menggunakan masing-masing.
- Menggunakan library `BluetoothSerial` bawaan ESP32 untuk komunikasi via SPP.
- Membangun ESP32 sebagai perangkat Bluetooth yang dapat dipasangkan dengan Android/Windows.
- Mengirim data sensor DHT11 secara *real-time* ke smartphone via Bluetooth.
- Membuat sistem kontrol relay berbasis perintah teks melalui Bluetooth.

---

## 52.1 Bluetooth Classic vs BLE vs WiFi

```
Perbandingan Komunikasi Nirkabel ESP32:

  WiFi (BAB 44-51):
    ▶ Jangkauan: 30-100m | Kecepatan: hingga 150 Mbps
    ▶ Butuh router/access point
    ▶ Cocok untuk: IoT cloud, streaming data besar

  Bluetooth Classic (BAB 52) — "Bluetooth lama":
    ▶ Jangkauan: 10-30m | Kecepatan: hingga 3 Mbps
    ▶ Peer-to-peer LANGSUNG — tidak butuh router!
    ▶ Profil SPP = "UART nirkabel" antara dua perangkat
    ▶ Cocok untuk: data logger ke HP, kontrol robot, printer

  BLE — Bluetooth Low Energy (BAB 53):
    ▶ Jangkauan: 10-30m | Kecepatan: 125kbps-2Mbps
    ▶ Konsumsi daya: 10x lebih hemat dari BT Classic
    ▶ Cocok untuk: sensor baterai, wearable, beacon

  ⚠️ PENTING — Bluetooth Classic & BLE adalah dua hal BERBEDA!
     ESP32 mendukung KEDUANYA, tapi:
     - BT Classic (BAB 52): pairing dulu baru kirim data
     - BLE (BAB 53): discovery tanpa pairing, model client-server GATT
```

| Aspek | WiFi | Bluetooth Classic | BLE |
|-------|------|------------------|-----|
| **Butuh Router** | ✅ | ❌ | ❌ |
| **Jangkauan** | 30-100m | 10-30m | 10-30m |
| **Konsumsi Daya** | Tinggi | Sedang | **Rendah** |
| **Kecepatan** | Tinggi | Sedang | Rendah-Sedang |
| **Koneksi** | Server-Client | Peer-to-Peer | GATT Client-Server |
| **Pairing** | Password | ✅ PIN/Confirm | ❌ Tidak Wajib |
| **Library ESP32** | WiFi.h | **BluetoothSerial.h** | NimBLE/ArduinoBLE |

---

## 52.2 Arsitektur Bluetooth Classic SPP

```
Profil SPP (Serial Port Profile) = kabel serial virtual via udara:

  Smartphone / PC                    ESP32 (BAB 52)
  ┌──────────────────┐               ┌──────────────────────┐
  │  Bluetooth app   │               │  BluetoothSerial     │
  │  (Serial BT      │               │  SerialBT.begin()    │
  │   Terminal, dll) │               │  SerialBT.available()│
  │                  │◄── SPP ──────►│  SerialBT.read()     │
  │  Kirim: "ON\n"   │               │  SerialBT.println()  │
  │  Terima: "OK\n"  │               │                      │
  └──────────────────┘               └──────────────────────┘

  Alur Komunikasi:
  1. ESP32 menyala → SerialBT.begin("Bluino-Kit") → nama Bluetooth terdaftar
  2. Smartphone scan Bluetooth → temukan "Bluino-Kit" → Pair
  3. Buka app "Serial Bluetooth Terminal" → Connect ke "Bluino-Kit"
  4. Ketik perintah di app → ESP32 terima via SerialBT.read()
  5. ESP32 balas → tampil di layar app
```

---

## 52.3 Library & API Reference

```
LIBRARY YANG DIGUNAKAN:

  BluetoothSerial.h  → BAWAAN ESP32 Arduino Core, tidak perlu install!
                       Sudah tersedia saat install ESP32 board via Board Manager.

  ⚠️ PENTING: BluetoothSerial HANYA tersedia di ESP32 (bukan ESP8266, bukan ESP32-S2/S3 C3!)
     Pastikan board manager mendeteksi "ESP32 Dev Module" (bukan varian lain).

API BluetoothSerial ESENSIAL:

  INISIASI:
    BluetoothSerial SerialBT;
    SerialBT.begin("NamaDevice");      → Mulai BT, nama yang muncul saat scan
    SerialBT.begin("NamaDevice", true) → Mode Master (sambungkan ke device lain)
    SerialBT.end();                    → Matikan Bluetooth

  STATUS:
    SerialBT.connected()               → true jika ada client terhubung
    SerialBT.hasClient()               → Alias connected() di versi lama
    SerialBT.available()               → Jumlah byte data masuk di buffer

  BACA DATA (menerima dari smartphone):
    int b = SerialBT.read()            → Baca 1 byte (-1 jika kosong)
    String s = SerialBT.readString()   → Baca semua buffer sebagai String
    SerialBT.readStringUntil('\n')     → Baca hingga newline

  KIRIM DATA (ke smartphone):
    SerialBT.print("teks")            → Kirim string tanpa newline
    SerialBT.println("teks")          → Kirim string + newline \r\n
    SerialBT.write(buf, len)          → Kirim binary data
    SerialBT.printf("%.1f", 29.1)     → Kirim formatted string

  CALLBACK (opsional — event notifikasi koneksi):
    SerialBT.register_callback(callbackFn);
    void callbackFn(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
      if (event == ESP_SPP_SRV_OPEN_EVT)  { /* client connected */ }
      if (event == ESP_SPP_CLOSE_EVT)     { /* client disconnected */ }
    }
```

---

## 52.4 Setup Smartphone untuk Testing

### Android (Disarankan)
1. Install aplikasi: **"Serial Bluetooth Terminal"** di Play Store (gratis, tanpa iklan)
2. Aktifkan Bluetooth di HP → Scan → Temukan **"Bluino-Kit"** → Pair
3. Buka app → klik ikon 3 garis → **Devices** → pilih **"Bluino-Kit"** → **Connect**
4. Ketik perintah di kolom bawah → tekan kirim

### Windows
1. Buka **Settings → Bluetooth** → Add device → Pilih **"Bluino-Kit"**
2. Buka **Device Manager** → Ports (COM & LPT) → Catat nomor COM baru (mis. COM8)
3. Buka **PuTTY** atau **Arduino Serial Monitor** → pilih port COM8 → 115200 baud

> ⚠️ **Catatan Pairing:** Secara default ESP32 dengan `BluetoothSerial` tidak memerlukan PIN. Jika diminta, coba PIN **1234** atau **0000**.

---

## 52.5 Program 1: Bluetooth Serial Echo & Command Handler

ESP32 sebagai **perangkat Bluetooth** yang dapat dipasangkan dengan smartphone. Menerima perintah teks dan membalas respons. Sangat baik untuk memahami alur dasar komunikasi SPP.

### Tujuan Pembelajaran
- Memahami inisiasi `BluetoothSerial` dan prosedur pairing
- Mempraktikkan baca/tulis data via `SerialBT`
- Membangun sistem perintah sederhana (command parser)

### Hardware
- OLED: tampilkan status koneksi dan perintah terakhir yang diterima

```cpp
/*
 * BAB 52 - Program 1: Bluetooth Serial Echo & Command Handler
 *
 * Fitur:
 *   ► ESP32 sebagai perangkat Bluetooth SPP bernama "Bluino-Kit"
 *   ► Semua teks yang dikirim smartphone di-echo kembali
 *   ► Command parser: STATUS, HELLO, HEAP, UPTIME, HELP
 *   ► Callback notifikasi: log saat client connect/disconnect
 *   ► OLED tampilkan status koneksi & perintah terakhir
 *   ► Heartbeat Serial tiap 60 detik
 *
 * Hardware:
 *   OLED → SDA=IO21, SCL=IO22 (hardwired Bluino Kit)
 *
 * Library:
 *   BluetoothSerial.h — BAWAAN ESP32 Core (tidak perlu install!)
 *
 * Cara Uji:
 *   1. Upload sketch, buka Serial Monitor 115200 baud
 *   2. Di HP Android: Settings → Bluetooth → Scan → "Bluino-Kit" → Pair
 *   3. Install "Serial Bluetooth Terminal" dari Play Store → Connect
 *   4. Ketik: STATUS → ESP32 balas info
 *   5. Ketik: HELLO  → ESP32 balas sapaan
 */

#include <BluetoothSerial.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Nama Perangkat Bluetooth ──────────────────────────────────────────
const char* BT_NAME = "Bluino-Kit";

// ── Hardware ──────────────────────────────────────────────────────────
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool oledReady = false;

// ── Objek Bluetooth ───────────────────────────────────────────────────
BluetoothSerial SerialBT;

// ── State ─────────────────────────────────────────────────────────────
bool     btConnected = false;
uint32_t rxCount     = 0;
char     lastCmd[64] = "—";

// ── OLED Helper ───────────────────────────────────────────────────────
void updateOLED() {
  if (!oledReady) return;
  oled.clearDisplay(); oled.setTextColor(WHITE); oled.setTextSize(1);
  oled.setCursor(0, 0);  oled.print("BT Classic SPP");
  oled.drawLine(0, 10, 127, 10, WHITE);
  oled.setCursor(0, 14); oled.print("Status: ");
  oled.print(btConnected ? "TERHUBUNG" : "Menunggu...");
  oled.setCursor(0, 26); oled.print("Nama: "); oled.print(BT_NAME);
  oled.setCursor(0, 38); oled.print("Rx:#"); oled.print(rxCount);
  oled.setCursor(0, 50); oled.print("Cmd: "); oled.print(lastCmd);
  oled.display();
}

// ── Callback: event koneksi BT ────────────────────────────────────────
// Dipanggil otomatis saat client connect/disconnect
void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    btConnected = true;
    Serial.println("[BT] ✅ Client terhubung!");
    updateOLED();
  } else if (event == ESP_SPP_CLOSE_EVT) {
    btConnected = false;
    Serial.println("[BT] ⚠️ Client terputus.");
    updateOLED();
  }
}

// ── Proses perintah dari smartphone ──────────────────────────────────
void processCommand(String cmd) {
  cmd.trim();
  cmd.toUpperCase();

  // Simpan untuk OLED (maks 10 karakter)
  strncpy(lastCmd, cmd.c_str(), sizeof(lastCmd) - 1);
  lastCmd[sizeof(lastCmd) - 1] = '\0';
  rxCount++;

  Serial.printf("[BT] Terima: '%s'\n", cmd.c_str());

  if (cmd == "STATUS") {
    SerialBT.printf("=== STATUS ESP32 ===\r\n");
    SerialBT.printf("Board  : ESP32\r\n");
    SerialBT.printf("Heap   : %u B\r\n", ESP.getFreeHeap());
    SerialBT.printf("Uptime : %lu s\r\n", millis() / 1000UL);
    SerialBT.printf("BT Name: %s\r\n", BT_NAME);
    SerialBT.printf("Rx Cnt : %u\r\n", rxCount);
  } else if (cmd == "HELLO") {
    SerialBT.println("Halo dari ESP32 Bluino Kit! 👋");
  } else if (cmd == "HEAP") {
    SerialBT.printf("Free Heap: %u B\r\n", ESP.getFreeHeap());
  } else if (cmd == "UPTIME") {
    uint32_t up = millis() / 1000UL;
    SerialBT.printf("Uptime: %02lu:%02lu:%02lu\r\n",
      up/3600, (up%3600)/60, up%60);
  } else if (cmd == "HELP") {
    SerialBT.println("Perintah tersedia:");
    SerialBT.println("  STATUS  → info ESP32");
    SerialBT.println("  HELLO   → sapaan");
    SerialBT.println("  HEAP    → free heap");
    SerialBT.println("  UPTIME  → waktu berjalan");
    SerialBT.println("  HELP    → daftar perintah");
  } else if (cmd.length() > 0) {
    // Echo perintah tidak dikenal
    SerialBT.printf("Echo: %s\r\n", cmd.c_str());
    SerialBT.println("(Ketik HELP untuk daftar perintah)");
  }

  updateOLED();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  Serial.println("\n=== BAB 52 Program 1: Bluetooth Classic SPP ===");
  Serial.printf("[BT] Nama perangkat: %s\n", BT_NAME);

  // OLED
  if (oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledReady = true;
    Serial.println("[OLED] ✅ Display terdeteksi di 0x3C");
  } else {
    Serial.println("[OLED] ⚠️ Display tidak terdeteksi — lanjut tanpa OLED.");
  }

  // Inisiasi Bluetooth
  SerialBT.register_callback(btCallback); // Daftarkan SEBELUM begin()!
  if (!SerialBT.begin(BT_NAME)) {
    Serial.println("[BT] ❌ Gagal inisiasi Bluetooth!");
    delay(3000); ESP.restart();
  }
  Serial.printf("[BT] ✅ Bluetooth siap! Nama: '%s'\n", BT_NAME);
  Serial.println("[BT] Lakukan Pairing dari HP, lalu Connect via Serial BT Terminal.");

  updateOLED();
}

void loop() {
  // Baca data dari Bluetooth (non-blocking)
  if (SerialBT.available()) {
    String incoming = SerialBT.readStringUntil('\n');
    processCommand(incoming);
  }

  // Heartbeat
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] BT:%s | Rx:%u | Heap:%u B | Up:%lu s\n",
      btConnected ? "ON" : "OFF", rxCount,
      ESP.getFreeHeap(), millis() / 1000UL);
    if (btConnected) {
      SerialBT.printf("[HB] Heap:%u Up:%lu s\r\n",
        ESP.getFreeHeap(), millis() / 1000UL);
    }
  }
}
```

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 52 Program 1: Bluetooth Classic SPP ===
[BT] Nama perangkat: Bluino-Kit
[OLED] ✅ Display terdeteksi di 0x3C
[BT] ✅ Bluetooth siap! Nama: 'Bluino-Kit'
[BT] Lakukan Pairing dari HP, lalu Connect via Serial BT Terminal.
--- (setelah smartphone pair & connect) ---
[BT] ✅ Client terhubung!
[BT] Terima: 'HELLO'
[BT] Terima: 'STATUS'
[BT] Terima: 'HELP'
[HB] BT:ON | Rx:3 | Heap:245600 B | Up:61 s
```

> ⚠️ **Penting — Callback vs Polling:**
> `SerialBT.register_callback()` **WAJIB** dipanggil sebelum `SerialBT.begin()`.
> Jika callback tidak terdaftar, event connect/disconnect tidak akan terpanggil.

---

## 52.6 Program 2: Bluetooth Sensor Monitor — Stream Data DHT11

ESP32 membaca DHT11 dan **mengirim data secara otomatis** ke smartphone via Bluetooth setiap 5 detik. Smartphone menjadi "dashboard nirkabel" tanpa internet!

### Tujuan Pembelajaran
- Mengirim data sensor secara *streaming* via Bluetooth SPP
- Membuat format pesan yang mudah dibaca di terminal smartphone
- Menangani command GET untuk pembacaan data on-demand

### Hardware
- DHT11 (IO27): baca suhu & kelembaban
- OLED: tampilkan suhu, kelembaban, status koneksi Bluetooth

```cpp
/*
 * BAB 52 - Program 2: Bluetooth Sensor Monitor — Stream Data DHT11
 *
 * Fitur:
 *   ► Kirim data DHT11 otomatis tiap 5 detik ke smartphone via BT
 *   ► Command: GET → baca sensor langsung; STATUS → info device
 *   ► Callback connect/disconnect untuk OLED notifikasi
 *   ► Format output rapi: [DATA] Suhu:29.1 Lembab:65%
 *   ► Non-blocking DHT11 read tiap 2 detik via millis()
 *   ► OLED tampilkan suhu, kelembaban, status BT
 *   ► Heartbeat tiap 60 detik
 *
 * Hardware:
 *   DHT11 → IO27 (hardwired Bluino Kit)
 *   OLED  → SDA=IO21, SCL=IO22 (hardwired)
 *
 * Library:
 *   BluetoothSerial.h — bawaan ESP32 Core (tidak perlu install)
 *   DHT sensor library by Adafruit (Library Manager)
 */

#include <BluetoothSerial.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Nama Perangkat Bluetooth ──────────────────────────────────────────
const char* BT_NAME = "Bluino-Sensor";

// ── Interval kirim data ke smartphone ────────────────────────────────
const unsigned long SEND_INTERVAL = 5000UL;  // 5 detik

// ── Hardware ──────────────────────────────────────────────────────────
#define DHT_PIN 27
DHT dht(DHT_PIN, DHT11);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool oledReady = false;

// ── Objek Bluetooth ───────────────────────────────────────────────────
BluetoothSerial SerialBT;

// ── State ─────────────────────────────────────────────────────────────
float    lastT     = NAN;
float    lastH     = NAN;
bool     btConn    = false;
uint32_t sendCount = 0;

// ── OLED Helper ───────────────────────────────────────────────────────
void updateOLED() {
  if (!oledReady) return;
  oled.clearDisplay(); oled.setTextColor(WHITE);
  oled.setTextSize(1); oled.setCursor(0, 0);
  oled.print("BT Sensor Monitor");
  oled.drawLine(0, 10, 127, 10, WHITE);
  oled.setCursor(0, 14);
  oled.print(btConn ? "BT: TERHUBUNG" : "BT: Menunggu...");
  oled.setTextSize(2); oled.setCursor(0, 26);
  if (!isnan(lastT)) { oled.print(lastT, 1); oled.print("C"); }
  else oled.print("---");
  oled.setTextSize(1); oled.setCursor(0, 48);
  if (!isnan(lastH)) { oled.print("H:"); oled.print(lastH, 0); oled.print("%"); }
  oled.print(" Tx:#"); oled.print(sendCount);
  oled.display();
}

// ── Callback koneksi BT ───────────────────────────────────────────────
void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    btConn = true;
    Serial.println("[BT] ✅ Client terhubung — mulai stream sensor.");
    SerialBT.printf("=== Bluino Sensor Monitor ===\r\n");
    SerialBT.printf("Data dikirim tiap %lu detik.\r\n", SEND_INTERVAL / 1000UL);
    SerialBT.println("Ketik GET untuk baca langsung, STATUS untuk info.");
    updateOLED();
  } else if (event == ESP_SPP_CLOSE_EVT) {
    btConn = false;
    Serial.println("[BT] ⚠️ Client terputus.");
    updateOLED();
  }
}

// ── Kirim data sensor ke smartphone ──────────────────────────────────
void sendSensorData() {
  if (!btConn) return;
  sendCount++;
  if (!isnan(lastT)) {
    SerialBT.printf("[DATA] Suhu:%.1fC Lembab:%.0f%% Uptime:%lus\r\n",
      lastT, lastH, millis() / 1000UL);
    Serial.printf("[BT] Tx #%u: %.1fC | %.0f%%\n", sendCount, lastT, lastH);
  } else {
    SerialBT.println("[DATA] ERROR — sensor DHT11 tidak terbaca! Cek kabel IO27.");
    Serial.println("[BT] Tx: sensor error");
  }
  updateOLED();
}

// ── Proses perintah masuk ─────────────────────────────────────────────
void processCommand(String cmd) {
  cmd.trim(); cmd.toUpperCase();
  Serial.printf("[BT] Cmd: '%s'\n", cmd.c_str());

  if (cmd == "GET") {
    // Baca sensor langsung (tidak tunggu interval)
    float t = dht.readTemperature(), h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      lastT = t; lastH = h;
      SerialBT.printf("[GET] Suhu:%.1fC Lembab:%.0f%%\r\n", t, h);
    } else {
      SerialBT.println("[GET] ERROR — gagal baca DHT11.");
    }
  } else if (cmd == "STATUS") {
    SerialBT.printf("[STATUS] Heap:%u Uptime:%lus Tx:%u\r\n",
      ESP.getFreeHeap(), millis() / 1000UL, sendCount);
  } else if (cmd.length() > 0) {
    SerialBT.println("Perintah: GET | STATUS");
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  Wire.begin(21, 22);

  Serial.println("\n=== BAB 52 Program 2: Bluetooth Sensor Monitor ===");

  if (oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledReady = true;
    Serial.println("[OLED] ✅ Display terdeteksi di 0x3C");
  } else {
    Serial.println("[OLED] ⚠️ Display tidak terdeteksi.");
  }

  // Baca sensor awal
  float t = dht.readTemperature(), h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    lastT = t; lastH = h;
    Serial.printf("[DHT] ✅ Baca OK: %.1f°C | %.0f%%\n", lastT, lastH);
  } else {
    Serial.println("[DHT] ⚠️ Gagal baca awal — cek wiring IO27.");
  }

  // Inisiasi Bluetooth
  SerialBT.register_callback(btCallback);
  if (!SerialBT.begin(BT_NAME)) {
    Serial.println("[BT] ❌ Gagal inisiasi Bluetooth!");
    delay(3000); ESP.restart();
  }
  Serial.printf("[BT] ✅ Bluetooth siap! Nama: '%s'\n", BT_NAME);
  Serial.println("[BT] Pair dari HP, connect via Serial BT Terminal.");

  updateOLED();
}

void loop() {
  // Terima perintah dari smartphone
  if (SerialBT.available()) {
    String incoming = SerialBT.readStringUntil('\n');
    processCommand(incoming);
  }

  // Baca DHT11 non-blocking tiap 2 detik
  static unsigned long tDHT = 0;
  if (millis() - tDHT >= 2000UL) {
    tDHT = millis();
    float t = dht.readTemperature(), h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      bool changed = (fabs(t - lastT) > 0.1f || fabs(h - lastH) > 1.0f);
      lastT = t; lastH = h;
      if (changed) updateOLED();
    }
  }

  // Kirim data tiap SEND_INTERVAL
  static unsigned long tSend = 0;
  if (millis() - tSend >= SEND_INTERVAL) {
    tSend = millis();
    sendSensorData();
  }

  // Heartbeat
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] %.1fC %.0f%% | BT:%s | Tx:%u | Heap:%u B | Up:%lu s\n",
      isnan(lastT)?0.0f:lastT, isnan(lastH)?0.0f:lastH,
      btConn?"ON":"OFF", sendCount, ESP.getFreeHeap(), millis()/1000UL);
  }
}
```

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 52 Program 2: Bluetooth Sensor Monitor ===
[OLED] ✅ Display terdeteksi di 0x3C
[DHT] ✅ Baca OK: 29.1°C | 65%
[BT] ✅ Bluetooth siap! Nama: 'Bluino-Sensor'
[BT] Pair dari HP, connect via Serial BT Terminal.
--- (setelah HP connect) ---
[BT] ✅ Client terhubung — mulai stream sensor.
[BT] Tx #1: 29.1C | 65%
[BT] Tx #2: 29.2C | 65%
[BT] Cmd: 'GET'
[BT] Cmd: 'STATUS'
[HB] 29.2C 65% | BT:ON | Tx:14 | Heap:243800 B | Up:61 s
```

> 💡 **Tip — Format Data untuk Parsing:**
> Jika smartphone app kamu butuh data dalam format terstruktur (mis. untuk ditampilkan di grafik), gunakan format CSV: `SerialBT.printf("%.1f,%.0f,%lu\r\n", lastT, lastH, millis()/1000UL);`
> Banyak app Android mendukung auto-parsing CSV untuk membuat grafik real-time.

---

## 52.7 Program 3: Bluetooth Remote Controller — Kontrol Relay via HP

Program paling praktis: **kontrol relay (perangkat listrik) dari smartphone** via Bluetooth. Tanpa internet, tanpa server, langsung peer-to-peer!

### Tujuan Pembelajaran
- Membangun sistem kontrol berbasis perintah teks yang robust
- Memberikan feedback konfirmasi setiap perubahan status
- Menangani edge cases: perintah tidak dikenal, disconnect saat relay ON

### Hardware
- Relay (IO4): dikontrol via perintah Bluetooth
- DHT11 (IO27): baca suhu sebagai feedback konteks
- OLED: tampilkan status relay, koneksi BT, suhu

```cpp
/*
 * BAB 52 - Program 3: Bluetooth Remote Controller — Kontrol Relay
 *
 * Fitur:
 *   ► Kontrol relay IO4 via perintah Bluetooth dari smartphone
 *   ► Perintah: ON, OFF, STATUS, TEMP, HELP, PING
 *   ► Feedback konfirmasi setiap perintah
 *   ► Safety: relay auto-OFF saat Bluetooth terputus
 *   ► OLED tampilkan status relay, BT, suhu real-time
 *   ► Log Serial Monitor untuk debugging
 *   ► Heartbeat ping tiap 30 detik ke smartphone
 *
 * Hardware:
 *   Relay → IO4 (hardwired Bluino Kit)
 *   DHT11 → IO27 (hardwired Bluino Kit)
 *   OLED  → SDA=IO21, SCL=IO22 (hardwired)
 *
 * Perintah Bluetooth (kirim dari "Serial BT Terminal" di HP):
 *   ON     → Hidupkan relay
 *   OFF    → Matikan relay
 *   STATUS → Info relay + suhu + heap
 *   TEMP   → Baca sensor DHT11 sekarang
 *   PING   → Cek koneksi (balas: PONG)
 *   HELP   → Daftar perintah
 */

#include <BluetoothSerial.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Nama Perangkat Bluetooth ──────────────────────────────────────────
const char* BT_NAME = "Bluino-Relay";

// ── Hardware ──────────────────────────────────────────────────────────
#define RELAY_PIN 4
#define DHT_PIN   27
DHT dht(DHT_PIN, DHT11);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool oledReady = false;

// ── Objek Bluetooth ───────────────────────────────────────────────────
BluetoothSerial SerialBT;

// ── State ─────────────────────────────────────────────────────────────
bool     relayState = false;
bool     btConn     = false;
float    lastT      = NAN;
float    lastH      = NAN;
uint32_t cmdCount   = 0;

// ── Kontrol Relay dengan log ──────────────────────────────────────────
void setRelay(bool on) {
  relayState = on;
  digitalWrite(RELAY_PIN, on ? HIGH : LOW);
  Serial.printf("[RELAY] → %s\n", on ? "ON" : "OFF");
}

// ── OLED Helper ───────────────────────────────────────────────────────
void updateOLED() {
  if (!oledReady) return;
  oled.clearDisplay(); oled.setTextColor(WHITE); oled.setTextSize(1);
  oled.setCursor(0, 0); oled.print("BT Remote Control");
  oled.drawLine(0, 10, 127, 10, WHITE);
  oled.setCursor(0, 14);
  oled.print(btConn ? "BT: TERHUBUNG" : "BT: Menunggu...");
  oled.setTextSize(2); oled.setCursor(0, 26);
  oled.print("RLY:");
  oled.print(relayState ? "ON " : "OFF");
  oled.setTextSize(1); oled.setCursor(0, 50);
  if (!isnan(lastT)) { oled.printf("%.1fC %.0f%%", lastT, lastH); }
  oled.print(" Cmd:#"); oled.print(cmdCount);
  oled.display();
}

// ── Callback koneksi BT ───────────────────────────────────────────────
void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
  if (event == ESP_SPP_SRV_OPEN_EVT) {
    btConn = true;
    Serial.println("[BT] ✅ Client terhubung!");
    SerialBT.println("=== Bluino Remote Controller ===");
    SerialBT.printf("Relay saat ini: %s\r\n", relayState ? "ON" : "OFF");
    SerialBT.println("Ketik HELP untuk daftar perintah.");
    updateOLED();
  } else if (event == ESP_SPP_CLOSE_EVT) {
    btConn = false;
    Serial.println("[BT] ⚠️ Client terputus.");
    // Safety: matikan relay saat koneksi terputus
    if (relayState) {
      setRelay(false);
      Serial.println("[SAFETY] Relay auto-OFF karena BT terputus!");
    }
    updateOLED();
  }
}

// ── Proses perintah ───────────────────────────────────────────────────
void processCommand(String cmd) {
  cmd.trim(); cmd.toUpperCase();
  if (cmd.length() == 0) return;

  cmdCount++;
  Serial.printf("[BT] Cmd #%u: '%s'\n", cmdCount, cmd.c_str());

  if (cmd == "ON") {
    setRelay(true);
    SerialBT.println("[OK] Relay → ON ✅");
  } else if (cmd == "OFF") {
    setRelay(false);
    SerialBT.println("[OK] Relay → OFF ✅");
  } else if (cmd == "STATUS") {
    SerialBT.printf("[STATUS] Relay:%s | Suhu:%.1fC | Lembab:%.0f%%\r\n",
      relayState?"ON":"OFF", isnan(lastT)?0.0f:lastT, isnan(lastH)?0.0f:lastH);
    SerialBT.printf("[STATUS] Heap:%u | Uptime:%lus | Cmd:%u\r\n",
      ESP.getFreeHeap(), millis()/1000UL, cmdCount);
  } else if (cmd == "TEMP") {
    float t = dht.readTemperature(), h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      lastT = t; lastH = h;
      SerialBT.printf("[TEMP] Suhu:%.1fC | Lembab:%.0f%%\r\n", t, h);
    } else {
      SerialBT.println("[TEMP] ERROR — gagal baca DHT11.");
    }
  } else if (cmd == "PING") {
    SerialBT.println("PONG");
    Serial.println("[BT] → PONG");
  } else if (cmd == "HELP") {
    SerialBT.println("Perintah tersedia:");
    SerialBT.println("  ON     → hidupkan relay");
    SerialBT.println("  OFF    → matikan relay");
    SerialBT.println("  STATUS → status lengkap");
    SerialBT.println("  TEMP   → baca suhu sekarang");
    SerialBT.println("  PING   → cek koneksi");
    SerialBT.println("  HELP   → daftar ini");
  } else {
    SerialBT.printf("[ERR] Perintah '%s' tidak dikenal. Ketik HELP.\r\n", cmd.c_str());
  }

  updateOLED();
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  setRelay(false);  // Pastikan relay OFF saat boot
  dht.begin();
  Wire.begin(21, 22);

  Serial.println("\n=== BAB 52 Program 3: Bluetooth Remote Controller ===");

  if (oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledReady = true;
    Serial.println("[OLED] ✅ Display terdeteksi di 0x3C");
  } else {
    Serial.println("[OLED] ⚠️ Display tidak terdeteksi.");
  }

  // Baca sensor awal
  float t = dht.readTemperature(), h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    lastT = t; lastH = h;
    Serial.printf("[DHT] ✅ Baca OK: %.1f°C | %.0f%%\n", lastT, lastH);
  }

  // Inisiasi Bluetooth
  SerialBT.register_callback(btCallback);
  if (!SerialBT.begin(BT_NAME)) {
    Serial.println("[BT] ❌ Gagal inisiasi Bluetooth!");
    delay(3000); ESP.restart();
  }
  Serial.printf("[BT] ✅ Bluetooth siap! Nama: '%s'\n", BT_NAME);
  Serial.println("[BT] Pair dari HP → Connect → Ketik ON/OFF untuk kontrol relay.");

  updateOLED();
}

void loop() {
  // Terima perintah dari smartphone
  if (SerialBT.available()) {
    String incoming = SerialBT.readStringUntil('\n');
    processCommand(incoming);
  }

  // Baca DHT11 non-blocking tiap 2 detik
  static unsigned long tDHT = 0;
  if (millis() - tDHT >= 2000UL) {
    tDHT = millis();
    float t = dht.readTemperature(), h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) { lastT = t; lastH = h; updateOLED(); }
  }

  // Heartbeat ping ke smartphone tiap 30 detik
  static unsigned long tHb = 0;
  if (millis() - tHb >= 30000UL) {
    tHb = millis();
    Serial.printf("[HB] Relay:%s | BT:%s | Heap:%u B | Up:%lu s\n",
      relayState?"ON":"OFF", btConn?"ON":"OFF",
      ESP.getFreeHeap(), millis()/1000UL);
    if (btConn) {
      SerialBT.printf("[HB] Relay:%s Heap:%u Up:%lus\r\n",
        relayState?"ON":"OFF", ESP.getFreeHeap(), millis()/1000UL);
    }
  }
}
```

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 52 Program 3: Bluetooth Remote Controller ===
[OLED] ✅ Display terdeteksi di 0x3C
[DHT] ✅ Baca OK: 29.1°C | 65%
[RELAY] → OFF
[BT] ✅ Bluetooth siap! Nama: 'Bluino-Relay'
--- (HP connect) ---
[BT] ✅ Client terhubung!
[BT] Cmd #1: 'ON'
[RELAY] → ON
[BT] Cmd #2: 'STATUS'
[BT] Cmd #3: 'OFF'
[RELAY] → OFF
--- (HP disconnect tiba-tiba saat relay ON) ---
[BT] ⚠️ Client terputus.
[SAFETY] Relay auto-OFF karena BT terputus!
[HB] Relay:OFF | BT:OFF | Heap:243200 B | Up:61 s
```

---

## 52.8 Troubleshooting Bluetooth Classic

| Gejala | Penyebab | Solusi |
|--------|----------|--------|
| "Bluino-Kit" tidak muncul saat scan HP | ESP32 belum menyala / `SerialBT.begin()` belum dipanggil | Pastikan program sudah ter-upload dan `SerialBT.begin()` dipanggil di `setup()` |
| Pair berhasil tapi tidak bisa connect | App Bluetooth di HP harus menggunakan **SPP** (bukan BLE) | Gunakan "Serial Bluetooth Terminal" (bukan app BLE) |
| Data yang diterima HP kosong/tidak lengkap | Baris diakhiri dengan `\n` saja, bukan `\r\n` | Gunakan `SerialBT.println()` atau tambahkan `\r\n` secara manual |
| Callback `btCallback` tidak pernah terpanggil | `register_callback()` dipanggil setelah `begin()` | Pastikan urutan: `register_callback()` DULU → baru `begin()` |
| Bluetooth error: "Already initialized" | `SerialBT.begin()` dipanggil dua kali | Panggil `SerialBT.end()` terlebih dahulu sebelum re-init |
| Compile error: `BluetoothSerial not found` | Board dipilih bukan "ESP32 Dev Module" | Di Arduino IDE: Tools → Board → ESP32 Arduino → **ESP32 Dev Module** |
| Relay tetap ON saat HP disconnect | Tidak ada safety auto-OFF | Tambahkan `setRelay(false)` di event `ESP_SPP_CLOSE_EVT` |
| Perintah dari HP tidak terbaca / terpotong | Newline setting di app berbeda | Di "Serial BT Terminal": Settings → Send → pilih **CR+LF** |

> ⚠️ **POLA WAJIB — Urutan inisiasi Bluetooth yang benar:**
> ```cpp
> BluetoothSerial SerialBT;
>
> void setup() {
>   SerialBT.register_callback(btCallback); // ← 1. Callback DULU
>   SerialBT.begin("NamaDevice");           // ← 2. Baru begin()
> }
>
> // Di dalam callback event:
> void btCallback(esp_spp_cb_event_t event, esp_spp_cb_param_t *param) {
>   if (event == ESP_SPP_SRV_OPEN_EVT)  { /* connected  */ }
>   if (event == ESP_SPP_CLOSE_EVT)     { /* disconnect */ }
> }
> ```

---

## 52.9 Perbandingan Protokol Komunikasi Nirkabel BAB 44-52

| Protokol | BAB | Butuh Router | Jarak | Daya | Cocok Untuk |
|----------|-----|-------------|-------|------|------------|
| WiFi HTTP | 48 | ✅ | 30-100m | Tinggi | REST API, web dashboard |
| WiFi HTTPS | 49 | ✅ | 30-100m | Tinggi | API produksi dengan enkripsi |
| WiFi MQTT | 50 | ✅ | 30-100m | Tinggi | Monitoring IoT real-time |
| WiFi CoAP | 51 | ✅ | 30-100m | Sedang | Sensor baterai, NB-IoT |
| **BT Classic** | **52** | **❌** | **10-30m** | **Sedang** | **Kontrol langsung, data logger** |
| BLE | 53 | ❌ | 10-30m | **Rendah** | Wearable, sensor baterai |

---

## 52.10 Ringkasan

```
RINGKASAN BAB 52 — Bluetooth Classic ESP32

PRINSIP DASAR:
  BluetoothSerial = SPP = "kabel serial virtual via udara"
  Peer-to-peer LANGSUNG — tidak butuh WiFi, router, atau internet!
  Satu ESP32 hanya bisa melayani SATU client BT pada satu waktu.

INISIASI (urutan WAJIB):
  BluetoothSerial SerialBT;
  SerialBT.register_callback(fn);  // ← DULU sebelum begin()!
  SerialBT.begin("NamaDevice");    // ← Mulai Bluetooth

TERIMA DATA (dari smartphone):
  if (SerialBT.available()) {
    String s = SerialBT.readStringUntil('\n');
    s.trim(); s.toUpperCase();  // Normalisasi input
  }

KIRIM DATA (ke smartphone):
  SerialBT.println("teks");           // + newline \r\n
  SerialBT.printf("%.1f C\r\n", t);  // Formatted
  SerialBT.write(buf, len);           // Binary

CEK STATUS:
  SerialBT.connected()  // true jika ada client terhubung

CALLBACK EVENT:
  ESP_SPP_SRV_OPEN_EVT → client terhubung
  ESP_SPP_CLOSE_EVT    → client terputus

SAFETY PATTERN:
  if (event == ESP_SPP_CLOSE_EVT) {
    setRelay(false);  // Auto-OFF saat disconnect!
  }

ANTIPATTERN:
  ❌ register_callback() setelah begin()  → event tidak terpanggil
  ❌ Tidak menambahkan \r\n              → display di app HP rusak
  ❌ Tidak ada safety saat disconnect     → relay bisa stuck ON selamanya
  ❌ Gunakan delay() panjang di loop()   → SerialBT.available() tidak diproses
```

---

## 52.11 Latihan

### Latihan Dasar
1. Upload **Program 1** → pair dari HP → kirim perintah `STATUS` → verifikasi output di Serial Monitor dan di app HP.
2. Ganti `BT_NAME` menjadi namamu sendiri → upload ulang → verifikasi nama baru muncul saat scan HP.
3. Upload **Program 2** → buka app "Serial BT Terminal" → amati data DHT11 yang muncul setiap 5 detik.

### Latihan Menengah
4. Modifikasi **Program 1**: tambahkan perintah `BLINK` yang menyalakan LED onboard (IO2) selama 3 detik, lalu kirim balasan `[OK] LED blink 3 detik`.
5. Modifikasi **Program 2**: ubah interval kirim dari 5 detik menjadi configurable via perintah `INTERVAL 10` (artinya: ubah interval menjadi 10 detik).
6. Upload **Program 3** → kontrol relay via HP → sambil relay ON, putuskan koneksi Bluetooth → verifikasi relay auto-OFF.

### Tantangan Lanjutan
7. **Dual Mode:** Gabungkan Program 2 (stream sensor) dan Program 3 (kontrol relay) dalam satu sketch.
8. **Password Lock:** Tambahkan fitur: perintah `ON`/`OFF` hanya dieksekusi jika sebelumnya sudah mengirim `AUTH 1234`. Jika salah 3x, blacklist client selama 60 detik.
9. **Data Logger:** Kirim data DHT11 ke laptop via Bluetooth Classic → tulis script Python (`pyserial`) yang menyimpan data ke file CSV.

---

## 52.12 Preview: Apa Selanjutnya? (→ BAB 53)

```
BAB 52 (Bluetooth Classic) — SPP, peer-to-peer, cocok untuk data stream:
  ESP32 ── Bluetooth SPP ──► Smartphone/PC
  ✅ Mudah dipairing | ❌ Boros daya | ❌ Hanya 1 client

BAB 53 (BLE — Bluetooth Low Energy) — GATT, hemat daya:
  ESP32 ── BLE GATT ──► Smartphone/Sensor/Wearable
  ✅ Sangat hemat daya | ✅ Multi-client | ✅ Tanpa pairing wajib
  ❌ Lebih kompleks (perlu pahami Service, Characteristic, Descriptor)

Kapan pilih BT Classic vs BLE?
  BT Classic: stream data besar, printer, robot, audio
  BLE       : sensor baterai, wearable, beacon, smart lock
```

---

## 52.13 Referensi

| Sumber | Link |
|--------|------|
| ESP32 BluetoothSerial docs | https://github.com/espressif/arduino-esp32/tree/master/libraries/BluetoothSerial |
| Serial Bluetooth Terminal (Android) | https://play.google.com/store/apps/details?id=de.kai_morich.serial_bluetooth_terminal |
| Bluetooth SPP Profile (IEEE) | https://www.bluetooth.com/specifications/profiles-overview/ |
| ESP32 Bluetooth Architecture | https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/bt.html |

---

## ☕ Dukung Proyek Ini

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

---

*Sebelumnya → [BAB 51: CoAP](../BAB-51/README.md)*

*Selanjutnya → [BAB 53: BLE — Bluetooth Low Energy](../BAB-53/README.md)*
