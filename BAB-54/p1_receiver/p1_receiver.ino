/*
 * BAB 54 - Program 1 (board B): Penerima LED Merah
 *
 * Menerima perintah dari board A (p1_sender) lewat ESP-NOW, lalu
 * menyalakan/mematikan LED merah. MAC board ini dicetak di Serial
 * Monitor supaya bisa disalin ke MAC_PENERIMA di p1_sender.
 *
 * Rangkaian board B: LED1 (merah, header LED) -> IO4
 * Alat uji: Serial Monitor, baud 115200.
 */

#include <WiFi.h>
#include <esp_wifi.h>  // esp_wifi_get_mac()
#include <esp_now.h>   // fungsi-fungsi ESP-NOW

#define LED_MERAH 4  // LED1 (merah). Board ESP32 polos tanpa kit: ganti menjadi 2

const uint8_t KANAL_ESPNOW = 1;  // harus sama dengan board A

// Isi paket: HARUS sama persis dengan struct di p1_sender
struct PaketLed {
  uint32_t nomor;     // nomor urut paket: 1, 2, 3, ...
  uint8_t  ledNyala;  // 1 = nyalakan LED merah, 0 = matikan
};

// Data bersama antara callback (task WiFi) dan loop(), dijaga kunci (BAB 10)
portMUX_TYPE kunci = portMUX_INITIALIZER_UNLOCKED;
volatile bool adaPaketBaru = false;
PaketLed paketMasuk;
uint8_t macPengirim[6];
int rssiPaket = 0;

bool espNowSiap = false;

void cetakMac(const uint8_t *mac) {
  Serial.printf("%02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// Mencetak MAC board ini dalam dua bentuk: teks biasa dan siap ditempel ke kode
void cetakMacBoardIni() {
  uint8_t mac[6];
  esp_wifi_get_mac(WIFI_IF_STA, mac);
  Serial.print("MAC board ini : ");
  cetakMac(mac);
  Serial.println();
  Serial.printf("Format kode   : {0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X, 0x%02X}\n",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

// Callback: dipanggil ESP-NOW setiap ada paket masuk (bentuk core 3.x)
void saatMenerima(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(PaketLed)) return;  // ukuran tidak cocok: bukan paket kita
  portENTER_CRITICAL(&kunci);
  memcpy(&paketMasuk, data, sizeof(PaketLed));
  memcpy(macPengirim, info->src_addr, 6);
  rssiPaket = info->rx_ctrl->rssi;
  adaPaketBaru = true;
  portEXIT_CRITICAL(&kunci);
}

bool mulaiEspNow() {
  WiFi.mode(WIFI_STA);            // 1. nyalakan radio WiFi sebagai Station
  WiFi.STA.begin();               // 2. tunggu radio benar-benar menyala (maks. 1 detik)
  WiFi.setChannel(KANAL_ESPNOW);  // 3. pindah ke kanal yang disepakati
  cetakMacBoardIni();
  Serial.printf("Kanal         : %ld\n", (long)WiFi.channel());

  if (esp_now_init() != ESP_OK) {  // 4. nyalakan ESP-NOW
    Serial.println("ESP-NOW gagal dimulai. Tekan tombol EN untuk mencoba lagi.");
    return false;
  }
  esp_now_register_recv_cb(saatMenerima);  // 5. daftarkan callback terima
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  pinMode(LED_MERAH, OUTPUT);
  digitalWrite(LED_MERAH, LOW);

  Serial.println();
  Serial.println("BAB 54 - Program 1: Penerima LED Merah (board B)");

  espNowSiap = mulaiEspNow();
  if (!espNowSiap) return;

  Serial.println("Salin baris \"Format kode\" ke MAC_PENERIMA di p1_sender.");
  Serial.println("Menunggu perintah dari board A...");
}

void loop() {
  if (!espNowSiap || !adaPaketBaru) return;

  // Ambil salinan data dari callback, lalu lepaskan kunci secepatnya
  PaketLed paket;
  uint8_t mac[6];
  int rssi;
  portENTER_CRITICAL(&kunci);
  paket = paketMasuk;
  memcpy(mac, macPengirim, 6);
  rssi = rssiPaket;
  adaPaketBaru = false;
  portEXIT_CRITICAL(&kunci);

  digitalWrite(LED_MERAH, paket.ledNyala ? HIGH : LOW);

  Serial.printf("Paket #%lu dari ", (unsigned long)paket.nomor);
  cetakMac(mac);
  Serial.printf(" (RSSI %d dBm): LED %s\n", rssi, paket.ledNyala ? "NYALA" : "MATI");
}
