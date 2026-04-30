/*
 * BAB 50 - Program 1: MQTT Publish — Kirim Data DHT11 ke Antares.id
 *
 * Fitur:
 *   ▶ Connect ke MQTT Broker Antares.id (port 1338)
 *   ▶ Publish data DHT11 (suhu & kelembaban) setiap 15 detik
 *   ▶ Payload format oneM2M (m2m:cin) — standar Antares
 *   ▶ Non-blocking reconnect WiFi & MQTT via millis()
 *   ▶ setBufferSize(512) — WAJIB agar payload oneM2M tidak terpotong
 *   ▶ Tampil status di OLED & Serial Monitor
 *   ▶ Heartbeat tiap 60 detik
 *
 * Hardware:
 *   DHT11 → IO27 (hardwired Bluino Kit)
 *   OLED  → SDA=IO21, SCL=IO22 (hardwired)
 *
 * Library:
 *   PubSubClient by Nick O'Leary (Library Manager)
 *   ArduinoJson by bblanchon >= 6.x
 *
 * Cara Uji:
 *   1. Install PubSubClient via Library Manager
 *   2. Isi WIFI_SSID, WIFI_PASS, ANTARES_ACCESS_KEY, ANTARES_APP, ANTARES_DEVICE
 *   3. Upload → Serial Monitor 115200 baud
 *   4. Buka Dashboard Antares → lihat data sensor masuk tiap 15 detik
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Konfigurasi WiFi ──────────────────────────────────────────────────
const char* WIFI_SSID = "NamaWiFiKamu";   // ← Ganti!
const char* WIFI_PASS = "PasswordWiFi";   // ← Ganti!

// ── Konfigurasi Antares MQTT ──────────────────────────────────────────
const char* ANTARES_HOST       = "platform.antares.id";
const int   ANTARES_PORT       = 1338;
const char* ANTARES_ACCESS_KEY = "1234567890abcdef:1234567890abcdef"; // ← Ganti!
const char* ANTARES_APP        = "bluino-kit";       // ← Ganti!
const char* ANTARES_DEVICE     = "sensor-ruangan";   // ← Ganti!

const unsigned long PUB_INTERVAL = 15000UL;  // 15 detik

// ── Topik (dibangun saat setup) ───────────────────────────────────────
char TOPIC_PUB[160];

// ── Hardware ──────────────────────────────────────────────────────────
#define DHT_PIN 27
DHT dht(DHT_PIN, DHT11);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);

// ── State ─────────────────────────────────────────────────────────────
float    lastT    = NAN;
float    lastH    = NAN;
uint32_t pubCount = 0;
char     clientId[24];

// ── Objek MQTT ────────────────────────────────────────────────────────
WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);

// ── Fungsi: Connect ke broker MQTT ───────────────────────────────────
bool mqttConnect() {
  Serial.printf("[MQTT] Menghubungkan ke broker... (ID: %s)\n", clientId);
  // connect(clientId, username, password)
  // Antares: username = password = ACCESS_KEY
  bool ok = mqtt.connect(clientId, ANTARES_ACCESS_KEY, ANTARES_ACCESS_KEY);
  if (ok) {
    Serial.printf("[MQTT] ✅ Terhubung! Client ID: %s\n", clientId);
  } else {
    Serial.printf("[MQTT] ❌ Gagal, rc=%d — lihat tabel state code di 50.3\n", mqtt.state());
  }
  return ok;
}

// ── Fungsi: Publish data sensor ke Antares ───────────────────────────
void publishSensor() {
  if (isnan(lastT)) {
    Serial.println("[PUB] ⚠️ Skip — belum ada data sensor valid.");
    return;
  }

  // Bangun 'con': JSON data sensor yang sebenarnya
  char conStr[64];
  snprintf(conStr, sizeof(conStr), "{\"temp\":%.1f,\"humidity\":%.0f}", lastT, lastH);

  // Bangun body oneM2M di stack (zero heap fragmentation)
  StaticJsonDocument<256> doc;
  JsonObject cin = doc.createNestedObject("m2m:cin");
  cin["con"] = conStr;

  char body[256];
  size_t bodyLen = serializeJson(doc, body, sizeof(body));

  Serial.printf("[MQTT] Publish #%u → %s\n", pubCount + 1, TOPIC_PUB);
  Serial.printf("[MQTT] Body: %s\n", body);

  // publish(topic, payload, length, retained)
  bool ok = mqtt.publish(TOPIC_PUB, (uint8_t*)body, bodyLen, false);
  if (ok) {
    pubCount++;
    Serial.printf("[MQTT] ✅ Publish berhasil! Heap: %u B\n", ESP.getFreeHeap());

    oled.clearDisplay();
    oled.setTextSize(1); oled.setTextColor(WHITE);
    oled.setCursor(0, 0);  oled.print("MQTT Pub OK!");
    oled.setCursor(0, 12); oled.print("T:"); oled.print(lastT, 1);
    oled.print("C H:"); oled.print(lastH, 0); oled.print("%");
    oled.setCursor(0, 24); oled.print("Pub: #"); oled.print(pubCount);
    oled.setCursor(0, 36); oled.print("Heap: "); oled.print(ESP.getFreeHeap()); oled.print("B");
    oled.display();
  } else {
    Serial.println("[MQTT] ❌ Publish gagal! Cek koneksi broker atau bufferSize.");
  }
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  Wire.begin(21, 22);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.clearDisplay(); oled.setTextColor(WHITE);

  // Client ID unik dari 3 byte terakhir MAC address
  uint8_t mac[6];
  WiFi.macAddress(mac);
  snprintf(clientId, sizeof(clientId), "BluinoKit-%02X%02X%02X", mac[3], mac[4], mac[5]);

  // Bangun topik publish
  snprintf(TOPIC_PUB, sizeof(TOPIC_PUB),
    "/oneM2M/req/%s/antares-cse/%s/%s/json",
    ANTARES_ACCESS_KEY, ANTARES_APP, ANTARES_DEVICE);

  Serial.println("\n=== BAB 50 Program 1: MQTT Publish — Antares.id ===");
  Serial.printf("[INFO] Broker: %s:%d\n", ANTARES_HOST, ANTARES_PORT);
  Serial.printf("[INFO] App: %s | Device: %s\n", ANTARES_APP, ANTARES_DEVICE);
  Serial.printf("[INFO] Client ID: %s\n", clientId);
  Serial.printf("[INFO] Publish topic: %s\n", TOPIC_PUB);

  // OLED splash
  oled.setCursor(0, 0);  oled.print("BAB 50 MQTT Pub");
  oled.setCursor(0, 12); oled.print("WiFi..."); oled.display();

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[WiFi] Menghubungkan ke '%s'...\n", WIFI_SSID);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - t0 >= 15000UL) { delay(2000); ESP.restart(); }
    Serial.print("."); delay(500);
  }
  Serial.printf("\n[WiFi] ✅ Terhubung! IP: %s | SSID: %s\n",
    WiFi.localIP().toString().c_str(), WiFi.SSID().c_str());

  // MQTT setup — WAJIB setBufferSize sebelum connect!
  mqtt.setServer(ANTARES_HOST, ANTARES_PORT);
  mqtt.setBufferSize(512);  // ← Default 256 byte terlalu kecil untuk payload oneM2M

  // Baca sensor pertama
  float t = dht.readTemperature();
  float h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    lastT = t; lastH = h;
    Serial.printf("[DHT] ✅ Baca OK: %.1f°C | %.0f%%\n", lastT, lastH);
  } else {
    Serial.println("[DHT] ⚠️ Gagal baca awal. Akan coba di loop().");
  }

  // Connect MQTT pertama kali
  mqttConnect();
  Serial.printf("[INFO] Publish tiap %lu detik ke Antares\n", PUB_INTERVAL / 1000UL);
}

void loop() {
  // ── Cek WiFi ────────────────────────────────────────────────────────
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] ⚠️ Terputus! Reconnect...");
    WiFi.reconnect();
    delay(5000); return;
  }

  // ── Reconnect MQTT (non-blocking, max 1 percobaan tiap 5 detik) ─────
  static unsigned long tRecon = 0;
  if (!mqtt.connected() && millis() - tRecon >= 5000UL) {
    tRecon = millis();
    mqttConnect();
  }

  // ── WAJIB: proses keep-alive & incoming data ─────────────────────────
  mqtt.loop();

  // ── Baca sensor + publish tiap PUB_INTERVAL ──────────────────────────
  static unsigned long tPub = 0;
  static bool firstRun = true;
  if (firstRun || millis() - tPub >= PUB_INTERVAL) {
    firstRun = false;
    tPub = millis();

    float t = dht.readTemperature();
    float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      lastT = t; lastH = h;
      Serial.printf("[DHT] ✅ Baca OK: %.1f°C | %.0f%%\n", lastT, lastH);
    } else {
      Serial.println("[DHT] ⚠️ Gagal baca! Cek IO27.");
    }

    if (mqtt.connected()) {
      publishSensor();
    } else {
      Serial.println("[PUB] ⚠️ MQTT belum tersambung, skip publish.");
    }
  }

  // ── Heartbeat tiap 60 detik ───────────────────────────────────────────
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] %.1fC %.0f%% | Pub: %u | Heap: %u B | Uptime: %lu s\n",
      isnan(lastT) ? 0.0f : lastT,
      isnan(lastH) ? 0.0f : lastH,
      pubCount, ESP.getFreeHeap(), millis() / 1000UL);
  }
}
