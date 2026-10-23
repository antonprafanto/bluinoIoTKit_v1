/*
 * BAB 24 - Program 1: Info Kartu microSD
 * Memasang kartu microSD lewat bus SPI, lalu menampilkan jenis kartu,
 * kapasitas, ruang terpakai, dan isi folder utama kartu.
 * Program ini hanya MEMBACA kartu: tidak ada file yang ditulis atau dihapus.
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

const uint32_t KECEPATAN_SPI = 4000000;      // 4 MHz, sama dengan bawaan library SD
const uint64_t SATU_MB = 1024ULL * 1024ULL;  // 1 MB = 1.048.576 byte

// Menerjemahkan hasil SD.cardType() menjadi teks
const char *namaJenis(sdcard_type_t jenis) {
  switch (jenis) {
    case CARD_MMC:  return "MMC";
    case CARD_SD:   return "SDSC (kapasitas standar)";
    case CARD_SDHC: return "SDHC/SDXC (kapasitas tinggi)";
    case CARD_NONE: return "tidak ada kartu";
    default:        return "tidak dikenal";
  }
}

// Menampilkan daftar file dan folder di folder utama ("/") kartu
void tampilkanFolderUtama() {
  File folder = SD.open("/");
  if (!folder) {
    Serial.println("Folder utama tidak bisa dibuka.");
    return;
  }

  Serial.println("Isi folder utama kartu:");
  int jumlah = 0;
  File item = folder.openNextFile();  // ambil isi pertama
  while (item) {
    if (item.isDirectory()) {
      Serial.printf("  [FOLDER] %s\n", item.name());
    } else {
      Serial.printf("  [FILE]   %s (%u byte)\n", item.name(), (unsigned)item.size());
    }
    jumlah = jumlah + 1;
    item.close();
    item = folder.openNextFile();     // ambil isi berikutnya
  }
  folder.close();

  if (jumlah == 0) {
    Serial.println("  (kosong)");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  Serial.println("BAB 24 - Program 1: Info Kartu microSD");

  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);  // mulai bus SPI; urutan: SCK, MISO, MOSI

  // CS diatur sendiri oleh library SD, jadi pin CS diberikan di sini
  if (!SD.begin(PIN_CS, SPI, KECEPATAN_SPI)) {
    Serial.println("Kartu gagal dipasang!");
    Serial.println("Periksa kartu, format FAT32, dan jumper cap MISO/CLK/MOSI/CS.");
    return;
  }
  Serial.println("Kartu berhasil dipasang.");

  Serial.printf("Jenis kartu     : %s\n", namaJenis(SD.cardType()));
  Serial.printf("Jumlah sektor   : %u x %u byte\n",
                (unsigned)SD.numSectors(), (unsigned)SD.sectorSize());
  Serial.printf("Kapasitas kartu : %llu MB\n", SD.cardSize() / SATU_MB);
  Serial.printf("Ukuran partisi  : %llu MB\n", SD.totalBytes() / SATU_MB);
  Serial.printf("Terpakai        : %llu MB\n", SD.usedBytes() / SATU_MB);

  tampilkanFolderUtama();
  Serial.println("Selesai. Tekan tombol EN untuk mengulang.");
}

void loop() {
  // Semua pemeriksaan sudah dijalankan sekali di setup().
}
