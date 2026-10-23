/*
 * BAB 49 - Program 4: Bundel Sertifikat Bawaan
 * Satu klien HTTPS dengan useBuiltinCACertBundle() mencoba beberapa alamat:
 * tiga situs sah dengan root CA berbeda (harus DITERIMA) dan dua situs uji
 * badssl.com yang sengaja memakai sertifikat bermasalah (harus DITOLAK).
 * Butuh board package esp32 versi 3.3.12 atau lebih baru.
 *
 * Rangkaian: tidak perlu. Library: tidak perlu library tambahan.
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>

const char* WIFI_SSID     = "NamaWiFiKamu";      // ← ganti dengan WiFi kamu
const char* WIFI_PASSWORD = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu

// Daftar alamat yang diuji (tipe data ditulis di atas, sebelum fungsi apa pun)
struct SitusUji {
  const char* url;
  const char* keterangan;
};

const SitusUji DAFTAR_SITUS[] = {
  {"https://www.howsmyssl.com/a/check", "root CA: ISRG Root X1"},
  {"https://api.thingspeak.com/channels/9/feeds.json?results=1", "root CA: DigiCert Global Root G2"},
  {"https://httpbin.org/get", "root CA: Amazon Root CA 1"},
  {"https://self-signed.badssl.com/", "sertifikat buatan sendiri, harus DITOLAK"},
  {"https://wrong.host.badssl.com/", "nama domain tidak cocok, harus DITOLAK"},
};
const int JUMLAH_SITUS = sizeof(DAFTAR_SITUS) / sizeof(DAFTAR_SITUS[0]);

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

void ujiSitus(int nomor, const SitusUji& situs) {
  Serial.printf("\n%d. %s\n   (%s)\n", nomor, situs.url, situs.keterangan);

  WiFiClientSecure klienAman;
  klienAman.useBuiltinCACertBundle();  // percayai root CA di bundel bawaan
  klienAman.setHandshakeTimeout(15);   // detik

  HTTPClient http;
  http.setConnectTimeout(10000);       // ms
  http.setTimeout(10000);              // ms
  if (!http.begin(klienAman, situs.url)) {
    Serial.println("   Alamat URL tidak valid.");
    return;
  }
  http.setUserAgent("BluinoKit-BAB49/1.0");  // "nama" client kita

  int kode = http.GET();
  if (kode > 0) {
    Serial.printf("   DITERIMA - sertifikat sah, server menjawab HTTP %d\n", kode);
  } else {
    char pesanTls[128];
    int kodeTls = klienAman.lastError(pesanTls, sizeof(pesanTls));
    if (kodeTls == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED) {
      Serial.printf("   DITOLAK - sertifikat tidak lolos pemeriksaan (kode TLS %d)\n", kodeTls);
    } else {
      Serial.printf("   GAGAL - %s (kode HTTPClient %d, kode TLS %d)\n",
                    http.errorToString(kode).c_str(), kode, kodeTls);
    }
  }
  http.end();
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("BAB 49 - Program 4: Bundel Sertifikat Bawaan");

  sambungWiFi();
  for (int i = 0; i < JUMLAH_SITUS; i++) {
    ujiSitus(i + 1, DAFTAR_SITUS[i]);
  }
  Serial.println("\nSelesai. Tekan tombol EN di board untuk mengulang.");
}

void loop() {
  // Tidak ada yang dikerjakan: pengujian hanya berjalan sekali di setup().
}
