/*
 * BAB 50 - Program 2: MQTT Subscribe — Kontrol Relay via Antares.id
 *
 * Fitur:
 *   ► Subscribe ke topik response Antares (oneM2M)
 *   ► Parse payload nested JSON (m2m:rsp → m2m:cin → con)
 *   ► Kendalikan Relay IO4 berdasarkan perintah {"relay":"ON"/"OFF"}
 *   ► Non-blocking reconnect WiFi & MQTT
 *   ► Re-subscribe otomatis setelah MQTT reconnect
 *
 * Hardware:
 *   Relay → IO4 (hardwired Bluino Kit)
 *   OLED  → SDA=IO21, SCL=IO22
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
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

// Topik subscribe: /oneM2M/resp/antares-cse/{ACCESS_KEY}/json
char TOPIC_SUB[160];

// ── Hardware ────────────────────────────────────────────────────────
#define RELAY_PIN 4
Adafruit_SSD1306 oled(128, 64, &Wire, -1);

// ── State ────────────────────────────────────────────────────────────
bool     relayState = false;
uint32_t msgCount   = 0;
char     clientId[24];

WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);

// ── OLED update ──────────────────────────────────────────────────────
void updateOLED() {
  oled.clearDisplay();
  oled.setTextSize(1); oled.setTextColor(WHITE);
  oled.setCursor(0, 0);  oled.print("MQTT Subscribe");
  oled.setCursor(0, 12); oled.print("Relay: ");
  oled.setTextSize(2);
  oled.setCursor(40, 10); oled.print(relayState ? "ON " : "OFF");
  oled.setTextSize(1);
  oled.setCursor(0, 36); oled.print("Msg masuk: #"); oled.print(msgCount);
  oled.setCursor(0, 48); oled.print("Heap: "); oled.print(ESP.getFreeHeap());
  oled.display();
}

// ── Callback: dipanggil tiap ada pesan masuk ─────────────────────────
void onMqttMessage(char* topic, byte* payload, unsigned int length) {
  msgCount++;
  Serial.printf("[MQTT] Pesan masuk di: %s\n", topic);

  // Salin ke buffer null-terminated — payload TIDAK null-terminated!
  char buf[512];
  if (length >= sizeof(buf)) {
    Serial.println("[MQTT] ⚠️ Payload terlalu besar, diabaikan.");
    return;
  }
  memcpy(buf, payload, length);
  buf[length] = '\0';
  Serial.printf("[MQTT] Payload: %s\n", buf);

  // Parse JSON response oneM2M dari Antares
  // Struktur: {"m2m:rsp":{"pc":{"m2m:cin":{"con":"{\"relay\":\"ON\"}"}}}}
  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, buf)) {
    Serial.println("[MQTT] ❌ JSON parse gagal.");
    return;
  }

  // Ambil 'con' dari nested path
  const char* conStr = doc["m2m:rsp"]["pc"]["m2m:cin"]["con"];
  if (!conStr) {
    Serial.println("[MQTT] ⚠️ Field 'con' tidak ditemukan.");
    return;
  }

  // Parse JSON di dalam 'con' (double-encoded JSON)
  StaticJsonDocument<128> inner;
  if (deserializeJson(inner, conStr)) {
    Serial.println("[MQTT] ❌ Parse 'con' gagal.");
    return;
  }

  const char* relayCmd = inner["relay"];
  if (!relayCmd) {
    Serial.println("[MQTT] ⚠️ Field 'relay' tidak ada.");
    return;
  }

  if (strcmp(relayCmd, "ON") == 0) {
    relayState = true;
    digitalWrite(RELAY_PIN, HIGH);
    Serial.println("[RELAY] ✅ Relay → ON");
  } else if (strcmp(relayCmd, "OFF") == 0) {
    relayState = false;
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("[RELAY] ✅ Relay → OFF");
  } else {
    Serial.printf("[RELAY] ⚠️ Perintah tidak dikenal: '%s'\n", relayCmd);
  }

  updateOLED();
}

// ── Non-blocking MQTT reconnect (+ re-subscribe otomatis) ────────────
void mqttConnect() {
  Serial.printf("[MQTT] Menghubungkan... (ID: %s)\n", clientId);
  bool ok = mqtt.connect(clientId, ANTARES_ACCESS_KEY, ANTARES_ACCESS_KEY);
  if (ok) {
    Serial.println("[MQTT] ✅ Terhubung!");
    bool subOk = mqtt.subscribe(TOPIC_SUB);
    Serial.printf("[MQTT] %s Subscribe: %s\n", subOk ? "✅" : "❌", TOPIC_SUB);
  } else {
    Serial.printf("[MQTT] ❌ Gagal, rc=%d\n", mqtt.state());
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  Wire.begin(21, 22);
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oled.clearDisplay(); oled.setTextColor(WHITE);

  uint8_t mac[6]; WiFi.macAddress(mac);
  snprintf(clientId, sizeof(clientId), "BluinoKit-%02X%02X%02X", mac[3], mac[4], mac[5]);

  snprintf(TOPIC_SUB, sizeof(TOPIC_SUB),
    "/oneM2M/resp/antares-cse/%s/json", ANTARES_ACCESS_KEY);

  Serial.println("\n=== BAB 50 Program 2: MQTT Subscribe — Antares.id ===");
  Serial.printf("[INFO] Broker: %s:%d\n", ANTARES_HOST, ANTARES_PORT);
  Serial.printf("[INFO] Subscribe: %s\n", TOPIC_SUB);

  oled.setCursor(0, 0);  oled.print("BAB 50 MQTT Sub");
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
  mqtt.setCallback(onMqttMessage);  // ← Daftarkan callback SEBELUM connect

  mqttConnect();
}

void loop() {
  // Cek WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] ⚠️ Terputus! Reconnect...");
    WiFi.reconnect(); delay(5000); return;
  }

  // Reconnect MQTT non-blocking
  static unsigned long tRecon = 0;
  if (!mqtt.connected() && millis() - tRecon >= 5000UL) {
    tRecon = millis();
    mqttConnect(); // ← Re-subscribe otomatis di dalam fungsi ini
  }

  // WAJIB: proses incoming message & keep-alive
  mqtt.loop();

  // Heartbeat
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] Relay: %s | Msg: %u | Heap: %u B | Uptime: %lu s\n",
      relayState ? "ON" : "OFF", msgCount, ESP.getFreeHeap(), millis() / 1000UL);
  }
}
