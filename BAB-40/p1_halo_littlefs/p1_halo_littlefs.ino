/*
 * BAB 40 - Program 1: Halo LittleFS
 * Menulis, membaca, dan menambah isi file di flash ESP32.
 *   /halo.txt    -> ditulis ulang (mode "w") setiap ESP32 menyala
 *   /riwayat.txt -> ditambah satu baris (mode "a") setiap ESP32 menyala
 * Tekan tombol EN beberapa kali dan perhatikan /riwayat.txt bertambah.
 * Rangkaian: tidak perlu.
 */

#include <LittleFS.h>

const char *FILE_HALO = "/halo.txt";
const char *FILE_RIWAYAT = "/riwayat.txt";

// Menampilkan seluruh isi sebuah file ke Serial Monitor, baris demi baris
void tampilkanIsi(const char *path) {
  File f = LittleFS.open(path, "r");
  if (!f) {
    Serial.printf("Gagal membuka %s untuk dibaca\n", path);
    return;
  }
  Serial.printf("--- Isi %s (%u byte) ---\n", path, (unsigned)f.size());
  while (f.available()) {
    String baris = f.readStringUntil('\n');  // baca sampai akhir baris
    baris.trim();                            // buang '\r' di ujung baris
    Serial.println(baris);
  }
  f.close();
  Serial.println("---");
}

// Menghitung jumlah baris (jumlah karakter '\n') di sebuah file
int hitungBaris(const char *path) {
  File f = LittleFS.open(path, "r");
  if (!f) {
    return 0;  // file belum ada -> 0 baris
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

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  Serial.println();
  Serial.println("BAB 40 - Program 1: Halo LittleFS");

  // 1. Mount LittleFS (diformat otomatis pada pemakaian pertama)
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS gagal di-mount! Periksa Tools > Partition Scheme.");
    return;
  }
  Serial.println("LittleFS siap.");
  size_t total = LittleFS.totalBytes();
  size_t terpakai = LittleFS.usedBytes();
  Serial.printf("Kapasitas: %u byte | terpakai: %u byte | sisa: %u byte\n",
                (unsigned)total, (unsigned)terpakai, (unsigned)(total - terpakai));
  Serial.println();

  // 2. Mode "w": buat file baru, atau kosongkan isi lama, lalu tulis
  File f = LittleFS.open(FILE_HALO, "w");
  if (!f) {
    Serial.println("Gagal membuat /halo.txt");
    return;
  }
  f.println("Halo dari LittleFS!");
  f.println("File ini ditulis ulang setiap ESP32 menyala.");
  f.close();  // tutup: tulisan benar-benar tersimpan di flash
  tampilkanIsi(FILE_HALO);
  Serial.println();

  // 3. Mode "a": tambah satu baris di akhir file, isi lama tetap
  int nomor = hitungBaris(FILE_RIWAYAT) + 1;
  f = LittleFS.open(FILE_RIWAYAT, "a");
  if (!f) {
    Serial.println("Gagal membuka /riwayat.txt");
    return;
  }
  f.print("ESP32 menyala ke-");
  f.println(nomor);
  f.close();
  tampilkanIsi(FILE_RIWAYAT);
  Serial.println();
  Serial.println("Tekan tombol EN untuk menyalakan ulang ESP32.");
}

void loop() {
  // Tidak ada tugas berulang. Semua contoh dijalankan sekali di setup().
}
