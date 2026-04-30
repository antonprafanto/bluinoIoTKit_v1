/*
 * BAB 54 - Program 2A: ESP-NOW Broadcast — SENDER
 *
 * Fungsi: Broadcast data DHT11 ke SEMUA ESP32 dalam jangkauan tiap 3 detik
 * Tidak perlu tahu MAC address Receiver — cukup gunakan broadcast MAC!
 *
 * Hardware: DHT11 → IO27
 */

#include <WiFi.h>
#include <esp_now.h>
#include <DHT.h>

#define DHT_PIN 27
DHT dht(DHT_PIN, DHT11);

// Broadcast MAC — semua ESP32 dalam jangkauan akan menerima
uint8_t BROADCAST_MAC[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

typedef struct {
  float    temperature;
  float    humidity;
  uint32_t uptime;
  uint32_t packetNum;
  char     senderID[12]; // Identitas sender (nama node)
} BroadcastPacket;

BroadcastPacket packet;
uint32_t sendCount = 0;

void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.printf("[ESPNOW] Broadcast #%u: %s\n",
    sendCount, status == ESP_NOW_SEND_SUCCESS ? "✅ OK" : "❌ GAGAL");
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  Serial.println("\n=== BAB 54 Program 2A: ESP-NOW Broadcast SENDER ===");

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.print("[WiFi] MAC Sender: ");
  Serial.println(WiFi.macAddress());

  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESPNOW] ❌ Init gagal! Reboot...");
    ESP.restart();
  }
  esp_now_register_send_cb(onSent);

  // Daftarkan Broadcast MAC sebagai peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, BROADCAST_MAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("[ESPNOW] ❌ Gagal daftarkan broadcast peer!");
  } else {
    Serial.println("[ESPNOW] ✅ Broadcast peer terdaftar (FF:FF:FF:FF:FF:FF)");
  }

  strncpy(packet.senderID, "Sensor-Node1", sizeof(packet.senderID));

  float t = dht.readTemperature(), h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    Serial.printf("[DHT] ✅ Baca OK: %.1f°C | %.0f%%\n", t, h);
  }

  Serial.println("[ESPNOW] Mulai broadcast tiap 3 detik...");
}

void loop() {
  static unsigned long tDHT = 0;
  static float lastT = NAN, lastH = NAN;
  if (millis() - tDHT >= 2000UL) {
    tDHT = millis();
    float t = dht.readTemperature(), h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) { lastT = t; lastH = h; }
  }

  static unsigned long tSend = 0;
  if (millis() - tSend >= 3000UL) {
    tSend = millis();
    if (!isnan(lastT)) {
      sendCount++;
      packet.temperature = lastT;
      packet.humidity    = lastH;
      packet.uptime      = millis() / 1000UL;
      packet.packetNum   = sendCount;

      esp_now_send(BROADCAST_MAC, (uint8_t*)&packet, sizeof(packet));
      Serial.printf("[ESPNOW] → Broadcast #%u: %.1f°C | %.0f%%\n",
        sendCount, lastT, lastH);
    }
  }

  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] Sent:%u | Heap:%u B | Up:%lu s\n",
      sendCount, ESP.getFreeHeap(), millis()/1000UL);
  }
}
