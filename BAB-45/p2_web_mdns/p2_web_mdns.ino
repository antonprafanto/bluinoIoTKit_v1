/*
 * BAB 45 - Program 2: Halaman Web di bluino.local
 * ESP32 menjalankan web server kecil dan mengumumkan layanan _http._tcp.
 * Buka dari browser di WiFi yang sama: http://bluino.local
 *
 * Rangkaian: tidak perlu (LED biru bawaan board di IO2).
 *   Opsional: LED3 (biru) → IO2.
 */

#include <WiFi.h>
#include <ESPmDNS.h>
#include <WebServer.h>

const char* WIFI_SSID     = "NamaWiFiKamu";      // ← ganti dengan WiFi kamu
const char* WIFI_PASSWORD = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu

const char* NAMA_MDNS = "bluino";  // → http://bluino.local

#define LED_BIRU 2  // LED biru bawaan board (dan LED3 kit) → IO2

WebServer server(80);  // web server di port 80 (port standar HTTP)

// Nama yang benar-benar dipakai mDNS (bisa menjadi "bluino-2" kalau namanya kembar)
String namaMdnsAktif() {
  char nama[MDNS_NAME_BUF_LEN];
  if (mdns_hostname_get(nama) == ESP_OK) {
    return String(nama);
  }
  return String(NAMA_MDNS);
}

// Dipanggil setiap kali ada yang membuka http://bluino.local/
void handleRoot() {
  String html;
  html += "<!DOCTYPE html><html lang=\"id\"><head><meta charset=\"UTF-8\">";
  html += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<title>ESP32 Bluino</title></head>";
  html += "<body style=\"font-family:sans-serif;max-width:480px;margin:auto;padding:16px\">";
  html += "<h1>Halo dari ESP32!</h1>";
  html += "<p>Nama mDNS: <b>http://" + namaMdnsAktif() + ".local</b></p>";
  html += "<p>Alamat IP: <b>" + WiFi.localIP().toString() + "</b></p>";
  html += "<p>Kekuatan sinyal WiFi: <b>" + String(WiFi.RSSI()) + " dBm</b></p>";
  html += "<p>Menyala sejak: <b>" + String(millis() / 1000) + " detik</b> yang lalu</p>";
  html += "<p>Muat ulang halaman ini untuk memperbarui angka.</p>";
  html += "</body></html>";
  server.send(200, "text/html", html);

  Serial.printf("Halaman dibuka dari %s\n", server.client().remoteIP().toString().c_str());
}

// Dipanggil kalau alamat yang diminta tidak dikenal, misalnya /abc
void handleNotFound() {
  server.send(404, "text/plain", "Halaman tidak ditemukan");
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BIRU, OUTPUT);
  digitalWrite(LED_BIRU, LOW);
  delay(500);
  Serial.println();
  Serial.println("BAB 45 - Program 2: Halaman Web di bluino.local");

  // 1. Sambungkan ke WiFi
  WiFi.setHostname(NAMA_MDNS);  // SEBELUM WiFi.mode()
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Menghubungkan ke WiFi");

  unsigned long mulai = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - mulai >= 20000) {  // menyerah setelah 20 detik
      Serial.println();
      Serial.println("Gagal terhubung! Periksa nama dan password WiFi (harus 2,4 GHz).");
      Serial.println("ESP32 restart dalam 5 detik...");
      delay(5000);
      ESP.restart();
    }
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("Terhubung! Alamat IP: ");
  Serial.println(WiFi.localIP());

  // 2. Mulai mDNS dan umumkan layanan web (DNS-SD)
  bool mdnsAktif = MDNS.begin(NAMA_MDNS);
  if (mdnsAktif) {
    MDNS.setInstanceName("Kit Bluino ESP32");                       // nama ramah di aplikasi pencari layanan
    MDNS.addService("http", "tcp", 80);                             // layanan _http._tcp di port 80
    MDNS.addServiceTxt("http", "tcp", "path", "/");                 // catatan TXT: halaman utama
    MDNS.addServiceTxt("http", "tcp", "board", "ESP32 DEVKIT V1");  // catatan TXT: info papan
    Serial.printf("mDNS aktif: %s.local\n", NAMA_MDNS);
    Serial.println("Layanan _http._tcp diumumkan di port 80");
  } else {
    Serial.println("Gagal memulai mDNS! Halaman tetap bisa dibuka lewat alamat IP.");
  }

  // 3. Jalankan web server
  server.on("/", handleRoot);
  server.onNotFound(handleNotFound);
  server.begin();
  if (mdnsAktif) {
    Serial.printf("Web server berjalan. Buka di browser: http://%s.local\n", NAMA_MDNS);
  }
  Serial.print("Kalau nama .local tidak bisa dibuka, pakai: http://");
  Serial.println(WiFi.localIP());
}

void loop() {
  server.handleClient();  // layani browser yang membuka halaman

  // LED biru menyala selama WiFi terhubung (halaman siap dibuka)
  digitalWrite(LED_BIRU, (WiFi.status() == WL_CONNECTED) ? HIGH : LOW);
}
