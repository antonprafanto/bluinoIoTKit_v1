/*
 * BAB 54 - Program 2 (board B, C, ...): Penerima Broadcast + OLED
 *
 * Menerima siaran suhu dari board mana pun di kanal yang sama, lalu
 * menampilkannya di OLED (kalau ada) dan Serial Monitor.
 * Program yang sama boleh diunggah ke beberapa board sekaligus.
 *
 * Rangkaian: OLED sudah tersambung lewat jumper cap JP (SDA IO21, SCL IO22).
 *            Board tanpa OLED tetap bisa dipakai (cukup Serial Monitor).
 * Library: Adafruit SSD1306 + Adafruit GFX Library by Adafruit
 * Alat uji: Serial Monitor, baud 115200.
 */

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

const uint8_t KANAL_ESPNOW = 1;               // harus sama dengan board A
const unsigned long BATAS_DATA_LAMA = 10000;  // ms: lebih lama dari ini = siaran terputus

// Isi paket: HARUS sama persis dengan struct di p2_broadcast_sender
struct PaketCuaca {
  uint32_t nomor;       // nomor urut paket
  float    suhu;        // °C
  float    kelembapan;  // %
  char     nama[12];    // nama board pengirim (teks diakhiri '\0')
};

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

Adafruit_SSD1306 oled(128, 64, &Wire, -1);
bool oledAda = false;
Timer timerLayar = {500, 0};  // gambar ulang OLED setiap 0,5 detik

// Data bersama antara callback (task WiFi) dan loop(), dijaga kunci
portMUX_TYPE kunci = portMUX_INITIALIZER_UNLOCKED;
volatile bool adaPaketBaru = false;
PaketCuaca paketMasuk;
uint8_t macPengirim[6];
int rssiPaket = 0;

// Data terakhir yang sudah diproses loop()
PaketCuaca dataTerakhir;
int rssiTerakhir = 0;
bool pernahMenerima = false;
bool sudahLaporTerputus = false;
unsigned long waktuTerima = 0;
uint32_t nomorSebelumnya = 0;
uint32_t jumlahHilang = 0;
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

// Callback terima: hanya salin dan angkat bendera
void saatMenerima(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(PaketCuaca)) return;
  portENTER_CRITICAL(&kunci);
  memcpy(&paketMasuk, data, sizeof(PaketCuaca));
  memcpy(macPengirim, info->src_addr, 6);
  rssiPaket = info->rx_ctrl->rssi;
  adaPaketBaru = true;
  portEXIT_CRITICAL(&kunci);
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
  esp_now_register_recv_cb(saatMenerima);
  return true;
}

void mulaiOled() {
  Wire.begin(21, 22);
  Wire.beginTransmission(0x3C);  // apakah OLED menjawab? (BAB 35)
  if (Wire.endTransmission() != 0) {
    Serial.println("OLED tidak menjawab di 0x3C, data hanya tampil di Serial Monitor.");
    return;
  }
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("RAM tidak cukup untuk buffer OLED.");
    return;
  }
  oledAda = true;
  oled.cp437(true);  // supaya simbol derajat (kode 248) tampil benar
  oled.setTextColor(SSD1306_WHITE);
  Serial.println("OLED siap di alamat 0x3C.");
}

void prosesPaketBaru() {
  PaketCuaca paket;
  uint8_t mac[6];
  int rssi;
  portENTER_CRITICAL(&kunci);
  paket = paketMasuk;
  memcpy(mac, macPengirim, 6);
  rssi = rssiPaket;
  adaPaketBaru = false;
  portEXIT_CRITICAL(&kunci);

  paket.nama[sizeof(paket.nama) - 1] = '\0';  // jaga-jaga: pastikan teks berakhir

  // Loncatan nomor urut = ada paket yang tidak sampai (broadcast tidak dikirim ulang)
  if (pernahMenerima && paket.nomor > nomorSebelumnya + 1) {
    uint32_t hilang = paket.nomor - nomorSebelumnya - 1;
    jumlahHilang = jumlahHilang + hilang;
    Serial.printf("(%lu paket sebelumnya tidak sampai)\n", (unsigned long)hilang);
  }
  nomorSebelumnya = paket.nomor;

  dataTerakhir = paket;
  rssiTerakhir = rssi;
  pernahMenerima = true;
  sudahLaporTerputus = false;
  waktuTerima = millis();

  Serial.printf("#%lu dari %s (", (unsigned long)paket.nomor, paket.nama);
  cetakMac(mac);
  Serial.printf(", RSSI %d dBm): %.1f °C | %.0f %%\n", rssi, paket.suhu, paket.kelembapan);
}

void tampilkanLayar() {
  if (!oledAda) return;
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print("ESP-NOW Broadcast");

  if (!pernahMenerima) {
    oled.setCursor(0, 24);
    oled.print("Menunggu siaran...");
    oled.display();
    return;
  }

  oled.setCursor(0, 12);
  oled.print("Dari: ");
  oled.print(dataTerakhir.nama);

  oled.setTextSize(2);
  oled.setCursor(0, 24);
  oled.print(dataTerakhir.suhu, 1);
  oled.write(248);  // simbol derajat
  oled.print("C");

  oled.setTextSize(1);
  oled.setCursor(0, 44);
  oled.printf("Lembap %.0f %%  %d dBm", dataTerakhir.kelembapan, rssiTerakhir);
  oled.setCursor(0, 56);
  unsigned long umur = millis() - waktuTerima;
  if (umur > BATAS_DATA_LAMA) {
    oled.print("Siaran terputus!");
  } else {
    oled.printf("%lu s lalu, hilang %lu", umur / 1000, (unsigned long)jumlahHilang);
  }
  oled.display();
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println();
  Serial.println("BAB 54 - Program 2: Penerima Broadcast + OLED");

  mulaiOled();
  espNowSiap = mulaiEspNow();
  if (!espNowSiap) return;

  Serial.println("Menunggu siaran dari board mana pun di kanal ini...");
  tampilkanLayar();
}

void loop() {
  if (!espNowSiap) return;

  if (adaPaketBaru) {
    prosesPaketBaru();
  }

  if (pernahMenerima && !sudahLaporTerputus && millis() - waktuTerima > BATAS_DATA_LAMA) {
    Serial.println("Tidak ada siaran selama 10 detik. Board A mati atau di luar jangkauan?");
    sudahLaporTerputus = true;
  }

  if (timerLayar.sudahWaktunya(millis())) {
    tampilkanLayar();
  }
}
