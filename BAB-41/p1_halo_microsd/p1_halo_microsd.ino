/*
 * BAB 41 - Program 1: Halo microSD
 * - Menulis ulang /halo.txt (mode FILE_WRITE)
 * - Menambah satu baris ke /riwayat.txt setiap kali ESP32 menyala (FILE_APPEND)
 * - Membaca dan menampilkan kedua file (FILE_READ)
 * - Menampilkan isi folder utama kartu
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

const uint32_t KECEPATAN_SPI = 4000000;  // 4 MHz, sama dengan bawaan library SD

// Menghitung jumlah baris sebuah file (0 kalau file belum ada)
int hitungBaris(const char *path) {
  File f = SD.open(path, FILE_READ);
  if (!f) {
    return 0;
  }
  int jumlah = 0;
  while (f.available()) {
    if (f.read() == '\n') {
      jumlah = jumlah + 1;
    }
  }
  f.close();
  return jumlah;
}

// Menampilkan seluruh isi sebuah file ke Serial Monitor
void tampilkanFile(const char *path) {
  File f = SD.open(path, FILE_READ);
  if (!f) {
    Serial.printf("Gagal membuka %s\n", path);
    return;
  }
  Serial.printf("--- Isi %s (%u byte) ---\n", path, (unsigned)f.size());
  while (f.available()) {
    Serial.write(f.read());
  }
  f.close();
  Serial.println("--- selesai ---");
}

// Menampilkan daftar file dan folder di folder utama ("/") kartu
void tampilkanFolderUtama() {
  File folder = SD.open("/");
  if (!folder) {
    Serial.println("Folder utama tidak bisa dibuka.");
    return;
  }
  Serial.println("Isi folder utama kartu:");
  File item = folder.openNextFile();
  while (item) {
    if (item.isDirectory()) {
      Serial.printf("  [FOLDER] %s\n", item.name());
    } else {
      Serial.printf("  [FILE]   %s (%u byte)\n", item.name(), (unsigned)item.size());
    }
    item.close();
    item = folder.openNextFile();
  }
  folder.close();
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  Serial.println();
  Serial.println("BAB 41 - Program 1: Halo microSD");

  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);  // urutan: SCK, MISO, MOSI
  if (!SD.begin(PIN_CS, SPI, KECEPATAN_SPI)) {
    Serial.println("Kartu gagal dipasang!");
    Serial.println("Periksa kartu, format FAT32, dan jumper cap MISO/CLK/MOSI/CS.");
    return;
  }
  Serial.println("Kartu berhasil dipasang.");

  // 1. FILE_WRITE: isi lama dibuang, file ditulis dari awal
  File f = SD.open("/halo.txt", FILE_WRITE);
  if (f) {
    f.println("Halo dari ESP32!");
    f.println("File ini ditulis ulang setiap kali ESP32 menyala.");
    f.close();  // data benar-benar dikirim ke kartu saat close()
  } else {
    Serial.println("Gagal menulis /halo.txt");
  }

  // 2. FILE_APPEND: isi lama tetap, baris baru ditambahkan di akhir
  int nyalaKe = hitungBaris("/riwayat.txt") + 1;
  f = SD.open("/riwayat.txt", FILE_APPEND);
  if (f) {
    f.printf("ESP32 menyala ke-%d\r\n", nyalaKe);
    f.close();
  } else {
    Serial.println("Gagal menambah /riwayat.txt");
  }

  // 3. FILE_READ: baca dan tampilkan
  tampilkanFile("/halo.txt");
  tampilkanFile("/riwayat.txt");
  tampilkanFolderUtama();

  SD.end();  // lepas kartu: semua file sudah ditutup
  Serial.println("Selesai. Kartu sudah dilepas. Tekan EN untuk mengulang.");
}

void loop() {
  // Semua pekerjaan sudah dijalankan sekali di setup().
}
