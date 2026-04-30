/*
 * BAB 54 - Program 1B: ESP-NOW Unicast — RECEIVER
 *
 * Fungsi: Terima paket sensor dari Sender → tampilkan di OLED & Serial
 *
 * Hardware: OLED → SDA=IO21, SCL=IO22
 * Tidak perlu tahu MAC address Sender (receiver menerima dari siapapun)
 */

#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool oledReady = false;

// ── Struct paket data — HARUS IDENTIK dengan Sender! ──────────────────
typedef struct {
  float    temperature;
  float    humidity;
  uint32_t uptime;
  uint32_t packetNum;
} SensorPacket;

SensorPacket rxPacket;
bool    newDataFlag   = false;
uint32_t recvCount    = 0;
uint8_t  lastSenderMAC[6];

// ── Callback: dipanggil saat data masuk (di FreeRTOS task, bukan loop!) ──
void onRecv(const uint8_t *mac, const uint8_t *data, int len) {
  if (len != sizeof(SensorPacket)) return;  // Tolak paket tidak valid
  memcpy(&rxPacket, data, sizeof(rxPacket));
  memcpy(lastSenderMAC, mac, 6);
  newDataFlag = true;
  recvCount++;
}

void updateOLED() {
  if (!oledReady) return;
  oled.clearDisplay(); oled.setTextColor(WHITE); oled.setTextSize(1);
  oled.setCursor(0, 0);  oled.print("ESP-NOW Receiver");
  oled.drawLine(0, 10, 127, 10, WHITE);
  oled.setCursor(0, 14);
  oled.printf("Pkt #%u | Up:%lu s", rxPacket.packetNum, rxPacket.uptime);
  oled.setTextSize(2); oled.setCursor(0, 26);
  oled.printf("%.1fC", rxPacket.temperature);
  oled.setTextSize(1); oled.setCursor(0, 50);
  oled.printf("H:%.0f%% | Rcv:%u", rxPacket.humidity, recvCount);
  oled.display();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  Serial.println("\n=== BAB 54 Program 1B: ESP-NOW Unicast RECEIVER ===");

  if (oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledReady = true;
    Serial.println("[OLED] ✅ Display terdeteksi di 0x3C");
    oled.clearDisplay(); oled.setTextColor(WHITE); oled.setTextSize(1);
    oled.setCursor(0, 0);  oled.print("ESP-NOW Receiver");
    oled.setCursor(0, 16); oled.print("Menunggu data...");
    oled.display();
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.print("[WiFi] MAC Receiver: ");
  Serial.println(WiFi.macAddress());
  Serial.println("[PETUNJUK] Salin MAC di atas ke RECEIVER_MAC di sketch Sender!");

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESPNOW] ❌ Init gagal! Reboot...");
    ESP.restart();
  }

  esp_now_register_recv_cb(onRecv);
  Serial.println("[ESPNOW] ✅ Menunggu data dari Sender...");
}

void loop() {
  // Proses data baru (flag dari callback) — aman dari race condition
  if (newDataFlag) {
    newDataFlag = false;
    Serial.printf("[ESPNOW] ✅ Terima pkt #%u dari %02X:%02X:%02X:%02X:%02X:%02X\n",
      rxPacket.packetNum,
      lastSenderMAC[0], lastSenderMAC[1], lastSenderMAC[2],
      lastSenderMAC[3], lastSenderMAC[4], lastSenderMAC[5]);
    Serial.printf("         Suhu:%.1f°C | Humi:%.0f%% | Up:%lu s | Total Rcv:%u\n",
      rxPacket.temperature, rxPacket.humidity,
      rxPacket.uptime, recvCount);
    updateOLED();
  }

  // Heartbeat
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] Rcv:%u | Heap:%u B | Up:%lu s\n",
      recvCount, ESP.getFreeHeap(), millis()/1000UL);
  }
}
