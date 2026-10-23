/*
 * BAB 49 - Program 1: HTTPS Pertama dengan setInsecure()
 * ESP32 menghubungi https://www.howsmyssl.com/a/check, lalu menampilkan
 * versi TLS, penilaian keamanan, dan memori yang dipakai TLS.
 *
 * PERCOBAAN SAJA: setInsecure() TIDAK memeriksa sertifikat server,
 * sehingga rentan serangan man-in-the-middle. Versi aman: Program 2.
 *
 * Rangkaian: tidak perlu.
 * Library  : ArduinoJson (Benoit Blanchon) versi 7.
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* WIFI_SSID     = "NamaWiFiKamu";      // ← ganti dengan WiFi kamu
const char* WIFI_PASSWORD = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu

const char* URL_CEK_TLS = "https://www.howsmyssl.com/a/check";
const unsigned long JEDA_CEK = 60000;  // ulangi setiap 60 detik

unsigned long waktuCekTerakhir = 0;
int nomorCek = 0;

void sambungWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Menghubungkan ke WiFi");

  unsigned long mulai = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - mulai >= 20000) {  // menyerah setelah 20 detik
      Serial.println();
      Serial.println("Gagal terhubung! Periksa nama dan password WiFi (harus 2,4 GHz).");
      Serial.println("ESP32 restart dalam 5 detik...");
      delay(5000);
      ESP.restart();
    }
    Serial.print(".");
    delay(500);
  }
  Serial.println();
  Serial.print("Terhubung! Alamat IP: ");
  Serial.println(WiFi.localIP());
}

void cekTls() {
  nomorCek = nomorCek + 1;
  Serial.printf("\n--- Cek #%d: GET %s ---\n", nomorCek, URL_CEK_TLS);

  uint32_t heapAwal = ESP.getFreeHeap();

  WiFiClientSecure klienAman;
  klienAman.setInsecure();            // PERCOBAAN SAJA: sertifikat tidak diperiksa!
  klienAman.setHandshakeTimeout(15);  // batas waktu handshake TLS, dalam DETIK

  HTTPClient http;
  http.setConnectTimeout(10000);      // batas waktu membuka sambungan (ms)
  http.setTimeout(10000);             // batas waktu menunggu balasan (ms)
  if (!http.begin(klienAman, URL_CEK_TLS)) {
    Serial.println("Alamat URL tidak valid.");
    return;
  }
  http.setUserAgent("BluinoKit-BAB49/1.0");  // "nama" client kita

  unsigned long mulai = millis();
  int kode = http.GET();              // handshake TLS + kirim permintaan GET
  unsigned long lama = millis() - mulai;
  uint32_t heapSaatTls = ESP.getFreeHeap();  // koneksi TLS masih terbuka

  if (kode == HTTP_CODE_OK) {
    String isi = http.getString();

    // Ambil dua data saja dari JSON yang panjang
    JsonDocument saringan;
    saringan["tls_version"] = true;
    saringan["rating"] = true;

    JsonDocument doc;
    DeserializationError galat = deserializeJson(doc, isi, DeserializationOption::Filter(saringan));
    if (galat) {
      Serial.printf("JSON tidak bisa dibaca: %s\n", galat.c_str());
    } else {
      Serial.printf("Status HTTP  : %d (berhasil)\n", kode);
      Serial.printf("Versi TLS    : %s\n", doc["tls_version"] | "?");
      Serial.printf("Penilaian    : %s\n", doc["rating"] | "?");
      Serial.printf("Lama GET     : %lu ms\n", lama);
    }
  } else if (kode > 0) {
    Serial.printf("Server menjawab dengan kode HTTP %d\n", kode);
  } else {
    Serial.printf("Gagal: %s (kode %d)\n", http.errorToString(kode).c_str(), kode);
  }
  http.end();  // tutup koneksi, memori TLS dikembalikan

  uint32_t dipakaiTls = (heapAwal > heapSaatTls) ? (heapAwal - heapSaatTls) : 0;
  Serial.printf("Heap sebelum : %u byte\n", (unsigned)heapAwal);
  Serial.printf("Heap saat TLS: %u byte (dipakai TLS +/- %u byte)\n", (unsigned)heapSaatTls, (unsigned)dipakaiTls);
  Serial.printf("Heap sesudah : %u byte\n", (unsigned)ESP.getFreeHeap());
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("BAB 49 - Program 1: HTTPS Pertama dengan setInsecure()");
  Serial.println("PERINGATAN: sertifikat server TIDAK diperiksa. Hanya untuk percobaan!");

  sambungWiFi();
  cekTls();
  waktuCekTerakhir = millis();
}

void loop() {
  // Ulangi pemeriksaan setiap JEDA_CEK tanpa delay() (pola BAB 16)
  if (millis() - waktuCekTerakhir >= JEDA_CEK) {
    waktuCekTerakhir = millis();
    if (WiFi.status() == WL_CONNECTED) {
      cekTls();
    } else {
      Serial.println("WiFi terputus, menunggu tersambung ulang...");
    }
  }
}
