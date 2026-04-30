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
