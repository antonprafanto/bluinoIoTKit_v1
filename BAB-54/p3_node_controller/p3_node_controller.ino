/*
 * BAB 54 - Program 3 (board B): Controller Relay + OLED
 *
 * Menerima suhu dari node sensor (board A, p3_node_sensor), mengatur relay
 * dengan histeresis seperti termostat BAB 37, lalu membalas status relay.
 * Pengaman: relay dimatikan kalau sensor gagal atau data hilang 10 detik.
 *
 * Rangkaian board B:
 *   RELAY (header biru, kanan BUZZ) -> IO26
 *   LED1 (merah)                    -> IO4   (data hilang / sensor gagal)
 *   LED2 (hijau)                    -> IO17  (data lancar)
 *   OLED: lewat jumper cap JP (SDA IO21, SCL IO22)
 * Library: Adafruit SSD1306 + Adafruit GFX Library by Adafruit
 * Alat uji: Serial Monitor, baud 115200. JANGAN sambungkan 220 V ke relay.
 */

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define RELAY_PIN 26
#define LED_MERAH 4
#define LED_HIJAU 17

const uint8_t KANAL_ESPNOW = 1;  // harus sama dengan board A

// ← Sesuaikan kedua batas ini dengan suhu ruanganmu (lihat langkah 2)
const float SUHU_NYALA = 31.0;  // °C: relay aktif kalau suhu >= angka ini
const float SUHU_MATI  = 29.0;  // °C: relay mati kalau suhu <= angka ini
const unsigned long BATAS_DATA_HILANG = 10000;  // ms tanpa paket sebelum relay dimatikan

// Dua jenis paket: HARUS sama persis di p3_node_sensor dan p3_node_controller
struct PaketSensor {     // node sensor -> controller
  uint32_t nomor;        // nomor urut paket
  float    suhu;         // °C
  float    kelembapan;   // %
  uint8_t  sensorOk;     // 1 = bacaan DHT11 valid, 0 = gagal
};

struct PaketBalasan {    // controller -> node sensor
  uint32_t nomorDibalas; // nomor PaketSensor yang dibalas
  uint8_t  relayAktif;   // 1 = relay aktif, 0 = mati
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
Timer timerLayar = {500, 0};

// Data bersama antara callback (task WiFi) dan loop(), dijaga kunci
portMUX_TYPE kunci = portMUX_INITIALIZER_UNLOCKED;
volatile bool adaPaketBaru = false;
PaketSensor paketMasuk;
uint8_t macPengirim[6];

PaketSensor dataTerakhir = {};
bool pernahMenerima = false;
bool dataHilang = false;
unsigned long waktuTerima = 0;
bool relayAktif = false;
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

void saatMenerima(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(PaketSensor)) return;
  portENTER_CRITICAL(&kunci);
  memcpy(&paketMasuk, data, sizeof(PaketSensor));
  memcpy(macPengirim, info->src_addr, 6);
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

bool tambahPeer(const uint8_t *mac) {
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = 0;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  esp_err_t hasil = esp_now_add_peer(&peer);
  if (hasil != ESP_OK) {
    Serial.printf("Gagal mendaftarkan peer: %s\n", esp_err_to_name(hasil));
    return false;
  }
  return true;
}

void mulaiOled() {
  Wire.begin(21, 22);
  Wire.beginTransmission(0x3C);
  if (Wire.endTransmission() != 0) {
    Serial.println("OLED tidak menjawab di 0x3C, data hanya tampil di Serial Monitor.");
    return;
  }
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("RAM tidak cukup untuk buffer OLED.");
    return;
  }
  oledAda = true;
  oled.cp437(true);
  oled.setTextColor(SSD1306_WHITE);
  Serial.println("OLED siap di alamat 0x3C.");
}

void aturRelay(bool aktif, const char *alasan) {
  if (aktif == relayAktif) return;  // tidak berubah: jangan klik-klik
  relayAktif = aktif;
  digitalWrite(RELAY_PIN, aktif ? HIGH : LOW);
  Serial.printf("Relay -> %s (%s)\n", aktif ? "AKTIF" : "MATI", alasan);
}

void prosesPaketBaru() {
  PaketSensor paket;
  uint8_t mac[6];
  portENTER_CRITICAL(&kunci);
  paket = paketMasuk;
  memcpy(mac, macPengirim, 6);
  adaPaketBaru = false;
  portEXIT_CRITICAL(&kunci);

  // 1. Pengirim baru? Daftarkan sebagai peer supaya bisa dibalas
  bool bisaMembalas = esp_now_is_peer_exist(mac);
  if (!bisaMembalas) {
    bisaMembalas = tambahPeer(mac);
    if (bisaMembalas) {
      Serial.print("Node baru terdaftar sebagai peer: ");
      cetakMac(mac);
      Serial.println();
    }
  }

  dataTerakhir = paket;
  pernahMenerima = true;
  waktuTerima = millis();
  if (dataHilang) {
    dataHilang = false;
    Serial.println("Data dari node sensor kembali lancar.");
  }

  // 2. Putuskan keadaan relay
  if (paket.sensorOk == 0) {
    Serial.printf("Paket #%lu: DHT11 di node GAGAL dibaca\n", (unsigned long)paket.nomor);
    aturRelay(false, "sensor gagal, demi keamanan");
  } else {
    Serial.printf("Paket #%lu: %.1f °C | %.0f %%\n",
                  (unsigned long)paket.nomor, paket.suhu, paket.kelembapan);
    if (paket.suhu >= SUHU_NYALA) {
      aturRelay(true, "suhu mencapai batas atas");
    } else if (paket.suhu <= SUHU_MATI) {
      aturRelay(false, "suhu turun ke batas bawah");
    }
    // di antara kedua batas: relay mempertahankan keadaannya (histeresis)
  }

  // 3. Balas ke node sensor: "paket nomor sekian sudah diproses, relay begini"
  if (!bisaMembalas) return;
  PaketBalasan balasan = {};
  balasan.nomorDibalas = paket.nomor;
  balasan.relayAktif = relayAktif ? 1 : 0;
  esp_err_t hasil = esp_now_send(mac, (uint8_t *)&balasan, sizeof(balasan));
  if (hasil != ESP_OK) {
    Serial.printf("Balasan ditolak: %s\n", esp_err_to_name(hasil));
  }
}

void periksaDataHilang() {
  if (!pernahMenerima || dataHilang) return;
  if (millis() - waktuTerima > BATAS_DATA_HILANG) {
    dataHilang = true;
    Serial.println("DATA HILANG: tidak ada paket selama 10 detik.");
    aturRelay(false, "data hilang, demi keamanan");
  }
}

void perbaruiLed() {
  bool sensorGagal = pernahMenerima && dataTerakhir.sensorOk == 0;
  bool lancar = pernahMenerima && !dataHilang && !sensorGagal;
  digitalWrite(LED_HIJAU, lancar ? HIGH : LOW);
  digitalWrite(LED_MERAH, (dataHilang || sensorGagal) ? HIGH : LOW);
}

void tampilkanLayar() {
  if (!oledAda) return;
  oled.clearDisplay();
  oled.setTextSize(1);
  oled.setCursor(0, 0);
  oled.print("Controller ESP-NOW");

  if (!pernahMenerima) {
    oled.setCursor(0, 24);
    oled.print("Menunggu node...");
    oled.display();
    return;
  }

  oled.setTextSize(2);
  oled.setCursor(0, 14);
  if (dataTerakhir.sensorOk) {
    oled.print(dataTerakhir.suhu, 1);
    oled.write(248);  // simbol derajat
    oled.print("C");
  } else {
    oled.print("DHT ERR");
  }

  oled.setTextSize(1);
  oled.setCursor(0, 36);
  if (dataTerakhir.sensorOk) {
    oled.printf("Lembap: %.0f %%", dataTerakhir.kelembapan);
  } else {
    oled.print("Lembap: -");
  }
  oled.setCursor(0, 46);
  oled.print("Relay : ");
  oled.print(relayAktif ? "AKTIF" : "MATI");
  oled.setCursor(0, 56);
  if (dataHilang) {
    oled.print("DATA HILANG!");
  } else {
    oled.printf("Data  : %lu s lalu", (millis() - waktuTerima) / 1000);
  }
  oled.display();
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, LOW);  // relay mati sejak awal
  pinMode(LED_MERAH, OUTPUT);
  digitalWrite(LED_MERAH, LOW);
  pinMode(LED_HIJAU, OUTPUT);
  digitalWrite(LED_HIJAU, LOW);

  Serial.println();
  Serial.println("BAB 54 - Program 3: Controller Relay + OLED (board B)");

  mulaiOled();
  espNowSiap = mulaiEspNow();
  if (!espNowSiap) return;

  Serial.println("Salin baris \"Format kode\" ke MAC_CONTROLLER di p3_node_sensor.");
  Serial.printf("Relay AKTIF jika suhu >= %.1f °C, MATI jika suhu <= %.1f °C\n",
                SUHU_NYALA, SUHU_MATI);
  Serial.println("Menunggu data dari node sensor...");
  tampilkanLayar();
}

void loop() {
  if (!espNowSiap) return;

  if (adaPaketBaru) {
    prosesPaketBaru();
  }
  periksaDataHilang();
  perbaruiLed();

  if (timerLayar.sudahWaktunya(millis())) {
    tampilkanLayar();
  }
}
