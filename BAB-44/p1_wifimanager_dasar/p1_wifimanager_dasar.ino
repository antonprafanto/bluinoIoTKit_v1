/*
 * BAB 44 - Program 1: WiFiManager Pertama
 * Saat menyala, ESP32 mencoba WiFi yang tersimpan. Kalau belum ada atau
 * gagal, ESP32 membuka hotspot "Bluino-Setup" berisi halaman pengaturan.
 * Pilih WiFi rumahmu lewat HP, dan ESP32 akan mengingatnya.
 *
 * Library  : WiFiManager by tzapu (versi 2.0.17)
 * Rangkaian: tidak perlu (LED biru bawaan board di IO2, tombol BOOT di IO0)
 * Alat uji : Serial Monitor (115200) + HP atau laptop
 * Hapus WiFi tersimpan: tahan tombol BOOT 3 detik setelah program berjalan.
 */

#include <WiFiManager.h>   // WiFiManager by tzapu (sudah ikut menyertakan WiFi.h)

#define LED_BIRU     2     // LED biru bawaan board (sama dengan LED3 di kit)
#define TOMBOL_BOOT  0     // tombol BOOT di board ESP32: ditekan = LOW

const char* PORTAL_SSID = "Bluino-Setup";  // nama hotspot portal
const char* PORTAL_PASS = "bluino123";     // password hotspot, 8 sampai 63 karakter

WiFiManager wm;

unsigned long mulaiTekanBoot = 0;      // kapan tombol BOOT mulai ditahan (0 = tidak ditahan)
bool peringatanBootTampil = false;     // supaya peringatan hanya dicetak sekali
unsigned long waktuLaporan = 0;        // kapan laporan terakhir dicetak

// Dipanggil WiFiManager tepat saat portal dibuka
void saatPortalDibuka(WiFiManager* portal) {
  Serial.println("Portal pengaturan dibuka:");
  Serial.printf("  1. Sambungkan HP ke WiFi \"%s\" (password: %s)\n",
                portal->getConfigPortalSSID().c_str(), PORTAL_PASS);
  Serial.println("  2. Tunggu halaman pengaturan muncul, atau buka http://192.168.4.1");
  Serial.println("  3. Pilih Configure WiFi, pilih WiFi rumahmu, isi password, tekan Save");
}

void tampilkanInfoWiFi() {
  Serial.println("Tersambung ke WiFi!");
  Serial.print("  Nama WiFi : ");
  Serial.println(WiFi.SSID());
  Serial.print("  Alamat IP : ");
  Serial.println(WiFi.localIP());
  Serial.print("  Sinyal    : ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");
}

// Hapus WiFi tersimpan kalau tombol BOOT ditahan 3 detik
void periksaTombolBoot() {
  if (digitalRead(TOMBOL_BOOT) == HIGH) {   // tidak ditekan (pull-up membuatnya HIGH)
    mulaiTekanBoot = 0;                     // batalkan hitungan
    peringatanBootTampil = false;
    return;
  }

  // Sampai di sini, tombol sedang ditekan (LOW)
  if (mulaiTekanBoot == 0) {
    mulaiTekanBoot = millis();              // catat awal penekanan
  }
  unsigned long lamaDitekan = millis() - mulaiTekanBoot;

  if (lamaDitekan >= 1000 && !peringatanBootTampil) {
    Serial.println("Tombol BOOT ditahan... lepaskan sekarang kalau tidak jadi menghapus.");
    peringatanBootTampil = true;
  }
  if (lamaDitekan >= 3000) {
    Serial.println("Menghapus WiFi tersimpan, lalu restart...");
    wm.resetSettings();                     // hapus SSID dan password dari NVS
    delay(500);
    ESP.restart();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);                          // beri waktu Serial Monitor tersambung
  pinMode(LED_BIRU, OUTPUT);
  digitalWrite(LED_BIRU, LOW);
  pinMode(TOMBOL_BOOT, INPUT_PULLUP);   // tombol BOOT menghubungkan IO0 ke GND

  Serial.println();
  Serial.println("BAB 44 - Program 1: WiFiManager Pertama");

  wm.setDebugOutput(false);             // matikan pesan bawaan "*wm:" supaya rapi
  wm.setTitle("Bluino IoT Kit");        // judul halaman portal
  wm.setConnectTimeout(20);             // coba sambung paling lama 20 detik
  wm.setConfigPortalTimeout(180);       // tutup portal setelah 3 menit tanpa aktivitas
  wm.setAPCallback(saatPortalDibuka);   // cetak petunjuk saat portal dibuka

  Serial.println("Mencoba WiFi tersimpan (paling lama 20 detik)...");

  // Blocking: baru kembali setelah tersambung, atau setelah portal ditutup
  bool tersambung = wm.autoConnect(PORTAL_SSID, PORTAL_PASS);

  if (!tersambung) {
    Serial.println("Portal ditutup tanpa pengaturan. Restart dalam 3 detik...");
    delay(3000);
    ESP.restart();                      // coba lagi dari awal
  }

  tampilkanInfoWiFi();
  Serial.println("Tahan tombol BOOT 3 detik untuk menghapus WiFi tersimpan.");
}

void loop() {
  periksaTombolBoot();

  // LED biru menyala selama WiFi tersambung
  digitalWrite(LED_BIRU, WiFi.status() == WL_CONNECTED ? HIGH : LOW);

  if (millis() - waktuLaporan >= 10000) {   // laporan setiap 10 detik
    waktuLaporan = millis();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("Masih tersambung, sinyal %d dBm\n", WiFi.RSSI());
    } else {
      Serial.println("WiFi terputus, ESP32 mencoba menyambung lagi...");
    }
  }
}
