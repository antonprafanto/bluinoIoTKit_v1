/*
 * BAB 46 - Program 2: API JSON Sensor
 * - GET /           -> halaman petunjuk singkat
 * - GET /api/sensor -> data sensor dalam format JSON (ArduinoJson 7)
 * DHT11 dibaca setiap 2 detik tanpa delay() (struct Timer dari BAB 16).
 * Potensiometer dibaca langsung setiap kali ada request.
 * Rangkaian: DHT (header hijau) -> IO27, VR10K (header kuning) -> IO34
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <DHT.h>

const char *NAMA_WIFI = "NamaWiFiKamu";          // ← ganti dengan WiFi kamu
const char *PASSWORD_WIFI = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu

#define PIN_DHT 27
#define TIPE_DHT DHT11
#define PIN_POT 34  // potensiometer VR10K (ADC1, aman dipakai bersama WiFi)

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
Timer timerDHT = {2000, 0};  // baca DHT11 setiap 2 detik (jangan kurang dari 2000)

// Hasil bacaan DHT11 terakhir, dipakai oleh handler
float suhu = NAN;
float kelembapan = NAN;
bool dhtOk = false;
bool bacaanPertama = true;

const char HALAMAN_BERANDA[] = R"rawliteral(
<!DOCTYPE html>
<html lang="id">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width, initial-scale=1">
<title>API Sensor ESP32</title>
<style>
  body { font-family: Arial, sans-serif; background: #f2f4f7; color: #222;
         max-width: 480px; margin: 0 auto; padding: 16px; }
  .kartu { background: #fff; border-radius: 12px; padding: 16px;
           box-shadow: 0 1px 4px rgba(0, 0, 0, 0.15); }
  h1 { font-size: 1.4em; color: #1565c0; margin-top: 0; }
  code { background: #e8eaf6; padding: 2px 4px; border-radius: 4px; }
  a { color: #1565c0; }
</style>
</head>
<body>
<div class="kartu">
  <h1>API Sensor ESP32</h1>
  <p>Data sensor tersedia dalam format JSON di <code>/api/sensor</code>.</p>
  <p><a href="/api/sensor">Buka /api/sensor</a></p>
  <p>Tekan F5 di halaman JSON untuk mengambil data terbaru.</p>
</div>
</body>
</html>
)rawliteral";

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

  // Cetak pesan hanya pada bacaan pertama atau saat status berubah
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

// Posisi potensiometer dalam persen (0-100)
int bacaPotPersen() {
  return analogRead(PIN_POT) * 100 / 4095;
}

// Menyusun data sensor menjadi teks JSON
String buatJsonSensor() {
  JsonDocument doc;
  if (dhtOk) {
    doc["suhu"] = suhu;              // °C
    doc["kelembapan"] = kelembapan;  // %RH
  } else {
    doc["suhu"] = nullptr;  // null = belum ada data valid
    doc["kelembapan"] = nullptr;
  }
  doc["dht_ok"] = dhtOk;
  doc["potensiometer"] = bacaPotPersen();  // %
  doc["waktu_nyala"] = millis() / 1000;    // detik sejak ESP32 menyala
  doc["rssi"] = WiFi.RSSI();               // kekuatan sinyal WiFi (dBm)

  String json;
  serializeJson(doc, json);
  return json;
}

// Handler untuk GET /
void halamanBeranda() {
  server.send(200, "text/html", HALAMAN_BERANDA);
  catatRequest(200);
}

// Handler untuk GET /api/sensor
void kirimSensor() {
  server.sendHeader("Cache-Control", "no-store");  // minta browser tidak menyimpan data lama
  server.send(200, "application/json", buatJsonSensor());
  catatRequest(200);
}

void halaman404() {
  server.send(404, "text/plain", "404 - alamat tidak ditemukan. Coba /api/sensor");
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
  Serial.println("\nBAB 46 - Program 2: API JSON Sensor");

  dht.begin();
  sambungkanWiFi();

  server.on("/", HTTP_GET, halamanBeranda);
  server.on("/api/sensor", HTTP_GET, kirimSensor);
  server.onNotFound(halaman404);
  server.begin();

  Serial.print("Web server siap. Buka di browser: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");
  Serial.print("API JSON: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/api/sensor");
}

void loop() {
  server.handleClient();  // tugas 1: layani request

  if (timerDHT.sudahWaktunya(millis())) {  // tugas 2: baca DHT11 setiap 2 detik
    bacaDHT();
  }
}
