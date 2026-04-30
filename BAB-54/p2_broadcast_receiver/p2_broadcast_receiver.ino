/*
 * BAB 54 - Program 2B: ESP-NOW Broadcast — RECEIVER
 *
 * Fungsi: Terima broadcast dari Sender manapun → tampilkan di OLED & Serial
 * Upload sketch ini ke SEMUA ESP32 yang ingin menjadi receiver!
 *
 * Hardware: OLED → SDA=IO21, SCL=IO22
 */

#include <WiFi.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool oledReady = false;

typedef struct {
  float    temperature;
  float    humidity;
  uint32_t uptime;
  uint32_t packetNum;
  char     senderID[12];
} BroadcastPacket;

BroadcastPacket rxPacket;
bool     newDataFlag = false;
uint32_t recvCount   = 0;

void onRecv(const uint8_t *mac, const uint8_t *data, int len) {
  if (len != sizeof(BroadcastPacket)) return;
  memcpy(&rxPacket, data, sizeof(rxPacket));
  newDataFlag = true;
  recvCount++;
}

void updateOLED() {
  if (!oledReady) return;
  oled.clearDisplay(); oled.setTextColor(WHITE); oled.setTextSize(1);
  oled.setCursor(0, 0);  oled.print("ESP-NOW Broadcast");
  oled.drawLine(0, 10, 127, 10, WHITE);
  oled.setCursor(0, 14);
  oled.printf("From: %s #%u", rxPacket.senderID, rxPacket.packetNum);
  oled.setTextSize(2); oled.setCursor(0, 26);
  oled.printf("%.1fC", rxPacket.temperature);
  oled.setTextSize(1); oled.setCursor(0, 50);
  oled.printf("H:%.0f%% Rcv:%u", rxPacket.humidity, recvCount);
  oled.display();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  Serial.println("\n=== BAB 54 Program 2B: ESP-NOW Broadcast RECEIVER ===");

  if (oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledReady = true;
    Serial.println("[OLED] ✅ Display terdeteksi di 0x3C");
    oled.clearDisplay(); oled.setTextColor(WHITE);
    oled.setCursor(0, 0); oled.print("ESP-NOW Broadcast");
    oled.setCursor(0, 16); oled.print("Menunggu data...");
    oled.display();
  }

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.print("[WiFi] MAC Receiver ini: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESPNOW] ❌ Init gagal! Reboot...");
    ESP.restart();
  }

  esp_now_register_recv_cb(onRecv);
  Serial.println("[ESPNOW] ✅ Siap terima broadcast...");
}

void loop() {
  if (newDataFlag) {
    newDataFlag = false;
    Serial.printf("[ESPNOW] ✅ Broadcast pkt #%u dari '%s': %.1f°C | %.0f%% | Up:%lu s\n",
      rxPacket.packetNum, rxPacket.senderID,
      rxPacket.temperature, rxPacket.humidity, rxPacket.uptime);
    updateOLED();
  }

  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] Rcv:%u | Heap:%u B | Up:%lu s\n",
      recvCount, ESP.getFreeHeap(), millis()/1000UL);
  }
}
