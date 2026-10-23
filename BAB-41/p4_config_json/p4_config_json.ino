/*
 * BAB 41 - Program 4: File Pengaturan JSON di Kartu
 * - Membaca /config.json saat menyala. Kalau rusak, memakai cadangan
 *   /config.bak; kalau belum ada sama sekali, membuatnya dari nilai bawaan
 * - Pengaturan bisa diubah lewat Serial Monitor atau dengan mengedit
 *   /config.json di laptop
 * - Menyimpan dengan aman: tulis /config.tmp, periksa, lalu ganti nama
 * Perintah (line ending: New Line):
 *   lihat, set <kunci> <nilai>, simpan, muat, file, bantuan
 *
 * Rangkaian: tidak perlu kabel. Adaptor microSD tersambung ke
 * CS = IO5, MOSI = IO23, CLK = IO18, MISO = IO19 lewat jumper cap di kit.
 */

#include <SPI.h>
#include <SD.h>
#include <ArduinoJson.h>

#define PIN_SCK  18  // CLK
#define PIN_MISO 19
#define PIN_MOSI 23
#define PIN_CS   5

const uint32_t KECEPATAN_SPI = 4000000;  // 4 MHz
const char *FILE_CONFIG = "/config.json";
const char *FILE_SEMENTARA = "/config.tmp";
const char *FILE_CADANGAN = "/config.bak";
const size_t UKURAN_MAKS_CONFIG = 1024;  // file yang lebih besar dari ini dianggap rusak
const unsigned int PANJANG_MAKS = 64;    // batas panjang satu perintah

// Pengaturan untuk data logger
struct Pengaturan {
  String namaAlat;
  int intervalBacaS;    // detik
  int intervalSimpanS;  // detik
  bool formatIndonesia;
  int batasUkuranKB;    // KB per file sebelum ganti file (rotasi)
};

Pengaturan atur;
bool kartuSiap = false;
String penampungPerintah = "";  // karakter perintah yang sudah tiba

void aturBawaan() {
  atur.namaAlat = "Logger-01";
  atur.intervalBacaS = 10;
  atur.intervalSimpanS = 60;
  atur.formatIndonesia = false;
  atur.batasUkuranKB = 512;
}

// Memastikan semua nilai masuk akal
void rapikanNilai() {
  atur.namaAlat.trim();
  if (atur.namaAlat.length() == 0 || atur.namaAlat.length() > 31) {
    atur.namaAlat = "Logger-01";
  }
  atur.intervalBacaS = constrain(atur.intervalBacaS, 2, 3600);  // DHT11: minimal 2 detik
  atur.intervalSimpanS = constrain(atur.intervalSimpanS, 10, 86400);
  if (atur.intervalSimpanS < atur.intervalBacaS) {
    atur.intervalSimpanS = atur.intervalBacaS;
  }
  atur.batasUkuranKB = constrain(atur.batasUkuranKB, 16, 1048576);  // 16 KB sampai 1 GB
}

// Membaca satu file pengaturan. Hasil: true kalau file ada dan isinya JSON yang sah
bool bacaDariFile(const char *path) {
  File f = SD.open(path, FILE_READ);
  if (!f) {
    return false;
  }
  if (f.size() > UKURAN_MAKS_CONFIG) {
    Serial.printf("[CONFIG] %s terlalu besar (%u byte), dianggap rusak\n",
                  path, (unsigned)f.size());
    f.close();
    return false;
  }
  JsonDocument doc;
  DeserializationError hasil = deserializeJson(doc, f);
  f.close();
  if (hasil) {
    Serial.printf("[CONFIG] %s rusak: %s\n", path, hasil.c_str());
    return false;
  }

  // Ambil setiap nilai; kalau kuncinya tidak ada atau jenisnya salah, pakai nilai bawaan
  atur.namaAlat = doc["nama_alat"] | "Logger-01";
  atur.intervalBacaS = doc["interval_baca_s"] | 10;
  atur.intervalSimpanS = doc["interval_simpan_s"] | 60;
  atur.formatIndonesia = doc["format_indonesia"] | false;
  atur.batasUkuranKB = doc["batas_ukuran_kb"] | 512;
  rapikanNilai();
  return true;
}

// Menyimpan dengan aman: tulis ke file sementara, periksa, lalu ganti nama
bool simpanPengaturan() {
  JsonDocument doc;
  doc["nama_alat"] = atur.namaAlat;
  doc["interval_baca_s"] = atur.intervalBacaS;
  doc["interval_simpan_s"] = atur.intervalSimpanS;
  doc["format_indonesia"] = atur.formatIndonesia;
  doc["batas_ukuran_kb"] = atur.batasUkuranKB;

  // 1. Tulis ke file sementara
  File f = SD.open(FILE_SEMENTARA, FILE_WRITE);
  if (!f) {
    Serial.println("[CONFIG] Gagal membuat /config.tmp");
    return false;
  }
  serializeJsonPretty(doc, f);
  f.close();

  // 2. Periksa: file sementara bisa dibaca lagi dan isinya JSON yang sah?
  bool sah = false;
  f = SD.open(FILE_SEMENTARA, FILE_READ);
  if (f) {
    JsonDocument cek;
    sah = !deserializeJson(cek, f);
    f.close();
  }
  if (!sah) {
    Serial.println("[CONFIG] /config.tmp tidak sah, penyimpanan dibatalkan");
    SD.remove(FILE_SEMENTARA);
    return false;
  }

  // 3. Ganti nama. Di FAT, rename() gagal kalau nama tujuannya sudah ada,
  //    jadi config.json lama dijadikan cadangan config.bak lebih dulu.
  SD.remove(FILE_CADANGAN);
  if (SD.exists(FILE_CONFIG) && !SD.rename(FILE_CONFIG, FILE_CADANGAN)) {
    Serial.println("[CONFIG] Gagal membuat cadangan /config.bak");
    return false;
  }
  if (!SD.rename(FILE_SEMENTARA, FILE_CONFIG)) {
    Serial.println("[CONFIG] Gagal mengganti nama /config.tmp");
    return false;
  }
  Serial.println("[CONFIG] Tersimpan di /config.json");
  return true;
}

// Urutan memuat: config.json -> cadangan config.bak -> nilai bawaan
void muatPengaturan() {
  if (bacaDariFile(FILE_CONFIG)) {
    Serial.println("[CONFIG] Dimuat dari /config.json");
  } else if (bacaDariFile(FILE_CADANGAN)) {
    Serial.println("[CONFIG] Memakai cadangan /config.bak. Ketik 'simpan' untuk memperbaiki /config.json");
  } else if (SD.exists(FILE_CONFIG)) {
    aturBawaan();
    Serial.println("[CONFIG] /config.json rusak dan tidak ada cadangan. Memakai nilai bawaan.");
    Serial.println("         Perbaiki file-nya di laptop, atau ketik 'simpan' untuk menimpanya.");
  } else {
    aturBawaan();
    Serial.println("[CONFIG] Belum ada /config.json. Membuat file baru dari nilai bawaan.");
    simpanPengaturan();
  }
}

void tampilkanPengaturan() {
  Serial.println("Pengaturan aktif:");
  Serial.printf("  nama_alat         : %s\n", atur.namaAlat.c_str());
  Serial.printf("  interval_baca_s   : %d\n", atur.intervalBacaS);
  Serial.printf("  interval_simpan_s : %d\n", atur.intervalSimpanS);
  Serial.printf("  format_indonesia  : %s\n", atur.formatIndonesia ? "true" : "false");
  Serial.printf("  batas_ukuran_kb   : %d\n", atur.batasUkuranKB);
}

// Menampilkan isi /config.json apa adanya
void tampilkanFileConfig() {
  File f = SD.open(FILE_CONFIG, FILE_READ);
  if (!f) {
    Serial.println("Belum ada /config.json");
    return;
  }
  Serial.println("--- Isi /config.json ---");
  while (f.available()) {
    Serial.write(f.read());
  }
  f.close();
  Serial.println();  // serializeJsonPretty tidak menambahkan baris baru di akhir
  Serial.println("--- selesai ---");
}

void tampilkanBantuan() {
  Serial.println("Perintah (line ending: New Line):");
  Serial.println("  lihat               - tampilkan pengaturan aktif");
  Serial.println("  set <kunci> <nilai> - ubah pengaturan, contoh: set interval_baca_s 30");
  Serial.println("  simpan              - simpan pengaturan ke /config.json");
  Serial.println("  muat                - muat ulang dari kartu (batalkan perubahan)");
  Serial.println("  file                - tampilkan isi /config.json apa adanya");
  Serial.println("  bantuan             - tampilkan daftar ini");
}

// Mengubah satu pengaturan di RAM (belum disimpan ke kartu)
void ubahNilai(String kunci, String nilai) {
  if (kunci == "nama_alat") {
    atur.namaAlat = nilai;
  } else if (kunci == "interval_baca_s") {
    atur.intervalBacaS = nilai.toInt();
  } else if (kunci == "interval_simpan_s") {
    atur.intervalSimpanS = nilai.toInt();
  } else if (kunci == "format_indonesia") {
    atur.formatIndonesia = (nilai == "true" || nilai == "1");
  } else if (kunci == "batas_ukuran_kb") {
    atur.batasUkuranKB = nilai.toInt();
  } else {
    Serial.printf("Kunci '%s' tidak dikenal. Ketik 'lihat' untuk melihat nama-nama kunci.\n",
                  kunci.c_str());
    return;
  }
  rapikanNilai();
  Serial.println("Diubah (belum disimpan). Ketik 'simpan' untuk menyimpan ke kartu.");
  tampilkanPengaturan();
}

void prosesPerintah(String perintah) {
  perintah.trim();
  if (perintah.length() == 0) {
    return;
  }
  Serial.print("> ");
  Serial.println(perintah);
  if (!kartuSiap) {
    Serial.println("Kartu tidak siap, perintah tidak bisa dijalankan.");
  } else if (perintah == "lihat") {
    tampilkanPengaturan();
  } else if (perintah == "simpan") {
    simpanPengaturan();
  } else if (perintah == "muat") {
    muatPengaturan();
    tampilkanPengaturan();
  } else if (perintah == "file") {
    tampilkanFileConfig();
  } else if (perintah == "bantuan") {
    tampilkanBantuan();
  } else if (perintah.startsWith("set ")) {
    String sisa = perintah.substring(4);  // buang "set "
    sisa.trim();
    int spasi = sisa.indexOf(' ');
    if (spasi < 0) {
      Serial.println("Format: set <kunci> <nilai>");
      return;
    }
    String nilai = sisa.substring(spasi + 1);
    nilai.trim();
    ubahNilai(sisa.substring(0, spasi), nilai);
  } else {
    Serial.println("Perintah tidak dikenal. Ketik 'bantuan'.");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  penampungPerintah.reserve(PANJANG_MAKS);
  Serial.println();
  Serial.println("BAB 41 - Program 4: File Pengaturan JSON di Kartu");

  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);  // urutan: SCK, MISO, MOSI
  kartuSiap = SD.begin(PIN_CS, SPI, KECEPATAN_SPI);
  if (kartuSiap) {
    Serial.println("Kartu berhasil dipasang.");
    muatPengaturan();
  } else {
    aturBawaan();
    Serial.println("Kartu gagal dipasang! Memakai nilai bawaan.");
  }
  tampilkanPengaturan();
  Serial.println("Ketik 'bantuan' untuk melihat daftar perintah (line ending: New Line).");
}

void loop() {
  // Membaca perintah dari Serial Monitor tanpa menunggu (pola BAB 16)
  while (Serial.available() > 0) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      prosesPerintah(penampungPerintah);
      penampungPerintah = "";
    } else if (penampungPerintah.length() < PANJANG_MAKS) {
      penampungPerintah += c;
    }
  }
}
