/*
 * BAB 42 - Program 3: Pemindai Jaringan WiFi
 * Memindai jaringan WiFi 2,4 GHz di sekitar tanpa memblokir loop(),
 * lalu menampilkan nama, kekuatan sinyal, kanal, dan jenis keamanannya.
 * Jaringan yang namanya sama dengan WIFI_SSID diberi tanda.
 *
 * Rangkaian: tidak perlu. LED biru bawaan board (IO2) menyala saat memindai.
 * Alat uji : Serial Monitor, baud 115200.
 */

#include <WiFi.h>

// Nama WiFi yang ingin dicari di daftar (password tidak diperlukan)
const char *WIFI_SSID = "NamaWiFiKamu";   // ← ganti dengan WiFi kamu

#define LED_BIRU 2   // LED biru bawaan board

const unsigned long INTERVAL_PINDAI = 20000;  // jeda antarpemindaian: 20 detik
const unsigned long INTERVAL_TITIK  = 250;    // titik "sedang memindai" setiap 250 ms

bool sedangMemindai = false;
unsigned long waktuSelesaiPindai = 0;
unsigned long waktuTitik = 0;

const char *namaKeamanan(wifi_auth_mode_t mode) {
  switch (mode) {
    case WIFI_AUTH_OPEN:            return "Terbuka";
    case WIFI_AUTH_WEP:             return "WEP";
    case WIFI_AUTH_WPA_PSK:         return "WPA";
    case WIFI_AUTH_WPA2_PSK:        return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Ent";
    case WIFI_AUTH_WPA3_PSK:        return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK:   return "WPA2/WPA3";
    default:                        return "Lainnya";
  }
}

// Batang sinyal sederhana berdasarkan tabel RSSI di 42.4.3
const char *batangSinyal(long rssi) {
  if (rssi >= -50) return "[####]";
  if (rssi >= -60) return "[### ]";
  if (rssi >= -70) return "[##  ]";
  if (rssi >= -80) return "[#   ]";
  return "[    ]";
}

void mulaiPindai() {
  Serial.println();
  Serial.print("Memindai jaringan WiFi");
  digitalWrite(LED_BIRU, HIGH);
  WiFi.scanNetworks(true);   // true = asinkron: langsung kembali, hasilnya diperiksa di loop()
  sedangMemindai = true;
}

void tampilkanHasil(int jumlah) {
  bool wifiKamuTerlihat = false;

  Serial.printf("Ditemukan %d jaringan (urut dari sinyal terkuat):\n", jumlah);
  Serial.printf(" No  %-24s  RSSI  Sinyal  Kanal  Keamanan\n", "Nama (SSID)");

  for (int i = 0; i < jumlah; i++) {
    String nama = WiFi.SSID(i);
    long rssi = WiFi.RSSI(i);
    bool cocok = (nama == WIFI_SSID);   // perbandingan peka huruf besar-kecil
    if (cocok) {
      wifiKamuTerlihat = true;
    }
    Serial.printf(" %2d  %-24.24s  %4ld  %s  %5ld  %s%s\n",
                  i + 1, nama.c_str(), rssi, batangSinyal(rssi),
                  (long)WiFi.channel(i), namaKeamanan(WiFi.encryptionType(i)),
                  cocok ? "  <- WiFi kamu" : "");
  }

  if (wifiKamuTerlihat) {
    Serial.printf("WiFi \"%s\" terlihat. ESP32 bisa mencoba tersambung ke sana.\n", WIFI_SSID);
  } else {
    Serial.printf("WiFi \"%s\" TIDAK terlihat. Cek huruf besar-kecil, pita 2,4 GHz, dan jarak.\n",
                  WIFI_SSID);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(LED_BIRU, OUTPUT);
  digitalWrite(LED_BIRU, LOW);

  Serial.println();
  Serial.println("BAB 42 - Program 3: Pemindai Jaringan WiFi");

  WiFi.mode(WIFI_STA);   // pemindaian dilakukan dalam mode Station
  mulaiPindai();
}

void loop() {
  unsigned long sekarang = millis();

  if (sedangMemindai) {
    int hasil = WiFi.scanComplete();

    if (hasil == WIFI_SCAN_RUNNING) {
      // Belum selesai. loop() tetap bebas: di sini cukup mencetak titik.
      if (sekarang - waktuTitik >= INTERVAL_TITIK) {
        waktuTitik = sekarang;
        Serial.print(".");
      }
    } else {
      Serial.println();
      digitalWrite(LED_BIRU, LOW);
      sedangMemindai = false;
      waktuSelesaiPindai = sekarang;

      if (hasil == WIFI_SCAN_FAILED) {
        Serial.println("Pemindaian gagal. Dicoba lagi nanti.");
      } else if (hasil == 0) {
        Serial.println("Tidak ada jaringan 2,4 GHz yang terlihat.");
      } else {
        tampilkanHasil(hasil);
      }
      WiFi.scanDelete();   // bebaskan memori hasil pindai
      Serial.printf("Pindai ulang dalam %lu detik...\n", INTERVAL_PINDAI / 1000);
    }
  } else if (sekarang - waktuSelesaiPindai >= INTERVAL_PINDAI) {
    mulaiPindai();
  }
}
