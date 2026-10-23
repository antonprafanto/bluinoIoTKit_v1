/*
 * BAB 24 - Program 3: Pengaruh Kecepatan Clock SPI
 * Membaca 128 sektor pertama kartu microSD (64 KB) pada beberapa kecepatan
 * clock SPI, lalu menampilkan lama baca dan kecepatan transfernya.
 * Program ini hanya MEMBACA sektor: tidak ada data yang ditulis ke kartu.
 *
 * Rangkaian: tidak perlu kabel. Adaptor microSD tersambung ke
 * CS = IO5, MOSI = IO23, CLK = IO18, MISO = IO19 lewat jumper cap di kit.
 */

#include <SPI.h>
#include <SD.h>

#define PIN_SCK  18  // CLK
#define PIN_MISO 19
#define PIN_MOSI 23
#define PIN_CS   5

// Kecepatan clock yang dicoba (Hz)
const uint32_t daftarKecepatan[] = {400000, 1000000, 4000000, 10000000, 20000000};
const int JUMLAH_KECEPATAN = 5;

const uint32_t JUMLAH_SEKTOR = 128;  // 128 sektor x 512 byte = 64 KB
uint8_t penampung[512];              // tempat menampung isi satu sektor

// Membaca JUMLAH_SEKTOR sektor pada satu kecepatan, lalu mencetak hasilnya
void ujiKecepatan(uint32_t kecepatan) {
  float mhz = kecepatan / 1000000.0;

  if (!SD.begin(PIN_CS, SPI, kecepatan)) {
    Serial.printf("Clock %5.1f MHz : kartu gagal dipasang\n", mhz);
    return;
  }

  uint32_t mulai = micros();
  for (uint32_t sektor = 0; sektor < JUMLAH_SEKTOR; sektor++) {
    if (!SD.readRAW(penampung, sektor)) {  // baca satu sektor mentah (512 byte)
      Serial.printf("Clock %5.1f MHz : gagal membaca sektor %lu\n", mhz, (unsigned long)sektor);
      SD.end();
      return;
    }
  }
  uint32_t lamaUs = micros() - mulai;

  float lamaMs = lamaUs / 1000.0;
  float kbPerDetik = (JUMLAH_SEKTOR * 512 / 1024.0) / (lamaUs / 1000000.0);
  Serial.printf("Clock %5.1f MHz : %7.1f ms -> %6.1f KB/s\n", mhz, lamaMs, kbPerDetik);

  SD.end();  // lepas kartu supaya bisa dipasang lagi dengan kecepatan lain
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  Serial.println("BAB 24 - Program 3: Pengaruh Kecepatan Clock SPI");
  Serial.printf("Membaca %lu sektor (%lu KB) pada setiap kecepatan...\n",
                (unsigned long)JUMLAH_SEKTOR, (unsigned long)(JUMLAH_SEKTOR * 512 / 1024));

  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);  // urutan: SCK, MISO, MOSI

  for (int i = 0; i < JUMLAH_KECEPATAN; i++) {
    ujiKecepatan(daftarKecepatan[i]);
  }
  Serial.println("Selesai. Tekan tombol EN untuk mengulang.");
}

void loop() {
  // Pengujian hanya dijalankan sekali di setup().
}
