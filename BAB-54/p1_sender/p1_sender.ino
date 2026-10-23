/*
 * BAB 54 - Program 1 (board A): Pengirim Tombol S1
 *
 * Setiap kali S1 ditekan, board A mengirim perintah "LED merah nyala/mati"
 * langsung ke board B lewat ESP-NOW (unicast, tanpa router).
 * LED biru bawaan board A berkedip singkat setiap kali radio board B
 * mengonfirmasi bahwa paketnya sampai (ACK).
 *
 * Rangkaian board A: SW1 (header kuning, pin ke-3 dari kiri) -> IO15
 * Board B menjalankan p1_receiver. Isi MAC_PENERIMA dengan MAC board B.
 * Alat uji: Serial Monitor, baud 115200.
 */

#include <WiFi.h>
#include <esp_wifi.h>  // esp_wifi_get_mac()
#include <esp_now.h>   // fungsi-fungsi ESP-NOW

#define TOMBOL_S1 15  // S1: ditekan = HIGH (pull-down 10 kΩ di kit)
#define LED_BIRU  2   // LED biru bawaan board ESP32 (tidak perlu kabel)

const uint8_t KANAL_ESPNOW = 1;  // harus sama dengan board B

// ← ganti dengan MAC board B (baris "Format kode" di Serial Monitor board B)
uint8_t MAC_PENERIMA[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

// Isi paket: HARUS sama persis dengan struct di p1_receiver
struct PaketLed {
  uint32_t nomor;     // nomor urut paket: 1, 2, 3, ...
  uint8_t  ledNyala;  // 1 = nyalakan LED merah, 0 = matikan
};

const unsigned long WAKTU_DEBOUNCE = 50;   // ms, seperti BAB 9
const unsigned long LAMA_KEDIP     = 150;  // ms, lama LED biru menyala saat ACK datang

// Hasil pengiriman: ditulis callback (task WiFi), dibaca loop()
const uint8_t HASIL_BELUM_ADA = 0;
const uint8_t HASIL_SUKSES    = 1;
const uint8_t HASIL_GAGAL     = 2;
volatile uint8_t hasilKirim = HASIL_BELUM_ADA;

bool espNowSiap = false;
bool peerSiap = false;
PaketLed paket = {0, 0};
bool ledBTerkonfirmasi = false;  // keadaan LED board B yang terakhir dikonfirmasi ACK

int bacaanSebelumnya = LOW;      // variabel debounce (BAB 9)
int statusStabil = LOW;
unsigned long waktuBerubah = 0;

bool ledBiruNyala = false;
unsigned long waktuKedip = 0;

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

bool macMasihKosong(const uint8_t *mac) {
  for (int i = 0; i < 6; i++) {
    if (mac[i] != 0) return false;
  }
  return true;
}

// Callback: dipanggil ESP-NOW setelah paket selesai dikirim (bentuk core 3.3.x)
void saatTerkirim(const wifi_tx_info_t *info, esp_now_send_status_t status) {
  hasilKirim = (status == ESP_NOW_SEND_SUCCESS) ? HASIL_SUKSES : HASIL_GAGAL;
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
  esp_now_register_send_cb(saatTerkirim);  // 5. daftarkan callback kirim
  return true;
}

bool tambahPeer(const uint8_t *mac) {
  esp_now_peer_info_t peer = {};  // semua kolom diisi 0 dulu
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = 0;               // 0 = ikut kanal radio saat ini
  peer.ifidx = WIFI_IF_STA;       // kirim lewat antarmuka Station
  peer.encrypt = false;           // tanpa enkripsi
  esp_err_t hasil = esp_now_add_peer(&peer);
  if (hasil != ESP_OK) {
    Serial.printf("Gagal mendaftarkan peer: %s\n", esp_err_to_name(hasil));
    return false;
  }
  return true;
}

void kirimPerintah(bool nyala) {
  if (!peerSiap) {
    Serial.println("MAC_PENERIMA belum diisi, tekanan S1 diabaikan.");
    return;
  }
  paket.nomor = paket.nomor + 1;
  paket.ledNyala = nyala ? 1 : 0;
  Serial.printf("Kirim #%lu: LED %s", (unsigned long)paket.nomor, nyala ? "NYALA" : "MATI");

  esp_err_t hasil = esp_now_send(MAC_PENERIMA, (uint8_t *)&paket, sizeof(paket));
  if (hasil != ESP_OK) {
    // Ditolak sebelum dikirim, jadi callback kirim tidak akan dipanggil
    Serial.printf(" -> ditolak: %s\n", esp_err_to_name(hasil));
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  pinMode(TOMBOL_S1, INPUT);
  pinMode(LED_BIRU, OUTPUT);
  digitalWrite(LED_BIRU, LOW);

  Serial.println();
  Serial.println("BAB 54 - Program 1: Pengirim Tombol S1 (board A)");

  espNowSiap = mulaiEspNow();
  if (!espNowSiap) return;

  if (macMasihKosong(MAC_PENERIMA)) {
    Serial.println("MAC_PENERIMA masih 00:00:00:00:00:00.");
    Serial.println("Isi dengan MAC board B, lalu upload ulang program ini.");
  } else {
    peerSiap = tambahPeer(MAC_PENERIMA);
    if (peerSiap) {
      Serial.print("Peer (board B): ");
      cetakMac(MAC_PENERIMA);
      Serial.println();
    }
  }
  Serial.println("Tekan S1 untuk menyalakan/mematikan LED merah di board B.");
}

void loop() {
  if (!espNowSiap) return;
  unsigned long sekarang = millis();

  // 1. Baca S1 dengan debounce, bereaksi saat baru ditekan (pola BAB 9)
  int bacaan = digitalRead(TOMBOL_S1);
  if (bacaan != bacaanSebelumnya) {
    waktuBerubah = sekarang;
    bacaanSebelumnya = bacaan;
  }
  if (sekarang - waktuBerubah > WAKTU_DEBOUNCE && bacaan != statusStabil) {
    statusStabil = bacaan;
    if (statusStabil == HIGH) {
      kirimPerintah(!ledBTerkonfirmasi);  // minta LED board B berganti keadaan
    }
  }

  // 2. Proses hasil pengiriman yang dilaporkan callback
  uint8_t hasil = hasilKirim;
  if (hasil != HASIL_BELUM_ADA) {
    hasilKirim = HASIL_BELUM_ADA;
    if (hasil == HASIL_SUKSES) {
      ledBTerkonfirmasi = (paket.ledNyala == 1);
      Serial.println(" -> sampai di board B (ACK)");
      digitalWrite(LED_BIRU, HIGH);
      ledBiruNyala = true;
      waktuKedip = sekarang;
    } else {
      Serial.println(" -> GAGAL: board B tidak menjawab");
    }
  }

  // 3. Matikan LED biru setelah LAMA_KEDIP
  if (ledBiruNyala && sekarang - waktuKedip >= LAMA_KEDIP) {
    digitalWrite(LED_BIRU, LOW);
    ledBiruNyala = false;
  }
}
