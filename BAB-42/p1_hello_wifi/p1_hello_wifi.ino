/*
 * BAB 42 - Program 1: Hello WiFi
 * Menyambungkan ESP32 ke WiFi dalam mode Station, lalu menampilkan
 * informasi jaringan dan kekuatan sinyal di Serial Monitor.
 *
 * Rangkaian: tidak perlu. LED biru bawaan board (IO2) menyala saat tersambung.
 * Alat uji : Serial Monitor, baud 115200.
 * Jaringan : WiFi 2,4 GHz. Nama dan password peka huruf besar-kecil.
 */

#include <WiFi.h>   // library WiFi, sudah termasuk di board package esp32

const char *WIFI_SSID = "NamaWiFiKamu";      // ← ganti dengan WiFi kamu
const char *WIFI_PASS = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu

#define LED_BIRU 2   // LED biru bawaan board (juga LED3 di kit lewat IO2)

const unsigned long BATAS_TUNGGU   = 15000;  // menunggu tersambung paling lama 15 detik
const unsigned long INTERVAL_LAPOR = 5000;   // laporan sinyal setiap 5 detik

unsigned long waktuLaporTerakhir = 0;

// Mengubah angka RSSI menjadi kata yang mudah dipahami (lihat tabel di 42.4.3)
const char *kualitasSinyal(int rssi) {
  if (rssi >= -50) return "sangat kuat";
  if (rssi >= -60) return "kuat";
  if (rssi >= -70) return "cukup";
  if (rssi >= -80) return "lemah";
  return "sangat lemah";
}

void tampilkanInfoJaringan() {
  int rssi = WiFi.RSSI();
  Serial.println("Tersambung!");
  Serial.printf("  Nama WiFi (SSID) : %s\n", WiFi.SSID().c_str());
  Serial.printf("  Alamat IP ESP32  : %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("  Subnet mask      : %s\n", WiFi.subnetMask().toString().c_str());
  Serial.printf("  Gateway (router) : %s\n", WiFi.gatewayIP().toString().c_str());
  Serial.printf("  Server DNS       : %s\n", WiFi.dnsIP().toString().c_str());
  Serial.printf("  Kanal (channel)  : %ld\n", (long)WiFi.channel());
  Serial.printf("  Hostname ESP32   : %s\n", WiFi.getHostname());
  Serial.printf("  Kekuatan sinyal  : %d dBm (%s)\n", rssi, kualitasSinyal(rssi));
}

void setup() {
  Serial.begin(115200);
  delay(1000);                    // beri waktu Serial Monitor tersambung
  pinMode(LED_BIRU, OUTPUT);
  digitalWrite(LED_BIRU, LOW);

  Serial.println();
  Serial.println("BAB 42 - Program 1: Hello WiFi");

  WiFi.mode(WIFI_STA);            // nyalakan WiFi sebagai Station ("tamu" di router)
  Serial.print("MAC address ESP32: ");
  Serial.println(WiFi.macAddress());

  Serial.printf("Menyambung ke \"%s\"", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);  // mulai menyambung; prosesnya berjalan di latar belakang

  // Tunggu sampai tersambung, tetapi paling lama BATAS_TUNGGU
  unsigned long mulai = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - mulai < BATAS_TUNGGU) {
    Serial.print(".");
    delay(500);
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_BIRU, HIGH);
    tampilkanInfoJaringan();
  } else {
    Serial.printf("Belum tersambung setelah %lu detik (status %d).\n",
                  BATAS_TUNGGU / 1000, (int)WiFi.status());
    Serial.println("Cek nama/password WiFi dan pastikan jaringannya 2,4 GHz.");
    Serial.println("ESP32 tetap mencoba menyambung di latar belakang...");
  }
}

void loop() {
  if (millis() - waktuLaporTerakhir >= INTERVAL_LAPOR) {
    waktuLaporTerakhir = millis();

    if (WiFi.status() == WL_CONNECTED) {
      digitalWrite(LED_BIRU, HIGH);
      int rssi = WiFi.RSSI();
      Serial.printf("Tersambung | IP %s | RSSI %d dBm (%s)\n",
                    WiFi.localIP().toString().c_str(), rssi, kualitasSinyal(rssi));
    } else {
      digitalWrite(LED_BIRU, LOW);
      Serial.printf("Tidak tersambung (status %d), menunggu sambung ulang otomatis...\n",
                    (int)WiFi.status());
    }
  }
}
