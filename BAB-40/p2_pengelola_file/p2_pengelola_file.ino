/*
 * BAB 40 - Program 2: Pengelola File Lewat Serial Monitor
 * Ketik perintah di Serial Monitor (line ending: New Line):
 *   bantuan               -> daftar perintah
 *   info                  -> kapasitas dan sisa ruang
 *   ls                    -> semua file dan folder
 *   baca <path>           -> tampilkan isi file
 *   tulis <path> <teks>   -> buat/timpa file berisi <teks>
 *   tambah <path> <teks>  -> tambahkan <teks> sebagai baris baru
 *   hapus <path>          -> hapus file
 *   ganti <lama> <baru>   -> ganti nama/pindahkan file
 *   buatfolder <path>     -> buat folder
 *   hapusfolder <path>    -> hapus folder kosong
 *   format ya             -> hapus SEMUA file (memformat LittleFS)
 * Rangkaian: tidak perlu.
 */

#include <LittleFS.h>

const unsigned int PANJANG_MAKS = 120;  // batas panjang satu perintah

String penampung = "";  // karakter yang sudah tiba, menunggu akhir baris
int jumlahFile = 0;
int jumlahFolder = 0;

// Menambahkan '/' di depan path kalau pengguna lupa menuliskannya
String rapikanPath(String path) {
  path.trim();
  if (!path.startsWith("/")) {
    path = "/" + path;
  }
  return path;
}

// Memecah "kata1 sisanya" menjadi kata pertama dan sisanya
void pisahKataPertama(const String &teks, String &kata, String &sisa) {
  int spasi = teks.indexOf(' ');
  if (spasi < 0) {
    kata = teks;
    sisa = "";
  } else {
    kata = teks.substring(0, spasi);
    sisa = teks.substring(spasi + 1);
    sisa.trim();
  }
}

size_t ukuranFile(const String &path) {
  File f = LittleFS.open(path, "r");
  if (!f) {
    return 0;
  }
  size_t ukuran = f.size();
  f.close();
  return ukuran;
}

void tampilkanBantuan() {
  Serial.println("Perintah (path diawali /, tanpa spasi):");
  Serial.println("  info                  kapasitas dan sisa ruang");
  Serial.println("  ls                    semua file dan folder");
  Serial.println("  baca <path>           tampilkan isi file");
  Serial.println("  tulis <path> <teks>   buat/timpa file berisi teks");
  Serial.println("  tambah <path> <teks>  tambah teks sebagai baris baru");
  Serial.println("  hapus <path>          hapus file");
  Serial.println("  ganti <lama> <baru>   ganti nama/pindahkan file");
  Serial.println("  buatfolder <path>     buat folder");
  Serial.println("  hapusfolder <path>    hapus folder kosong");
  Serial.println("  format ya             hapus SEMUA file");
}

void tampilkanInfo() {
  size_t total = LittleFS.totalBytes();
  size_t terpakai = LittleFS.usedBytes();
  Serial.printf("Kapasitas: %u byte | terpakai: %u byte (%.1f%%) | sisa: %u byte\n",
                (unsigned)total, (unsigned)terpakai, 100.0 * terpakai / total,
                (unsigned)(total - terpakai));
}

// Menampilkan isi folder; memanggil dirinya sendiri untuk setiap subfolder
void tampilkanFolder(const String &pathFolder, int kedalaman) {
  File folder = LittleFS.open(pathFolder);
  if (!folder || !folder.isDirectory()) {
    Serial.printf("%s bukan folder\n", pathFolder.c_str());
    return;
  }
  File isi = folder.openNextFile();
  while (isi) {
    for (int i = 0; i < kedalaman; i++) {
      Serial.print("  ");  // menjorok sesuai kedalaman folder
    }
    if (isi.isDirectory()) {
      Serial.printf("[%s]\n", isi.name());
      jumlahFolder = jumlahFolder + 1;
      String pathSub = isi.path();  // salin path-nya sebelum ditutup
      isi.close();
      tampilkanFolder(pathSub, kedalaman + 1);
    } else {
      Serial.printf("%s  (%u byte)\n", isi.name(), (unsigned)isi.size());
      jumlahFile = jumlahFile + 1;
    }
    isi = folder.openNextFile();
  }
}

void perintahLs() {
  jumlahFile = 0;
  jumlahFolder = 0;
  Serial.println("/");
  tampilkanFolder("/", 1);
  Serial.printf("Jumlah: %d file, %d folder\n", jumlahFile, jumlahFolder);
  tampilkanInfo();
}

void perintahBaca(const String &path) {
  File f = LittleFS.open(path, "r");
  if (!f || f.isDirectory()) {
    Serial.printf("Tidak bisa membaca %s (tidak ada, atau itu folder)\n", path.c_str());
    return;
  }
  Serial.printf("--- Isi %s (%u byte) ---\n", path.c_str(), (unsigned)f.size());
  int terakhir = '\n';
  while (f.available()) {
    terakhir = f.read();
    Serial.write(terakhir);  // kirim apa adanya, byte demi byte
  }
  f.close();
  if (terakhir != '\n') {
    Serial.println();  // file tidak diakhiri baris baru
  }
  Serial.println("--- selesai ---");
}

// mode "w" = timpa isi lama, mode "a" = tambah di akhir
void perintahTulis(const String &argumen, const char *mode) {
  String path, teks;
  pisahKataPertama(argumen, path, teks);
  if (path.length() == 0 || teks.length() == 0) {
    Serial.println("Format: tulis <path> <teks>  atau  tambah <path> <teks>");
    return;
  }
  path = rapikanPath(path);
  File f = LittleFS.open(path, mode);
  if (!f) {
    Serial.printf("Gagal membuka %s (foldernya sudah dibuat?)\n", path.c_str());
    return;
  }
  f.println(teks);
  f.close();
  Serial.printf("OK: %s sekarang %u byte\n", path.c_str(), (unsigned)ukuranFile(path));
}

void perintahGanti(const String &argumen) {
  String lama, baru;
  pisahKataPertama(argumen, lama, baru);
  if (lama.length() == 0 || baru.length() == 0) {
    Serial.println("Format: ganti <path_lama> <path_baru>");
    return;
  }
  lama = rapikanPath(lama);
  baru = rapikanPath(baru);
  if (!LittleFS.exists(lama)) {
    Serial.printf("%s tidak ada\n", lama.c_str());
    return;
  }
  if (LittleFS.rename(lama, baru)) {
    Serial.printf("OK: %s -> %s\n", lama.c_str(), baru.c_str());
  } else {
    Serial.println("Gagal mengganti nama (foldernya sudah dibuat?)");
  }
}

void prosesPerintah(String baris) {
  baris.trim();
  if (baris.length() == 0) {
    return;  // baris kosong (misalnya dari line ending Both NL & CR): abaikan
  }
  Serial.print("> ");
  Serial.println(baris);

  String perintah, argumen;
  pisahKataPertama(baris, perintah, argumen);
  perintah.toLowerCase();  // hanya kata perintah yang dijadikan huruf kecil

  if (perintah == "bantuan") {
    tampilkanBantuan();
  } else if (perintah == "info") {
    tampilkanInfo();
  } else if (perintah == "ls") {
    perintahLs();
  } else if (perintah == "tulis") {
    perintahTulis(argumen, "w");
  } else if (perintah == "tambah") {
    perintahTulis(argumen, "a");
  } else if (perintah == "ganti") {
    perintahGanti(argumen);
  } else if (perintah == "format") {
    if (argumen == "ya") {
      Serial.println("Memformat LittleFS...");
      Serial.println(LittleFS.format() ? "OK: semua file sudah dihapus" : "Gagal memformat");
    } else {
      Serial.println("PERINGATAN: format menghapus SEMUA file. Ketik 'format ya' kalau yakin.");
    }
  } else if (perintah == "baca" || perintah == "hapus" ||
             perintah == "buatfolder" || perintah == "hapusfolder") {
    if (argumen.length() == 0) {
      Serial.printf("Format: %s <path>\n", perintah.c_str());
      return;
    }
    String path = rapikanPath(argumen);
    if (perintah == "baca") {
      perintahBaca(path);
    } else if (perintah == "hapus") {
      if (LittleFS.remove(path)) {
        Serial.printf("OK: %s dihapus\n", path.c_str());
      } else {
        Serial.printf("Gagal menghapus %s (tidak ada, atau itu folder?)\n", path.c_str());
      }
    } else if (perintah == "buatfolder") {
      if (LittleFS.mkdir(path)) {
        Serial.printf("OK: folder %s siap\n", path.c_str());
      } else {
        Serial.printf("Gagal membuat folder %s\n", path.c_str());
      }
    } else {
      if (LittleFS.rmdir(path)) {
        Serial.printf("OK: folder %s dihapus\n", path.c_str());
      } else {
        Serial.printf("Gagal menghapus folder %s (tidak ada, atau belum kosong)\n", path.c_str());
      }
    }
  } else {
    Serial.println("Perintah tidak dikenal. Ketik: bantuan");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  penampung.reserve(PANJANG_MAKS);
  Serial.println();
  Serial.println("BAB 40 - Program 2: Pengelola File Lewat Serial Monitor");
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS gagal di-mount! Periksa Tools > Partition Scheme.");
    while (true) {
      delay(1000);  // berhenti di sini; tidak ada yang bisa dikerjakan
    }
  }
  Serial.println("LittleFS siap. Ketik: bantuan (line ending: New Line)");
}

void loop() {
  // Ambil karakter yang SUDAH tiba saja, tanpa menunggu (pola BAB 16)
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      prosesPerintah(penampung);
      penampung = "";
    } else if (penampung.length() < PANJANG_MAKS) {
      penampung += c;
    }
  }
}
