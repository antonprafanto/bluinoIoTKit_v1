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
