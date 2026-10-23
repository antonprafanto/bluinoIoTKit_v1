/*
 * BAB 25 - Program 2: Pemindai Bus 1-Wire
 * Mengirim reset pulse dan memeriksa presence pulse, mencari kode ROM
 * semua perangkat di bus dengan Search ROM, lalu memeriksa CRC-nya.
 * Program ini hanya memakai library OneWire (tingkat bawah).
 *
 * Rangkaian: kabel jumper dari pin DS (header hijau) ke IO14.
 */

#include <OneWire.h>

#define PIN_ONEWIRE 14   // jalur data DS18B20 (pin DS -> IO14)

OneWire oneWire(PIN_ONEWIRE);

// Menerjemahkan family code (byte 0 kode ROM) menjadi nama chip
const char *namaChip(uint8_t familyCode) {
  switch (familyCode) {
    case 0x28: return "DS18B20";
    case 0x10: return "DS18S20/DS1820";
    case 0x22: return "DS1822";
    case 0x3B: return "DS1825/MAX31850";
    default:   return "perangkat 1-Wire lain";
  }
}

// Mencetak 8 byte kode ROM dalam heksadesimal, misalnya 28:61:64:12:3C:7C:2F:27
void cetakKodeRom(const uint8_t rom[8]) {
  for (int i = 0; i < 8; i++) {
    Serial.printf("%02X", rom[i]);
    if (i < 7) {
      Serial.print(":");
    }
  }
}

// Mencetak kode ROM dalam bentuk baris kode yang siap disalin ke Program 4
void cetakBarisProgram4(const uint8_t rom[8], int nomor) {
  Serial.print("    { {");
  for (int i = 0; i < 8; i++) {
    Serial.printf("0x%02X", rom[i]);
    if (i < 7) {
      Serial.print(", ");
    }
  }
  Serial.printf("}, \"Sensor %d\" },\n", nomor);
}

void pindaiBus() {
  // Langkah 1: kirim reset pulse, lalu dengarkan presence pulse
  if (oneWire.reset() == 0) {
    Serial.println("Tidak ada presence pulse: tidak ada perangkat yang menjawab.");
    Serial.println("Periksa kabel DS -> 14.");
    return;
  }
  Serial.println("Presence pulse diterima: ada perangkat di bus.");

  // Langkah 2: Search ROM berulang-ulang sampai semua perangkat ditemukan
  uint8_t rom[8];
  int jumlah = 0;
  oneWire.reset_search();          // mulai pencarian dari awal
  while (oneWire.search(rom)) {    // true = satu kode ROM lagi ditemukan
    jumlah++;
    Serial.printf("Perangkat %d\n", jumlah);
    Serial.print("  Kode ROM    : ");
    cetakKodeRom(rom);
    Serial.println();
    Serial.printf("  Family code : 0x%02X (%s)\n", rom[0], namaChip(rom[0]));

    // Langkah 3: hitung ulang CRC dari 7 byte pertama, bandingkan dengan byte ke-8
    uint8_t crcHitung = OneWire::crc8(rom, 7);
    if (crcHitung == rom[7]) {
      Serial.printf("  CRC         : 0x%02X cocok, kode ROM utuh\n", rom[7]);
    } else {
      Serial.printf("  CRC         : 0x%02X, seharusnya 0x%02X -> data rusak!\n", rom[7], crcHitung);
    }

    Serial.println("  Baris untuk Program 4:");
    cetakBarisProgram4(rom, jumlah);
  }

  Serial.printf("Selesai: %d perangkat ditemukan.\n", jumlah);
  if (jumlah == 0) {
    Serial.println("Tidak ada kode ROM yang terbaca. Periksa kabel DS -> 14.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung

  Serial.println("BAB 25 - Program 2: Pemindai Bus 1-Wire");
  pindaiBus();
  Serial.println("Tekan tombol EN untuk memindai ulang.");
}

void loop() {
  // Pemindaian hanya dijalankan sekali di setup().
}
