/*
 * BAB 50 - Program 1: Publish Data DHT11
 * Setiap 10 detik ESP32 membaca DHT11, lalu mengirim (publish) hasilnya
 * dalam format JSON ke topik bluino/<ID_TOPIK>/sensor di broker publik.
 * Pantau dengan MQTT Explorer / HiveMQ WebSocket Client:
 *   subscribe ke bluino/<ID_TOPIK>/#
 *
 * Rangkaian: DHT (header hijau) -> IO27
 * Library  : PubSubClient (Nick O'Leary), ArduinoJson 7 (Benoit Blanchon),
 *            DHT sensor library + Adafruit Unified Sensor (Adafruit)
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <DHT.h>
#include "esp_mac.h"  // esp_read_mac(): membaca alamat MAC untuk Client ID

const char *NAMA_WIFI = "NamaWiFiKamu";          // ← ganti dengan WiFi kamu
const char *PASSWORD_WIFI = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu

// Broker publik gratis tanpa akun. Cadangan: "broker.emqx.io" atau "test.mosquitto.org"
const char *MQTT_BROKER = "broker.hivemq.com";
const uint16_t MQTT_PORT = 1883;

// Awalan topik unik: nama panggilan + 4 angka acak (huruf kecil, tanpa spasi)
#define ID_TOPIK "andi-4821"  // ← ganti dengan awalan milikmu
const char *TOPIK_SENSOR = "bluino/" ID_TOPIK "/sensor";

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

WiFiClient wifiClient;          // jalur TCP yang dipakai MQTT
PubSubClient mqtt(wifiClient);  // client MQTT
DHT dht(PIN_DHT, TIPE_DHT);

Timer timerKirim = {10000, 0};  // kirim data setiap 10 detik
char idKlien[24];               // Client ID, misalnya "bluino-A1B2C3"

// Variabel sambung ulang MQTT dengan backoff
unsigned long jedaSambung = 0;  // jeda sebelum percobaan berikutnya (0 = boleh langsung)
unsigned long waktuCoba = 0;    // millis() saat percobaan terakhir
bool mqttTadiTersambung = false;
bool wifiTadiTersambung = true;

// Client ID dari 3 byte terakhir alamat MAC, supaya setiap kit berbeda
void buatIdKlien() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  snprintf(idKlien, sizeof(idKlien), "bluino-%02X%02X%02X", mac[3], mac[4], mac[5]);
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
  Serial.print("Alamat IP: ");
  Serial.println(WiFi.localIP());
}

// Satu kali percobaan menyambung ke broker. Hasilnya true kalau berhasil.
bool sambungMqtt() {
  Serial.printf("[MQTT] Menyambung ke %s:%u sebagai %s ... ", MQTT_BROKER, MQTT_PORT, idKlien);
  if (mqtt.connect(idKlien)) {
    Serial.println("berhasil!");
    return true;
  }
  Serial.printf("gagal, state=%d\n", mqtt.state());
  return false;
}

// Dipanggil setiap putaran loop(). Tidak pernah menunggu di dalam perulangan.
void jagaKoneksi() {
  // 1. Laporkan perubahan status WiFi (sambung ulang WiFi dikerjakan driver)
  bool wifiTersambung = (WiFi.status() == WL_CONNECTED);
  if (wifiTersambung != wifiTadiTersambung) {
    wifiTadiTersambung = wifiTersambung;
    if (wifiTersambung) {
      Serial.printf("[WiFi] Tersambung lagi, IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
      Serial.println("[WiFi] Terputus, menunggu sambung ulang otomatis...");
    }
  }

  // 2. Laporkan kalau koneksi MQTT baru saja putus
  bool mqttTersambung = mqtt.connected();
  if (mqttTadiTersambung && !mqttTersambung) {
    Serial.printf("[MQTT] Koneksi terputus, state=%d\n", mqtt.state());
  }
  mqttTadiTersambung = mqttTersambung;
  if (mqttTersambung || !wifiTersambung) {
    return;  // sudah tersambung, atau WiFi belum siap
  }

  // 3. Belum waktunya mencoba lagi? Langsung keluar.
  unsigned long sekarang = millis();
  if (sekarang - waktuCoba < jedaSambung) {
    return;
  }
  waktuCoba = sekarang;

  if (sambungMqtt()) {
    mqttTadiTersambung = true;
    jedaSambung = 0;  // berhasil: kalau nanti putus, langsung coba lagi
  } else {
    // gagal: jeda 2, 4, 8, 16, 32, lalu paling lama 60 detik
    jedaSambung = (jedaSambung == 0) ? 2000 : min(jedaSambung * 2, 60000UL);
    Serial.printf("[MQTT] Coba lagi dalam %lu detik\n", jedaSambung / 1000);
  }
}

void kirimDataSensor() {
  float kelembapan = dht.readHumidity();
  float suhu = dht.readTemperature();
  bool dhtOk = !isnan(kelembapan) && !isnan(suhu);

  JsonDocument doc;
  if (dhtOk) {
    doc["suhu"] = suhu;              // °C
    doc["kelembapan"] = kelembapan;  // %RH
  } else {
    doc["suhu"] = nullptr;  // null = tidak ada data valid
    doc["kelembapan"] = nullptr;
    Serial.println("[DHT11] Gagal membaca. Periksa kabel DHT -> IO27.");
  }
  doc["dht_ok"] = dhtOk;
  doc["rssi"] = WiFi.RSSI();             // kekuatan sinyal WiFi (dBm)
  doc["waktu_nyala"] = millis() / 1000;  // detik sejak ESP32 menyala

  String payload;
  serializeJson(doc, payload);

  if (mqtt.publish(TOPIK_SENSOR, payload.c_str())) {
    Serial.printf("[PUB] %s -> %s\n", TOPIK_SENSOR, payload.c_str());
  } else {
    Serial.println("[PUB] Gagal! Koneksi putus atau pesan lebih besar dari buffer.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nBAB 50 - Program 1: Publish Data DHT11");

  dht.begin();
  buatIdKlien();
  sambungkanWiFi();

  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setSocketTimeout(5);  // tunggu jawaban broker paling lama 5 detik
  Serial.printf("Client ID: %s\n", idKlien);
  Serial.printf("Topik    : %s\n", TOPIK_SENSOR);
}

void loop() {
  jagaKoneksi();  // tugas 1: jaga koneksi WiFi dan MQTT
  mqtt.loop();    // tugas 2: keep-alive dan pesan masuk

  // tugas 3: kirim data setiap 10 detik, hanya kalau sedang tersambung
  if (timerKirim.sudahWaktunya(millis()) && mqtt.connected()) {
    kirimDataSensor();
  }
}
