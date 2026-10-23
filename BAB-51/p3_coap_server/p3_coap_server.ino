/*
 * BAB 51 - Program 3: ESP32 sebagai CoAP Server
 * Resource yang disediakan:
 *   GET coap://IP/sensor            -> JSON suhu & kelembapan DHT11
 *   GET coap://IP/led               -> status LED merah: 1 (menyala) / 0 (padam)
 *   PUT coap://IP/led  (isi 1 / 0)  -> menyalakan / memadamkan LED merah
 *   GET coap://IP/.well-known/core  -> daftar resource (CoRE Link Format)
 * DHT11 dibaca setiap 2 detik tanpa delay() (struct Timer dari BAB 16).
 * Rangkaian: DHT (header hijau) -> IO27, LED1 (merah) -> IO4
 * Uji dari laptop di WiFi yang sama: aiocoap-client coap://IP/sensor
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include <ArduinoJson.h>
#include <DHT.h>

const char *NAMA_WIFI = "NamaWiFiKamu";          // ← ganti dengan WiFi kamu
const char *PASSWORD_WIFI = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu

#define PIN_DHT 27
#define TIPE_DHT DHT11
#define LED_MERAH 4  // LED1 (merah)

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

WiFiUDP udp;
Coap coap(udp, 256);
DHT dht(PIN_DHT, TIPE_DHT);
Timer timerDHT = {2000, 0};  // baca DHT11 setiap 2 detik

// Hasil bacaan DHT11 terakhir
float suhu = NAN;
float kelembapan = NAN;
bool dhtOk = false;
bool bacaanPertama = true;

bool ledMenyala = false;
unsigned long jumlahRequest = 0;
bool wifiTersambung = true;

// Daftar resource untuk /.well-known/core (format CoRE Link Format, RFC 6690)
const char DAFTAR_RESOURCE[] =
  "</sensor>;title=\"Suhu dan kelembapan\";ct=50,"
  "</led>;title=\"LED merah (PUT 1/0)\";ct=0";

const char *namaJenis(uint8_t jenis) {
  switch (jenis) {
    case COAP_CON: return "CON";
    case COAP_NONCON: return "NON";
    case COAP_ACK: return "ACK";
    case COAP_RESET: return "RST";
    default: return "?";
  }
}

const char *namaMethod(uint8_t kode) {
  switch (kode) {
    case COAP_GET: return "GET";
    case COAP_POST: return "POST";
    case COAP_PUT: return "PUT";
    case COAP_DELETE: return "DELETE";
    default: return "?";
  }
}

// Mengirim jawaban (ACK) dengan Message ID dan token yang sama dengan request
void balas(CoapPacket &paket, IPAddress ip, int port, const char *isi,
           COAP_RESPONSE_CODE kode, COAP_CONTENT_TYPE format) {
  coap.sendResponse(ip, port, paket.messageid, isi, strlen(isi),
                    kode, format, paket.token, paket.tokenlen);
}

// Mencatat request yang baru saja dijawab ke Serial Monitor
void catat(CoapPacket &paket, IPAddress ip, int port, const char *path, uint8_t kodeJawaban) {
  jumlahRequest++;
  Serial.printf("[CoAP #%lu] %s %s %s dari %s:%d, token %u byte -> %d.%02d\n",
                jumlahRequest, namaJenis(paket.type), namaMethod(paket.code), path,
                ip.toString().c_str(), port, paket.tokenlen,
                kodeJawaban >> 5, kodeJawaban & 0x1F);
}

// Resource /sensor
void tanganiSensor(CoapPacket &paket, IPAddress ip, int port) {
  if (paket.code != COAP_GET) {
    balas(paket, ip, port, "pakai GET", COAP_METHOD_NOT_ALLOWED, COAP_TEXT_PLAIN);
    catat(paket, ip, port, "/sensor", COAP_METHOD_NOT_ALLOWED);
    return;
  }
  if (!dhtOk) {
    balas(paket, ip, port, "DHT11 belum terbaca", COAP_SERVICE_UNAVAILABLE, COAP_TEXT_PLAIN);
    catat(paket, ip, port, "/sensor", COAP_SERVICE_UNAVAILABLE);
    return;
  }

  JsonDocument doc;
  doc["suhu"] = suhu;                   // °C
  doc["kelembapan"] = kelembapan;       // %RH
  doc["waktu_nyala"] = millis() / 1000;  // detik sejak ESP32 menyala
  char json[96];
  serializeJson(doc, json, sizeof(json));

  balas(paket, ip, port, json, COAP_CONTENT, COAP_APPLICATION_JSON);
  catat(paket, ip, port, "/sensor", COAP_CONTENT);
}

// Resource /led
void tanganiLed(CoapPacket &paket, IPAddress ip, int port) {
  if (paket.code == COAP_GET) {
    balas(paket, ip, port, ledMenyala ? "1" : "0", COAP_CONTENT, COAP_TEXT_PLAIN);
    catat(paket, ip, port, "/led", COAP_CONTENT);

  } else if (paket.code == COAP_PUT) {
    // Salin isi pesan ke string C (payload TIDAK diakhiri '\0')
    char perintah[8] = "";
    size_t panjang = min(paket.payloadlen, sizeof(perintah) - 1);
    if (panjang > 0) memcpy(perintah, paket.payload, panjang);
    perintah[panjang] = '\0';

    if (strcmp(perintah, "1") == 0 || strcmp(perintah, "0") == 0) {
      ledMenyala = (perintah[0] == '1');
      digitalWrite(LED_MERAH, ledMenyala ? HIGH : LOW);
      balas(paket, ip, port, perintah, COAP_CHANGED, COAP_TEXT_PLAIN);
      catat(paket, ip, port, "/led", COAP_CHANGED);
      Serial.printf("          LED merah %s\n", ledMenyala ? "MENYALA" : "PADAM");
    } else {
      balas(paket, ip, port, "isi harus 1 atau 0", COAP_BAD_REQUEST, COAP_TEXT_PLAIN);
      catat(paket, ip, port, "/led", COAP_BAD_REQUEST);
    }

  } else {
    balas(paket, ip, port, "pakai GET atau PUT", COAP_METHOD_NOT_ALLOWED, COAP_TEXT_PLAIN);
    catat(paket, ip, port, "/led", COAP_METHOD_NOT_ALLOWED);
  }
}

// Resource /.well-known/core
void tanganiDaftar(CoapPacket &paket, IPAddress ip, int port) {
  balas(paket, ip, port, DAFTAR_RESOURCE, COAP_CONTENT, COAP_APPLICATION_LINK_FORMAT);
  catat(paket, ip, port, "/.well-known/core", COAP_CONTENT);
}

// Callback jawaban: ESP32 ini tidak mengirim request, tetapi callback ini
// WAJIB didaftarkan supaya program tidak crash kalau ada paket ACK masuk.
void abaikanJawaban(CoapPacket &paket, IPAddress ip, int port) {
  Serial.printf("[INFO] Paket ACK dari %s diabaikan.\n", ip.toString().c_str());
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

// Memberi tahu kalau WiFi terputus atau tersambung lagi
void pantauWiFi() {
  bool tersambung = (WiFi.status() == WL_CONNECTED);
  if (tersambung == wifiTersambung) return;
  wifiTersambung = tersambung;
  if (tersambung) {
    Serial.print("[WiFi] Tersambung lagi. Alamat IP ESP32: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[WiFi] Terputus. ESP32 mencoba menyambung ulang otomatis...");
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
  Serial.println(" tersambung!");
  WiFi.setSleep(false);  // matikan hemat daya WiFi supaya request lebih cepat dijawab
  Serial.print("Alamat IP ESP32: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);
  digitalWrite(LED_MERAH, LOW);
  delay(1000);
  Serial.println("\nBAB 51 - Program 3: ESP32 sebagai CoAP Server");

  dht.begin();
  sambungkanWiFi();

  // Daftarkan semua callback SEBELUM coap.start()
  coap.server(tanganiSensor, "sensor");
  coap.server(tanganiLed, "led");
  coap.server(tanganiDaftar, ".well-known/core");
  coap.response(abaikanJawaban);
  coap.start();  // buka UDP port 5683

  String ip = WiFi.localIP().toString();
  Serial.println("CoAP server siap di UDP port 5683. Coba dari laptop di WiFi yang sama:");
  Serial.printf("  aiocoap-client coap://%s/sensor\n", ip.c_str());
  Serial.printf("  aiocoap-client -m PUT --payload 1 coap://%s/led\n", ip.c_str());
}

void loop() {
  coap.loop();  // tugas 1: layani request CoAP yang masuk

  if (timerDHT.sudahWaktunya(millis())) {  // tugas 2: baca DHT11 setiap 2 detik
    bacaDHT();
  }

  pantauWiFi();  // tugas 3: laporkan perubahan status WiFi
}
