/*
 * BAB 50 - Program 3: MQTT Full IoT — Publish + Subscribe + LWT
 * ─────────────────────────────────────────────────────────────────
 * Fitur:
 *   ▶ MQTT Publish: kirim data DHT11 ke Antares tiap 15 detik
 *   ▶ MQTT Subscribe: terima perintah relay dari dashboard Antares
 *   ▶ Last Will & Testament (LWT): jika ESP32 putus mendadak, broker
 *     otomatis publish "OFFLINE" ke topik status device
 *   ▶ Status ONLINE/OFFLINE dikirim ke Antares topik status saat connect/disconnect
 *   ▶ Relay IO4: dikontrol via pesan MQTT subscribe
 *   ▶ DHT11 IO27: baca non-blocking tiap 2 detik
 *   ▶ OLED: tampilkan status broker, suhu, dan status relay
 *   ▶ Non-blocking reconnect (millis-based, coba ulang tiap 5 detik)
 *   ▶ Heartbeat tiap 60 detik
 *
 * Tentang Last Will & Testament (LWT):
 *   LWT adalah pesan yang dititipkan ESP32 ke broker PADA SAAT PERTAMA
 *   KALI TERHUBUNG. Broker menyimpan pesan ini dan akan otomatis
 *   mempublishnya ke topik yang ditentukan jika ESP32 putus mendadak
 *   (timeout keepalive, mati listrik, dsb.) tanpa disconnect yang proper.
 *
 *   Dalam program ini:
 *   - Saat connect: ESP32 publish "ONLINE" ke topik status
 *   - Saat disconnect normal: ESP32 publish "OFFLINE" sendiri
 *   - Saat disconnect mendadak: BROKER yang publish "OFFLINE" via LWT
 *
 * Format Topik Antares (oneM2M):
 *   Publish data: /oneM2M/req/{KEY}/antares-cse/{APP}/{DEVICE}/json
 *   Subscribe   : /oneM2M/resp/antares-cse/{KEY}/json
 *   Status LWT  : /oneM2M/req/{KEY}/antares-cse/{APP}/status/json
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
const char* ACCESS_KEY     = "ACCESSKEY-ANDA-DISINI"; // ← Ganti!
const char* APP_NAME       = "bluino-kit";             // ← Sesuaikan!
const char* DEVICE_NAME    = "sensor-ruangan";         // ← Sesuaikan!
const char* STATUS_DEVICE  = "status";  // Device khusus untuk LWT di Antares

const char* MQTT_HOST      = "platform.antares.id";
const int   MQTT_PORT      = 1338;

const unsigned long PUBLISH_INTERVAL = 15000UL;

// ── Pin ──────────────────────────────────────────────────────────
#define DHT_PIN   27
#define DHT_TYPE  DHT11
#define RELAY_PIN  4   // IO4 — hardwired di Bluino Kit

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
bool     relayState  = false;
uint32_t pubCount    = 0;
uint32_t msgCount    = 0;

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
// Helper: kontrol relay
// ─────────────────────────────────────────────────────────────────
void setRelay(bool on) {
  relayState = on;
  digitalWrite(RELAY_PIN, on ? HIGH : LOW);
  Serial.printf("[RELAY] ✅ Relay → %s\n", on ? "ON" : "OFF");
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
  oled.println("BAB 50 - Program 3");
  oled.println();
  oled.println("MQTT Full IoT");
  oled.println("Pub + Sub + LWT");
  oled.println("Menghubungkan...");
  oled.display();
}

void oledShowDashboard() {
  if (!oledReady) return;
  oled.clearDisplay();
  oled.setTextSize(1); oled.setTextColor(SSD1306_WHITE);

  // Baris 1: Status MQTT
  oled.setCursor(0, 0);
  oled.print("MQTT:");
  oled.print(mqtt.connected() ? "OK " : "-- ");
  // Baris 1 lanjut: Relay state
  oled.print("RLY:");
  oled.println(relayState ? "ON " : "OFF");
  oled.drawLine(0, 10, OLED_W - 1, 10, SSD1306_WHITE);

  // Baris 2: Suhu besar
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

  // Baris 3-4: Statistik
  oled.setTextSize(1);
  oled.setCursor(0, 38);
  oled.printf("Pub:#%lu  Msg:#%lu", pubCount, msgCount);
  oled.setCursor(0, 50);
  oled.printf("Heap: %u B", ESP.getFreeHeap());
  oled.display();
}

// ─────────────────────────────────────────────────────────────────
// Callback MQTT — dipanggil otomatis setiap ada pesan masuk
// ─────────────────────────────────────────────────────────────────
void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  msgCount++;
  Serial.printf("[MQTT] 📨 Pesan masuk di topik: %s\n", topic);

  char msg[512];
  size_t copyLen = (length < sizeof(msg) - 1) ? length : sizeof(msg) - 1;
  memcpy(msg, payload, copyLen);
  msg[copyLen] = '\0';

  // Parse oneM2M response: m2m:rsp → pc → m2m:cin → con
  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, msg)) {
    Serial.println("[MQTT] ⚠️ JSON parse error.");
    return;
  }

  const char* con = doc["m2m:rsp"]["pc"]["m2m:cin"]["con"] | nullptr;
  if (!con) return;

  // Parse inner JSON di dalam "con"
  StaticJsonDocument<128> innerDoc;
  if (deserializeJson(innerDoc, con)) return;

  const char* relayCmd = innerDoc["relay"] | nullptr;
  if (!relayCmd) {
    Serial.println("[MQTT] ⚠️ Field 'relay' tidak ada.");
    return;
  }

  if (strcmp(relayCmd, "ON") == 0)       setRelay(true);
  else if (strcmp(relayCmd, "OFF") == 0) setRelay(false);
  else Serial.printf("[MQTT] ⚠️ Perintah tidak dikenal: '%s'\n", relayCmd);

  oledShowDashboard();
}

// ─────────────────────────────────────────────────────────────────
// Publish pesan status ONLINE/OFFLINE ke Antares
// ─────────────────────────────────────────────────────────────────
void publishStatus(const char* status) {
  char topic[192];
  snprintf(topic, sizeof(topic),
           "/oneM2M/req/%s/antares-cse/%s/%s/json",
           ACCESS_KEY, APP_NAME, STATUS_DEVICE);

  // Bungkus dalam oneM2M m2m:cin
  char payload[128];
  snprintf(payload, sizeof(payload),
           "{\"m2m:cin\":{\"con\":\"{\\\"status\\\":\\\"%s\\\"}\"}}",
           status);

  mqtt.publish(topic, payload);
  Serial.printf("[MQTT] Publish %s → topik status\n", status);
}

// ─────────────────────────────────────────────────────────────────
// Publish data sensor ke Antares
// ─────────────────────────────────────────────────────────────────
void publishSensor() {
  if (!mqtt.connected()) return;

  char topic[192];
  snprintf(topic, sizeof(topic),
           "/oneM2M/req/%s/antares-cse/%s/%s/json",
           ACCESS_KEY, APP_NAME, DEVICE_NAME);

  // StaticJsonDocument<160>: headroom aman untuk 4 field (temp, humidity, relay, uptime)
  StaticJsonDocument<160> innerDoc;
  innerDoc["temp"]     = sensorOk ? lastTemp  : (float)NAN;
  innerDoc["humidity"] = sensorOk ? lastHumid : (float)NAN;
  innerDoc["relay"]    = relayState ? "ON" : "OFF";
  innerDoc["uptime"]   = millis() / 1000UL;

  char innerJson[128];
  serializeJson(innerDoc, innerJson, sizeof(innerJson));

  StaticJsonDocument<256> outerDoc;
  outerDoc["m2m:cin"]["con"] = innerJson;

  char payload[256];
  size_t payloadLen = serializeJson(outerDoc, payload, sizeof(payload));

  Serial.printf("[MQTT] Publish #%lu → data sensor\n", pubCount + 1);

  if (mqtt.publish(topic, (uint8_t*)payload, payloadLen, false)) {
    pubCount++;
    Serial.printf("[MQTT] ✅ Publish berhasil! Heap: %u B\n", ESP.getFreeHeap());
  } else {
    Serial.println("[MQTT] ❌ Publish gagal!");
  }

  oledShowDashboard();
}

// ─────────────────────────────────────────────────────────────────
// Non-blocking MQTT Reconnect + LWT + Resubscribe
// ─────────────────────────────────────────────────────────────────
void mqttReconnect() {
  static unsigned long tLastAttempt = 0;
  if (millis() - tLastAttempt < 5000UL) return;
  tLastAttempt = millis();

  String clientId = buildClientId();
  Serial.printf("[MQTT] Menghubungkan ke broker Antares... (ID: %s)\n", clientId.c_str());

  // ── Bangun LWT topic dan payload ─────────────────────────────
  // LWT dititipkan ke broker saat connect — akan dipublish jika ESP32
  // putus mendadak (tanpa disconnect proper)
  char lwtTopic[192];
  snprintf(lwtTopic, sizeof(lwtTopic),
           "/oneM2M/req/%s/antares-cse/%s/%s/json",
           ACCESS_KEY, APP_NAME, STATUS_DEVICE);

  // Payload LWT: "OFFLINE" dalam format oneM2M
  const char* lwtPayload =
    "{\"m2m:cin\":{\"con\":\"{\\\"status\\\":\\\"OFFLINE\\\"}\"}}";

  // connect(clientId, user, pass, lwtTopic, lwtQoS, lwtRetain, lwtMsg)
  if (mqtt.connect(clientId.c_str(),
                   ACCESS_KEY, ACCESS_KEY,
                   lwtTopic, 0, false, lwtPayload)) {
    Serial.printf("[MQTT] ✅ Terhubung! LWT aktif di topik status\n");

    // Publish ONLINE saat berhasil terhubung
    publishStatus("ONLINE");

    // Subscribe topik response oneM2M
    char subTopic[128];
    snprintf(subTopic, sizeof(subTopic),
             "/oneM2M/resp/antares-cse/%s/json", ACCESS_KEY);
    mqtt.subscribe(subTopic);
    Serial.printf("[MQTT] ✅ Subscribe: %s\n", subTopic);

    oledShowDashboard();
  } else {
    Serial.printf("[MQTT] ❌ Gagal (state: %d) — coba lagi dalam 5 detik\n", mqtt.state());
  }
}

// ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  dht.begin();
  Wire.begin(21, 22);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  delay(500);

  Serial.println("\n=== BAB 50 Program 3: MQTT Full IoT — Pub + Sub + LWT ===");
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
  mqtt.setCallback(onMqttMessage);
  mqtt.setBufferSize(512);
  mqtt.setKeepAlive(30); // Keepalive 30 detik — broker mendeteksi disconnect dalam 30 s

  mqttReconnect();

  // Baca sensor pertama saat boot
  float t = dht.readTemperature(), h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    lastTemp = t; lastHumid = h; sensorOk = true;
    Serial.printf("[DHT] ✅ Baca OK: %.1f°C | %.0f%%\n", t, h);
  }

  Serial.printf("[INFO] Publish tiap %lu detik | LWT aktif\n", PUBLISH_INTERVAL / 1000UL);
}

// ─────────────────────────────────────────────────────────────────
void loop() {
  // ── MQTT Loop & Reconnect ─────────────────────────────────────
  if (!mqtt.connected()) {
    mqttReconnect();
  } else {
    mqtt.loop();
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

  // ── Publish sensor Non-Blocking ──────────────────────────────
  static unsigned long tPub = 0;
  if (millis() - tPub >= PUBLISH_INTERVAL) {
    tPub = millis();
    if (mqtt.connected()) {
      Serial.printf("[DHT] %.1f°C | %.0f%%\n", lastTemp, lastHumid);
      publishSensor();
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
    Serial.printf("[HB] %.1fC %.0f%% | Relay: %s | Pub: %lu | Heap: %u B\n",
                  lastTemp, lastHumid, relayState ? "ON" : "OFF",
                  pubCount, ESP.getFreeHeap());
  }

  // ── Update OLED tiap 5 detik ──────────────────────────────────
  static unsigned long tOled = 0;
  if (millis() - tOled >= 5000UL) {
    tOled = millis();
    oledShowDashboard();
  }
}
