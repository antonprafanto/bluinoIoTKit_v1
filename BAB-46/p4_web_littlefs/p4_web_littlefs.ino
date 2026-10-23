/*
 * BAB 46 - Program 4: Halaman Web dari LittleFS
 * File halaman (index.html, style.css, app.js) ada di folder "data" dan
 * diunggah ke LittleFS dengan plugin arduino-littlefs-upload (BAB 40).
 * - GET /           -> /index.html dari LittleFS (server.streamFile)
 * - GET /style.css, /app.js, dll. -> file dari LittleFS (server.serveStatic)
 * - GET /api/sensor -> data sensor dalam JSON (sama dengan Program 2)
 * Rangkaian: DHT (header hijau) -> IO27, VR10K (header kuning) -> IO34
 * Tools -> Partition Scheme: Default 4MB with spiffs
 */

#include <WiFi.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <DHT.h>

const char *NAMA_WIFI = "NamaWiFiKamu";          // ← ganti dengan WiFi kamu
const char *PASSWORD_WIFI = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu

#define PIN_DHT 27
#define TIPE_DHT DHT11
#define PIN_POT 34  // potensiometer VR10K (ADC1)

// struct Timer dari BAB 16
struct Timer {
  unsigned long interval;   // ms
  unsigned long waktuLalu;  // hasil millis() saat terakhir dijalankan

  // Mengembalikan true kalau sudah waktunya, lalu mencatat waktunya
  bool sudahWaktunya(unsigned long sekarang) {
    if (sekarang - waktuLalu >= interval) {
      waktuLalu = sekarang;
      return true;
    }
    return false;
  }
};

WebServer server(80);
DHT dht(PIN_DHT, TIPE_DHT);
Timer timerDHT = {2000, 0};  // baca DHT11 setiap 2 detik

float suhu = NAN;
float kelembapan = NAN;
bool dhtOk = false;
bool bacaanPertama = true;

void catatRequest(int kodeStatus) {
  Serial.printf("[HTTP] %s %s -> %d (dari %s)\n",
                http_method_str(server.method()), server.uri().c_str(),
                kodeStatus, server.client().remoteIP().toString().c_str());
}

void bacaDHT() {
  dht.read(true);  // paksa membaca sensor sekarang (±25 ms), lihat BAB 26
  float h = dht.readHumidity();
  float t = dht.readTemperature();
  bool berhasil = !isnan(h) && !isnan(t);

  if (bacaanPertama || berhasil != dhtOk) {
    if (berhasil) {
      Serial.printf("[DHT11] OK: %.1f °C, %.0f %%RH\n", t, h);
    } else {
      Serial.println("[DHT11] Gagal membaca. Periksa kabel DHT -> IO27.");
    }
  }
  bacaanPertama = false;
  dhtOk = berhasil;
  if (berhasil) {
    suhu = t;
    kelembapan = h;
  }
}

int bacaPotPersen() {
  return analogRead(PIN_POT) * 100 / 4095;
}

// Handler untuk GET /api/sensor (dipanggil app.js setiap 2 detik, tidak dicatat)
void kirimSensor() {
  JsonDocument doc;
  if (dhtOk) {
    doc["suhu"] = suhu;
    doc["kelembapan"] = kelembapan;
  } else {
    doc["suhu"] = nullptr;
    doc["kelembapan"] = nullptr;
  }
  doc["dht_ok"] = dhtOk;
  doc["potensiometer"] = bacaPotPersen();
  doc["waktu_nyala"] = millis() / 1000;
  doc["rssi"] = WiFi.RSSI();

  String json;
  serializeJson(doc, json);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

// Handler untuk GET /: kirim isi file /index.html
void halamanUtama() {
  File f = LittleFS.open("/index.html", "r");
  if (!f) {
    server.send(500, "text/plain",
                "File /index.html tidak ada di LittleFS. Unggah folder data dulu (lihat BAB 40).");
    catatRequest(500);
    return;
  }
  server.streamFile(f, "text/html");  // kirim isi file sedikit demi sedikit
  f.close();
  catatRequest(200);
}

void halaman404() {
  server.send(404, "text/plain", "404 - alamat tidak ditemukan");
  catatRequest(404);
}

void sambungkanWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(NAMA_WIFI, PASSWORD_WIFI);
  Serial.printf("Menyambung ke WiFi \"%s\"", NAMA_WIFI);

  unsigned long mulai = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - mulai > 20000) {  // menyerah setelah 20 detik
      Serial.println("\nGagal tersambung. Periksa nama/password WiFi dan pastikan 2,4 GHz.");
      Serial.println("ESP32 restart dalam 5 detik...");
      delay(5000);
      ESP.restart();
    }
    delay(500);
    Serial.print(".");
  }
  Serial.println(" tersambung!");
  WiFi.setSleep(false);  // matikan hemat daya WiFi supaya halaman lebih cepat dijawab
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nBAB 46 - Program 4: Halaman Web dari LittleFS");

  // true = format otomatis kalau partisi belum pernah dipakai
  if (!LittleFS.begin(true)) {
    Serial.println("[LittleFS] Gagal dipasang. Periksa Tools -> Partition Scheme.");
  } else if (!LittleFS.exists("/index.html")) {
    Serial.println("[LittleFS] /index.html belum ada. Unggah folder data dulu (BAB 40).");
  } else {
    Serial.println("[LittleFS] OK: /index.html ditemukan.");
  }

  dht.begin();
  sambungkanWiFi();

  // Urutan penting: route khusus didaftarkan SEBELUM serveStatic
  server.on("/api/sensor", HTTP_GET, kirimSensor);
  server.on("/", HTTP_GET, halamanUtama);
  server.serveStatic("/", LittleFS, "/");  // /style.css, /app.js, dll. dari LittleFS
  server.onNotFound(halaman404);
  server.begin();

  Serial.print("Web server siap. Buka di browser: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
}

void loop() {
  server.handleClient();

  if (timerDHT.sudahWaktunya(millis())) {
    bacaDHT();
  }
}
