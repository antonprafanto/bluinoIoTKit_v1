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
