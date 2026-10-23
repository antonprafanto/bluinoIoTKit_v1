/*
 * BAB 40 - Program 3: Pencatat Suhu ke File CSV
 * - Membaca DHT11 setiap 5 detik (hasilnya hanya ditampung di RAM)
 * - Setiap 60 detik, menyimpan RATA-RATA suhu dan kelembapan
 *   sebagai satu baris di /log/suhu.csv (hemat penulisan flash)
 * - Kalau /log/suhu.csv mencapai 64 KB, file diganti nama menjadi
 *   /log/suhu_lama.csv dan pencatatan dilanjutkan di file baru
 * Perintah Serial Monitor (line ending: New Line): lihat, info, hapus
 * Rangkaian: DHT (header hijau) -> IO27
 */

#include <LittleFS.h>
#include <DHT.h>

#define PIN_DHT 27
#define TIPE_DHT DHT11

const char *FOLDER_LOG = "/log";
const char *FILE_LOG = "/log/suhu.csv";
const char *FILE_LOG_LAMA = "/log/suhu_lama.csv";
const size_t UKURAN_MAKS_LOG = 64 * 1024;  // 64 KB per file
const unsigned int PANJANG_MAKS = 20;      // batas panjang satu perintah

// struct Timer dari BAB 16
struct Timer {
  unsigned long interval;   // ms
  unsigned long waktuLalu;  // hasil millis() saat terakhir dijalankan

  // Mengembalikan true kalau sudah waktunya, lalu mencatat waktunya
  bool sudahWaktunya(unsigned long sekarang) {
    if (sekarang - waktuLalu >= interval) {
      waktuLalu = sekarang;
      return true;
    }
    return false;
  }
};

Timer timerBaca = {5000, 0};     // baca sensor setiap 5 detik
Timer timerSimpan = {60000, 0};  // simpan ke file setiap 60 detik

DHT dht(PIN_DHT, TIPE_DHT);
bool fsSiap = false;
String penampung = "";  // karakter perintah yang sudah tiba

// Penampung untuk menghitung rata-rata (di RAM, bukan di flash)
float jumlahSuhu = 0;
float jumlahLembap = 0;
int jumlahSampel = 0;
unsigned long barisTersimpan = 0;  // baris yang disimpan sejak ESP32 menyala

size_t ukuranFile(const char *path) {
  File f = LittleFS.open(path, "r");
  if (!f) {
    return 0;
  }
  size_t ukuran = f.size();
  f.close();
  return ukuran;
}

// Membuat folder /log dan file CSV beserta baris judulnya kalau belum ada
void siapkanFileLog() {
  LittleFS.mkdir(FOLDER_LOG);
  if (!LittleFS.exists(FILE_LOG)) {
    File f = LittleFS.open(FILE_LOG, "w");
    if (f) {
      f.println("waktu_s,suhu_C,kelembapan_persen,jumlah_sampel");
      f.close();
      Serial.println("[FILE] /log/suhu.csv baru dibuat");
    }
  }
}

// Kalau file log sudah mencapai batas, jadikan file lama lalu mulai file baru
void putarFileLog() {
  if (ukuranFile(FILE_LOG) < UKURAN_MAKS_LOG) {
    return;
  }
  LittleFS.remove(FILE_LOG_LAMA);  // buang file lama sebelumnya (kalau ada)
  if (LittleFS.rename(FILE_LOG, FILE_LOG_LAMA)) {
    Serial.println("[FILE] suhu.csv mencapai batas -> diganti nama menjadi suhu_lama.csv");
  }
  siapkanFileLog();
}

void bacaSensor() {
  float suhu = dht.readTemperature();
  float lembap = dht.readHumidity();
  if (isnan(suhu) || isnan(lembap)) {
    Serial.println("[BACA] Gagal membaca DHT11 (cek kabel DHT -> 27)");
    return;
  }
  jumlahSuhu = jumlahSuhu + suhu;
  jumlahLembap = jumlahLembap + lembap;
  jumlahSampel = jumlahSampel + 1;
  Serial.printf("[BACA] %.1f °C | %.0f %% (sampel ke-%d)\n", suhu, lembap, jumlahSampel);
}

void simpanRataRata(unsigned long sekarang) {
  if (jumlahSampel == 0) {
    Serial.println("[SIMPAN] Dilewati: belum ada pembacaan yang berhasil");
    return;
  }
  float rataSuhu = jumlahSuhu / jumlahSampel;
  float rataLembap = jumlahLembap / jumlahSampel;

  if (!fsSiap) {
    Serial.println("[SIMPAN] Dilewati: LittleFS tidak siap");
  } else {
    putarFileLog();
    File f = LittleFS.open(FILE_LOG, "a");
    if (!f) {
      Serial.println("[SIMPAN] Gagal membuka /log/suhu.csv");
    } else {
      // satu baris CSV, diakhiri "\r\n" seperti baris judul
      f.printf("%lu,%.1f,%.0f,%d\r\n", sekarang / 1000, rataSuhu, rataLembap, jumlahSampel);
      f.close();
      barisTersimpan = barisTersimpan + 1;
      Serial.printf("[SIMPAN] Baris ke-%lu: %lu s, %.1f °C, %.0f %% (rata-rata %d sampel)\n",
                    barisTersimpan, sekarang / 1000, rataSuhu, rataLembap, jumlahSampel);
    }
  }

  // mulai mengumpulkan sampel untuk menit berikutnya
  jumlahSuhu = 0;
  jumlahLembap = 0;
  jumlahSampel = 0;
}

void tampilkanLog() {
  File f = LittleFS.open(FILE_LOG, "r");
  if (!f) {
    Serial.println("Belum ada /log/suhu.csv");
    return;
  }
  Serial.println("--- Isi /log/suhu.csv ---");
  while (f.available()) {
    Serial.write(f.read());
  }
  f.close();
  Serial.println("--- selesai ---");
}

void tampilkanInfo() {
  Serial.printf("/log/suhu.csv: %u byte (batas %u byte)\n",
                (unsigned)ukuranFile(FILE_LOG), (unsigned)UKURAN_MAKS_LOG);
  if (LittleFS.exists(FILE_LOG_LAMA)) {
    Serial.printf("/log/suhu_lama.csv: %u byte\n", (unsigned)ukuranFile(FILE_LOG_LAMA));
  }
  Serial.printf("Baris tersimpan sejak menyala: %lu\n", barisTersimpan);
  Serial.printf("LittleFS terpakai: %u dari %u byte\n",
                (unsigned)LittleFS.usedBytes(), (unsigned)LittleFS.totalBytes());
}

void hapusLog() {
  LittleFS.remove(FILE_LOG);
  LittleFS.remove(FILE_LOG_LAMA);
  Serial.println("Semua file log dihapus.");
  siapkanFileLog();
}

void prosesPerintah(String perintah) {
  perintah.trim();
  perintah.toLowerCase();
  if (perintah.length() == 0) {
    return;
  }
  Serial.print("> ");
  Serial.println(perintah);
  if (!fsSiap) {
    Serial.println("LittleFS tidak siap, perintah tidak bisa dijalankan.");
  } else if (perintah == "lihat") {
    tampilkanLog();
  } else if (perintah == "info") {
    tampilkanInfo();
  } else if (perintah == "hapus") {
    hapusLog();
  } else {
    Serial.println("Perintah tidak dikenal. Pilihan: lihat, info, hapus");
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  penampung.reserve(PANJANG_MAKS);
  dht.begin();
  Serial.println();
  Serial.println("BAB 40 - Program 3: Pencatat Suhu ke File CSV");

  fsSiap = LittleFS.begin(true);
  if (fsSiap) {
    siapkanFileLog();
    tampilkanInfo();
  } else {
    Serial.println("LittleFS gagal di-mount! Data hanya ditampilkan, tidak disimpan.");
  }
  Serial.println("Baca setiap 5 detik, simpan rata-rata setiap 60 detik.");
  Serial.println("Perintah: lihat, info, hapus (line ending: New Line)");
}

void loop() {
  unsigned long sekarang = millis();

  if (timerBaca.sudahWaktunya(sekarang)) {
    bacaSensor();
  }
  if (timerSimpan.sudahWaktunya(sekarang)) {
    simpanRataRata(sekarang);
  }

  // Perintah dari Serial Monitor, tanpa menunggu (pola BAB 16)
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
