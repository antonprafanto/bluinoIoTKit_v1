/*
 * BAB 56 - Program 1: Lampu Matter (LED Merah + Relay)
 * ESP32 menjadi perangkat Matter jenis "On/Off Light" lewat WiFi.
 * Setelah didaftarkan (commissioning), LED merah dan relay bisa
 * dikendalikan dari Apple Home, Google Home, Amazon Alexa, atau
 * Home Assistant.
 *
 * Rangkaian: LED1 (merah) -> IO4, RELAY -> IO26, SW1 -> IO15
 * Board    : ESP32 Dev Module
 *            Tools -> Partition Scheme -> Huge APP (3MB No OTA/1MB SPIFFS)
 * Library  : Matter, WiFi, Preferences (bawaan core esp32 3.3.x)
 *
 * Tombol S1: tekan sebentar -> lampu ganti nyala/mati
 *            tahan 5 detik  -> hapus pendaftaran Matter (decommission)
 * Relay dicoba tanpa beban. Jangan pernah menyambungkan listrik PLN 220 V!
 */

#include <WiFi.h>
#include <Matter.h>
#include <Preferences.h>

const char *NAMA_WIFI = "NamaWiFiKamu";          // ← ganti dengan WiFi kamu
const char *PASSWORD_WIFI = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu

#define PIN_LED_MERAH 4   // LED1 (merah), aktif HIGH
#define PIN_RELAY 26      // relay, aktif HIGH
#define PIN_TOMBOL_S1 15  // SW1, ditekan = HIGH (pull-down 10 kOhm di PCB)

// Jendela pendaftaran Matter tertutup 15 menit setelah menyala,
// jadi ESP32 di-restart kalau 10 menit belum didaftarkan
const unsigned long BATAS_TUNGGU_DAFTAR = 10UL * 60UL * 1000UL;

MatterOnOffLight lampu;  // endpoint Matter jenis On/Off Light
MatterButton tombolS1;   // pembaca tombol dari library Matter
Preferences memori;      // menyimpan keadaan lampu terakhir di NVS

bool sudahTerdaftar = false;     // status pendaftaran yang terakhir dilihat
bool sedangReset = false;        // true setelah Matter.decommission()
unsigned long waktuReset = 0;

// Dipanggil setiap kali keadaan lampu berubah: dari aplikasi,
// dari tombol S1, atau dari updateAccessory() saat menyala
bool ubahLampu(bool nyala) {
  digitalWrite(PIN_LED_MERAH, nyala ? HIGH : LOW);
  digitalWrite(PIN_RELAY, nyala ? HIGH : LOW);
  memori.putBool("nyala", nyala);  // ingat keadaan terakhir
  Serial.printf("[LAMPU] %s\n", nyala ? "NYALA" : "MATI");
  return true;  // true = perubahan diterima
}

void sambungWiFi() {
  Serial.printf("[WiFi] Menyambung ke %s", NAMA_WIFI);
  WiFi.mode(WIFI_STA);
  WiFi.enableIPv6(true);  // Matter berjalan di atas IPv6
  WiFi.begin(NAMA_WIFI, PASSWORD_WIFI);
  unsigned long mulai = millis();
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (millis() - mulai >= 20000) {
      Serial.println();
      Serial.println("[WiFi] Belum tersambung. Cek nama/password dan pastikan WiFi 2,4 GHz.");
      mulai = millis();
    }
  }
  Serial.println();
  Serial.printf("[WiFi] Tersambung, IP: %s\n", WiFi.localIP().toString().c_str());
}

void tampilkanKodePairing() {
  Serial.println();
  Serial.println("[MATTER] Belum terdaftar. Daftarkan dari aplikasi rumah pintar:");
  Serial.printf("[MATTER] Kode pairing manual: %s\n", Matter.getManualPairingCode().c_str());
  Serial.println("[MATTER] Buka URL ini di browser untuk melihat kode QR:");
  Serial.println(Matter.getOnboardingQRCodeUrl());
  Serial.println();
}

// Memantau perubahan status pendaftaran Matter
void cekStatusMatter() {
  if (sedangReset) {
    // Matter.decommission() akan me-restart ESP32 sendiri.
    // Kalau 10 detik belum restart juga, restart manual.
    if (millis() - waktuReset >= 10000) {
      ESP.restart();
    }
    return;
  }

  bool terdaftar = Matter.isDeviceCommissioned();
  if (terdaftar && !sudahTerdaftar) {
    Serial.println("[MATTER] Berhasil didaftarkan ke aplikasi rumah pintar!");
  } else if (!terdaftar && sudahTerdaftar) {
    Serial.println("[MATTER] Perangkat dihapus dari aplikasi. Restart...");
    delay(500);
    ESP.restart();  // setelah restart, siap didaftarkan lagi
  } else if (!terdaftar && millis() >= BATAS_TUNGGU_DAFTAR) {
    Serial.println("[MATTER] 10 menit belum didaftarkan. Restart supaya jendela pendaftaran terbuka lagi...");
    delay(500);
    ESP.restart();
  }
  sudahTerdaftar = terdaftar;
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("=== BAB 56 - Program 1: Lampu Matter ===");

  pinMode(PIN_LED_MERAH, OUTPUT);
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_LED_MERAH, LOW);
  digitalWrite(PIN_RELAY, LOW);

  tombolS1.begin(PIN_TOMBOL_S1, INPUT);  // kit sudah punya pull-down
  tombolS1.setActiveLow(false);          // ditekan = HIGH

  sambungWiFi();

  memori.begin("lampuMatter", false);
  bool keadaanAwal = memori.getBool("nyala", false);

  lampu.begin(keadaanAwal);   // 1) daftarkan endpoint lampu
  lampu.onChange(ubahLampu);  // 2) pasang fungsi pengubah LED/relay
  Matter.begin();             // 3) nyalakan Matter (selalu terakhir)
  lampu.updateAccessory();    // samakan LED/relay dengan keadaan awal

  sudahTerdaftar = Matter.isDeviceCommissioned();
  if (sudahTerdaftar) {
    Serial.println("[MATTER] Sudah terdaftar. Kendalikan lampu dari aplikasi.");
  } else {
    tampilkanKodePairing();
  }
}

void loop() {
  cekStatusMatter();

  matterButtonEvent_t kejadian;
  while ((kejadian = tombolS1.poll()) != MATTER_BUTTON_NONE) {
    if (kejadian == MATTER_BUTTON_CLICK && !sedangReset) {
      Serial.println("[TOMBOL] S1 ditekan -> balik keadaan lampu");
      lampu.toggle();  // aplikasi ikut melihat perubahannya
    } else if (kejadian == MATTER_BUTTON_LONG_HOLD && !sedangReset) {
      Serial.println("[TOMBOL] S1 ditahan 5 detik -> hapus pendaftaran Matter");
      lampu.setOnOff(false);
      sedangReset = true;
      waktuReset = millis();
      Matter.decommission();  // hapus data Matter, lalu ESP32 restart
    }
  }
  delay(10);
}
