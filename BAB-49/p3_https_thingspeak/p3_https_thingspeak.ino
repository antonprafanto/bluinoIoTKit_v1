/*
 * BAB 49 - Program 3: Kirim Data DHT11 ke ThingSpeak lewat HTTPS
 * Setiap 20 detik: baca DHT11, lalu kirim suhu (field1) dan kelembapan
 * (field2) ke ThingSpeak dengan HTTPS POST. Sertifikat server diperiksa
 * dengan setCACert() memakai root CA DigiCert Global Root G2.
 * Kata sandi WiFi dan kunci API ada di tab "rahasia.h" (JANGAN dibagikan).
 *
 * Rangkaian (kabel jumper female-female):
 *   DHT        -> IO27
 *   LED1 merah -> IO4  (menyala 1 detik = gagal)
 *   LED2 hijau -> IO17 (menyala 1 detik = berhasil)
 * Library: DHT sensor library + Adafruit Unified Sensor (Adafruit).
 */

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <DHT.h>
#include "rahasia.h"  // RAHASIA_WIFI_SSID, RAHASIA_WIFI_PASSWORD, RAHASIA_THINGSPEAK_KEY

#define PIN_DHT    27
#define LED_MERAH  4
#define LED_HIJAU  17

const char* URL_THINGSPEAK = "https://api.thingspeak.com/update";
const unsigned long JEDA_KIRIM = 20000;  // minimal 15 detik (batas akun gratis)
const unsigned long LAMA_LED   = 1000;   // LED status menyala 1 detik

// Root CA untuk api.thingspeak.com
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

DHT dht(PIN_DHT, DHT11);

bool sudahPernahKirim = false;
unsigned long waktuKirimTerakhir = 0;
unsigned long waktuLedNyala = 0;
bool ledMenyala = false;
int jumlahBerhasil = 0;
int jumlahGagal = 0;

void sambungWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(RAHASIA_WIFI_SSID, RAHASIA_WIFI_PASSWORD);
  Serial.print("Menghubungkan ke WiFi");

  unsigned long mulai = millis();
  while (WiFi.status() != WL_CONNECTED) {
    if (millis() - mulai >= 20000) {  // menyerah setelah 20 detik
      Serial.println();
      Serial.println("Gagal terhubung! Periksa isi rahasia.h (WiFi harus 2,4 GHz).");
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

// Nyalakan satu LED status; dimatikan otomatis di loop() setelah LAMA_LED
void nyalakanLed(int pin) {
  digitalWrite(LED_MERAH, LOW);
  digitalWrite(LED_HIJAU, LOW);
  digitalWrite(pin, HIGH);
  waktuLedNyala = millis();
  ledMenyala = true;
}

// Mengirim data ke ThingSpeak. Hasilnya true kalau data diterima.
bool kirimKeThingSpeak(float suhu, float kelembapan) {
  WiFiClientSecure klienAman;
  klienAman.setCACert(ROOT_CA_DIGICERT_G2);  // periksa sertifikat api.thingspeak.com
  klienAman.setHandshakeTimeout(15);         // detik

  HTTPClient http;
  http.setConnectTimeout(10000);             // ms
  http.setTimeout(10000);                    // ms
  if (!http.begin(klienAman, URL_THINGSPEAK)) {
    Serial.println("Alamat URL tidak valid.");
    return false;
  }
  http.setUserAgent("BluinoKit-BAB49/1.0");  // "nama" client kita
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  // Kunci API dikirim di ISI permintaan (body), bukan di alamat URL
  char isi[96];
  snprintf(isi, sizeof(isi), "api_key=%s&field1=%.1f&field2=%.0f",
           RAHASIA_THINGSPEAK_KEY, suhu, kelembapan);

  int kode = http.POST(String(isi));
  bool berhasil = false;

  if (kode == HTTP_CODE_OK) {
    String balasan = http.getString();  // nomor entri, atau "0" kalau ditolak
    long nomorEntri = balasan.toInt();
    if (nomorEntri > 0) {
      Serial.printf("Terkirim! Nomor entri ThingSpeak: %ld\n", nomorEntri);
      berhasil = true;
    } else {
      Serial.println("Ditolak ThingSpeak (balasan 0). Jeda kiriman kurang dari 15 detik?");
    }
  } else if (kode > 0) {
    Serial.printf("ThingSpeak menjawab HTTP %d. Periksa Write API Key di rahasia.h.\n", kode);
  } else {
    char pesanTls[128];
    int kodeTls = klienAman.lastError(pesanTls, sizeof(pesanTls));
    Serial.printf("Koneksi gagal: %s (kode %d)\n", http.errorToString(kode).c_str(), kode);
    Serial.printf("Pesan TLS: %s (kode %d)\n", pesanTls, kodeTls);
  }

  http.end();
  return berhasil;
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);
  digitalWrite(LED_MERAH, LOW);
  digitalWrite(LED_HIJAU, LOW);
  delay(500);
  Serial.println();
  Serial.println("BAB 49 - Program 3: Kirim Data DHT11 ke ThingSpeak lewat HTTPS");

  dht.begin();
  sambungWiFi();
}

void loop() {
  // 1. Matikan LED status setelah LAMA_LED (tanpa delay)
  if (ledMenyala && millis() - waktuLedNyala >= LAMA_LED) {
    digitalWrite(LED_MERAH, LOW);
    digitalWrite(LED_HIJAU, LOW);
    ledMenyala = false;
  }

  // 2. Baca sensor dan kirim data setiap JEDA_KIRIM
  if (!sudahPernahKirim || millis() - waktuKirimTerakhir >= JEDA_KIRIM) {
    sudahPernahKirim = true;
    waktuKirimTerakhir = millis();

    float kelembapan = dht.readHumidity();
    float suhu = dht.readTemperature();
    if (isnan(suhu) || isnan(kelembapan)) {
      Serial.println("\nGagal membaca DHT11! Periksa kabel DHT -> IO27.");
      jumlahGagal = jumlahGagal + 1;
      nyalakanLed(LED_MERAH);
      return;
    }
    Serial.printf("\nSuhu: %.1f °C | Kelembapan: %.0f %%\n", suhu, kelembapan);

    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("WiFi terputus, data kali ini dilewati.");
      jumlahGagal = jumlahGagal + 1;
      nyalakanLed(LED_MERAH);
      return;
    }

    if (kirimKeThingSpeak(suhu, kelembapan)) {
      jumlahBerhasil = jumlahBerhasil + 1;
      nyalakanLed(LED_HIJAU);
    } else {
      jumlahGagal = jumlahGagal + 1;
      nyalakanLed(LED_MERAH);
    }
    Serial.printf("Total: %d berhasil, %d gagal\n", jumlahBerhasil, jumlahGagal);
  }
}
