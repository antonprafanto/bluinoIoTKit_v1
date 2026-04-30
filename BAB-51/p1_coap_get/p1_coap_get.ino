/*
 * BAB 51 - Program 1: CoAP GET — Baca Data dari Server Publik (coap.me)
 *
 * Fitur:
 *   ► ESP32 sebagai CoAP Client (UDP, bukan TCP!)
 *   ► CoAP GET ke coap.me/hello tiap 30 detik
 *   ► Response callback asinkron — UDP stateless
 *   ► Decode CoAP response code (cls.dtl format)
 *   ► OLED tampilkan response & counter
 *   ► Non-blocking reconnect WiFi
 *   ► Heartbeat tiap 60 detik
 *
 * Hardware:
 *   OLED → SDA=IO21, SCL=IO22 (hardwired Bluino Kit)
 *
 * Library:
 *   coap-simple by hirotakaster (Library Manager)
 *
 * Server: coap.me — server publik Eclipse untuk testing CoAP
 *   Endpoint tersedia: /hello, /test, /sink (terima semua POST)
 *   Tidak perlu akun atau API key!
 *
 * Perbedaan dari HTTP GET (BAB 48):
 *   HTTP : TCP connect → request → response → disconnect (sinkron)
 *   CoAP : UDP packet → kirim → callback saat response tiba (asinkron)
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ── Konfigurasi WiFi ──────────────────────────────────────────────────
const char* WIFI_SSID = "NamaWiFiKamu";  // ← Ganti!
const char* WIFI_PASS = "PasswordWiFi";  // ← Ganti!

// ── CoAP Server (coap.me — public test server) ───────────────────────
const char* COAP_SERVER   = "coap.me";
const int   COAP_PORT     = 5683;
const char* COAP_RESOURCE = "/hello";

const unsigned long GET_INTERVAL = 30000UL;  // 30 detik

// ── Hardware ──────────────────────────────────────────────────────────
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool oledReady = false;

// ── State ─────────────────────────────────────────────────────────────
char     lastResp[128] = "Menunggu...";
uint32_t getCount = 0;
uint32_t okCount  = 0;

// ── Objek CoAP (UDP) ──────────────────────────────────────────────────
WiFiUDP udp;
Coap    coap(udp);

// ── OLED Helper ───────────────────────────────────────────────────────
void updateOLED() {
  if (!oledReady) return;
  oled.clearDisplay(); oled.setTextColor(WHITE); oled.setTextSize(1);
  oled.setCursor(0, 0);  oled.print("CoAP GET Client");
  oled.drawLine(0, 10, 127, 10, WHITE);
  oled.setCursor(0, 14); oled.print("coap.me/hello");
  oled.setCursor(0, 26); oled.print("Req:"); oled.print(getCount);
  oled.print("  OK:"); oled.print(okCount);
  oled.setCursor(0, 38); oled.print("Response:");
  oled.setCursor(0, 50); oled.print(lastResp);
  oled.display();
}

// ── Callback: dipanggil saat response CoAP diterima (ASINKRON!) ───────
void callbackResponse(CoapPacket &packet, IPAddress ip, int port) {
  // WAJIB: payload tidak null-terminated!
  memset(lastResp, 0, sizeof(lastResp));
  size_t copyLen = min((size_t)packet.payloadlen, sizeof(lastResp) - 1);
  if (copyLen > 0) memcpy(lastResp, packet.payload, copyLen);

  // Decode response code: high 3 bits = class, low 5 bits = detail
  uint8_t cls = packet.code >> 5;
  uint8_t dtl = packet.code & 0x1F;

  Serial.printf("[CoAP] ← Response dari %s\n", ip.toString().c_str());
  Serial.printf("[CoAP] Code: %u.%02u | Payload: %s\n", cls, dtl, lastResp);

  if (cls == 2) {
    okCount++;
    Serial.printf("[CoAP] ✅ Sukses! Heap: %u B\n", ESP.getFreeHeap());
  } else {
    Serial.printf("[CoAP] ⚠️ Error response: %u.%02u\n", cls, dtl);
  }
  updateOLED();
}

void setup() {
  Serial.begin(115200);
  Wire.begin(21, 22);

  Serial.println("\n=== BAB 51 Program 1: CoAP GET — coap.me ===");
  Serial.printf("[INFO] Target: coap://%s:%d%s\n", COAP_SERVER, COAP_PORT, COAP_RESOURCE);

  // OLED
  if (oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    oledReady = true;
    Serial.println("[OLED] ✅ Display terdeteksi di 0x3C");
    oled.clearDisplay(); oled.setTextColor(WHITE); oled.setTextSize(1);
    oled.setCursor(0, 0);  oled.print("BAB 51 CoAP GET");
    oled.setCursor(0, 12); oled.print("WiFi..."); oled.display();
  } else {
    Serial.println("[OLED] ⚠️ Display tidak terdeteksi — lanjut tanpa OLED.");
  }

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

  // CoAP setup — WAJIB: daftarkan callback SEBELUM coap.start()!
  coap.response(callbackResponse);
  coap.start();  // Buka UDP listener di port 5683
  Serial.println("[CoAP] ✅ Client siap (UDP port 5683)");
  Serial.printf("[INFO] GET tiap %lu detik\n", GET_INTERVAL / 1000UL);
}

void loop() {
  // Cek WiFi
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[WiFi] ⚠️ Terputus! Reconnect...");
    WiFi.reconnect(); delay(5000); return;
  }

  // WAJIB: proses semua incoming UDP (response CoAP)
  coap.loop();

  // CoAP GET tiap GET_INTERVAL
  static unsigned long tGet = 0;
  static bool firstRun = true;
  if (firstRun || millis() - tGet >= GET_INTERVAL) {
    firstRun = false;
    tGet = millis();
    getCount++;

    // Resolve hostname ke IP (DNS)
    IPAddress serverIP;
    if (!WiFi.hostByName(COAP_SERVER, serverIP)) {
      Serial.printf("[CoAP] ❌ DNS gagal resolve '%s'\n", COAP_SERVER);
      return;
    }

    Serial.printf("[CoAP] → GET #%u: coap://%s%s\n", getCount, COAP_SERVER, COAP_RESOURCE);
    // coap.get() mengirim paket UDP dan langsung return msgId
    // Response akan datang ASINKRON via callbackResponse()
    uint16_t msgId = coap.get(serverIP, COAP_PORT, COAP_RESOURCE);
    if (msgId > 0) {
      Serial.printf("[CoAP] Terkirim (msgId:%u) — tunggu response...\n", msgId);
    } else {
      Serial.println("[CoAP] ❌ Gagal kirim — cek koneksi WiFi.");
    }
    updateOLED();
  }

  // Heartbeat
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] GET:%u OK:%u | Heap:%u B | Uptime:%lu s\n",
      getCount, okCount, ESP.getFreeHeap(), millis() / 1000UL);
  }
}
