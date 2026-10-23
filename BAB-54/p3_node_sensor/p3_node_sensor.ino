/*
 * BAB 54 - Program 3 (board A): Node Sensor DHT11
 *
 * Setiap 2 detik mengirim suhu dan kelembapan ke controller (board B,
 * p3_node_controller) lewat ESP-NOW unicast, lalu menerima balasan berisi
 * status relay. LED biru bawaan board menyala selama balasan lancar.
 *
 * Rangkaian board A: DHT (header hijau, pin ke-3 dari atas) -> IO27
 * Isi MAC_CONTROLLER dengan MAC board B.
 * Library: DHT sensor library by Adafruit (+ Adafruit Unified Sensor)
 * Alat uji: Serial Monitor, baud 115200.
 */

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <DHT.h>

#define DHT_PIN  27
#define DHT_TYPE DHT11
#define LED_BIRU 2  // LED biru bawaan board ESP32

const uint8_t KANAL_ESPNOW = 1;            // harus sama dengan board B
const unsigned long BATAS_BALASAN = 5000;  // ms tanpa balasan = dianggap putus

// ← ganti dengan MAC board B (baris "Format kode" di Serial Monitor board B)
uint8_t MAC_CONTROLLER[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

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

DHT dht(DHT_PIN, DHT_TYPE);
Timer timerKirim = {2000, 0};  // DHT11 paling cepat dibaca tiap 2 detik

// Hasil ACK radio dari callback kirim
const uint8_t HASIL_BELUM_ADA = 0;
const uint8_t HASIL_SUKSES    = 1;
const uint8_t HASIL_GAGAL     = 2;
volatile uint8_t hasilKirim = HASIL_BELUM_ADA;

// Balasan dari callback terima, dijaga kunci
portMUX_TYPE kunci = portMUX_INITIALIZER_UNLOCKED;
volatile bool adaBalasanBaru = false;
PaketBalasan balasanMasuk;

PaketSensor paket = {};
bool espNowSiap = false;
bool peerSiap = false;
bool pernahDibalas = false;
bool sudahLaporPutus = false;
unsigned long waktuBalasan = 0;

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

bool macMasihKosong(const uint8_t *mac) {
  for (int i = 0; i < 6; i++) {
    if (mac[i] != 0) return false;
  }
  return true;
}

void saatTerkirim(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  hasilKirim = (status == ESP_NOW_SEND_SUCCESS) ? HASIL_SUKSES : HASIL_GAGAL;
}

void saatMenerima(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
  if (len != sizeof(PaketBalasan)) return;
  if (memcmp(info->src_addr, MAC_CONTROLLER, 6) != 0) return;  // hanya dari controller
  portENTER_CRITICAL(&kunci);
  memcpy(&balasanMasuk, data, sizeof(PaketBalasan));
  adaBalasanBaru = true;
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
  esp_now_register_send_cb(saatTerkirim);
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

void kirimData() {
  if (!peerSiap) {
    Serial.println("MAC_CONTROLLER belum diisi, tidak ada yang dikirim.");
    return;
  }

  float suhu = dht.readTemperature();
  float kelembapan = dht.readHumidity();
  paket.nomor = paket.nomor + 1;

  if (isnan(suhu) || isnan(kelembapan)) {
    paket.suhu = 0;
    paket.kelembapan = 0;
    paket.sensorOk = 0;  // beri tahu controller bahwa sensor bermasalah
    Serial.printf("Kirim #%lu: DHT11 GAGAL dibaca\n", (unsigned long)paket.nomor);
  } else {
    paket.suhu = suhu;
    paket.kelembapan = kelembapan;
    paket.sensorOk = 1;
    Serial.printf("Kirim #%lu: %.1f °C | %.0f %%\n",
                  (unsigned long)paket.nomor, suhu, kelembapan);
  }

  esp_err_t hasil = esp_now_send(MAC_CONTROLLER, (uint8_t *)&paket, sizeof(paket));
  if (hasil != ESP_OK) {
    Serial.printf("  Ditolak sebelum dikirim: %s\n", esp_err_to_name(hasil));
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  pinMode(LED_BIRU, OUTPUT);
  digitalWrite(LED_BIRU, LOW);
  dht.begin();

  Serial.println();
  Serial.println("BAB 54 - Program 3: Node Sensor DHT11 (board A)");

  espNowSiap = mulaiEspNow();
  if (!espNowSiap) return;

  if (macMasihKosong(MAC_CONTROLLER)) {
    Serial.println("MAC_CONTROLLER masih 00:00:00:00:00:00.");
    Serial.println("Isi dengan MAC board B, lalu upload ulang program ini.");
  } else {
    peerSiap = tambahPeer(MAC_CONTROLLER);
    if (peerSiap) {
      Serial.print("Peer (controller): ");
      cetakMac(MAC_CONTROLLER);
      Serial.println();
    }
  }
  Serial.println("Mengirim data setiap 2 detik...");
}

void loop() {
  if (!espNowSiap) return;
  unsigned long sekarang = millis();

  if (timerKirim.sudahWaktunya(sekarang)) {
    kirimData();
  }

  // 1. ACK radio (lapisan MAC): hanya kegagalan yang dicetak
  uint8_t hasil = hasilKirim;
  if (hasil != HASIL_BELUM_ADA) {
    hasilKirim = HASIL_BELUM_ADA;
    if (hasil == HASIL_GAGAL) {
      Serial.println("  ACK radio GAGAL: controller tidak terjangkau");
    }
  }

  // 2. Balasan dari program controller (lapisan aplikasi)
  if (adaBalasanBaru) {
    PaketBalasan balasan;
    portENTER_CRITICAL(&kunci);
    balasan = balasanMasuk;
    adaBalasanBaru = false;
    portEXIT_CRITICAL(&kunci);

    pernahDibalas = true;
    sudahLaporPutus = false;
    waktuBalasan = sekarang;
    Serial.printf("  Balasan #%lu: relay %s\n", (unsigned long)balasan.nomorDibalas,
                  balasan.relayAktif ? "AKTIF" : "MATI");
  }

  // 3. LED biru menyala selama balasan masih lancar
  bool tersambung = pernahDibalas && (sekarang - waktuBalasan < BATAS_BALASAN);
  digitalWrite(LED_BIRU, tersambung ? HIGH : LOW);
  if (pernahDibalas && !tersambung && !sudahLaporPutus) {
    Serial.println("Tidak ada balasan selama 5 detik: controller mati atau di luar jangkauan?");
    sudahLaporPutus = true;
  }
}
