/*
 * BAB 56 - Program 1: Matter Data Model Simulator
 *
 * ESP32 Classic yang mensimulasikan Matter Device via HTTP REST API
 *
 * Matter Data Model yang disimulasikan:
 *   Node: ESP32 (NodeId = MAC address)
 *   ├── Endpoint 0: Root (info node)
 *   ├── Endpoint 1: On/Off Light (OnOff Cluster) → Relay IO4
 *   └── Endpoint 2: Temp/Humidity Sensor → DHT11 IO27
 *
 * Cara uji:
 *   1. Ganti WIFI_SSID dan WIFI_PASS
 *   2. Upload ke ESP32 → buka Serial Monitor → catat IP address
 *   3. Buka browser: http://<IP_ESP32>
 *   4. Eksplorasi API: http://<IP_ESP32>/matter
 *
 * Hardware: DHT11 → IO27 | Relay → IO4
 * Library : WiFi.h, WebServer.h, ArduinoJson (sudah ada dari BAB 55)
 */

#include <WiFi.h>
#include <WebServer.h>
#include <DHT.h>
#include <ArduinoJson.h>

// ── Konfigurasi WiFi — GANTI dengan jaringan kamu! ────────────────────
const char* WIFI_SSID = "SSID_KAMU";
const char* WIFI_PASS = "PASSWORD_WIFI";

// ── Hardware ──────────────────────────────────────────────────────────
#define DHT_PIN   27
#define RELAY_PIN  4
DHT dht(DHT_PIN, DHT11);

WebServer server(80);

// ── Matter Data Model (disederhanakan) ────────────────────────────────
struct {
  char   nodeId[18];       // MAC address sebagai NodeID
  bool   ep1_onOff;        // Endpoint 1: OnOff Cluster → Attribute OnOff
  float  ep2_temp;         // Endpoint 2: Temperature Measurement Cluster
  float  ep2_hum;          // Endpoint 2: Relative Humidity Cluster
} matterNode;

uint32_t cmdCount   = 0;
uint32_t readCount  = 0;

// ── Baca sensor DHT11 ──────────────────────────────────────────────────
void readSensor() {
  float t = dht.readTemperature(), h = dht.readHumidity();
  if (!isnan(t)) matterNode.ep2_temp = t;
  if (!isnan(h)) matterNode.ep2_hum  = h;
}

// ── Set relay & sync dengan Matter state ──────────────────────────────
void setOnOff(bool on) {
  matterNode.ep1_onOff = on;
  digitalWrite(RELAY_PIN, on ? HIGH : LOW);
  cmdCount++;
  Serial.printf("[MATTER] Cmd #%u: Endpoint1/OnOff → %s\n", cmdCount, on?"ON":"OFF");
}

// ── Handler: GET / (dashboard HTML) ──────────────────────────────────
void handleDashboard() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width'>";
  html += "<title>Matter Simulator — Bluino IoT Kit</title>";
  html += "<style>*{box-sizing:border-box}body{font-family:monospace;background:#0d1117;color:#c9d1d9;padding:20px;max-width:600px;margin:auto}";
  html += "h1{color:#58a6ff}h2{color:#3fb950;border-bottom:1px solid #30363d;padding-bottom:4px}";
  html += ".card{background:#161b22;border:1px solid #30363d;border-radius:6px;padding:16px;margin:12px 0}";
  html += ".badge{display:inline-block;padding:2px 8px;border-radius:4px;font-size:12px}";
  html += ".on{background:#1a4731;color:#3fb950}.off{background:#2d1c1c;color:#f85149}";
  html += "button{background:#238636;color:#fff;border:none;padding:8px 20px;border-radius:6px;cursor:pointer;font-size:14px}";
  html += "button:hover{background:#2ea043}pre{background:#0d1117;padding:12px;border-radius:4px;overflow-x:auto;font-size:12px}</style></head><body>";
  html += "<h1>🔌 Matter Device Simulator</h1>";
  html += "<div class='card'><b>NodeID:</b> " + String(matterNode.nodeId) + "<br>";
  html += "<b>Vendor:</b> Bluino &nbsp;|&nbsp; <b>Product:</b> IoT Kit 2026</div>";

  html += "<h2>Endpoint 1 — On/Off Light (Relay)</h2><div class='card'>";
  html += "<b>Cluster:</b> OnOff &nbsp; <b>Attribute:</b> OnOff = ";
  html += matterNode.ep1_onOff ? "<span class='badge on'>TRUE (ON)</span>" : "<span class='badge off'>FALSE (OFF)</span>";
  html += "<br><br><button onclick=\"fetch('/matter/1/onoff',{method:'PUT',headers:{'Content-Type':'application/json'},body:JSON.stringify({toggle:true})}).then(()=>location.reload())\">⚡ TOGGLE</button></div>";

  html += "<h2>Endpoint 2 — Temperature & Humidity Sensor</h2><div class='card'>";
  html += "<b>Cluster:</b> TemperatureMeasurement<br>";
  html += "<b>MeasuredValue:</b> " + String((int)(matterNode.ep2_temp * 100)) + " (= " + String(matterNode.ep2_temp, 1) + "°C)<br>";
  html += "<b>Cluster:</b> RelativeHumidityMeasurement<br>";
  html += "<b>MeasuredValue:</b> " + String((int)(matterNode.ep2_hum * 100)) + " (= " + String(matterNode.ep2_hum, 0) + "%)";
  html += "</div>";

  html += "<h2>Matter REST API</h2><div class='card'><pre>";
  html += "GET  /matter            → Semua endpoint (Node info)\n";
  html += "GET  /matter/1/onoff    → Baca OnOff attribute\n";
  html += "PUT  /matter/1/onoff    → Tulis: {\"value\":true}\n";
  html += "PUT  /matter/1/onoff    → Toggle: {\"toggle\":true}\n";
  html += "GET  /matter/2/temp     → Baca suhu (MeasuredValue)\n";
  html += "GET  /matter/2/humidity → Baca kelembaban</pre></div>";
  html += "</body></html>";
  server.send(200, "text/html", html);
}

// ── Handler: GET /matter (semua endpoint) ─────────────────────────────
void handleMatterRoot() {
  readCount++;
  DynamicJsonDocument doc(512);
  doc["nodeId"]       = matterNode.nodeId;
  doc["vendorName"]   = "Bluino";
  doc["productName"]  = "IoT Kit 2026";
  doc["swVersion"]    = "1.0.0";
  doc["readCount"]    = readCount;

  JsonArray eps = doc.createNestedArray("endpoints");

  JsonObject ep0 = eps.createNestedObject();
  ep0["endpointId"] = 0; ep0["deviceType"] = "Root Node";

  JsonObject ep1 = eps.createNestedObject();
  ep1["endpointId"] = 1; ep1["deviceType"] = "On/Off Light";
  ep1["cluster"]    = "OnOff";
  ep1["OnOff"]      = matterNode.ep1_onOff;

  JsonObject ep2 = eps.createNestedObject();
  ep2["endpointId"]    = 2; ep2["deviceType"] = "Temperature Sensor";
  ep2["tempCelsius"]   = round(matterNode.ep2_temp * 10) / 10.0;
  ep2["humidityPct"]   = round(matterNode.ep2_hum);
  ep2["tempRaw"]       = (int)(matterNode.ep2_temp * 100); // Matter unit: 0.01°C

  String resp;
  serializeJsonPretty(doc, resp);
  server.send(200, "application/json", resp);
  Serial.printf("[HTTP] GET /matter → %u bytes\n", resp.length());
}

// ── Handler: GET /matter/1/onoff ──────────────────────────────────────
void handleReadOnOff() {
  DynamicJsonDocument doc(128);
  doc["endpointId"] = 1;
  doc["cluster"]    = "OnOff";
  doc["attribute"]  = "OnOff";
  doc["value"]      = matterNode.ep1_onOff;
  String resp; serializeJson(doc, resp);
  server.send(200, "application/json", resp);
}

// ── Handler: PUT /matter/1/onoff ──────────────────────────────────────
void handleWriteOnOff() {
  if (!server.hasArg("plain")) {
    server.send(400, "application/json", "{\"error\":\"Body kosong\"}");
    return;
  }
  DynamicJsonDocument req(64);
  if (deserializeJson(req, server.arg("plain")) != DeserializationError::Ok) {
    server.send(400, "application/json", "{\"error\":\"JSON tidak valid\"}");
    return;
  }
  bool newState;
  if (req.containsKey("toggle") && req["toggle"].as<bool>()) {
    newState = !matterNode.ep1_onOff;
  } else {
    newState = req["value"] | false;
  }
  setOnOff(newState);

  DynamicJsonDocument resp(64);
  resp["endpointId"] = 1;
  resp["attribute"]  = "OnOff";
  resp["value"]      = matterNode.ep1_onOff;
  String respStr; serializeJson(resp, respStr);
  server.send(200, "application/json", respStr);
}

// ── Handler: GET /matter/2/temp & /matter/2/humidity ──────────────────
void handleReadTemp() {
  DynamicJsonDocument doc(128);
  doc["endpointId"]   = 2;
  doc["cluster"]      = "TemperatureMeasurement";
  doc["attribute"]    = "MeasuredValue";
  doc["value"]        = (int)(matterNode.ep2_temp * 100); // Matter unit: 0.01°C
  doc["displayValue"] = String(matterNode.ep2_temp, 1) + "°C";
  String resp; serializeJson(doc, resp);
  server.send(200, "application/json", resp);
}

void handleReadHumidity() {
  DynamicJsonDocument doc(128);
  doc["endpointId"]   = 2;
  doc["cluster"]      = "RelativeHumidityMeasurement";
  doc["attribute"]    = "MeasuredValue";
  doc["value"]        = (int)(matterNode.ep2_hum * 100); // Matter unit: 0.01%
  doc["displayValue"] = String(matterNode.ep2_hum, 0) + "%";
  String resp; serializeJson(doc, resp);
  server.send(200, "application/json", resp);
}

void setup() {
  Serial.begin(115200);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);
  dht.begin();
  Serial.println("\n=== BAB 56 Program 1: Matter Data Model Simulator ===");

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  Serial.print("[WiFi] Menghubungkan ke '" + String(WIFI_SSID) + "'");
  while (WiFi.status() != WL_CONNECTED) { delay(500); Serial.print("."); }
  Serial.printf("\n[WiFi] ✅ Terhubung! IP: %s\n", WiFi.localIP().toString().c_str());

  // Init Matter Node info
  String mac = WiFi.macAddress();
  mac.toCharArray(matterNode.nodeId, sizeof(matterNode.nodeId));
  matterNode.ep1_onOff = false;

  // Baca sensor awal
  readSensor();
  Serial.printf("[DHT] ✅ Awal: %.1f°C | %.0f%%\n", matterNode.ep2_temp, matterNode.ep2_hum);

  // Daftarkan semua route HTTP
  server.on("/",                  HTTP_GET,  handleDashboard);
  server.on("/matter",            HTTP_GET,  handleMatterRoot);
  server.on("/matter/1/onoff",    HTTP_GET,  handleReadOnOff);
  server.on("/matter/1/onoff",    HTTP_PUT,  handleWriteOnOff);
  server.on("/matter/2/temp",     HTTP_GET,  handleReadTemp);
  server.on("/matter/2/humidity", HTTP_GET,  handleReadHumidity);
  server.begin();

  Serial.println("[MATTER] ✅ Simulator aktif!");
  Serial.printf("[MATTER] Dashboard : http://%s\n", WiFi.localIP().toString().c_str());
  Serial.printf("[MATTER] Node Info : http://%s/matter\n", WiFi.localIP().toString().c_str());
  Serial.printf("[MATTER] OnOff API : http://%s/matter/1/onoff\n", WiFi.localIP().toString().c_str());
  Serial.printf("[MATTER] Temp API  : http://%s/matter/2/temp\n", WiFi.localIP().toString().c_str());
}

void loop() {
  server.handleClient();

  // Baca sensor tiap 5 detik
  static unsigned long tSensor = 0;
  if (millis() - tSensor >= 5000UL) {
    tSensor = millis();
    readSensor();
  }

  // Heartbeat
  static unsigned long tHb = 0;
  if (millis() - tHb >= 60000UL) {
    tHb = millis();
    Serial.printf("[HB] OnOff:%s | T:%.1f | H:%.0f | Cmd:%u | Heap:%u B\n",
      matterNode.ep1_onOff?"ON":"OFF", matterNode.ep2_temp,
      matterNode.ep2_hum, cmdCount, ESP.getFreeHeap());
  }
}
