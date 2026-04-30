# 📡 BAB 54 — ESP-NOW: Komunikasi Nirkabel Antar ESP32 Tanpa Router

> ✅ **Prasyarat:** Selesaikan **BAB 53 (BLE)**. Kamu harus sudah memahami konsep protokol nirkabel peer-to-peer dan pola callback.

> 📖 **Glosarium Singkatan BAB ini:**
> | Singkatan | Kepanjangan |
> |-----------|-------------|
> | **ESP-NOW** | Protokol komunikasi nirkabel proprietary Espressif — tanpa router, tanpa internet |
> | **Peer** | Node ESP32 lain yang sudah terdaftar sebagai tujuan pengiriman |
> | **Unicast** | Pengiriman data ke satu peer spesifik via MAC address |
> | **Broadcast** | Pengiriman data ke semua peer sekaligus (MAC: FF:FF:FF:FF:FF:FF) |
> | **MAC** | *Media Access Control* — ID hardware jaringan unik tiap ESP32 (6 byte) |
> | **Channel** | Kanal frekuensi WiFi (1-13) yang digunakan ESP-NOW — harus sama antar node |
> | **RSSI** | *Received Signal Strength Indicator* — kekuatan sinyal yang diterima (dBm) |
> | **Payload** | Data aktual yang dikirim — maks 250 byte per paket ESP-NOW |
> | **ACK** | *Acknowledgment* — konfirmasi bahwa data berhasil diterima |

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami arsitektur ESP-NOW dan perbedaannya dengan WiFi/BLE.
- Mendaftarkan peer ESP32 menggunakan MAC address dan mengirim data terstruktur (`struct`).
- Menggunakan callback `OnDataSent` dan `OnDataRecv` untuk komunikasi asinkron.
- Membangun topologi unicast, broadcast, dan bidirectional antar ESP32.
- Menerapkan komunikasi sensor-ke-kontroler tanpa router menggunakan struct paket data.

---

## 54.1 Apa itu ESP-NOW?

```
ESP-NOW vs Protokol Lain:

  WiFi HTTP/MQTT:  ESP32 ──WiFi──► Router ──Internet──► Server
  BLE:             ESP32 ──BLE──► Smartphone (GATT)
  ESP-NOW:         ESP32 ──────────────────────────────► ESP32
                   (Langsung! Tanpa router, tanpa internet, tanpa pairing!)

Karakteristik ESP-NOW:
  ► Latency: <1ms (sangat rendah!)
  ► Jangkauan: ~200m (open area), ~30m (dalam ruangan)
  ► Payload maks: 250 byte per paket
  ► Max peer terdaftar: 20 peer
  ► Bisa coexist dengan WiFi (pada channel yang sama)
  ► Tidak butuh infrastruktur apapun — cukup 2+ ESP32!
  ► Komunikasi terenkripsi (opsional, AES-128 CCM)
```

| Aspek | WiFi MQTT | BLE | ESP-NOW |
|-------|-----------|-----|---------|
| **Butuh Router** | ✅ Wajib | ❌ | **❌ Tidak Perlu** |
| **Latency** | ~100ms | ~10ms | **<1ms** |
| **Jangkauan** | 30-100m | 10-30m | **~200m** |
| **Max Payload** | Unlimited | 512 byte (MTU) | **250 byte** |
| **Jumlah Node** | Unlimited | Multi | **Max 20 peer** |
| **Daya** | Tinggi | Rendah | **Rendah** |
| **Cocok Untuk** | IoT Cloud | Sensor HP | **Sensor Mesh, Drone, Robot** |

---

## 54.2 Arsitektur ESP-NOW

```
Topologi yang Didukung ESP-NOW:

  1. UNICAST (Point-to-Point):
     Node-A ──────────────────► Node-B
     (Sender kirim ke 1 MAC address tertentu)

  2. BROADCAST (One-to-Many):
     Node-A ──────────────────► Node-B
               └───────────────► Node-C
               └───────────────► Node-D
     (Sender kirim ke FF:FF:FF:FF:FF:FF — semua Node menerima)

  3. BIDIRECTIONAL (Two-Way):
     Node-A ──── data sensor ──────► Node-B
     Node-A ◄─── ACK + perintah ─── Node-B
     (Keduanya bisa jadi sender & receiver sekaligus)

Alur Kerja ESP-NOW:
  1. Panggil WiFi.mode(WIFI_STA) — wajib sebelum esp_now_init()
  2. esp_now_init() — inisiasi stack ESP-NOW
  3. esp_now_register_send_cb()  — daftarkan callback "setelah kirim"
  4. esp_now_register_recv_cb()  — daftarkan callback "saat terima"
  5. Tambahkan peer via esp_now_add_peer() — masukkan MAC address tujuan
  6. esp_now_send(peerMAC, data, len) — kirim data (asinkron!)
  7. Callback onSent dipanggil → cek status kirim (berhasil/gagal)
  8. Di penerima: callback onRecv dipanggil → proses data

PENTING — Struct Paket Data:
  Sender dan Receiver HARUS menggunakan struct yang IDENTIK!
  Urutan field, tipe data, dan nama harus sama persis!
```

---

## 54.3 Library & API Reference

```
LIBRARY YANG DIGUNAKAN:
  WiFi.h      → BAWAAN ESP32 Core (untuk WiFi.mode & WiFi.macAddress)
  esp_now.h   → BAWAAN ESP32 Core (ESP-NOW API)

  Tidak perlu install library tambahan!

CARA DAPATKAN MAC ADDRESS ESP32 (wajib dicatat sebelum mulai):
  #include <WiFi.h>
  void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    Serial.print("MAC: ");
    Serial.println(WiFi.macAddress()); // Contoh: 24:6F:28:AB:CD:EF
  }

API ESP-NOW ESENSIAL:

  INISIASI (urutan WAJIB):
    WiFi.mode(WIFI_STA);             // 1. Set WiFi mode DULU
    WiFi.disconnect();               // 2. Pastikan tidak connect ke AP
    esp_now_init();                  // 3. Init ESP-NOW
    esp_now_register_send_cb(onSent); // 4. Daftarkan callback kirim
    esp_now_register_recv_cb(onRecv); // 5. Daftarkan callback terima

  TAMBAH PEER:
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, receiverMAC, 6);
    peerInfo.channel = 0;            // 0 = channel saat ini
    peerInfo.encrypt = false;        // true untuk enkripsi AES
    esp_now_add_peer(&peerInfo);

  KIRIM DATA:
    esp_now_send(receiverMAC, (uint8_t*)&myData, sizeof(myData));

  CALLBACK KIRIM (di sender):
    void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
      if (status == ESP_NOW_SEND_SUCCESS) { /* Berhasil */ }
      else { /* Gagal */ }
    }

  CALLBACK TERIMA (di receiver):
    void onRecv(const uint8_t *mac, const uint8_t *data, int len) {
      memcpy(&myData, data, sizeof(myData)); // Copy ke struct lokal
    }

STRUCT PAKET DATA (contoh):
  typedef struct {
    float    temperature;  // 4 byte
    float    humidity;     // 4 byte
    uint32_t uptime;       // 4 byte
    char     label[16];    // 16 byte
  } SensorPacket;          // Total: 28 byte << 250 byte limit
```

---

## 54.4 Cara Mendapatkan MAC Address ESP32

> ⚠️ **WAJIB dilakukan sebelum Program 1!** MAC address setiap ESP32 unik dan berbeda. Kamu harus mengetahuinya sebelum mengisi kode sender/receiver.

Upload sketch kecil ini ke **setiap** ESP32 yang akan digunakan:

```cpp
/*
 * Sketch Utilitas — Tampilkan MAC Address ESP32
 * Upload ke setiap ESP32 yang akan digunakan di BAB 54!
 * Catat MAC address masing-masing sebelum lanjut ke Program 1.
 */
#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  delay(500);
  WiFi.mode(WIFI_STA);
  Serial.println("\n=== MAC Address ESP32 ini ===");
  Serial.print("MAC: ");
  Serial.println(WiFi.macAddress());
  Serial.println("Salin alamat MAC di atas ke sketch sender/receiver!");
}

void loop() {}
```

**Contoh output:**
```
=== MAC Address ESP32 ini ===
MAC: 24:6F:28:AB:CD:EF
```
> 💡 **Konvensi format di kode:** MAC dari `WiFi.macAddress()` berformat `"XX:XX:XX:XX:XX:XX"`. Untuk array byte di kode C++, ubah menjadi: `{0x24, 0x6F, 0x28, 0xAB, 0xCD, 0xEF}`.

---

## 54.5 Program 1: ESP-NOW Unicast — Sensor Sender & Display Receiver

**Topologi:** Satu ESP32 (Sender) membaca DHT11 dan mengirim data ke satu ESP32 (Receiver) setiap 5 detik via unicast. Receiver menampilkan data di OLED.

> ⚠️ **Program 1 terdiri dari DUA sketch terpisah:**
> - `p1_sender/p1_sender.ino` → upload ke **ESP32 #1** (terhubung DHT11)
> - `p1_receiver/p1_receiver.ino` → upload ke **ESP32 #2** (terhubung OLED)

### Tujuan Pembelajaran
- Memahami alur unicast ESP-NOW: struct paket → `esp_now_send()` → callback onSent
- Mendaftarkan peer via MAC address dan mengirim data terstruktur
- Menangkap data di receiver via callback `onRecv` dan `memcpy` ke struct lokal

### Hardware
- **ESP32 #1 (Sender):** DHT11 (IO27)
- **ESP32 #2 (Receiver):** OLED SDA=IO21, SCL=IO22

### Sketch 1A — Sender (`p1_sender.ino`)

```cpp
/*
 * BAB 54 - Program 1A: ESP-NOW Unicast — SENDER
 *
 * Fungsi: Baca DHT11 → kirim struct data ke ESP32 Receiver tiap 5 detik
 *
 * SEBELUM UPLOAD:
 *   Ganti RECEIVER_MAC dengan MAC address ESP32 Receiver kamu!
 *   (Lihat 54.4 untuk cara mendapatkan MAC address)
 *
 * Hardware: DHT11 → IO27
 */

#include <WiFi.h>
#include <esp_now.h>
#include <DHT.h>

#define DHT_PIN 27
DHT dht(DHT_PIN, DHT11);

// ── GANTI dengan MAC address ESP32 Receiver kamu! ─────────────────────
uint8_t RECEIVER_MAC[] = {0x24, 0x6F, 0x28, 0xAB, 0xCD, 0xEF};

// ── Struct paket data — HARUS IDENTIK dengan Receiver! ────────────────
typedef struct {
  float    temperature;
  float    humidity;
  uint32_t uptime;
  uint32_t packetNum;
} SensorPacket;

SensorPacket packet;
uint32_t sendCount  = 0;
bool     lastSendOK = true;

// ── Callback: dipanggil setelah esp_now_send() selesai ────────────────
void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  lastSendOK = (status == ESP_NOW_SEND_SUCCESS);
  Serial.printf("[ESPNOW] Kirim #%u: %s\n",
    sendCount, lastSendOK ? "✅ OK" : "❌ GAGAL");
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  Serial.println("\n=== BAB 54 Program 1A: ESP-NOW Unicast SENDER ===");

  // 1. Set WiFi mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.print("[WiFi] MAC Sender: ");
  Serial.println(WiFi.macAddress());

  // 2. Inisiasi ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESPNOW] ❌ Init gagal! Reboot...");
    ESP.restart();
  }
  Serial.println("[ESPNOW] ✅ ESP-NOW siap");

  // 3. Daftarkan callback kirim
  esp_now_register_send_cb(onSent);

  // 4. Tambahkan Receiver sebagai peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, RECEIVER_MAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("[ESPNOW] ❌ Gagal tambah peer! Cek MAC address.");
  } else {
    Serial.println("[ESPNOW] ✅ Peer Receiver terdaftar");
  }

  // Baca sensor awal
  float t = dht.readTemperature(), h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    Serial.printf("[DHT] ✅ Baca OK: %.1f°C | %.0f%%\n", t, h);
  }

  Serial.println("[ESPNOW] Mulai kirim data tiap 5 detik...");
}

void loop() {
  // Baca DHT11 non-blocking tiap 2 detik
  static unsigned long tDHT = 0;
  static float lastT = NAN, lastH = NAN;
  if (millis() - tDHT >= 2000UL) {
    tDHT = millis();
    float t = dht.readTemperature(), h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) { lastT = t; lastH = h; }
  }

  // Kirim ke Receiver tiap 5 detik
  static unsigned long tSend = 0;
  if (millis() - tSend >= 5000UL) {
    tSend = millis();
    if (!isnan(lastT)) {
      sendCount++;
      packet.temperature = lastT;
      packet.humidity    = lastH;
      packet.uptime      = millis() / 1000UL;
      packet.packetNum   = sendCount;

      esp_now_send(RECEIVER_MAC, (uint8_t*)&packet, sizeof(packet));
      Serial.printf("[ESPNOW] → Kirim #%u: %.1f°C | %.0f%% | Up:%lu s\n",
        sendCount, lastT, lastH, packet.uptime);
    }
  }

  // Heartbeat
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] Sent:%u | LastOK:%s | Heap:%u B | Up:%lu s\n",
      sendCount, lastSendOK?"✅":"❌", ESP.getFreeHeap(), millis()/1000UL);
  }
}
```

**Output Serial Monitor Sender:**
```
=== BAB 54 Program 1A: ESP-NOW Unicast SENDER ===
[WiFi] MAC Sender: 24:6F:28:11:22:33
[ESPNOW] ✅ ESP-NOW siap
[ESPNOW] ✅ Peer Receiver terdaftar
[DHT] ✅ Baca OK: 29.1°C | 65%
[ESPNOW] Mulai kirim data tiap 5 detik...
[ESPNOW] → Kirim #1: 29.1°C | 65% | Up:5 s
[ESPNOW] Kirim #1: ✅ OK
[ESPNOW] → Kirim #2: 29.2°C | 65% | Up:10 s
[ESPNOW] Kirim #2: ✅ OK
```

### Sketch 1B — Receiver (`p1_receiver.ino`)

```cpp
/*
 * BAB 54 - Program 1B: ESP-NOW Unicast — RECEIVER
 *
 * Fungsi: Terima paket sensor dari Sender → tampilkan di OLED & Serial
 *
 * Hardware: OLED → SDA=IO21, SCL=IO22
 * Tidak perlu tahu MAC address Sender (receiver menerima dari siapapun)
 */

#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool oledReady = false;

// ── Struct paket data — HARUS IDENTIK dengan Sender! ──────────────────
typedef struct {
  float    temperature;
  float    humidity;
  uint32_t uptime;
  uint32_t packetNum;
} SensorPacket;

SensorPacket rxPacket;
bool    newDataFlag   = false;
uint32_t recvCount    = 0;
uint8_t  lastSenderMAC[6];

// ── Callback: dipanggil saat data masuk (di FreeRTOS task, bukan loop!) ──
void onRecv(const uint8_t *mac, const uint8_t *data, int len) {
  if (len != sizeof(SensorPacket)) return;  // Tolak paket tidak valid
  memcpy(&rxPacket, data, sizeof(rxPacket));
  memcpy(lastSenderMAC, mac, 6);
  newDataFlag = true;
  recvCount++;
}

void updateOLED() {
  if (!oledReady) return;
  oled.clearDisplay(); oled.setTextColor(WHITE); oled.setTextSize(1);
  oled.setCursor(0, 0);  oled.print("ESP-NOW Receiver");
  oled.drawLine(0, 10, 127, 10, WHITE);
  oled.setCursor(0, 14);
  oled.printf("Pkt #%u | Up:%lu s", rxPacket.packetNum, rxPacket.uptime);
  oled.setTextSize(2); oled.setCursor(0, 26);
  oled.printf("%.1fC", rxPacket.temperature);
  oled.setTextSize(1); oled.setCursor(0, 50);
  oled.printf("H:%.0f%% | Rcv:%u", rxPacket.humidity, recvCount);
  oled.display();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  Serial.println("\n=== BAB 54 Program 1B: ESP-NOW Unicast RECEIVER ===");

  if (oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledReady = true;
    Serial.println("[OLED] ✅ Display terdeteksi di 0x3C");
    oled.clearDisplay(); oled.setTextColor(WHITE); oled.setTextSize(1);
    oled.setCursor(0, 0);  oled.print("ESP-NOW Receiver");
    oled.setCursor(0, 16); oled.print("Menunggu data...");
    oled.display();
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.print("[WiFi] MAC Receiver: ");
  Serial.println(WiFi.macAddress());
  Serial.println("[PETUNJUK] Salin MAC di atas ke RECEIVER_MAC di sketch Sender!");

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESPNOW] ❌ Init gagal! Reboot...");
    ESP.restart();
  }

  esp_now_register_recv_cb(onRecv);
  Serial.println("[ESPNOW] ✅ Menunggu data dari Sender...");
}

void loop() {
  // Proses data baru (flag dari callback) — aman dari race condition
  if (newDataFlag) {
    newDataFlag = false;
    Serial.printf("[ESPNOW] ✅ Terima pkt #%u dari %02X:%02X:%02X:%02X:%02X:%02X\n",
      rxPacket.packetNum,
      lastSenderMAC[0], lastSenderMAC[1], lastSenderMAC[2],
      lastSenderMAC[3], lastSenderMAC[4], lastSenderMAC[5]);
    Serial.printf("         Suhu:%.1f°C | Humi:%.0f%% | Up:%lu s | Total Rcv:%u\n",
      rxPacket.temperature, rxPacket.humidity,
      rxPacket.uptime, recvCount);
    updateOLED();
  }

  // Heartbeat
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] Rcv:%u | Heap:%u B | Up:%lu s\n",
      recvCount, ESP.getFreeHeap(), millis()/1000UL);
  }
}
```

**Output Serial Monitor Receiver:**
```
=== BAB 54 Program 1B: ESP-NOW Unicast RECEIVER ===
[OLED] ✅ Display terdeteksi di 0x3C
[WiFi] MAC Receiver: 24:6F:28:AB:CD:EF
[PETUNJUK] Salin MAC di atas ke RECEIVER_MAC di sketch Sender!
[ESPNOW] ✅ Menunggu data dari Sender...
--- (Sender mengirim paket) ---
[ESPNOW] ✅ Terima pkt #1 dari 24:6F:28:11:22:33
         Suhu:29.1°C | Humi:65% | Up:5 s | Total Rcv:1
[ESPNOW] ✅ Terima pkt #2 dari 24:6F:28:11:22:33
         Suhu:29.2°C | Humi:65% | Up:10 s | Total Rcv:2
```

> ⚠️ **Dua Aturan Kritis ESP-NOW yang Sering Dilupakan:**
> 1. **Struct harus IDENTIK** di Sender dan Receiver — tipe, urutan, dan ukuran field harus sama persis.
> 2. **Callback `onRecv` berjalan di task WiFi, bukan di `loop()`** — jangan lakukan operasi berat di dalam callback. Gunakan flag (`newDataFlag`) dan proses di `loop()`.

---

## 54.6 Program 2: ESP-NOW Broadcast — Satu Sender, Banyak Receiver

**Topologi:** Satu ESP32 Sender mem-broadcast data sensor ke semua node dalam jangkauan tanpa perlu mendaftarkan peer satu per satu. Semua ESP32 yang menjalankan sketch Receiver akan menerima data.

> ⚠️ **Program 2 terdiri dari DUA sketch:**
> - `p2_broadcast_sender/p2_broadcast_sender.ino` → upload ke **ESP32 #1**
> - `p2_broadcast_receiver/p2_broadcast_receiver.ino` → upload ke **ESP32 #2, #3, dst.**

### Tujuan Pembelajaran
- Memahami perbedaan unicast vs broadcast — cukup daftarkan 1 peer `FF:FF:FF:FF:FF:FF`
- Mengirim data ke banyak node sekaligus tanpa mengubah kode sender

### Hardware
- **ESP32 #1 (Sender):** DHT11 (IO27)
- **ESP32 #2+ (Receiver):** OLED atau Serial Monitor

### Sketch 2A — Broadcast Sender (`p2_broadcast_sender.ino`)

```cpp
/*
 * BAB 54 - Program 2A: ESP-NOW Broadcast — SENDER
 *
 * Fungsi: Broadcast data DHT11 ke SEMUA ESP32 dalam jangkauan tiap 3 detik
 * Tidak perlu tahu MAC address Receiver — cukup gunakan broadcast MAC!
 *
 * Hardware: DHT11 → IO27
 */

#include <WiFi.h>
#include <esp_now.h>
#include <DHT.h>

#define DHT_PIN 27
DHT dht(DHT_PIN, DHT11);

// Broadcast MAC — semua ESP32 dalam jangkauan akan menerima
uint8_t BROADCAST_MAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct {
  float    temperature;
  float    humidity;
  uint32_t uptime;
  uint32_t packetNum;
  char     senderID[12]; // Identitas sender (nama node)
} BroadcastPacket;

BroadcastPacket packet;
uint32_t sendCount = 0;

void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.printf("[ESPNOW] Broadcast #%u: %s\n",
    sendCount, status == ESP_NOW_SEND_SUCCESS ? "✅ OK" : "❌ GAGAL");
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  Serial.println("\n=== BAB 54 Program 2A: ESP-NOW Broadcast SENDER ===");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.print("[WiFi] MAC Sender: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESPNOW] ❌ Init gagal! Reboot...");
    ESP.restart();
  }
  esp_now_register_send_cb(onSent);

  // Daftarkan Broadcast MAC sebagai peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, BROADCAST_MAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("[ESPNOW] ❌ Gagal daftarkan broadcast peer!");
  } else {
    Serial.println("[ESPNOW] ✅ Broadcast peer terdaftar (FF:FF:FF:FF:FF:FF)");
  }

  strncpy(packet.senderID, "Sensor-Node1", sizeof(packet.senderID));

  float t = dht.readTemperature(), h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    Serial.printf("[DHT] ✅ Baca OK: %.1f°C | %.0f%%\n", t, h);
  }

  Serial.println("[ESPNOW] Mulai broadcast tiap 3 detik...");
}

void loop() {
  static unsigned long tDHT = 0;
  static float lastT = NAN, lastH = NAN;
  if (millis() - tDHT >= 2000UL) {
    tDHT = millis();
    float t = dht.readTemperature(), h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) { lastT = t; lastH = h; }
  }

  static unsigned long tSend = 0;
  if (millis() - tSend >= 3000UL) {
    tSend = millis();
    if (!isnan(lastT)) {
      sendCount++;
      packet.temperature = lastT;
      packet.humidity    = lastH;
      packet.uptime      = millis() / 1000UL;
      packet.packetNum   = sendCount;

      esp_now_send(BROADCAST_MAC, (uint8_t*)&packet, sizeof(packet));
      Serial.printf("[ESPNOW] → Broadcast #%u: %.1f°C | %.0f%%\n",
        sendCount, lastT, lastH);
    }
  }

  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] Sent:%u | Heap:%u B | Up:%lu s\n",
      sendCount, ESP.getFreeHeap(), millis()/1000UL);
  }
}
```

### Sketch 2B — Broadcast Receiver (`p2_broadcast_receiver.ino`)

```cpp
/*
 * BAB 54 - Program 2B: ESP-NOW Broadcast — RECEIVER
 *
 * Fungsi: Terima broadcast dari Sender manapun → tampilkan di OLED & Serial
 * Upload sketch ini ke SEMUA ESP32 yang ingin menjadi receiver!
 *
 * Hardware: OLED → SDA=IO21, SCL=IO22
 */

#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool oledReady = false;

typedef struct {
  float    temperature;
  float    humidity;
  uint32_t uptime;
  uint32_t packetNum;
  char     senderID[12];
} BroadcastPacket;

BroadcastPacket rxPacket;
bool     newDataFlag = false;
uint32_t recvCount   = 0;

void onRecv(const uint8_t *mac, const uint8_t *data, int len) {
  if (len != sizeof(BroadcastPacket)) return;
  memcpy(&rxPacket, data, sizeof(rxPacket));
  newDataFlag = true;
  recvCount++;
}

void updateOLED() {
  if (!oledReady) return;
  oled.clearDisplay(); oled.setTextColor(WHITE); oled.setTextSize(1);
  oled.setCursor(0, 0);  oled.print("ESP-NOW Broadcast");
  oled.drawLine(0, 10, 127, 10, WHITE);
  oled.setCursor(0, 14);
  oled.printf("From: %s #%u", rxPacket.senderID, rxPacket.packetNum);
  oled.setTextSize(2); oled.setCursor(0, 26);
  oled.printf("%.1fC", rxPacket.temperature);
  oled.setTextSize(1); oled.setCursor(0, 50);
  oled.printf("H:%.0f%% Rcv:%u", rxPacket.humidity, recvCount);
  oled.display();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  Serial.println("\n=== BAB 54 Program 2B: ESP-NOW Broadcast RECEIVER ===");

  if (oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledReady = true;
    Serial.println("[OLED] ✅ Display terdeteksi di 0x3C");
    oled.clearDisplay(); oled.setTextColor(WHITE);
    oled.setCursor(0, 0); oled.print("ESP-NOW Broadcast");
    oled.setCursor(0, 16); oled.print("Menunggu data...");
    oled.display();
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.print("[WiFi] MAC Receiver ini: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESPNOW] ❌ Init gagal! Reboot...");
    ESP.restart();
  }

  esp_now_register_recv_cb(onRecv);
  Serial.println("[ESPNOW] ✅ Siap terima broadcast...");
}

void loop() {
  if (newDataFlag) {
    newDataFlag = false;
    Serial.printf("[ESPNOW] ✅ Broadcast pkt #%u dari '%s': %.1f°C | %.0f%% | Up:%lu s\n",
      rxPacket.packetNum, rxPacket.senderID,
      rxPacket.temperature, rxPacket.humidity, rxPacket.uptime);
    updateOLED();
  }

  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] Rcv:%u | Heap:%u B | Up:%lu s\n",
      recvCount, ESP.getFreeHeap(), millis()/1000UL);
  }
}
```

**Output Serial Monitor Receiver:**
```
=== BAB 54 Program 2B: ESP-NOW Broadcast RECEIVER ===
[OLED] ✅ Display terdeteksi di 0x3C
[WiFi] MAC Receiver ini: 24:6F:28:AA:BB:CC
[ESPNOW] ✅ Siap terima broadcast...
[ESPNOW] ✅ Broadcast pkt #1 dari 'Sensor-Node1': 29.1°C | 65% | Up:3 s
[ESPNOW] ✅ Broadcast pkt #2 dari 'Sensor-Node1': 29.2°C | 65% | Up:6 s
```

---

## 54.7 Program 3: ESP-NOW Bidirectional — Sensor Node + Controller Node

**Topologi:** Dua ESP32 saling berkomunikasi dua arah. Node Sensor mengirim data DHT11 ke Node Controller. Node Controller membalas ACK + perintah relay. Komunikasi penuh tanpa router!

> ⚠️ **Program 3 terdiri dari DUA sketch:**
> - `p3_node_sensor/p3_node_sensor.ino` → upload ke **ESP32 #1** (ada DHT11)
> - `p3_node_controller/p3_node_controller.ino` → upload ke **ESP32 #2** (ada Relay)

### Tujuan Pembelajaran
- Membangun komunikasi dua arah — kedua node menjadi sender DAN receiver
- Mengirim ACK + perintah balik dari controller ke sensor node
- Menerapkan logic: Controller auto-matikan relay jika tidak menerima data sensor selama 15 detik (watchdog)

### Hardware
- **ESP32 #1 (Sensor Node):** DHT11 (IO27), OLED
- **ESP32 #2 (Controller Node):** Relay (IO4), OLED

### Sketch 3A — Sensor Node (`p3_node_sensor.ino`)

```cpp
/*
 * BAB 54 - Program 3A: ESP-NOW Bidirectional — SENSOR NODE
 *
 * Fungsi:
 *   ► Kirim data DHT11 ke Controller Node tiap 3 detik
 *   ► Terima ACK + status relay dari Controller
 *   ► OLED tampilkan data lokal + status relay di controller
 *
 * SEBELUM UPLOAD:
 *   Ganti CONTROLLER_MAC dengan MAC address ESP32 Controller!
 *
 * Hardware: DHT11 → IO27 | OLED → SDA=IO21, SCL=IO22
 */

#include <WiFi.h>
#include <esp_now.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define DHT_PIN 27
DHT dht(DHT_PIN, DHT11);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool oledReady = false;

// ── GANTI dengan MAC address ESP32 Controller! ────────────────────────
uint8_t CONTROLLER_MAC[] = {0x24, 0x6F, 0x28, 0xAA, 0xBB, 0xCC};

// ── Struct data sensor (Sensor → Controller) ──────────────────────────
typedef struct {
  float    temperature;
  float    humidity;
  uint32_t uptime;
  uint32_t seqNum;
} SensorData;

// ── Struct ACK (Controller → Sensor) ─────────────────────────────────
typedef struct {
  bool     relayState;
  uint32_t ackSeqNum;
  char     message[24];
} ControllerACK;

SensorData  txData;
ControllerACK rxACK;
bool     newACKFlag   = false;
bool     remoteRelay  = false;
uint32_t sendCount    = 0;
uint32_t lastSendOK   = 0;

void onSent(const uint8_t *mac, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) lastSendOK = sendCount;
  Serial.printf("[ESPNOW] Kirim #%u: %s\n",
    sendCount, status == ESP_NOW_SEND_SUCCESS ? "✅ OK" : "❌ GAGAL");
}

void onRecv(const uint8_t *mac, const uint8_t *data, int len) {
  if (len != sizeof(ControllerACK)) return;
  memcpy(&rxACK, data, sizeof(rxACK));
  remoteRelay = rxACK.relayState;
  newACKFlag  = true;
}

void updateOLED(float t, float h) {
  if (!oledReady) return;
  oled.clearDisplay(); oled.setTextColor(WHITE); oled.setTextSize(1);
  oled.setCursor(0, 0);  oled.print("Sensor Node");
  oled.drawLine(0, 10, 127, 10, WHITE);
  oled.setCursor(0, 14);
  oled.printf("Sent:#%u OK:#%u", sendCount, lastSendOK);
  oled.setTextSize(2); oled.setCursor(0, 26);
  oled.printf("%.1fC", t);
  oled.setTextSize(1); oled.setCursor(0, 50);
  oled.printf("H:%.0f%%  RLY:%s", h, remoteRelay ? "ON" : "OFF");
  oled.display();
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  Wire.begin(21, 22);

  Serial.println("\n=== BAB 54 Program 3A: ESP-NOW Bidirectional — SENSOR NODE ===");

  if (oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledReady = true;
    Serial.println("[OLED] ✅ Display terdeteksi di 0x3C");
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.print("[WiFi] MAC Sensor Node: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESPNOW] ❌ Init gagal! Reboot...");
    ESP.restart();
  }

  esp_now_register_send_cb(onSent);
  esp_now_register_recv_cb(onRecv);

  // Daftarkan Controller sebagai peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, CONTROLLER_MAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) == ESP_OK) {
    Serial.println("[ESPNOW] ✅ Controller peer terdaftar");
  } else {
    Serial.println("[ESPNOW] ❌ Gagal daftarkan Controller! Cek MAC.");
  }

  float t = dht.readTemperature(), h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    Serial.printf("[DHT] ✅ Baca OK: %.1f°C | %.0f%%\n", t, h);
  }
}

void loop() {
  static unsigned long tDHT = 0;
  static float lastT = NAN, lastH = NAN;
  if (millis() - tDHT >= 2000UL) {
    tDHT = millis();
    float t = dht.readTemperature(), h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) { lastT = t; lastH = h; }
  }

  // Kirim data ke Controller tiap 3 detik
  static unsigned long tSend = 0;
  if (millis() - tSend >= 3000UL) {
    tSend = millis();
    if (!isnan(lastT)) {
      sendCount++;
      txData.temperature = lastT;
      txData.humidity    = lastH;
      txData.uptime      = millis() / 1000UL;
      txData.seqNum      = sendCount;

      esp_now_send(CONTROLLER_MAC, (uint8_t*)&txData, sizeof(txData));
      Serial.printf("[ESPNOW] → Kirim #%u: %.1f°C | %.0f%%\n",
        sendCount, lastT, lastH);
      updateOLED(lastT, lastH);
    }
  }

  // Proses ACK dari Controller
  if (newACKFlag) {
    newACKFlag = false;
    Serial.printf("[ESPNOW] ← ACK #%u: Relay=%s | '%s'\n",
      rxACK.ackSeqNum, rxACK.relayState ? "ON" : "OFF", rxACK.message);
    if (!isnan(lastT)) updateOLED(lastT, lastH);
  }

  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] Sent:%u | OKd:%u | Heap:%u B | Up:%lu s\n",
      sendCount, lastSendOK, ESP.getFreeHeap(), millis()/1000UL);
  }
}
```

### Sketch 3B — Controller Node (`p3_node_controller.ino`)

```cpp
/*
 * BAB 54 - Program 3B: ESP-NOW Bidirectional — CONTROLLER NODE
 *
 * Fungsi:
 *   ► Terima data sensor dari Sensor Node
 *   ► Kontrol relay berdasarkan suhu (auto ON jika >30°C, OFF jika <=30°C)
 *   ► Kirim ACK + status relay balik ke Sensor Node
 *   ► Watchdog: relay auto-OFF jika tidak ada data sensor selama 15 detik
 *   ► OLED tampilkan data sensor + status relay
 *
 * SEBELUM UPLOAD:
 *   Ganti SENSOR_MAC dengan MAC address ESP32 Sensor Node!
 *
 * Hardware: Relay → IO4 | OLED → SDA=IO21, SCL=IO22
 */

#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define RELAY_PIN 4
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool oledReady = false;

// ── GANTI dengan MAC address ESP32 Sensor Node! ───────────────────────
uint8_t SENSOR_MAC[] = {0x24, 0x6F, 0x28, 0x11, 0x22, 0x33};

typedef struct {
  float    temperature;
  float    humidity;
  uint32_t uptime;
  uint32_t seqNum;
} SensorData;

typedef struct {
  bool     relayState;
  uint32_t ackSeqNum;
  char     message[24];
} ControllerACK;

SensorData  rxData;
ControllerACK txACK;
bool     newDataFlag  = false;
bool     relayState   = false;
uint32_t recvCount    = 0;
unsigned long lastRecvTime = 0;

void setRelay(bool on, const char* reason) {
  relayState = on;
  digitalWrite(RELAY_PIN, on ? HIGH : LOW);
  Serial.printf("[RELAY] → %s (%s)\n", on ? "ON" : "OFF", reason);
}

void onSent(const uint8_t *mac, esp_now_send_status_t status) {
  Serial.printf("[ESPNOW] ACK balik: %s\n",
    status == ESP_NOW_SEND_SUCCESS ? "✅ OK" : "❌ GAGAL");
}

void onRecv(const uint8_t *mac, const uint8_t *data, int len) {
  if (len != sizeof(SensorData)) return;
  memcpy(&rxData, data, sizeof(rxData));
  lastRecvTime = millis();
  newDataFlag  = true;
  recvCount++;
}

void updateOLED() {
  if (!oledReady) return;
  oled.clearDisplay(); oled.setTextColor(WHITE); oled.setTextSize(1);
  oled.setCursor(0, 0);  oled.print("Controller Node");
  oled.drawLine(0, 10, 127, 10, WHITE);
  oled.setCursor(0, 14);
  oled.printf("Rcv:#%u RLY:%s", recvCount, relayState ? "ON" : "OFF");
  oled.setTextSize(2); oled.setCursor(0, 26);
  oled.printf("%.1fC", rxData.temperature);
  oled.setTextSize(1); oled.setCursor(0, 50);
  oled.printf("H:%.0f%% Seq:%u", rxData.humidity, rxData.seqNum);
  oled.display();
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  Wire.begin(21, 22);

  Serial.println("\n=== BAB 54 Program 3B: ESP-NOW Bidirectional — CONTROLLER NODE ===");

  if (oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledReady = true;
    Serial.println("[OLED] ✅ Display terdeteksi di 0x3C");
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.print("[WiFi] MAC Controller Node: ");
  Serial.println(WiFi.macAddress());
  Serial.println("[PETUNJUK] Salin MAC di atas ke CONTROLLER_MAC di sketch Sensor Node!");

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESPNOW] ❌ Init gagal! Reboot...");
    ESP.restart();
  }

  esp_now_register_send_cb(onSent);
  esp_now_register_recv_cb(onRecv);

  // Daftarkan Sensor Node sebagai peer (untuk kirim ACK balik)
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, SENSOR_MAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) == ESP_OK) {
    Serial.println("[ESPNOW] ✅ Sensor Node peer terdaftar");
  } else {
    Serial.println("[ESPNOW] ❌ Gagal daftarkan Sensor! Cek MAC.");
  }

  lastRecvTime = millis();
  Serial.println("[ESPNOW] ✅ Siap terima data sensor...");
}

void loop() {
  // Proses data baru dari Sensor Node
  if (newDataFlag) {
    newDataFlag = false;
    Serial.printf("[ESPNOW] ← Terima #%u: %.1f°C | %.0f%% | Up:%lu s\n",
      rxData.seqNum, rxData.temperature, rxData.humidity, rxData.uptime);

    // Logic otomatis: relay ON jika suhu > 30°C (contoh kontrol kipas/AC)
    bool shouldOn = (rxData.temperature > 30.0f);
    if (shouldOn != relayState) {
      setRelay(shouldOn, rxData.temperature > 30.0f ? "Suhu>30C" : "Suhu<=30C");
    }

    // Kirim ACK + status relay balik ke Sensor Node
    txACK.relayState = relayState;
    txACK.ackSeqNum  = rxData.seqNum;
    snprintf(txACK.message, sizeof(txACK.message),
      "T>30=%s", relayState ? "ON" : "OFF");
    esp_now_send(SENSOR_MAC, (uint8_t*)&txACK, sizeof(txACK));

    updateOLED();
  }

  // Watchdog: auto-OFF relay jika tidak ada data sensor 15 detik
  if (relayState && (millis() - lastRecvTime > 15000UL)) {
    setRelay(false, "Watchdog timeout!");
    Serial.println("[SAFETY] ⚠️ Relay auto-OFF — tidak ada data sensor 15 detik!");
    updateOLED();
  }

  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] Rcv:%u | Relay:%s | Heap:%u B | Up:%lu s\n",
      recvCount, relayState?"ON":"OFF", ESP.getFreeHeap(), millis()/1000UL);
  }
}
```

**Output Serial Monitor Controller:**
```
=== BAB 54 Program 3B: ESP-NOW Bidirectional — CONTROLLER NODE ===
[OLED] ✅ Display terdeteksi di 0x3C
[WiFi] MAC Controller Node: 24:6F:28:AA:BB:CC
[PETUNJUK] Salin MAC di atas ke CONTROLLER_MAC di sketch Sensor Node!
[ESPNOW] ✅ Sensor Node peer terdaftar
[ESPNOW] ✅ Siap terima data sensor...
--- (Sensor Node mengirim data) ---
[ESPNOW] ← Terima #1: 29.1°C | 65% | Up:3 s
[ESPNOW] ACK balik: ✅ OK
[ESPNOW] ← Terima #8: 31.2°C | 62% | Up:24 s
[RELAY] → ON (Suhu>30C)
[ESPNOW] ACK balik: ✅ OK
--- (Sensor Node mati tiba-tiba) ---
[SAFETY] ⚠️ Relay auto-OFF — tidak ada data sensor 15 detik!
[HB] Rcv:10 | Relay:OFF | Heap:236000 B | Up:61 s
```

---

## 54.8 Troubleshooting ESP-NOW

| Gejala | Penyebab | Solusi |
|--------|----------|--------|
| `esp_now_init()` return error | `WiFi.mode()` belum dipanggil sebelum init | Pastikan `WiFi.mode(WIFI_STA)` dipanggil SEBELUM `esp_now_init()` |
| Callback `onSent` selalu `❌ GAGAL` | MAC address Receiver salah / Receiver tidak nyala | Verifikasi MAC dengan sketch utilitas 54.4; pastikan Receiver sudah diupload & menyala |
| Struct tidak terbaca / nilai aneh | Struct di Sender dan Receiver tidak identik | Salin struct yang persis sama ke kedua sketch — tipe, urutan, ukuran field harus sama |
| `onRecv` tidak pernah dipanggil | `esp_now_register_recv_cb()` belum dipanggil | Pastikan registrasi callback ada di `setup()` sebelum loop berjalan |
| ESP-NOW berhenti bekerja saat WiFi connect | Channel WiFi berubah, tidak sama dengan channel ESP-NOW | Paksa channel WiFi ke channel yang sama: gunakan `WiFi.begin()` dengan channel tertentu |
| Data terpotong / korup | Payload > 250 byte | Kurangi ukuran struct; periksa dengan `sizeof(myStruct)` |
| `esp_now_add_peer()` return error | Peer sudah pernah didaftarkan (double register) | Cek dengan `esp_now_is_peer_exist()` sebelum `add_peer()` |
| Compile error: `esp_now.h` not found | Board bukan ESP32 | Tools → Board → **ESP32 Arduino → ESP32 Dev Module** |

> ⚠️ **POLA WAJIB — Urutan inisiasi ESP-NOW yang benar:**
> ```cpp
> WiFi.mode(WIFI_STA);                     // 1. Set mode WiFi DULU
> WiFi.disconnect();                       // 2. Lepas dari AP jika ada
> esp_now_init();                          // 3. Init ESP-NOW
> esp_now_register_send_cb(onSent);        // 4. Callback kirim
> esp_now_register_recv_cb(onRecv);        // 5. Callback terima
>
> esp_now_peer_info_t peerInfo = {};       // 6. Siapkan struct peer
> memcpy(peerInfo.peer_addr, peerMAC, 6); // 7. Isi MAC address
> peerInfo.channel = 0;                   // 8. Channel (0 = auto)
> peerInfo.encrypt = false;               // 9. Enkripsi (opsional)
> esp_now_add_peer(&peerInfo);            // 10. Daftarkan peer
>
> // Di dalam callback onRecv — JANGAN lakukan operasi berat!
> void onRecv(const uint8_t *mac, const uint8_t *data, int len) {
>   memcpy(&myStruct, data, sizeof(myStruct)); // Hanya copy data
>   newDataFlag = true;                        // Set flag saja!
> }
> // Proses data di loop(), bukan di dalam callback!
> ```

> 💡 **Catatan Kompatibilitas ESP32 Core Versi Baru (>=3.0):**
> Pada ESP32 Arduino Core versi 3.0+, signature callback `onRecv` berubah:
> ```cpp
> // Core 2.x (umum digunakan):
> void onRecv(const uint8_t *mac, const uint8_t *data, int len)
>
> // Core 3.x (baru):
> void onRecv(const esp_now_recv_info_t *recv_info, const uint8_t *data, int len)
> ```
> Jika compile error pada callback, periksa versi core di Arduino IDE: Tools → Board → Board Manager → ESP32 → versi yang terinstall.

---

## 54.9 Perbandingan Protokol Komunikasi Nirkabel BAB 44-54

| Protokol | BAB | Butuh Router | Jarak | Daya | Latency | Cocok Untuk |
|----------|-----|-------------|-------|------|---------|------------|
| WiFi HTTP | 48 | ✅ | 30-100m | Tinggi | ~100ms | REST API, web dashboard |
| WiFi MQTT | 50 | ✅ | 30-100m | Tinggi | ~50ms | Monitoring IoT real-time |
| WiFi CoAP | 51 | ✅ | 30-100m | Sedang | ~50ms | Sensor baterai, NB-IoT |
| BT Classic | 52 | ❌ | 10-30m | Sedang | ~10ms | Stream data, kontrol serial |
| BLE | 53 | ❌ | 10-30m | Rendah | ~10ms | Wearable, sensor baterai |
| **ESP-NOW** | **54** | **❌** | **~200m** | **Rendah** | **<1ms** | **Sensor mesh, drone, robot** |

---

## 54.10 Ringkasan

```
RINGKASAN BAB 54 — ESP-NOW Komunikasi Antar ESP32

PRINSIP DASAR:
  ESP-NOW = komunikasi nirkabel proprietary Espressif
  Tidak butuh router, internet, atau pairing — cukup MAC address!
  Max payload: 250 byte per paket | Max peer: 20 node

URUTAN INISIASI (WAJIB):
  WiFi.mode(WIFI_STA) → WiFi.disconnect() →
  esp_now_init() → register_send_cb() → register_recv_cb() →
  add_peer(MAC) → esp_now_send()

STRUCT DATA (paling penting!):
  typedef struct { float t; float h; uint32_t up; } MyData;
  // Struct HARUS identik di Sender dan Receiver!
  esp_now_send(peerMAC, (uint8_t*)&myData, sizeof(myData));

TERIMA DATA (di callback — HANYA copy & flag!):
  void onRecv(const uint8_t *mac, const uint8_t *data, int len) {
    memcpy(&rxData, data, sizeof(rxData)); // Copy saja
    newDataFlag = true;                   // Flag untuk loop()
  }
  // Proses data di loop(), BUKAN di dalam callback!

TOPOLOGI:
  Unicast   : esp_now_send(peerMAC, ...)  → 1 node tujuan
  Broadcast : esp_now_send(FF:FF:..., ...) → semua node
  Dua Arah  : Kedua node jadi sender + receiver sekaligus

SAFETY PATTERN (Watchdog):
  if (relayState && millis() - lastRecvTime > 15000UL) {
    setRelay(false); // Auto-OFF jika tidak ada data 15 detik!
  }

ANTIPATTERN:
  ❌ Lupa WiFi.mode(WIFI_STA) sebelum init → esp_now_init() gagal
  ❌ Struct berbeda antara Sender & Receiver → data korup/tidak terbaca
  ❌ Operasi berat di dalam onRecv callback → crash / data loss
  ❌ Payload > 250 byte → data terpotong
  ❌ Tidak ada watchdog → relay stuck ON saat sensor node mati
```

---

## 54.11 Latihan

### Latihan Dasar
1. Upload **sketch utilitas 54.4** ke kedua ESP32 → catat MAC address masing-masing.
2. Upload **Program 1A** (Sender) dan **Program 1B** (Receiver) → verifikasi data DHT11 muncul di OLED Receiver setiap 5 detik.
3. Ganti `strncpy(packet.senderID, "Sensor-Node1", ...)` dengan namamu sendiri di Program 2A → verifikasi nama muncul di display Receiver.

### Latihan Menengah
4. Modifikasi **Program 1 Sender**: tambahkan field `bool alarm` di struct — kirim `alarm=true` jika suhu > 35°C, `alarm=false` jika tidak. Receiver tampilkan "⚠️ PANAS!" di OLED saat `alarm=true`.
5. Modifikasi **Program 2 Receiver**: hitung berapa node unik yang mengirim broadcast (track unique MAC addresses) dan tampilkan jumlahnya di Serial Monitor.
6. Upload **Program 3** → pastikan relay di Controller menyala otomatis saat suhu > 30°C → matikan Sensor Node → verifikasi watchdog auto-OFF relay setelah 15 detik.

### Tantangan Lanjutan
7. **ESP-NOW + WiFi Coexistence:** Buat sketch yang menjalankan ESP-NOW (terima data sensor dari node lain) DAN WiFi MQTT (forward data ke Antares) secara bersamaan — ESP32 berfungsi sebagai gateway!
8. **Multi-Sensor Mesh (3 Node):** Tiga ESP32 masing-masing dengan sensor DHT11 dan `senderID` berbeda. Semua broadcast ke satu Gateway Node yang mengumpulkan dan menampilkan data ketiganya di Serial Monitor.
9. **ESP-NOW ACK + Retry:** Implementasikan mekanisme retry manual — jika callback `onSent` mengembalikan status gagal, simpan paket dan coba kirim ulang hingga 3 kali sebelum log error.

---

## 54.12 Preview: Apa Selanjutnya? (→ BAB 55)

```
BAB 54 (ESP-NOW) — Point-to-Point atau One-to-Many tanpa router:
  ESP32 ──────────────────────────► ESP32
  ✅ Ultra-low latency | ✅ Tanpa router | ❌ Max 20 peer | ❌ Max 250 byte

BAB 55 (ESP-MESH) — Jaringan mesh self-healing yang lebih besar:
  ESP32 ──► ESP32 ──► ESP32 ──► ESP32 ──► (Root Node → Internet)
  ✅ Bisa ratusan node | ✅ Self-healing | ✅ Routing otomatis
  ✅ Koneksi ke internet via Root Node
  ❌ Lebih kompleks (butuh pahami topologi mesh & routing)

Kapan pilih ESP-NOW vs ESP-MESH?
  ESP-NOW  : Jaringan kecil (<20 node), latency kritis, tanpa internet
  ESP-MESH : Jaringan besar, butuh jangkauan luas, atau butuh cloud
```

---

## 54.13 Referensi

| Sumber | Link |
|--------|------|
| ESP-NOW API Reference (Espressif) | https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/network/esp_now.html |
| ESP-NOW Arduino Examples | https://github.com/espressif/arduino-esp32/tree/master/libraries/ESP32/examples/ESPNow |
| ESP-NOW Protocol Overview | https://www.espressif.com/en/solutions/low-power-solutions/esp-now |
| MAC Address ESP32 Guide | https://randomnerdtutorials.com/get-change-esp32-esp8266-mac-address-arduino/ |

---

## ☕ Dukung Proyek Ini

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

---

*Sebelumnya → [BAB 53: BLE — Bluetooth Low Energy](../BAB-53/README.md)*

*Selanjutnya → [BAB 55: ESP-MESH](../BAB-55/README.md)*
