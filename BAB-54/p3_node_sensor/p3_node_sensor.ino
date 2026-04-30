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
