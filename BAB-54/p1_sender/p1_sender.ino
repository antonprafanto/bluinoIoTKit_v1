/*
 * BAB 54 - Program 1A: ESP-NOW Unicast — SENDER
 *
 * Fungsi: Baca DHT11 → kirim struct data ke ESP32 Receiver tiap 5 detik
 *
 * SEBELUM UPLOAD:
 *   Ganti RECEIVER_MAC dengan MAC address ESP32 Receiver kamu!
 *   (Lihat 54.4 untuk cara mendapatkan MAC address)
 *
 * Hardware: DHT11 → IO27
 */

#include <WiFi.h>
#include <esp_now.h>
#include <DHT.h>

#define DHT_PIN 27
DHT dht(DHT_PIN, DHT11);

// ── GANTI dengan MAC address ESP32 Receiver kamu! ─────────────────────
uint8_t RECEIVER_MAC[] = {0x24, 0x6F, 0x28, 0xAB, 0xCD, 0xEF};

// ── Struct paket data — HARUS IDENTIK dengan Receiver! ────────────────
typedef struct {
  float    temperature;
  float    humidity;
  uint32_t uptime;
  uint32_t packetNum;
} SensorPacket;

SensorPacket packet;
uint32_t sendCount  = 0;
bool     lastSendOK = true;

// ── Callback: dipanggil setelah esp_now_send() selesai ────────────────
void onSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  lastSendOK = (status == ESP_NOW_SEND_SUCCESS);
  Serial.printf("[ESPNOW] Kirim #%u: %s\n",
    sendCount, lastSendOK ? "✅ OK" : "❌ GAGAL");
}

void setup() {
  Serial.begin(115200);
  dht.begin();

  Serial.println("\n=== BAB 54 Program 1A: ESP-NOW Unicast SENDER ===");

  // 1. Set WiFi mode
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  Serial.print("[WiFi] MAC Sender: ");
  Serial.println(WiFi.macAddress());

  // 2. Inisiasi ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("[ESPNOW] ❌ Init gagal! Reboot...");
    ESP.restart();
  }
  Serial.println("[ESPNOW] ✅ ESP-NOW siap");

  // 3. Daftarkan callback kirim
  esp_now_register_send_cb(onSent);

  // 4. Tambahkan Receiver sebagai peer
  esp_now_peer_info_t peerInfo = {};
  memcpy(peerInfo.peer_addr, RECEIVER_MAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("[ESPNOW] ❌ Gagal tambah peer! Cek MAC address.");
  } else {
    Serial.println("[ESPNOW] ✅ Peer Receiver terdaftar");
  }

  // Baca sensor awal
  float t = dht.readTemperature(), h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    Serial.printf("[DHT] ✅ Baca OK: %.1f°C | %.0f%%\n", t, h);
  }

  Serial.println("[ESPNOW] Mulai kirim data tiap 5 detik...");
}

void loop() {
  // Baca DHT11 non-blocking tiap 2 detik
  static unsigned long tDHT = 0;
  static float lastT = NAN, lastH = NAN;
  if (millis() - tDHT >= 2000UL) {
    tDHT = millis();
    float t = dht.readTemperature(), h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) { lastT = t; lastH = h; }
  }

  // Kirim ke Receiver tiap 5 detik
  static unsigned long tSend = 0;
  if (millis() - tSend >= 5000UL) {
    tSend = millis();
    if (!isnan(lastT)) {
      sendCount++;
      packet.temperature = lastT;
      packet.humidity    = lastH;
      packet.uptime      = millis() / 1000UL;
      packet.packetNum   = sendCount;

      esp_now_send(RECEIVER_MAC, (uint8_t*)&packet, sizeof(packet));
      Serial.printf("[ESPNOW] → Kirim #%u: %.1f°C | %.0f%% | Up:%lu s\n",
        sendCount, lastT, lastH, packet.uptime);
    }
  }

  // Heartbeat
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] Sent:%u | LastOK:%s | Heap:%u B | Up:%lu s\n",
      sendCount, lastSendOK?"✅":"❌", ESP.getFreeHeap(), millis()/1000UL);
  }
}
