/*
 * BAB 45 - Program 1: Memberi Nama ESP32 dengan mDNS
 * ESP32 terhubung ke WiFi, lalu mengumumkan nama "bluino.local".
 * Uji dari laptop di WiFi yang sama: ping bluino.local
 *
 * Rangkaian: tidak perlu (LED biru bawaan board di IO2).
 *   Opsional: LED3 (biru) → IO2 supaya LED biru di kit ikut menyala.
 */

#include <WiFi.h>
#include <ESPmDNS.h>

const char* WIFI_SSID     = "NamaWiFiKamu";      // ← ganti dengan WiFi kamu
const char* WIFI_PASSWORD = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu

// Nama mDNS TANPA ".local": huruf kecil, angka, tanda hubung (maks. 31 karakter)
const char* NAMA_MDNS = "bluino";

#define LED_BIRU 2  // LED biru bawaan board (dan LED3 kit) → IO2

bool mdnsAktif = false;
unsigned long waktuLaporan = 0;

// Nama yang benar-benar dipakai mDNS saat ini. Bisa berbeda dari NAMA_MDNS
// kalau nama itu sudah dipakai perangkat lain (misalnya menjadi "bluino-2").
String namaMdnsAktif() {
  char nama[MDNS_NAME_BUF_LEN];
  if (mdns_hostname_get(nama) == ESP_OK) {
    return String(nama);
  }
  return String("");  // mDNS belum berjalan
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_BIRU, OUTPUT);
  digitalWrite(LED_BIRU, LOW);
  delay(500);
  Serial.println();
  Serial.println("BAB 45 - Program 1: Memberi Nama ESP32 dengan mDNS");

  // 1. Sambungkan ke WiFi
  WiFi.setHostname(NAMA_MDNS);  // nama di daftar perangkat router — SEBELUM WiFi.mode()
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

  // 2. Mulai mDNS (setelah WiFi terhubung)
  mdnsAktif = MDNS.begin(NAMA_MDNS);
  if (mdnsAktif) {
    Serial.printf("mDNS aktif: %s.local\n", NAMA_MDNS);
    Serial.printf("Uji dari laptop di WiFi yang sama: ping %s.local\n", NAMA_MDNS);
  } else {
    Serial.println("Gagal memulai mDNS! Periksa isi NAMA_MDNS.");
  }
}

void loop() {
  // Tidak perlu MDNS.update(): mDNS berjalan sendiri di latar belakang dan
  // otomatis mengumumkan alamat IP baru setelah WiFi tersambung ulang.

  bool wifiTerhubung = (WiFi.status() == WL_CONNECTED);
  digitalWrite(LED_BIRU, (wifiTerhubung && mdnsAktif) ? HIGH : LOW);  // menyala = siap

  // Laporan status setiap 10 detik (tanpa delay, lihat BAB 16)
  if (millis() - waktuLaporan >= 10000) {
    waktuLaporan = millis();
    if (!wifiTerhubung) {
      Serial.println("WiFi terputus, menunggu tersambung ulang...");
    } else if (mdnsAktif) {
      Serial.printf("Nama: %s.local | IP: %s\n",
                    namaMdnsAktif().c_str(), WiFi.localIP().toString().c_str());
    } else {
      Serial.printf("mDNS tidak aktif | IP: %s\n", WiFi.localIP().toString().c_str());
    }
  }
}
