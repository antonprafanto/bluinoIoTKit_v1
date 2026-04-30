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
