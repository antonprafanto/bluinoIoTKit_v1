/*
 * BAB 50 - Program 1: MQTT Publish — Data DHT11 ke Antares.id
 * ─────────────────────────────────────────────────────────────────
 * Fitur:
 *   ▶ Baca DHT11 (IO27) — suhu & kelembaban, non-blocking tiap 2 detik
 *   ▶ MQTT Publish ke platform.antares.id:1338 tiap PUBLISH_INTERVAL
 *   ▶ Format payload: oneM2M m2m:cin (standar Antares)
 *   ▶ ArduinoJson — stack allocated, zero heap fragmentation
 *   ▶ Non-blocking reconnect (millis-based, coba ulang tiap 5 detik)
 *   ▶ OLED: tampilkan suhu, kelembaban, jumlah publish sukses
 *   ▶ Heartbeat tiap 60 detik
 *
 * Tentang Antares MQTT:
 *   Antares menggunakan protokol MQTT di atas standar oneM2M.
 *   Format topik dan payload mengikuti konvensi /oneM2M/req/... yang
 *   berbeda dari MQTT "biasa", namun tetap menggunakan PubSubClient
 *   sebagai library MQTT di sisi ESP32.
 *
 * Format Publish (oneM2M):
 *   Topic : /oneM2M/req/{ACCESS_KEY}/antares-cse/{APP}/{DEVICE}/json
 *   Payload: {"m2m:cin":{"con":"{\"temp\":29.1,\"humidity\":65}"}}
 *
 * Cara Daftar Antares (jika belum):
 *   1. https://antares.id → Daftar (gratis!)
 *   2. Login → Profil (kanan atas) → Salin Access Key
 *   3. Buat Application (misal: "bluino-kit")
 *   4. Buat Device di dalamnya (misal: "sensor-ruangan")
 *
 * Library yang dibutuhkan:
 *   - PubSubClient by Nick O'Leary   (Library Manager: "PubSubClient")
 *   - ArduinoJson >= 6.x              (Library Manager)
 *   - DHT sensor library by Adafruit  (Library Manager)
 *   - Adafruit Unified Sensor         (Library Manager)
 *   - Adafruit SSD1306 + GFX          (Library Manager)
 *   - WiFi.h                          (bawaan ESP32 Core)
 *
 * ⚠️ ESP32 hanya mendukung WiFi 2.4 GHz!
 * ⚠️ Ganti SEMUA konfigurasi di bawah sebelum upload!
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Konfigurasi WiFi ──────────────────────────────────────────────
const char* WIFI_SSID = "NamaWiFiKamu";   // ← Ganti!
const char* WIFI_PASS = "PasswordWiFi";   // ← Ganti!

// ── Konfigurasi Antares MQTT ──────────────────────────────────────
// Daftar gratis di https://antares.id
const char* ACCESS_KEY  = "ACCESSKEY-ANDA-DISINI"; // ← Ganti!
const char* APP_NAME    = "bluino-kit";             // ← Sesuaikan!
const char* DEVICE_NAME = "sensor-ruangan";         // ← Sesuaikan!

const char* MQTT_HOST   = "platform.antares.id";
const int   MQTT_PORT   = 1338;

// Interval publish data (minimum 15 detik disarankan)
const unsigned long PUBLISH_INTERVAL = 15000UL;

// ── Pin ──────────────────────────────────────────────────────────
#define DHT_PIN  27
#define DHT_TYPE DHT11

// ── OLED ──────────────────────────────────────────────────────────
#define OLED_W    128
#define OLED_H     64
#define OLED_ADDR 0x3C

// ── Objek ─────────────────────────────────────────────────────────
WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);
DHT          dht(DHT_PIN, DHT_TYPE);
Adafruit_SSD1306 oled(OLED_W, OLED_H, &Wire, -1);
bool oledReady = false;

// ── State ─────────────────────────────────────────────────────────
float    lastTemp    = NAN;
float    lastHumid   = NAN;
bool     sensorOk    = false;
uint32_t pubCount    = 0;

// ─────────────────────────────────────────────────────────────────
// Bangun Client ID unik dari MAC address ESP32
// ─────────────────────────────────────────────────────────────────
String buildClientId() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char id[24];
  snprintf(id, sizeof(id), "BluinoKit-%02X%02X%02X", mac[3], mac[4], mac[5]);
  return String(id);
}

// ─────────────────────────────────────────────────────────────────
// OLED Helpers
// ─────────────────────────────────────────────────────────────────
void oledShowBoot() {
  if (!oledReady) return;
  oled.clearDisplay();
  oled.setTextSize(1); oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);
  oled.println("Bluino IoT Kit");
  oled.println("BAB 50 - Program 1");
  oled.println();
  oled.println("MQTT Publish");
  oled.print("App: "); oled.println(APP_NAME);
  oled.println("Menghubungkan...");
  oled.display();
}

void oledShowTelemetry() {
  if (!oledReady) return;
  oled.clearDisplay();
  oled.setTextSize(1); oled.setTextColor(SSD1306_WHITE);

  oled.setCursor(0, 0);
  oled.print("MQTT: ");
  oled.println(mqtt.connected() ? "ONLINE" : "OFFLINE");
  oled.drawLine(0, 10, OLED_W - 1, 10, SSD1306_WHITE);

  if (sensorOk) {
    oled.setTextSize(2);
    oled.setCursor(0, 14);
    oled.printf("%.1fC", lastTemp);
    oled.setTextSize(1);
    oled.setCursor(70, 14);
    oled.printf("H:%.0f%%", lastHumid);
  } else {
    oled.setCursor(0, 14);
    oled.println("Sensor Error!");
  }

  oled.setTextSize(1);
  oled.setCursor(0, 38);
  oled.printf("Publish: #%lu", pubCount);
  oled.setCursor(0, 50);
  oled.printf("Heap: %u B", ESP.getFreeHeap());
  oled.display();
}

// ─────────────────────────────────────────────────────────────────
// Non-blocking MQTT Reconnect
// Pattern: coba ulang tiap 5 detik tanpa memblokir loop()
// ─────────────────────────────────────────────────────────────────
void mqttReconnect() {
  static unsigned long tLastAttempt = 0;
  if (millis() - tLastAttempt < 5000UL) return; // Throttle — jangan spam broker
  tLastAttempt = millis();

  String clientId = buildClientId();
  Serial.printf("[MQTT] Menghubungkan ke broker Antares... (ID: %s)\n", clientId.c_str());

  // connect(clientId, username, password)
  // Antares: username = password = ACCESS_KEY
  if (mqtt.connect(clientId.c_str(), ACCESS_KEY, ACCESS_KEY)) {
    Serial.printf("[MQTT] ✅ Terhubung! Client ID: %s\n", clientId.c_str());
    oledShowTelemetry();
  } else {
    Serial.printf("[MQTT] ❌ Gagal (state: %d) — coba lagi dalam 5 detik\n", mqtt.state());
    // State codes: -4=timeout, -3=conn refused, -2=conn lost, -1=disconnect, 1=bad protocol,
    //              2=id rejected, 3=unavailable, 4=bad credentials, 5=unauthorized
  }
}

// ─────────────────────────────────────────────────────────────────
// Publish data sensor ke Antares via MQTT (format oneM2M)
// ─────────────────────────────────────────────────────────────────
void publishToAntares() {
  if (!mqtt.connected()) return;

  // ── Bangun topik publish ──────────────────────────────────────
  char topic[192];
  snprintf(topic, sizeof(topic),
           "/oneM2M/req/%s/antares-cse/%s/%s/json",
           ACCESS_KEY, APP_NAME, DEVICE_NAME);

  // ── Bangun payload JSON (dua lapis — format oneM2M) ───────────
  // StaticJsonDocument<160>: headroom aman di atas worst-case 4 field sensor (~55 chars)
  StaticJsonDocument<160> innerDoc;
  innerDoc["temp"]     = sensorOk ? lastTemp  : (float)NAN;
  innerDoc["humidity"] = sensorOk ? lastHumid : (float)NAN;
  innerDoc["uptime"]   = millis() / 1000UL;
  innerDoc["pub"]      = pubCount + 1;

  char innerJson[128];
  serializeJson(innerDoc, innerJson, sizeof(innerJson));

  StaticJsonDocument<256> outerDoc;
  outerDoc["m2m:cin"]["con"] = innerJson;

  char payload[256];
  size_t payloadLen = serializeJson(outerDoc, payload, sizeof(payload));

  Serial.printf("[MQTT] Publish #%lu → %s\n", pubCount + 1, topic);

  if (mqtt.publish(topic, (uint8_t*)payload, payloadLen, false)) {
    pubCount++;
    Serial.printf("[MQTT] ✅ Publish berhasil! Heap: %u B\n", ESP.getFreeHeap());
  } else {
    Serial.println("[MQTT] ❌ Publish gagal! Cek koneksi broker.");
  }

  oledShowTelemetry();
}

// ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  dht.begin();
  Wire.begin(21, 22);
  delay(500);

  Serial.println("\n=== BAB 50 Program 1: MQTT Publish — Antares.id ===");
  Serial.printf("[INFO] Broker: %s:%d\n", MQTT_HOST, MQTT_PORT);
  Serial.printf("[INFO] App: %s | Device: %s\n", APP_NAME, DEVICE_NAME);

  // ── OLED ──────────────────────────────────────────────────────
  if (oled.begin(SSD1306_SWITCHCAPVCC, OLED_ADDR)) {
    oledReady = true;
    Serial.println("[OLED] ✅ Display terdeteksi di 0x3C");
    oledShowBoot();
  } else {
    Serial.println("[OLED] ⚠️ Display tidak ditemukan — lanjut tanpa OLED.");
  }

  // ── Koneksi WiFi ──────────────────────────────────────────────
  Serial.printf("[WiFi] Menghubungkan ke '%s'...\n", WIFI_SSID);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname("bluino");
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  uint32_t tStart = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - tStart >= 15000UL) {
      Serial.println("\n[ERR] Timeout WiFi. Restart...");
      delay(2000); ESP.restart();
    }
    Serial.print("."); delay(500);
  }
  Serial.println();
  Serial.printf("[WiFi] ✅ Terhubung! IP: %s | SSID: %s\n",
                WiFi.localIP().toString().c_str(), WiFi.SSID().c_str());

  // ── Setup MQTT ────────────────────────────────────────────────
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setBufferSize(512); // Perbesar buffer untuk payload oneM2M

  // Connect pertama kali
  mqttReconnect();

  // Baca sensor pertama saat boot
  float t = dht.readTemperature(), h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    lastTemp = t; lastHumid = h; sensorOk = true;
    Serial.printf("[DHT] ✅ Baca OK: %.1f°C | %.0f%%\n", t, h);
  }

  Serial.printf("[INFO] Publish tiap %lu detik ke Antares\n", PUBLISH_INTERVAL / 1000UL);
}

// ─────────────────────────────────────────────────────────────────
void loop() {
  // ── MQTT Loop & Reconnect ─────────────────────────────────────
  if (!mqtt.connected()) {
    mqttReconnect();
  } else {
    mqtt.loop(); // Wajib dipanggil rutin — keepalive ping ke broker
  }

  // ── DHT11 Non-Blocking (tiap 2 detik) ────────────────────────
  static unsigned long tDHT = 0;
  if (millis() - tDHT >= 2000UL) {
    tDHT = millis();
    float t = dht.readTemperature(), h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      if (!sensorOk) Serial.println("[DHT] ✅ Sensor pulih!");
      lastTemp = t; lastHumid = h; sensorOk = true;
    } else {
      if (sensorOk) Serial.println("[DHT] ⚠️ Gagal baca! Cek wiring IO27.");
      sensorOk = false;
    }
  }

  // ── Publish ke Antares Non-Blocking ──────────────────────────
  static unsigned long tPub = 0;
  if (millis() - tPub >= PUBLISH_INTERVAL) {
    tPub = millis();
    if (mqtt.connected()) {
      Serial.printf("[DHT] %.1f°C | %.0f%%\n", lastTemp, lastHumid);
      publishToAntares();
    }
  }

  // ── Deteksi Transisi WiFi ─────────────────────────────────────
  static bool prevWiFi = true;
  bool currWiFi = (WiFi.status() == WL_CONNECTED);
  if (!prevWiFi && currWiFi)
    Serial.printf("[WiFi] ✅ Reconnect! IP: %s\n", WiFi.localIP().toString().c_str());
  if (prevWiFi && !currWiFi)
    Serial.println("[WiFi] ⚠️ Koneksi terputus...");
  prevWiFi = currWiFi;

  // ── Heartbeat (tiap 60 detik) ─────────────────────────────────
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] %.1fC %.0f%% | Pub: %lu | Heap: %u B\n",
                  lastTemp, lastHumid, pubCount, ESP.getFreeHeap());
  }
}
