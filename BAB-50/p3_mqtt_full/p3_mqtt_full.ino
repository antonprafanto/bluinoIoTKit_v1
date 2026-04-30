/*
 * BAB 50 - Program 3: MQTT Full IoT — Publish + Subscribe + LWT
 *
 * Arsitektur: Pub + Sub + Status Monitoring via LWT
 *
 * Fitur:
 *   ► PUBLISH: Data DHT11 ke Antares setiap 15 detik
 *   ► SUBSCRIBE: Terima & eksekusi perintah relay via callback
 *   ► LWT: Broker auto-publish "OFFLINE" jika ESP32 mati mendadak
 *   ► ONLINE: Publish status "ONLINE" setelah connect berhasil
 *   ► Non-blocking reconnect WiFi & MQTT
 *   ► Heartbeat tiap 30 detik
 *
 * Hardware:
 *   DHT11 → IO27, Relay → IO4, OLED → SDA=IO21, SCL=IO22
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Konfigurasi ────────────────────────────────────────────────────
const char* WIFI_SSID          = "NamaWiFiKamu";
const char* WIFI_PASS          = "PasswordWiFi";
const char* ANTARES_HOST       = "platform.antares.id";
const int   ANTARES_PORT       = 1338;
const char* ANTARES_ACCESS_KEY = "1234567890abcdef:1234567890abcdef"; // ← Ganti!
const char* ANTARES_APP        = "bluino-kit";       // ← Ganti!
const char* ANTARES_DEVICE     = "sensor-ruangan";   // ← Ganti!

const unsigned long PUB_INTERVAL = 15000UL;

// ── Topik ───────────────────────────────────────────────────────────
char TOPIC_PUB[160];     // publish data sensor
char TOPIC_SUB[160];     // subscribe perintah
char TOPIC_STATUS[160];  // LWT status (ONLINE / OFFLINE)

// ── Hardware ────────────────────────────────────────────────────────
#define DHT_PIN   27
#define RELAY_PIN 4
DHT dht(DHT_PIN, DHT11);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);

// ── State ─────────────────────────────────────────────────────────────
float    lastT      = NAN;
float    lastH      = NAN;
bool     relayState = false;
uint32_t pubCount   = 0;
uint32_t msgCount   = 0;
char     clientId[24];

WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);

// ── OLED update ──────────────────────────────────────────────────────
void updateOLED() {
  oled.clearDisplay();
  oled.setTextColor(WHITE); oled.setTextSize(1);

  // Baris 1: Status broker
  oled.setCursor(0, 0);
  oled.print(mqtt.connected() ? "MQTT:OK " : "MQTT:-- ");
  oled.print(relayState ? "[RLY:ON] " : "[RLY:OFF]");

  // Baris 2-3: Suhu besar
  oled.setTextSize(2); oled.setCursor(0, 12);
  if (!isnan(lastT)) {
    oled.print(lastT, 1); oled.print("C");
  } else { oled.print("---"); }

  // Baris 4: Kelembaban
  oled.setTextSize(1); oled.setCursor(0, 36);
  if (!isnan(lastH)) { oled.print("H: "); oled.print(lastH, 0); oled.print("%"); }

  // Baris 5: Counter
  oled.setCursor(0, 48);
  oled.print("Pub:#"); oled.print(pubCount);
  oled.print(" Msg:#"); oled.print(msgCount);
  oled.display();
}

// ── Callback: pesan masuk dari broker ───────────────────────────────
void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  msgCount++;
  char buf[512];
  if (length >= sizeof(buf)) return;
  memcpy(buf, payload, length); buf[length] = '\0';
  Serial.printf("[MQTT] Pesan masuk: %s\n", buf);

  // Parse oneM2M response
  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, buf)) { Serial.println("[MQTT] ❌ Parse gagal."); return; }

  const char* conStr = doc["m2m:rsp"]["pc"]["m2m:cin"]["con"];
  if (!conStr) { Serial.println("[MQTT] ⚠️ 'con' tidak ditemukan."); return; }

  StaticJsonDocument<128> inner;
  if (deserializeJson(inner, conStr)) { Serial.println("[MQTT] ❌ Parse 'con' gagal."); return; }

  const char* relayCmd = inner["relay"];
  if (!relayCmd) return;

  if (strcmp(relayCmd, "ON") == 0) {
    relayState = true; digitalWrite(RELAY_PIN, HIGH);
    Serial.println("[RELAY] ✅ Relay → ON");
  } else if (strcmp(relayCmd, "OFF") == 0) {
    relayState = false; digitalWrite(RELAY_PIN, LOW);
    Serial.println("[RELAY] ✅ Relay → OFF");
  }
  updateOLED();
}

// ── Connect MQTT dengan LWT ──────────────────────────────────────────
void mqttConnect() {
  Serial.printf("[MQTT] Menghubungkan... (ID: %s)\n", clientId);

  // LWT: jika ESP32 mati mendadak, broker publish "OFFLINE"
  bool ok = mqtt.connect(
    clientId,
    ANTARES_ACCESS_KEY,   // username
    ANTARES_ACCESS_KEY,   // password
    TOPIC_STATUS,         // willTopic
    0,                    // willQos
    true,                 // willRetain — pesan LWT disimpan broker
    "OFFLINE"             // willMessage
  );

  if (ok) {
    Serial.printf("[MQTT] ✅ Terhubung! LWT aktif di: %s\n", TOPIC_STATUS);

    // Publish status ONLINE
    mqtt.publish(TOPIC_STATUS, "ONLINE", true);  // retained
    Serial.println("[MQTT] Publish ONLINE → topik status");

    // Subscribe perintah
    bool subOk = mqtt.subscribe(TOPIC_SUB);
    Serial.printf("[MQTT] %s Subscribe: %s\n", subOk ? "✅" : "❌", TOPIC_SUB);
  } else {
    Serial.printf("[MQTT] ❌ Gagal, rc=%d\n", mqtt.state());
  }
}

// ── Publish sensor ────────────────────────────────────────────────────
void publishSensor() {
  if (isnan(lastT)) { Serial.println("[PUB] ⚠️ Skip — sensor belum valid."); return; }

  char conStr[64];
  snprintf(conStr, sizeof(conStr), "{\"temp\":%.1f,\"humidity\":%.0f}", lastT, lastH);

  StaticJsonDocument<256> doc;
  doc.createNestedObject("m2m:cin")["con"] = conStr;

  char body[256];
  size_t len = serializeJson(doc, body, sizeof(body));

  Serial.printf("[MQTT] Publish #%u → data sensor\n", pubCount + 1);
  if (mqtt.publish(TOPIC_PUB, (uint8_t*)body, len, false)) {
    pubCount++;
    Serial.printf("[MQTT] ✅ Publish berhasil! Heap: %u B\n", ESP.getFreeHeap());
  } else {
    Serial.println("[MQTT] ❌ Publish gagal!");
  }
  updateOLED();
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT); digitalWrite(RELAY_PIN, LOW);
  dht.begin();
  Wire.begin(21, 22);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.clearDisplay(); oled.setTextColor(WHITE);

  // Client ID dari MAC
  uint8_t mac[6]; WiFi.macAddress(mac);
  snprintf(clientId, sizeof(clientId), "BluinoKit-%02X%02X%02X", mac[3], mac[4], mac[5]);

  // Bangun semua topik
  snprintf(TOPIC_PUB, sizeof(TOPIC_PUB),
    "/oneM2M/req/%s/antares-cse/%s/%s/json",
    ANTARES_ACCESS_KEY, ANTARES_APP, ANTARES_DEVICE);
  snprintf(TOPIC_SUB, sizeof(TOPIC_SUB),
    "/oneM2M/resp/antares-cse/%s/json", ANTARES_ACCESS_KEY);
  snprintf(TOPIC_STATUS, sizeof(TOPIC_STATUS),
    "/oneM2M/req/%s/antares-cse/%s/status/json",
    ANTARES_ACCESS_KEY, ANTARES_APP);

  Serial.println("\n=== BAB 50 Program 3: MQTT Full IoT — Pub + Sub + LWT ===");
  Serial.printf("[INFO] Broker: %s:%d\n", ANTARES_HOST, ANTARES_PORT);
  Serial.printf("[INFO] App: %s | Device: %s\n", ANTARES_APP, ANTARES_DEVICE);

  oled.setCursor(0, 0);  oled.print("BAB 50 Full IoT");
  oled.setCursor(0, 12); oled.print("WiFi..."); oled.display();

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.printf("[WiFi] Menghubungkan ke '%s'...\n", WIFI_SSID);
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - t0 >= 15000UL) { delay(2000); ESP.restart(); }
    Serial.print("."); delay(500);
  }
  Serial.printf("\n[WiFi] ✅ Terhubung! IP: %s\n", WiFi.localIP().toString().c_str());

  mqtt.setServer(ANTARES_HOST, ANTARES_PORT);
  mqtt.setBufferSize(512);
  mqtt.setCallback(onMqttMessage);

  // Baca sensor awal
  float t = dht.readTemperature(); float h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) { lastT = t; lastH = h; }

  mqttConnect();
  Serial.printf("[INFO] Publish tiap %lu detik | LWT aktif\n", PUB_INTERVAL / 1000UL);
  updateOLED();
}

void loop() {
  // WiFi check
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] ⚠️ Terputus! Reconnect...");
    WiFi.reconnect(); delay(5000); return;
  }

  // MQTT reconnect non-blocking
  static unsigned long tRecon = 0;
  if (!mqtt.connected() && millis() - tRecon >= 5000UL) {
    tRecon = millis(); mqttConnect();
  }

  // WAJIB: proses semua incoming messages & keep-alive
  mqtt.loop();

  // Publish tiap PUB_INTERVAL
  static unsigned long tPub = 0;
  static bool firstRun = true;
  if (firstRun || millis() - tPub >= PUB_INTERVAL) {
    firstRun = false; tPub = millis();

    float t = dht.readTemperature(); float h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) {
      lastT = t; lastH = h;
      Serial.printf("[DHT] ✅ Baca OK: %.1f°C | %.0f%%\n", lastT, lastH);
    } else {
      Serial.println("[DHT] ⚠️ Gagal baca! Cek IO27.");
    }

    if (mqtt.connected()) publishSensor();
    else Serial.println("[PUB] ⚠️ MQTT terputus, skip publish.");
  }

  // Heartbeat
  static unsigned long tHb = 0;
  if (millis() - tHb >= 30000UL) {
    tHb = millis();
    Serial.printf("[HB] %.1fC %.0f%% | Relay:%s | Pub:%u | Heap:%u B | Up:%lu s\n",
      isnan(lastT) ? 0.0f : lastT, isnan(lastH) ? 0.0f : lastH,
      relayState ? "ON" : "OFF", pubCount, ESP.getFreeHeap(), millis() / 1000UL);
  }
}
