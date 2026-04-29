/*
 * BAB 50 - Program 2: MQTT Subscribe — Kontrol Relay via Antares.id
 * ─────────────────────────────────────────────────────────────────
 * Fitur:
 *   ▶ Subscribe ke topik response Antares via MQTT
 *   ▶ Terima perintah {"relay":"ON"} atau {"relay":"OFF"} dari dashboard
 *   ▶ Kontrol Relay (IO4) berdasarkan perintah yang diterima
 *   ▶ Non-blocking reconnect (millis-based, coba ulang tiap 5 detik)
 *   ▶ OLED: tampilkan status broker dan status relay terakhir
 *   ▶ Heartbeat tiap 60 detik
 *
 * Format Subscribe Antares (oneM2M response):
 *   Topic   : /oneM2M/resp/antares-cse/{ACCESS_KEY}/json
 *   Payload : {"m2m:rsp":{"pc":{"m2m:cin":{"con":"{\"relay\":\"ON\"}"}},...}}
 *             ↑ Data kita ada di dalam path: m2m:rsp → pc → m2m:cin → con
 *
 * Cara Mengirim Perintah dari Antares Dashboard:
 *   1. Login ke https://antares.id → Application → Device Anda
 *   2. Klik "Send Downlink" atau gunakan API:
 *      POST /~/antares-cse/antares-id/{APP}/{DEVICE}
 *      Body: {"m2m:cin":{"con":"{\"relay\":\"ON\"}"}}
 *   3. Relay ESP32 langsung bereaksi!
 *
 * Library yang dibutuhkan:
 *   - PubSubClient by Nick O'Leary   (Library Manager: "PubSubClient")
 *   - ArduinoJson >= 6.x              (Library Manager)
 *   - Adafruit SSD1306 + GFX          (Library Manager)
 *   - WiFi.h                          (bawaan ESP32 Core)
 *
 * ⚠️ ESP32 hanya mendukung WiFi 2.4 GHz!
 * ⚠️ Ganti SEMUA konfigurasi di bawah sebelum upload!
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Konfigurasi WiFi ──────────────────────────────────────────────
const char* WIFI_SSID = "NamaWiFiKamu";   // ← Ganti!
const char* WIFI_PASS = "PasswordWiFi";   // ← Ganti!

// ── Konfigurasi Antares MQTT ──────────────────────────────────────
const char* ACCESS_KEY  = "ACCESSKEY-ANDA-DISINI"; // ← Ganti!
const char* APP_NAME    = "bluino-kit";             // ← Sesuaikan!
const char* DEVICE_NAME = "sensor-ruangan";         // ← Sesuaikan!

const char* MQTT_HOST   = "platform.antares.id";
const int   MQTT_PORT   = 1338;

// ── Pin ──────────────────────────────────────────────────────────
#define RELAY_PIN 4  // IO4 — hardwired di Bluino Kit

// ── OLED ──────────────────────────────────────────────────────────
#define OLED_W    128
#define OLED_H     64
#define OLED_ADDR 0x3C

// ── Objek ─────────────────────────────────────────────────────────
WiFiClient   wifiClient;
PubSubClient mqtt(wifiClient);
Adafruit_SSD1306 oled(OLED_W, OLED_H, &Wire, -1);
bool oledReady = false;

// ── State ─────────────────────────────────────────────────────────
bool relayState  = false;    // false = OFF
uint32_t msgCount = 0;       // Jumlah pesan MQTT diterima

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
// Helper: Set relay dan update state
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
  oled.println("BAB 50 - Program 2");
  oled.println();
  oled.println("MQTT Subscribe");
  oled.println("Kontrol Relay IO4");
  oled.println("Menghubungkan...");
  oled.display();
}

void oledShowStatus() {
  if (!oledReady) return;
  oled.clearDisplay();
  oled.setTextSize(1); oled.setTextColor(SSD1306_WHITE);

  oled.setCursor(0, 0);
  oled.print("MQTT: ");
  oled.println(mqtt.connected() ? "ONLINE" : "OFFLINE");
  oled.drawLine(0, 10, OLED_W - 1, 10, SSD1306_WHITE);

  oled.setTextSize(2);
  oled.setCursor(0, 16);
  oled.println(relayState ? "RELAY:ON " : "RELAY:OFF");

  oled.setTextSize(1);
  oled.setCursor(0, 38);
  oled.printf("Pesan masuk: %lu", msgCount);
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

  // Konversi payload byte[] ke null-terminated string
  // Gunakan buffer stack — tidak ada alokasi heap baru
  char msg[512];
  size_t copyLen = (length < sizeof(msg) - 1) ? length : sizeof(msg) - 1;
  memcpy(msg, payload, copyLen);
  msg[copyLen] = '\0';
  Serial.printf("[MQTT] Payload: %s\n", msg);

  // ── Parse oneM2M response: m2m:rsp → pc → m2m:cin → con ──────
  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, msg);
  if (err) {
    Serial.printf("[MQTT] ⚠️ JSON parse error: %s\n", err.c_str());
    return;
  }

  // Ambil string "con" dari dalam nested oneM2M response
  const char* con = doc["m2m:rsp"]["pc"]["m2m:cin"]["con"] | nullptr;
  if (!con) {
    Serial.println("[MQTT] ⚠️ Field 'con' tidak ditemukan di payload.");
    return;
  }

  // Parse inner JSON di dalam "con" (berupa string JSON yang ter-escape)
  StaticJsonDocument<128> innerDoc;
  DeserializationError err2 = deserializeJson(innerDoc, con);
  if (err2) {
    Serial.printf("[MQTT] ⚠️ Inner JSON parse error: %s\n", err2.c_str());
    return;
  }

  const char* relayCmd = innerDoc["relay"] | nullptr;
  if (!relayCmd) {
    Serial.println("[MQTT] ⚠️ Field 'relay' tidak ada di pesan.");
    return;
  }

  if (strcmp(relayCmd, "ON") == 0) {
    setRelay(true);
  } else if (strcmp(relayCmd, "OFF") == 0) {
    setRelay(false);
  } else {
    Serial.printf("[MQTT] ⚠️ Perintah tidak dikenal: '%s'\n", relayCmd);
  }

  oledShowStatus();
}

// ─────────────────────────────────────────────────────────────────
// Non-blocking MQTT Reconnect + Resubscribe
// ─────────────────────────────────────────────────────────────────
void mqttReconnect() {
  static unsigned long tLastAttempt = 0;
  if (millis() - tLastAttempt < 5000UL) return;
  tLastAttempt = millis();

  String clientId = buildClientId();
  Serial.printf("[MQTT] Menghubungkan ke broker Antares... (ID: %s)\n", clientId.c_str());

  if (mqtt.connect(clientId.c_str(), ACCESS_KEY, ACCESS_KEY)) {
    Serial.printf("[MQTT] ✅ Terhubung!\n");

    // Subscribe ke topik response Antares (oneM2M)
    char subTopic[128];
    snprintf(subTopic, sizeof(subTopic),
             "/oneM2M/resp/antares-cse/%s/json", ACCESS_KEY);

    if (mqtt.subscribe(subTopic)) {
      Serial.printf("[MQTT] ✅ Subscribe: %s\n", subTopic);
    } else {
      Serial.println("[MQTT] ❌ Subscribe gagal!");
    }

    oledShowStatus();
  } else {
    Serial.printf("[MQTT] ❌ Gagal (state: %d) — coba lagi dalam 5 detik\n", mqtt.state());
  }
}

// ─────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW); // Relay OFF saat boot

  Serial.println("\n=== BAB 50 Program 2: MQTT Subscribe — Antares.id ===");
  Serial.printf("[INFO] Broker: %s:%d\n", MQTT_HOST, MQTT_PORT);

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
  mqtt.setCallback(onMqttMessage); // ← Daftarkan callback handler
  mqtt.setBufferSize(512);

  mqttReconnect();
}

// ─────────────────────────────────────────────────────────────────
void loop() {
  // ── MQTT Loop & Reconnect ─────────────────────────────────────
  if (!mqtt.connected()) {
    mqttReconnect();
  } else {
    mqtt.loop(); // Wajib — proses incoming messages & keepalive ping
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
    Serial.printf("[HB] Relay: %s | Msg: %lu | Heap: %u B | Uptime: %lu s\n",
                  relayState ? "ON" : "OFF", msgCount,
                  ESP.getFreeHeap(), millis() / 1000UL);
  }

  // ── Update OLED tiap 5 detik ──────────────────────────────────
  static unsigned long tOled = 0;
  if (millis() - tOled >= 5000UL) {
    tOled = millis();
    oledShowStatus();
  }
}
