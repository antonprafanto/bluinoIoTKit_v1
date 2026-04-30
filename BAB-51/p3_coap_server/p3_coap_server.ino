/*
 * BAB 51 - Program 3: CoAP Server — ESP32 sebagai Endpoint IoT
 *
 * Arsitektur: Edge IoT — ESP32 melayani request langsung, tanpa cloud
 *
 * Fitur:
 *   ► ESP32 berjalan sebagai CoAP Server di port 5683
 *   ► Endpoint /sensor → GET: return JSON suhu & kelembaban DHT11
 *   ► Endpoint /relay  → GET: return status relay
 *                      → PUT: ubah status relay (payload: "ON" / "OFF")
 *   ► Semua CoAP client di jaringan LAN yang sama dapat query ESP32 ini
 *   ► OLED tampilkan IP, request counter, suhu
 *   ► Non-blocking DHT11 read tiap 2 detik
 *   ► Heartbeat tiap 60 detik
 *
 * Hardware:
 *   DHT11 → IO27, Relay → IO4, OLED → SDA=IO21, SCL=IO22
 *
 * Cara Uji (dari laptop/PC di jaringan yang sama):
 *   Install coap-client: sudo apt install libcoap-bin  (Linux)
 *                        brew install libcoap           (macOS)
 *
 *   GET sensor: coap-client -m get coap://192.168.1.105/sensor
 *   GET relay : coap-client -m get coap://192.168.1.105/relay
 *   SET relay : coap-client -m put coap://192.168.1.105/relay -e "ON"
 *
 *   Atau gunakan MQTT Explorer versi CoAP: Copper (Firefox addon)
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

// ── Hardware ──────────────────────────────────────────────────────────
#define DHT_PIN   27
#define RELAY_PIN 4
DHT dht(DHT_PIN, DHT11);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool oledReady = false;

// ── State ─────────────────────────────────────────────────────────────
float    lastT      = NAN;
float    lastH      = NAN;
bool     relayState = false;
uint32_t reqCount   = 0;

// ── Objek CoAP Server ─────────────────────────────────────────────────
WiFiUDP udp;
Coap    coap(udp);

// ── OLED Helper ───────────────────────────────────────────────────────
void updateOLED() {
  if (!oledReady) return;
  oled.clearDisplay(); oled.setTextColor(WHITE); oled.setTextSize(1);
  oled.setCursor(0, 0); oled.print("CoAP Server");
  oled.setCursor(0, 10); oled.print("IP:"); oled.print(WiFi.localIP());
  oled.drawLine(0, 20, 127, 20, WHITE);
  oled.setTextSize(2); oled.setCursor(0, 24);
  if (!isnan(lastT)) { oled.print(lastT, 1); oled.print("C"); }
  else oled.print("---");
  oled.setTextSize(1); oled.setCursor(0, 46);
  oled.print("Req:#"); oled.print(reqCount);
  oled.print(" RLY:"); oled.print(relayState ? "ON" : "OFF");
  oled.display();
}

// ── Callback: Endpoint /sensor ────────────────────────────────────────
// Melayani GET /sensor → return JSON suhu & kelembaban
void callbackSensor(CoapPacket &packet, IPAddress clientIP, int clientPort) {
  reqCount++;
  Serial.printf("[CoAP] ← %s GET /sensor (#%u)\n",
    clientIP.toString().c_str(), reqCount);

  // Bangun JSON response (float langsung → tidak ada ambiguitas type)
  StaticJsonDocument<128> doc;
  if (!isnan(lastT)) {
    doc["temp"]     = lastT;           // → 29.1 (number, bukan "29.1" string)
    doc["humidity"] = (int)lastH;      // → 65   (integer)
  } else {
    doc["error"] = "sensor_unavailable";
  }
  doc["uptime"] = millis() / 1000UL;

  char payload[128];
  size_t payLen = serializeJson(doc, payload, sizeof(payload));

  Serial.printf("[CoAP] → Response: %s\n", payload);

  // Kirim response 2.05 Content
  coap.sendResponse(clientIP, clientPort, packet.messageid,
    payload, payLen,
    COAP_RESPONSE_CODE(CONTENT),
    COAP_CONTENT_TYPE(APPLICATION_JSON),
    packet.token, packet.tokenlen);

  updateOLED();
}

// ── Callback: Endpoint /relay ─────────────────────────────────────────
// Melayani GET /relay → return status
//           PUT /relay → ubah status (payload: "ON" / "OFF")
void callbackRelay(CoapPacket &packet, IPAddress clientIP, int clientPort) {
  reqCount++;

  if (packet.code == COAP_GET) {
    // GET: balas status relay
    Serial.printf("[CoAP] ← %s GET /relay (#%u)\n",
      clientIP.toString().c_str(), reqCount);

    char payload[32];
    snprintf(payload, sizeof(payload), "{\"relay\":\"%s\"}",
      relayState ? "ON" : "OFF");

    coap.sendResponse(clientIP, clientPort, packet.messageid,
      payload, strlen(payload),
      COAP_RESPONSE_CODE(CONTENT),
      COAP_CONTENT_TYPE(APPLICATION_JSON),
      packet.token, packet.tokenlen);

    Serial.printf("[CoAP] → Response: %s\n", payload);

  } else if (packet.code == COAP_PUT) {
    // PUT: ubah status relay
    char cmd[16] = "";
    size_t copyLen = min((size_t)packet.payloadlen, sizeof(cmd) - 1);
    if (copyLen > 0) { memcpy(cmd, packet.payload, copyLen); }

    Serial.printf("[CoAP] ← %s PUT /relay cmd='%s' (#%u)\n",
      clientIP.toString().c_str(), cmd, reqCount);

    if (strcmp(cmd, "ON") == 0) {
      relayState = true;
      digitalWrite(RELAY_PIN, HIGH);
      Serial.println("[RELAY] ✅ Relay → ON");
      coap.sendResponse(clientIP, clientPort, packet.messageid,
        "OK", 2, COAP_RESPONSE_CODE(CHANGED),
        COAP_CONTENT_TYPE(TEXT_PLAIN),
        packet.token, packet.tokenlen);
    } else if (strcmp(cmd, "OFF") == 0) {
      relayState = false;
      digitalWrite(RELAY_PIN, LOW);
      Serial.println("[RELAY] ✅ Relay → OFF");
      coap.sendResponse(clientIP, clientPort, packet.messageid,
        "OK", 2, COAP_RESPONSE_CODE(CHANGED),
        COAP_CONTENT_TYPE(TEXT_PLAIN),
        packet.token, packet.tokenlen);
    } else {
      Serial.printf("[RELAY] ⚠️ Perintah tidak dikenal: '%s'\n", cmd);
      coap.sendResponse(clientIP, clientPort, packet.messageid,
        "BAD_REQUEST", 11, COAP_RESPONSE_CODE(BAD_REQUEST),
        COAP_CONTENT_TYPE(TEXT_PLAIN),
        packet.token, packet.tokenlen);
    }
  }
  updateOLED();
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT); digitalWrite(RELAY_PIN, LOW);
  dht.begin();
  Wire.begin(21, 22);

  Serial.println("\n=== BAB 51 Program 3: CoAP Server — ESP32 Endpoint ===");

  if (oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledReady = true;
    Serial.println("[OLED] ✅ Display terdeteksi di 0x3C");
  }
  oled.clearDisplay(); oled.setTextColor(WHITE);
  oled.setCursor(0, 0);  oled.print("BAB 51 CoAP Srv");
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
  Serial.printf("\n[WiFi] ✅ Terhubung! IP: %s\n",
    WiFi.localIP().toString().c_str());

  // CoAP Server setup — daftarkan endpoint SEBELUM coap.start()
  coap.server(callbackSensor, "sensor");  // → coap://IP/sensor
  coap.server(callbackRelay,  "relay");   // → coap://IP/relay
  coap.start();                           // Buka port UDP 5683

  Serial.println("[CoAP] ✅ Server siap di port 5683");
  Serial.printf("[CoAP] Endpoint: coap://%s/sensor\n", WiFi.localIP().toString().c_str());
  Serial.printf("[CoAP] Endpoint: coap://%s/relay\n", WiFi.localIP().toString().c_str());
  Serial.println("[CoAP] Cara uji: coap-client -m get coap://IP/sensor");

  // Baca sensor awal
  float t = dht.readTemperature(), h = dht.readHumidity();
  if (!isnan(t) && !isnan(h)) { lastT = t; lastH = h; }
  updateOLED();
}

void loop() {
  // Cek WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] ⚠️ Terputus! Reconnect...");
    WiFi.reconnect(); delay(5000); return;
  }

  // WAJIB: proses semua incoming CoAP request
  coap.loop();

  // Baca DHT11 non-blocking tiap 2 detik
  static unsigned long tDHT = 0;
  if (millis() - tDHT >= 2000UL) {
    tDHT = millis();
    float t = dht.readTemperature(), h = dht.readHumidity();
    if (!isnan(t) && !isnan(h)) { lastT = t; lastH = h; }
  }

  // Heartbeat
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] %.1fC %.0f%% | Relay:%s | Req:%u | Heap:%u B | Up:%lu s\n",
      isnan(lastT)?0.0f:lastT, isnan(lastH)?0.0f:lastH,
      relayState?"ON":"OFF", reqCount, ESP.getFreeHeap(), millis()/1000UL);
  }
}
