/*
 * BAB 51 - Program 2: CoAP POST — Kirim Data DHT11 ke Antares.id
 *
 * Fitur:
 *   ► CoAP POST data DHT11 ke Antares IoT Platform tiap 15 detik
 *   ► Payload format oneM2M (m2m:cin) — standar Antares
 *   ► Response callback asinkron (2.01 Created = sukses)
 *   ► ArduinoJson stack-allocated (zero heap fragmentation)
 *   ► Non-blocking reconnect WiFi
 *   ► OLED tampilkan suhu, kelembaban, counter POST
 *
 * Hardware:
 *   DHT11 → IO27 (hardwired Bluino Kit)
 *   OLED  → SDA=IO21, SCL=IO22 (hardwired)
 *
 * Format Antares CoAP (oneM2M):
 *   URL    : coap://platform.antares.id:5683/~/ACCESS_KEY/APP/DEVICE
 *   Method : POST
 *   Payload: {"m2m:cin":{"con":"{\"temp\":29.1,\"humidity\":65}"}}
 *   Sukses : Response code 2.01 Created
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Konfigurasi WiFi ──────────────────────────────────────────────────
const char* WIFI_SSID = "NamaWiFiKamu";  // ← Ganti!
const char* WIFI_PASS = "PasswordWiFi";  // ← Ganti!

// ── Konfigurasi Antares CoAP ──────────────────────────────────────────
const char* ANTARES_HOST = "platform.antares.id";
const int   ANTARES_PORT = 5683;
const char* ACCESS_KEY   = "ACCESSKEY-ANDA-DISINI"; // ← Ganti!
const char* APP_NAME     = "bluino-kit";             // ← Ganti!
const char* DEVICE_NAME  = "sensor-ruangan";         // ← Ganti!

const unsigned long POST_INTERVAL = 15000UL;

// ── Hardware ──────────────────────────────────────────────────────────
#define DHT_PIN 27
DHT dht(DHT_PIN, DHT11);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool oledReady = false;

// ── State ─────────────────────────────────────────────────────────────
float    lastT     = NAN;
float    lastH     = NAN;
uint32_t postCount = 0;
uint32_t okCount   = 0;
char     coapPath[200];

// ── Objek CoAP ────────────────────────────────────────────────────────
WiFiUDP udp;
Coap    coap(udp);

// ── OLED Helper ───────────────────────────────────────────────────────
void updateOLED() {
  if (!oledReady) return;
  oled.clearDisplay(); oled.setTextColor(WHITE);
  oled.setTextSize(1); oled.setCursor(0, 0); oled.print("CoAP POST Antares");
  oled.drawLine(0, 10, 127, 10, WHITE);
  oled.setTextSize(2); oled.setCursor(0, 14);
  if (!isnan(lastT)) { oled.print(lastT, 1); oled.print("C"); }
  else oled.print("---");
  oled.setTextSize(1); oled.setCursor(0, 36);
  if (!isnan(lastH)) { oled.print("Humid: "); oled.print(lastH, 0); oled.print("%"); }
  oled.setCursor(0, 48);
  oled.print("Post:#"); oled.print(postCount);
  oled.print(" OK:#"); oled.print(okCount);
  oled.display();
}

// ── Callback: response dari Antares ───────────────────────────────────
void callbackResponse(CoapPacket &packet, IPAddress ip, int port) {
  uint8_t cls = packet.code >> 5;
  uint8_t dtl = packet.code & 0x1F;

  char respBuf[128] = "";
  size_t copyLen = min((size_t)packet.payloadlen, sizeof(respBuf) - 1);
  if (copyLen > 0) { memcpy(respBuf, packet.payload, copyLen); }

  Serial.printf("[CoAP] ← Response: %u.%02u", cls, dtl);
  if (strlen(respBuf)) Serial.printf(" | %s", respBuf);
  Serial.println();

  if (cls == 2) {
    okCount++;
    Serial.printf("[CoAP] ✅ Antares menerima data! Total OK: %u | Heap: %u B\n",
      okCount, ESP.getFreeHeap());
  } else {
    // 4.01 = Unauthorized (cek ACCESS_KEY)
    // 4.04 = Not Found    (cek APP_NAME / DEVICE_NAME)
    // 5.00 = Server Error (coba lagi nanti)
    Serial.printf("[CoAP] ⚠️ Error %u.%02u — cek konfigurasi Antares\n", cls, dtl);
  }
  updateOLED();
}

void setup() {
  Serial.begin(115200);
  dht.begin();
  Wire.begin(21, 22);

  // Bangun path CoAP Antares
  snprintf(coapPath, sizeof(coapPath), "/~/%s/%s/%s",
    ACCESS_KEY, APP_NAME, DEVICE_NAME);

  Serial.println("\n=== BAB 51 Program 2: CoAP POST — Antares.id ===");
  Serial.printf("[INFO] Host: %s:%d\n", ANTARES_HOST, ANTARES_PORT);
  Serial.printf("[INFO] Path: %s\n", coapPath);

  if (oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledReady = true;
    Serial.println("[OLED] ✅ Display terdeteksi di 0x3C");
  } else {
    Serial.println("[OLED] ⚠️ Display tidak terdeteksi.");
  }
  oled.clearDisplay(); oled.setTextColor(WHITE);
  oled.setCursor(0, 0);  oled.print("BAB 51 CoAP POST");
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
  Serial.printf("\n[WiFi] ✅ Terhubung! IP: %s\n", WiFi.localIP().toString().c_str());

  // Baca sensor awal
  float t = dht.readTemperature(), h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) {
    lastT = t; lastH = h;
    Serial.printf("[DHT] ✅ Baca OK: %.1f°C | %.0f%%\n", lastT, lastH);
  } else {
    Serial.println("[DHT] ⚠️ Gagal baca awal.");
  }

  // CoAP setup
  coap.response(callbackResponse);
  coap.start();
  Serial.println("[CoAP] ✅ Client siap");
  Serial.printf("[INFO] POST tiap %lu detik ke Antares\n", POST_INTERVAL / 1000UL);
}

void loop() {
  // Cek WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] ⚠️ Terputus! Reconnect...");
    WiFi.reconnect(); delay(5000); return;
  }

  // WAJIB: proses incoming UDP
  coap.loop();

  // Baca DHT11 tiap 2 detik
  static unsigned long tDHT = 0;
  if (millis() - tDHT >= 2000UL) {
    tDHT = millis();
    float t = dht.readTemperature(), h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) { lastT = t; lastH = h; }
  }

  // POST tiap POST_INTERVAL
  static unsigned long tPost = 0;
  static bool firstRun = true;
  if (firstRun || millis() - tPost >= POST_INTERVAL) {
    firstRun = false;
    tPost = millis();

    if (isnan(lastT)) {
      Serial.println("[POST] ⚠️ Skip — sensor belum valid.");
    } else {
      // Resolve Antares hostname
      IPAddress antaresIP;
      if (!WiFi.hostByName(ANTARES_HOST, antaresIP)) {
        Serial.println("[CoAP] ❌ DNS gagal — coba lagi nanti.");
      } else {
        // Bangun payload oneM2M (stack-allocated)
        char conStr[64];
        snprintf(conStr, sizeof(conStr),
          "{\"temp\":%.1f,\"humidity\":%.0f}", lastT, lastH);

        StaticJsonDocument<256> doc;
        doc["m2m:cin"]["con"] = conStr;
        char payload[256];
        size_t payLen = serializeJson(doc, payload, sizeof(payload));

        postCount++;
        Serial.printf("[DHT] %.1f°C | %.0f%%\n", lastT, lastH);
        Serial.printf("[CoAP] → POST #%u: %s\n", postCount, coapPath);
        Serial.printf("[CoAP] Payload: %s\n", payload);

        uint16_t msgId = coap.post(antaresIP, ANTARES_PORT, coapPath, payload);
        if (msgId > 0) {
          Serial.printf("[CoAP] Terkirim (msgId:%u) — tunggu response...\n", msgId);
        } else {
          Serial.println("[CoAP] ❌ Gagal kirim POST!");
        }
        updateOLED();
      }
    }
  }

  // Heartbeat
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] %.1fC %.0f%% | Post:%u OK:%u | Heap:%u B | Up:%lu s\n",
      isnan(lastT)?0.0f:lastT, isnan(lastH)?0.0f:lastH,
      postCount, okCount, ESP.getFreeHeap(), millis()/1000UL);
  }
}
