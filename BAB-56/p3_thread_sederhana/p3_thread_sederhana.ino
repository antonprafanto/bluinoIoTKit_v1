/*
 * BAB 56 - Program 3: Jaringan Thread Sederhana
 * BUTUH 2 board ESP32-C6 atau ESP32-H2 (punya radio IEEE 802.15.4).
 * Board ini TIDAK termasuk kit. ESP32 di kit tidak bisa Thread.
 *
 * Board pertama : JADI_LEADER true  -> membuat jaringan Thread baru
 * Board kedua   : JADI_LEADER false -> bergabung dengan kunci yang sama
 * Setiap 10 detik, setiap board mengirim pesan UDP ke semua node
 * (alamat multicast ff03::1) dan menampilkan pesan dari node lain.
 *
 * Board   : ESP32C6 Dev Module (atau ESP32H2 Dev Module)
 * Library : OpenThread (bawaan core esp32 3.3.x)
 */

#include <Arduino.h>
#include "OThread.h"
#include "OThreadUDP.h"

#define JADI_LEADER true  // ← ubah menjadi false untuk board kedua

// Pengaturan jaringan: harus SAMA di semua board
const char *NAMA_JARINGAN = "BluinoThread";
const uint8_t KANAL = 15;        // kanal 802.15.4: 11 sampai 26
const uint16_t PAN_ID = 0x1234;  // nomor jaringan 16-bit
const uint8_t EXT_PAN_ID[OT_EXT_PAN_ID_SIZE] = {0xB1, 0x0E, 0x00, 0x56, 0x00, 0x00, 0x00, 0x01};
// Kunci jaringan 16 byte (seperti password WiFi). Ganti dengan angka acakmu sendiri!
const uint8_t KUNCI_JARINGAN[OT_NETWORK_KEY_SIZE] = {
  0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
  0x88, 0x99, 0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF
};
const uint16_t PORT_UDP = 5050;  // jangan pakai port milik Thread (misalnya 61631)

// struct Timer dari BAB 16
struct Timer {
  unsigned long interval;   // ms
  unsigned long waktuLalu;  // hasil millis() saat terakhir dijalankan

  bool sudahWaktunya(unsigned long sekarang) {
    if (sekarang - waktuLalu >= interval) {
      waktuLalu = sekarang;
      return true;
    }
    return false;
  }
};

DataSet dataset;  // kumpulan pengaturan jaringan Thread
OThreadUDP udp;   // UDP lewat Thread
Timer timerKirim = {10000, 0};

char idNode[9];  // 4 byte terakhir alamat EUI-64, misalnya "1A2B3C4D"
bool udpSiap = false;
unsigned long nomorPesan = 0;
ot_device_role_t peranLalu = OT_ROLE_DISABLED;

void buatIdNode() {
  uint8_t eui[8] = {0};
  OThread.getEui64(eui);
  snprintf(idNode, sizeof(idNode), "%02X%02X%02X%02X", eui[4], eui[5], eui[6], eui[7]);
}

// Menampilkan perubahan peran (Detached, Child, Router, Leader)
void cekPeran() {
  ot_device_role_t peran = OThread.otGetDeviceRole();
  if (peran == peranLalu) {
    return;
  }
  Serial.printf("[THREAD] Peran berubah: %s -> %s\n", otRoleString[peranLalu], OThread.otGetStringDeviceRole());
  peranLalu = peran;
  if (peran >= OT_ROLE_CHILD) {
    OThread.clearAllAddressCache();  // alamat bisa berubah saat peran berubah
    Serial.printf("[THREAD] Jaringan: %s | Kanal: %u | PAN ID: 0x%04X | RLOC16: 0x%04X\n",
                  OThread.getNetworkName().c_str(), OThread.getChannel(), OThread.getPanId(), OThread.getRloc16());
    Serial.printf("[THREAD] Alamat IPv6 (mesh-local): %s\n", OThread.getMeshLocalEid().toString().c_str());
  }
}

void kirimSapaan() {
  nomorPesan = nomorPesan + 1;
  char pesan[64];
  snprintf(pesan, sizeof(pesan), "Halo dari %s, pesan ke-%lu", idNode, nomorPesan);
  bool berhasil = udp.beginPacket("ff03::1", PORT_UDP);  // ff03::1 = semua node Thread
  if (berhasil) {
    udp.write((const uint8_t *)pesan, strlen(pesan));
    berhasil = udp.endPacket();
  }
  Serial.printf("[KIRIM] %s (%s)\n", pesan, berhasil ? "OK" : "gagal");
}

void bacaPesanMasuk() {
  while (udp.parsePacket() > 0) {
    char isi[80];
    int panjang = udp.read(isi, sizeof(isi) - 1);
    if (panjang < 0) {
      panjang = 0;
    }
    isi[panjang] = '\0';
    if (strstr(isi, idNode) != NULL) {
      continue;  // abaikan pesan buatan sendiri
    }
    Serial.printf("[TERIMA] dari %s: %s\n", udp.remoteIP().toString().c_str(), isi);
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println();
  Serial.println("=== BAB 56 - Program 3: Jaringan Thread Sederhana ===");

  OThread.begin(false);  // false = jangan langsung memakai pengaturan lama
  buatIdNode();
  Serial.printf("[THREAD] ID node ini: %s\n", idNode);

#if JADI_LEADER
  dataset.initNew();  // isi acak untuk bagian yang tidak diatur di bawah
  dataset.setNetworkName(NAMA_JARINGAN);
  dataset.setExtendedPanId(EXT_PAN_ID);
  dataset.setPanId(PAN_ID);
  Serial.println("[THREAD] Mode: membuat jaringan baru");
#else
  dataset.clear();  // sisanya dipelajari dari Leader setelah bergabung
  Serial.println("[THREAD] Mode: bergabung ke jaringan yang sudah ada");
#endif
  dataset.setNetworkKey(KUNCI_JARINGAN);
  dataset.setChannel(KANAL);

  OThread.commitDataSet(dataset);  // simpan sebagai pengaturan aktif
  OThread.networkInterfaceUp();    // nyalakan antarmuka (seperti "ifconfig up")
  OThread.start();                 // mulai mencari/membentuk jaringan
  Serial.println("[THREAD] Menunggu tersambung ke jaringan Thread...");
}

void loop() {
  cekPeran();

  // UDP baru dibuka setelah tergabung (Child, Router, atau Leader)
  if (!udpSiap && OThread.otGetDeviceRole() >= OT_ROLE_CHILD) {
    udpSiap = udp.begin(PORT_UDP);
    Serial.printf("[UDP] Port %u %s\n", PORT_UDP, udpSiap ? "siap" : "gagal dibuka");
  }

  if (udpSiap) {
    bacaPesanMasuk();
    if (timerKirim.sudahWaktunya(millis())) {
      kirimSapaan();
    }
  }
  delay(10);
}
