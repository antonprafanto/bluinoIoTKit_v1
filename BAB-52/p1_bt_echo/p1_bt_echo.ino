/*
 * BAB 52 - Program 1: Halo Bluetooth — Echo dan Obrolan
 * ESP32 menjadi perangkat Bluetooth Classic (SPP) bernama "Bluino-Kit".
 *   - Teks dari HP ditampilkan di Serial Monitor, lalu dibalas ke HP (echo).
 *   - Teks yang diketik di Serial Monitor dikirim ke HP.
 *   - LED biru bawaan board (IO2) menyala selama ada perangkat tersambung.
 *
 * Board: ESP32 Dev Module (chip ESP32 "klasik", bukan S3/C3/C6)
 * Alat uji: Serial Monitor (115200, line ending New Line) dan aplikasi
 *           Serial Bluetooth Terminal di HP Android
 */

#include <BluetoothSerial.h>

// Pemeriksaan saat compile: Bluetooth Classic hanya ada di chip ESP32 "klasik"
#if !defined(CONFIG_BT_SPP_ENABLED)
#error "Bluetooth Classic (SPP) tidak tersedia. Pilih Tools > Board > ESP32 Dev Module."
#endif

#define LED_BIRU 2  // LED biru bawaan board (juga LED3 kit kalau disambung ke IO2)

const char *NAMA_BT = "Bluino-Kit";  // nama yang muncul saat HP mencari perangkat

BluetoothSerial SerialBT;

// Pengumpul satu baris teks tanpa menunggu (pola dari BAB 22).
// Bisa dipakai untuk Serial maupun SerialBT karena keduanya turunan Stream.
struct PembacaBaris {
  char isi[64];     // teks C: maksimal 63 karakter + '\0'
  int panjang = 0;  // jumlah karakter yang sudah terkumpul

  // Hasilnya true kalau satu baris lengkap sudah siap di 'isi'
  bool baca(Stream &sumber) {
    while (sumber.available() > 0) {
      char c = sumber.read();
      if (c == '\r' || c == '\n') {   // CR atau LF = akhir baris
        if (panjang == 0) continue;   // baris kosong: lewati
        isi[panjang] = '\0';          // tutup teks dengan karakter nol
        panjang = 0;                  // siap untuk baris berikutnya
        return true;
      }
      if (panjang < (int)sizeof(isi) - 1) {   // sisakan 1 tempat untuk '\0'
        isi[panjang] = c;
        panjang = panjang + 1;
      }                                       // kalau penuh, karakter dibuang
    }
    return false;                             // baris belum lengkap
  }
};

PembacaBaris dariHP;      // baris dari HP (lewat Bluetooth)
PembacaBaris dariLaptop;  // baris dari Serial Monitor (lewat kabel USB)
bool adaKoneksi = false;  // keadaan koneksi pada putaran loop() sebelumnya

void setup() {
  Serial.begin(115200);
  pinMode(LED_BIRU, OUTPUT);
  digitalWrite(LED_BIRU, LOW);

  Serial.println();
  Serial.println("BAB 52 - Program 1: Halo Bluetooth - Echo dan Obrolan");

  if (!SerialBT.begin(NAMA_BT)) {
    Serial.println("Bluetooth gagal dinyalakan. Tekan tombol EN untuk mencoba lagi.");
    while (true) {
      delay(1000);  // program berhenti di sini
    }
  }

  Serial.printf("Bluetooth siap. Nama: %s\n", NAMA_BT);
  Serial.printf("Alamat MAC Bluetooth: %s\n", SerialBT.getBtAddressString().c_str());
  Serial.println("Lakukan pairing dari HP, lalu connect dari aplikasi Serial Bluetooth Terminal.");
}

void loop() {
  // 1. Pantau perubahan koneksi (polling)
  bool tersambung = SerialBT.hasClient();
  if (tersambung != adaKoneksi) {
    adaKoneksi = tersambung;
    digitalWrite(LED_BIRU, tersambung ? HIGH : LOW);
    if (tersambung) {
      Serial.println("[BT] Perangkat tersambung.");
      SerialBT.printf("Halo! Kamu tersambung ke %s.\r\n", NAMA_BT);
      SerialBT.println("Ketik apa saja, ESP32 akan membalasnya.");
    } else {
      Serial.println("[BT] Perangkat terputus.");
    }
  }

  // 2. Baris dari HP: tampilkan di Serial Monitor, lalu balas ke HP
  if (dariHP.baca(SerialBT)) {
    Serial.printf("[HP] %s\n", dariHP.isi);
    SerialBT.printf("ESP32 menerima: %s\r\n", dariHP.isi);
  }

  // 3. Baris dari Serial Monitor: kirim ke HP
  if (dariLaptop.baca(Serial)) {
    if (adaKoneksi) {
      SerialBT.printf("[Laptop] %s\r\n", dariLaptop.isi);
      Serial.printf("[Terkirim ke HP] %s\n", dariLaptop.isi);
    } else {
      Serial.println("Belum ada perangkat yang tersambung - teks tidak dikirim.");
    }
  }
}
