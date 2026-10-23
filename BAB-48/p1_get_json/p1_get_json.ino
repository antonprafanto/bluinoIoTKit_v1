/*
 * BAB 48 - Program 1: Cuaca Terkini dari Open-Meteo (HTTP GET + JSON)
 * ESP32 meminta data cuaca ke API Open-Meteo (gratis, tanpa kunci API)
 * saat menyala lalu setiap 15 menit, dan menampilkannya di Serial Monitor.
 * Ketik sebuah huruf + Enter di Serial Monitor (line ending: New Line)
 * untuk mengambil data sekarang.
 * Rangkaian: tidak perlu.
 * Library  : ArduinoJson by Benoit Blanchon (versi 7)
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char *NAMA_WIFI = "NamaWiFiKamu";          // ← ganti dengan WiFi kamu
const char *PASSWORD_WIFI = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu

// Lokasi yang ingin dilihat cuacanya (koordinat kota lain ada di README).
// Desimal memakai titik, bukan koma.
const char *NAMA_KOTA = "Jakarta";
const char *LINTANG = "-6.2088";  // latitude (minus = selatan khatulistiwa)
const char *BUJUR = "106.8456";   // longitude

// Data "current" Open-Meteo diperbarui setiap 15 menit, jadi tidak ada
// gunanya meminta lebih sering dari itu.
const unsigned long INTERVAL_CUACA = 15UL * 60UL * 1000UL;  // 15 menit (ms)

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

Timer timerCuaca = {INTERVAL_CUACA, 0};
char urlCuaca[256];  // alamat lengkap API, disusun di setup()

// Menerjemahkan kode cuaca WMO dari Open-Meteo ke bahasa Indonesia
const char *artiKodeCuaca(int kode) {
  switch (kode) {
    case 0: return "Cerah";
    case 1: return "Cerah berawan";
    case 2: return "Berawan sebagian";
    case 3: return "Mendung";
    case 45:
    case 48: return "Berkabut";
    case 51:
    case 53:
    case 55: return "Gerimis";
    case 61: return "Hujan ringan";
    case 63: return "Hujan sedang";
    case 65: return "Hujan lebat";
    case 80: return "Hujan lokal ringan";
    case 81: return "Hujan lokal sedang";
    case 82: return "Hujan lokal sangat lebat";
    case 95: return "Badai petir";
    case 96:
    case 99: return "Badai petir disertai hujan es";
    default: return "Lainnya (lihat tabel kode WMO)";
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

void ambilCuaca() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[CUACA] WiFi terputus, permintaan dilewati.");
    return;
  }

  Serial.println();
  Serial.printf("[HTTP] GET %s\n", urlCuaca);
  unsigned long mulai = millis();

  // Langkah 1: siapkan alamat tujuan dan pengaturan request
  HTTPClient http;
  http.begin(urlCuaca);
  http.setConnectTimeout(5000);              // batas waktu menyambung (ms)
  http.setTimeout(5000);                     // batas waktu menunggu data (ms)
  http.setUserAgent("BluinoKit-BAB48/1.0");  // "nama" client kita

  // Langkah 2: kirim GET dan tunggu jawaban
  int kode = http.GET();
  if (kode < 0) {  // negatif = gagal sebelum server menjawab
    Serial.printf("[HTTP] Gagal: %d (%s)\n", kode, http.errorToString(kode).c_str());
    http.end();
    return;
  }

  // Langkah 3 dan 4: ambil isi jawaban, lalu tutup sambungan
  String isi = http.getString();
  http.end();
  Serial.printf("[HTTP] Kode status %d, %u byte, %lu ms\n",
                kode, isi.length(), millis() - mulai);

  if (kode != HTTP_CODE_OK) {  // HTTP_CODE_OK = 200
    Serial.print("[HTTP] Permintaan ditolak. Isi jawaban: ");
    Serial.println(isi);
    return;
  }
  Serial.println(isi);  // JSON mentah: bandingkan dengan tampilan di browser

  // Langkah 5: baca JSON dengan ArduinoJson 7
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, isi);
  if (err) {
    Serial.printf("[JSON] Gagal dibaca: %s\n", err.c_str());
    return;
  }

  JsonObject kini = doc["current"];
  if (!kini["temperature_2m"].is<float>() || !kini["weather_code"].is<int>()) {
    Serial.println("[JSON] Bagian \"current\" tidak lengkap.");
    return;
  }

  float suhu = kini["temperature_2m"];
  float terasa = kini["apparent_temperature"];
  int kelembapan = kini["relative_humidity_2m"];
  float angin = kini["wind_speed_10m"];
  int kodeCuaca = kini["weather_code"];
  const char *waktu = kini["time"] | "-";  // contoh: "2026-10-24T05:30"
  const char *jam = strchr(waktu, 'T');      // ambil bagian setelah huruf T
  jam = (jam != nullptr) ? jam + 1 : waktu;

  Serial.printf("=== Cuaca %s (data pukul %s) ===\n", NAMA_KOTA, jam);
  Serial.printf("Kondisi    : %s (kode %d)\n", artiKodeCuaca(kodeCuaca), kodeCuaca);
  Serial.printf("Suhu       : %.1f °C (terasa %.1f °C)\n", suhu, terasa);
  Serial.printf("Kelembapan : %d %%\n", kelembapan);
  Serial.printf("Angin      : %.1f km/jam\n", angin);
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nBAB 48 - Program 1: Cuaca Terkini dari Open-Meteo");

  // Susun alamat API: koordinat + daftar data "current" yang diminta.
  // timezone=auto: jam di jawaban mengikuti zona waktu lokasi (WIB/WITA/WIT).
  snprintf(urlCuaca, sizeof(urlCuaca),
           "http://api.open-meteo.com/v1/forecast?latitude=%s&longitude=%s"
           "&current=temperature_2m,relative_humidity_2m,apparent_temperature,"
           "weather_code,wind_speed_10m&timezone=auto",
           LINTANG, BUJUR);

  sambungkanWiFi();
  Serial.println("Ketik sebuah huruf lalu tekan Enter di Serial Monitor untuk mengambil data sekarang.");

  ambilCuaca();                     // data pertama langsung diambil
  timerCuaca.waktuLalu = millis();  // berikutnya 15 menit lagi
}

void loop() {
  // Tugas 1: ambil cuaca setiap 15 menit
  if (timerCuaca.sudahWaktunya(millis())) {
    ambilCuaca();
  }

  // Tugas 2: ambil sekarang kalau ada Enter dari Serial Monitor
  bool adaEnter = false;
  while (Serial.available() > 0) {
    if (Serial.read() == '\n') {
      adaEnter = true;
    }
  }
  if (adaEnter) {
    ambilCuaca();
    timerCuaca.waktuLalu = millis();  // hitung ulang 15 menit dari sekarang
  }
}
