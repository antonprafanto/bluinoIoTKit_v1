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
