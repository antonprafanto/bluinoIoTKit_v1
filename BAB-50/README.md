# 📡 BAB 50 — MQTT: Message Queuing Telemetry Transport

## Konsep yang Dipelajari

- Arsitektur Publish-Subscribe (berbeda total dengan Request-Response HTTP/HTTPS)
- Komponen MQTT: Broker, Publisher, Subscriber, Topic
- Quality of Service (QoS) levels — 0, 1, 2
- Last Will & Testament (LWT) — deteksi perangkat offline otomatis
- Implementasi MQTT di ESP32 menggunakan library `PubSubClient`
- Integrasi dengan **Antares.id** — Platform IoT Indonesia berbasis oneM2M

---

## 🧪 Perangkat & Library yang Dibutuhkan

### Hardware
| Komponen | Pin |
|---|---|
| ESP32 DEVKIT V1 | — |
| DHT11 | IO27 (hardwired) |
| Relay | IO4 (hardwired) |
| OLED 128×64 I2C | SDA=IO21, SCL=IO22 (hardwired) |

### Library (Install via Arduino Library Manager)
| Library | Penulis | Keterangan |
|---------|---------|-----------|
| PubSubClient | Nick O'Leary | Library MQTT untuk Arduino/ESP32 |
| ArduinoJson | >= 6.x | Parse & build JSON payload |
| Adafruit SSD1306 | Adafruit | Driver OLED |
| Adafruit GFX Library | Adafruit | Dependensi OLED |
| DHT sensor library | Adafruit | Baca DHT11 (P1, P2 & P3) |

> **Library Bawaan ESP32 Core (tidak perlu install):**
> `WiFi.h`, `WiFiClientSecure.h`

---

## 📡 Arsitektur MQTT: Publish-Subscribe

```
         ┌─────────────────────────────────────────┐
         │          MQTT Broker (Antares)           │
         │         platform.antares.id:1338         │
         └──────────┬──────────────────┬────────────┘
                    │                  │
         PUBLISH    │                  │   SUBSCRIBE
    (kirim data)    ▼                  ▼   (terima data)
         ┌──────────────┐    ┌─────────────────────┐
         │   ESP32 #1   │    │  Dashboard Antares  │
         │  (Sensor +   │    │  / App lain yang    │
         │   Relay IoT) │    │  subscribe topik    │
         └──────────────┘    └─────────────────────┘
```

### Perbedaan HTTP vs MQTT

| Aspek | HTTP/HTTPS (BAB 48-49) | MQTT (BAB 50) |
|-------|----------------------|---------------|
| Model | Request-Response | Publish-Subscribe |
| Inisiator | Client selalu memulai | Broker mendistribusikan |
| Koneksi | Dibuka-ditutup tiap request | **Persisten** — tetap terhubung |
| Real-time | ❌ Polling berkala | ✅ Push instan ke subscriber |
| Overhead | Header besar per request | Header sangat kecil (2 byte min) |
| Cocok untuk | REST API, upload data berkala | Monitoring real-time, kontrol IoT |

---

## 🏗️ Komponen MQTT

### 1. Broker
Server pusat yang menerima dan mendistribusikan semua pesan. Di BAB 50 kita menggunakan **Antares.id** sebagai broker.

### 2. Topic (Topik)
Alamat pesan dalam MQTT. Format hierarki mirip path file:
```
bluino/sensor/suhu          ← topik sederhana
bluino/+/data               ← wildcard satu level (single-level)
bluino/#                    ← wildcard semua level (multi-level)
```

Di Antares, format topik mengikuti standar **oneM2M**:
```
Publish : /oneM2M/req/{ACCESS_KEY}/antares-cse/{APP}/{DEVICE}/json
Subscribe: /oneM2M/resp/antares-cse/{ACCESS_KEY}/json
```

### 3. Publisher
Perangkat yang mengirim pesan ke topik tertentu. ESP32 kita berperan sebagai Publisher saat mengirim data sensor.

### 4. Subscriber
Perangkat/aplikasi yang mendaftar untuk menerima pesan dari topik tertentu. Dashboard Antares.id berperan sebagai Subscriber yang menerima data sensor kita.

### 5. Quality of Service (QoS)
| QoS | Nama | Jaminan |
|-----|------|---------|
| 0 | At most once | Pesan dikirim sekali, mungkin hilang |
| 1 | At least once | Pesan dijamin terkirim, mungkin duplikat |
| 2 | Exactly once | Pesan dijamin tepat sekali (overhead tertinggi) |

> `PubSubClient` mendukung QoS 0 secara default — cukup untuk monitoring sensor IoT.

---

## 🔗 Konfigurasi Antares MQTT

### Parameter Koneksi
| Parameter | Nilai |
|-----------|-------|
| Host | `platform.antares.id` |
| Port | `1338` |
| Username | Access Key Antares Anda |
| Password | Access Key Antares Anda |
| Client ID | `"BluinoKit-" + 6 digit MAC address` |

### Format Topik Antares (oneM2M)
```
Publish ke Antares:
  /oneM2M/req/{ACCESS_KEY}/antares-cse/{APP}/{DEVICE}/json

Subscribe dari Antares (terima response):
  /oneM2M/resp/antares-cse/{ACCESS_KEY}/json
```

### Format Payload Publish (oneM2M `m2m:cin`)
```json
{
  "m2m:cin": {
    "con": "{\"temp\":29.1,\"humidity\":65}"
  }
}
```

### Cara Daftar / Siapkan Antares (jika belum dari BAB 49)
1. Buka [https://antares.id](https://antares.id) → Daftar (gratis!)
2. Login → Klik profil (kanan atas) → Salin **Access Key**
3. Buat **Application** (misal: `bluino-kit`)
4. Di dalam Application → buat **Device** (misal: `sensor-ruangan`)

---

## 📊 Program 1: MQTT Publish — Kirim Data DHT11

### Konsep
ESP32 membaca DHT11 lalu **mempublish** data sensor (suhu & kelembaban) ke Antares sebagai payload JSON dalam format oneM2M. Data langsung muncul di Dashboard Antares secara real-time.

### Tujuan Pembelajaran
- Memahami cara kerja Publisher dalam arsitektur MQTT
- Implementasi non-blocking reconnect ke broker
- Membangun payload JSON dengan ArduinoJson (stack only)
- Memonitor data di Dashboard Antares.id

### Hardware
- DHT11 (IO27): baca suhu & kelembaban
- OLED: tampilkan suhu, kelembaban, dan jumlah publish

### Kode Program
> Lihat file: [`p1_mqtt_publish/p1_mqtt_publish.ino`](p1_mqtt_publish/p1_mqtt_publish.ino)

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 50 Program 1: MQTT Publish — Antares.id ===
[INFO] Broker: platform.antares.id:1338
[INFO] App: bluino-kit | Device: sensor-ruangan
[OLED] ✅ Display terdeteksi di 0x3C
[WiFi] Menghubungkan ke 'WiFiRumah'...
[WiFi] ✅ Terhubung! IP: 192.168.1.105 | SSID: WiFiRumah
[MQTT] Menghubungkan ke broker Antares... (ID: BluinoKit-A1B2C3)
[MQTT] ✅ Terhubung! Client ID: BluinoKit-A1B2C3
[DHT] ✅ Baca OK: 29.1°C | 65%
[INFO] Publish tiap 15 detik ke Antares
[DHT] 29.1°C | 65%
[MQTT] Publish #1 → /oneM2M/req/.../json
[MQTT] ✅ Publish berhasil! Heap: 195200 B
[HB] 29.1C 65% | Pub: 1 | Heap: 194800 B
```

---

## 🔧 Program 2: MQTT Subscribe — Kontrol Relay

### Konsep
ESP32 **subscribe** ke topik Antares. Saat sebuah perintah dikirim dari dashboard Antares (atau perangkat lain) ke topik tersebut, ESP32 menerimanya dan mengeksekusi perintah: mengaktifkan atau mematikan Relay (IO4).

### Tujuan Pembelajaran
- Memahami cara kerja Subscriber dalam arsitektur MQTT
- Implementasi callback function `onMqttMessage()`
- Parse payload JSON response oneM2M dari Antares
- Kontrol aktuator (Relay) via MQTT — pola "remote control IoT"

### Hardware
- Relay (IO4): dikendalikan via pesan MQTT
- OLED: tampilkan status koneksi dan status relay terakhir

### Cara Mengirim Perintah dari Antares Dashboard
1. Login ke [https://antares.id](https://antares.id)
2. Masuk ke Application → Device Anda
3. Klik **Send Message** → isi payload dalam format oneM2M:
   ```json
   {"m2m:cin":{"con":"{\"relay\":\"ON\"}"}}
   ```
   atau untuk mematikan:
   ```json
   {"m2m:cin":{"con":"{\"relay\":\"OFF\"}"}}
   ```
4. Relay ESP32 langsung bereaksi!

### Kode Program
> Lihat file: [`p2_mqtt_subscribe/p2_mqtt_subscribe.ino`](p2_mqtt_subscribe/p2_mqtt_subscribe.ino)

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 50 Program 2: MQTT Subscribe — Antares.id ===
[INFO] Broker: platform.antares.id:1338
[OLED] ✅ Display terdeteksi di 0x3C
[WiFi] Menghubungkan ke 'WiFiRumah'...
[WiFi] ✅ Terhubung! IP: 192.168.1.105 | SSID: WiFiRumah
[MQTT] Menghubungkan ke broker Antares... (ID: BluinoKit-A1B2C3)
[MQTT] ✅ Terhubung!
[MQTT] ✅ Subscribe: /oneM2M/resp/antares-cse/.../json
[MQTT] 📨 Pesan masuk di topik: /oneM2M/resp/antares-cse/.../json
[MQTT] Payload: {"m2m:rsp":{"pc":{"m2m:cin":{"con":"{\"relay\":\"ON\"}"}},...}}
[RELAY] ✅ Relay → ON
[HB] Relay: ON | Msg: 1 | Heap: 196400 B | Uptime: 60 s
```

---

## ☁️ Program 3: MQTT Full IoT — Publish + Subscribe + LWT

### Konsep
Menggabungkan P1 & P2 menjadi pola IoT industri sempurna: ESP32 **sekaligus** mengirim data sensor (Publisher) **dan** menerima perintah kontrol relay (Subscriber), dilengkapi **Last Will & Testament (LWT)** untuk deteksi kondisi offline otomatis.

### Tentang Last Will & Testament (LWT)
LWT adalah pesan yang dititipkan ESP32 ke broker **saat pertama kali terhubung**. Jika ESP32 putus koneksi secara tidak normal (mati mendadak, kehilangan WiFi), broker otomatis mempublish pesan LWT ini ke topik status.

```
Skenario Normal:
  ESP32 terhubung → publish "ONLINE" ke:
  /oneM2M/req/{KEY}/antares-cse/{APP}/status/json
  ESP32 disconnect normal → tidak ada LWT

Skenario Darurat (mati mendadak):
  Broker mendeteksi koneksi terputus tanpa disconnect proper
  Broker otomatis publish "OFFLINE" ke topik status yang sama → semua subscriber tahu!
```

### Tujuan Pembelajaran
- Arsitektur IoT penuh: Pub + Sub + LWT
- Pemantauan ketersediaan perangkat (availability monitoring)
- Pola reconnect yang robust dan non-blocking

### Hardware
- DHT11 (IO27): publish data sensor
- Relay (IO4): dikontrol via subscribe
- OLED: tampilkan status broker, suhu, status relay

### Kode Program
> Lihat file: [`p3_mqtt_full/p3_mqtt_full.ino`](p3_mqtt_full/p3_mqtt_full.ino)

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 50 Program 3: MQTT Full IoT — Pub + Sub + LWT ===
[INFO] Broker: platform.antares.id:1338
[INFO] App: bluino-kit | Device: sensor-ruangan
[OLED] ✅ Display terdeteksi di 0x3C
[WiFi] Menghubungkan ke 'WiFiRumah'...
[WiFi] ✅ Terhubung! IP: 192.168.1.105 | SSID: WiFiRumah
[MQTT] Menghubungkan ke broker Antares... (ID: BluinoKit-A1B2C3)
[MQTT] ✅ Terhubung! LWT aktif di topik status
[MQTT] Publish ONLINE → topik status
[MQTT] ✅ Subscribe: /oneM2M/resp/antares-cse/.../json
[DHT] ✅ Baca OK: 29.1°C | 65%
[INFO] Publish tiap 15 detik | LWT aktif
[DHT] 29.1°C | 65%
[MQTT] Publish #1 → data sensor
[MQTT] ✅ Publish berhasil! Heap: 193200 B
[MQTT] 📨 Pesan masuk di topik: /oneM2M/resp/antares-cse/.../json
[RELAY] ✅ Relay → ON
[HB] 29.1C 65% | Relay: ON | Pub: 1 | Heap: 193200 B
```

---

## 🔑 Rangkuman Konsep MQTT

| Konsep | Penjelasan |
|--------|-----------|
| **Broker** | Pusat distribusi pesan — semua pub/sub melewati broker |
| **Topic** | Alamat pesan — hierarki dengan `/` sebagai pemisah |
| **Publisher** | Pengirim pesan ke topik |
| **Subscriber** | Penerima pesan dari topik |
| **QoS** | Jaminan pengiriman pesan (0/1/2) |
| **LWT** | Pesan otomatis broker saat publisher putus tidak normal |
| **Retained** | Pesan terakhir disimpan broker untuk subscriber baru |
| **Keep-alive** | Interval ping ESP32 → broker untuk menjaga koneksi aktif |

---

## 🔗 Referensi

- [Antares MQTT Documentation](https://antares.id/id/docs.html)
- [PubSubClient Library](https://github.com/knolleary/pubsubclient)
- [MQTT Specification v3.1.1](https://mqtt.org/mqtt-specification/)
- [ArduinoJson Documentation](https://arduinojson.org)
