/*
 * BAB 48 - Program 2: Mengirim Data DHT11 dengan HTTP POST
 * DHT11 dibaca setiap 2 detik. Setiap 30 detik ESP32 mengirim suhu dan
 * kelembapan dalam format JSON ke http://httpbin.org/post. Server itu
 * memantulkan kembali data yang diterimanya, jadi kita bisa mengeceknya.
 * Rangkaian: DHT (header hijau) -> IO27
 * Library  : DHT sensor library + Adafruit Unified Sensor (Adafruit),
 *            ArduinoJson by Benoit Blanchon (versi 7)
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DHT.h>

const char *NAMA_WIFI = "NamaWiFiKamu";          // ← ganti dengan WiFi kamu
const char *PASSWORD_WIFI = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu

// Alamat tujuan. Untuk melihat data di browser, ganti dengan alamat
// webhook.site milikmu (diawali http://, lihat README bagian 48.7.1).
const char *URL_TUJUAN = "http://httpbin.org/post";
const char *NAMA_PERANGKAT = "bluino-01";  // nama kit-mu di dalam data

#define PIN_DHT 27
#define TIPE_DHT DHT11

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

DHT dht(PIN_DHT, TIPE_DHT);
Timer timerDHT = {2000, 0};     // baca DHT11 setiap 2 detik (jangan kurang dari 2000)
Timer timerKirim = {30000, 0};  // kirim data setiap 30 detik

// Hasil bacaan DHT11 terakhir
float suhu = NAN;
float kelembapan = NAN;
bool dhtOk = false;
bool bacaanPertama = true;
unsigned long nomorKirim = 0;  // sudah berapa kali mencoba mengirim

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
  Serial.print(" tersambung! IP: ");
  Serial.println(WiFi.localIP());
}

void kirimData() {
  if (!dhtOk) {
    Serial.println("[KIRIM] Dilewati: belum ada bacaan DHT11 yang valid.");
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[KIRIM] Dilewati: WiFi terputus.");
    return;
  }
  nomorKirim = nomorKirim + 1;

  // 1. Susun data JSON
  JsonDocument data;
  data["perangkat"] = NAMA_PERANGKAT;
  data["suhu"] = suhu;                    // °C
  data["kelembapan"] = kelembapan;        // %RH
  data["nomor"] = nomorKirim;             // urutan pengiriman
  data["waktu_nyala"] = millis() / 1000;  // detik sejak ESP32 menyala
  String body;
  serializeJson(data, body);

  Serial.println();
  Serial.printf("[HTTP] POST %s\n", URL_TUJUAN);
  Serial.printf("[HTTP] Body: %s\n", body.c_str());
  unsigned long mulai = millis();

  // 2. Kirim dengan POST
  HTTPClient http;
  http.begin(URL_TUJUAN);
  http.setConnectTimeout(5000);
  http.setTimeout(5000);
  http.setUserAgent("BluinoKit-BAB48/1.0");
  http.addHeader("Content-Type", "application/json");  // beri tahu server: isinya JSON
  int kode = http.POST(body);
  if (kode < 0) {
    Serial.printf("[HTTP] Gagal: %d (%s)\n", kode, http.errorToString(kode).c_str());
    http.end();
    return;
  }
  String jawaban = http.getString();
  http.end();
  Serial.printf("[HTTP] Kode status %d, %lu ms\n", kode, millis() - mulai);

  if (kode < 200 || kode > 299) {  // 2xx = berhasil
    Serial.print("[HTTP] Ditolak server. Isi jawaban: ");
    Serial.println(jawaban.substring(0, 200));
    return;
  }

  // 3. httpbin.org memantulkan JSON kita di bagian "json".
  //    Filter: simpan bagian itu saja, abaikan sisanya.
  JsonDocument filter;
  filter["json"] = true;
  JsonDocument pantulan;
  DeserializationError err = deserializeJson(pantulan, jawaban,
                                             DeserializationOption::Filter(filter));
  if (!err && !pantulan["json"].isNull()) {
    Serial.print("[HTTP] Diterima server: ");
    serializeJson(pantulan["json"], Serial);
    Serial.println();
  } else {
    // Server lain (misalnya webhook.site) tidak memantulkan JSON
    Serial.print("[HTTP] Jawaban server: ");
    Serial.println(jawaban.substring(0, 120));
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nBAB 48 - Program 2: Mengirim Data DHT11 dengan HTTP POST");

  dht.begin();
  sambungkanWiFi();
  Serial.printf("Data dikirim setiap %lu detik ke %s\n", timerKirim.interval / 1000, URL_TUJUAN);
}

void loop() {
  unsigned long sekarang = millis();

  if (timerDHT.sudahWaktunya(sekarang)) {  // tugas 1: baca DHT11 setiap 2 detik
    bacaDHT();
  }
  if (timerKirim.sudahWaktunya(sekarang)) {  // tugas 2: kirim setiap 30 detik
    kirimData();
  }
}
