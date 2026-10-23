/*
 * BAB 43 - Program 2: Pemantau Perangkat Tersambung
 * - Hotspot memakai alamat IP sendiri (192.168.10.1) lewat softAPConfig()
 * - Event WiFi memberi tahu saat perangkat masuk, mendapat IP, dan keluar
 * - Ketik "daftar" di Serial Monitor (line ending: New Line) untuk melihat
 *   MAC, IP, dan kekuatan sinyal setiap perangkat
 */

#include <WiFi.h>
#include <esp_wifi.h>                  // esp_wifi_ap_get_sta_list()
#include <esp_wifi_ap_get_sta_list.h>  // esp_wifi_ap_get_sta_list_with_ip()

// ── Pengaturan hotspot ─────────────────────────────────────────────
const char *AP_SSID     = "Bluino-AP-43";
const char *AP_PASSWORD = "bluino123";

const IPAddress AP_IP(192, 168, 10, 1);
const IPAddress AP_GATEWAY(192, 168, 10, 1);  // sama dengan AP_IP
const IPAddress AP_SUBNET(255, 255, 255, 0);

String bufferPerintah = "";  // menampung huruf dari Serial Monitor

// Ubah 6 byte alamat MAC menjadi teks "AA:BB:CC:DD:EE:FF"
String macKeTeks(const uint8_t *mac) {
  char teks[18];
  snprintf(teks, sizeof(teks), "%02X:%02X:%02X:%02X:%02X:%02X",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(teks);
}

// Dipanggil otomatis oleh ESP32 setiap ada kejadian WiFi
void onWiFiEvent(WiFiEvent_t event, WiFiEventInfo_t info) {
  switch (event) {
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      Serial.printf("[EVENT] Perangkat masuk  : %s\n",
                    macKeTeks(info.wifi_ap_staconnected.mac).c_str());
      break;

    case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
      Serial.printf("[EVENT] IP diberikan     : %s -> %s\n",
                    macKeTeks(info.wifi_ap_staipassigned.mac).c_str(),
                    IPAddress(info.wifi_ap_staipassigned.ip.addr).toString().c_str());
      break;

    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      Serial.printf("[EVENT] Perangkat keluar : %s\n",
                    macKeTeks(info.wifi_ap_stadisconnected.mac).c_str());
      break;

    default:
      break;  // event lain diabaikan
  }
}

// Tampilkan semua perangkat yang sedang tersambung
void tampilkanDaftar() {
  wifi_sta_list_t daftarWifi;      // MAC + kekuatan sinyal
  wifi_sta_mac_ip_list_t daftarIp; // MAC + alamat IP

  if (esp_wifi_ap_get_sta_list(&daftarWifi) != ESP_OK ||
      esp_wifi_ap_get_sta_list_with_ip(&daftarWifi, &daftarIp) != ESP_OK) {
    Serial.println("Gagal membaca daftar perangkat.");
    return;
  }

  Serial.printf("Perangkat tersambung: %d\n", daftarWifi.num);
  for (int i = 0; i < daftarWifi.num; i++) {
    IPAddress ip(daftarIp.sta[i].ip.addr);
    Serial.printf("  %d. MAC %s | IP %s | sinyal %d dBm\n",
                  i + 1,
                  macKeTeks(daftarWifi.sta[i].mac).c_str(),
                  ip.toString().c_str(),
                  daftarWifi.sta[i].rssi);
  }
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("=== BAB 43 Program 2: Pemantau Perangkat Tersambung ===");

  WiFi.onEvent(onWiFiEvent);  // daftarkan event SEBELUM hotspot menyala
  WiFi.mode(WIFI_AP);

  if (!WiFi.softAPConfig(AP_IP, AP_GATEWAY, AP_SUBNET)) {
    Serial.println("Gagal mengatur alamat IP hotspot! Periksa AP_IP, AP_GATEWAY, AP_SUBNET.");
    while (true) {
      delay(1000);
    }
  }

  if (!WiFi.softAP(AP_SSID, AP_PASSWORD)) {
    Serial.println("Gagal membuat hotspot! Periksa kata sandi (minimal 8 karakter).");
    while (true) {
      delay(1000);
    }
  }

  Serial.printf("Hotspot \"%s\" aktif. IP ESP32: %s\n",
                AP_SSID, WiFi.softAPIP().toString().c_str());
  Serial.println("Ketik daftar lalu Enter untuk melihat perangkat yang tersambung.");
}

void loop() {
  // Baca perintah dari Serial Monitor tanpa menunggu (pola BAB 16)
  while (Serial.available() > 0) {
    char huruf = Serial.read();
    if (huruf == '\n') {
      bufferPerintah.trim();
      bufferPerintah.toLowerCase();
      if (bufferPerintah == "daftar") {
        tampilkanDaftar();
      } else if (bufferPerintah.length() > 0) {
        Serial.println("Perintah tidak dikenal. Ketik: daftar");
      }
      bufferPerintah = "";
    } else if (huruf != '\r' && bufferPerintah.length() < 40) {
      bufferPerintah += huruf;
    }
  }
}
