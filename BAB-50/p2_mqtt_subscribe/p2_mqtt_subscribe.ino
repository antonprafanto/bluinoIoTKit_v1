/*
 * BAB 50 - Program 2: Subscribe - Kendalikan LED dan Relay
 * ESP32 berlangganan bluino/<ID_TOPIK>/perintah/+ lalu menjalankan perintah:
 *   .../perintah/led   -> LED merah (LED1, IO4)
 *   .../perintah/relay -> relay (IO26)
 *   isi pesan: ON, OFF, atau TOGGLE (huruf besar/kecil sama saja)
 * Keadaan terbaru dilaporkan sebagai retained message ke
 *   .../keadaan/led dan .../keadaan/relay
 *
 * Rangkaian: LED1 (merah) -> IO4, RELAY -> IO26
 * Library  : PubSubClient (Nick O'Leary)
 * Relay dicoba tanpa beban. Jangan pernah menyambungkan listrik PLN 220 V!
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include "esp_mac.h"  // esp_read_mac(): membaca alamat MAC untuk Client ID

const char *NAMA_WIFI = "NamaWiFiKamu";          // ← ganti dengan WiFi kamu
const char *PASSWORD_WIFI = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu

const char *MQTT_BROKER = "broker.hivemq.com";  // cadangan: "broker.emqx.io"
const uint16_t MQTT_PORT = 1883;

#define ID_TOPIK "andi-4821"  // ← ganti dengan awalan milikmu
const char *TOPIK_PERINTAH = "bluino/" ID_TOPIK "/perintah/+";  // + = led atau relay
const char *TOPIK_PERINTAH_LED = "bluino/" ID_TOPIK "/perintah/led";
const char *TOPIK_PERINTAH_RELAY = "bluino/" ID_TOPIK "/perintah/relay";
const char *TOPIK_KEADAAN_LED = "bluino/" ID_TOPIK "/keadaan/led";
const char *TOPIK_KEADAAN_RELAY = "bluino/" ID_TOPIK "/keadaan/relay";

#define PIN_LED_MERAH 4  // LED1 (merah), aktif HIGH
#define PIN_RELAY 26     // relay, aktif HIGH

WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

char idKlien[24];
bool ledNyala = false;
bool relayNyala = false;
bool perluLapor = false;  // true = keadaan perlu dilaporkan dari loop()

// Variabel sambung ulang MQTT dengan backoff
unsigned long jedaSambung = 0;
unsigned long waktuCoba = 0;
bool mqttTadiTersambung = false;
bool wifiTadiTersambung = true;

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

// Mengubah teks perintah menjadi keadaan baru: 1 = nyala, 0 = mati, -1 = tidak dikenal
int bacaPerintah(const char *isi, bool keadaanSekarang) {
  if (strcasecmp(isi, "ON") == 0) return 1;  // strcasecmp: huruf besar/kecil dianggap sama
  if (strcasecmp(isi, "OFF") == 0) return 0;
  if (strcasecmp(isi, "TOGGLE") == 0) return keadaanSekarang ? 0 : 1;
  return -1;
}

// Callback: dipanggil oleh mqtt.loop() setiap ada pesan di topik langganan.
// payload TIDAK diakhiri '\0', jadi disalin dulu menjadi teks biasa.
void pesanMasuk(char *topik, byte *payload, unsigned int panjang) {
  char isi[16];
  if (panjang >= sizeof(isi)) {
    Serial.printf("[SUB] %s -> (pesan %u byte, terlalu panjang, diabaikan)\n", topik, panjang);
    return;
  }
  memcpy(isi, payload, panjang);
  isi[panjang] = '\0';
  Serial.printf("[SUB] %s -> %s\n", topik, isi);

  if (panjang == 0) {
    return;  // pesan kosong (misalnya penghapus retained message): abaikan
  }

  bool untukLed = (strcmp(topik, TOPIK_PERINTAH_LED) == 0);
  bool untukRelay = (strcmp(topik, TOPIK_PERINTAH_RELAY) == 0);
  if (!untukLed && !untukRelay) {
    Serial.println("      Topik tidak dikenal, diabaikan.");
    return;
  }

  int baru = bacaPerintah(isi, untukLed ? ledNyala : relayNyala);
  if (baru < 0) {
    Serial.println("      Perintah tidak dikenal. Pakai ON, OFF, atau TOGGLE.");
    return;
  }

  if (untukLed) {
    ledNyala = (baru == 1);
    digitalWrite(PIN_LED_MERAH, ledNyala ? HIGH : LOW);
    Serial.printf("      LED merah -> %s\n", ledNyala ? "ON" : "OFF");
  } else {
    relayNyala = (baru == 1);
    digitalWrite(PIN_RELAY, relayNyala ? HIGH : LOW);
    Serial.printf("      Relay -> %s\n", relayNyala ? "ON" : "OFF");
  }
  perluLapor = true;  // laporan dikirim dari loop(), bukan dari dalam callback
}

// Melaporkan keadaan LED dan relay sebagai retained message
void laporKeadaan() {
  perluLapor = false;
  bool berhasil = mqtt.publish(TOPIK_KEADAAN_LED, ledNyala ? "ON" : "OFF", true) &&
                  mqtt.publish(TOPIK_KEADAAN_RELAY, relayNyala ? "ON" : "OFF", true);
  if (berhasil) {
    Serial.printf("[PUB] Keadaan: led=%s, relay=%s (retained)\n",
                  ledNyala ? "ON" : "OFF", relayNyala ? "ON" : "OFF");
  } else {
    Serial.println("[PUB] Gagal mengirim keadaan.");
  }
}

// Satu kali percobaan menyambung, lalu berlangganan ulang
bool sambungMqtt() {
  Serial.printf("[MQTT] Menyambung ke %s:%u sebagai %s ... ", MQTT_BROKER, MQTT_PORT, idKlien);
  if (!mqtt.connect(idKlien)) {
    Serial.printf("gagal, state=%d\n", mqtt.state());
    return false;
  }
  Serial.println("berhasil!");

  // Clean session: langganan hilang setiap koneksi putus, jadi subscribe lagi di sini
  bool ok = mqtt.subscribe(TOPIK_PERINTAH, 1);  // QoS 1
  Serial.printf("[MQTT] Subscribe %s (QoS 1): %s\n", TOPIK_PERINTAH, ok ? "OK" : "gagal");
  perluLapor = true;  // laporkan keadaan terbaru setelah tersambung
  return true;
}

// Dipanggil setiap putaran loop(). Tidak pernah menunggu di dalam perulangan.
void jagaKoneksi() {
  bool wifiTersambung = (WiFi.status() == WL_CONNECTED);
  if (wifiTersambung != wifiTadiTersambung) {
    wifiTadiTersambung = wifiTersambung;
    if (wifiTersambung) {
      Serial.printf("[WiFi] Tersambung lagi, IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
      Serial.println("[WiFi] Terputus, menunggu sambung ulang otomatis...");
    }
  }

  bool mqttTersambung = mqtt.connected();
  if (mqttTadiTersambung && !mqttTersambung) {
    Serial.printf("[MQTT] Koneksi terputus, state=%d\n", mqtt.state());
  }
  mqttTadiTersambung = mqttTersambung;
  if (mqttTersambung || !wifiTersambung) {
    return;
  }

  unsigned long sekarang = millis();
  if (sekarang - waktuCoba < jedaSambung) {
    return;
  }
  waktuCoba = sekarang;

  if (sambungMqtt()) {
    mqttTadiTersambung = true;
    jedaSambung = 0;
  } else {
    jedaSambung = (jedaSambung == 0) ? 2000 : min(jedaSambung * 2, 60000UL);
    Serial.printf("[MQTT] Coba lagi dalam %lu detik\n", jedaSambung / 1000);
  }
}

void setup() {
  Serial.begin(115200);
  // pinMode dulu, baru digitalWrite (di board package 3.x urutan ini wajib)
  pinMode(PIN_LED_MERAH, OUTPUT);
  digitalWrite(PIN_LED_MERAH, LOW);
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);  // relay mati saat ESP32 baru menyala
  delay(1000);
  Serial.println("\nBAB 50 - Program 2: Subscribe - Kendalikan LED dan Relay");

  buatIdKlien();
  sambungkanWiFi();

  mqtt.setServer(MQTT_BROKER, MQTT_PORT);
  mqtt.setCallback(pesanMasuk);  // fungsi yang dipanggil saat pesan masuk
  mqtt.setSocketTimeout(5);
  Serial.printf("Client ID: %s\n", idKlien);
  Serial.printf("Perintah LED  : %s\n", TOPIK_PERINTAH_LED);
  Serial.printf("Perintah relay: %s\n", TOPIK_PERINTAH_RELAY);
}

void loop() {
  jagaKoneksi();  // tugas 1: jaga koneksi WiFi dan MQTT
  mqtt.loop();    // tugas 2: terima pesan -> pesanMasuk() dipanggil dari sini

  if (perluLapor && mqtt.connected()) {  // tugas 3: laporkan keadaan kalau berubah
    laporKeadaan();
  }
}
