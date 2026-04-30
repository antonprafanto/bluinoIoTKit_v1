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
