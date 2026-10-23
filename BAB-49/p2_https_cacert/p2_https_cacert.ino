/*
 * BAB 49 - Program 2: HTTPS Aman dengan setCACert()
 * 1. Menyetel jam lewat NTP (BAB 42).
 * 2. Uji A: www.howsmyssl.com dengan root CA yang BENAR (ISRG Root X1) -> diterima.
 * 3. Uji B: www.howsmyssl.com dengan root CA yang SALAH (DigiCert Global Root G2)
 *    -> ditolak, sama seperti saat penyerang menyodorkan sertifikat palsu.
 *
 * Rangkaian: tidak perlu.
 * Library  : ArduinoJson (Benoit Blanchon) versi 7.
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

const char* WIFI_SSID     = "NamaWiFiKamu";      // ← ganti dengan WiFi kamu
const char* WIFI_PASSWORD = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu

const char* URL_CEK_TLS = "https://www.howsmyssl.com/a/check";

const char* ZONA_WAKTU   = "WIB-7";            // WITA-8 atau WIT-9 (lihat BAB 42)
const char* SERVER_NTP_1 = "id.pool.ntp.org";
const char* SERVER_NTP_2 = "pool.ntp.org";

// Root CA yang BENAR untuk www.howsmyssl.com (sertifikatnya dari Let's Encrypt).
// Sumber: https://letsencrypt.org/certs/isrgrootx1.pem
// Berlaku sampai: 4 Juni 2035
const char ROOT_CA_ISRG_X1[] = R"EOF(
-----BEGIN CERTIFICATE-----
MIIFazCCA1OgAwIBAgIRAIIQz7DSQONZRGPgu2OCiwAwDQYJKoZIhvcNAQELBQAw
TzELMAkGA1UEBhMCVVMxKTAnBgNVBAoTIEludGVybmV0IFNlY3VyaXR5IFJlc2Vh
cmNoIEdyb3VwMRUwEwYDVQQDEwxJU1JHIFJvb3QgWDEwHhcNMTUwNjA0MTEwNDM4
WhcNMzUwNjA0MTEwNDM4WjBPMQswCQYDVQQGEwJVUzEpMCcGA1UEChMgSW50ZXJu
ZXQgU2VjdXJpdHkgUmVzZWFyY2ggR3JvdXAxFTATBgNVBAMTDElTUkcgUm9vdCBY
MTCCAiIwDQYJKoZIhvcNAQEBBQADggIPADCCAgoCggIBAK3oJHP0FDfzm54rVygc
h77ct984kIxuPOZXoHj3dcKi/vVqbvYATyjb3miGbESTtrFj/RQSa78f0uoxmyF+
0TM8ukj13Xnfs7j/EvEhmkvBioZxaUpmZmyPfjxwv60pIgbz5MDmgK7iS4+3mX6U
A5/TR5d8mUgjU+g4rk8Kb4Mu0UlXjIB0ttov0DiNewNwIRt18jA8+o+u3dpjq+sW
T8KOEUt+zwvo/7V3LvSye0rgTBIlDHCNAymg4VMk7BPZ7hm/ELNKjD+Jo2FR3qyH
B5T0Y3HsLuJvW5iB4YlcNHlsdu87kGJ55tukmi8mxdAQ4Q7e2RCOFvu396j3x+UC
B5iPNgiV5+I3lg02dZ77DnKxHZu8A/lJBdiB3QW0KtZB6awBdpUKD9jf1b0SHzUv
KBds0pjBqAlkd25HN7rOrFleaJ1/ctaJxQZBKT5ZPt0m9STJEadao0xAH0ahmbWn
OlFuhjuefXKnEgV4We0+UXgVCwOPjdAvBbI+e0ocS3MFEvzG6uBQE3xDk3SzynTn
jh8BCNAw1FtxNrQHusEwMFxIt4I7mKZ9YIqioymCzLq9gwQbooMDQaHWBfEbwrbw
qHyGO0aoSCqI3Haadr8faqU9GY/rOPNk3sgrDQoo//fb4hVC1CLQJ13hef4Y53CI
rU7m2Ys6xt0nUW7/vGT1M0NPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNV
HRMBAf8EBTADAQH/MB0GA1UdDgQWBBR5tFnme7bl5AFzgAiIyBpY9umbbjANBgkq
hkiG9w0BAQsFAAOCAgEAVR9YqbyyqFDQDLHYGmkgJykIrGF1XIpu+ILlaS/V9lZL
ubhzEFnTIZd+50xx+7LSYK05qAvqFyFWhfFQDlnrzuBZ6brJFe+GnY+EgPbk6ZGQ
3BebYhtF8GaV0nxvwuo77x/Py9auJ/GpsMiu/X1+mvoiBOv/2X/qkSsisRcOj/KK
NFtY2PwByVS5uCbMiogziUwthDyC3+6WVwW6LLv3xLfHTjuCvjHIInNzktHCgKQ5
ORAzI4JMPJ+GslWYHb4phowim57iaztXOoJwTdwJx4nLCgdNbOhdjsnvzqvHu7Ur
TkXWStAmzOVyyghqpZXjFaH3pO3JLF+l+/+sKAIuvtd7u+Nxe5AW0wdeRlN8NwdC
jNPElpzVmbUq4JUagEiuTDkHzsxHpFKVK7q4+63SM1N95R1NbdWhscdCb+ZAJzVc
oyi3B43njTOQ5yOf+1CceWxG1bQVs5ZufpsMljq4Ui0/1lvh+wjChP4kqKOJ2qxq
4RgqsahDYVvTH9w7jXbyLeiNdd8XM2w9U/t7y0Ff/9yi0GE44Za4rF2LN9d11TPA
mRGunUHBcnWEvgJBQl9nJEiU0Zsnvgc/ubhPgXRR4Xq37Z0j4r7g1SgEEzwxA57d
emyPxgcYxn/eR44/KJ4EBs+lVDR3veyJm+kXQ99b21/+jh5Xos1AnX5iItreGCc=
-----END CERTIFICATE-----
)EOF";

// Root CA milik penerbit LAIN (DigiCert). Di program ini SENGAJA dipakai
// sebagai "kunci yang salah" untuk Uji B. (Di Program 3 dipakai untuk ThingSpeak.)
// Sumber: https://cacerts.digicert.com/DigiCertGlobalRootG2.crt.pem
// Berlaku sampai: 15 Januari 2038
const char ROOT_CA_DIGICERT_G2[] = R"EOF(
-----BEGIN CERTIFICATE-----
MIIDjjCCAnagAwIBAgIQAzrx5qcRqaC7KGSxHQn65TANBgkqhkiG9w0BAQsFADBh
MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3
d3cuZGlnaWNlcnQuY29tMSAwHgYDVQQDExdEaWdpQ2VydCBHbG9iYWwgUm9vdCBH
MjAeFw0xMzA4MDExMjAwMDBaFw0zODAxMTUxMjAwMDBaMGExCzAJBgNVBAYTAlVT
MRUwEwYDVQQKEwxEaWdpQ2VydCBJbmMxGTAXBgNVBAsTEHd3dy5kaWdpY2VydC5j
b20xIDAeBgNVBAMTF0RpZ2lDZXJ0IEdsb2JhbCBSb290IEcyMIIBIjANBgkqhkiG
9w0BAQEFAAOCAQ8AMIIBCgKCAQEAuzfNNNx7a8myaJCtSnX/RrohCgiN9RlUyfuI
2/Ou8jqJkTx65qsGGmvPrC3oXgkkRLpimn7Wo6h+4FR1IAWsULecYxpsMNzaHxmx
1x7e/dfgy5SDN67sH0NO3Xss0r0upS/kqbitOtSZpLYl6ZtrAGCSYP9PIUkY92eQ
q2EGnI/yuum06ZIya7XzV+hdG82MHauVBJVJ8zUtluNJbd134/tJS7SsVQepj5Wz
tCO7TG1F8PapspUwtP1MVYwnSlcUfIKdzXOS0xZKBgyMUNGPHgm+F6HmIcr9g+UQ
vIOlCsRnKPZzFBQ9RnbDhxSJITRNrw9FDKZJobq7nMWxM4MphQIDAQABo0IwQDAP
BgNVHRMBAf8EBTADAQH/MA4GA1UdDwEB/wQEAwIBhjAdBgNVHQ4EFgQUTiJUIBiV
5uNu5g/6+rkS7QYXjzkwDQYJKoZIhvcNAQELBQADggEBAGBnKJRvDkhj6zHd6mcY
1Yl9PMWLSn/pvtsrF9+wX3N3KjITOYFnQoQj8kVnNeyIv/iPsGEMNKSuIEyExtv4
NeF22d+mQrvHRAiGfzZ0JFrabA0UWTW98kndth/Jsw1HKj2ZL7tcu7XUIOGZX1NG
Fdtom/DzMNU+MeKNhJ7jitralj41E6Vf8PlwUHBHQRFXGU7Aj64GxJUTFy8bJZ91
8rGOmaFvE7FBcf6IKshPECBV1/MUReXgRPTqh5Uykw7+U0b6LJ3/iyK5S9kJRaTe
pLiaWN0bfVKfjllDiIGknibVb63dDcY3fe0Dkhvld1927jyNxF1WW6LZZm6zNTfl
MrY=
-----END CERTIFICATE-----
)EOF";

// Kemungkinan hasil satu uji
const int HASIL_BERHASIL = 0;   // sertifikat diterima, server menjawab 200
const int HASIL_DITOLAK  = 1;   // sertifikat server DITOLAK oleh ESP32
const int HASIL_GAGAL    = 2;   // gagal karena hal lain (jaringan, server, dll.)

const unsigned long JEDA_UJI = 300000;  // ulangi setiap 5 menit
unsigned long waktuUjiTerakhir = 0;

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

void setelJam() {
  configTzTime(ZONA_WAKTU, SERVER_NTP_1, SERVER_NTP_2);
  Serial.print("Menyetel jam lewat NTP");

  struct tm waktu;
  unsigned long mulai = millis();
  while (!getLocalTime(&waktu, 0)) {  // 0 = jangan menunggu di dalam fungsi
    if (millis() - mulai >= 15000) {
      Serial.println();
      Serial.println("Jam belum tersinkron. Program tetap lanjut (lihat 49.8).");
      return;
    }
    Serial.print(".");
    delay(500);
  }

  char teksWaktu[32];
  strftime(teksWaktu, sizeof(teksWaktu), "%Y-%m-%d %H:%M:%S %Z", &waktu);
  Serial.println();
  Serial.printf("Jam sekarang: %s\n", teksWaktu);
}

// Menghubungi url dan hanya memercayai rootCa. Mengembalikan HASIL_...
int ujiHttps(const char* label, const char* url, const char* rootCa) {
  Serial.printf("\n[%s] GET %s\n", label, url);

  WiFiClientSecure klienAman;
  klienAman.setCACert(rootCa);        // hanya percaya pada root CA ini
  klienAman.setHandshakeTimeout(15);  // detik

  HTTPClient http;
  http.setConnectTimeout(10000);      // ms
  http.setTimeout(10000);             // ms
  if (!http.begin(klienAman, url)) {
    Serial.printf("[%s] Alamat URL tidak valid.\n", label);
    return HASIL_GAGAL;
  }
  http.setUserAgent("BluinoKit-BAB49/1.0");  // "nama" client kita

  int kode = http.GET();
  int hasil = HASIL_GAGAL;

  if (kode == HTTP_CODE_OK) {
    JsonDocument saringan;
    saringan["tls_version"] = true;
    JsonDocument doc;
    deserializeJson(doc, http.getString(), DeserializationOption::Filter(saringan));
    Serial.printf("[%s] BERHASIL: sertifikat server terverifikasi, HTTP %d, %s\n",
                  label, kode, doc["tls_version"] | "TLS ?");
    hasil = HASIL_BERHASIL;
  } else if (kode > 0) {
    Serial.printf("[%s] Sertifikat terverifikasi, tetapi server menjawab HTTP %d\n", label, kode);
  } else {
    // Koneksi gagal. Tanyakan penyebabnya ke lapisan TLS.
    char pesanTls[128];
    int kodeTls = klienAman.lastError(pesanTls, sizeof(pesanTls));
    if (kodeTls == MBEDTLS_ERR_X509_CERT_VERIFY_FAILED) {
      Serial.printf("[%s] DITOLAK: %s (kode HTTPClient %d)\n", label, http.errorToString(kode).c_str(), kode);
      hasil = HASIL_DITOLAK;
    } else {
      Serial.printf("[%s] GAGAL: %s (kode HTTPClient %d)\n", label, http.errorToString(kode).c_str(), kode);
    }
    Serial.printf("[%s] Pesan TLS: %s (kode %d)\n", label, pesanTls, kodeTls);
  }

  http.end();
  return hasil;
}

void jalankanUji() {
  int hasilA = ujiHttps("Uji A", URL_CEK_TLS, ROOT_CA_ISRG_X1);      // root CA benar
  int hasilB = ujiHttps("Uji B", URL_CEK_TLS, ROOT_CA_DIGICERT_G2);  // root CA salah

  Serial.println();
  Serial.println("Kesimpulan:");
  if (hasilA == HASIL_BERHASIL) {
    Serial.println("- Root CA benar (ISRG Root X1)       : diterima");
  } else {
    Serial.println("- Root CA benar (ISRG Root X1)       : GAGAL, periksa jaringan atau sertifikat");
  }
  if (hasilB == HASIL_DITOLAK) {
    Serial.println("- Root CA salah (DigiCert Global G2) : ditolak, sesuai harapan");
  } else if (hasilB == HASIL_BERHASIL) {
    Serial.println("- Root CA salah (DigiCert Global G2) : diterima?! Periksa kodenya");
  } else {
    Serial.println("- Root CA salah (DigiCert Global G2) : gagal karena hal lain, coba lagi");
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("BAB 49 - Program 2: HTTPS Aman dengan setCACert()");

  sambungWiFi();
  setelJam();
  jalankanUji();
  waktuUjiTerakhir = millis();
}

void loop() {
  if (millis() - waktuUjiTerakhir >= JEDA_UJI) {
    waktuUjiTerakhir = millis();
    if (WiFi.status() == WL_CONNECTED) {
      jalankanUji();
    } else {
      Serial.println("WiFi terputus, menunggu tersambung ulang...");
    }
  }
}
