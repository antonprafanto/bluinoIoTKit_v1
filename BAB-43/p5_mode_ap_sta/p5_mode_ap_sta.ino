/*
 * BAB 43 - Program 5: Mode AP+STA
 * ESP32 menjadi hotspot "Bluino-AP-STA" (192.168.50.1)
 * sekaligus tersambung ke WiFi rumah/kampus.
 * Perhatikan: channel hotspot ikut pindah ke channel router.
 */

#include <WiFi.h>

// ── Sisi AP: hotspot ESP32 ─────────────────────────────────────────
const char *AP_SSID     = "Bluino-AP-STA";
const char *AP_PASSWORD = "bluino123";
const IPAddress AP_IP(192, 168, 50, 1);       // beda dari jaringan router
const IPAddress AP_SUBNET(255, 255, 255, 0);

// ── Sisi STA: WiFi rumah/kampus (harus 2,4 GHz) ────────────────────
const char *WIFI_SSID     = "NamaWiFiKamu";      // ← ganti dengan WiFi kamu
const char *WIFI_PASSWORD = "PasswordWiFiKamu";  // ← ganti dengan WiFi kamu

const unsigned long INTERVAL_LAPORAN = 10000;  // 10 detik
unsigned long waktuLaporanTerakhir = 0;
bool routerTersambungTadi = false;  // untuk mendeteksi perubahan

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("=== BAB 43 Program 5: Mode AP+STA ===");

  WiFi.mode(WIFI_AP_STA);  // hotspot + tamu sekaligus

  // Sisi AP
  if (!WiFi.softAPConfig(AP_IP, AP_IP, AP_SUBNET)) {
    Serial.println("[AP]  Gagal mengatur alamat IP hotspot!");
  }
  if (!WiFi.softAP(AP_SSID, AP_PASSWORD, 1)) {  // minta channel 1
    Serial.println("[AP]  Gagal membuat hotspot! Periksa kata sandi (minimal 8 karakter).");
    while (true) {
      delay(1000);
    }
  }
  Serial.printf("[AP]  Hotspot %s aktif di %s (channel %d)\n",
                AP_SSID, WiFi.softAPIP().toString().c_str(), (int)WiFi.channel());

  // Sisi STA: mulai menyambung, tanpa menunggu di sini
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.printf("[STA] Menyambung ke router \"%s\"...\n", WIFI_SSID);
}

void loop() {
  // Laporkan setiap kali status sambungan ke router berubah
  bool routerTersambung = (WiFi.status() == WL_CONNECTED);
  if (routerTersambung != routerTersambungTadi) {
    routerTersambungTadi = routerTersambung;
    if (routerTersambung) {
      Serial.printf("[STA] Tersambung ke router. IP dari router: %s\n",
                    WiFi.localIP().toString().c_str());
      Serial.printf("[STA] Channel router: %d -> hotspot ikut memakai channel ini\n",
                    (int)WiFi.channel());
    } else {
      Serial.println("[STA] Terputus dari router. ESP32 mencoba lagi otomatis...");
    }
  }

  // Laporan berkala
  unsigned long sekarang = millis();
  if (sekarang - waktuLaporanTerakhir >= INTERVAL_LAPORAN) {
    waktuLaporanTerakhir = sekarang;
    Serial.printf("[Status] Perangkat di hotspot: %d | Router: %s | Channel: %d\n",
                  WiFi.softAPgetStationNum(),
                  routerTersambung ? WiFi.localIP().toString().c_str() : "belum tersambung",
                  (int)WiFi.channel());
  }
}
