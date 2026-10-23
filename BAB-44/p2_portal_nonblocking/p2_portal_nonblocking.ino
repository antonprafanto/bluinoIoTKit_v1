/*
 * BAB 44 - Program 2: Portal Non-Blocking dengan LED Status
 * Portal WiFiManager berjalan "di latar belakang": loop() tetap berputar,
 * sehingga LED status berkedip, tombol BOOT tetap dibaca, dan laporan
 * tetap dicetak selama portal terbuka.
 *
 * LED biru (IO2):
 *   kedip cepat  = portal terbuka, menunggu pengaturan
 *   menyala      = tersambung ke WiFi
 *   kedip lambat = WiFi terputus, ESP32 mencoba menyambung lagi
 *
 * Library  : WiFiManager by tzapu (versi 2.0.17)
 * Rangkaian: tidak perlu (LED biru bawaan di IO2, tombol BOOT di IO0)
 * Alat uji : Serial Monitor (115200) + HP atau laptop
 */

#include <WiFiManager.h>

#define LED_BIRU     2     // LED biru bawaan board (sama dengan LED3 di kit)
#define TOMBOL_BOOT  0     // tombol BOOT di board ESP32: ditekan = LOW

const char* PORTAL_SSID = "Bluino-Setup";
const char* PORTAL_PASS = "bluino123";

// Tombol di halaman utama portal: hanya "Configure WiFi" dan "Info"
const char* MENU_PORTAL[] = {"wifi", "info"};

WiFiManager wm;

bool portalTadiAktif = false;        // status portal pada putaran loop() sebelumnya
unsigned long mulaiTekanBoot = 0;
bool peringatanBootTampil = false;
unsigned long waktuLaporan = 0;

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

// Pola LED biru sesuai status (tanpa delay)
void aturLed() {
  unsigned long t = millis();
  if (wm.getConfigPortalActive()) {
    digitalWrite(LED_BIRU, (t / 100) % 2);    // berganti setiap 100 ms: kedip cepat
  } else if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_BIRU, HIGH);             // menyala terus
  } else {
    digitalWrite(LED_BIRU, (t / 1000) % 2);   // berganti setiap 1 detik: kedip lambat
  }
}

// Hapus WiFi tersimpan kalau tombol BOOT ditahan 3 detik
void periksaTombolBoot() {
  if (digitalRead(TOMBOL_BOOT) == HIGH) {     // tidak ditekan
    mulaiTekanBoot = 0;
    peringatanBootTampil = false;
    return;
  }
  if (mulaiTekanBoot == 0) {
    mulaiTekanBoot = millis();
  }
  unsigned long lamaDitekan = millis() - mulaiTekanBoot;
  if (lamaDitekan >= 1000 && !peringatanBootTampil) {
    Serial.println("Tombol BOOT ditahan... lepaskan sekarang kalau tidak jadi menghapus.");
    peringatanBootTampil = true;
  }
  if (lamaDitekan >= 3000) {
    Serial.println("Menghapus WiFi tersimpan, lalu restart...");
    wm.resetSettings();
    delay(500);
    ESP.restart();
  }
}

void cetakLaporan() {
  Serial.printf("[%lu s] ", millis() / 1000);
  if (wm.getConfigPortalActive()) {
    Serial.printf("Portal terbuka, %d perangkat tersambung ke hotspot\n",
                  WiFi.softAPgetStationNum());
  } else if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Tersambung ke %s, sinyal %d dBm\n",
                  WiFi.SSID().c_str(), WiFi.RSSI());
  } else {
    Serial.println("WiFi terputus, mencoba menyambung lagi...");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(LED_BIRU, OUTPUT);
  digitalWrite(LED_BIRU, LOW);
  pinMode(TOMBOL_BOOT, INPUT_PULLUP);

  Serial.println();
  Serial.println("BAB 44 - Program 2: Portal Non-Blocking dengan LED Status");

  wm.setDebugOutput(false);
  wm.setTitle("Bluino IoT Kit");
  wm.setMenu(MENU_PORTAL, 2);          // tampilkan 2 tombol menu saja
  wm.setShowInfoUpdate(false);         // sembunyikan tombol Update di halaman Info
  wm.setConnectTimeout(20);            // coba sambung paling lama 20 detik
  wm.setConfigPortalTimeout(180);      // tutup portal setelah 3 menit tanpa aktivitas
  wm.setBreakAfterConfig(true);        // WiFi baru gagal disambungi? tutup portal
  wm.setConfigPortalBlocking(false);   // portal non-blocking
  wm.setAPCallback(saatPortalDibuka);

  Serial.println("Mencoba WiFi tersimpan (paling lama 20 detik)...");
  if (wm.autoConnect(PORTAL_SSID, PORTAL_PASS)) {
    tampilkanInfoWiFi();
  }
  // Kalau gagal, autoConnect() langsung kembali (false) dan portal tetap
  // terbuka. Portal dilayani oleh wm.process() di loop().
  portalTadiAktif = wm.getConfigPortalActive();
}

void loop() {
  // 1. Layani portal. Hasilnya true sekali, saat WiFi baru berhasil tersambung.
  if (wm.process()) {
    Serial.println("WiFi baru tersimpan.");
    tampilkanInfoWiFi();
  }

  // 2. Portal baru saja tertutup, tetapi WiFi belum tersambung?
  //    (batas waktu habis, atau WiFi yang diisi gagal disambungi)
  bool portalAktif = wm.getConfigPortalActive();
  if (portalTadiAktif && !portalAktif && WiFi.status() != WL_CONNECTED) {
    Serial.println("Portal ditutup tanpa WiFi yang berhasil. Restart untuk mencoba lagi...");
    delay(1000);
    ESP.restart();
  }
  portalTadiAktif = portalAktif;

  // 3. Tugas lain tetap berjalan, portal terbuka atau tidak
  aturLed();
  periksaTombolBoot();
  if (millis() - waktuLaporan >= 5000) {   // laporan setiap 5 detik
    waktuLaporan = millis();
    cetakLaporan();
  }
}
