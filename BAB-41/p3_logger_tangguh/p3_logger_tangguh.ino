/*
 * BAB 41 - Program 3: Data Logger Tangguh
 * - Membaca DHT11 setiap 10 detik; baris CSV ditampung di RAM (maks. 8 KB)
 *   lalu ditulis ke kartu setiap 60 detik
 * - File bernomor di folder /log: data_0001.csv, data_0002.csv, ...
 *   File baru dimulai setiap kali kartu dipasang, dan saat file aktif
 *   mencapai BATAS_UKURAN_FILE atau berumur BATAS_UMUR_FILE (rotasi)
 * - Tombol S1: lepas kartu dengan aman; tekan lagi untuk memasang kembali
 * - Kalau kartu hilang atau gagal ditulis: data tetap ditampung di RAM
 *   dan program mencoba memasang kartu lagi setiap 10 detik
 * - LED hijau menyala = kartu sedang dipakai (jangan dicabut)
 *   LED merah menyala = kartu tidak terbaca
 *   keduanya mati     = kartu sudah dilepas, aman dicabut
 *
 * Rangkaian: DHT -> IO27, SW1 -> IO15, LED1 (merah) -> IO4, LED2 (hijau) -> IO17.
 * microSD: CS = IO5, MOSI = IO23, CLK = IO18, MISO = IO19 (lewat jumper cap).
 */

#include <SPI.h>
#include <SD.h>
#include <DHT.h>
#include <esp_timer.h>

#define PIN_SCK   18  // CLK
#define PIN_MISO  19
#define PIN_MOSI  23
#define PIN_CS    5
#define PIN_DHT   27
#define TIPE_DHT  DHT11
#define TOMBOL_S1 15  // ditekan = HIGH (pull-down 10 kΩ di PCB)
#define LED_MERAH 4   // LED1
#define LED_HIJAU 17  // LED2

// Format CSV (lihat 41.5.3): false = koma + titik desimal, true = titik koma + koma desimal
const bool FORMAT_INDONESIA = false;

const uint32_t KECEPATAN_SPI = 4000000;  // 4 MHz
const char *FOLDER_LOG = "/log";
const char *JUDUL_KOLOM = "detik,suhu_C,kelembapan_persen,status";

// Aturan rotasi: file baru kalau salah satu batas ini tercapai
const size_t BATAS_UKURAN_FILE = 512UL * 1024UL;                    // 512 KB
const unsigned long BATAS_UMUR_FILE = 24UL * 60UL * 60UL * 1000UL;  // 24 jam dalam ms

const size_t BATAS_PENAMPUNG = 8 * 1024;  // maks. 8 KB data yang belum tersimpan
const unsigned long WAKTU_DEBOUNCE = 50;  // ms

// Tiga keadaan kartu (state machine, BAB 17)
enum StatusKartu { KARTU_TERPASANG, KARTU_DILEPAS, KARTU_BERMASALAH };

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

Timer timerBaca = {10000, 0};        // baca sensor setiap 10 detik
Timer timerTulis = {60000, 0};       // tulis ke kartu setiap 60 detik
Timer timerCobaPasang = {10000, 0};  // coba pasang ulang kartu setiap 10 detik

DHT dht(PIN_DHT, TIPE_DHT);
StatusKartu statusKartu = KARTU_DILEPAS;

String penampung = "";          // baris CSV yang belum tersimpan di kartu
int barisDitampung = 0;         // jumlah baris di penampung
unsigned long barisHilang = 0;  // baris yang terpaksa dibuang karena RAM penuh

char fileAktif[32] = "";          // contoh: /log/data_0007.csv
size_t ukuranFileAktif = 0;       // byte
unsigned long waktuMulaiFile = 0;  // millis() saat file aktif dibuat

// Variabel debounce tombol S1 (pola BAB 9)
int bacaanSebelumnya = LOW;
int statusStabil = LOW;
unsigned long waktuBerubah = 0;

// koma (pemisah kolom) -> titik koma, titik (desimal) -> koma
void sesuaikanFormat(char *baris) {
  if (!FORMAT_INDONESIA) {
    return;
  }
  for (int i = 0; baris[i] != '\0'; i++) {
    if (baris[i] == ',') {
      baris[i] = ';';
    } else if (baris[i] == '.') {
      baris[i] = ',';
    }
  }
}

// Menyalakan LED sesuai keadaan kartu
void aturLed() {
  digitalWrite(LED_HIJAU, statusKartu == KARTU_TERPASANG ? HIGH : LOW);
  digitalWrite(LED_MERAH, statusKartu == KARTU_BERMASALAH ? HIGH : LOW);
}

// Mencari nomor terbesar dari file data_XXXX.csv di folder /log
int cariNomorTerakhir() {
  int terbesar = 0;
  File folder = SD.open(FOLDER_LOG);
  if (!folder) {
    return 0;
  }
  File item = folder.openNextFile();
  while (item) {
    int nomor = 0;
    if (sscanf(item.name(), "data_%d", &nomor) == 1 && nomor > terbesar) {
      terbesar = nomor;
    }
    item.close();
    item = folder.openNextFile();
  }
  folder.close();
  return terbesar;
}

// Membuat file log baru bernomor berikutnya, lengkap dengan baris judul
bool mulaiFileBaru() {
  if (!SD.mkdir(FOLDER_LOG)) {  // true juga kalau folder sudah ada
    return false;
  }
  int nomor = cariNomorTerakhir() + 1;
  snprintf(fileAktif, sizeof(fileAktif), "%s/data_%04d.csv", FOLDER_LOG, nomor);
  while (SD.exists(fileAktif)) {  // jaga-jaga kalau ada file yang diganti nama di laptop
    nomor = nomor + 1;
    snprintf(fileAktif, sizeof(fileAktif), "%s/data_%04d.csv", FOLDER_LOG, nomor);
  }

  File f = SD.open(fileAktif, FILE_WRITE);
  if (!f) {
    return false;
  }
  char judul[64];
  snprintf(judul, sizeof(judul), "%s", JUDUL_KOLOM);
  sesuaikanFormat(judul);
  size_t ditulis = f.print(judul);
  ditulis = ditulis + f.print("\r\n");
  f.close();

  ukuranFileAktif = ditulis;
  waktuMulaiFile = millis();
  Serial.printf("[FILE] File baru: %s\n", fileAktif);
  return true;
}

// Memasang kartu lalu memulai file baru. Hasil: true kalau berhasil
bool pasangKartu() {
  if (!SD.begin(PIN_CS, SPI, KECEPATAN_SPI)) {
    return false;
  }
  if (!mulaiFileBaru()) {
    SD.end();
    return false;
  }
  statusKartu = KARTU_TERPASANG;
  aturLed();
  Serial.println("[KARTU] Terpasang. LED hijau menyala: jangan cabut kartu.");
  return true;
}

// Kartu tidak bisa dipakai: lepas, tampung data di RAM, coba lagi nanti
void kartuBermasalah(const char *alasan) {
  SD.end();
  statusKartu = KARTU_BERMASALAH;
  aturLed();
  timerCobaPasang.waktuLalu = millis();  // percobaan berikutnya 10 detik lagi
  Serial.printf("[KARTU] Bermasalah: %s. Data ditampung di RAM, dicoba lagi tiap 10 detik.\n",
                alasan);
}

// Menambahkan satu baris ke penampung RAM (dibuang kalau penampung penuh)
void tampung(const char *baris) {
  if (penampung.length() + strlen(baris) + 2 > BATAS_PENAMPUNG) {
    barisHilang = barisHilang + 1;
    Serial.printf("[RAM] Penampung penuh! Baris dibuang (total %lu baris hilang)\n", barisHilang);
    return;
  }
  penampung += baris;
  penampung += "\r\n";
  barisDitampung = barisDitampung + 1;
}

// Membaca DHT11 lalu menampung hasilnya sebagai satu baris CSV
void bacaSensor() {
  unsigned long long detik = esp_timer_get_time() / 1000000ULL;  // detik sejak menyala
  float suhu = dht.readTemperature();
  float lembap = dht.readHumidity();
  bool gagal = isnan(suhu) || isnan(lembap);

  char baris[48];
  if (gagal) {
    snprintf(baris, sizeof(baris), "%llu,,,gagal", detik);
  } else {
    snprintf(baris, sizeof(baris), "%llu,%.1f,%.0f,ok", detik, suhu, lembap);
  }
  sesuaikanFormat(baris);
  tampung(baris);

  if (gagal) {
    Serial.printf("[BACA] %llu s: gagal membaca DHT11 | di RAM: %d baris\n",
                  detik, barisDitampung);
  } else {
    Serial.printf("[BACA] %llu s: %.1f °C, %.0f %% | di RAM: %d baris\n",
                  detik, suhu, lembap, barisDitampung);
  }
}

// Menulis isi penampung ke file aktif (rotasi dulu kalau perlu)
void tulisPenampung() {
  if (statusKartu != KARTU_TERPASANG || barisDitampung == 0) {
    return;
  }

  // Rotasi: file aktif sudah terlalu besar atau terlalu lama?
  bool terlaluBesar = ukuranFileAktif >= BATAS_UKURAN_FILE;
  bool terlaluLama = millis() - waktuMulaiFile >= BATAS_UMUR_FILE;
  if (terlaluBesar || terlaluLama) {
    Serial.printf("[FILE] %s %s, ganti file\n", fileAktif,
                  terlaluBesar ? "mencapai batas ukuran" : "mencapai batas umur");
    if (!mulaiFileBaru()) {
      kartuBermasalah("gagal membuat file baru");
      return;
    }
  }

  File f = SD.open(fileAktif, FILE_APPEND);
  if (!f) {
    kartuBermasalah("file tidak bisa dibuka (kartu dicabut?)");
    return;
  }
  size_t ukuranAwal = f.size();
  f.print(penampung);
  f.flush();                      // paksa data dikirim ke kartu sekarang
  size_t ukuranAkhir = f.size();  // ukuran menurut kartu sesudah flush()
  f.close();

  if (ukuranAkhir != ukuranAwal + penampung.length()) {
    kartuBermasalah("data tidak tertulis utuh");
    return;  // penampung tidak dikosongkan: dicoba lagi setelah kartu terpasang
  }
  ukuranFileAktif = ukuranAkhir;
  Serial.printf("[TULIS] %d baris -> %s (%u byte)\n",
                barisDitampung, fileAktif, (unsigned)ukuranFileAktif);
  penampung = "";
  barisDitampung = 0;
}

// Mengembalikan true sekali saja setiap kali S1 baru ditekan (debounce BAB 9)
bool s1BaruDitekan() {
  int bacaan = digitalRead(TOMBOL_S1);
  if (bacaan != bacaanSebelumnya) {
    waktuBerubah = millis();
    bacaanSebelumnya = bacaan;
  }
  if (millis() - waktuBerubah > WAKTU_DEBOUNCE && bacaan != statusStabil) {
    statusStabil = bacaan;
    return statusStabil == HIGH;
  }
  return false;
}

// S1: lepas kartu kalau sedang terpasang, pasang kartu kalau sedang tidak terpasang
void tanganiTombol() {
  if (statusKartu == KARTU_TERPASANG) {
    Serial.println("[S1] Melepas kartu...");
    tulisPenampung();  // simpan dulu data yang masih di RAM
    if (statusKartu == KARTU_TERPASANG) {  // penulisan barusan bisa saja gagal
      SD.end();
      statusKartu = KARTU_DILEPAS;
      aturLed();
      Serial.println("[KARTU] Dilepas. Kartu aman dicabut. Data baru ditampung di RAM.");
      Serial.println("        Tekan S1 lagi untuk memasang kartu kembali.");
    }
  } else {
    Serial.println("[S1] Memasang kartu...");
    if (pasangKartu()) {
      tulisPenampung();  // langsung simpan data yang menunggu di RAM
    } else {
      kartuBermasalah("kartu tidak terbaca");
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // beri waktu Serial Monitor tersambung
  pinMode(TOMBOL_S1, INPUT);  // pull-down sudah ada di PCB
  pinMode(LED_MERAH, OUTPUT);
  pinMode(LED_HIJAU, OUTPUT);
  aturLed();
  dht.begin();
  penampung.reserve(BATAS_PENAMPUNG);  // siapkan 8 KB di RAM sejak awal
  Serial.println();
  Serial.println("BAB 41 - Program 3: Data Logger Tangguh");
  Serial.println("S1 = lepas/pasang kartu. Hijau = kartu dipakai, merah = kartu bermasalah.");

  SPI.begin(PIN_SCK, PIN_MISO, PIN_MOSI);  // urutan: SCK, MISO, MOSI
  if (!pasangKartu()) {
    kartuBermasalah("kartu tidak terbaca saat menyala");
  }
}

void loop() {
  unsigned long sekarang = millis();

  if (timerBaca.sudahWaktunya(sekarang)) {
    bacaSensor();
  }
  if (timerTulis.sudahWaktunya(sekarang)) {
    tulisPenampung();
  }
  if (statusKartu == KARTU_BERMASALAH && timerCobaPasang.sudahWaktunya(sekarang)) {
    Serial.println("[KARTU] Mencoba memasang kartu lagi...");
    if (pasangKartu()) {
      tulisPenampung();
    } else {
      Serial.println("[KARTU] Belum berhasil, dicoba lagi 10 detik lagi.");
    }
  }
  if (s1BaruDitekan()) {
    tanganiTombol();
  }
}
