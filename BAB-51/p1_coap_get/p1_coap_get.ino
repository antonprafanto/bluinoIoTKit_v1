/*
 * BAB 51 - Program 1: CoAP GET ke Server Publik
 * ESP32 menjadi client CoAP dan mengirim GET ke coap://coap.me/<resource>.
 * - Saat menyala: GET coap://coap.me/hello
 * - Ketik nama resource di Serial Monitor (line ending: New Line),
 *   misalnya test atau abc, lalu tekan Enter untuk mengirim GET lain.
 * - Kalau jawaban tidak datang dalam 5 detik, program memberi tahu.
 * Rangkaian: tidak perlu. Butuh WiFi 2,4 GHz yang tersambung ke internet.
 */

#include <WiFi.h>
#include <WiFiUdp.h>
#include <coap-simple.h>

const char *NAMA_WIFI = "NamaWiFiKamu";          // ← ganti dengan WiFi kamu
const char *PASSWORD_WIFI = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu

const char *SERVER_COAP = "coap.me";      // server uji CoAP publik
const int PORT_COAP = 5683;               // port bawaan CoAP
const unsigned long BATAS_TUNGGU = 5000;  // ms, batas menunggu jawaban

WiFiUDP udp;
Coap coap(udp, 256);  // buffer kirim & terima masing-masing 256 byte

// Data request yang sedang menunggu jawaban
bool menunggu = false;
uint16_t idDitunggu = 0;
unsigned long waktuKirim = 0;

// Nama jenis pesan CoAP
const char *namaJenis(uint8_t jenis) {
  switch (jenis) {
    case COAP_CON: return "CON";
    case COAP_NONCON: return "NON";
    case COAP_ACK: return "ACK";
    case COAP_RESET: return "RST";
    default: return "?";
  }
}

// Arti beberapa kode respons yang sering muncul
const char *artiKode(uint8_t kode) {
  switch (kode) {
    case 0: return "Empty (ACK kosong, jawaban menyusul terpisah)";
    case COAP_CREATED: return "Created (berhasil dibuat)";
    case COAP_DELETED: return "Deleted (berhasil dihapus)";
    case COAP_CHANGED: return "Changed (berhasil diubah)";
    case COAP_CONTENT: return "Content (berhasil, ada isi)";
    case COAP_BAD_REQUEST: return "Bad Request (permintaan tidak dimengerti)";
    case COAP_NOT_FOUND: return "Not Found (resource tidak ada)";
    case COAP_METHOD_NOT_ALLOWED: return "Method Not Allowed (method tidak didukung)";
    case COAP_INTERNAL_SERVER_ERROR: return "Internal Server Error (kesalahan di server)";
    case COAP_SERVICE_UNAVAILABLE: return "Service Unavailable (server sedang tidak bisa melayani)";
    default: return "(lihat tabel kode respons)";
  }
}

// Callback: dipanggil oleh coap.loop() saat paket ACK (jawaban) datang
void terimaJawaban(CoapPacket &paket, IPAddress ip, int port) {
  if (!menunggu || paket.messageid != idDitunggu) {
    Serial.printf("[INFO] Jawaban untuk Message ID %u tidak sedang ditunggu, diabaikan.\n",
                  paket.messageid);
    return;
  }
  menunggu = false;

  Serial.printf("[JAWABAN] %s, Message ID %u, %lu ms\n",
                namaJenis(paket.type), paket.messageid, millis() - waktuKirim);
  Serial.printf("[JAWABAN] kode %d.%02d = %s\n",
                paket.code >> 5, paket.code & 0x1F, artiKode(paket.code));
  Serial.printf("[JAWABAN] isi (%u byte): ", (unsigned)paket.payloadlen);
  if (paket.payloadlen > 0) {
    Serial.write(paket.payload, paket.payloadlen);  // payload TIDAK diakhiri '\0'
  } else {
    Serial.print("(kosong)");
  }
  Serial.println();
}

// Mengirim GET ke coap://coap.me/<resource>
void kirimGet(const String &resource) {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[GAGAL] WiFi belum tersambung.");
    return;
  }

  // DNS: ubah nama "coap.me" menjadi alamat IP (menunggu sebentar)
  IPAddress ipServer;
  if (!WiFi.hostByName(SERVER_COAP, ipServer)) {
    Serial.printf("[GAGAL] Alamat IP %s tidak ditemukan (DNS gagal).\n", SERVER_COAP);
    return;
  }

  // Path ditulis TANPA '/' di depan, misalnya "hello"
  uint16_t id = coap.get(ipServer, PORT_COAP, resource.c_str());
  if (id == 0) {
    Serial.println("[GAGAL] Paket tidak bisa disusun (nama resource terlalu panjang?).");
    return;
  }

  menunggu = true;
  idDitunggu = id;
  waktuKirim = millis();
  Serial.println();
  Serial.printf("[GET] coap://%s/%s (IP %s), Message ID %u\n",
                SERVER_COAP, resource.c_str(), ipServer.toString().c_str(), id);
}

// Membaca nama resource yang diketik di Serial Monitor
void bacaSerial() {
  if (!Serial.available()) return;

  String masukan = Serial.readStringUntil('\n');
  masukan.trim();
  if (masukan.startsWith("/")) masukan.remove(0, 1);  // buang '/' di depan
  if (masukan.length() == 0) return;

  if (menunggu) {
    Serial.println("[INFO] Masih menunggu jawaban sebelumnya. Coba lagi sebentar.");
    return;
  }
  kirimGet(masukan);
}

// Memberi tahu kalau jawaban tidak datang dalam BATAS_TUNGGU
void periksaBatasWaktu() {
  if (menunggu && millis() - waktuKirim >= BATAS_TUNGGU) {
    menunggu = false;
    Serial.printf("[GAGAL] Tidak ada jawaban untuk Message ID %u dalam %lu detik.\n",
                  idDitunggu, BATAS_TUNGGU / 1000);
    Serial.println("        Paket hilang, atau jaringan memblokir UDP. Coba kirim lagi.");
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
  Serial.print("Alamat IP ESP32: ");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\nBAB 51 - Program 1: CoAP GET ke Server Publik");

  sambungkanWiFi();

  coap.response(terimaJawaban);  // daftarkan callback SEBELUM start()
  coap.start();                  // buka UDP port 5683
  Serial.println("CoAP siap (UDP port 5683).");
  Serial.println("Ketik nama resource lalu tekan Enter, misalnya: hello, test, abc");

  kirimGet("hello");
}

void loop() {
  coap.loop();          // tugas 1: proses paket CoAP yang masuk (memanggil callback)
  bacaSerial();         // tugas 2: cek apakah ada nama resource yang diketik
  periksaBatasWaktu();  // tugas 3: cek jawaban yang terlalu lama
}
