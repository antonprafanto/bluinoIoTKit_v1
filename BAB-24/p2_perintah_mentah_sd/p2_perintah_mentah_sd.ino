/*
 * BAB 24 - Program 2: Perintah Mentah ke Kartu microSD
 * Berkenalan dengan kartu microSD tanpa library SD: mengirim CMD0, CMD8,
 * ACMD41, dan CMD58 lewat SPI, lalu menampilkan jawaban kartu.
 * Program ini hanya mengirim perintah perkenalan; isi kartu tidak diubah.
 *
 * Rangkaian: tidak perlu kabel. Adaptor microSD tersambung ke
 * CS = IO5, MOSI = IO23, CLK = IO18, MISO = IO19 lewat jumper cap di kit.
 */

#include <SPI.h>

#define PIN_SCK  18  // CLK
#define PIN_MISO 19
#define PIN_MOSI 23
#define PIN_CS   5

// Aturan komunikasi saat perkenalan dengan kartu: 400 kHz, MSB dulu, mode 0
SPISettings aturanKartu(400000, MSBFIRST, SPI_MODE0);

// Mengirim satu perintah ke kartu, lalu membaca jawabannya.
//   nomor   : nomor perintah (misalnya 8 untuk CMD8)
//   argumen : argumen 32 bit
//   crc     : byte terakhir perintah (CRC + bit penutup)
//   data    : tempat 4 byte tambahan (untuk CMD8 dan CMD58), atau nullptr
// Hasil: byte jawaban R1, atau 0xFF kalau kartu tidak menjawab.
uint8_t kirimPerintah(uint8_t nomor, uint32_t argumen, uint8_t crc, uint8_t *data) {
  SPI.beginTransaction(aturanKartu);  // 1. pinjam bus dengan aturan kartu
  digitalWrite(PIN_CS, LOW);          // 2. pilih kartu
  SPI.transfer(0xFF);                 //    8 detak jeda sebelum perintah

  // 3. Kirim perintah: selalu 6 byte
  SPI.transfer(0x40 | nomor);              // byte 1: bit 01 + nomor perintah
  SPI.transfer((uint8_t)(argumen >> 24));  // byte 2-5: argumen, byte terbesar dulu
  SPI.transfer((uint8_t)(argumen >> 16));
  SPI.transfer((uint8_t)(argumen >> 8));
  SPI.transfer((uint8_t)argumen);
  SPI.transfer(crc);                       // byte 6: CRC + bit penutup

  // Tunggu jawaban R1. Selama kartu belum menjawab, MISO terbaca 0xFF.
  uint8_t r1 = 0xFF;
  for (int i = 0; i < 10; i++) {
    r1 = SPI.transfer(0xFF);  // kirim 0xFF supaya ada detak untuk menerima
    if ((r1 & 0x80) == 0) {   // bit 7 bernilai 0 = ini jawaban R1
      break;
    }
  }

  // CMD8 dan CMD58 mengirim 4 byte tambahan setelah R1
  if (data != nullptr && r1 <= 0x01) {
    for (int i = 0; i < 4; i++) {
      data[i] = SPI.transfer(0xFF);
    }
  }

  digitalWrite(PIN_CS, HIGH);  // 4. lepas kartu
  SPI.transfer(0xFF);          //    8 detak tambahan agar kartu melepas jalur MISO
  SPI.endTransaction();        // 5. kembalikan bus
  return r1;
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  Serial.println("BAB 24 - Program 2: Perintah Mentah ke Kartu microSD");

  pinMode(PIN_CS, OUTPUT);
  digitalWrite(PIN_CS, HIGH);              // kartu belum dipilih
  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);  // urutan: SCK, MISO, MOSI

  // Langkah 1: bangunkan kartu dengan 80 detak saat CS HIGH
  SPI.beginTransaction(aturanKartu);
  for (int i = 0; i < 10; i++) {
    SPI.transfer(0xFF);  // 10 byte x 8 bit = 80 detak
  }
  SPI.endTransaction();

  // Langkah 2: CMD0 - reset kartu dan masuk mode SPI (dicoba sampai 3 kali)
  uint8_t r1 = 0xFF;
  for (int coba = 0; coba < 3 && r1 != 0x01; coba++) {
    r1 = kirimPerintah(0, 0, 0x95, nullptr);
    delay(20);
  }
  Serial.printf("CMD0   (reset, masuk mode SPI)   -> R1 = 0x%02X\n", r1);
  if (r1 != 0x01) {
    Serial.println("Kartu tidak menjawab dengan benar. Periksa kartu dan jumper cap.");
    return;
  }

  // Langkah 3: CMD8 - cek tegangan dan pola uji 0xAA
  uint8_t r7[4] = {0, 0, 0, 0};
  bool kartuVersi2 = false;
  r1 = kirimPerintah(8, 0x000001AA, 0x87, r7);
  if (r1 == 0x01) {
    Serial.printf("CMD8   (cek tegangan, pola 0xAA) -> R1 = 0x%02X, data = %02X %02X %02X %02X\n",
                  r1, r7[0], r7[1], r7[2], r7[3]);
    if (r7[3] != 0xAA) {
      Serial.println("       Pola 0xAA tidak kembali utuh: jalur data bermasalah.");
      return;
    }
    Serial.println("       Pola 0xAA kembali utuh: kartu SD versi 2.00 atau lebih baru.");
    kartuVersi2 = true;
  } else if (r1 == 0x05) {
    Serial.println("CMD8   (cek tegangan, pola 0xAA) -> R1 = 0x05 (ditolak: kartu SD versi 1.x)");
  } else {
    Serial.printf("CMD8   (cek tegangan, pola 0xAA) -> R1 = 0x%02X (jawaban tidak dikenal)\n", r1);
    return;
  }

  // Langkah 4: ACMD41 - minta kartu menyiapkan diri, ulangi sampai R1 = 0x00
  uint32_t argumen = kartuVersi2 ? 0x40000000 : 0;  // bit 30 (HCS) untuk kartu versi 2
  uint8_t crcAcmd41 = kartuVersi2 ? 0x77 : 0xE5;    // CRC yang cocok untuk argumen itu
  uint32_t mulai = millis();
  do {
    kirimPerintah(55, 0, 0x65, nullptr);            // CMD55: perintah berikutnya ACMD
    r1 = kirimPerintah(41, argumen, crcAcmd41, nullptr);
  } while (r1 == 0x01 && millis() - mulai < 3000);
  uint32_t lama = millis() - mulai;
  Serial.printf("ACMD41 (mulai menyiapkan kartu)  -> R1 = 0x%02X setelah %lu ms\n",
                r1, (unsigned long)lama);
  if (r1 != 0x00) {
    Serial.println("Kartu tidak selesai menyiapkan diri. Cabut USB sebentar atau coba kartu lain.");
    return;
  }

  // Langkah 5: CMD58 - baca register OCR (4 byte)
  uint8_t ocr[4] = {0, 0, 0, 0};
  r1 = kirimPerintah(58, 0, 0xFD, ocr);
  uint32_t nilaiOcr = ((uint32_t)ocr[0] << 24) | ((uint32_t)ocr[1] << 16) |
                      ((uint32_t)ocr[2] << 8) | ocr[3];
  Serial.printf("CMD58  (baca register OCR)       -> R1 = 0x%02X, OCR = 0x%08lX\n",
                r1, (unsigned long)nilaiOcr);
  if (r1 != 0x00) {
    Serial.println("Register OCR gagal dibaca.");
    return;
  }
  if (nilaiOcr & 0x40000000) {  // bit 30 = CCS
    Serial.println("       Bit 30 (CCS) = 1: kartu kapasitas tinggi (SDHC/SDXC).");
  } else {
    Serial.println("       Bit 30 (CCS) = 0: kartu kapasitas standar (SDSC).");
  }

  Serial.println("Selesai. Tekan tombol EN untuk mengulang.");
}

void loop() {
  // Semua perintah sudah dikirim sekali di setup().
}
