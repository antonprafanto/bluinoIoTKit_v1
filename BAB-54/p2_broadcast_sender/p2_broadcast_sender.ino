/*
 * BAB 54 - Program 2 (board A): Broadcast Suhu DHT11
 *
 * Setiap 3 detik board A membaca DHT11, lalu menyiarkan (broadcast) suhu
 * dan kelembapan ke SEMUA board ESP-NOW di kanal yang sama.
 * Tidak perlu tahu MAC penerima.
 *
 * Rangkaian board A: DHT (header hijau, pin ke-3 dari atas) -> IO27
 * Library: DHT sensor library by Adafruit (+ Adafruit Unified Sensor)
 * Alat uji: Serial Monitor, baud 115200.
 */

#include <WiFi.h>
#include <esp_wifi.h>  // esp_wifi_get_mac()
#include <esp_now.h>   // fungsi-fungsi ESP-NOW
#include <DHT.h>

#define DHT_PIN  27
#define DHT_TYPE DHT11

const uint8_t KANAL_ESPNOW = 1;       // harus sama di semua board
const char NAMA_BOARD[] = "Kamar-A";  // ← boleh diganti, maksimal 11 huruf

uint8_t MAC_BROADCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Isi paket: HARUS sama persis dengan struct di p2_broadcast_receiver
struct PaketCuaca {
  uint32_t nomor;       // nomor urut paket
  float    suhu;        // °C
  float    kelembapan;  // %
  char     nama[12];    // nama board pengirim (teks diakhiri '\0')
};

// Pola timer dari BAB 16
struct Timer {
  unsigned long interval;
  unsigned long waktuLalu;
  bool sudahWaktunya(unsigned long sekarang) {
    if (sekarang - waktuLalu >= interval) {
      waktuLalu = sekarang;
      return true;
    }
    return false;
  }
};

DHT dht(DHT_PIN, DHT_TYPE);
Timer timerKirim = {3000, 0};  // 3 detik; DHT11 paling cepat dibaca tiap 2 detik
PaketCuaca paket = {};          // semua kolom diisi 0 dulu
bool espNowSiap = false;

void cetakMac(const uint8_t *mac) {
  Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void cetakMacBoardIni() {
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  Serial.print("MAC board ini : ");
  cetakMac(mac);
  Serial.println();
  Serial.printf("Format kode   : {0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X}\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

bool mulaiEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.STA.begin();
  WiFi.setChannel(KANAL_ESPNOW);
  cetakMacBoardIni();
  Serial.printf("Kanal         : %ld\n", (long)WiFi.channel());

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW gagal dimulai. Tekan tombol EN untuk mencoba lagi.");
    return false;
  }
  return true;  // broadcast tidak punya ACK, jadi callback kirim tidak dipakai
}

bool tambahPeer(const uint8_t *mac) {
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = 0;          // 0 = ikut kanal radio saat ini
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;      // broadcast tidak bisa dienkripsi
  esp_err_t hasil = esp_now_add_peer(&peer);
  if (hasil != ESP_OK) {
    Serial.printf("Gagal mendaftarkan peer: %s\n", esp_err_to_name(hasil));
    return false;
  }
  return true;
}

void kirimData() {
  float suhu = dht.readTemperature();
  float kelembapan = dht.readHumidity();
  if (isnan(suhu) || isnan(kelembapan)) {
    Serial.println("Gagal membaca DHT11, paket tidak dikirim. Periksa kabel DHT -> IO27.");
    return;
  }

  paket.nomor = paket.nomor + 1;
  paket.suhu = suhu;
  paket.kelembapan = kelembapan;

  esp_err_t hasil = esp_now_send(MAC_BROADCAST, (uint8_t *)&paket, sizeof(paket));
  if (hasil == ESP_OK) {
    Serial.printf("Broadcast #%lu: %.1f °C | %.0f %%\n",
                  (unsigned long)paket.nomor, suhu, kelembapan);
  } else {
    Serial.printf("Broadcast #%lu ditolak: %s\n",
                  (unsigned long)paket.nomor, esp_err_to_name(hasil));
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  dht.begin();

  Serial.println();
  Serial.println("BAB 54 - Program 2: Broadcast Suhu DHT11 (board A)");

  espNowSiap = mulaiEspNow();
  if (!espNowSiap) return;

  if (tambahPeer(MAC_BROADCAST)) {
    Serial.println("Peer broadcast FF:FF:FF:FF:FF:FF terdaftar.");
  }
  strncpy(paket.nama, NAMA_BOARD, sizeof(paket.nama) - 1);  // sisakan 1 byte untuk '\0'
  Serial.printf("Nama board    : %s\n", paket.nama);
  Serial.println("Menyiarkan data setiap 3 detik...");
}

void loop() {
  if (!espNowSiap) return;
  if (timerKirim.sudahWaktunya(millis())) {
    kirimData();
  }
}
