/*
 * BAB 44 - Program 3: Isian Tambahan dan Portal Sesuai Permintaan
 * Portal WiFiManager mendapat 3 isian tambahan: nama perangkat, server MQTT,
 * dan port MQTT. Isian disimpan ke NVS dengan Preferences.
 * Tombol S1 membuka portal kapan saja untuk mengubah pengaturan.
 *
 * Library  : WiFiManager by tzapu (versi 2.0.17)
 * Rangkaian: SW1 (tombol S1) -> IO15
 * Tombol   : S1 = buka portal
 *            BOOT ditahan 3 detik = kembali ke pengaturan pabrik
 * Alat uji : Serial Monitor (115200) + HP atau laptop
 */

#include <WiFiManager.h>
#include <Preferences.h>

#define LED_BIRU     2     // LED biru bawaan board (sama dengan LED3 di kit)
#define TOMBOL_BOOT  0     // tombol BOOT di board ESP32: ditekan = LOW
#define TOMBOL_S1    15    // SW1 di kit: ditekan = HIGH (pull-down 10 kΩ di PCB)

const char* PORTAL_SSID = "Bluino-Setup";
const char* PORTAL_PASS = "bluino123";
const char* MENU_PORTAL[] = {"wifi", "info", "exit"};

// Pengaturan pabrik (dipakai kalau NVS masih kosong)
const char* NAMA_BAWAAN   = "bluino-01";
const char* SERVER_BAWAAN = "broker.hivemq.com";
const uint16_t PORT_BAWAAN = 1883;

const unsigned long WAKTU_DEBOUNCE = 50;   // ms

// Pengaturan yang sedang dipakai (salinan di RAM)
String namaPerangkat;
String serverMqtt;
uint16_t portMqtt;

Preferences prefs;
WiFiManager wm;

// Isian tambahan di portal: id, label, nilai awal, panjang maksimal
WiFiManagerParameter isianNama("nama", "Nama perangkat (huruf, angka, tanda -)", "", 32);
WiFiManagerParameter isianServer("server", "Server MQTT", "", 40);
WiFiManagerParameter isianPort("port", "Port MQTT (1-65535)", "", 5);

bool portalTadiAktif = false;
unsigned long mulaiTekanBoot = 0;
bool peringatanBootTampil = false;
unsigned long waktuLaporan = 0;

// Data debounce tombol S1 (pola dari BAB 9)
int bacaanS1Sebelumnya = LOW;
int statusS1Stabil = LOW;
unsigned long waktuS1Berubah = 0;

// ---------- Pengaturan tambahan (NVS) ----------

void muatPengaturan() {
  prefs.begin("bab44", false);   // false = baca-tulis (namespace dibuat kalau belum ada)
  namaPerangkat = prefs.getString("nama", NAMA_BAWAAN);
  serverMqtt    = prefs.getString("server", SERVER_BAWAAN);
  portMqtt      = prefs.getUShort("port", PORT_BAWAAN);
  prefs.end();
}

void tampilkanPengaturan() {
  Serial.println("  Nama perangkat : " + namaPerangkat);
  Serial.println("  Server MQTT    : " + serverMqtt);
  Serial.println("  Port MQTT      : " + String(portMqtt));
}

// Salin pengaturan terbaru ke kotak isian portal
void isiKotakPortal() {
  isianNama.setValue(namaPerangkat.c_str(), 32);
  isianServer.setValue(serverMqtt.c_str(), 40);
  isianPort.setValue(String(portMqtt).c_str(), 5);
}

// Hanya huruf, angka, dan tanda - . _ yang diterima (tidak boleh kosong)
bool teksAman(const String& teks) {
  if (teks.length() == 0) return false;
  for (size_t i = 0; i < teks.length(); i++) {
    char c = teks[i];
    if (!isalnum((unsigned char)c) && c != '-' && c != '.' && c != '_') return false;
  }
  return true;
}

// Callback: dipanggil WiFiManager saat tombol Save di portal ditekan
void simpanParameter() {
  String nama   = isianNama.getValue();
  String server = isianServer.getValue();
  long port     = String(isianPort.getValue()).toInt();   // teks -> angka (0 kalau bukan angka)
  nama.trim();
  server.trim();

  if (teksAman(nama)) {
    namaPerangkat = nama;
  } else {
    Serial.println("Nama perangkat tidak valid, nilai lama tetap dipakai.");
  }
  if (teksAman(server)) {
    serverMqtt = server;
  } else {
    Serial.println("Server MQTT tidak valid, nilai lama tetap dipakai.");
  }
  if (port >= 1 && port <= 65535) {
    portMqtt = port;
  } else {
    Serial.println("Port MQTT tidak valid, nilai lama tetap dipakai.");
  }

  prefs.begin("bab44", false);
  prefs.putString("nama", namaPerangkat);
  prefs.putString("server", serverMqtt);
  prefs.putUShort("port", portMqtt);
  prefs.end();

  Serial.println("Pengaturan tambahan disimpan ke NVS:");
  tampilkanPengaturan();
  isiKotakPortal();   // kalau portal dibuka lagi, kotak berisi nilai yang benar
}

// ---------- WiFi dan portal ----------

void saatPortalDibuka(WiFiManager* portal) {
  Serial.println("Portal pengaturan dibuka:");
  Serial.printf("  1. Sambungkan HP ke WiFi \"%s\" (password: %s)\n",
                portal->getConfigPortalSSID().c_str(), PORTAL_PASS);
  Serial.println("  2. Tunggu halaman pengaturan muncul, atau buka http://192.168.4.1");
  Serial.println("  3. Pilih Configure WiFi, isi WiFi dan isian tambahan, tekan Save");
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

void aturLed() {
  unsigned long t = millis();
  if (wm.getConfigPortalActive()) {
    digitalWrite(LED_BIRU, (t / 100) % 2);    // kedip cepat: portal terbuka
  } else if (WiFi.status() == WL_CONNECTED) {
    digitalWrite(LED_BIRU, HIGH);             // menyala: tersambung
  } else {
    digitalWrite(LED_BIRU, (t / 1000) % 2);   // kedip lambat: terputus
  }
}

// ---------- Tombol ----------

// true sekali setiap kali S1 baru ditekan (debounce + edge detection)
bool s1BaruDitekan() {
  int bacaan = digitalRead(TOMBOL_S1);
  if (bacaan != bacaanS1Sebelumnya) {
    waktuS1Berubah = millis();
    bacaanS1Sebelumnya = bacaan;
  }
  if ((millis() - waktuS1Berubah) > WAKTU_DEBOUNCE && bacaan != statusS1Stabil) {
    statusS1Stabil = bacaan;
    return statusS1Stabil == HIGH;
  }
  return false;
}

// Tahan BOOT 3 detik: hapus WiFi dan isian tambahan (pengaturan pabrik)
void periksaTombolBoot() {
  if (digitalRead(TOMBOL_BOOT) == HIGH) {
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
    Serial.println("Kembali ke pengaturan pabrik, lalu restart...");
    prefs.begin("bab44", false);
    prefs.clear();                // hapus isian tambahan
    prefs.end();
    wm.resetSettings();           // hapus WiFi tersimpan
    delay(500);
    ESP.restart();
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(LED_BIRU, OUTPUT);
  digitalWrite(LED_BIRU, LOW);
  pinMode(TOMBOL_BOOT, INPUT_PULLUP);
  pinMode(TOMBOL_S1, INPUT);      // pull-down sudah ada di PCB kit

  Serial.println();
  Serial.println("BAB 44 - Program 3: Isian Tambahan dan Portal Sesuai Permintaan");

  muatPengaturan();
  Serial.println("Pengaturan tambahan:");
  tampilkanPengaturan();

  isiKotakPortal();
  wm.addParameter(&isianNama);
  wm.addParameter(&isianServer);
  wm.addParameter(&isianPort);
  wm.setSaveParamsCallback(simpanParameter);

  wm.setDebugOutput(false);
  wm.setTitle("Bluino IoT Kit");
  wm.setMenu(MENU_PORTAL, 3);
  wm.setShowInfoUpdate(false);
  wm.setConnectTimeout(20);
  wm.setConfigPortalTimeout(180);
  wm.setBreakAfterConfig(true);
  wm.setConfigPortalBlocking(false);
  wm.setAPCallback(saatPortalDibuka);

  Serial.println("Tombol: S1 = buka portal, BOOT ditahan 3 detik = pengaturan pabrik.");
  Serial.println("Mencoba WiFi tersimpan (paling lama 20 detik)...");
  if (wm.autoConnect(PORTAL_SSID, PORTAL_PASS)) {
    tampilkanInfoWiFi();
  }
  portalTadiAktif = wm.getConfigPortalActive();
}

void loop() {
  if (wm.process()) {
    Serial.println("WiFi baru tersimpan.");
    tampilkanInfoWiFi();
  }

  // Portal baru saja tertutup?
  bool portalAktif = wm.getConfigPortalActive();
  if (portalTadiAktif && !portalAktif) {
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Portal ditutup.");
    } else {
      Serial.println("Portal ditutup tanpa WiFi yang berhasil. Restart untuk mencoba lagi...");
      delay(1000);
      ESP.restart();
    }
  }
  portalTadiAktif = portalAktif;

  // Tombol S1: buka portal sesuai permintaan (kalau belum terbuka)
  if (s1BaruDitekan() && !portalAktif) {
    Serial.println("Tombol S1 ditekan: membuka portal pengaturan...");
    isiKotakPortal();
    wm.startConfigPortal(PORTAL_SSID, PORTAL_PASS);   // non-blocking: langsung kembali
  }

  aturLed();
  periksaTombolBoot();

  if (millis() - waktuLaporan >= 10000) {   // laporan setiap 10 detik
    waktuLaporan = millis();
    Serial.print("[" + namaPerangkat + "] ");
    if (wm.getConfigPortalActive()) {
      Serial.println("portal terbuka, menunggu pengaturan");
    } else if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("tersambung ke %s, MQTT %s:%u\n",
                    WiFi.SSID().c_str(), serverMqtt.c_str(), (unsigned int)portMqtt);
    } else {
      Serial.println("WiFi terputus, mencoba menyambung lagi...");
    }
  }
}
