/*
 * BAB 43 - Program 1: Hotspot Pertama
 * ESP32 memancarkan WiFi sendiri (mode Access Point).
 * Sambungkan HP/laptop ke "Bluino-AP-43" dengan kata sandi "bluino123",
 * lalu lihat jumlah perangkat yang tersambung di Serial Monitor.
 */

#include <WiFi.h>

// ── Pengaturan hotspot ─────────────────────────────────────────────
const char *AP_SSID     = "Bluino-AP-43";  // nama WiFi yang muncul di HP
const char *AP_PASSWORD = "bluino123";     // minimal 8 karakter!

// ── Laporan berkala tanpa delay() (pola BAB 16) ────────────────────
const unsigned long INTERVAL_LAPORAN = 5000;  // 5 detik
unsigned long waktuLaporanTerakhir = 0;

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("=== BAB 43 Program 1: Hotspot Pertama ===");

  WiFi.mode(WIFI_AP);  // jadikan ESP32 access point

  // Nyalakan hotspot. Hasilnya true kalau berhasil.
  if (!WiFi.softAP(AP_SSID, AP_PASSWORD)) {
    Serial.println("Gagal membuat hotspot! Periksa kata sandi (minimal 8 karakter).");
    while (true) {
      delay(1000);  // berhenti di sini
    }
  }

  Serial.println("Hotspot aktif!");
  Serial.print("Nama WiFi (SSID) : ");
  Serial.println(WiFi.softAPSSID());
  Serial.print("Kata sandi       : ");
  Serial.println(AP_PASSWORD);
  Serial.print("Alamat IP ESP32  : ");
  Serial.println(WiFi.softAPIP());
  Serial.print("MAC hotspot      : ");
  Serial.println(WiFi.softAPmacAddress());
  Serial.println("Sambungkan HP/laptop ke WiFi di atas.");
}

void loop() {
  unsigned long sekarang = millis();
  if (sekarang - waktuLaporanTerakhir >= INTERVAL_LAPORAN) {
    waktuLaporanTerakhir = sekarang;
    Serial.print("Perangkat tersambung: ");
    Serial.println(WiFi.softAPgetStationNum());
  }
}
