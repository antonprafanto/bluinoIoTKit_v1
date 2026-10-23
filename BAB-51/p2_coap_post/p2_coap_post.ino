/*
 * BAB 51 - Program 2: Kirim Data DHT11 dengan CoAP POST
 * Setiap 30 detik ESP32 mengirim suhu & kelembapan (JSON) ke
 * coap://coap.me/sink dengan POST berjenis CON.
 * Kalau ACK tidak datang, pesan yang sama (Message ID sama) dikirim ulang:
 * tunggu 2 detik, lalu 4 detik, lalu 8 detik sebelum menyerah.
 * DHT11 dibaca setiap 2 detik tanpa delay() (struct Timer dari BAB 16).
 * Rangkaian: DHT (header hijau) -> IO27. Butuh WiFi yang tersambung ke internet.
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>
#include <ArduinoJson.h>
#include <DHT.h>

const char *NAMA_WIFI = "NamaWiFiKamu";          // ← ganti dengan WiFi kamu
const char *PASSWORD_WIFI = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu

const char *SERVER_COAP = "coap.me";        // server uji CoAP publik
const int PORT_COAP = 5683;                 // port bawaan CoAP
const char *RESOURCE = "sink";              // tanpa '/' di depan
const char *NAMA_PERANGKAT = "bluino-kit";  // ganti dengan nama unik, misalnya "bluino-andi"

#define PIN_DHT 27
#define TIPE_DHT DHT11

const unsigned long TUNGGU_ACK_AWAL = 2000;  // ms, waktu tunggu ACK pertama
const uint8_t MAKS_KIRIM = 3;                // 1 kali kirim + 2 kali kirim ulang

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

Timer timerDHT = {2000, 0};     // baca DHT11 setiap 2 detik
Timer timerKirim = {30000, 0};  // kirim data setiap 30 detik

// Hasil bacaan DHT11 terakhir
float suhu = NAN;
float kelembapan = NAN;
bool dhtOk = false;
bool bacaanPertama = true;
bool sudahPernahKirim = false;

// Pesan yang sedang menunggu ACK
bool menungguAck = false;
uint16_t idPesan = 0;         // Message ID pesan yang sedang dikirim
uint16_t idBerikutnya = 1;    // Message ID untuk pesan baru berikutnya
uint8_t jumlahKirim = 0;      // sudah berapa kali pesan ini dikirim
unsigned long waktuKirim = 0;      // millis() saat kiriman terakhir
unsigned long waktuKirimAwal = 0;  // millis() saat kiriman pertama
unsigned long batasTunggu = 0;     // ms, waktu tunggu ACK saat ini
IPAddress ipServer;
char isiPesan[100];
size_t panjangIsi = 0;
unsigned long jumlahBerhasil = 0;
unsigned long jumlahGagal = 0;

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

void cetakTotal() {
  Serial.printf("[TOTAL] berhasil %lu, gagal %lu\n", jumlahBerhasil, jumlahGagal);
}

// Mengirim (atau mengirim ulang) pesan yang tersimpan di isiPesan
void kirimPaket() {
  jumlahKirim++;
  waktuKirim = millis();
  uint16_t hasil = coap.send(ipServer, PORT_COAP, RESOURCE,
                             COAP_CON, COAP_POST,              // jenis CON, method POST
                             NULL, 0,                          // tanpa token
                             (const uint8_t *)isiPesan, panjangIsi,
                             COAP_APPLICATION_JSON,            // Content-Format 50
                             idPesan);                         // Message ID pilihan kita
  if (hasil == 0) {
    Serial.println("[GAGAL] Paket tidak bisa disusun (isi terlalu besar untuk buffer?).");
    menungguAck = false;
    jumlahGagal++;
    cetakTotal();
    return;
  }
  Serial.printf("[KIRIM] Message ID %u, percobaan %u dari %u\n", idPesan, jumlahKirim, MAKS_KIRIM);
}

// Menyiapkan pesan baru berisi data DHT11, lalu mengirimnya
void mulaiKirimData() {
  if (menungguAck) {
    Serial.println("[INFO] Pesan sebelumnya belum selesai, data kali ini dilewati.");
    return;
  }
  if (!dhtOk) {
    Serial.println("[INFO] Belum ada bacaan DHT11 yang valid, pengiriman dilewati.");
    return;
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[INFO] WiFi terputus, pengiriman dilewati.");
    return;
  }
  if (!WiFi.hostByName(SERVER_COAP, ipServer)) {  // DNS: nama -> alamat IP
    Serial.printf("[GAGAL] Alamat IP %s tidak ditemukan (DNS gagal).\n", SERVER_COAP);
    return;
  }

  // Susun JSON, misalnya {"id":"bluino-kit","suhu":28.6,"kelembapan":70}
  JsonDocument doc;
  doc["id"] = NAMA_PERANGKAT;
  doc["suhu"] = suhu;
  doc["kelembapan"] = kelembapan;
  panjangIsi = serializeJson(doc, isiPesan, sizeof(isiPesan));

  // Pesan baru = Message ID baru (0 tidak dipakai)
  idPesan = idBerikutnya;
  idBerikutnya++;
  if (idBerikutnya == 0) idBerikutnya = 1;

  jumlahKirim = 0;
  batasTunggu = TUNGGU_ACK_AWAL;
  menungguAck = true;
  waktuKirimAwal = millis();
  Serial.println();
  Serial.printf("[POST] coap://%s/%s, isi: %s\n", SERVER_COAP, RESOURCE, isiPesan);
  kirimPaket();
}

// Callback: dipanggil oleh coap.loop() saat paket ACK datang
void terimaJawaban(CoapPacket &paket, IPAddress ip, int port) {
  if (!menungguAck || paket.messageid != idPesan) {
    Serial.printf("[INFO] ACK untuk Message ID %u datang terlambat, diabaikan.\n",
                  paket.messageid);
    return;
  }
  menungguAck = false;

  int kelas = paket.code >> 5;
  int detail = paket.code & 0x1F;
  Serial.printf("[ACK] Message ID %u, %lu ms sejak kiriman pertama\n",
                paket.messageid, millis() - waktuKirimAwal);
  Serial.printf("[ACK] kode %d.%02d", kelas, detail);
  if (paket.code == COAP_CREATED) Serial.print(" = Created (berhasil dibuat)");
  Serial.print(", isi: ");
  if (paket.payloadlen > 0) {
    Serial.write(paket.payload, paket.payloadlen);  // payload TIDAK diakhiri '\0'
  } else {
    Serial.print("(kosong)");
  }
  Serial.println();

  if (kelas == 2) {
    jumlahBerhasil++;
  } else {
    jumlahGagal++;  // misalnya 4.xx: permintaan ditolak server
  }
  cetakTotal();
}

// Kirim ulang kalau ACK belum datang, dengan waktu tunggu yang digandakan
void periksaAck() {
  if (!menungguAck) return;
  if (millis() - waktuKirim < batasTunggu) return;

  Serial.printf("[ULANG] Belum ada ACK setelah %lu detik.\n", batasTunggu / 1000);
  if (jumlahKirim >= MAKS_KIRIM) {
    menungguAck = false;
    jumlahGagal++;
    Serial.printf("[GAGAL] Tidak ada ACK setelah %u kali kirim. Data ini dilewati.\n", jumlahKirim);
    cetakTotal();
    return;
  }
  batasTunggu = batasTunggu * 2;  // 2 s -> 4 s -> 8 s
  kirimPaket();                   // Message ID tetap sama
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
  Serial.print("Alamat IP ESP32: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nBAB 51 - Program 2: Kirim Data DHT11 dengan CoAP POST");

  dht.begin();
  sambungkanWiFi();

  idBerikutnya = random(1, 65536);  // Message ID awal acak (random() memakai generator acak ESP32)

  coap.response(terimaJawaban);  // daftarkan callback SEBELUM start()
  coap.start();                  // buka UDP port 5683
  Serial.printf("CoAP siap. Data dikirim ke coap://%s/%s setiap 30 detik.\n",
                SERVER_COAP, RESOURCE);
}

void loop() {
  unsigned long sekarang = millis();

  coap.loop();  // tugas 1: proses ACK yang masuk (memanggil callback)

  if (timerDHT.sudahWaktunya(sekarang)) {  // tugas 2: baca DHT11 setiap 2 detik
    bacaDHT();
  }

  // Tugas 3: kirim pertama segera setelah DHT11 terbaca, berikutnya setiap 30 detik
  if (dhtOk && !sudahPernahKirim) {
    sudahPernahKirim = true;
    timerKirim.waktuLalu = sekarang;
    mulaiKirimData();
  } else if (timerKirim.sudahWaktunya(sekarang)) {
    mulaiKirimData();
  }

  periksaAck();  // tugas 4: kirim ulang kalau ACK belum datang
}
