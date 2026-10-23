/*
 * BAB 43 - Program 4: Panel LED Lewat Browser
 * ESP32 menjadi hotspot "Bluino-LED" sekaligus web server.
 * Buka http://192.168.4.1 dari HP yang tersambung ke hotspot itu.
 * Halaman tidak memuat apa pun dari internet (aman untuk mode AP).
 */

#include <WiFi.h>
#include <WebServer.h>

// ── Pengaturan hotspot ─────────────────────────────────────────────
const char *AP_SSID     = "Bluino-LED";
const char *AP_PASSWORD = "bluino123";

// ── Pin LED (Peta Pin Standar) ─────────────────────────────────────
#define LED_MERAH  4   // LED1
#define LED_HIJAU  17  // LED2
#define LED_BIRU   2   // LED3
#define LED_KUNING 16  // LED4

const int JUMLAH_LED = 4;
const int pinLed[JUMLAH_LED]       = {LED_MERAH, LED_HIJAU, LED_BIRU, LED_KUNING};
const char *namaLed[JUMLAH_LED]    = {"Merah", "Hijau", "Biru", "Kuning"};
const char *warnaLed[JUMLAH_LED]   = {"#e53935", "#43a047", "#1e88e5", "#fdd835"};
bool ledNyala[JUMLAH_LED]          = {false, false, false, false};

WebServer server(80);  // web server di port 80 (port bawaan HTTP)

// Susun halaman HTML berdasarkan keadaan LED saat ini
String buatHalaman() {
  String html;
  html.reserve(3000);
  html += "<!DOCTYPE html><html lang=\"id\"><head><meta charset=\"utf-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<title>Panel LED ESP32</title><style>";
  html += "body{font-family:sans-serif;max-width:420px;margin:auto;padding:16px;";
  html += "background:#f4f6f8;color:#222}";
  html += ".kartu{background:#fff;border-radius:12px;padding:12px 16px;margin:12px 0;";
  html += "box-shadow:0 1px 4px rgba(0,0,0,.2)}";
  html += ".lampu{display:inline-block;width:18px;height:18px;border-radius:50%;";
  html += "border:2px solid #888;vertical-align:middle;margin-right:8px}";
  html += ".tombol{display:inline-block;padding:12px 18px;margin:8px 8px 0 0;";
  html += "border-radius:8px;color:#fff;text-decoration:none;font-size:18px}";
  html += ".nyala{background:#2e7d32}.mati{background:#616161}";
  html += "</style></head><body>";
  html += "<h1>Panel LED ESP32</h1>";
  html += "<p>Hotspot: <b>" + String(AP_SSID) + "</b><br>";
  html += "Perangkat tersambung: <b>" + String(WiFi.softAPgetStationNum()) + "</b></p>";

  for (int i = 0; i < JUMLAH_LED; i++) {
    html += "<div class=\"kartu\"><span class=\"lampu\" style=\"background:";
    html += ledNyala[i] ? warnaLed[i] : "#ddd";
    html += "\"></span><b>LED " + String(namaLed[i]) + "</b>: ";
    html += ledNyala[i] ? "NYALA" : "MATI";
    html += "<br><a class=\"tombol nyala\" href=\"/atur?led=" + String(i) + "&nyala=1\">Nyalakan</a>";
    html += "<a class=\"tombol mati\" href=\"/atur?led=" + String(i) + "&nyala=0\">Matikan</a></div>";
  }

  html += "</body></html>";
  return html;
}

// Alamat "/" → kirim halaman
void handleRoot() {
  server.send(200, "text/html", buatHalaman());
}

// Alamat "/atur?led=0&nyala=1" → ubah LED, lalu kembali ke "/"
void handleAtur() {
  String teksLed = server.arg("led");      // "" kalau tidak ada
  String teksNyala = server.arg("nyala");

  if (teksLed.length() == 1 && isDigit(teksLed[0])) {
    int i = teksLed.toInt();
    if (i < JUMLAH_LED) {
      ledNyala[i] = (teksNyala == "1");
      digitalWrite(pinLed[i], ledNyala[i] ? HIGH : LOW);
      Serial.printf("LED %s -> %s\n", namaLed[i], ledNyala[i] ? "NYALA" : "MATI");
    }
  }

  // Suruh browser kembali ke halaman utama
  server.sendHeader("Location", "/");
  server.send(303);
}

// Alamat lain → pesan "tidak ditemukan"
void handleTidakDitemukan() {
  server.send(404, "text/plain", "Halaman tidak ditemukan. Buka http://192.168.4.1");
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("=== BAB 43 Program 4: Panel LED Lewat Browser ===");

  // pinMode dulu, baru digitalWrite
  for (int i = 0; i < JUMLAH_LED; i++) {
    pinMode(pinLed[i], OUTPUT);
    digitalWrite(pinLed[i], LOW);
  }

  WiFi.mode(WIFI_AP);
  if (!WiFi.softAP(AP_SSID, AP_PASSWORD)) {
    Serial.println("Gagal membuat hotspot! Periksa kata sandi (minimal 8 karakter).");
    while (true) {
      delay(1000);
    }
  }
  Serial.printf("Hotspot aktif: %s (kata sandi: %s)\n", AP_SSID, AP_PASSWORD);
  Serial.printf("Buka di browser: http://%s\n", WiFi.softAPIP().toString().c_str());

  server.on("/", handleRoot);
  server.on("/atur", handleAtur);
  server.onNotFound(handleTidakDitemukan);
  server.begin();
  Serial.println("Web server berjalan di port 80.");
}

void loop() {
  server.handleClient();  // layani permintaan dari browser
}
