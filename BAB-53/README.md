# 📡 BAB 53 — BLE (Bluetooth Low Energy): Sensor Nirkabel Hemat Daya

> ✅ **Prasyarat:** Selesaikan **BAB 52 (Bluetooth Classic)**. Kamu harus sudah memahami konsep pairing BT, callback koneksi, dan pola non-blocking dengan `millis()`.

> 📖 **Glosarium Singkatan BAB ini:**
> | Singkatan | Kepanjangan |
> |-----------|-------------|
> | **BLE** | *Bluetooth Low Energy* — versi Bluetooth hemat daya (Bluetooth 4.0+) |
> | **GATT** | *Generic Attribute Profile* — protokol lapisan atas BLE untuk baca/tulis data |
> | **UUID** | *Universally Unique Identifier* — ID unik 128-bit untuk Service/Characteristic |
> | **Service** | Kumpulan Characteristic terkait (mis. Environmental Sensing Service) |
> | **Characteristic** | Unit data terkecil dalam GATT — nilai yang bisa dibaca/ditulis/dinotifikasi |
> | **CCCD** | *Client Characteristic Configuration Descriptor* — diaktifkan client untuk enable NOTIFY |
> | **Peripheral** | Perangkat BLE yang mengiklankan diri (ESP32 di BAB ini) |
> | **Central** | Perangkat yang terhubung ke Peripheral (smartphone) |
> | **MTU** | *Maximum Transmission Unit* — ukuran paket BLE maksimum (default 23 byte) |
> | **Advertising** | Proses ESP32 menyiarkan keberadaannya agar bisa ditemukan Central |

---

## 📌 Tujuan Pembelajaran

Setelah mempelajari bab ini, kamu akan mampu:
- Memahami arsitektur GATT (Service → Characteristic → Descriptor) pada BLE.
- Menggunakan library `BLEDevice.h` bawaan ESP32 untuk membuat GATT Server.
- Mengimplementasikan properti **NOTIFY** untuk push data sensor ke smartphone.
- Mengimplementasikan properti **WRITE** untuk menerima perintah dari smartphone.
- Membangun ESP32 BLE node lengkap dengan multi-characteristic dan safety pattern.

---

## 53.1 BLE vs Bluetooth Classic — Perbedaan Fundamental

```
Bluetooth Classic (BAB 52):          BLE (BAB 53):
  Model: SPP / Serial Stream            Model: GATT (Service-Characteristic)
  Pairing: WAJIB sebelum kirim          Pairing: Opsional (bisa langsung connect)
  Kecepatan: hingga 3 Mbps             Kecepatan: 125kbps - 2 Mbps
  Konsumsi: ~30mA saat aktif           Konsumsi: ~15mA peak, ~2uA sleep
  Library: BluetoothSerial.h           Library: BLEDevice.h (bawaan ESP32)
  Cocok: stream data besar, audio       Cocok: sensor baterai, wearable, beacon
```

| Aspek | Bluetooth Classic | BLE |
|-------|------------------|-----|
| **Konsumsi Daya** | Sedang (~30mA) | **Sangat Rendah** (~2µA sleep) |
| **Pairing** | Wajib | Opsional |
| **Model Data** | Stream (UART) | Terstruktur (GATT) |
| **Multi-Client** | ❌ 1 client | ✅ Bisa multi |
| **Library ESP32** | BluetoothSerial.h | **BLEDevice.h** |
| **App Testing** | Serial BT Terminal | **nRF Connect** |

---

## 53.2 Arsitektur GATT — Service & Characteristic

```
Hirarki GATT ESP32 (BAB 53):

  ESP32 BLE Server (Peripheral)
  └── Service (UUID: 4fafc201-...)       ← Kelompok fungsionalitas
      ├── Characteristic: Sensor Data    ← Unit data individual
      │   ├── Properties: READ, NOTIFY   ← Apa yang bisa dilakukan
      │   ├── Value: {"t":29.1,"h":65}   ← Data aktual
      │   └── Descriptor (CCCD 0x2902)   ← Aktifkan NOTIFY dari client
      │
      └── Characteristic: Relay Control
          ├── Properties: READ, WRITE
          └── Value: "OFF"

  Alur NOTIFY (server push ke client):
  1. Client subscribe ke Characteristic → tulis CCCD → enable notification
  2. ESP32 panggil pChar->notify() → data dikirim otomatis ke client
  3. Tidak perlu client polling! Sangat hemat bandwidth & daya.

  Alur WRITE (client push ke server):
  1. Client tulis nilai baru ke Characteristic
  2. BLECharacteristicCallbacks::onWrite() terpanggil di ESP32
  3. ESP32 proses nilai → aksi (ON/OFF relay, ubah interval, dll)
```

---

## 53.3 Library & API Reference

```
LIBRARY YANG DIGUNAKAN:
  BLEDevice.h  → BAWAAN ESP32 Arduino Core (tidak perlu install!)
  BLEServer.h  → Bawaan ESP32 Core
  BLEUtils.h   → Bawaan ESP32 Core
  BLE2902.h    → Bawaan ESP32 Core — CCCD Descriptor untuk NOTIFY

API BLE ESENSIAL:

  INISIASI:
    BLEDevice::init("NamaDevice");
    BLEServer* pSrv = BLEDevice::createServer();
    pSrv->setCallbacks(new MyServerCallbacks());

  BUAT SERVICE & CHARACTERISTIC:
    BLEService* pSvc = pSrv->createService(SERVICE_UUID);
    BLECharacteristic* pChar = pSvc->createCharacteristic(
      CHAR_UUID,
      BLECharacteristic::PROPERTY_READ   |
      BLECharacteristic::PROPERTY_NOTIFY |
      BLECharacteristic::PROPERTY_WRITE
    );
    pChar->addDescriptor(new BLE2902()); // ← WAJIB untuk NOTIFY!
    pChar->setCallbacks(new MyCharCb()); // ← Untuk menangani WRITE
    pChar->setValue("nilai_awal");
    pSvc->start();

  NOTIFY DATA KE CLIENT:
    pChar->setValue((uint8_t*)buf, strlen(buf)); // Set nilai (binary safe)
    pChar->notify();                             // Push ke semua subscriber

  ADVERTISING:
    BLEAdvertising* pAdv = BLEDevice::getAdvertising();
    pAdv->addServiceUUID(SERVICE_UUID);
    pAdv->setScanResponse(true);
    pAdv->setMinPreferred(0x06);   // Kompatibilitas iPhone
    BLEDevice::startAdvertising();

  CALLBACK SERVER (connect/disconnect):
    class MyCallbacks : public BLEServerCallbacks {
      void onConnect(BLEServer* pSrv) override    { /* connected  */ }
      void onDisconnect(BLEServer* pSrv) override {
        delay(500);
        pSrv->startAdvertising(); // ← WAJIB restart setelah disconnect!
      }
    };

  CALLBACK CHARACTERISTIC (WRITE dari client):
    class MyCharCb : public BLECharacteristicCallbacks {
      void onWrite(BLECharacteristic* pChar) override {
        std::string val = pChar->getValue();  // Baca nilai yang dikirim
        // Konversi ke Arduino String:
        String cmd = String(val.c_str());
        cmd.trim(); cmd.toUpperCase();
      }
    };
```

---

## 53.4 Setup Aplikasi Smartphone untuk Testing BLE

### Android & iOS — nRF Connect (Disarankan)
1. Install **"nRF Connect for Mobile"** di Play Store / App Store (gratis, by Nordic Semiconductor).
2. Buka app → tab **SCANNER** → klik **SCAN**.
3. Temukan **"Bluino-BLE"** → klik **CONNECT**.
4. Buka tab **CLIENT** → expand service → lihat characteristics.
5. Klik ikon **panah bawah** (subscribe) untuk aktifkan NOTIFY — data sensor akan muncul otomatis.
6. Klik ikon **panah atas** (write) lalu ketik **"ON"** atau **"OFF"** (pilih format UTF-8) untuk kontrol relay.

> 💡 **Alternatif:** Gunakan **"LightBlue"** (iOS/Android) sebagai pengganti nRF Connect.

---

## 53.5 Program 1: BLE Notify — Stream Data DHT11 ke Smartphone

ESP32 sebagai BLE Peripheral yang secara otomatis mengirim data sensor DHT11 setiap 5 detik ke smartphone yang berlangganan (subscribe). Tanpa polling, tanpa internet!

### Tujuan Pembelajaran
- Memahami alur GATT: Service → Characteristic → CCCD → Notify
- Mempraktikkan `BLEDevice::init()`, `createService()`, `createCharacteristic()`
- Memahami pentingnya `BLE2902` descriptor dan `startAdvertising()` setelah disconnect

### Hardware
- DHT11 (IO27): sumber data sensor
- OLED: tampilkan suhu, kelembaban, status BLE, counter notify

```cpp
/*
 * BAB 53 - Program 1: BLE Notify — Stream Data DHT11 ke Smartphone
 *
 * Fitur:
 *   ► ESP32 sebagai BLE Peripheral (GATT Server)
 *   ► 1 Service dengan 1 Characteristic (READ + NOTIFY)
 *   ► Notify JSON data DHT11 tiap 5 detik ke smartphone
 *   ► Callback onConnect / onDisconnect — auto-restart advertising
 *   ► OLED tampilkan suhu, kelembaban, status BLE, counter notify
 *   ► Non-blocking DHT11 read tiap 2 detik
 *   ► Heartbeat tiap 60 detik
 *
 * Hardware:
 *   DHT11 → IO27 (hardwired Bluino Kit)
 *   OLED  → SDA=IO21, SCL=IO22 (hardwired)
 *
 * Cara Uji (nRF Connect):
 *   1. Upload sketch → buka Serial Monitor 115200
 *   2. nRF Connect → SCAN → "Bluino-BLE" → CONNECT
 *   3. Expand service → klik panah bawah (Subscribe) pada Characteristic
 *   4. Data DHT11 muncul otomatis tiap 5 detik!
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── UUID (generate: https://www.uuidgenerator.net/) ───────────────────
#define SERVICE_UUID     "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHAR_SENSOR_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// ── Hardware ──────────────────────────────────────────────────────────
#define DHT_PIN 27
DHT dht(DHT_PIN, DHT11);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool oledReady = false;

// ── BLE Objects ───────────────────────────────────────────────────────
BLEServer*         pServer     = nullptr;
BLECharacteristic* pSensorChar = nullptr;

// ── State ─────────────────────────────────────────────────────────────
bool     deviceConnected = false;
float    lastT           = NAN;
float    lastH           = NAN;
uint32_t notifyCount     = 0;

// ── OLED Helper ───────────────────────────────────────────────────────
void updateOLED() {
  if (!oledReady) return;
  oled.clearDisplay(); oled.setTextColor(WHITE); oled.setTextSize(1);
  oled.setCursor(0, 0);  oled.print("BLE Sensor Notify");
  oled.drawLine(0, 10, 127, 10, WHITE);
  oled.setCursor(0, 14);
  oled.print(deviceConnected ? "BLE: TERHUBUNG" : "BLE: Advertising...");
  oled.setTextSize(2); oled.setCursor(0, 26);
  if (!isnan(lastT)) { oled.print(lastT, 1); oled.print("C"); }
  else oled.print("---");
  oled.setTextSize(1); oled.setCursor(0, 50);
  if (!isnan(lastH)) { oled.print("H:"); oled.print(lastH, 0); oled.print("%"); }
  oled.print(" Ntf:#"); oled.print(notifyCount);
  oled.display();
}

// ── Server Callbacks ──────────────────────────────────────────────────
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pSrv) override {
    deviceConnected = true;
    Serial.println("[BLE] ✅ Client terhubung!");
    updateOLED();
  }
  void onDisconnect(BLEServer* pSrv) override {
    deviceConnected = false;
    Serial.println("[BLE] ⚠️ Client terputus — restart advertising...");
    delay(500);
    pSrv->startAdvertising(); // WAJIB! Tanpa ini tidak bisa connect lagi
    updateOLED();
  }
};

void setup() {
  Serial.begin(115200);
  dht.begin();
  Wire.begin(21, 22);

  Serial.println("\n=== BAB 53 Program 1: BLE Notify — DHT11 ===");

  if (oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledReady = true;
    Serial.println("[OLED] ✅ Display terdeteksi di 0x3C");
  } else {
    Serial.println("[OLED] ⚠️ Display tidak terdeteksi.");
  }

  float t = dht.readTemperature(), h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    lastT = t; lastH = h;
    Serial.printf("[DHT] ✅ Baca OK: %.1f°C | %.0f%%\n", lastT, lastH);
  }

  // ── Inisiasi BLE ──────────────────────────────────────────────────
  BLEDevice::init("Bluino-BLE");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);

  pSensorChar = pService->createCharacteristic(
    CHAR_SENSOR_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  pSensorChar->addDescriptor(new BLE2902()); // WAJIB untuk NOTIFY!
  pSensorChar->setValue("Menunggu sensor...");

  pService->start();

  BLEAdvertising* pAdv = BLEDevice::getAdvertising();
  pAdv->addServiceUUID(SERVICE_UUID);
  pAdv->setScanResponse(true);
  pAdv->setMinPreferred(0x06);
  BLEDevice::startAdvertising();

  Serial.println("[BLE] ✅ Advertising dimulai. Nama: 'Bluino-BLE'");
  Serial.println("[BLE] nRF Connect → SCAN → 'Bluino-BLE' → CONNECT → Subscribe");
  updateOLED();
}

void loop() {
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

  // Notify tiap 5 detik
  static unsigned long tNotify = 0;
  if (millis() - tNotify >= 5000UL) {
    tNotify = millis();
    if (deviceConnected && !isnan(lastT)) {
      char payload[80];
      snprintf(payload, sizeof(payload),
        "{\"temp\":%.1f,\"humidity\":%.0f,\"uptime\":%lu}",
        lastT, lastH, millis() / 1000UL);
      pSensorChar->setValue((uint8_t*)payload, strlen(payload));
      pSensorChar->notify();
      notifyCount++;
      Serial.printf("[BLE] Notify #%u: %s\n", notifyCount, payload);
      updateOLED();
    }
  }

  // Heartbeat
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] BLE:%s | Ntf:%u | Heap:%u B | Up:%lu s\n",
      deviceConnected ? "ON" : "OFF", notifyCount,
      ESP.getFreeHeap(), millis() / 1000UL);
  }
}
```

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 53 Program 1: BLE Notify — DHT11 ===
[OLED] ✅ Display terdeteksi di 0x3C
[DHT] ✅ Baca OK: 29.1°C | 65%
[BLE] ✅ Advertising dimulai. Nama: 'Bluino-BLE'
[BLE] nRF Connect → SCAN → 'Bluino-BLE' → CONNECT → Subscribe
--- (setelah smartphone connect & subscribe) ---
[BLE] ✅ Client terhubung!
[BLE] Notify #1: {"temp":29.1,"humidity":65,"uptime":12}
[BLE] Notify #2: {"temp":29.2,"humidity":65,"uptime":17}
[HB] BLE:ON | Ntf:12 | Heap:192000 B | Up:61 s
--- (smartphone disconnect) ---
[BLE] ⚠️ Client terputus — restart advertising...
```

> ⚠️ **Dua Aturan Kritis BLE yang Sering Dilupakan:**
> 1. **`addDescriptor(new BLE2902())`** — WAJIB pada Characteristic NOTIFY. Tanpa ini, NOTIFY tidak terkirim meski `notify()` dipanggil.
> 2. **`pServer->startAdvertising()` di `onDisconnect()`** — WAJIB agar ESP32 bisa di-connect lagi setelah client disconnect.

---

## 53.6 Program 2: BLE Read + Write — Kontrol Relay dari Smartphone

ESP32 menyediakan dua Characteristic: satu untuk membaca data sensor (READ + NOTIFY) dan satu untuk menulis perintah relay (READ + WRITE). Seluruh interaksi bisa dilakukan dari nRF Connect.

### Tujuan Pembelajaran
- Membangun multi-Characteristic GATT server (sensor + relay)
- Menangani WRITE dari client via `BLECharacteristicCallbacks`
- Menerapkan safety pattern: relay auto-OFF saat BLE disconnect

### Hardware
- DHT11 (IO27): data sensor dibaca dan di-notify
- Relay (IO4): dikontrol via WRITE dari smartphone
- OLED: tampilkan status relay, koneksi BLE, suhu

```cpp
/*
 * BAB 53 - Program 2: BLE Read + Write — Kontrol Relay dari Smartphone
 *
 * Fitur:
 *   ► 1 Service dengan 2 Characteristic:
 *       - Sensor Data (READ + NOTIFY): JSON suhu & kelembaban tiap 5 detik
 *       - Relay Control (READ + WRITE): tulis "ON"/"OFF"/"TOGGLE" dari nRF Connect
 *   ► Safety: relay auto-OFF saat BLE client disconnect
 *   ► Callback WRITE: BLECharacteristicCallbacks::onWrite()
 *   ► OLED tampilkan status relay, BLE, suhu
 *   ► Heartbeat tiap 60 detik
 *
 * Hardware:
 *   DHT11 → IO27 | Relay → IO4 | OLED → SDA=IO21, SCL=IO22
 *
 * Cara Uji (nRF Connect):
 *   1. CONNECT ke "Bluino-Relay-BLE"
 *   2. Subscribe panah bawah pada Char Sensor → data suhu muncul tiap 5 detik
 *   3. Klik panah atas pada Char Relay → ketik "ON" (UTF-8) → SEND → relay ON!
 *   4. Ketik "OFF" → relay OFF | Ketik "TOGGLE" → toggle status relay
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── UUID ──────────────────────────────────────────────────────────────
#define SERVICE_UUID      "6e400001-b5a3-f393-e0a9-e50e24dcca9e"
#define CHAR_SENSOR_UUID  "6e400002-b5a3-f393-e0a9-e50e24dcca9e"  // READ+NOTIFY
#define CHAR_RELAY_UUID   "6e400003-b5a3-f393-e0a9-e50e24dcca9e"  // READ+WRITE

// ── Hardware ──────────────────────────────────────────────────────────
#define RELAY_PIN 4
#define DHT_PIN   27
DHT dht(DHT_PIN, DHT11);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool oledReady = false;

// ── BLE Objects ───────────────────────────────────────────────────────
BLEServer*         pServer     = nullptr;
BLECharacteristic* pSensorChar = nullptr;
BLECharacteristic* pRelayChar  = nullptr;

// ── State ─────────────────────────────────────────────────────────────
bool     deviceConnected = false;
bool     relayState      = false;
float    lastT           = NAN;
float    lastH           = NAN;
uint32_t notifyCount     = 0;
uint32_t writeCount      = 0;

// ── Kontrol Relay ─────────────────────────────────────────────────────
void setRelay(bool on) {
  relayState = on;
  digitalWrite(RELAY_PIN, on ? HIGH : LOW);
  // Update nilai Characteristic agar client bisa READ status terkini
  pRelayChar->setValue(on ? "ON" : "OFF");
  Serial.printf("[RELAY] → %s\n", on ? "ON" : "OFF");
}

// ── OLED Helper ───────────────────────────────────────────────────────
void updateOLED() {
  if (!oledReady) return;
  oled.clearDisplay(); oled.setTextColor(WHITE); oled.setTextSize(1);
  oled.setCursor(0, 0);  oled.print("BLE Read+Write");
  oled.drawLine(0, 10, 127, 10, WHITE);
  oled.setCursor(0, 14);
  oled.print(deviceConnected ? "BLE: TERHUBUNG" : "BLE: Advertising...");
  oled.setTextSize(2); oled.setCursor(0, 26);
  oled.print("RLY:");
  oled.print(relayState ? "ON " : "OFF");
  oled.setTextSize(1); oled.setCursor(0, 50);
  if (!isnan(lastT)) { oled.printf("%.1fC %.0f%%", lastT, lastH); }
  oled.print(" Ntf:#"); oled.print(notifyCount);
  oled.display();
}

// ── Callback: WRITE pada Characteristic Relay ─────────────────────────
class RelayCharCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) override {
    std::string val = pChar->getValue();
    if (val.length() == 0) return;

    String cmd = String(val.c_str());
    cmd.trim(); cmd.toUpperCase();
    writeCount++;
    Serial.printf("[BLE] Write #%u: '%s'\n", writeCount, cmd.c_str());

    if (cmd == "ON") {
      setRelay(true);
    } else if (cmd == "OFF") {
      setRelay(false);
    } else if (cmd == "TOGGLE") {
      setRelay(!relayState);
    } else {
      Serial.printf("[BLE] ⚠️ Perintah tidak dikenal: '%s' (ON/OFF/TOGGLE)\n", cmd.c_str());
    }
    updateOLED();
  }
};

// ── Server Callbacks ──────────────────────────────────────────────────
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pSrv) override {
    deviceConnected = true;
    Serial.println("[BLE] ✅ Client terhubung!");
    updateOLED();
  }
  void onDisconnect(BLEServer* pSrv) override {
    deviceConnected = false;
    Serial.println("[BLE] ⚠️ Client terputus — restart advertising...");
    // Safety: matikan relay saat BLE disconnect
    if (relayState) {
      setRelay(false);
      Serial.println("[SAFETY] Relay auto-OFF karena BLE terputus!");
    }
    delay(500);
    pSrv->startAdvertising();
    updateOLED();
  }
};

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  dht.begin();
  Wire.begin(21, 22);

  Serial.println("\n=== BAB 53 Program 2: BLE Read+Write — Relay ===");

  if (oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledReady = true;
    Serial.println("[OLED] ✅ Display terdeteksi di 0x3C");
  } else {
    Serial.println("[OLED] ⚠️ Display tidak terdeteksi.");
  }

  float t = dht.readTemperature(), h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    lastT = t; lastH = h;
    Serial.printf("[DHT] ✅ Baca OK: %.1f°C | %.0f%%\n", lastT, lastH);
  }

  // ── Inisiasi BLE ──────────────────────────────────────────────────
  BLEDevice::init("Bluino-Relay-BLE");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService* pService = pServer->createService(SERVICE_UUID);

  // Characteristic 1: Sensor Data (READ + NOTIFY)
  pSensorChar = pService->createCharacteristic(
    CHAR_SENSOR_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  pSensorChar->addDescriptor(new BLE2902()); // WAJIB untuk NOTIFY
  pSensorChar->setValue("Menunggu sensor...");

  // Characteristic 2: Relay Control (READ + WRITE)
  pRelayChar = pService->createCharacteristic(
    CHAR_RELAY_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );
  pRelayChar->setCallbacks(new RelayCharCallbacks()); // Tangani WRITE
  pRelayChar->setValue("OFF"); // Nilai awal

  pService->start();

  BLEAdvertising* pAdv = BLEDevice::getAdvertising();
  pAdv->addServiceUUID(SERVICE_UUID);
  pAdv->setScanResponse(true);
  pAdv->setMinPreferred(0x06);
  BLEDevice::startAdvertising();

  Serial.println("[BLE] ✅ Advertising: 'Bluino-Relay-BLE'");
  Serial.println("[BLE] nRF Connect → CONNECT → Subscribe sensor → Write relay ON/OFF");
  updateOLED();
}

void loop() {
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

  // Notify sensor tiap 5 detik
  static unsigned long tNotify = 0;
  if (millis() - tNotify >= 5000UL) {
    tNotify = millis();
    if (deviceConnected && !isnan(lastT)) {
      char payload[96];
      snprintf(payload, sizeof(payload),
        "{\"temp\":%.1f,\"humidity\":%.0f,\"relay\":\"%s\",\"uptime\":%lu}",
        lastT, lastH, relayState ? "ON" : "OFF", millis() / 1000UL);
      pSensorChar->setValue((uint8_t*)payload, strlen(payload));
      pSensorChar->notify();
      notifyCount++;
      Serial.printf("[BLE] Notify #%u: %s\n", notifyCount, payload);
      updateOLED();
    }
  }

  // Heartbeat
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] BLE:%s | Relay:%s | Ntf:%u | Wr:%u | Heap:%u B | Up:%lu s\n",
      deviceConnected?"ON":"OFF", relayState?"ON":"OFF",
      notifyCount, writeCount, ESP.getFreeHeap(), millis()/1000UL);
  }
}
```

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 53 Program 2: BLE Read+Write — Relay ===
[OLED] ✅ Display terdeteksi di 0x3C
[DHT] ✅ Baca OK: 29.1°C | 65%
[BLE] ✅ Advertising: 'Bluino-Relay-BLE'
--- (smartphone connect & subscribe) ---
[BLE] ✅ Client terhubung!
[BLE] Notify #1: {"temp":29.1,"humidity":65,"relay":"OFF","uptime":12}
[BLE] Write #1: 'ON'
[RELAY] → ON
[BLE] Notify #2: {"temp":29.2,"humidity":65,"relay":"ON","uptime":17}
[BLE] Write #2: 'TOGGLE'
[RELAY] → OFF
--- (disconnect tiba-tiba) ---
[BLE] ⚠️ Client terputus — restart advertising...
[SAFETY] Relay auto-OFF karena BLE terputus!
[HB] BLE:OFF | Relay:OFF | Ntf:14 | Wr:3 | Heap:188000 B | Up:61 s
```

---

## 53.7 Program 3: BLE Dashboard — Multi-Characteristic Full IoT Node

Program paling lengkap: ESP32 sebagai **full IoT node** dengan 3 Characteristic (sensor, relay, diagnostik), OLED, dan safety pattern lengkap. Ini adalah template siap pakai untuk proyek BLE IoT nyata.

### Tujuan Pembelajaran
- Menggabungkan semua konsep BLE dalam satu sketch produksi
- Mengelola multiple Characteristic dalam satu Service
- Memahami penggunaan PROPERTY_INDICATE vs PROPERTY_NOTIFY
- Menerapkan pola kode yang *maintainable* dan *scalable*

### Hardware
- DHT11 (IO27): sensor suhu & kelembaban
- Relay (IO4): dikontrol via BLE
- OLED: menampilkan IP BLE MAC + suhu + relay + counter

```cpp
/*
 * BAB 53 - Program 3: BLE Dashboard — Full IoT Node (3 Characteristic)
 *
 * Arsitektur Service GATT:
 *   Service UUID: a1b2c3d4-...
 *   ├── Char 1: SENSOR  (READ + NOTIFY) → JSON: suhu, kelembaban, uptime
 *   ├── Char 2: RELAY   (READ + WRITE)  → "ON" / "OFF" / "TOGGLE" / "STATUS"
 *   └── Char 3: DIAG    (READ)          → JSON: heap, uptime, ntf count, write count
 *
 * Fitur:
 *   ► 3 Characteristic dalam 1 Service — arsitektur IoT lengkap
 *   ► SENSOR: NOTIFY tiap 5 detik + READ on demand
 *   ► RELAY: WRITE dengan 4 perintah (ON/OFF/TOGGLE/STATUS)
 *   ► DIAG: READ diagnostik ESP32 kapan saja
 *   ► Safety: relay auto-OFF saat BLE disconnect
 *   ► OLED tampilkan MAC address BLE + status lengkap
 *   ► Heartbeat tiap 60 detik ke Serial Monitor
 *
 * Hardware:
 *   DHT11 → IO27 | Relay → IO4 | OLED → SDA=IO21, SCL=IO22
 */

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── UUID (3 Characteristic dalam 1 Service) ───────────────────────────
#define SERVICE_UUID    "a1b2c3d4-e5f6-7890-abcd-ef1234567890"
#define CHAR_SENSOR_UUID "a1b2c3d4-e5f6-7890-abcd-ef1234567891"  // READ+NOTIFY
#define CHAR_RELAY_UUID  "a1b2c3d4-e5f6-7890-abcd-ef1234567892"  // READ+WRITE
#define CHAR_DIAG_UUID   "a1b2c3d4-e5f6-7890-abcd-ef1234567893"  // READ

// ── Hardware ──────────────────────────────────────────────────────────
#define RELAY_PIN 4
#define DHT_PIN   27
DHT dht(DHT_PIN, DHT11);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool oledReady = false;

// ── BLE Objects ───────────────────────────────────────────────────────
BLEServer*         pServer     = nullptr;
BLECharacteristic* pSensorChar = nullptr;
BLECharacteristic* pRelayChar  = nullptr;
BLECharacteristic* pDiagChar   = nullptr;

// ── State ─────────────────────────────────────────────────────────────
bool     deviceConnected = false;
bool     relayState      = false;
float    lastT           = NAN;
float    lastH           = NAN;
uint32_t notifyCount     = 0;
uint32_t writeCount      = 0;

// ── Kontrol Relay ─────────────────────────────────────────────────────
void setRelay(bool on) {
  relayState = on;
  digitalWrite(RELAY_PIN, on ? HIGH : LOW);
  pRelayChar->setValue(on ? "ON" : "OFF");
  Serial.printf("[RELAY] → %s\n", on ? "ON" : "OFF");
}

// ── Update Characteristic Diagnostik ─────────────────────────────────
void updateDiagChar() {
  char diagBuf[128];
  snprintf(diagBuf, sizeof(diagBuf),
    "{\"heap\":%u,\"uptime\":%lu,\"ntf\":%u,\"wr\":%u,\"relay\":\"%s\"}",
    ESP.getFreeHeap(), millis()/1000UL, notifyCount, writeCount,
    relayState ? "ON" : "OFF");
  pDiagChar->setValue((uint8_t*)diagBuf, strlen(diagBuf));
}

// ── OLED Helper ───────────────────────────────────────────────────────
void updateOLED() {
  if (!oledReady) return;
  oled.clearDisplay(); oled.setTextColor(WHITE); oled.setTextSize(1);
  oled.setCursor(0, 0);  oled.print("BLE Dashboard");
  oled.drawLine(0, 10, 127, 10, WHITE);
  oled.setCursor(0, 14);
  oled.print(deviceConnected ? "BLE: TERHUBUNG" : "BLE: Advertising...");
  oled.setTextSize(2); oled.setCursor(0, 26);
  oled.print("RLY:");
  oled.print(relayState ? "ON " : "OFF");
  oled.setTextSize(1); oled.setCursor(0, 50);
  if (!isnan(lastT)) { oled.printf("%.1fC %.0f%% N:%u", lastT, lastH, notifyCount); }
  oled.display();
}

// ── Callback: WRITE Relay Characteristic ──────────────────────────────
class RelayCharCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* pChar) override {
    std::string val = pChar->getValue();
    if (val.length() == 0) return;
    String cmd = String(val.c_str());
    cmd.trim(); cmd.toUpperCase();
    writeCount++;
    Serial.printf("[BLE] Write #%u: '%s'\n", writeCount, cmd.c_str());

    if      (cmd == "ON")     { setRelay(true); }
    else if (cmd == "OFF")    { setRelay(false); }
    else if (cmd == "TOGGLE") { setRelay(!relayState); }
    else if (cmd == "STATUS") {
      // Kirim balik status via Relay Char nilai terkini
      Serial.printf("[BLE] STATUS: Relay=%s T=%.1f H=%.0f\n",
        relayState?"ON":"OFF", lastT, lastH);
    } else {
      Serial.printf("[BLE] ⚠️ Perintah tidak dikenal: '%s'\n", cmd.c_str());
    }
    updateDiagChar();
    updateOLED();
  }
};

// ── Server Callbacks ──────────────────────────────────────────────────
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pSrv) override {
    deviceConnected = true;
    Serial.println("[BLE] ✅ Client terhubung!");
    updateDiagChar();
    updateOLED();
  }
  void onDisconnect(BLEServer* pSrv) override {
    deviceConnected = false;
    Serial.println("[BLE] ⚠️ Client terputus — restart advertising...");
    if (relayState) {
      setRelay(false);
      Serial.println("[SAFETY] Relay auto-OFF karena BLE terputus!");
    }
    delay(500);
    pSrv->startAdvertising();
    updateOLED();
  }
};

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  dht.begin();
  Wire.begin(21, 22);

  Serial.println("\n=== BAB 53 Program 3: BLE Dashboard — Full IoT Node ===");

  if (oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledReady = true;
    Serial.println("[OLED] ✅ Display terdeteksi di 0x3C");
  } else {
    Serial.println("[OLED] ⚠️ Display tidak terdeteksi.");
  }

  float t = dht.readTemperature(), h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    lastT = t; lastH = h;
    Serial.printf("[DHT] ✅ Baca OK: %.1f°C | %.0f%%\n", lastT, lastH);
  }

  // ── Inisiasi BLE ──────────────────────────────────────────────────
  BLEDevice::init("Bluino-Dashboard");
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService* pService = pServer->createService(
    BLEUUID(SERVICE_UUID), 20 // 20 = max handles (cukup untuk 3 char + descriptor)
  );

  // Char 1: Sensor (READ + NOTIFY)
  pSensorChar = pService->createCharacteristic(
    CHAR_SENSOR_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
  );
  pSensorChar->addDescriptor(new BLE2902());
  pSensorChar->setValue("Menunggu sensor...");

  // Char 2: Relay (READ + WRITE)
  pRelayChar = pService->createCharacteristic(
    CHAR_RELAY_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE
  );
  pRelayChar->setCallbacks(new RelayCharCallbacks());
  pRelayChar->setValue("OFF");

  // Char 3: Diagnostik (READ only)
  pDiagChar = pService->createCharacteristic(
    CHAR_DIAG_UUID,
    BLECharacteristic::PROPERTY_READ
  );
  updateDiagChar();

  pService->start();

  BLEAdvertising* pAdv = BLEDevice::getAdvertising();
  pAdv->addServiceUUID(SERVICE_UUID);
  pAdv->setScanResponse(true);
  pAdv->setMinPreferred(0x06);
  BLEDevice::startAdvertising();

  // Tampilkan MAC Address BLE di Serial
  Serial.printf("[BLE] ✅ Advertising: 'Bluino-Dashboard'\n");
  Serial.printf("[BLE] MAC: %s\n", BLEDevice::getAddress().toString().c_str());
  Serial.println("[BLE] Char 1 (Sensor): READ + NOTIFY tiap 5 detik");
  Serial.println("[BLE] Char 2 (Relay) : READ + WRITE (ON/OFF/TOGGLE/STATUS)");
  Serial.println("[BLE] Char 3 (Diag)  : READ diagnostik ESP32");

  updateOLED();
}

void loop() {
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

  // Notify Sensor tiap 5 detik
  static unsigned long tNotify = 0;
  if (millis() - tNotify >= 5000UL) {
    tNotify = millis();
    if (!isnan(lastT)) {
      char payload[100];
      snprintf(payload, sizeof(payload),
        "{\"temp\":%.1f,\"humidity\":%.0f,\"relay\":\"%s\",\"uptime\":%lu}",
        lastT, lastH, relayState ? "ON" : "OFF", millis() / 1000UL);
      pSensorChar->setValue((uint8_t*)payload, strlen(payload));
      if (deviceConnected) {
        pSensorChar->notify();
        notifyCount++;
        Serial.printf("[BLE] Notify #%u: %s\n", notifyCount, payload);
      }
      updateDiagChar();
      updateOLED();
    }
  }

  // Heartbeat
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] BLE:%s | Relay:%s | Ntf:%u | Wr:%u | Heap:%u B | Up:%lu s\n",
      deviceConnected?"ON":"OFF", relayState?"ON":"OFF",
      notifyCount, writeCount, ESP.getFreeHeap(), millis()/1000UL);
  }
}
```

**Output Serial Monitor yang Diharapkan:**
```
=== BAB 53 Program 3: BLE Dashboard — Full IoT Node ===
[OLED] ✅ Display terdeteksi di 0x3C
[DHT] ✅ Baca OK: 29.1°C | 65%
[BLE] ✅ Advertising: 'Bluino-Dashboard'
[BLE] MAC: 24:6f:28:ab:cd:ef
[BLE] Char 1 (Sensor): READ + NOTIFY tiap 5 detik
[BLE] Char 2 (Relay) : READ + WRITE (ON/OFF/TOGGLE/STATUS)
[BLE] Char 3 (Diag)  : READ diagnostik ESP32
--- (smartphone connect) ---
[BLE] ✅ Client terhubung!
[BLE] Notify #1: {"temp":29.1,"humidity":65,"relay":"OFF","uptime":18}
[BLE] Write #1: 'ON'
[RELAY] → ON
[BLE] Write #2: 'STATUS'
[BLE] STATUS: Relay=ON T=29.1 H=65
[HB] BLE:ON | Relay:ON | Ntf:12 | Wr:2 | Heap:184000 B | Up:61 s
```

---

## 53.8 Troubleshooting BLE

| Gejala | Penyebab | Solusi |
|--------|----------|--------|
| ESP32 tidak muncul di nRF Connect | `BLEDevice::init()` / `startAdvertising()` belum dipanggil | Pastikan setup() lengkap; cek Serial Monitor untuk error |
| Connect berhasil tapi NOTIFY tidak muncul | Lupa `addDescriptor(new BLE2902())` | Tambahkan setelah `createCharacteristic()` untuk setiap Characteristic NOTIFY |
| Setelah disconnect tidak bisa connect lagi | Lupa `pServer->startAdvertising()` di `onDisconnect()` | Tambahkan `delay(500); pSrv->startAdvertising();` di dalam `onDisconnect()` |
| `onWrite()` tidak pernah dipanggil | `pChar->setCallbacks()` belum dipanggil | Pastikan `setCallbacks(new MyCharCb())` dipanggil sebelum `pService->start()` |
| Compile error: `BLE2902 not found` | Board bukan ESP32 Dev Module | Tools → Board → **ESP32 Arduino → ESP32 Dev Module** |
| Nilai Characteristic terpotong | Panjang payload > MTU (default 23 byte) | Gunakan `BLEDevice::setMTU(517)` di `setup()` **setelah** `BLEDevice::init()` dan sebelum advertising |
| ESP32 crash / reboot saat BLE aktif | Heap terlalu kecil — BLE butuh ~80KB | Kurangi stack `StaticJsonDocument`, hindari `String` besar di global scope |
| nRF Connect bisa READ tapi tidak NOTIFY | Client belum klik Subscribe | Di nRF Connect: klik ikon panah bawah pada Characteristic untuk subscribe |

> ⚠️ **POLA WAJIB — Urutan inisiasi BLE yang benar:**
> ```cpp
> BLEDevice::init("NamaDevice");            // 1. Init BLE
> pServer = BLEDevice::createServer();      // 2. Buat server
> pServer->setCallbacks(new MyCb());        // 3. Pasang server callback
> BLEService* svc = pServer->createService(SVC_UUID); // 4. Buat service
> BLECharacteristic* ch = svc->createCharacteristic(CH_UUID, PROPS); // 5.
> ch->addDescriptor(new BLE2902());         // 6. WAJIB jika NOTIFY
> ch->setCallbacks(new MyCh());             // 7. WAJIB jika WRITE
> ch->setValue("nilai_awal");              // 8. Set nilai awal
> svc->start();                             // 9. Start service
> BLEDevice::getAdvertising()->start();     // 10. Start advertising
> ```

---

## 53.9 Perbandingan Protokol Komunikasi Nirkabel BAB 44-53

| Protokol | BAB | Butuh Router | Jarak | Daya | Cocok Untuk |
|----------|-----|-------------|-------|------|------------|
| WiFi HTTP | 48 | ✅ | 30-100m | Tinggi | REST API, web dashboard |
| WiFi HTTPS | 49 | ✅ | 30-100m | Tinggi | API produksi dengan enkripsi |
| WiFi MQTT | 50 | ✅ | 30-100m | Tinggi | Monitoring IoT real-time |
| WiFi CoAP | 51 | ✅ | 30-100m | Sedang | Sensor baterai, NB-IoT |
| BT Classic | 52 | ❌ | 10-30m | Sedang | Stream data, kontrol serial |
| **BLE** | **53** | **❌** | **10-30m** | **Sangat Rendah** | **Wearable, sensor baterai, beacon** |

---

## 53.10 Ringkasan

```
RINGKASAN BAB 53 — BLE (Bluetooth Low Energy) ESP32

PRINSIP DASAR:
  BLE = GATT architecture — bukan stream serial seperti BT Classic!
  Server (ESP32/Peripheral) mengiklankan Service dengan Characteristic.
  Client (smartphone/Central) connect dan baca/tulis Characteristic.

URUTAN INISIASI (WAJIB):
  BLEDevice::init("Nama") → createServer() → setCallbacks() →
  createService() → createCharacteristic() → addDescriptor(BLE2902) →
  setCallbacks(charCb) → setValue() → service.start() → startAdvertising()

NOTIFY (push data ke client):
  pChar->addDescriptor(new BLE2902());      // Tambah saat setup
  pChar->setValue((uint8_t*)buf, len);      // Set data
  pChar->notify();                          // Push ke subscriber

WRITE (terima perintah dari client):
  class MyCb : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* p) override {
      std::string val = p->getValue();      // Baca nilai
      String cmd = String(val.c_str());
      cmd.trim(); cmd.toUpperCase();        // Normalisasi
    }
  };

RESTART SETELAH DISCONNECT (WAJIB):
  void onDisconnect(BLEServer* pSrv) override {
    delay(500);
    pSrv->startAdvertising(); // Tanpa ini: tidak bisa connect lagi!
  }

SAFETY PATTERN:
  void onDisconnect(BLEServer* pSrv) override {
    if (relayState) setRelay(false); // Auto-OFF!
    delay(500);
    pSrv->startAdvertising();
  }

ANTIPATTERN:
  ❌ Lupa addDescriptor(new BLE2902()) → NOTIFY tidak bekerja
  ❌ Lupa startAdvertising() di onDisconnect → tidak bisa connect lagi
  ❌ Lupa setCallbacks() sebelum service.start() → onWrite tidak terpanggil
  ❌ payload > MTU (23 byte) tanpa setMTU() → data terpotong
  ❌ Terlalu banyak global String/JsonDocument → heap habis, ESP32 crash
```

---

## 53.11 Latihan

### Latihan Dasar
1. Upload **Program 1** → buka nRF Connect → SCAN → "Bluino-BLE" → CONNECT → Subscribe → amati data DHT11 tiap 5 detik.
2. Ganti `BLEDevice::init("Bluino-BLE")` dengan namamu sendiri → upload ulang → verifikasi nama baru muncul saat scan.
3. Upload **Program 2** → dari nRF Connect, Write "ON" (format UTF-8) ke Char Relay → verifikasi relay menyala.

### Latihan Menengah
4. Modifikasi **Program 1**: tambahkan perintah via Characteristic baru (WRITE) yang mengubah interval NOTIFY (default 5 detik → dikirim nilai baru dalam detik dari smartphone).
5. Modifikasi **Program 2**: setelah relay ON selama 30 detik, relay otomatis OFF dan kirim NOTIFY ke client bahwa relay telah auto-off.
6. Upload **Program 3** → disconnect HP tiba-tiba saat relay ON → verifikasi relay auto-OFF terpanggil via Serial Monitor.

### Tantangan Lanjutan
7. **BLE + WiFi Coexistence:** Buat sketch yang menjalankan WiFi (koneksi ke MQTT Antares) dan BLE **secara bersamaan** — kirim data ke Antares via MQTT sekaligus notify ke smartphone via BLE setiap 10 detik.
8. **BLE Beacon:** Implementasikan ESP32 sebagai iBeacon — broadcast data suhu/kelembaban dalam payload advertising (tanpa perlu connect) menggunakan `BLEAdvertisementData`.
9. **BLE OTA:** Gunakan library **ElegantOTA** atau **ArduinoOTA** untuk flashing firmware ESP32 via WiFi, sementara BLE tetap aktif memberikan status update.

---

## 53.12 Preview: Apa Selanjutnya? (→ BAB 54)

```
BAB 53 (BLE) — GATT, hemat daya, cocok untuk wearable & sensor baterai:
  ESP32 ── BLE GATT ──► nRF Connect / Custom App
  ✅ Hemat daya | ✅ Terstruktur | ❌ Butuh app khusus untuk parsing GATT

BAB 54 (ESP-NOW) — protokol mesh proprietary Espressif:
  ESP32 ──► ESP32 ──► ESP32 (tanpa router, tanpa internet!)
  ✅ Ultra-low latency (<1ms) | ✅ Multi-hop up to 20 node
  ✅ Tidak butuh WiFi/BT pairing apapun
  ✅ Cocok untuk: sensor mesh, remote kontrol robot, jaringan sensor lapangan

Kapan pilih BLE vs ESP-NOW?
  BLE     : Komunikasi ke smartphone/PC (Central BLE)
  ESP-NOW : Komunikasi antar ESP32 node (mesh lokal)
```

---

## 53.13 Referensi

| Sumber | Link |
|--------|------|
| ESP32 BLE Arduino Library | https://github.com/espressif/arduino-esp32/tree/master/libraries/BLE |
| nRF Connect for Mobile | https://www.nordicsemi.com/Products/Development-tools/nRF-Connect-for-mobile |
| UUID Generator | https://www.uuidgenerator.net/ |
| Bluetooth GATT Specification | https://www.bluetooth.com/specifications/gatt/ |
| Standard GATT Service UUIDs | https://www.bluetooth.com/specifications/assigned-numbers/ |
| ESP32 BLE Architecture (Espressif) | https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/ble/ |

---

## ☕ Dukung Proyek Ini

[![Trakteer](https://img.shields.io/badge/Trakteer-Dukung_Penulis-red?style=for-the-badge&logo=trakteer&logoColor=white)](https://trakteer.id/limitless7/tip)

**💬 Konsultasi & Kolaborasi:**
- ✉️ **Email:** antonprafanto@unmul.ac.id
- 📞 **WhatsApp:** 0811-553-393 / 0852-5069-0673

---

*Sebelumnya → [BAB 52: Bluetooth Classic](../BAB-52/README.md)*

*Selanjutnya → [BAB 54: ESP-NOW](../BAB-54/README.md)*
