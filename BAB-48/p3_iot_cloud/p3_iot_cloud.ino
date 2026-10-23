/*
 * BAB 48 - Program 3: Data DHT11 ke Cloud ThingSpeak
 * Setiap 20 detik ESP32 mengirim suhu, kelembapan, dan kekuatan sinyal WiFi
 * ke channel ThingSpeak dengan HTTP POST. Grafiknya bisa dilihat di browser.
 * Kalau pengiriman gagal, jeda berikutnya dilipatgandakan (maks. 5 menit).
 * LED2 (hijau) berkedip saat berhasil, LED1 (merah) menyala selama gagal.
 * Rangkaian: DHT -> IO27, LED1 (merah) -> IO4, LED2 (hijau) -> IO17
 * Library  : DHT sensor library + Adafruit Unified Sensor (Adafruit)
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

const char *NAMA_WIFI = "NamaWiFiKamu";          // ← ganti dengan WiFi kamu
const char *PASSWORD_WIFI = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu

// Write API Key channel ThingSpeak-mu (tab "API Keys"). Rahasiakan!
const char *WRITE_API_KEY = "KUNCI_API_KAMU";  // ← ganti dengan Write API Key kamu
const char *URL_THINGSPEAK = "http://api.thingspeak.com/update";

#define PIN_DHT 27
#define TIPE_DHT DHT11
#define LED_MERAH 4   // LED1 (merah): menyala selama pengiriman terakhir gagal
#define LED_HIJAU 17  // LED2 (hijau): berkedip sebentar setiap kali berhasil

// Akun ThingSpeak gratis: paling cepat 1 data setiap 15 detik per channel.
const unsigned long INTERVAL_NORMAL = 20000;  // 20 detik (ada sisa aman)
const unsigned long INTERVAL_MAKS = 300000;   // jeda terlama setelah gagal: 5 menit

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
Timer timerDHT = {2000, 0};               // baca DHT11 setiap 2 detik
Timer timerKirim = {INTERVAL_NORMAL, 0};  // jadwal kirim (intervalnya bisa berubah)
Timer timerKedip = {200, 0};              // lama LED hijau menyala (ms)

float suhu = NAN;
float kelembapan = NAN;
bool dhtOk = false;
bool bacaanPertama = true;
unsigned long nomorKirim = 0;  // urutan pengiriman dari sisi ESP32
bool ledHijauNyala = false;

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

// Mengirim satu data ke ThingSpeak. Hasilnya true kalau data disimpan.
bool kirimKeThingSpeak() {
  int rssi = WiFi.RSSI();  // kekuatan sinyal WiFi (dBm)

  // Body berformat formulir: nama=nilai, dipisah tanda &
  char body[128];
  snprintf(body, sizeof(body), "api_key=%s&field1=%.1f&field2=%.0f&field3=%d",
           WRITE_API_KEY, suhu, kelembapan, rssi);

  Serial.println();
  // Kunci API sengaja tidak ikut dicetak
  Serial.printf("[KIRIM #%lu] suhu=%.1f kelembapan=%.0f rssi=%d\n",
                nomorKirim, suhu, kelembapan, rssi);
  unsigned long mulai = millis();

  HTTPClient http;
  http.begin(URL_THINGSPEAK);
  http.setConnectTimeout(5000);
  http.setTimeout(5000);
  http.setUserAgent("BluinoKit-BAB48/1.0");
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int kode = http.POST(body);
  if (kode < 0) {
    Serial.printf("[HTTP] Gagal: %d (%s)\n", kode, http.errorToString(kode).c_str());
    http.end();
    return false;
  }
  String jawaban = http.getString();
  http.end();
  jawaban.trim();
  Serial.printf("[HTTP] Kode status %d, jawaban \"%s\", %lu ms\n",
                kode, jawaban.substring(0, 40).c_str(), millis() - mulai);

  // ThingSpeak menjawab dengan nomor entri (1, 2, 3, ...) kalau data
  // disimpan, atau "0" kalau ditolak -- kadang tetap dengan kode 200!
  long nomorEntri = jawaban.toInt();
  if (kode == HTTP_CODE_OK && nomorEntri > 0) {
    Serial.printf("[KIRIM #%lu] Berhasil, tersimpan sebagai entri ke-%ld.\n", nomorKirim, nomorEntri);
    return true;
  }
  if (kode == HTTP_CODE_OK) {
    Serial.printf("[KIRIM #%lu] Ditolak (jawaban 0): jeda kurang dari 15 detik atau kuota habis.\n", nomorKirim);
  } else if (kode == HTTP_CODE_BAD_REQUEST || kode == HTTP_CODE_UNAUTHORIZED) {
    Serial.printf("[KIRIM #%lu] Ditolak: periksa Write API Key.\n", nomorKirim);
  } else {
    Serial.printf("[KIRIM #%lu] Server bermasalah, coba lagi nanti.\n", nomorKirim);
  }
  return false;
}

// Menentukan jeda sampai pengiriman berikutnya (backoff saat gagal)
void aturJadwal(bool berhasil) {
  if (berhasil) {
    timerKirim.interval = INTERVAL_NORMAL;
  } else {
    timerKirim.interval = timerKirim.interval * 2;  // 40, 80, 160 detik, ...
    if (timerKirim.interval > INTERVAL_MAKS) {
      timerKirim.interval = INTERVAL_MAKS;
    }
  }
  Serial.printf("[JADWAL] Kirim berikutnya %lu detik lagi.\n", timerKirim.interval / 1000);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);
  digitalWrite(LED_MERAH, LOW);
  digitalWrite(LED_HIJAU, LOW);
  delay(1000);
  Serial.println("\nBAB 48 - Program 3: Data DHT11 ke Cloud ThingSpeak");

  if (strcmp(WRITE_API_KEY, "KUNCI_API_KAMU") == 0) {
    Serial.println("PERHATIAN: WRITE_API_KEY belum diganti! Lihat README bagian 48.9.1.");
  }

  dht.begin();
  sambungkanWiFi();
  Serial.printf("Data dikirim setiap %lu detik ke %s\n", INTERVAL_NORMAL / 1000, URL_THINGSPEAK);
}

void loop() {
  unsigned long sekarang = millis();

  // Tugas 1: baca DHT11 setiap 2 detik
  if (timerDHT.sudahWaktunya(sekarang)) {
    bacaDHT();
  }

  // Tugas 2: kirim ke ThingSpeak sesuai jadwal
  if (timerKirim.sudahWaktunya(sekarang)) {
    if (!dhtOk) {
      Serial.println("[KIRIM] Dilewati: belum ada bacaan DHT11 yang valid.");
    } else if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[KIRIM] Dilewati: WiFi terputus, menunggu tersambung ulang.");
    } else {
      nomorKirim = nomorKirim + 1;
      bool berhasil = kirimKeThingSpeak();
      digitalWrite(LED_MERAH, berhasil ? LOW : HIGH);
      if (berhasil) {
        digitalWrite(LED_HIJAU, HIGH);  // mulai kedip
        ledHijauNyala = true;
        timerKedip.waktuLalu = millis();
      }
      aturJadwal(berhasil);
    }
  }

  // Tugas 3: matikan LED hijau 200 ms setelah menyala
  if (ledHijauNyala && timerKedip.sudahWaktunya(millis())) {
    digitalWrite(LED_HIJAU, LOW);
    ledHijauNyala = false;
  }
}
