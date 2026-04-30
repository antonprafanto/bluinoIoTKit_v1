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
